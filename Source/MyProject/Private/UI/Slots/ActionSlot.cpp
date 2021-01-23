// Fill out your copyright notice in the Description page of Project Settings.

#include "MyProject.h"
#include "ActionSlot.h"

#include "TextBlock.h"
#include "UserInput.h"
#include "UI/HUDManager.h"

#include "UI/UserWidgets/ToolTipWidget.h"
#include "UMG/Public/Components/Image.h"
#include "UMG/Public/Components/Button.h"

TSubclassOf<UToolTipWidget> UActionSlot::toolTipWidgetClass = nullptr;

UActionSlot::UActionSlot(const FObjectInitializer& o) : UUserWidget(o)
{
   ConstructorHelpers::FClassFinder<UToolTipWidget> tooltipClass(TEXT("/Game/RTS_Tutorial/HUDs/HelpUI/GameIndicators/BP_ToolTipBox"));
   toolTipWidgetClass = tooltipClass.Class;
}

void UActionSlot::SetSlotImage(UTexture2D* image)
{
   actionImage->SetBrushFromTexture(image, false);
}

void UActionSlot::SetInfo(FText newInfo)
{
   infoText->SetText(newInfo);
}

void UActionSlot::SetImageFromMaterial(UMaterialInstanceDynamic* image)
{
   actionImage->SetBrushFromMaterial(image);
}

UTexture2D* UActionSlot::GetImage() const
{
   UTexture* slotTex = Cast<UTexture>(actionImage->Brush.GetResourceObject());
   if(!slotTex)
   {
      UMaterialInterface* slotMaterial = Cast<UMaterialInterface>(actionImage->Brush.GetResourceObject());
      slotMaterial->GetTextureParameterValue(TEXT("RadialTexture"), slotTex);
   }

   return Cast<UTexture2D>(slotTex);
}

void UActionSlot::NativeOnInitialized()
{
   Super::NativeOnInitialized();
   btnAction->OnClicked.AddDynamic(this, &UActionSlot::OnBtnClick);
   btnAction->OnHovered.AddDynamic(this, &UActionSlot::OnBtnHover);
   CPCRef = Cast<AUserInput>(GetWorld()->GetGameInstance()->GetFirstLocalPlayerController());
}

void UActionSlot::OnBtnHover()
{
   const TWeakObjectPtr<UToolTipWidget> tooltipWidgetRef = CreateWidget<UToolTipWidget>(CPCRef, toolTipWidgetClass);
   ShowDesc(tooltipWidgetRef.Get());
   btnAction->SetToolTip(tooltipWidgetRef.Get());
}
