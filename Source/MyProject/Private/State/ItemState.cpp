// Fill out your copyright notice in the Description page of Project Settings.

#include "MyProject.h"
#include "ItemState.h"

ItemState::ItemState(ABaseHero* hero) { heroRef = hero; }

ItemState::~ItemState() {}

void ItemState::Enter(AUnit& unit) {}

void ItemState::Exit(AUnit& unit) {}

void ItemState::Update(AUnit& unit, float deltaSeconds) {}
