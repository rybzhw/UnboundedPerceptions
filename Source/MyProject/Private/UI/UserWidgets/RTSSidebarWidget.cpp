// Fill out your copyright notice in the Description page of Project Settings.

#include "MyProject.h"
#include "UserWidgets/RTSSidebarWidget.h"

#include "AbilitySystemComponent.h"
#include "BaseHero.h"
#include "BasePlayer.h"
#include "BluJsonObj.h"
#include "GameplayEffectExtension.h"
#include "Image.h"
#include "PartyDelegateContext.h"
#include "RTSPawn.h"
#include "UpStatComponent.h"
#include "UserInput.h"
#include "Blu/Public/BluEye.h"
#include "Blu/Public/BluBlueprintFunctionLibrary.h"

void URTSSidebarWidget::NativeOnInitialized()
{
   Super::NativeOnInitialized();
   cpcRef = GetOwningPlayer<AUserInput>();
   if(cpcRef)
   {
      if(ABasePlayer* BasePlayer = cpcRef->GetBasePlayer())
      {
         BasePlayer->partyUpdatedEvent.AddUObject(this, &URTSSidebarWidget::UpdatePartyInformation);
      }
      cpcRef->GetLocalPlayer()->GetSubsystem<UPartyDelegateContext>()->OnHeroSelectedDelegate.AddDynamic(this, &URTSSidebarWidget::UpdateSingleHeroSelect);
      cpcRef->GetLocalPlayer()->GetSubsystem<UPartyDelegateContext>()->OnAllySelectionToggled.AddDynamic(this, &URTSSidebarWidget::UpdateHeroSelectionToggled);
      cpcRef->GetLocalPlayer()->GetSubsystem<UPartyDelegateContext>()->OnSelectionClearedDelegate.AddDynamic(this, &URTSSidebarWidget::UpdateDeselectAllHeroes);
   }

   StartDisplay(width, height);
   browser->ScriptEventEmitter.AddDynamic(this, &URTSSidebarWidget::HandleBluEvent);
}

void URTSSidebarWidget::NativeConstruct()
{
   Super::NativeConstruct();
   GetWorld()->GetTimerManager().SetTimer(
       initialSetupTimer,
       [this]() {
          if(browser)
          {
             if(!browser->IsBrowserLoading())
             {
                UpdatePartyInformation();
                GetWorld()->GetTimerManager().ClearTimer(initialSetupTimer);
             }
          }
       },
       1.f, true, 0.f);
   UpdatePartyInformation();
}

void URTSSidebarWidget::UpdatePartyInformation()
{
   if(browser && !browser->IsBrowserLoading())
   {
      if(ABasePlayer* BasePlayer = cpcRef->GetBasePlayer())
      {
         // Unsubscribe existing party members
         int index = 0;
         for(auto& handles : subscribedAttributeSetDelegateHandles)
         {
            if(handles.Key.IsValid() && handles.Value.IsValid())
            {
               URTSAttributeSet* attributeSet = const_cast<URTSAttributeSet*>(BasePlayer->GetHeroes()[index]->GetAbilitySystemComponent()->GetSet<URTSAttributeSet>());
               attributeSet->statUpdatedEvent.Remove(handles.Key);
               attributeSet->baseStatUpdatedEvent.Remove(handles.Value);
            }
            else
            {
               break;
            }
            ++index;
         }
         subscribedAttributeSetDelegateHandles.Empty();

         // Create the string to send to the browser
         FString heroInfoString;
         auto    writer = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&heroInfoString);

         writer->WriteArrayStart();
         // Create JSON for each hero object which is used to update the sidebar browser

         for(auto hero : BasePlayer->GetHeroes())
         {
            TSharedPtr<FJsonObject> heroObj = MakeShareable(new FJsonObject);
            heroObj->SetStringField("name", hero->GetGameName().ToString());
            heroObj->SetNumberField("hitpoints", hero->GetStatComponent()->GetVitalCurValue(EVitals::Health));
            heroObj->SetNumberField("maxHitpoints", hero->GetStatComponent()->GetVitalBaseValue(EVitals::Health));
            heroObj->SetNumberField("mana", hero->GetStatComponent()->GetVitalCurValue(EVitals::Mana));
            heroObj->SetNumberField("maxMana", hero->GetStatComponent()->GetVitalBaseValue(EVitals::Mana));
            heroObj->SetBoolField("bSelected", hero->GetUnitSelected());

            // Start listening to the health values
            URTSAttributeSet* attSet = const_cast<URTSAttributeSet*>(hero->GetAbilitySystemComponent()->GetSet<URTSAttributeSet>());
            subscribedAttributeSetDelegateHandles.Emplace(MakeTuple(attSet->statUpdatedEvent.AddUObject(this, &URTSSidebarWidget::UpdateHeroVitals),
                                                                    attSet->baseStatUpdatedEvent.AddUObject(this, &URTSSidebarWidget::UpdateHeroMaxVitals)));
            // Serialize our object (write the object to the string)
            FJsonSerializer::Serialize(heroObj.ToSharedRef(), writer);
         }

         heroInfoString.AppendChar(']');

         UpdateInformation("updatePartyInfo", heroInfoString);
      }
   }
}

void URTSSidebarWidget::UpdateDeselectAllHeroes()
{
   UpdateInformation("updateDeselectAllHeroes", TEXT(""_SV));
}

void URTSSidebarWidget::UpdateHeroSelectionToggled(AAlly* deselectedHeroRef)
{
   if(deselectedHeroRef->GetClass()->IsChildOf(ABaseHero::StaticClass()))
   {
      const FString heroInfoString = MakeSingleHeroSelectedJson(deselectedHeroRef);

      if(!deselectedHeroRef->IsSelected())
      {
         UpdateInformation("updateHeroToggleDeselect", heroInfoString);
      }
      else
      {
         UpdateInformation("updateSingleHeroToggleSelect", heroInfoString);
      }
   }
}

void URTSSidebarWidget::UpdateSingleHeroSelect(ABaseHero* selectedHero)
{
   GetWorld()->GetTimerManager().SetTimerForNextTick([this, selectedHero]() {
      const FString heroInfoString = MakeSingleHeroSelectedJson(selectedHero);

      if(cpcRef->GetBasePlayer()->GetSelectedHeroes().Num() == 1)
      {
         UpdateInformation("updateSingleHeroSelect", heroInfoString);
      }
      else
      {
         UpdateInformation("updateSingleHeroToggleSelect", heroInfoString);
      }
   });
}

void URTSSidebarWidget::UpdateHeroHealth(const FGameplayAttribute& attributeModified, float newAttributeValue, AUnit* unitAffected)
{
   FString heroInfoString = MakeHeroVitalJson(unitAffected, "hitpoints", newAttributeValue);
   UpdateInformation("updateHeroHealthInfo", heroInfoString);
}

void URTSSidebarWidget::UpdateHeroMana(const FGameplayAttribute& attributeModified, float newAttributeValue, AUnit* unitAffected)
{
   FString heroInfoString = MakeHeroVitalJson(unitAffected, "mana", newAttributeValue);
   UpdateInformation("updateHeroManaInfo", heroInfoString);
}

void URTSSidebarWidget::UpdateHeroVitals(const FGameplayAttribute& attributeModified, float newAttributeValue, AUnit* unitAffected)
{
   if(attributeModified.GetName() == "Health")
   {
      UpdateHeroHealth(attributeModified, newAttributeValue, unitAffected);
   }
   else if(attributeModified.GetName() == "Mana")
   {
      UpdateHeroMana(attributeModified, newAttributeValue, unitAffected);
   }
}

void URTSSidebarWidget::UpdateHeroMaxHealth(const FGameplayAttribute& attributeModified, float newAttributeValue, AUnit* unitAffected)
{
   FString heroInfoString = MakeHeroVitalJson(unitAffected, "maxHitpoints", newAttributeValue);
   UpdateInformation("updateHeroMaxHealthInfo", heroInfoString);
}

void URTSSidebarWidget::UpdateHeroMaxMana(const FGameplayAttribute& attributeModified, float newAttributeValue, AUnit* unitAffected)
{
   FString heroInfoString = MakeHeroVitalJson(unitAffected, "maxMana", newAttributeValue);
   UpdateInformation("updateHeroMaxManaInfo", heroInfoString);
}

void URTSSidebarWidget::UpdateHeroMaxVitals(const FGameplayAttribute& attributeModified, float newAttributeValue, AUnit* unitAffected)
{
   if(attributeModified.GetName() == "Health")
   {
      UpdateHeroMaxHealth(attributeModified, newAttributeValue, unitAffected);
   }
   else if(attributeModified.GetName() == "Mana")
   {
      UpdateHeroMaxMana(attributeModified, newAttributeValue, unitAffected);
   }
}

FString URTSSidebarWidget::MakeHeroVitalJson(const AUnit* heroRef, const FString& vitalKey, double vitalValue) const
{
   FString                 heroInfoString = "";
   const auto              writer         = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&heroInfoString);
   TSharedPtr<FJsonObject> heroObj        = MakeShareable(new FJsonObject);
   heroObj->SetStringField("name", heroRef->GetGameName().ToString());
   heroObj->SetNumberField(vitalKey, vitalValue);
   FJsonSerializer::Serialize(heroObj.ToSharedRef(), writer);
   return heroInfoString;
}

FString URTSSidebarWidget::MakeSingleHeroSelectedJson(const AUnit* heroRef) const
{
   FString                 heroInfoString = "";
   const auto              writer         = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&heroInfoString);
   TSharedPtr<FJsonObject> heroObj        = MakeShareable(new FJsonObject);
   heroObj->SetStringField("name", heroRef->GetGameName().ToString());
   FJsonSerializer::Serialize(heroObj.ToSharedRef(), writer);

   // Don't need to set bSelected, browser contains that logic
   return heroInfoString;
}

FReply URTSSidebarWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
   Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
   const ECursorStateEnum cursorStateRef                 = cpcRef->GetCameraPawn()->GetCursorState();
   const bool             letPlayerClickOnBrowserCursors = cursorStateRef == ECursorStateEnum::Moving || cursorStateRef == ECursorStateEnum::Select;
   if(InMouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton) && letPlayerClickOnBrowserCursors)
   {
      const FVector2D screenSpaceTolocalWigetPosition{InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition())};
      GetBrowser()->TriggerLeftMouseDown(screenSpaceTolocalWigetPosition);
      GetBrowser()->TriggerLeftMouseUp(screenSpaceTolocalWigetPosition);
      return FReply::Unhandled(); // Return unhandled so main game still handles click logic
   }
   return FReply::Unhandled();
}

FReply URTSSidebarWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
   Super::NativeOnMouseMove(InGeometry, InMouseEvent);
   if(UNLIKELY(GetBrowser() && !GetBrowser()->IsBrowserLoading()))
   {
      const FVector2D screenSpaceTolocalWidgetPosition{InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition())};
      GetBrowser()->TriggerMouseMove(screenSpaceTolocalWidgetPosition);
      return FReply::Unhandled();
   }
   return FReply::Unhandled();
}

void URTSSidebarWidget::HandleBluEvent(const FString& eventName, const FString& eventMessage)
{
   if(LIKELY(eventName == "selectHero"))
   {
      // Notify pawn we clicked on the browser (since the the pawn deselects everything when we click on the ground typically and we don't want that)
      // When this flag is set it means we performed a successful browser interaction and the browser fires off a blu_event
      cpcRef->GetCameraPawn()->clickedOnBrowserHud = true;

      UBluJsonObj* jsonObj         = UBluBlueprintFunctionLibrary::ParseJSON(eventMessage);
      const int    heroIndex       = jsonObj->GetNumValue("value");
      ABaseHero*   selectedHeroRef = cpcRef->GetBasePlayer()->GetHeroes()[heroIndex];

      if(!cpcRef->IsInputKeyDown(EKeys::LeftShift))
      {
         cpcRef->GetBasePlayer()->ClearSelectedUnits();
         selectedHeroRef->SetUnitSelected(true);
      }
      else
      {
         selectedHeroRef->SetUnitSelected(!selectedHeroRef->GetUnitSelected());
      }
   }
}
