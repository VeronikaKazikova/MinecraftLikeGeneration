// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "ChunkSpawner.h"
#include "MCGameInstance.generated.h"


/**
 * 
 */
UCLASS()
class TESTINGMC_API UMCGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:

	//setter for seed value for Perlin noise - set in UI
	UFUNCTION(BlueprintCallable)
	void OnSetSeed(int NewSeed);

	//getter for seed value
	int GetSeedValue();

	bool ShouldDataBeLoaded;
	
	TMap<FVector, FChunkMod> LoadedChanges;
	FVector PlayerStartPosition;

	//saving game to a slot
	UFUNCTION(BlueprintCallable)
	void SaveGameToSlot(FString SlotName);

	//loading game from a slot
	UFUNCTION(BlueprintCallable)
	void LoadGame(FString SlotName);

	UPROPERTY()
	int SeedValue;
	
};
