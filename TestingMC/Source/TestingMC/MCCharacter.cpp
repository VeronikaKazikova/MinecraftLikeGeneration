#include "MCCharacter.h"
#include "EnhancedInputSubsystems.h"
#include "Components/CapsuleComponent.h"
#include "Chunk.h"
#include "EnhancedInputComponent.h"
#include "GenericPlatform/GenericPlatformMath.h"
#include "Kismet/GameplayStatics.h"
#include "Chunk.h"
#include "ChunkSpawner.h"
#include "TestingMCGameModeBase.h"
#include "Camera/CameraComponent.h"
#include "EGroundTypes.h"

AMCCharacter::AMCCharacter()
{

	PrimaryActorTick.bCanEverTick = false;
	//setup the capsule comp
	GetCapsuleComponent()->InitCapsuleSize(40.f, 50.0f);

	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->SetRelativeLocation(FVector(-10.f, 0.f, 60.f));
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

}

// Called when the game starts or when spawned
void AMCCharacter::BeginPlay()
{
	Super::BeginPlay();

	ChunkSpawnerRef = Cast<AChunkSpawner>(UGameplayStatics::GetActorOfClass(GetWorld(), AChunkSpawner::StaticClass()));

	//Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

//bind functionality to input
void AMCCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		//Moving binding
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMCCharacter::Move);

		//Looking binding
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMCCharacter::Look);

		//Jumping binding
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		//Removing blocks binding
		EnhancedInputComponent->BindAction(RemoveAction, ETriggerEvent::Started, this, &AMCCharacter::RemoveBlock);

		//Adding blocks 
		EnhancedInputComponent->BindAction(AddAction, ETriggerEvent::Started, this, &AMCCharacter::AddBlock);

		//Selecting texture
		EnhancedInputComponent->BindAction(SelectGrass, ETriggerEvent::Started, this, &AMCCharacter::GrassSelected);
		EnhancedInputComponent->BindAction(SelectDirt, ETriggerEvent::Started, this, &AMCCharacter::DirtSelected);
		EnhancedInputComponent->BindAction(SelectStone, ETriggerEvent::Started, this, &AMCCharacter::StoneSelected);

		//pause menu
		EnhancedInputComponent->BindAction(Pause, ETriggerEvent::Started, this, &AMCCharacter::GamePaused);
	}
}

void AMCCharacter::GrassSelected(const FInputActionValue& Value)
{
	if (Controller != nullptr)
	{
		GetWorld()->GetAuthGameMode<ATestingMCGameModeBase>()->SetActiveGroundType(EGroundType::Grass);
	}
}

void AMCCharacter::DirtSelected(const FInputActionValue& Value)
{
	if (Controller != nullptr)
	{
		GetWorld()->GetAuthGameMode<ATestingMCGameModeBase>()->SetActiveGroundType(EGroundType::Dirt);
	}
}

void AMCCharacter::StoneSelected(const FInputActionValue& Value)
{
	if (Controller != nullptr)
	{
		GetWorld()->GetAuthGameMode<ATestingMCGameModeBase>()->SetActiveGroundType(EGroundType::Stone);
	}
}

void AMCCharacter::Move(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		AddMovementInput(GetActorForwardVector(), MovementVector.Y);
		AddMovementInput(GetActorRightVector(), MovementVector.X);
	}
}

void AMCCharacter::Look(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

//adding cubes with LMB input
void AMCCharacter::AddBlock(const FInputActionValue& Value)
{
	if (Controller != nullptr)
	{
		if (LineTraceCubes())
		{
			//need to check if character is not in the place where the cube would be placed
			FCollisionQueryParams Params;
			FHitResult CheckResults;
			FVector EndCharacterCheck = GetActorLocation();
			if (GetWorld()->LineTraceSingleByChannel(CheckResults, HitResults.ImpactPoint, EndCharacterCheck, ECollisionChannel::ECC_WorldDynamic, Params, FCollisionResponseParams()))
			{
				int DistBannedForBuilding = 70;
				if (CheckResults.Distance < DistBannedForBuilding)
				{
					return;
				}
			}
			//if character is far enough, calculate point which will be inside the cube and would serve as starting point for linetracing and pass that info to 
			//particular chunk, which will be adding new cube
			FVector PointInside = HitResults.ImpactPoint + 5 * HitResults.ImpactNormal;
			Cast<AChunk>(HitResults.GetActor())->AddCube(PointInside);
		}
	}
}

//removing cubes with RMB input
void AMCCharacter::RemoveBlock(const FInputActionValue& Value)
{
	if (Controller != nullptr)
	{ 
		if (LineTraceCubes())
		{
			//calculated point inside the hit cube
			FVector PointInside = HitResults.ImpactPoint - 15 * HitResults.ImpactNormal;
			//tell particular chunk it has been hit, and where
			Cast<AChunk>(HitResults.GetActor())->RemoveCube(PointInside);
		}
	}
}

//line trace cubes in front of the character for further removing and adding
bool AMCCharacter::LineTraceCubes()
{
	constexpr float MaxTraceLength = 350.f;
	FVector TraceEnd = FirstPersonCameraComponent->GetComponentLocation() + FirstPersonCameraComponent->GetForwardVector() * MaxTraceLength;
	FCollisionQueryParams Params;
	//ignore character itself character
	Params.AddIgnoredActor(this);
	if (GetWorld()->LineTraceSingleByChannel(HitResults, FirstPersonCameraComponent->GetComponentLocation(), TraceEnd, ECollisionChannel::ECC_Visibility, Params, FCollisionResponseParams()))
	{
		return true;
	}
	return false;
}

