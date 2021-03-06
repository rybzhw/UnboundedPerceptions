// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/SceneCapture2D.h"
#include "MySceneCapture2D.generated.h"

/**
 * Scene capture with base settings saved in C++ so we can inherit this and use it for things such as the minimap.
 */
UCLASS()
class MYPROJECT_API AMySceneCapture2D : public ASceneCapture2D
{
   GENERATED_BODY()

   AMySceneCapture2D();
};
