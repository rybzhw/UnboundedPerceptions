// Fill out your copyright notice in the Description page of Project Settings.

#include "MyProject.h"
#include "State/HeroStateMachine.h"
#include "WorldObjects/BaseHero.h"

HeroStateMachine::HeroStateMachine(ABaseHero* hero) :
   RTSStateMachine(hero), Interacting{InteractState(hero)}, UsingItem{ItemState(hero)} {}

HeroStateMachine::~HeroStateMachine() {}

void HeroStateMachine::ChangeState(EUnitState newState)
{
   // if we currently have a state in our state machine
   if (!currentState) { GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, TEXT("Error, currentState is nullptr")); }

   if (newState != currentState->GetName()) // make sure we're not just switching to the same state >_>
   {
      currentState->Exit(*unitOwner);
      currentState = getStateFromEnum(newState);
      currentState->Enter(*unitOwner);
   }
}

IUnitState* HeroStateMachine::getStateFromEnum(EUnitState enumVal)
{
   switch (enumVal) {
      case EUnitState::STATE_IDLE: return &Idle;
      case EUnitState::STATE_ATTACKING: return &Attacking;
      case EUnitState::STATE_ATTACK_MOVE: return &AttackMove;
      case EUnitState::STATE_CASTING: return &Casting;
      case EUnitState::STATE_INTERACTING: return &Interacting;
      case EUnitState::STATE_ITEM: return &UsingItem;
      case EUnitState::STATE_CHANNELING: return &Channeling;
      case EUnitState::STATE_INCANTATION: return &Incanting;
      case EUnitState::STATE_MOVING: return &Moving;
      case EUnitState::STATE_CHASING: return &Chasing;
      default: return nullptr;
   }
}
