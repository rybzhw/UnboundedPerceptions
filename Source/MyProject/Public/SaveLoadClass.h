#pragma once
#include "SaveGameStructs.h"
#include "LevelSaveStructs.h"
#include "SaveLoadClass.generated.h"

class AUserInput;
class ABasePlayer;
class UUpStatComponent;

class AAlly;
class ASummon;
class ABaseHero;

/**
* Class that deals with saving and loading to disk
* Saving requires us to store our information in a binary array, and then write that array to disk
* When writing to the binary array, we have to pass information by reference, which is why we want to copy our information to a struct.
* For certain data, we can only grab some copies of information.  We can workaround this using friend classes, however
* we'd have to modify the source code to get certain informatoin like getting the transformation in scene component.
*/
UCLASS(Blueprintable)
class MYPROJECT_API USaveLoadClass : public UObject
{
   GENERATED_BODY()

 public:
   ~USaveLoadClass();
   void Init();

   /**@param filePath: The absolute filepath of the folder we should save the data to*/
   bool SaveToFilePath(const FString& filePath);
   /**@param filePath: The absolute filepath of the folder we should save the data to*/
   bool LoadFromFilePath(const FString& filePath);

   UPROPERTY()
   AUserInput* controllerRef;    // Used to print message to client log seen with (~) key

   bool        currentlyLoading; // Check if a load was initiated so level callback knows to deal with it

 private:
   FSaveGameDataInfo         gameSaveSaveData;
   FSceneSaveInfo            sceneSaveData;
   FCameraSaveInfo           cameraSaveData;
   FBasePlayerSaveInfo       playerSaveData;
   TArray<FAllySaveInfo>     npcsSaveData;
   TArray<FSummonSaveInfo>   summonsSaveData;
   TArray<FHeroSaveInfo>     heroesSaveData;
   TMap<FName, FMapSaveInfo> mapSaveData;

   // Remember: Passing Information by reference doesn't require storage and doesn't require indirection
   /**Function used to save and load data from binary array by using the overloaded << operator which saves/loads depending on FArchive derived type
    *@param ar: Archive used to save/load data from the disk to binary array and vice versa*/
   void SaveLoadFunction(FArchive& ar, bool isSaving);

#pragma region SaveSetup

   /**Setup structs used to save data*/
   // TODO: Save settings and load them at game start.  Could be seperate from this system
   void SetupSaveData();
   /**Setup structs with controller information*/
   void SetupSaveControllerData();
   /**Save data about player*/
   void SetupSavePlayerData();
   /**Save data about unit's stats*/
   void SetupSaveBaseCharacterData(const UUpStatComponent& baseChar, FBaseCharacterSaveInfo& saveInfo);
   /**Save Ally data inside heroes, summons, and NPC esorts*/
   void SetupSaveAllyData(AAlly& ally, FAllySaveInfo& allyInfo);
   /**Save data about summons in party*/
   void SetupSaveSummonData();
   /**Save data about heroes in party*/
   void SetupSaveHeroData();
   /**Save data about npc escorts in party*/
   void SetupNPCEscortData();
   /**Save data about any changes occuring inside levels*/
   void SetupLevelSaveData();
   /**Actually set the actor's data values according to what is loaded*/

#pragma endregion

#pragma region LoadedDataSetup
 public:
   inline void SetupController();
   inline void SetupPlayer();
   inline void SetupAlliedUnits();
   inline void SetupBaseCharacter(AAlly* spawnedAlly, FBaseCharacterSaveInfo& baseCSaveInfo);
   inline void SetupLevelLoadData();
   /**Called after player attempts to load a save*/
   void SetupLoad();
#pragma endregion
};