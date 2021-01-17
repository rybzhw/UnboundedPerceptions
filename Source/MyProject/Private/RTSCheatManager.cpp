#include "MyProject.h"
#include "RTSCheatManager.h"

#include "RTSGameMode.h"
#include "RTSGameState.h"
#include "UserInput.h"
#include "BasePlayer.h"
#include "RTSIngameWidget.h"
#include "RTSVisionComponent.h"

#include "UpResourceManager.h"
#include "UpStatComponent.h"
#include "Quests/QuestManager.h"
#include "Quests/Quest.h"

#include "UI/HUDManager.h"
#include "UI/Actionbar/ActionbarInterface.h"
#include "UI/Actionbar/ESkillContainer.h"
#include "UI/Slots/SkillSlot.h"

#include "SpellSystem/SpellDataManager.h"
#include "SpellSystem/RTSAbilitySystemComponent.h"

#include "WorldObjects/Unit.h"
#include "WorldObjects/BaseHero.h"
#include "WorldObjects/Enemies/Enemy.h"

#include "EventSystem/EventManager.h"

void URTSCheatManager::InitCheatManager()
{
   Super::InitCheatManager();
   userInputRef = Cast<AUserInput>(GetWorld()->GetFirstPlayerController());
   gameModeRef  = Cast<ARTSGameMode>(GetWorld()->GetAuthGameMode());
   gameStateRef = Cast<ARTSGameState>(GetWorld()->GetGameState());
}

void URTSCheatManager::LevelUp(FString heroName)
{
   GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::White, TEXT("Leveling Up via Cheats!"));
   if(ABaseHero* heroRef = *userInputRef->GetBasePlayer()->GetHeroes().FindByPredicate([heroName](ABaseHero* hero) { return hero->GetGameName().ToString() == heroName; })
   ) { heroRef->LevelUp(); }
}

void URTSCheatManager::LevelUpToLevel(FString heroName, int level)
{
   if(ABaseHero* heroRef = *userInputRef->GetBasePlayer()->GetHeroes().FindByPredicate([heroName](ABaseHero* hero)
   {
      return hero->GetGameName().ToString() == heroName;
   }))
   {
      while(heroRef->GetStatComponent()->GetUnitLevel() < level)
         heroRef->LevelUp();
   }
}

void URTSCheatManager::GodMode(FString objectID, int toggleGodMode)
{
#if UE_EDITOR
   if(AUnit* unitRef = UpResourceManager::FindTriggerObjectInWorld<AUnit>(objectID, userInputRef->GetWorld()))
   {
      if(toggleGodMode)
         unitRef->GetAbilitySystemComponent()->AddLooseGameplayTag(FGameplayTag::RequestGameplayTag("Combat.Effect.Buff.GodMode"));
      else
         unitRef->GetAbilitySystemComponent()->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag("Combat.Effect.Buff.GodMode"));
   }
#endif
}

void URTSCheatManager::AddQuest(FString questName)
{
   const auto questTag = FGameplayTag::RequestGameplayTag(*(FString("QuestName." + questName)));
   if(gameModeRef->GetQuestManager()->questClassList.Contains(questTag))
   {
      FTriggerData addQuestTrigger;
      addQuestTrigger.enabled       = true;
      addQuestTrigger.numCalls      = 1;
      addQuestTrigger.triggerType   = ETriggerType::AddQuestTrigger;
      addQuestTrigger.triggerValues = {questName, "1"};
      gameModeRef->GetTriggerManager()->ActivateTrigger(addQuestTrigger);
   } else { UE_LOG(LogTemp, Error, TEXT("Quest cheat command passed invalid parameters!")); }
}

void URTSCheatManager::FinishQuest(FString questName, int isSucessful)
{
   AQuest* quest = *gameModeRef->GetQuestManager()->quests.FindByPredicate([questName](AQuest* quest) { return quest->questInfo.name.ToString() == questName; });
   if(quest) { isSucessful == 0 ? quest->CompleteQuest(false) : quest->CompleteQuest(true); }
}

void URTSCheatManager::EquipSpell(FString spellNameTag, int slot)
{
   if(AUnit* focusedUnit = userInputRef->GetBasePlayer()->GetFocusedUnit())
   {
      if(slot >= 0 && slot < focusedUnit->GetAbilitySystemComponent()->GetAbilities().Num())
      {
         focusedUnit->GetAbilitySystemComponent()->SetSpellAtSlot(
         USpellDataManager::GetData().GetSpellClass(
         FGameplayTag::RequestGameplayTag(*(FString("Skill.Name.") + spellNameTag))), slot);
      }
   }
}

void URTSCheatManager::RefreshSpells()
{
   for(AUnit* ally : userInputRef->GetGameState()->GetAllFriendlyUnits())
      ally->GetAbilitySystemComponent()->RemoveActiveEffectsWithGrantedTags(FGameplayTagContainer(FGameplayTag::RequestGameplayTag("Skill.Name")));
   for(AUnit* enemy : userInputRef->GetGameState()->GetAllEnemyUnits())
      enemy->GetAbilitySystemComponent()->RemoveActiveEffectsWithGrantedTags(FGameplayTagContainer(FGameplayTag::RequestGameplayTag("Skill.Name")));
}

void URTSCheatManager::RefreshHeroSpells(FString heroName)
{
   if(!heroName.IsEmpty())
   {
      if(ABaseHero* heroRef = *userInputRef->GetBasePlayer()->GetHeroes().FindByPredicate([heroName](ABaseHero* hero)
      {
         return hero->GetGameName().ToString() == heroName;
      }))
      {
         heroRef->GetAbilitySystemComponent()->RemoveActiveEffectsWithGrantedTags(FGameplayTagContainer(FGameplayTag::RequestGameplayTag("Skill.Name")));
      }
   } else
   {
      for(ABaseHero* hero : userInputRef->GetBasePlayer()->GetHeroes())
         hero->GetAbilitySystemComponent()->RemoveActiveEffectsWithGrantedTags(FGameplayTagContainer(FGameplayTag::RequestGameplayTag("Skill.Name")));
   }
}

void URTSCheatManager::PauseGameTimer() { gameStateRef->UpdateGameSpeed(0); }

void URTSCheatManager::SetGameTime(int seconds, int minutes, int hours) { gameStateRef->SetGameTime(FUpTime(seconds, minutes, hours), FUpDate()); }

void URTSCheatManager::SetGameDay(int day, int month, int year) { gameStateRef->SetGameTime(FUpTime(), FUpDate(day, month, year)); }

void URTSCheatManager::SetUnitCurHP(FString unitName, int hpVal)
{
   if(AUnit* unit = UpResourceManager::FindTriggerObjectInWorld<AUnit>(unitName, GetWorld())) { unit->GetStatComponent()->ModifyStats(hpVal, EVitals::Health); }
}

void URTSCheatManager::SeeAll()
{
   for(AUnit* e : gameStateRef->GetAllEnemyUnits())
   {
      e->GetVisionComponent()->IncVisionCount();
      e->GetCapsuleComponent()->SetVisibility(true, true);
   }
}

void URTSCheatManager::LearnAllTopics()
{
   const TSharedPtr<FGameplayTagNode>   rootDialogNode = UGameplayTagsManager::Get().FindTagNode("Dialog");
   TSet<TSharedPtr<FGameplayTagNode>>   leafNodes{};
   TSet<TSharedPtr<FGameplayTagNode>>   newLeafNodes{};
   TArray<TSharedPtr<FGameplayTagNode>> childNodes{};

   leafNodes.Add(rootDialogNode);

   while(leafNodes.Num() > 0)
   {
      for(TSharedPtr<FGameplayTagNode> node : leafNodes)
      {
         childNodes = node->GetChildTagNodes();
         if(childNodes.Num() > 0) { newLeafNodes.Append(childNodes); } else { userInputRef->GetBasePlayer()->LearnDialogTopic(node->GetCompleteTag()); }
      }
      leafNodes = newLeafNodes;
      newLeafNodes.Empty();
   }
}

void URTSCheatManager::SetChapterAndSection(int chapter, int section) { gameModeRef->eventManager->SkipToEvent(chapter, section); }

void URTSCheatManager::SpawnEnemies(FName id, int level, int numberToSpawn, FVector spawnLocation)
{
   for(int i = 0; i < numberToSpawn; ++i)
   {
   }
}

void URTSCheatManager::EnableCSVCategories(FString csvCategories)
{
   TArray<FString> CsvCategories;
   csvCategories.ParseIntoArray(CsvCategories, TEXT(","), true);
   for(auto x : CsvCategories)
   {
      if(!FCsvProfiler::Get()->EnableCategoryByString(x))
      UE_LOG(LogTemp, Error, TEXT("Category %s was unsucessfully enabled!"), *x);
   }
}
