// Fill out your copyright notice in the Description page of Project Settings.

#include "MyProject.h"
#include "MyAttributeSet.h"
#include "GameplayEffect.h"
#include "WorldObjects/Unit.h"
#include "GameplayEffectExtension.h"
#include "AbilitySystemComponent.h"
#include "NumericLimits.h"
#include "UpStatComponent.h"

TArray<FGameplayAttribute> UMyAttributeSet::attList = TArray<FGameplayAttribute>();
TSet<FGameplayAttribute>   UMyAttributeSet::attSet  = TSet<FGameplayAttribute>();

UMyAttributeSet::UMyAttributeSet()
{
   attList = {GetStrengthAttribute(),  GetUnderstandingAttribute(), GetIntelligenceAttribute(), GetExplosivenessAttribute(),
              GetEnduranceAttribute(), GetAgilityAttribute(),       GetLuckAttribute()};
   attSet  = TSet<FGameplayAttribute>(attList);
}

void UMyAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& data)
{
   UAbilitySystemComponent* source = data.EffectSpec.GetContext().GetOriginalInstigatorAbilitySystemComponent();

   // This logic is already handled by the damage component
   /*if (HealthAttribute() == data.EvaluatedData.Attribute) {
      AActor*      damagedActor      = nullptr;
      AController* damagedController = nullptr;
      if (data.Target.AbilityActorInfo.IsValid() && data.Target.AbilityActorInfo->AvatarActor.IsValid()) { damagedActor = data.Target.AbilityActorInfo->AvatarActor.Get(); }
      AActor* AttackingActor = nullptr;
      if (source && source->AbilityActorInfo.IsValid() && source->AbilityActorInfo->AvatarActor.IsValid()) { AttackingActor = source->AbilityActorInfo->AvatarActor.Get(); }

      Health.SetCurrentValue(FMath::Clamp(Health.GetCurrentValue(), 0.0f, MAX_HEALTH));
      if (Health.GetCurrentValue() <= 0) {
         AUnit* unit = Cast<AUnit>(damagedActor);
         unit->Die();
      }
   }*/
}

TArray<FGameplayAttribute> UMyAttributeSet::GetAtts()
{
   TArray<FGameplayAttribute> atts = {GetStrengthAttribute(),  GetUnderstandingAttribute(), GetIntelligenceAttribute(), GetExplosivenessAttribute(),
                                      GetEnduranceAttribute(), GetAgilityAttribute(),       GetLuckAttribute()};
   return atts;
}

TArray<FGameplayAttribute> UMyAttributeSet::GetSkills()
{
   TArray<FGameplayAttribute> skills = {
       GetCritical_ChanceAttribute(), GetCritical_DamageAttribute(), GetAccuracyAttribute(),     GetDodgeAttribute(),         GetAttack_SpeedAttribute(), GetCast_SpeedAttribute(),
       GetPhysical_AffAttribute(),    GetFire_AffAttribute(),        GetWater_AffAttribute(),    GetWind_AffAttribute(),      GetEarth_AffAttribute(),    GetElectric_AffAttribute(),
       GetDarkness_AffAttribute(),    GetLight_AffAttribute(),       GetArcane_AffAttribute(),   GetChaos_AffAttribute(),     GetPoison_AffAttribute(),   GetBlood_AffAttribute(),
       GetEthereal_AffAttribute(),    GetPhysical_ResistAttribute(), GetFire_ResistAttribute(),  GetWater_ResistAttribute(),  GetWind_ResistAttribute(),  GetEarth_ResistAttribute(),
       GetElectric_ResistAttribute(), GetDarkness_ResistAttribute(), GetLight_ResistAttribute(), GetArcane_ResistAttribute(), GetChaos_ResistAttribute(), GetPoison_ResistAttribute(),
       GetBlood_ResistAttribute(),    GetEthereal_ResistAttribute()};
   return skills;
}

TArray<FGameplayAttribute> UMyAttributeSet::GetVitals()
{
   TArray<FGameplayAttribute> vits = {GetHealthAttribute(), GetManaAttribute(), GetPsycheAttribute(), GetMoxieAttribute(), GetShieldAttribute()};
   return vits;
}

TArray<FGameplayAttribute> UMyAttributeSet::GetMechanics()
{
   TArray<FGameplayAttribute> mechs = {GetMovementSpeedAttribute(), GetAttackRangeAttribute(), GetWeaponPowerAttribute(), GetGlobalDamageModifierAttribute()};
   return mechs;
}

FGameplayAttribute UMyAttributeSet::IndexAtts(int index)
{
   return attList[index];
}

void UMyAttributeSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
   AUnit* unit = Cast<AUnit>(GetOwningActor());
   if(attSet.Contains(Attribute)) { unit->GetStatComponent()->UpdateStats(Attribute); }

   baseStatUpdatedEvent.Broadcast(Attribute, NewValue, unit);
}

void UMyAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
   AUnit* unit = Cast<AUnit>(GetOwningActor());
   if(Attribute == GetHealthAttribute()) { // Health clamping
      Health.SetCurrentValue(FMath::Clamp(Health.GetCurrentValue(), 0.0f, Health.GetBaseValue()));
   } else if(attSet.Contains(Attribute)) { // Needs to be run before the broadcast to get proper values to the stat widget in the character menu
      unit->GetStatComponent()->UpdateStats(Attribute);
   }

   statUpdatedEvent.Broadcast(Attribute, NewValue, unit);
}
