// Fill out your copyright notice in the Description page of Project Settings.

#include "MyProject.h"
#include "DIRender.h"
#include "UserInput.h"
#include "Unit.h"
#include "RTSPawn.h"

UDIRender::UDIRender()
{
   static ConstructorHelpers::FObjectFinder<UCurveFloat> curveFinder(TEXT("/Game/RTS_Tutorial/Blueprints/Effects/DamageIndicator/DICurve"));
   if (curveFinder.Succeeded()) {
      FOnTimelineFloat animDI;
      animDI.BindUFunction(this, FName("TimelineEffect"));
      tL.AddInterpFloat(curveFinder.Object, animDI, FName(TEXT("TimelineEffect")));
      tL.SetTimelineLengthMode(ETimelineLengthMode::TL_LastKeyFrame);
      tL.SetLooping(false);
      FOnTimelineEvent e;
      e.BindUFunction(this, "OnTimelineFinished");
      tL.SetTimelineFinishedFunc(e);
   }

   static ConstructorHelpers::FObjectFinder<UMaterialInstance> materialFinder(TEXT("/Game/RTS_Tutorial/Materials/Text/MAT_UnlitText_Inst"));
   if (materialFinder.Succeeded()) SetTextMaterial(materialFinder.Object);

   PrimaryComponentTick.bCanEverTick = true;
   bAllowConcurrentTick = true;
   bCanEverAffectNavigation = false;
}

void UDIRender::BeginPlay()
{
   Super::BeginPlay();
   cameraPawnRef = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
   tL.PlayFromStart();
   start = FVector::UpVector * (GetOwner()->GetSimpleCollisionHalfHeight() - 20) + FVector(FMath::RandRange(-20,20), FMath::RandRange(-20,20), 0);
   end = start + FVector(0, 0, jumpHeight);
   SetWorldScale3D(FVector(2.f, 2.f, 2.f));
}

void UDIRender::TickComponent(float deltaSeconds, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction)
{
   // TODO: Make this look better and remove tick and replace with timer?
   Super::TickComponent(deltaSeconds, tickType, thisTickFunction);
   // Ensure damage values always facing camera even when camera is rotated
   SetWorldRotation(FRotator(0, -180, 0) + cameraPawnRef->GetActorRotation());
   tL.TickTimeline(deltaSeconds);
}

void UDIRender::TimelineEffect(float val)
{
   const FVector newLocation = FMath::Lerp(start, end, val);
   SetRelativeLocation(newLocation);
}

void UDIRender::OnTimelineFinished()
{
   DestroyComponent(false);
}
