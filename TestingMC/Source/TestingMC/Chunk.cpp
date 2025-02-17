// Fill out your copyright notice in the Description page of Project Settings.

#include "Chunk.h"
#include "Components/BoxComponent.h"
#include "GenericPlatform/GenericPlatformMath.h"
#include "PerlinNoise/FastNoiseLite.h"
#include "EGroundTypes.h"
#include "MCGameInstance.h"
#include "ChunkSpawner.h"
#include "GameFramework/Actor.h"
#include "TestingMCGameModeBase.h"
#include <Runtime/Engine/Classes/Kismet/GameplayStatics.h>


// Sets default values
AChunk::AChunk()
{
	PrimaryActorTick.bCanEverTick = false;

	IsUsed = true;
	InstancedMesh = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("ChunkMesh"));
	SetRootComponent(InstancedMesh);
	InstancedMesh->SetCastShadow(false);
	InstancedMesh->SetMobility(EComponentMobility::Movable);
	//at the begining all collisions are turned off and only collisions around player will be turned on
	InstancedMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}


void AChunk::BeginPlay()
{
	Super::BeginPlay();
	ChunkManager = Cast<AChunkSpawner>(UGameplayStatics::GetActorOfClass(GetWorld(), AChunkSpawner::StaticClass()));
	CalculateMyCoordinates();
	//take seed from gameinstance to generate and draw terrain
	Seed = Cast<UMCGameInstance>(UGameplayStatics::GetGameInstance(this))->GetSeedValue();;
	GenerateTerrain();
	DrawTerrain();
}

void AChunk::CalculateMyCoordinates()
{
	int ChunkSize = ChunkManager->GetChunkXYSize();
	MyCoords = FVector(FGenericPlatformMath::RoundToInt(GetActorLocation().X / ChunkSize), FGenericPlatformMath::RoundToInt(GetActorLocation().Y / ChunkSize), FGenericPlatformMath::RoundToInt(GetActorLocation().Z / ChunkSize));
}

void AChunk::SetSeed(int NewSeed)
{
	Seed = NewSeed;
}

UInstancedStaticMeshComponent* AChunk::GetInstancedMesh()
{
	return InstancedMesh;
}

/*this code is not used at the moment,but is left here for future possible cave implementation*/
float AChunk::Apply3DNoise(int x, int y, float Cutoff, float InstZPos)
{
	//setup noise generation
	FastNoiseLite CaveNoise;
	CaveNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
	CaveNoise.SetSeed(Seed);
	CaveNoise.SetFrequency(CaveNoiseFrequency);

	//generate noise
	float Noise3D = CaveNoise.GetNoise((float)(x * VoxelSize), (float)(y * VoxelSize), (float)InstZPos);

	if ((Noise3D > Cutoff))
	{
		return Noise3D;
	}
	else
	{
		return InstZPos;
	}

}

void AChunk::SetGroundValue(int x, int y, int z, int TopCube)
{
	const int MinAltitudeForStone = 32;
	const int SurfaceLayer = 3;
	//decide which terrain for cube based on height
	//lowest indestructible floor
	if (z == 0)
	{
		//stone
		CubeTypesArray[x][y][z] = EGroundType::Stone;
	}
	else if (z > 0 && z < TopCube - SurfaceLayer)
	{
		//layer between bottom and surface
		CubeTypesArray[x][y][z] = EGroundType::Dirt;
		
	}
	else if (z >= TopCube - SurfaceLayer && z <= TopCube && z <= MinAltitudeForStone)
	{
		//surface grass layer
		CubeTypesArray[x][y][z] = EGroundType::Grass;
	}
	if (z >= TopCube - SurfaceLayer && z <= TopCube && z > MinAltitudeForStone)
	{
		//surface layer above particular altitude is different texture
		CubeTypesArray[x][y][z] = EGroundType::Stone;
	}
	if (z > TopCube)
	{
		//rest of the chunk is air
		CubeTypesArray[x][y][z] = EGroundType::Air;
	}
}

FastNoiseLite AChunk::Setup2DNoise()
{
	FastNoiseLite Noise;
	Noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
	Noise.SetSeed(Seed);
	Noise.SetFrequency(Frequency);
	return Noise;
}

int AChunk::Generate2DNoise(int x, int y, FastNoiseLite* Noise)
{
	FVector PreNoiseCoordinatesInChunk = FVector(0, 0, 0);
	PreNoiseCoordinatesInChunk.X = floor(GetActorLocation().X / VoxelSize) + x;
	PreNoiseCoordinatesInChunk.Y = floor(GetActorLocation().Y / VoxelSize) + y;
	FVector CubeScale = FVector(1, 1, 1);
	float PerlinHeight = Noise->GetNoise((float)(PreNoiseCoordinatesInChunk.X * VoxelSize), (float)(PreNoiseCoordinatesInChunk.Y * VoxelSize));
	float InstanceZPos = ((FGenericPlatformMath::RoundToInt(PerlinHeight * NoiseScale))) * VoxelSize;
	const int NoiseOffset = 30;
	return InstanceZPos / VoxelSize + NoiseOffset;
}

void  AChunk::GenerateTerrain()
{
	// Create and configure FastNoise object
	FastNoiseLite Noise = Setup2DNoise();
	
	//going throught all x, y coordinates to generate perlin noise and get height
	for (int x = -ChunkHalfWidth; x <= ChunkHalfWidth; x++)
	{
		for (int y = -ChunkHalfWidth; y <= ChunkHalfWidth; y++)
		{
			GenerateOneColumn(x, y, &Noise);
		}
	}
}

void AChunk::GenerateOneColumn(int x, int y, FastNoiseLite* Noise)
{
	int ThirdIndex = Generate2DNoise(x, y, Noise);
	if (ChunkManager)
	{
		//check if there were any changes made and saved to that particular cube
		if (FChunkMod* ThisChunkMods = ChunkManager->ChunkChanges.Find(MyCoords))
		{
			for (int z = 0; z < ChunkTotalHeight; z++)
			{
				if (ThisChunkMods->ChangesInChunk.Find(FVector(x, y, z)))
				{
					CubeTypesArray[x + ChunkHalfWidth][y + ChunkHalfWidth][z] = ThisChunkMods->ChangesInChunk[FVector(x, y, z)];
				}
				else
				{
					SetGroundValue(x + ChunkHalfWidth, y + ChunkHalfWidth, z, ThirdIndex);
				}
			}
		}
		else
		{
			//when there were no changes, generate new value for terrain based on perlin noise generated height
			for (int z = 0; z < ChunkTotalHeight; z++)
			{
				SetGroundValue(x + ChunkHalfWidth, y + ChunkHalfWidth, z, ThirdIndex);
			}
		}
	}
}


void AChunk::UninitializeChunk()
{
	InstancedMesh->ClearInstances();
}


void AChunk::AddInstanceAndMaterial(const FRotator& Rotation, const FVector& Position, EGroundType GroundType)
{
	FVector CubeScale = FVector(1, 1, 1);
	int Index = InstancedMesh->AddInstance(FTransform(Rotation, Position, CubeScale));

	//switch on enum to generate custom data to pass to ISM for material selection
	//this custom data will be then used in material appplied to cubes - M_CubesMaterial, where the texture is decided based on this data
	TArray<float> CustomData;
	switch (GroundType)
	{
		case EGroundType::Grass:
		{
			CustomData = {1};
		}
		break;
		case EGroundType::Dirt:
		{
			CustomData = {0};
		}
		break;
		case EGroundType::Stone:
		{
			CustomData = {2};
		}
		break;
	}
	InstancedMesh->SetCustomData(Index, CustomData);
}

void AChunk::DrawUpperSide(EGroundType GroundTypeToDraw, int i, int j, int k)
{
	//condition for drawing upper side
	if (k != ChunkHeight - 1 && CubeTypesArray[i + ChunkHalfWidth][j + ChunkHalfWidth][k + 1] == EGroundType::Air)
	{
		AddInstanceAndMaterial(FRotator::ZeroRotator, FVector(VoxelSize * (i), VoxelSize * (j),
			k * VoxelSize + InstanceOffsetFromCubeMiddle), GroundTypeToDraw);
	}
}

void AChunk::DrawLowerSide(EGroundType GroundTypeToDraw, int i, int j, int k)
{
	//condition for drawing lower side
	if (k != 0 && CubeTypesArray[i + ChunkHalfWidth][j + ChunkHalfWidth][k - 1] == EGroundType::Air)
	{
		AddInstanceAndMaterial(FRotator(0, 0, 180), FVector(VoxelSize * (i), VoxelSize * (j),
			k * VoxelSize - InstanceOffsetFromCubeMiddle), GroundTypeToDraw);
	}
	else if (k == 0)
	{
		//if its bottom of the chunk its always drawn and undestructable
		AddInstanceAndMaterial(FRotator(0, 0, 0), FVector(VoxelSize * (i), VoxelSize * (j),
			k * VoxelSize - InstanceOffsetFromCubeMiddle), GroundTypeToDraw);
	}
}

void AChunk::DrawCubeSides(EGroundType GroundTypeToDraw, int i, int j, int k)
{
	//conditions for drawing remaining 4 sides of the cube
	//1
	if (i != ChunkHalfWidth && CubeTypesArray[i + 1 + ChunkHalfWidth][j + ChunkHalfWidth][k] == EGroundType::Air)
	{
		AddInstanceAndMaterial(FRotator(0, 270, 90), FVector(VoxelSize * (i)+InstanceOffsetFromCubeMiddle, VoxelSize * (j),
			k * VoxelSize), GroundTypeToDraw);
	}
	else if (i == ChunkHalfWidth)
	{
		AddInstanceAndMaterial(FRotator(0, 270, 90), FVector(VoxelSize * (i)+InstanceOffsetFromCubeMiddle, VoxelSize * (j),
			k * VoxelSize), GroundTypeToDraw);
	}

	//2
	if (i != -ChunkHalfWidth && CubeTypesArray[i - 1 + ChunkHalfWidth][j + ChunkHalfWidth][k] == EGroundType::Air)
	{
		AddInstanceAndMaterial(FRotator(270, 180, 0), FVector(VoxelSize * (i)-InstanceOffsetFromCubeMiddle, VoxelSize * (j),
			k * VoxelSize), GroundTypeToDraw);
	}
	else if (i == -ChunkHalfWidth)
	{
		AddInstanceAndMaterial(FRotator(270, 180, 0), FVector(VoxelSize * (i)-InstanceOffsetFromCubeMiddle, VoxelSize * (j),
			k * VoxelSize), GroundTypeToDraw);
	}

	//3
	if (j != ChunkHalfWidth && CubeTypesArray[i + ChunkHalfWidth][j + 1 + ChunkHalfWidth][k] == EGroundType::Air)
	{
		AddInstanceAndMaterial(FRotator(90, 270, 0), FVector(VoxelSize * (i), VoxelSize * (j)+InstanceOffsetFromCubeMiddle,
			k * VoxelSize), GroundTypeToDraw);
	}
	else if (j == ChunkHalfWidth)
	{
		AddInstanceAndMaterial(FRotator(90, 270, 0), FVector(VoxelSize * (i), VoxelSize * (j)+InstanceOffsetFromCubeMiddle,
			k * VoxelSize), GroundTypeToDraw);
	}

	//4
	if (j != -ChunkHalfWidth && CubeTypesArray[i + ChunkHalfWidth][j - 1 + ChunkHalfWidth][k] == EGroundType::Air)
	{
		AddInstanceAndMaterial(FRotator(270, -90, 0), FVector(VoxelSize * (i), VoxelSize * (j)-InstanceOffsetFromCubeMiddle,
			k * VoxelSize), GroundTypeToDraw);
	}
	else if (j == -ChunkHalfWidth)
	{
		AddInstanceAndMaterial(FRotator(270, -90, 0), FVector(VoxelSize * (i), VoxelSize * (j)-InstanceOffsetFromCubeMiddle,
			k * VoxelSize), GroundTypeToDraw);
	}
}

void AChunk::DrawOneCube(int i, int j, int k)
{
	EGroundType GroundTypeToDraw = CubeTypesArray[i + ChunkHalfWidth][j + ChunkHalfWidth][k];
	
	DrawUpperSide(GroundTypeToDraw, i, j, k);
	DrawLowerSide(GroundTypeToDraw, i, j, k);
	DrawCubeSides(GroundTypeToDraw, i, j, k);
}

void AChunk::DrawTerrain()
{
	//go through all generated ground types in 3d array
	for (int i = -ChunkHalfWidth; i <= ChunkHalfWidth; i++)
	{
		for (int j = -ChunkHalfWidth; j <= ChunkHalfWidth; j++)
		{
			for (int k = 0; k < ChunkHeight; k++)
			{
				if (CubeTypesArray[i + ChunkHalfWidth][j + ChunkHalfWidth][k] != EGroundType::Air)
				{
					DrawOneCube(i, j, k);
				}
			}
		}
	}
}

void AChunk::TraceAndRemoveSide(const FVector& EndTrace, const FVector& PointInCube)
{
	//prepare linetracing parameters
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(UGameplayStatics::GetPlayerPawn(this, 0));

	FHitResult UpResults;
	if (GetWorld()->LineTraceSingleByChannel(UpResults, EndTrace, PointInCube, ECollisionChannel::ECC_GameTraceChannel2, Params))
	{
		InstancedMesh->RemoveInstance(UpResults.Item);
	}
}

void AChunk::ManageUpperSideAfterRemoving(int i, int j, int k, const FVector& PointInCube)
{
	//if there is air block aboveremoved cube, remove the face
	if ((k < (ChunkTotalHeight - 1) && CubeTypesArray[i + ChunkHalfWidth][j + ChunkHalfWidth][k + 1] == EGroundType::Air) || k == ChunkTotalHeight - 1)
	{
		
		TraceAndRemoveSide(FVector(PointInCube.X, PointInCube.Y, PointInCube.Z + VoxelSize), PointInCube);
	}
	//and when there is block, add face to it
	else if (k < (ChunkTotalHeight - 1) && CubeTypesArray[i + ChunkHalfWidth][j + ChunkHalfWidth][k + 1] != EGroundType::Air)
	{
		FVector WorldPos = VoxelSize * FVector(i, j, k + 1);
		WorldPos = FVector(WorldPos.X, WorldPos.Y, WorldPos.Z - InstanceOffsetFromCubeMiddle);
		EGroundType ActiveGround = CubeTypesArray[i + ChunkHalfWidth][j + ChunkHalfWidth][k + 1];
		AddInstanceAndMaterial(FRotator::ZeroRotator, WorldPos, ActiveGround);
	}
}

void AChunk::ManageLowerSideAfterRemoving(int i, int j, int k, const FVector& PointInCube)
{
	//if there is air block under removed cube, remove the face
	if ((k > 0 && CubeTypesArray[i + ChunkHalfWidth][j + ChunkHalfWidth][k - 1] == EGroundType::Air) || k == 0)
	{
		TraceAndRemoveSide(FVector(PointInCube.X, PointInCube.Y, PointInCube.Z - VoxelSize), PointInCube);
	}
	//and when there is block, add face to it
	else if (k > 0 && CubeTypesArray[i + ChunkHalfWidth][j + ChunkHalfWidth][k - 1] != EGroundType::Air)
	{
		FVector WorldPos = 100 * FVector(i, j, k - 1);
		WorldPos = FVector(WorldPos.X, WorldPos.Y, WorldPos.Z + InstanceOffsetFromCubeMiddle);
		EGroundType ActiveGround = CubeTypesArray[i + ChunkHalfWidth][j + ChunkHalfWidth][k - 1];
		AddInstanceAndMaterial(FRotator::ZeroRotator, WorldPos, ActiveGround);
	}
}

void AChunk::ManageSidesAfterRemoving(int i, int j, int k, const FVector& PointInCube)
{
	//first side
	//if there is air block on the side of removed cube, remove the face
	if ((i < (ChunkHalfWidth) && CubeTypesArray[i + ChunkHalfWidth + 1][j + ChunkHalfWidth][k] == EGroundType::Air) || i == (ChunkHalfWidth))
	{
		//raytrace +x
		TraceAndRemoveSide(FVector(PointInCube.X + VoxelSize, PointInCube.Y, PointInCube.Z), PointInCube);
	}
	//and when there is block, add face to it
	else if (i < (ChunkHalfWidth) && CubeTypesArray[i + ChunkHalfWidth + 1][j + ChunkHalfWidth][k] != EGroundType::Air)
	{
		FVector WorldPos = 100 * FVector(i + 1, j, k);
		WorldPos = FVector(WorldPos.X - InstanceOffsetFromCubeMiddle, WorldPos.Y, WorldPos.Z);
		EGroundType ActiveGround = CubeTypesArray[i + ChunkHalfWidth + 1][j + ChunkHalfWidth][k];
		AddInstanceAndMaterial(FRotator(0, 90, 90), WorldPos, ActiveGround);
	}

	//second side
	//if there is air block on the side of removed cube, remove the face
	if ((i > -ChunkHalfWidth && CubeTypesArray[i + ChunkHalfWidth - 1][j + ChunkHalfWidth][k] == EGroundType::Air) || i == -ChunkHalfWidth)
	{
		//raytrace -x
		TraceAndRemoveSide(FVector(PointInCube.X - VoxelSize, PointInCube.Y, PointInCube.Z), PointInCube);
	}
	//and when there is block, add face to it
	else if (i > -ChunkHalfWidth && CubeTypesArray[i + ChunkHalfWidth - 1][j + ChunkHalfWidth][k] != EGroundType::Air)
	{
		FVector WorldPos = VoxelSize * FVector(i - 1, j, k);
		WorldPos = FVector(WorldPos.X + InstanceOffsetFromCubeMiddle, WorldPos.Y, WorldPos.Z);
		EGroundType ActiveGround = CubeTypesArray[i + ChunkHalfWidth - 1][j + ChunkHalfWidth][k];
		AddInstanceAndMaterial(FRotator(0, 270, 90), WorldPos, ActiveGround);
	}

	//third side
	//if there is air block on the side of removed cube, remove the face
	if ((j < (ChunkHalfWidth) && CubeTypesArray[i + ChunkHalfWidth][j + ChunkHalfWidth + 1][k] == EGroundType::Air) || j == ChunkHalfWidth)
	{
		//raytrace +y
		TraceAndRemoveSide(FVector(PointInCube.X, PointInCube.Y + VoxelSize, PointInCube.Z), PointInCube);
	}
	//and when there is block, add face to it
	else if (j < (ChunkHalfWidth) && CubeTypesArray[i + ChunkHalfWidth][j + ChunkHalfWidth + 1][k] != EGroundType::Air)
	{
		FVector WorldPos = VoxelSize * FVector(i, j + 1, k);
		WorldPos = FVector(WorldPos.X, WorldPos.Y - InstanceOffsetFromCubeMiddle, WorldPos.Z);
		EGroundType ActiveGround = CubeTypesArray[i + ChunkHalfWidth][j + ChunkHalfWidth + 1][k];
		AddInstanceAndMaterial(FRotator(90, 90, 0), WorldPos, ActiveGround);
	}

	//fourth side
	//if there is air block on the side of removed cube, remove the face
	if ((j > -ChunkHalfWidth && CubeTypesArray[i + ChunkHalfWidth][j + ChunkHalfWidth - 1][k] == EGroundType::Air) || j == -ChunkHalfWidth)
	{
		//raytrace -y
		TraceAndRemoveSide(FVector(PointInCube.X, PointInCube.Y - VoxelSize, PointInCube.Z), PointInCube);
	}
	//and when there is block, add face to it
	else if (j > -ChunkHalfWidth && CubeTypesArray[i + ChunkHalfWidth][j + ChunkHalfWidth - 1][k] != EGroundType::Air)
	{
		FVector WorldPos = VoxelSize * FVector(i, j - 1, k);
		WorldPos = FVector(WorldPos.X, WorldPos.Y + InstanceOffsetFromCubeMiddle, WorldPos.Z);
		EGroundType ActiveGround = CubeTypesArray[i + ChunkHalfWidth][j + ChunkHalfWidth - 1][k];
		AddInstanceAndMaterial(FRotator(90, 90, 180), WorldPos, ActiveGround);
	}
}

void AChunk::RemoveCube(const FVector& PointInCube)
{
	//from point inside cube in world coordinates calculate its indices (coordinates) in chunk coordinates
	FIntVector Indices = GetPositionIndices(PointInCube);
	int i = Indices.X;
	int j = Indices.Y;
	int k = Indices.Z;
	
	//ensuring last layer of chunk is not destructible
	if (k == 0) {
		return;
	}

	//change ground type for position of removed cube to air
	CubeTypesArray[i + ChunkHalfWidth][j + ChunkHalfWidth][k] = EGroundType::Air;

	ManageUpperSideAfterRemoving(i, j, k, PointInCube);
	ManageLowerSideAfterRemoving(i,j ,k, PointInCube);
	ManageSidesAfterRemoving(i, j, k, PointInCube);
	
	//tell this change to ChunkMaanager to remember it
	if (ChunkManager)
	{
		ChunkManager->SetChanges(MyCoords, FVector(i, j, k), EGroundType::Air);
	}
}

FIntVector AChunk::GetPositionIndices(const FVector& Position)
{
	const auto loc = GetActorLocation();
	int X = FGenericPlatformMath::RoundToInt((Position.X - loc.X)/VoxelSize );
	//boundary cubes offset caused by origin in the middle of cube
	if (X == ChunkHalfWidth + 1)
	{
		X = ChunkHalfWidth;
	}
	if (X == -(ChunkHalfWidth + 1))
	{
		X = -ChunkHalfWidth;
	}
	int Y = FGenericPlatformMath::RoundToInt((Position.Y - loc.Y)/VoxelSize );
	if (Y == ChunkHalfWidth + 1)
	{
		Y = ChunkHalfWidth;
	}
	if (Y == -(ChunkHalfWidth + 1))
	{
		Y = -ChunkHalfWidth;
	}
	int Z = FGenericPlatformMath::RoundToInt((Position.Z - loc.Z)/VoxelSize );
	return FIntVector(X, Y, Z);
}

void AChunk::InteractWithNeighbourChunk(const FVector& VectorToFind, const FVector& OffsetDirection, const FVector& PointInside)
{
	int index = ChunkManager->SpawnedChunksCoordinates.Find(VectorToFind);
	if (ChunkManager->SpawnedChunksRefs.IsValidIndex(index))
	{
		AChunk* NeighbourChunkRef = ChunkManager->SpawnedChunksRefs[index];
		FVector PointInsideNeighbour = PointInside + OffsetDirection * InstanceOffsetFromCubeMiddle;
		NeighbourChunkRef->AddCube(PointInsideNeighbour);
	}
}

void AChunk::TellNeighbouringChunkToAddCube(int i, int j, const FVector& PointInside)
{
	//ask Chunk spawner which chunk is it that particular direction beyond this chunk, and tell him to add cube 
	if (i == ChunkHalfWidth)
	{
		InteractWithNeighbourChunk(FVector(MyCoords.X + 1, MyCoords.Y, MyCoords.Z), FVector(1, 0, 0), PointInside);
		return;
	}
	if (i == -ChunkHalfWidth)
	{
		InteractWithNeighbourChunk(FVector(MyCoords.X - 1, MyCoords.Y, MyCoords.Z), FVector(-1, 0, 0), PointInside);
		return;
	}
	if (j == ChunkHalfWidth)
	{
		InteractWithNeighbourChunk(FVector(MyCoords.X, MyCoords.Y + 1, MyCoords.Z), FVector(0, 1, 0), PointInside);
		return;
	}
	if (j == -ChunkHalfWidth)
	{
		InteractWithNeighbourChunk(FVector(MyCoords.X, MyCoords.Y - 1, MyCoords.Z), FVector(0, -1, 0), PointInside);
		return;
	}
}


void AChunk::ManageUpperSideAfterAdding(int i, int j, int k, const FVector& PointInCube, EGroundType ActiveGround)
{
	FVector WorldPos;

	//if there is air above, add face
	if ((k < (ChunkTotalHeight - 1) && CubeTypesArray[i + ChunkHalfWidth][j + ChunkHalfWidth][k + 1] == EGroundType::Air) || k == (ChunkTotalHeight - 1))
	{
		WorldPos = FVector(i * VoxelSize, j * VoxelSize, k * VoxelSize + InstanceOffsetFromCubeMiddle);
		AddInstanceAndMaterial(FRotator(0, 0, 0), WorldPos, ActiveGround);
	}
	//and when there is block, remove that side
	else if (k < (ChunkTotalHeight - 1) && CubeTypesArray[i + ChunkHalfWidth][j + ChunkHalfWidth][k + 1] != EGroundType::Air)
	{
		TraceAndRemoveSide(FVector(PointInCube.X, PointInCube.Y, PointInCube.Z + VoxelSize), PointInCube);
	}
}

void AChunk::ManageLowerSideAfterAdding(int i, int j, int k, const FVector& PointInCube, EGroundType ActiveGround)
{
	FVector WorldPos;

	//if there is an air under, add face
	if ((k > 0 && CubeTypesArray[i + ChunkHalfWidth][j + ChunkHalfWidth][k - 1] == EGroundType::Air) || k == 0)
	{
		WorldPos = FVector(i * VoxelSize, j * VoxelSize, k * VoxelSize - InstanceOffsetFromCubeMiddle);
		AddInstanceAndMaterial(FRotator(180, 0, 0), WorldPos, ActiveGround);
	}
	//if there is block, remove the side
	else if (k > 0 && CubeTypesArray[i + ChunkHalfWidth][j + ChunkHalfWidth][k - 1] != EGroundType::Air)
	{
		TraceAndRemoveSide(FVector(PointInCube.X, PointInCube.Y, PointInCube.Z - VoxelSize), PointInCube);
	}
}

void AChunk::ManageSidesAfterAdding(int i, int j, int k, const FVector& PointInCube, EGroundType ActiveGround)
{
	FVector WorldPos;

	//look at all neighbouring cubes in XY plane and if there is air, add face in that direction, if there is ground, remove its face (it is no longer visible so it is not needed)
	if ((i < (ChunkHalfWidth) && CubeTypesArray[i + 1 + ChunkHalfWidth][j + ChunkHalfWidth][k] == EGroundType::Air) || i == ChunkHalfWidth)
	{
		WorldPos = FVector(i * VoxelSize + InstanceOffsetFromCubeMiddle, j * VoxelSize, k * VoxelSize);
		AddInstanceAndMaterial(FRotator(0, 270, 90), WorldPos, ActiveGround);
	}
	else if (i < (ChunkHalfWidth) && CubeTypesArray[i + 1 + ChunkHalfWidth][j + ChunkHalfWidth][k] != EGroundType::Air)
	{
		TraceAndRemoveSide(FVector(PointInCube.X + VoxelSize, PointInCube.Y, PointInCube.Z), PointInCube);
	}

	if ((i > -ChunkHalfWidth && CubeTypesArray[i - 1 + ChunkHalfWidth][j + ChunkHalfWidth][k] == EGroundType::Air) || i == -ChunkHalfWidth)
	{
		WorldPos = FVector(i * VoxelSize - InstanceOffsetFromCubeMiddle, j * VoxelSize, k * VoxelSize);
		AddInstanceAndMaterial(FRotator(270, 180, 0), WorldPos, ActiveGround);
	}
	else if (i > -ChunkHalfWidth && CubeTypesArray[i - 1 + ChunkHalfWidth][j + ChunkHalfWidth][k] != EGroundType::Air)
	{
		TraceAndRemoveSide(FVector(PointInCube.X - VoxelSize, PointInCube.Y, PointInCube.Z), PointInCube);
	}

	if ((j < (ChunkHalfWidth) && CubeTypesArray[i + ChunkHalfWidth][j + 1 + ChunkHalfWidth][k] == EGroundType::Air) || j == ChunkHalfWidth)
	{
		WorldPos = FVector(i * VoxelSize, j * VoxelSize + InstanceOffsetFromCubeMiddle, k * VoxelSize);
		AddInstanceAndMaterial(FRotator(90, 270, 0), WorldPos, ActiveGround);
	}
	else if (j < (ChunkHalfWidth) && CubeTypesArray[i + ChunkHalfWidth][j + 1 + ChunkHalfWidth][k] != EGroundType::Air)
	{
		TraceAndRemoveSide(FVector(PointInCube.X, PointInCube.Y + VoxelSize, PointInCube.Z), PointInCube);
	}

	if ((j > -ChunkHalfWidth && CubeTypesArray[i + ChunkHalfWidth][j - 1 + ChunkHalfWidth][k] == EGroundType::Air) || j == -ChunkHalfWidth)
	{
		WorldPos = FVector(i * VoxelSize, j * VoxelSize - InstanceOffsetFromCubeMiddle, k * VoxelSize);
		AddInstanceAndMaterial(FRotator(270, -90, 0), WorldPos, ActiveGround);
	}
	else if (j > -ChunkHalfWidth && CubeTypesArray[i + ChunkHalfWidth][j - 1 + ChunkHalfWidth][k] != EGroundType::Air)
	{
		TraceAndRemoveSide(FVector(PointInCube.X, PointInCube.Y - VoxelSize, PointInCube.Z), PointInCube);
	}

}

void AChunk::AddCube(const FVector& PointInside)
{
	//ask game mode which type of ground player wants to add
	EGroundType ActiveGround = GetWorld()->GetAuthGameMode<ATestingMCGameModeBase>()->GetActiveGroundType();
	//calculate position on which cube will be added in arbitrary coordinates
	FIntVector PosIndices = GetPositionIndices(PointInside);
	int i = PosIndices.X;
	int j = PosIndices.Y;
	int k = PosIndices.Z;

	//if player wants to add cube beyond this chunk (the last cube of this on X/Y axis is already placed) we need to communicate it to the next chunk
	if ((i == ChunkHalfWidth || i == -ChunkHalfWidth || j == ChunkHalfWidth || j == -ChunkHalfWidth) && CubeTypesArray[i + ChunkHalfWidth][j + ChunkHalfWidth][k] != EGroundType::Air)
	{
		TellNeighbouringChunkToAddCube(i, j, PointInside);
	}

	//set ground type added
	CubeTypesArray[i + ChunkHalfWidth][j + ChunkHalfWidth][k] = ActiveGround;

	ManageUpperSideAfterAdding(i, j, k, PointInside, ActiveGround);
	ManageLowerSideAfterAdding(i, j, k, PointInside, ActiveGround);
	ManageSidesAfterAdding(i, j, k, PointInside, ActiveGround);
	
	//tell chunk manager about the change to remember it
	if (ChunkManager)
	{
		ChunkManager->SetChanges(MyCoords, FVector(i, j, k), ActiveGround);
	}
}
