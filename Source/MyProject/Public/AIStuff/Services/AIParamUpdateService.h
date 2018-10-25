// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "AIParamUpdateService.generated.h"

/**
 * Service to continuously update AI as long as it is in combat
 */

class AUserInput;
class AUnit;

struct FBTAIParamUpdateServiceMemory {
   AUnit* owner;
};

UCLASS()
class MYPROJECT_API UAIParamUpdateService : public UBTService
{
   GENERATED_BODY()

   UPROPERTY(BlueprintReadOnly, Meta = (AllowPrivateAccess = "true", ExposeOnSpawn = "true"))
   AUserInput* controllerRef;

   void OnSearchStart(FBehaviorTreeSearchData& SearchData) override;
   void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

   void    InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryInit::Type InitType) const override;
   uint16  GetInstanceMemorySize() const override { return sizeof(FBTAIParamUpdateServiceMemory); }
   FString GetStaticDescription() const override;
};