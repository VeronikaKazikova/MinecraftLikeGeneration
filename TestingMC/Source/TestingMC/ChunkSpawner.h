// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Chunk.h"
#include "EGroundTypes.h"
#include "ChunkSpawner.generated.h"

USTRUCT(BlueprintType)
struct FChunkMod
{
	GENERATED_BODY()

		UPROPERTY()
		TMap<FVector, EGroundType> ChangesInChunk;
};

UCLASS()
class TESTINGMC_API AChunkSpawner : public AActor
{
	GENERATED_BODY()

public:	
	AChunkSpawner();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	//called from chunks if something has been changed in them, for chunk spawner to remember those changes
	void SetChanges(const FVector& ChunkPos, const FVector& CubePos, EGroundType BlockType);

	//getter for all changes made in world
	TMap<FVector, FChunkMod> GetChanges();

	//returns size of chunk in X/Y axis
	int GetChunkXYSize();

	//will get player's position in our arbitrary chunk coordinates system
	FVector GetPlayerCoordinates() const;

	//last remembered coordinates of player
	FVector LastCoordinates;

	//queues for chunks wainting to be created/removed
	TArray<FVector> CoordToCreateChunks;
	TArray<FVector> CoordsToRemoveChunks;

	//TMAp storing all changes to the chunk, so it can be redrawn or saved to file
	TMap<FVector, FChunkMod> ChunkChanges;

	//references to spawned chunks
	UPROPERTY()
	TArray<AChunk*> SpawnedChunksRefs;

	//coordinates of spawned and used chunks
	TArray<FVector> SpawnedChunksCoordinates;

	//all chunks, even those not used (without instances od ISM) but spawned and waiting for reuse
	TArray<AChunk*> ExistingChunks;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void CreateOneChunk(const FVector Coordinates);

	void RemoveOneChunk(const FVector Coordinates);

	//add collisions to chunk around player if they changes position
	void UpdateCollisions(const FVector& CurrentCoordinates);

	//calculate which chunks needs to be added to the queue, wainting to be added, after player has moved
	void ComputeChunks();

	//mark chunks which are beyond sphere of render range as ready to be removed, after player has moved
	void ComputeChunksToRemove();

	FVector UpdateChunks();

	UPROPERTY(EditAnywhere)
	int ChunkSize = 1100;

	//units = chunks
	UPROPERTY(EditAnywhere)
	int RenderRange = 12;

	//voxel size 
	float ChunkVoxelSize = 100;

	UPROPERTY(EditAnywhere)
	TSubclassOf<AChunk> ChunkClass;

};
