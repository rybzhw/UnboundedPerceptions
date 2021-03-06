// Fill out your copyright notice in the Description page of Project Settings.

#include "MyProject.h"
#include "ConfirmationBox.h"
#include "UserInput.h"
#include "UI/HUDManager.h"

void UConfirmationBox::Confirm()
{
   if(OnConfirmationMade().IsBound())
      OnConfirmationMade().Execute();
   hudManagerRef->HideHUD(EHUDs::HS_Confirmation);
}
