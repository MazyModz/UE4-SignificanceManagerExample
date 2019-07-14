// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "DemoCharacter.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"

//////////////////////////////////////////////////////////////////////////
// ADemoCharacter

ADemoCharacter::ADemoCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f); // ...at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 300.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named MyCharacter (to avoid direct content references in C++)
	
	SignificanceSettings.SignificanceThresholds.Emplace(2.f, 1000.f);
	SignificanceSettings.SignificanceThresholds.Emplace(1.f, 2500.f);
	SignificanceSettings.SignificanceThresholds.Emplace(0.f, 5000.f);
}

//////////////////////////////////////////////////////////////////////////
// Input

void ADemoCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAxis("MoveForward", this, &ADemoCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ADemoCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &ADemoCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &ADemoCharacter::LookUpAtRate);

	// handle touch devices
	PlayerInputComponent->BindTouch(IE_Pressed, this, &ADemoCharacter::TouchStarted);
	PlayerInputComponent->BindTouch(IE_Released, this, &ADemoCharacter::TouchStopped);

	// VR headset functionality
	PlayerInputComponent->BindAction("ResetVR", IE_Pressed, this, &ADemoCharacter::OnResetVR);
}

void ADemoCharacter::OnResetVR()
{
	UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition();
}

void ADemoCharacter::TouchStarted(ETouchIndex::Type FingerIndex, FVector Location)
{
		Jump();
}

void ADemoCharacter::TouchStopped(ETouchIndex::Type FingerIndex, FVector Location)
{
		StopJumping();
}

void ADemoCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void ADemoCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void ADemoCharacter::MoveForward(float Value)
{
	if ((Controller != NULL) && (Value != 0.0f))
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void ADemoCharacter::MoveRight(float Value)
{
	if ( (Controller != NULL) && (Value != 0.0f) )
	{
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
	
		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		AddMovementInput(Direction, Value);
	}
}

void ADemoCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (!IsNetMode(NM_DedicatedServer))
	{
		USignificanceManager* SignificanceManager = FSignificanceManagerModule::Get(GetWorld());
		if (SignificanceManager)
		{
			auto Significance = [&](USignificanceManager::FManagedObjectInfo* ObjectInfo, const FTransform& Viewpoint) -> float
			{
				return SignficanceFunction(ObjectInfo, Viewpoint);
			};

			auto PostSignificance = [&](USignificanceManager::FManagedObjectInfo* ObjectInfo, float OldSignificance, float Significance, bool bFinal)
			{
				PostSignficanceFunction(ObjectInfo, OldSignificance, Significance, bFinal);
			};

			SignificanceManager->RegisterObject(this, TEXT("Character"), Significance, USignificanceManager::EPostSignificanceType::Sequential, PostSignificance);
		}
	}
}

// ------------------------------------------------------------------------------
// Significance Manager implementation

float ADemoCharacter::SignficanceFunction(USignificanceManager::FManagedObjectInfo* ObjectInfo, const FTransform& Viewpoint)
{
	if(ObjectInfo->GetTag() == TEXT("Character"))
	{
		ADemoCharacter* Character = CastChecked<ADemoCharacter>(ObjectInfo->GetObject());
		const float Distance = (Character->GetActorLocation() - Viewpoint.GetLocation()).Size();

		return GetSignificanceByDistance(Distance);
	}

	return 0.f;
}

void ADemoCharacter::PostSignficanceFunction(USignificanceManager::FManagedObjectInfo* ObjectInfo, float OldSignificance, float Significance, bool bFinal)
{
	if (ObjectInfo->GetTag() == TEXT("Character"))
	{
		ADemoCharacter* Character = CastChecked<ADemoCharacter>(ObjectInfo->GetObject());
		if (Significance == 0.f)
		{
			Character->GetCharacterMovement()->SetComponentTickInterval(0.25f);
		}
		else if (Significance == 1.f)
		{
			Character->GetCharacterMovement()->SetComponentTickInterval(0.1f);
		}
		else if (Significance == 2.f)
		{
			Character->GetCharacterMovement()->SetComponentTickInterval(0.f);
		}
	}
}

float ADemoCharacter::GetSignificanceByDistance(float Distance)
{
	const int32 NumThresholds = SignificanceSettings.SignificanceThresholds.Num();
	if (Distance >= SignificanceSettings.SignificanceThresholds[NumThresholds - 1].MaxDistance)
	{
		return SignificanceSettings.SignificanceThresholds[NumThresholds - 1].Significance;
	}
	else
	{
		for (int32 Idx = 0; Idx < NumThresholds; Idx++)
		{
			FSignificanceSettings::FSignificanceThresholds& Thresholds = SignificanceSettings.SignificanceThresholds[Idx];
			if (Distance <= Thresholds.MaxDistance)
			{
				return Thresholds.Significance;
			}
		}
	}

	return 0.f;
}
