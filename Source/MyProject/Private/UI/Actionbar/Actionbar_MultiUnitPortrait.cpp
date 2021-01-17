﻿// Created 1/15/21 9:05 PM

#include "MyProject.h"
#include "Actionbar_MultiUnitPortrait.h"
#include "Ally.h"
#include "BasePlayer.h"
#include "UnitSelectionSlot.h"
#include "UserInput.h"

void UActionbar_MultiUnitPortrait::RefreshDisplayedUnitImages()
{
   if(AUserInput* PC = GetOwningPlayer<AUserInput>())
   {
      const TArray<AAlly*>& allies         = PC->GetBasePlayer()->allies;
      const int             numSlotsToShow = FMath::Min(allies.Num(), unitSlots.Num());
      for(int i = 0; i < numSlotsToShow; ++i)
      {
         unitSlots[i]->SetUnitInformation(allies[i]);
         unitSlots[i]->SetVisibility(ESlateVisibility::Visible);
      }
      for(int i = numSlotsToShow; i < unitSlots.Num(); ++i)
      {
         unitSlots[i]->SetVisibility(ESlateVisibility::Collapsed);
      }
   }
}
