// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TestingMCGameModeBase.generated.h"

/**
 * 
 */
enum class EGroundType : uint8;

UCLASS()
class TESTINGMC_API ATestingMCGameModeBase : public AGameModeBase
{
	GENERATED_BODY()
	
public:
	ATestingMCGameModeBase();

	//called when player selects type of ground from UI
	void SetActiveGroundType(EGroundType GroundType);

	//returns enum of currenctly selected ground type
	EGroundType GetActiveGroundType();

	class AChunkSpawner* ChunkSpawnerRef;

	//enum for different types of cubes
	EGroundType ActiveGroundType;
	
};
