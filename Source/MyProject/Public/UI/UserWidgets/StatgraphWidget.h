#pragma once

#include "CoreMinimal.h"
#include "UI/UserWidgetExtensions/RTSBrowserWidgetBase.h"
#include "RTSAttributeSet.h"
#include "StatgraphWidget.generated.h"

class AUnit;

/**
 * Widget that displays our stats in a bar graph format
 */
UCLASS()
class MYPROJECT_API UStatgraphWidget : public URTSBrowserWidgetBase
{
   GENERATED_BODY()

 public:
   /**
   * Sends data to the browser relating to elemental affinities
   */
   UFUNCTION(BlueprintCallable)
   void ShowElementalOffensive();

   /**
   * Sends data to the browser relating to elemental defense
   */
   UFUNCTION(BlueprintCallable)
   void ShowElementalDefense();

   /**
   * Sends data to the browser relating to mechanics
   */
   UFUNCTION(BlueprintCallable)
   void ShowMechanics();

   /**
   * Sends data to the browser relating to vitals
   */
   UFUNCTION(BlueprintCallable)
   void ShowVitals();

   /**
   * Changes the active hero to show information for. Called when the character menu initially opens. Currently not used since I removed the title to save space, but
   * if we do add being able to change the hero who's stats are being shown in CharacterMenu it will see use again.
   */
   UFUNCTION(BlueprintCallable)
   void SwapHero();

   /**
   * Call this once to display browser when widget starts up and to setup listener to hero stats
   */
   UFUNCTION(BlueprintCallable)
   void Startup();

   /**
   * Call this to unregister listener to hero when the widget is closed
   */
   UFUNCTION(BlueprintCallable)
   void Cleanup();

 protected:
   void NativeOnInitialized() override final;

   UPROPERTY(EditDefaultsOnly)
   int width;

   UPROPERTY(EditDefaultsOnly)
   int height;

   // Listens to get updates when the hero whose character menu is currently displayed has their stat changed. First value is for adj val updates, second value is for base val updates
   TPair<FDelegateHandle, FDelegateHandle> statListeners;

 private:
   /**
    * Sends data to the browser to update one of the stats. If the stat is on screen, it will be updated
    */
   void UpdateStat(const FGameplayAttribute& attributeModified, float newAttributeValue, AUnit* unitAffected);

   /**
   * Sends data to the browser to update one of the base stats. If the stat is on screen, it will be updated
   */
   void UpdateBaseStat(const FGameplayAttribute& attributeModified, float newAttributeValue, AUnit* unitAffected);

   /**
    * Helper function for Updating and sending Stats to Browser
    */
   void CreateAndSendStatUpdate(const FGameplayAttribute& attributeModified, float newAttributeValue, AUnit* unitAffected, const FString& keyName);

   FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override final;

   /**
    * A check to see if the focused unit is a hero. In theory this UI will only be open if a hero is focused but ya never know
    */
   const AUnit* CheckIfFocusedUnitHero() const;

   UPROPERTY(BlueprintReadOnly, Category = "References", Meta = (AllowPrivateAccess = "true"))
   class AUserInput* cpcRef;
};
