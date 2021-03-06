#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "StatChangeEffect.generated.h"

USTRUCT(BlueprintType)
struct FStatChange
{
   GENERATED_BODY()

   /**Attribute to change*/
   UPROPERTY(EditAnywhere, BlueprintReadWrite)
   FGameplayAttribute changedAtt;

   /**Magnitude of the changed stat value*/
   UPROPERTY(EditAnywhere, BlueprintReadWrite)
   float changeStatMagnitude;
};

/**
 * OBSOLETE
 * Modifiers are now done by being change on the default GameplayEffect we create.  This is done because
 * those modifiers can be reset and replicated across the network by the nature of how they are made.
 * Every stat change effect must use the stat change calc...
 */

UCLASS()
class MYPROJECT_API UStatChangeEffect : public UGameplayEffect
{
   GENERATED_UCLASS_BODY()

 public:
   /** List of stat changes this effect will provide. Only useful on SetByCaller type magnitudes. */
   UPROPERTY(BlueprintReadWrite, meta = (AllowPrivateAccess = "true", ExposeOnSpawn = "true"), Category = "Stats")
   TArray<FStatChange> StatChanges = TArray<FStatChange>{FStatChange()};
};
