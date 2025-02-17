// Fill out your copyright notice in the Description page of Project Settings.


#include "MCGameInstance.h"
#include "ChunkSpawner.h"
#include "Kismet/GameplayStatics.h"
#include "Save.h"


void UMCGameInstance::SaveGameToSlot(FString SlotName)
{
	//prepare save file
	AChunkSpawner* ChunkSpawnerRef = Cast<AChunkSpawner>(UGameplayStatics::GetActorOfClass(GetWorld(), AChunkSpawner::StaticClass()));
	USave* SaveGameRef = Cast<USave>(UGameplayStatics::CreateSaveGameObject(USave::StaticClass()));
	//take information we want to save
	SaveGameRef->PlayerLocation = UGameplayStatics::GetPlayerPawn(this, 0)->GetActorLocation();
	SaveGameRef->SavedChanges = ChunkSpawnerRef->GetChanges();

	//save
	UGameplayStatics::SaveGameToSlot(SaveGameRef, SlotName, 0);
	
}

void UMCGameInstance::LoadGame(FString SlotName)
{
	ShouldDataBeLoaded = true;
	USave* SaveGameRef = Cast<USave>(UGameplayStatics::CreateSaveGameObject(USave::StaticClass()));
	SaveGameRef = Cast<USave>(UGameplayStatics::LoadGameFromSlot(SlotName, 0));
	//take information we want to load
	PlayerStartPosition = SaveGameRef->PlayerLocation;
	LoadedChanges = SaveGameRef->SavedChanges;

	//load level and give it information
	UGameplayStatics::OpenLevel(GetWorld(), "MainMap");
	
	//get player pawn and set position to it
	UGameplayStatics::GetPlayerPawn(GetWorld(), 0)->SetActorLocation(PlayerStartPosition);
}

void UMCGameInstance::OnSetSeed(int NewSeed)
{
	SeedValue = NewSeed;
}

int UMCGameInstance::GetSeedValue()
{
	return SeedValue;
}