// Fill out your copyright notice in the Description page of Project Settings.

#include "MyProject.h"
#include "RTSInputBox.h"
#include "UserInput.h"
#include "UI/HUDManager.h"

void URTSInputBox::Confirm()
{
   if(OnInputConfirmed().Execute(txtInput))
      hudManagerRef->HideHUD(EHUDs::HS_InputBox);
}
