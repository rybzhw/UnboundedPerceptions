// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "AbilitySystemInterface.h"
#include "TypeContainer.h"
#include "GameFramework/PlayerController.h"
#include "UserInput.generated.h"

class ABasePlayer;
class AUnit;
class AAlly;
class AEnemy;
class ABaseHero;
class UMyGameInstance;
class AHUDManager;
class ARTSGameMode;
class ARTSGameState;
class ARTSPawn;
class UQuestManager;
class IWidgetToggler;
class IHUDProvider;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerControllerSetup);

/**
 * This is the CameraPawnController base class.  It holds input that is common across all types of pawns.  In the RTS/RPG hybrid, there are several different pawns representing different input schemes in the case of
 * minigames and such.  A lot of references are also stored here, so expect this class to be included several times.
 */
UCLASS()
class MYPROJECT_API AUserInput : public APlayerController
{
   GENERATED_BODY()

 public:
   AUserInput();

   /**Replacement for BeginPlay because in 4.19 GameInstance BeginPlay always starts off but actor beginplay goes before PlayerController for some reason...*/

   void BeginPlay() override;
   void Tick(float deltaSeconds) override;

   virtual void SetupInputComponent() override; // Bind functionality to input

   FOnPlayerControllerSetup& OnPlayerControllerSetup() const { return PlayerControllerFinishSetupEvent; }

#pragma region references

 private:
   /*PlayerState reference*/
   UPROPERTY(VisibleAnywhere, BlueprintGetter = "GetBasePlayer", Category = "References")
   ABasePlayer* basePlayer;

   /**GameInstance reference.  GameInstance is used for storing consistant data between levels*/
   UPROPERTY(VisibleAnywhere, BlueprintGetter = "GetMyGameInstance", Category = "References")
   UMyGameInstance* gameInstance;

   /** HUDManager ref.  Set this in UserInput because it depends on UserInput being created first. */
   UPROPERTY()
   AHUDManager* hudManagerRef;

   UPROPERTY()
   ARTSGameMode* gameMode;

   UPROPERTY()
   ARTSGameState* gameState;

   UPROPERTY()
   ARTSPawn* cameraPawn;

   UPROPERTY(EditDefaultsOnly, Meta = (AllowPrivateAccess = "true"))
   TSubclassOf<AHUDManager> hudManagerClass;

   UPROPERTY(BlueprintAssignable, Meta = (AllowPrivateAccess = true))
   mutable FOnPlayerControllerSetup PlayerControllerFinishSetupEvent;

 public:
   /**Offset used for widgets when dragging around*/
   UPROPERTY(BlueprintReadWrite)
   FVector2D offset;

   /**
    * @brief Won't exist until after widgets created
    * @return Returns ref to HUDManager
    */
   UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Accessors")
   FORCEINLINE AHUDManager* GetHUDManager() const { return hudManagerRef; }

   UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Accessors")
   TScriptInterface<IWidgetToggler> GetWidgetToggler() const;

   UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Accessors")
   TScriptInterface<IHUDProvider> GetWidgetProvider() const;

   UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Accessors")
   FORCEINLINE ARTSGameState* GetGameState() const { return gameState; }

   UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Accessors")
   FORCEINLINE ARTSGameMode* GetGameMode() const { return gameMode; }

   UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Accessors")
   FORCEINLINE ABasePlayer* GetBasePlayer() const { return basePlayer; }

   UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Accessors")
   FORCEINLINE ARTSPawn* GetCameraPawn() const { return cameraPawn; }

   UFUNCTION(BlueprintGetter)
   FORCEINLINE UMyGameInstance* GetMyGameInstance() const { return gameInstance; }

 private:
#pragma endregion

   /**Input stored in the controller can be called despite whatever pawn is possessed*/
#pragma region input
   void ToggleBreakMenu();
   void ToggleInventory();
   void ToggleQuestJournal();
   void ToggleQuestList();
   void ToggleCharacterMenu();
   void ToggleEquipmentMenu();
   void ToggleSpellbookMenu();

   FORCEINLINE bool NotInMinigame();

   void OnRep_PlayerState() override;
   void OnRep_Pawn() override;
#pragma endregion
};
