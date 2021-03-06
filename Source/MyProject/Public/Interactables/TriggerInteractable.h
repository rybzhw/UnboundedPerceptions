// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interactables/Interactable.h"
#include "EventSystem/Trigger.h"
#include "EventSystem/RTSConditional.h"
#include "TriggerInteractable.generated.h"

class UMyGameInstance;
class ARTSGameMode;

/*An object that activates a trigger when interacted with*/

UCLASS()
class MYPROJECT_API ATriggerInteractable : public AActor, public IInteractable
{
   GENERATED_BODY()

   ARTSGameMode* gameModeRef;

 public:
   ATriggerInteractable();

   UPROPERTY(EditAnywhere)
   USceneComponent* scene;

   UPROPERTY(EditAnywhere)
   UStaticMeshComponent* staticMesh;

   virtual void BeginPlay() override;

   void    Interact_Implementation(ABaseHero* hero) override;
   FVector GetInteractableLocation_Implementation() const override;

   /**Set this trigger to something when you want to interact*/
   UPROPERTY(EditAnywhere, Category = "Trigger")
   FTriggerData triggerActivatedOnInteract;
};
