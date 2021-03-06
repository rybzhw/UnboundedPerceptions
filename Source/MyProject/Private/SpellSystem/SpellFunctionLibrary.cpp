#include "MyProject.h"
#include "SpellFunctionLibrary.h"

#include "AbilitySystemComponent.h"
#include "GameplayAbility.h"
#include "GameplayTagContainer.h"
#include "GameplayEffect.h"

#include "RTSProjectile.h"

#include "MySpell.h"

#include "WorldObjects/Unit.h"
#include "UserInput.h"
#include "HUDManager.h"
#include "RTSProjectileStrategy.h"
#include "TextFormatter.h"
#include "Engine/CompositeCurveTable.h"

UCompositeCurveTable* USpellFunctionLibrary::effectPowerTableRef = nullptr;

USpellFunctionLibrary::USpellFunctionLibrary(const FObjectInitializer& o) : Super(o)
{
   const ConstructorHelpers::FObjectFinder<UCompositeCurveTable> skillTable(TEXT("/Game/RTS_Tutorial/Blueprints/SpellSystem/SpellEffect/CurveTables/Up_CT_AllSkills"));
   if(skillTable.Succeeded())
   {
      effectPowerTableRef = skillTable.Object;
      effectPowerTableRef->AddToRoot();
   }
}

FGameplayEffectSpecHandle USpellFunctionLibrary::MakeGameplayEffect(UGameplayAbility* AbilityRef, TSubclassOf<UGameplayEffect> EffectClass, int Level, float Duration,
                                                                    float Period, FGameplayTag Elem, FGameplayTag EffectName, FGameplayTagContainer AssetTags)
{
   FGameplayEffectSpecHandle effect           = AbilityRef->MakeOutgoingGameplayEffectSpec(EffectClass, Level);
   const UGameplayEffect*    effectDefinition = effect.Data->Def;

   effect.Data->DynamicGrantedTags.AddTag(Elem); // Use add tag to check if tag is valid and prevents duplicate tags.
   effect.Data->DynamicGrantedTags.AddTag(EffectName);
   effect.Data->DynamicGrantedTags.AppendTags(AssetTags);
   if(effectDefinition->DurationPolicy != EGameplayEffectDurationType::Instant && effectDefinition->DurationPolicy != EGameplayEffectDurationType::Infinite)
   {
      if(effectDefinition->Executions.Num())
      {
         // Spells that have executions and durations need to be periodic but we don't want the period to do anything so set it to something really large.
         if(effectDefinition->Period.Value <= 0 && Period <= 0)
         {
            Period = 999;
         }
      }

      if(Period > 0)
      {
         effect.Data->Period = Period;
      }

      if(Duration > 0)
      {
         effect.Data->SetDuration(Duration, true); // if we don't lock the duration, the duration will be recalcuated somewhere in active effect creation ...
      }
   }
   return effect;
}

FGameplayEffectSpecHandle USpellFunctionLibrary::MakeDamageOrHealingEffect(UGameplayAbility* AbilityRef, TSubclassOf<UGameplayEffect> EffectClass, int Level,
                                                                           float Duration, float Period, FGameplayTag Elem, FGameplayTag EffectName,
                                                                           FGameplayTagContainer assetTags, FDamageScalarStruct damageVals)
{
   FGameplayEffectSpecHandle effect = MakeGameplayEffect(AbilityRef, EffectClass, Level, Duration, Period, Elem, EffectName, assetTags);
   effect.Data->DynamicGrantedTags.AddTag(Elem); // Use add tag to check if tag is valid and prevents duplicate tags.
   effect.Data->DynamicGrantedTags.AddTag(EffectName);

   // no period since damage is instant application
   effect.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag("Combat.Stats.Health"), damageVals.hitpoints);
   effect.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag("Combat.Stats.Strength"), damageVals.strength);
   effect.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag("Combat.Stats.Intelligence"), damageVals.intelligence);
   effect.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag("Combat.Stats.Agility"), damageVals.agility);
   effect.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag("Combat.Stats.Understanding"), damageVals.understanding);
   return effect;
}

FGameplayEffectSpecHandle USpellFunctionLibrary::MakeStatChangeEffect(UGameplayAbility* AbilityRef, TSubclassOf<UGameplayEffect> EffectClass, int Level, float Duration,
                                                                      float Period, FGameplayTag Elem, FGameplayTag EffectName, FGameplayTagContainer assetTags,
                                                                      TArray<FStatChange> StatChanges = TArray<FStatChange>())
{
   FGameplayEffectSpecHandle effect = MakeGameplayEffect(AbilityRef, EffectClass, Level, Duration, Period, Elem, EffectName, assetTags);
   for(FStatChange statChange : StatChanges)
   {
      effect.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(*(FString("Combat.Stats.") + statChange.changedAtt.GetName())),
                                           statChange.changeStatMagnitude);
      // GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::White, *(FString("Combat.Stats.") + statChange.changedAtt.GetName()));
   }
   // statChanges = MoveTemp(StatChanges);
   return effect;
}

ARTSProjectile* USpellFunctionLibrary::SetupBulletTargetting(AUnit* casterRef, TSubclassOf<ARTSProjectile> bulletClass,
                                                             TSubclassOf<URTSProjectileStrategy>           projectileStrategyClass,
                                                             UPARAM(ref) TArray<FGameplayEffectSpecHandle> specHandles, bool canGoThroughWalls)
{
   FTransform spawnTransform = casterRef->GetActorTransform();
   spawnTransform.SetLocation(spawnTransform.GetLocation() + casterRef->GetActorForwardVector() * 10.f);

   URTSProjectileStrategy* projectileStrategy = nullptr;
   if(projectileStrategyClass)
   {
      projectileStrategy                    = NewObject<URTSProjectileStrategy>(casterRef, projectileStrategyClass);
      projectileStrategy->canGoThroughWalls = canGoThroughWalls;
   }
   else
   {
      projectileStrategy = URTSProjectileStrategy::StaticClass()->GetDefaultObject<URTSProjectileStrategy>();
   }
   projectileStrategy->defaultHitEffects = specHandles;

   ARTSProjectile* projectile =
       ARTSProjectile::MakeRTSProjectile(casterRef->GetWorld(), casterRef->GetTargetComponent(), casterRef->GetActorTransform(), bulletClass, projectileStrategy);

   return projectile;
}

FText USpellFunctionLibrary::ParseDesc(const FText& inputText, const UAbilitySystemComponent* compRef, const UMySpell* spell)
{
   FTextFormatPatternDefinitionRef descriptionPatternDef = MakeShared<FTextFormatPatternDefinition, ESPMode::ThreadSafe>();
   descriptionPatternDef->SetArgStartChar(TEXT('['));
   descriptionPatternDef->SetArgEndChar(TEXT(']'));

   const FString descriptionString = inputText.ToString();

   FFormatNamedArguments effectArgs;
   const FTextFormat     effectFormat = FTextFormat::FromString(descriptionString, descriptionPatternDef);

   if(FCString::Strchr(*descriptionString, descriptionPatternDef->ArgStartChar) != nullptr)
   {
      if(effectFormat.IsValid())
      {
         TArray<FString> effectFormatTokens;

         effectFormat.GetFormatArgumentNames(effectFormatTokens);

         for(const FString& token : effectFormatTokens)
         {
            if(effectPowerTableRef->GetRowMap().Contains(*token))
            {
               // The field might have less entries than the max level of the ability.
               FRealCurve* effectPowerCurve = effectPowerTableRef->GetRowMap()[*token];
               float       minTime, maxTime;
               effectPowerCurve->GetTimeRange(minTime, maxTime);

               int trueMaxTimeIndex = maxTime;
               for(int i = minTime; i < maxTime; ++i)
               {
                  if(FKeyHandle keyHandle = effectPowerCurve->FindKey(i); keyHandle != FKeyHandle::Invalid())
                  {
                     const float keyValue = effectPowerCurve->GetKeyValue(keyHandle);
                     if(keyValue == 0)
                     {
                        trueMaxTimeIndex = i - 1;
                        break;
                     }
                  }
               }

               const int index = UMySpell::GetIndex(spell->GetLevel(compRef), trueMaxTimeIndex, spell->GetMaxLevel());

               if(index > -1)
               {
                  const float effectTokenArgValue = effectPowerCurve->Eval(index + 1);
                  effectArgs.Add(*token, effectTokenArgValue);
               }
            }
         }
      }
   }

   FFormatNamedArguments args;

   static const TCHAR* strKey      = TEXT("str");
   static const TCHAR* intKey      = TEXT("int");
   static const TCHAR* agiKey      = TEXT("agi");
   static const TCHAR* undKey      = TEXT("und");
   static const TCHAR* hpKey       = TEXT("hit");
   static const TCHAR* aoeKey      = TEXT("aoe");
   static const TCHAR* durationKey = TEXT("dur");
   static const TCHAR* periodKey   = TEXT("per");

   args.Add(strKey, spell->GetDamage(compRef).strength);
   args.Add(intKey, spell->GetDamage(compRef).intelligence);
   args.Add(agiKey, spell->GetDamage(compRef).agility);
   args.Add(undKey, spell->GetDamage(compRef).understanding);
   args.Add(hpKey, spell->GetDamage(compRef).hitpoints);
   args.Add(aoeKey, spell->GetAOE(compRef));
   args.Add(durationKey, spell->GetSpellDuration(compRef));
   args.Add(periodKey, spell->GetPeriod(compRef));

   return FText::Format(FText::FromString(FTextFormatter::FormatStr(effectFormat, effectArgs, false, true)), args);
}

void USpellFunctionLibrary::SpellConfirmSwap(TSubclassOf<UMySpell> confirmSpell, TSubclassOf<UMySpell> originalSpell, AUnit* ownerRef, bool bSwapInConfirm)
{
   TSubclassOf<UMySpell> spellToReplace, replacementSpell;
   if(bSwapInConfirm)
   {
      spellToReplace   = originalSpell;
      replacementSpell = confirmSpell;
   }
   else
   {
      spellToReplace   = confirmSpell;
      replacementSpell = originalSpell;
   }

   SpellSwap(spellToReplace, replacementSpell, ownerRef);
}

UObject* USpellFunctionLibrary::GetDefaultObjectFromSpellClass(TSubclassOf<UMySpell> spellClass)
{
   return spellClass.GetDefaultObject();
}

void USpellFunctionLibrary::SpellSwap(TSubclassOf<UMySpell> originalSpell, TSubclassOf<UMySpell> newSpell, AUnit* ownerRef)
{
   const int slot = ownerRef->GetAbilitySystemComponent()->GetAbilities().Find(originalSpell);

   if(slot == INDEX_NONE)
   {
      UE_LOG(LogTemp, Error, TEXT("Cannot find original spell to swap with"));
      return;
   }

   ownerRef->GetAbilitySystemComponent()->SetSpellAtSlot(newSpell, slot);
}
