﻿#pragma once
#include "CoreMinimal.h"

bool bEnableEnemyControl = 0;
static FAutoConsoleVariableRef CVarEnableEnemyControl(
	TEXT("enableEnemyControl"), 
	bEnableEnemyControl, 
	TEXT("When this flag is set to 1, we change the control scheme to allow enemy controls")
);