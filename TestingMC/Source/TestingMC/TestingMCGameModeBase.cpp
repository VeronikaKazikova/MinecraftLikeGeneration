// Copyright Epic Games, Inc. All Rights Reserved.


#include "TestingMCGameModeBase.h"
#include "EGroundTypes.h"
#include "ChunkSpawner.h"
#include "Save.h"
#include "ChunkSpawner.h"
#include "Kismet/GameplayStatics.h"

ATestingMCGameModeBase::ATestingMCGameModeBase()
	: Super()
{
	ActiveGroundType = EGroundType::Grass;
}

void ATestingMCGameModeBase::SetActiveGroundType(EGroundType GroundType)
{
	ActiveGroundType = GroundType;
}

EGroundType ATestingMCGameModeBase::GetActiveGroundType()
{
	return ActiveGroundType;
}