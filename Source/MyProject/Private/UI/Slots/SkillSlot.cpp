#include "MyProject.h"
#include "SkillSlot.h"
#include "ActionSlot.h"
#include "UserInput.h"
#include "../HUDManager.h"
#include "SpellSystem/MySpell.h"

#include "WorldObjects/Ally.h"
#include "AIStuff/AIControllers/AllyAIController.h"

#include "AbilitySystemComponent.h"
#include "BasePlayer.h"

#include "PanelWidget.h"
#include "RTSAbilitySystemComponent.h"
#include "RTSActionBarSkillDrag.h"
#include "RTSSpellbookDrag.h"
#include "SpellFunctionLibrary.h"
#include "ToolTipWidget.h"
#include "UIDelegateContext.h"

#include "Materials/MaterialInstanceDynamic.h"
#include "UMG/Public/Components/Image.h"
#include "UMG/Public/Components/TextBlock.h"

class UUIDelegateContext;

UMaterialInterface* USkillSlot::cdMatInstance    = nullptr;
UMaterialInterface* USkillSlot::skillMatInstance = nullptr;

USkillSlot::USkillSlot(const FObjectInitializer& o) : UActionSlot(o)
{
   // static ConstructorHelpers::FObjectFinder<UCurveFloat> curve(TEXT("/Game/RTS_Tutorial/HUDs/ActionUI/StandardLinear"));
   // checkf(curve.Object, TEXT("Curve for ability timelines not found!"))

   const ConstructorHelpers::FObjectFinder<UMaterialInterface> loadedCDMat(TEXT("/Game/RTS_Tutorial/Materials/UIMats/SkillMats/MI_RadialSkillCD"));
   checkf(loadedCDMat.Object, TEXT("Material interface for skillslots not found!"));
   cdMatInstance = loadedCDMat.Object;

   const ConstructorHelpers::FObjectFinder<UMaterialInterface> loadedSkillMat(TEXT("/Game/RTS_Tutorial/Materials/UIMats/SkillMats/RadialMatSkill_Instance"));
   checkf(loadedSkillMat.Object, TEXT("Material interface for skillslots not found!"));
   skillMatInstance = loadedSkillMat.Object;
}

void USkillSlot::NativeOnInitialized()
{
   Super::NativeOnInitialized();
   imageDMatInst = UMaterialInstanceDynamic::Create(skillMatInstance, this);
   cdDMatInst    = UMaterialInstanceDynamic::Create(cdMatInstance, this);
   actionImage->SetBrushFromMaterial(imageDMatInst);
   Image_CD->SetBrushFromMaterial(cdDMatInst);
}

void USkillSlot::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
   if(const TSubclassOf<UMySpell> spellCorrespondingToSlot = GetOwningAbilityComponent()->GetSpellAtSlot(slotIndex))
   {
      UDraggedActionWidget* dragVisual = CreateDragIndicator();
      dragVisual->SetDraggedImage(spellCorrespondingToSlot.GetDefaultObject()->GetSpellDefaults().image);

      URTSActionBarSkillDrag* dragOp = NewObject<URTSActionBarSkillDrag>(this);
      dragOp->Pivot                  = EDragPivot::CenterCenter;
      dragOp->DefaultDragVisual      = dragVisual;
      dragOp->slotIndex              = slotIndex;
      OutOperation                   = MoveTemp(dragOp);
   }
}

void USkillSlot::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
   if(GetOwningAbilityComponent()->GetSpellAtSlot(slotIndex))
   {
      Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
   }
}

bool USkillSlot::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
   if(URTSActionBarSkillDrag* dragOp = Cast<URTSActionBarSkillDrag>(InOperation))
   {
      GetOwningLocalPlayer()->GetSubsystem<UUIDelegateContext>()->OnSkillSlotDroppedEvent.Broadcast(dragOp->slotIndex, slotIndex);
      return true;
   }
   if(URTSSpellbookDrag* spellbookDragOp = Cast<URTSSpellbookDrag>(InOperation))
   {
      GetOwningLocalPlayer()->GetSubsystem<UUIDelegateContext>()->OnSkillSlotDroppedSBEvent.Broadcast(spellbookDragOp->slotIndex, slotIndex);
      return true;
   }
   return false;
}

void USkillSlot::UpdateSkillSlot(TSubclassOf<UMySpell> spellClass)
{
   if(IsValid(spellClass))
   {
      UMySpell* spellObject = spellClass.GetDefaultObject();

      SetSlotImage(spellObject->GetSpellDefaults().image);

      if(URTSAbilitySystemComponent* ownerAbilitySystemComp = GetOwningAbilityComponent())
      {
         const bool bIsSpellOffCooldown = spellObject->GetCooldownTimeRemaining(ownerAbilitySystemComp->AbilityActorInfo.Get()) > SMALL_NUMBER;

         if(bIsSpellOffCooldown)
         {
            ShowCooldown();
         }
         else
         {
            OnCDFinished();
         }
      }
   }
   else
   {
      ResetSkillSlot();
   }
}

void USkillSlot::UpdateCD()
{
   if(const URTSAbilitySystemComponent* abilityComponent = GetOwningAbilityComponent())
   {
      if(abilityComponent->GetAbilities().Num() < slotIndex || abilityComponent->GetSpellAtSlot(slotIndex) == nullptr)
      {
         OnCDFinished();
         return;
      }

      const float cdTimeRemaining =
          abilityComponent->GetAbilities()[slotIndex].GetDefaultObject()->GetCooldownTimeRemaining(GetOwningAbilityComponent()->AbilityActorInfo.Get());
      const float cdDuration = abilityComponent->GetAbilities()[slotIndex].GetDefaultObject()->GetCDDuration(GetOwningAbilityComponent());

      if(LIKELY(cdTimeRemaining > 0))
      {
         cdDMatInst->SetScalarParameterValue("Percent", cdTimeRemaining / cdDuration);
         Text_CDTime->SetText(FText::AsNumber(static_cast<int>(cdTimeRemaining)));
      }
      else
      {
         OnCDFinished();
      }
   }
   else
   {
      OnCDFinished();
   }
}

void USkillSlot::OnCDFinished()
{
   GetWorld()->GetTimerManager().ClearTimer(cooldownProgressTimer);
   Image_CD->SetVisibility(ESlateVisibility::Hidden);
   Text_CDTime->SetVisibility(ESlateVisibility::Hidden);
}

void USkillSlot::ShowCooldown()
{
   GetWorld()->GetTimerManager().SetTimer(cooldownProgressTimer, this, &USkillSlot::UpdateCD, .1f, true, 0);
   ShowCDVisuals();
}

void USkillSlot::ShowCDVisuals() const
{
   Image_CD->SetVisibility(ESlateVisibility::HitTestInvisible);
   Text_CDTime->SetVisibility(ESlateVisibility::HitTestInvisible);
}

URTSAbilitySystemComponent* USkillSlot::GetOwningAbilityComponent() const
{
   if(AUserInput* CPC = GetOwningPlayer<AUserInput>())
   {
      if(const AUnit* focusedUnit = CPC->GetBasePlayer()->GetFocusedUnit())
      {
         if(URTSAbilitySystemComponent* abilityComp = focusedUnit->FindComponentByClass<URTSAbilitySystemComponent>())
         {
            return abilityComp;
         }
      }
   }
   return nullptr;
}

void USkillSlot::SetSlotImage(UTexture2D* image)
{
   if(IsValid(image))
   {
      imageDMatInst->SetTextureParameterValue("RadialTexture", image);
      SetImageFromMaterial(imageDMatInst);
      cdDMatInst->SetTextureParameterValue("RadialTexture", image); // update the cooldown image
      Image_CD->SetBrushFromMaterial(cdDMatInst);
   }
   else
   {
      imageDMatInst->SetTextureParameterValue("RadialTexture", defaultSlotTexture);
      SetImageFromMaterial(imageDMatInst);
   }
}

void USkillSlot::ResetSkillSlot()
{
   SetSlotImage(defaultSlotTexture);
   OnCDFinished();
}

void USkillSlot::ShowDesc(UToolTipWidget* tooltip)
{
   if(const auto ownerAbilityComp = GetOwningAbilityComponent())
   {
      if(const TSubclassOf<UMySpell> spellClass = ownerAbilityComp->GetSpellAtSlot(slotIndex))
      {
         UMySpell* spellAtSlot = spellClass.GetDefaultObject();
         FString   relevantSpellInfo;

         if(spellAtSlot->GetSpellDefaults().Cost.Num() > 0)
         {
            relevantSpellInfo += "Costs " + FString::FromInt(spellAtSlot->GetCost(ownerAbilityComp)) + " mana";
         }

         if(spellAtSlot->GetSpellDefaults().Cooldown.Num() > 0)
         {
            relevantSpellInfo += "\r\n" + FString::FromInt(spellAtSlot->GetCDDuration(ownerAbilityComp)) + " second CD";
         }

         if(spellAtSlot->GetSpellDefaults().Range.Num() > 0)
         {
            relevantSpellInfo += "\r\n" + FString::FromInt(spellAtSlot->GetRange(ownerAbilityComp)) + " range";
         }

         if(AUserInput* CPCRef = Cast<AUserInput>(GetOwningPlayer<AUserInput>()))
         {
            tooltip->SetupTTBoxText(
                FText::Format(NSLOCTEXT("SkillSlotDesc", "SpellNameAndLevel", "{0} ({1})"), spellAtSlot->GetSpellName(), spellAtSlot->GetLevel(ownerAbilityComp)),
                USpellFunctionLibrary::ParseDesc(spellAtSlot->GetDescription(), ownerAbilityComp, spellAtSlot), spellAtSlot->GetElem(),
                FText::FromString(relevantSpellInfo), FText::GetEmpty());
         }
      }
   }
}
