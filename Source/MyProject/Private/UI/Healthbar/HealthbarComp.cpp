#include "MyProject.h"
#include "WorldObjects/Unit.h"
#include "HealthbarComp.h"
#include "Healthbar.h"
#include "UpStatComponent.h"

UHealthbarComp::UHealthbarComp()
{
   SetWidgetSpace(EWidgetSpace::Screen);
}

void UHealthbarComp::BeginPlay()
{
   Super::BeginPlay();
   SetCollisionProfileName("NoCollision");
   healthBar = Cast<UHealthbar>(GetUserWidgetObject());
   unitRef   = Cast<AUnit>(GetOwner());
   unitRef->OnUnitDamageReceived().AddUObject(this, &UHealthbarComp::OnDamageReceived);
   unitRef->OnUnitHealingReceived().AddUObject(this, &UHealthbarComp::OnHealingReceived);
}

void UHealthbarComp::OnDamageReceived(const FUpDamage& damage)
{
   GetWorld()->GetTimerManager().SetTimerForNextTick([this]() {
      healthBar->UpdateHealthbar(unitRef->GetStatComponent()->GetVitalCurValue(EVitals::Health) / unitRef->GetStatComponent()->GetVitalBaseValue(EVitals::Health));
   });
}

void UHealthbarComp::OnHealingReceived(const FUpDamage& damage)
{
   GetWorld()->GetTimerManager().SetTimerForNextTick([this]() {
      healthBar->UpdateHealthbar(unitRef->GetStatComponent()->GetVitalCurValue(EVitals::Health) / unitRef->GetStatComponent()->GetVitalBaseValue(EVitals::Health));
   });
}

void UHealthbarComp::EndPlay(const EEndPlayReason::Type EPR)
{
   Super::EndPlay(EPR);
}
