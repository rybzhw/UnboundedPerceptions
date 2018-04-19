// Fill out your copyright notice in the Description page of Project Settings.

#include "MyProject.h"
#include "Spawner.h"

ASpawner::ASpawner() : triggered(false)
{
	//attach two delegates to this function that notifies our class when any actor walks into the edge of our trigger box
	GetCollisionComponent()->OnComponentBeginOverlap.AddDynamic(this, &ASpawner::OnBeginOverlap);
	GetCollisionComponent()->OnComponentEndOverlap.AddDynamic(this, &ASpawner::OnEndOverlap);

	if (!MaxTriggers)
		MaxTriggers = 1;
}

void ASpawner::BeginPlay()
{
	SetActorHiddenInGame(false);
}

void ASpawner::OnBeginOverlap(class UPrimitiveComponent* overlappingComponent, AActor* other, UPrimitiveComponent* box, int32 otherBodyIndex, bool bFromSweep, const FHitResult &hitResult)
{
	if (other && !triggered && MaxTriggers > 0)
	{
		triggered = true;
		FActorSpawnParameters parameters;
		parameters.Owner = this;
		parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		for (int i = 0; i < Count; ++i)
		{
			for (TSubclassOf<class ACharacter> actor : CharactersToSpawn)
			{
				UE_LOG(LogActor, Warning, TEXT("%s"), *actor->GetClass()->GetName());
				if (SpawnLocation)
				{
					GetWorld()->SpawnActor<class ACharacter>(actor, SpawnLocation->GetTransform(), parameters);
				}
			}
		}
		MaxTriggers--;
		/*Player overlapped the NPC spawner, spawn enemies*/
	}
}

void ASpawner::OnEndOverlap(class UPrimitiveComponent* OtherComp, class AActor * OtherActor, UPrimitiveComponent* otherComp, int32 OtherBodyIndex)
{
	ACharacter* pc = Cast<ACharacter>(OtherActor);
	if (pc && triggered)
	{
		triggered = false;
	}
}

