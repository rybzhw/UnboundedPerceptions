// Fill out your copyright notice in the Description page of Project Settings.

#include "MyProject.h"
#include "MySpell.h"
#include "GameplayEffects/CoolDownEffect.h"
#include "GameplayEffects/RTSDamageEffect.h"
#include "SpellDataManager.h"
#include "WorldObjects/Unit.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemGlobals.h"
#include "UpStatComponent.h"
#include "DA_DefaultTargetingScheme.h"
#include "GlobalDataAssetsSS.h"
#include "SpellFunctionLibrary.h"
#include "TargetComponent.h"
#include "SpellTargetingTypes.h"

UMySpell::UMySpell() : UGameplayAbility()
{
   CooldownGameplayEffectClass = UCoolDownEffect::StaticClass();
}

TSubclassOf<UUpSpellTargeting> UMySpell::GetTargeting() const
{
   return spellDefaults.Targeting;
}

bool UMySpell::CheckCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, OUT FGameplayTagContainer* OptionalRelevantTags) const
{
   const FGameplayTagContainer* CooldownTags = GetCooldownTags();
   if(CooldownTags)
   {
      check(ActorInfo->AbilitySystemComponent.IsValid());

      // CooldownTags->Num() > 0 && ActorInfo->AbilitySystemComponent->HasAnyMatchingGameplayTags(*CooldownTags) ||
      if(ActorInfo->AbilitySystemComponent->HasMatchingGameplayTag(AbilityTags.GetByIndex(0)))
      {
         const FGameplayTag& CooldownTag = UAbilitySystemGlobals::Get().ActivateFailCooldownTag;

         if(OptionalRelevantTags && CooldownTag.IsValid())
         {
            OptionalRelevantTags->AddTag(CooldownTag);
         }
         // GEngine->AddOnScreenDebugMessage(-1, 5, FColor::Red, "Cooldown checked");
         return false;
      }
   }
   return true;
}

FGameplayEffectContextHandle UMySpell::MakeEffectContext(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo) const
{
   check(ActorInfo);
   FGameplayEffectContextHandle Context = FGameplayEffectContextHandle(UAbilitySystemGlobals::Get().AllocGameplayEffectContext());
   Context.AddInstigator(ActorInfo->AvatarActor.Get(), ActorInfo->AvatarActor.Get());
   Context.SetAbility(this);
   return Context;
}

void UMySpell::CommitExecute(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
   // Our cooldown effects are all the same but we manually add the name tag of this spell to know it is on cooldown.
   FGameplayEffectSpecHandle sH = MakeOutgoingGameplayEffectSpec(CooldownGameplayEffectClass, 1);
   UAbilitySystemBlueprintLibrary::SetDuration(sH, GetCDDuration(ActorInfo->AbilitySystemComponent.Get()));
   UAbilitySystemBlueprintLibrary::AddGrantedTag(sH, AbilityTags.GetByIndex(0));

   K2_ApplyGameplayEffectSpecToOwner(sH);

   if(AUnit* unitRef = GetAvatarUnit())
   {
      unitRef->GetStatComponent()->ModifyStats<false>(unitRef->GetStatComponent()->GetVitalCurValue(EVitals::Mana) - GetCost(unitRef->GetAbilitySystemComponent()),
                                                      EVitals::Mana);
   }
}

float UMySpell::GetCooldownTimeRemaining(const FGameplayAbilityActorInfo* ActorInfo) const
{
   if(ActorInfo->AbilitySystemComponent.IsValid())
   {
      const FGameplayTagContainer c         = FGameplayTagContainer(spellDefaults.nameTag);
      FGameplayEffectQuery const  Query     = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(c);
      TArray<float>               Durations = ActorInfo->AbilitySystemComponent->GetActiveEffectsTimeRemaining(Query);
      if(Durations.Num() > 0) return Durations[Durations.Num() - 1];
   }

   return 0.f;
}

bool UMySpell::IsOnCD(const UAbilitySystemComponent* abilityComponent) const
{
   if(abilityComponent->HasMatchingGameplayTag(AbilityTags.GetByIndex(0))) return true;
   return false;
}

float UMySpell::GetCDDuration(UAbilitySystemComponent* abilityComponent) const
{
   return GetSpellDefaultValueChecked(abilityComponent, spellDefaults.Cooldown);
}

int UMySpell::GetRange(UAbilitySystemComponent* abilityComponent) const
{
   return GetSpellDefaultValueChecked(abilityComponent, spellDefaults.Range);
}

int UMySpell::GetReqLevel(UAbilitySystemComponent* abilityComponent) const
{
   return GetSpellDefaultValueChecked(abilityComponent, spellDefaults.LevelReq);
}

int UMySpell::GetCost(const UAbilitySystemComponent* abilityComponent) const
{
   return GetSpellDefaultValueChecked(abilityComponent, spellDefaults.Cost);
}

TArray<FText> UMySpell::GetPreReqNames() const
{
   TArray<FText> preReqNames;
   for(TSubclassOf<UMySpell> preReqSpell : spellDefaults.PreReqs)
   {
      preReqNames.Add(preReqSpell.GetDefaultObject()->GetSpellDefaults().Name);
   }
   return preReqNames;
}

float UMySpell::GetSpellDuration(UAbilitySystemComponent* abilityComponent) const
{
   return GetSpellDefaultValueChecked(abilityComponent, spellDefaults.Duration);
}

FDamageScalarStruct UMySpell::GetDamage(UAbilitySystemComponent* abilityComponent) const
{
   return GetSpellDefaultValueChecked(abilityComponent, spellDefaults.Damage);
}

float UMySpell::GetPeriod(UAbilitySystemComponent* abilityComponent) const
{
   return GetSpellDefaultValueChecked(abilityComponent, spellDefaults.Period);
}

float UMySpell::GetCastTime(UAbilitySystemComponent* abilityComponent) const
{
   return GetSpellDefaultValueChecked(abilityComponent, spellDefaults.CastTime);
}

float UMySpell::GetSecondaryTime(UAbilitySystemComponent* abilityComponent) const
{
   return GetSpellDefaultValueChecked(abilityComponent, spellDefaults.SecondaryTime);
}

float UMySpell::GetAOE(UAbilitySystemComponent* abilityComponent) const
{
   return GetSpellDefaultValueChecked(abilityComponent, spellDefaults.AOE);
}

int UMySpell::GetLevel(UAbilitySystemComponent* abilityComponent) const
{
  FGameplayAbilitySpec* spec = GetAbilitySpec(abilityComponent);
  if(spec)
  {
     return spec->Level;
  }
  return 0;
}

FGameplayEffectSpecHandle UMySpell::SetScaling(FGameplayEffectSpecHandle specHandle)
{
   const FDamageScalarStruct damageScalings = GetDamage(UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetAvatarActorFromActorInfo()));
   specHandle =
       UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(specHandle, FGameplayTag::RequestGameplayTag("Combat.Stats.Strength"), damageScalings.strength);
   specHandle = UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(specHandle, FGameplayTag::RequestGameplayTag("Combat.Stats.Intelligence"),
                                                                              damageScalings.intelligence);
   specHandle =
       UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(specHandle, FGameplayTag::RequestGameplayTag("Combat.Stats.Agility"), damageScalings.agility);
   specHandle = UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(specHandle, FGameplayTag::RequestGameplayTag("Combat.Stats.Understanding"),
                                                                              damageScalings.understanding);
   return specHandle;
}

FGameplayEffectSpecHandle UMySpell::SetPeriod(FGameplayEffectSpecHandle specHandle, float period)
{
   FGameplayEffectSpec* Spec = specHandle.Data.Get();
   if(Spec)
   {
      Spec->Period = period;
   }
   else
   {
      ABILITY_LOG(Warning, TEXT("UMySpell::SetPeriod called with invalid SpecHandle"));
   }
   return specHandle;
}

FGameplayEffectSpecHandle UMySpell::CreateGameplayEffectFromTableValues(TSubclassOf<UGameplayEffect> effectClass, FGameplayTag name, FGameplayTagContainer assetTags)
{
   UAbilitySystemComponent*  abilityComp = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetActorInfo().AvatarActor.Get());
   FGameplayEffectSpecHandle effect      = MakeOutgoingGameplayEffectSpec(effectClass, GetLevel(abilityComp));

   effect.Data->DynamicAssetTags.AddTag(GetElemTag());
   effect.Data->DynamicAssetTags.AddTag(name);
   effect.Data->DynamicAssetTags.AppendTags(assetTags);
   if(effect.Data->Def->DurationPolicy != EGameplayEffectDurationType::Instant)
   {
      effect.Data->Period = GetPeriod(abilityComp);
      // If we don't lock the duration, the duration will be recalcuated somewhere in active effect creation ...
      effect.Data->SetDuration(GetSpellDuration(abilityComp), true);
   }

   if(effectClass->IsChildOf(URTSDamageEffect::StaticClass())) SetScaling(effect);
   return effect;
}

UEnvQuery* UMySpell::GetDefaultQueryForSpell(UWorld* worldRef)
{
   // ! Not making classes for these since most spell cast usage will have custom targeting anyways
   if(const auto dataAssetSS = worldRef->GetGameInstance()->GetSubsystem<UGlobalDataAssetsSS>())
   {
      if(spellDefaults.descriptionTags.HasTag(FGameplayTag::RequestGameplayTag("Skill.Category.Offensive")))
      {
         return GetTargeting().GetDefaultObject()->GetDefaultQueryForTargetingScheme(dataAssetSS->defaultOffensiveTargetSchemes);
      }
      if(spellDefaults.descriptionTags.HasTag(FGameplayTag::RequestGameplayTag("Skill.Category.Support")))
      {
         return GetTargeting().GetDefaultObject()->GetDefaultQueryForTargetingScheme(dataAssetSS->defaultSupportTargetSchemes);
      }
      if(spellDefaults.descriptionTags.HasTag(FGameplayTag::RequestGameplayTag("Skill.Category.Purge")))
      {
         return GetTargeting().GetDefaultObject()->GetDefaultQueryForTargetingScheme(dataAssetSS->defaultPurgeTargetSchemes);
      }
      if(spellDefaults.descriptionTags.HasTag(FGameplayTag::RequestGameplayTag("Skill.Category.Disable")))
      {
         return GetTargeting().GetDefaultObject()->GetDefaultQueryForTargetingScheme(dataAssetSS->defaultDisableTargetSchemes);
      }
   }
   return nullptr;
}

UAbilitySystemComponent* UMySpell::GetOwnerAbilitySystemComponent() const
{
   return UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwningActorFromActorInfo());
}

UAbilitySystemComponent* UMySpell::GetAvatarAbilitySystemComponent() const
{
   return UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetAvatarActorFromActorInfo());
}

AUnit* UMySpell::GetAvatarUnit() const
{
   return Cast<AUnit>(GetAvatarActorFromActorInfo());
}

FGameplayAbilityTargetingLocationInfo UMySpell::MakeTargetLocationInfoFromTargetComponent()
{
   FGameplayAbilityTargetingLocationInfo targetLocInfo;
   targetLocInfo.LiteralTransform = FTransform(GetAvatarTargetLocation());
   targetLocInfo.SourceAbility    = this;
   targetLocInfo.SourceActor      = GetAvatarActorFromActorInfo();
   return targetLocInfo;
}

FGameplayAbilityTargetDataHandle UMySpell::GetAvatarTargetData() const
{
   return GetAvatarUnit()->GetTargetComponent()->GetTargetData();
}

FVector UMySpell::GetAvatarTargetLocation() const
{
   return GetAvatarUnit()->GetTargetComponent()->GetTargetLocationVisit();
}

TArray<TEnumAsByte<ECollisionChannel>> UMySpell::GetTraceChannelForEnemy() const
{
   AUnit* unit = GetAvatarUnit();
   if(unit->GetIsEnemy())
   {
      return {FRIENDLY_CHANNEL};
   }
   else
   {
      return {ENEMY_CHANNEL};
   }
}

TArray<TEnumAsByte<ECollisionChannel>> UMySpell::GetTraceChannelForFriendly() const
{
   AUnit* unit = GetAvatarUnit();
   if(unit->GetIsEnemy())
   {
      return {ENEMY_CHANNEL};
   }
   else
   {
      return {FRIENDLY_CHANNEL};
   }
}

FGameplayAbilitySpec* UMySpell::GetAbilitySpec(const UAbilitySystemComponent* abilityComponent) const
{
   FGameplayAbilitySpec* abilitySpec = const_cast<UAbilitySystemComponent*>(abilityComponent)->FindAbilitySpecFromClass(GetClass());
   ensureMsgf(abilitySpec,  TEXT("Ability hasn't been registered to unit"));
   return abilitySpec;
}

int UMySpell::GetIndex(int currentLevel, int numCategories, int maxLevel)
{
   const int denom = maxLevel * currentLevel;
   if(numCategories <= 0)
   {
      UE_LOG(LogTemp, Error, TEXT("Attempting to access some out of bounds spell property!"));
   }
   if(denom != 0)
   {
      return FMath::CeilToInt(static_cast<float>(numCategories) / denom) - 1;
   }
   return -1;
}
