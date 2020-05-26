#include "MyProject.h"
#include "UpResourceManager.h"
#include "BaseCharacter.h"

TMap<FGameplayTag, FColor> UpResourceManager::elementalMap = TMap<FGameplayTag, FColor>();
FGameplayTagContainer      UpResourceManager::supportTags = FGameplayTagContainer();
FGameplayTagContainer      UpResourceManager::offensiveTags = FGameplayTagContainer();

void UpResourceManager::InitUpResourceManager()
{
   InitElementalMap();
   InitSupportTags();
   InitOffensiveTags();
}

void UpResourceManager::InitElementalMap()
{
   elementalMap.Add(FGameplayTag::RequestGameplayTag("Combat.Element.None"), FColor::White);
   elementalMap.Add(FGameplayTag::RequestGameplayTag("Combat.Element.Arcane"), FColor::Cyan);
   elementalMap.Add(FGameplayTag::RequestGameplayTag("Combat.Element.Blood"), FColor(255, 51, 51));
   elementalMap.Add(FGameplayTag::RequestGameplayTag("Combat.Element.Chaos"), FColor::Purple);
   elementalMap.Add(FGameplayTag::RequestGameplayTag("Combat.Element.Dark"), FColor::Black);
   elementalMap.Add(FGameplayTag::RequestGameplayTag("Combat.Element.Earth"), FColor(210, 180, 140));
   elementalMap.Add(FGameplayTag::RequestGameplayTag("Combat.Element.Electric"), FColor::Yellow);
   elementalMap.Add(FGameplayTag::RequestGameplayTag("Combat.Element.Ethereal"), FColor::Emerald);
   elementalMap.Add(FGameplayTag::RequestGameplayTag("Combat.Element.Fire"), FColor::Red);
   elementalMap.Add(FGameplayTag::RequestGameplayTag("Combat.Element.Force"), FColor(96, 96, 96));
   elementalMap.Add(FGameplayTag::RequestGameplayTag("Combat.Element.Light"), FColor::White);
   elementalMap.Add(FGameplayTag::RequestGameplayTag("Combat.Element.Poison"), FColor(255, 102, 255));
   elementalMap.Add(FGameplayTag::RequestGameplayTag("Combat.Element.Water"), FColor::Blue);
   elementalMap.Add(FGameplayTag::RequestGameplayTag("Combat.Element.Wind"), FColor(51, 255, 153));
}

void UpResourceManager::InitSupportTags() { supportTags.AddTag(FGameplayTag::RequestGameplayTag("Skill.Category.Support")); }

void UpResourceManager::InitOffensiveTags() { offensiveTags.AddTag(FGameplayTag::RequestGameplayTag("Skill.Category.Offensive")); }

float UpResourceManager::FindOrientation(const FVector& v)
{
   bool    positiveX = v.X >= 0;
   FVector up = positiveX ? FVector::RightVector : -FVector::RightVector;
   float   normalizedDot = (up.X * v.X + up.Y * v.Y) / (FMath::Sqrt(v.SizeSquared()));
   return positiveX ? normalizedDot + 2 : normalizedDot;
}

void UpResourceManager::ExecuteFunctionFromWorldObject(UObject* objectRef, FName functionToExecute)
{
   if (objectRef) {
      UFunction* function = objectRef->FindFunction(functionToExecute);
      if (function) {
         // pointer to memory of local variables in stack
         void* locals = nullptr;
         // creates the stack frame for the function
         TUniquePtr<FFrame> frame = TUniquePtr<FFrame>(new FFrame{ objectRef, function, locals });
         // call the UFunction
         // processEvent (if it has params)
         objectRef->CallFunction(*frame, locals, function);
      }
   }
}

template <>
ABaseHero* UpResourceManager::FindTriggerObjectInWorld<ABaseHero>(FString nameToMatch, UWorld* worldRef)
{
   AUserInput* cpcRef = Cast<AUserInput>(worldRef->GetFirstPlayerController());
   for (ABaseHero* hero : cpcRef->GetBasePlayer()->allHeroes) { if (hero->GetGameName().ToString() == nameToMatch) { return Cast<ABaseHero>(hero); } }
   return nullptr;
}