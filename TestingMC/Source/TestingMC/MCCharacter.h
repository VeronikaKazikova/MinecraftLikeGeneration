#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "MCCharacter.generated.h"

UCLASS()
class TESTINGMC_API AMCCharacter : public ACharacter
{
	GENERATED_BODY()

public:

	AMCCharacter();

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/** First person camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FirstPersonCameraComponent;
	
	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputMappingContext* DefaultMappingContext;

	//section for INPUTS
	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* LookAction;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* JumpAction;

	/** Removing blocks*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* RemoveAction;

	/** Adding Blocks*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* AddAction;

	/** Selecting Different Texture*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* SelectGrass;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* SelectDirt;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* SelectStone;

	/**Opening Pause Menu*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* Pause;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	//function for line tracing cubes in front of the character
	bool LineTraceCubes();

	//ref to chunk manager
	UPROPERTY(EditAnywhere)
		class AChunkSpawner* ChunkSpawnerRef;

	//used for line tracing in front of the character
	FHitResult HitResults;

	/** Section for all input functions */
	void Move(const FInputActionValue& Value);

	void Look(const FInputActionValue& Value);

	void RemoveBlock(const FInputActionValue& Value);

	void AddBlock(const FInputActionValue& Value);

	void GrassSelected(const FInputActionValue& Value);

	void DirtSelected(const FInputActionValue& Value);

	void StoneSelected(const FInputActionValue& Value);

	UFUNCTION(BlueprintImplementableEvent)
	void GamePaused(const FInputActionValue& Value);

};
