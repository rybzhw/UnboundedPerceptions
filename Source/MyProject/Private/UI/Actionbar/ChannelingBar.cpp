// Fill out your copyright notice in the Description page of Project Settings.

#include "MyProject.h"
#include "ChannelingBar.h"
#include "UserInput.h"
#include "BasePlayer.h"
#include "Unit.h"
#include "UnitController.h"
#include "SpellSystem/MySpell.h"

void UChannelingBar::NativeOnInitialized()
{
   Super::NativeOnInitialized();
   controllerRef = GetOwningPlayer<AUserInput>();
}

FText UChannelingBar::GetChannelingName()
{
   AUnit* channelingUnit = controllerRef->GetBasePlayer()->GetFocusedUnit();
   if(IsValid(channelingUnit)) {
      const TSubclassOf<UMySpell> channeledSpellClass = channelingUnit->GetUnitController()->FindComponentByClass<USpellCastComponent>()->GetCurrentSpell();
      if(IsValid(channeledSpellClass)) { return channeledSpellClass.GetDefaultObject()->GetSpellName(); }
   }
   return FText();
}

float UChannelingBar::GetSpellChannelProgress()
{
   // The unit could die while we have the channelingbar visible so we need a null check
   AUnit* channelingUnit = controllerRef->GetBasePlayer()->GetFocusedUnit();
   if(IsValid(channelingUnit)) {
      // TODO: Update this via timer
      return channelingUnit->GetUnitController()->FindComponentByClass<USpellCastComponent>()->GetCurrentChannelingTime() /
             channelingUnit->GetUnitController()->FindComponentByClass<USpellCastComponent>()->GetCurrentSpell().GetDefaultObject()->GetCastTime(
                 channelingUnit->GetAbilitySystemComponent());
   }
   SetVisibility(ESlateVisibility::Hidden);
   return 0;
}

ESlateVisibility UChannelingBar::IsFocusedUnitChanneling()
{
   AUnit* channelingUnit = controllerRef->GetBasePlayer()->GetFocusedUnit();
   if(IsValid(channelingUnit) && (channelingUnit->GetState() == EUnitState::STATE_CHANNELING || channelingUnit->GetState() == EUnitState::STATE_INCANTATION))
      return ESlateVisibility::SelfHitTestInvisible;

   return ESlateVisibility::Hidden;
}
