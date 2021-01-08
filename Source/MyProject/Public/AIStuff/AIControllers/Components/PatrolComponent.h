// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "PatrolComponent.generated.h"

class UBehaviorTreeComponent;
class AAIController;
class UBehaviorTree;

/**
 * Allows us to easily set patrol points for enemy and npc's because it is linked to a visualization
 * AIs that use the patrol task rely on the AIs having a PatrolComponent
 */

UCLASS(ClassGroup = (Custom), Within = AIController, meta = (BlueprintSpawnableComponent))
class MYPROJECT_API UPatrolComponent : public USceneComponent
{
   GENERATED_BODY()

 public:
   UPatrolComponent();

   UFUNCTION()
   void DeletePatrolPoint(int patrolIndex);

   UFUNCTION(BlueprintCallable)
   FORCEINLINE FVector GetCurrentPatrolPoint() { return patrolPoints[currentPatrolIndex]; }

   UFUNCTION(BlueprintCallable)
   void SetPatrolPoints(const TArray<FVector>& pointsToPatrol) { patrolPoints = pointsToPatrol; }

   /**
    * @brief Initiates a patrol command
    */
   UFUNCTION(BlueprintCallable)
   bool Patrol();

   UFUNCTION(BlueprintCallable)
   void StopPatrolling();

   /**
    * @brief Should this actor start patrolling when the level loads?
    */
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "NPCMovement")
   bool patrolOnStart;

   /**List of NPC patrol points.  NPC will go to each point in order and loop.*/
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "NPCMovement", Meta = (MakeEditWidget = true))
   TArray<FVector> patrolPoints;

   static FLinearColor EDITOR_UNSELECTED_PATROL_COLOR;
   static FLinearColor EDITOR_SELECTED_PATROL_COLOR;

 protected:
   virtual void BeginPlay() override final;

   /* Patrolling is a behavior since it's more complex than a single task. */
   UPROPERTY(EditDefaultsOnly)
   UBehaviorTree* patrolTree;

 private:
   /** Called in Patrol task to move unit to next point*/
   void MoveToNextPatrolPoint(FAIRequestID requestId, EPathFollowingResult::Type moveRes);

   bool MoveToNextPointAfterMoveRequestFail(EPathFollowingRequestResult::Type moveRes);
   
   UBehaviorTreeComponent* ownerBTComp;

   int currentPatrolIndex = -1;
};