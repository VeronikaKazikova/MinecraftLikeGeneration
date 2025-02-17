// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "ChunkSpawner.h"
#include "Save.generated.h"

/**
 * 
 */
UCLASS()
class TESTINGMC_API USave : public USaveGame
{
	GENERATED_BODY()

public:
	//save file consists of player position and TMap of all changes made to chunk
	UPROPERTY(EditAnywhere)
	TMap<FVector, FChunkMod>  SavedChanges;

	UPROPERTY(EditAnywhere)
	FVector PlayerLocation = FVector(0 ,0, 0);
	
};
