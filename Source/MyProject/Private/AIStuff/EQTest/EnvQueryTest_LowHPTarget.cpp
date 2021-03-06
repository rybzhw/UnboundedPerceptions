// Fill out your copyright notice in the Description page of Project Settings.

#include "MyProject.h"
#include "EQTest/EnvQueryTest_LowHPTarget.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Actor.h"
#include "Unit.h"
#include "UpStatComponent.h"

UEnvQueryTest_LowHPTarget::UEnvQueryTest_LowHPTarget()
{
   Cost          = EEnvTestCost::Low;
   ValidItemType = UEnvQueryItemType_ActorBase::StaticClass();
}

void UEnvQueryTest_LowHPTarget::RunTest(FEnvQueryInstance& QueryInstance) const
{
   // make sure we have an owner for this test instance
   UObject* QueryOwner = QueryInstance.Owner.Get();
   if (QueryOwner == nullptr) { return; }

   /*Setup min and max test values allowed.  Values are bound for faster access*/

   FloatValueMin.BindData(QueryOwner, QueryInstance.QueryID);
   const float MinThresholdValue = FloatValueMin.GetValue();

   FloatValueMax.BindData(QueryOwner, QueryInstance.QueryID);
   const float MaxThresholdValue = FloatValueMax.GetValue();

   for (FEnvQueryInstance::ItemIterator It(this, QueryInstance); It; ++It) {
      const AActor* itemActor = GetItemActor(QueryInstance, It.GetIndex());
      if (const AUnit* unitRef = Cast<AUnit>(itemActor)) {
         const float health = unitRef->GetStatComponent()->GetVitalCurValue(EVitals::Health);
         It.SetScore(TestPurpose, FilterType, health, MinThresholdValue, MaxThresholdValue);
      }
   }
}

FText UEnvQueryTest_LowHPTarget::GetDescriptionTitle() const
{
   return NSLOCTEXT("EQTest", "LowHPDesc", "FindLowestHP of Units");
}

FText UEnvQueryTest_LowHPTarget::GetDescriptionDetails() const
{
   return DescribeFloatTestParams();
}
