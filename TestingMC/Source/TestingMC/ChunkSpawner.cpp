// Fill out your copyright notice in the Description page of Project Settings.


#include "ChunkSpawner.h"
#include "Kismet/GameplayStatics.h"
#include "MCGameInstance.h"
#include "Engine/World.h"
#include "EGroundTypes.h"
#include "Math/Rotator.h"
#include "GenericPlatform/GenericPlatformMath.h"
#include "Math/Vector.h"
#include "MCGameInstance.h"
#include "Chunk.h"

// Sets default values
AChunkSpawner::AChunkSpawner()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
}

// Called when the game starts or when spawned
void AChunkSpawner::BeginPlay()
{
	Super::BeginPlay();

	//check if we are loading data or creating a new game
	UMCGameInstance* CurrentGameInstance = Cast<UMCGameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
	if (CurrentGameInstance->ShouldDataBeLoaded)
	{
		ChunkChanges = Cast<UMCGameInstance>(UGameplayStatics::GetGameInstance(GetWorld()))->LoadedChanges;
		CurrentGameInstance->ShouldDataBeLoaded = false;
	}
	//remmeber player coordinates, calculate which chunks need to be created and turn on collisions around player
	LastCoordinates = UpdateChunks();
}

int AChunkSpawner::GetChunkXYSize()
{
	return ChunkSize;
}

// Called every frame
void AChunkSpawner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	//every tick we check if there are any chunks pending to be drawn or removed, and if yes, draw/remove one
	if (!CoordToCreateChunks.IsEmpty())
	{
		CreateOneChunk(CoordToCreateChunks.Last());
	}

	if (!CoordsToRemoveChunks.IsEmpty())
	{
		RemoveOneChunk(CoordsToRemoveChunks.Last());
	}
	UpdateCollisions(GetPlayerCoordinates());

	//if player coordinates changes in arbitrary coord system - so only if playes goes to another chunk, recalculare which chunks needs to be removed/drawn and add them to queue. Also add collisions around player
	if ((GetPlayerCoordinates().X != LastCoordinates.X) || (GetPlayerCoordinates().Y != LastCoordinates.Y))
	{
		LastCoordinates = UpdateChunks();
	}
}

FVector AChunkSpawner::UpdateChunks()
{
	FVector RecallculatedLastCoordinates = GetPlayerCoordinates();
	ComputeChunksToRemove();
	ComputeChunks();
	UpdateCollisions(GetPlayerCoordinates());
	return RecallculatedLastCoordinates;
}

TMap<FVector, FChunkMod> AChunkSpawner::GetChanges()
{
	return ChunkChanges;
}

void AChunkSpawner::UpdateCollisions(const FVector& CurrentCoordinates)
{
	for (int i = -1; i <= 1; i++)
	{
		for (int j = -1; j <= 1; j++)
		{
			if (SpawnedChunksCoordinates.Contains(FVector(CurrentCoordinates.X + i, CurrentCoordinates.Y + j, 0)))
			{
				int Index = SpawnedChunksCoordinates.Find(FVector(CurrentCoordinates.X + i, CurrentCoordinates.Y + j, 0 ));
				
				SpawnedChunksRefs[Index]->GetInstancedMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			}
		}
	}
}

FVector AChunkSpawner::GetPlayerCoordinates() const
{
	//devide location bz chunk size and round it, to get "number" of chunk this position belongs to
	FVector WorldLocation = UGameplayStatics::GetPlayerPawn(this, 0)->GetActorLocation();
	FVector Coords = WorldLocation / ChunkSize;
	Coords.X = FGenericPlatformMath::RoundToInt(Coords.X);
	Coords.Y = FGenericPlatformMath::RoundToInt(Coords.Y);
	Coords.Z = FGenericPlatformMath::RoundToInt(Coords.Z);
	return Coords;
}

void AChunkSpawner::ComputeChunksToRemove()
{
	for (int i = 0; i < SpawnedChunksCoordinates.Num(); i++)
	{
		float ActorChunkDist = FVector::Dist(FVector(SpawnedChunksCoordinates[i].X, SpawnedChunksCoordinates[i].Y, 0), FVector(GetPlayerCoordinates().X, GetPlayerCoordinates().Y, 0));
		if (ActorChunkDist > RenderRange)
		{
			CoordsToRemoveChunks.Add(SpawnedChunksCoordinates[i]);
		}
	}
}

void AChunkSpawner::ComputeChunks()
{
	for (int i = -RenderRange; i <= RenderRange; i++)
	{
		for (int j = -RenderRange; j <= RenderRange; j++)
		{
			FVector CurrentCoords = GetPlayerCoordinates();
			FVector CurrentCoordsInXYPlane = FVector(CurrentCoords.X, CurrentCoords.Y, 0);
			FVector ChunkCoordinates = FVector((CurrentCoords.X + i), (CurrentCoords.Y + j), 0);
			FVector SpawnPosition = FVector(ChunkCoordinates.X * ChunkSize, ChunkCoordinates.Y * ChunkSize, 0);

			float ChunkToPlayerVectorDist = FVector::Dist(CurrentCoordsInXYPlane * ChunkSize, SpawnPosition);

			if (!SpawnedChunksCoordinates.Contains(ChunkCoordinates) && (ChunkToPlayerVectorDist <= (ChunkSize * RenderRange)) && !CoordToCreateChunks.Contains(ChunkCoordinates))
			{
				CoordToCreateChunks.Add(ChunkCoordinates);
			}
		}
	}
}

void AChunkSpawner::CreateOneChunk(const FVector Coordinates)
{
	bool NeedToCreateNewChunk = true;
	//if there is available chunk - spawned but without instances, waiting to be used
	for (AChunk* ExistingChunk : ExistingChunks)
	{
		if (!ExistingChunk->IsUsed)
		{
			//take it, mark it as used, change its position, add ISMs, recalculate ground types, add chunk coordinates to SpawnedChunksCoordinates
			ExistingChunk->IsUsed = true;
			ExistingChunk->SetActorLocation(FVector(Coordinates.X * ChunkSize, Coordinates.Y * ChunkSize, 0));
			ExistingChunk->MyCoords = Coordinates;
			ExistingChunk->GenerateTerrain();
			ExistingChunk->DrawTerrain();
			SpawnedChunksCoordinates.Add(Coordinates);
			SpawnedChunksRefs.Add(ExistingChunk);
			NeedToCreateNewChunk = false;
			CoordToCreateChunks.Remove(Coordinates);
			break;
		}
	}

	//if there is no available created chunk, create new
	if (NeedToCreateNewChunk)
	{
		SpawnedChunksCoordinates.Add(Coordinates);

		FVector SpawnPosition = FVector(Coordinates.X * ChunkSize, Coordinates.Y * ChunkSize, 0);
		if (ChunkClass)
		{
			AChunk* NewChunk = GetWorld()->SpawnActor<AChunk>(ChunkClass, SpawnPosition, FRotator::ZeroRotator);
			SpawnedChunksRefs.Add(NewChunk);
			ExistingChunks.Add(NewChunk);
			//if the chunk is one chunk away from player, set colision enabled
			if (Coordinates.X - GetPlayerCoordinates().X <= 1 || Coordinates.Y - GetPlayerCoordinates().Y)
			{
				NewChunk->SetActorEnableCollision(true);
			}
		}
		CoordToCreateChunks.Remove(Coordinates);
	}
}

void AChunkSpawner::RemoveOneChunk(const FVector Coordinates)
{
	int IndexToRemove = SpawnedChunksCoordinates.Find(Coordinates);
	
	//if chunks are not in range anymore, dont destroy them, just remove instances and data and mark them as ready to be reused
	
	SpawnedChunksRefs[IndexToRemove]->IsUsed = false;
	SpawnedChunksRefs[IndexToRemove]->UninitializeChunk();
	SpawnedChunksCoordinates.RemoveAt(IndexToRemove);
	SpawnedChunksRefs.RemoveAt(IndexToRemove);
	CoordsToRemoveChunks.Remove(Coordinates);
}

void AChunkSpawner::SetChanges(const FVector& ChunkPos, const FVector& CubePos, EGroundType BlockType)
{
	//find will return null ptr if there is no value with that key, otherwise pointer to value with that key
	FChunkMod TestChunkMod;
	FChunkMod* ChangedChunk = ChunkChanges.Find(ChunkPos);
	
	if (ChangedChunk)
	{
		//I dont have to check it this cube has been modified previously, bc if this key exist, it will only change its value (keys are unique)
		ChangedChunk->ChangesInChunk.Add(CubePos, BlockType);
	}
	else
	{
		//if this chunk coordinates is not already in Tmap, create new key-value
		FChunkMod NewChangedChunk;
		NewChangedChunk.ChangesInChunk.Add(CubePos, BlockType);
		ChunkChanges.Add(ChunkPos, NewChangedChunk);
	}
}


