#include "MyProject.h"
#include "UserWidget.h"
#include "BasePlayer.h"
#include "UI/HUDManager.h"
#include "UI/Minimap.h"
#include "WorldObjects/BaseHero.h"
#include "QuestManager.h"

#include "GameplayDelegateContext.h"
#include "UI/QuestList.h"
#include "UI/QuestListSlot.h"
#include "UI/QuestJournal.h"
#include "Quest.h"
#include "WorldObjects/Enemies/Enemy.h"
#include "WorldObjects/NPC.h"
#include "UserInput.h"
#include "GoalActor.h"
#include "ItemDelegateContext.h"
#include "RTSIngameWidget.h"
#include "Interactables/NamedInteractableDecorator.h"
#include "Items/ItemManager.h"
#include "ItemDelegateContext.h"
#include "QuestListSlotGoalEntry.h"
#include "UpFunctionLibrary.h"
#include "DialogSystem/NPCDelegateStore.h"
#include "Runtime/AssetRegistry/Public/AssetRegistryModule.h"

#if WITH_EDITOR
void UQuestManager::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
   Super::PostEditChangeProperty(PropertyChangedEvent);
   UpdateQuestClassList();
}
#endif

void UQuestManager::UpdateQuestClassList()
{
   UE_LOG(LogTemp, Display, TEXT("Quest Class Map Updated"));

   FAssetRegistryModule& assetReg        = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
   IAssetRegistry&       assetRegistry   = assetReg.Get();
   const FString         questFolderPath = TEXT("/Game/RTS_Tutorial/Blueprints/Quest/Blueprints/Quests");

   assetRegistry.AddPath(questFolderPath);
   assetRegistry.ScanPathsSynchronous({questFolderPath});
   TArray<FAssetData> questAssets;
   assetRegistry.GetAssetsByPath(*questFolderPath, questAssets, true);

   for(FAssetData& asset : questAssets)
   {
      const UBlueprint* questBP = Cast<UBlueprint>(asset.GetAsset());
      if(questBP)
      {
         AQuest* quest = Cast<AQuest>(questBP->GeneratedClass->GetDefaultObject(false));
         if(quest)
         {
            TSubclassOf<AQuest> questClass = questBP->GeneratedClass.Get();
            questClassList.Add(quest->GetQuestInfo().id, questClass);
         }
      }
   }
}

void UQuestManager::Init()
{
   if(!HasAuthority() || !IsRunningDedicatedServer())
   {
      controllerRef = Cast<AUserInput>(GetOuter()->GetWorld()->GetFirstPlayerController());
      if(controllerRef)
      {
         hudManagerRef = controllerRef->GetHUDManager();
         // Widgets may not be initialized due to begin play order so setup next frame.
         GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UQuestManager::SetupWidgetReferences);
      }
   }

   completedQuests.Reserve(4);
   failedQuests.Reserve(4);
   activeQuests.Reserve(4);

   GetWorld()->GetFirstLocalPlayerFromController()->GetSubsystem<UItemDelegateContext>()->OnItemPickedUp().AddUObject(this, &UQuestManager::OnItemPickedUp);
   GetWorld()->GetFirstLocalPlayerFromController()->GetSubsystem<UItemDelegateContext>()->OnItemDropped().AddUObject(this, &UQuestManager::OnItemPickedUp);
   GetWorld()->GetFirstLocalPlayerFromController()->GetSubsystem<UItemDelegateContext>()->OnItemPurchased().AddUObject(this, &UQuestManager::OnItemPurchased);
   GetWorld()->GetFirstLocalPlayerFromController()->GetSubsystem<UItemDelegateContext>()->OnItemUsed().AddUObject(this, &UQuestManager::OnItemPickedUp);

   // TODO: One more listener for when item rewards are given from quests
   NPCDelegateContext::OnNPCConversationEvent.AddUObject(this, &UQuestManager::OnTalkNPC);
   NPCDelegateContext::OnNPCTalkedEvent.AddUObject(this, &UQuestManager::OnTalkNPC, FGameplayTag::EmptyTag);

   GetWorld()->GetFirstLocalPlayerFromController()->GetSubsystem<UGameplayDelegateContext>()->OnInteracted().AddUObject(this, &UQuestManager::OnInteracted);
   GetWorld()->GetFirstLocalPlayerFromController()->GetSubsystem<UGameplayDelegateContext>()->OnUnitDieGlobal().AddUObject(this, &UQuestManager::OnEnemyDie);
}

void UQuestManager::SetupWidgetReferences()
{
   questListRef    = controllerRef->GetHUDManager()->GetIngameHUD()->GetQuestList();
   questJournalRef = controllerRef->GetHUDManager()->GetIngameHUD()->GetQuestJournal();
   questListRef->OnSubgoalSwitched().AddUObject(this, &UQuestManager::OnSubgoalSwitched);
}

void UQuestManager::CompleteSubgoal(AQuest* quest, int goalIndex)
{
   if(quest)
   {
      quest->CompleteSubGoal(goalIndex);
   }
}

bool UQuestManager::AddNewQuest(TSubclassOf<AQuest> questClassToSpawn)
{
   auto condition = [questClassToSpawn](AQuest* quest) {
      return questClassToSpawn == quest->GetClass();
   };

   if(IsValid(questClassToSpawn) && !activeQuests.ContainsByPredicate(condition))
   {
      AQuest* quest =
          questListRef->GetWorld()->SpawnActorDeferred<AQuest>(questClassToSpawn.Get(), FTransform(), nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
      quest->FinishSpawning(FTransform());

      activeQuests.Add(quest);
      quest->SetupStartingGoals();

      OnQuestStartedDelegate.Broadcast(quest);
   }
   return false;
}

void UQuestManager::OnSubgoalSwitched(AQuest* quest, int goalIndex)
{
   if(!HasAuthority() || !IsRunningDedicatedServer())
   {
      if(currentGoalActor)
      {
         currentGoalActor->Destroy();
      }

      if(quest->GetGoalAtIndex(goalIndex)->GetGoalStyleInfo().goalLocation != FGoalStyleInfo::invalidGoalLocation)
      {
         currentGoalActor = controllerRef->GetWorld()->SpawnActorAbsolute<AGoalActor>(
             goalActorClass, FTransform(quest->GetGoalAtIndex(goalIndex)->GetGoalStyleInfo().goalLocation), FActorSpawnParameters());
      }
   }
}

bool UQuestManager::HasAuthority() const
{
   AActor* Outer = Cast<AActor>(GetOuter());
   check(Outer);
   return Outer->HasAuthority();
}

void UQuestManager::OnQuestCompleted(AQuest* questToEnd)
{
   activeQuests.Remove(questToEnd);
   switch(questToEnd->GetQuestState())
   {
      case EQuestState::currentQuests: break;
      case EQuestState::failedQuests: failedQuests.Add(questToEnd); break;
      case EQuestState::completedQuests: completedQuests.Add(questToEnd); break;
   }

   if(questToEnd == questListRef->GetCurrentlySelectedQuest())
   {
      // Also remove the goal actor from the minimap
      if(IsValid(currentGoalActor))
      {
         currentGoalActor->Destroy();
         questListRef->ToggleDistanceIndicatorVisibility(false);
         controllerRef->GetWidgetProvider()->GetIngameHUD()->GetMinimap()->ToggleDirectionArrowVisibility(false);
      }
   }

   if(questToEnd->GetQuestState() == EQuestState::completedQuests)
   {
      controllerRef->GetBasePlayer()->UpdateEXP(questToEnd->GetQuestInfo().questReward.exp);
      controllerRef->GetBasePlayer()->UpdateGold(questToEnd->GetQuestInfo().questReward.gold);
      if(controllerRef->GetBasePlayer()->GetPartyLeader())
      {
         controllerRef->GetBasePlayer()->GetPartyLeader()->GetBackpack().AddItems(questToEnd->GetQuestInfo().questReward.items);
      }
   }
}

void UQuestManager::OnEnemyDie(AUnit* deadUnit)
{
   if(AEnemy* enemy = Cast<AEnemy>(deadUnit))
   {
      // For updating UI, we need to use TaskGraphMainThread
      auto dieFuture = Async(EAsyncExecution::TaskGraphMainThread, [this, enemy]() {
         for(AQuest* quest : activeQuests)
         {
            for(const int goalIndex : quest->GetCurrentGoalIndices())
            {
               if(UUpGoal* goal = quest->GetGoalAtIndex(goalIndex))
               {
                  if(UUpHuntingGoal* huntingGoal = Cast<UUpHuntingGoal>(goal))
                  {
                     if(enemy->GetGameName().EqualTo(huntingGoal->GetEnemyToHuntClass().GetDefaultObject()->GetGameName()))
                     {
                        huntingGoal->SetNumCurrentlyHunted(huntingGoal->GetNumCurrentlyHunted() + 1);
                        if(huntingGoal->GetNumCurrentlyHunted() >= huntingGoal->GetNumberOfEnemiesToHunt())
                        {
                           quest->CompleteSubGoal(goalIndex);
                           return;
                        }
                        questListRef->GetQuestListSlot(quest)->GetGoalEntryWithGoalIndex(goalIndex)->UpdateText();
                     }
                  }
               }
            }

            // Regardless if whether this goal finishes the quest or not, update the quest journal
            if(quest == questJournalRef->GetSelectedQuest())
            {
               questJournalRef->UpdateDetailWindow();
            }
         }
      });
   }
}

void UQuestManager::OnTalkNPC(ANPC* talkedToNPC, FGameplayTag conversationTopic)
{
   auto talkFuture = Async(EAsyncExecution::TaskGraphMainThread, [this, talkedToNPC, conversationTopic]() {
      for(AQuest* quest : activeQuests)
      {
         for(const int goalIndex : quest->GetCurrentGoalIndices())
         {
            if(UUpGoal* goal = quest->GetGoalAtIndex(goalIndex))
            {
               if(UUpTalkGoal* talkGoal = Cast<UUpTalkGoal>(goal))
               {
                  if(talkGoal->GetNPCToTurnInItemsTo() == talkedToNPC->GetClass())
                  {
                     if(talkGoal->GetTopicToTalkAbout() == FGameplayTag::EmptyTag)
                     {
                        quest->CompleteSubGoal(goalIndex);
                        return;
                     }
                     if(talkGoal->GetTopicToTalkAbout() == conversationTopic)
                     {
                        quest->CompleteSubGoal(goalIndex);
                        return;
                     }
                  }
               }
               else if(UUpGatherGoal* gatherGoal = Cast<UUpGatherGoal>(goal))
               {
                  if(conversationTopic == FGameplayTag::RequestGameplayTag("Dialog.Quest.Turn In Rewards"))
                  {
                     if(gatherGoal->GetNumCurrentlyGathered() >= gatherGoal->GetNumberOfItemToGather())
                     {
                        if(TurnInItemsFromGatherGoal(gatherGoal->GetItemToGatherId(), gatherGoal->GetNumberOfItemToGather()))
                        {
                           quest->CompleteSubGoal(goalIndex);
                           return;
                        }
                     }
                  }
               }
            }
         }
      }
   });
}

bool UQuestManager::TurnInItemsFromGatherGoal(int gatherItemId, int numItemsToGather)
{
   int numItemsToRemove = numItemsToGather;
   for(ABaseHero* hero : UUpFunctionLibrary::GetHeroes(this))
   {
      FBackpackUpdateResult res = hero->GetBackpack().RemoveItem(FMyItem(gatherItemId, numItemsToRemove));
      if(!res.bSuccessfulOperation)
      {
         numItemsToRemove = res.numUpdatedItemsRemaining;
      }
      else
      {
         return true;
      }
   }
   return false;
}

void UQuestManager::OnItemPickedUp(const ABaseHero* heroPickingItem, const FBackpackUpdateResult& itemUpdateResult)
{
   if(itemUpdateResult.numUpdatedItemsRequested != itemUpdateResult.numUpdatedItemsRemaining)
   {
      const int     numItemsPickedUp = itemUpdateResult.numUpdatedItemsRequested - itemUpdateResult.numUpdatedItemsRemaining;
      const FMyItem newItem{itemUpdateResult.itemId, numItemsPickedUp};

      auto itemPickupFuture = Async(EAsyncExecution::TaskGraphMainThread, [this, newItem]() {
         RecalculateItemCountsForGoals(newItem);
      });
   }
}

void UQuestManager::OnItemPurchased(const ABaseHero* purchasingHero, const FBackpackUpdateResult& addPurchasedItemResult,
                                    const TArray<FBackpackUpdateResult>& removePaymentItemsResults)
{
   OnItemPickedUp(purchasingHero, addPurchasedItemResult);
   for(const FBackpackUpdateResult& backpackUpdateResult : removePaymentItemsResults)
   {
      OnItemPickedUp(purchasingHero, backpackUpdateResult);
   }
}

void UQuestManager::RecalculateItemCountsForGoals(const FMyItem item)
{
   int  questIndex = 0;
   bool changed    = false; // Flag to know if we modified this goal's item or completed this goal so update it in the quest entry

   for(AQuest* quest : activeQuests)
   {
      for(const int goalIndex : quest->GetCurrentGoalIndices())
      {
         UUpGoal* goal = quest->GetGoalAtIndex(goalIndex);
         if(UUpGatherGoal* gatherGoal = Cast<UUpGatherGoal>(goal))
         {
            if(gatherGoal->GetItemToGatherId() == item.id)
            {
               gatherGoal->SetNumCurrentlyGathered(FMath::Min(gatherGoal->GetNumberOfItemToGather(), item.count));
               if(gatherGoal->GetNPCToTurnInItemsTo() == nullptr) // If we don't have to turn in the items
               {
                  if(gatherGoal->GetNumCurrentlyGathered() == gatherGoal->GetNumberOfItemToGather())
                  {
                     quest->CompleteSubGoal(goalIndex);
                  }
               }
            }
         }

         if(changed)
         {
            questListRef->GetQuestListSlot(quest)->GetGoalEntryWithGoalIndex(goalIndex)->UpdateText();
            questJournalRef->UpdateDetailWindow();
            changed = false;
         }

         ++questIndex;
      }
   }
}

void UQuestManager::OnInteracted(TSubclassOf<AInteractableBase> interactableClass, const FText& decoratorName)
{
   auto itemPickupFuture = Async(EAsyncExecution::TaskGraphMainThread, [this, interactableClass, decoratorName]() {
      for(AQuest* quest : activeQuests)
      {
         for(const int goalIndex : quest->GetCurrentGoalIndices())
         {
            // If this goal is to interact with something, and the interactable's name matches the name in this goal
            if(UUpGoal* goal = quest->GetGoalAtIndex(goalIndex))
            {
               if(UUpInteractGoal* interactGoal = Cast<UUpInteractGoal>(goal))
               {
                  if(interactGoal->GetInteractableClass()->IsChildOf(interactableClass))
                  {
                     if(interactGoal->GetInteractableName().EqualTo(decoratorName) || interactGoal->GetInteractableName().IsEmpty())
                     {
                        quest->CompleteSubGoal(goalIndex);
                        if(quest == questJournalRef->GetSelectedQuest())
                        {
                           questJournalRef->UpdateDetailWindow();
                        }
                        return;
                     }
                  }
               }
            }
         }
      }
   });
}
