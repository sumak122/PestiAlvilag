// Fill out your copyright notice in the Description page of Project Settings.

#include "PestiPerson.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "PestiCar.h"
#include "PoliceCar.h"
#include "TrafficCar.h"
#include "TrafficManager.h"

namespace PestiPerson
{
	constexpr float CapsuleRadius = 35.f;
	constexpr float CapsuleHalfHeight = 92.f;

	constexpr float ArmLength = 380.f;
	constexpr float MinPitch = -70.f;
	constexpr float MaxPitch = 40.f;

	// Limbs swing this far at a run (degrees), and one stride is this many cm.
	constexpr float MaxSwing = 35.f;
	constexpr float StrideLength = 120.f;

	constexpr double MinTimeOnFoot = 0.3;

	// A dark winter coat.
	const FLinearColor CoatColor(0.08f, 0.06f, 0.05f);
}

APestiPerson::APestiPerson()
{
	PrimaryActorTick.bCanEverTick = true;

	BaseMaterial = CarBody::LoadPaintMaterial();

	GetCapsuleComponent()->InitCapsuleSize(PestiPerson::CapsuleRadius, PestiPerson::CapsuleHalfHeight);

	// The body turns to face where he walks; the camera follows the mouse.
	bUseControllerRotationYaw = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->bOrientRotationToMovement = true;
	Movement->RotationRate = FRotator(0.f, 600.f, 0.f);
	Movement->MaxWalkSpeed = JogSpeed;
	Movement->JumpZVelocity = 380.f;
	Movement->AirControl = 0.2f;

	CarBody::BuildPerson(this, GetCapsuleComponent(), BodyParts, BodyPaints);

	CameraArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraArm"));
	CameraArm->SetupAttachment(GetCapsuleComponent());
	CameraArm->TargetArmLength = PestiPerson::ArmLength;
	CameraArm->SocketOffset = FVector(0.f, 0.f, 60.f);
	CameraArm->bUsePawnControlRotation = true;
	CameraArm->bEnableCameraLag = true;
	CameraArm->CameraLagSpeed = 12.f;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraArm, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
}

void APestiPerson::BeginPlay()
{
	Super::BeginPlay();

	CarBody::Paint(this, BaseMaterial, BodyParts, BodyPaints, PestiPerson::CoatColor);
}

void APestiPerson::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	SwingLimbs(DeltaSeconds);

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		return;
	}

	if (!bCameraAimed)
	{
		// Start looking the way he faces, a little downwards (the game mode
		// aims the view at the level's PlayerStart, which may be anywhere).
		PC->SetControlRotation(FRotator(-10.f, GetActorRotation().Yaw, 0.f));
		bCameraAimed = true;
	}
	Look(PC, DeltaSeconds);
	Walk(PC);

	if (bCarrying)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(1, 0.f, FColor::White, TEXT("On foot, carrying something heavy"));
		}
		return;
	}

	if (PC->WasInputKeyJustPressed(EKeys::SpaceBar) || PC->WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Bottom))
	{
		Jump();
	}
	if (PC->WasInputKeyJustPressed(EKeys::F) || PC->WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Right))
	{
		TryEnterCar(PC);
		return;
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(1, 0.f, FColor::White,
			TEXT("On foot   (F: get in or steal a car, Shift: sprint, Space: jump, N: traffic view, P: police test, F9: new game)"));
	}
}

void APestiPerson::Look(APlayerController* PC, float DeltaSeconds)
{
	float MouseX = 0.f;
	float MouseY = 0.f;
	PC->GetInputMouseDelta(MouseX, MouseY);
	const float TurnX = MouseX * MouseLookScale + PC->GetInputAnalogKeyState(EKeys::Gamepad_RightX) * StickLookRate * DeltaSeconds;
	const float TurnY = MouseY * MouseLookScale + PC->GetInputAnalogKeyState(EKeys::Gamepad_RightY) * StickLookRate * DeltaSeconds;
	if (FMath::IsNearlyZero(TurnX) && FMath::IsNearlyZero(TurnY))
	{
		return;
	}
	const FRotator View = PC->GetControlRotation();
	const float Pitch = FMath::Clamp(FRotator::NormalizeAxis(View.Pitch + TurnY), PestiPerson::MinPitch, PestiPerson::MaxPitch);
	PC->SetControlRotation(FRotator(Pitch, View.Yaw + TurnX, 0.f));
}

void APestiPerson::Walk(const APlayerController* PC)
{
	auto KeyAxis = [PC](const FKey& Positive, const FKey& Negative)
	{
		return (PC->IsInputKeyDown(Positive) ? 1.f : 0.f) - (PC->IsInputKeyDown(Negative) ? 1.f : 0.f);
	};
	const float Forward = FMath::Clamp(KeyAxis(EKeys::W, EKeys::S) + KeyAxis(EKeys::Up, EKeys::Down)
		+ PC->GetInputAnalogKeyState(EKeys::Gamepad_LeftY), -1.f, 1.f);
	const float Right = FMath::Clamp(KeyAxis(EKeys::D, EKeys::A) + KeyAxis(EKeys::Right, EKeys::Left)
		+ PC->GetInputAnalogKeyState(EKeys::Gamepad_LeftX), -1.f, 1.f);

	// Walk relative to where the camera looks.
	const FRotator Heading(0.f, PC->GetControlRotation().Yaw, 0.f);
	AddMovementInput(FRotationMatrix(Heading).GetUnitAxis(EAxis::X), Forward);
	AddMovementInput(FRotationMatrix(Heading).GetUnitAxis(EAxis::Y), Right);

	const bool bRun = PC->IsInputKeyDown(EKeys::LeftShift) || PC->IsInputKeyDown(EKeys::Gamepad_LeftThumbstick);
	GetCharacterMovement()->MaxWalkSpeed = bCarrying ? CarrySpeed : bRun ? RunSpeed : JogSpeed;
}

void APestiPerson::SwingLimbs(float DeltaSeconds)
{
	// No animation rig yet: swing the arms and legs from the shapes' middles,
	// legs against arms, faster and wider the faster he goes.
	const float Speed = GetCharacterMovement()->IsMovingOnGround() ? static_cast<float>(GetVelocity().Size2D()) : 0.f;
	StridePhase += DeltaSeconds * Speed / PestiPerson::StrideLength * PI;
	const float Swing = FMath::Sin(StridePhase) * PestiPerson::MaxSwing * FMath::Clamp(Speed / RunSpeed, 0.f, 1.f);
	constexpr int32 NumLimbs = 4;
	// The arm with a load in it hangs still.
	const float Swings[NumLimbs] = { Swing, -Swing, -Swing, bCarrying ? 0.f : Swing };  // left leg, right leg, left arm, right arm
	for (int32 i = 0; i < NumLimbs && i < BodyParts.Num(); ++i)
	{
		if (BodyParts[i])
		{
			BodyParts[i]->SetRelativeRotation(FRotator(Swings[i], 0.f, 0.f));
		}
	}
}

void APestiPerson::TryEnterCar(APlayerController* PC)
{
	// The F that got him out of a car may still read as pressed this frame.
	if (GetWorld()->GetTimeSeconds() - OutOfCarTime < PestiPerson::MinTimeOnFoot)
	{
		return;
	}
	APestiCar* Nearest = nullptr;
	double NearestDist = EnterCarRange;
	for (TActorIterator<APestiCar> It(GetWorld()); It; ++It)
	{
		const double Dist = FVector::Dist2D(It->GetActorLocation(), GetActorLocation());
		if (!It->GetController() && Dist < NearestDist)
		{
			Nearest = *It;
			NearestDist = Dist;
		}
	}
	// A car from the traffic that is nearer still: pull the driver out and take it.
	ATrafficCar* Victim = nullptr;
	for (TActorIterator<ATrafficCar> It(GetWorld()); It; ++It)
	{
		const double Dist = FVector::Dist2D(It->GetActorLocation(), GetActorLocation());
		if (!Cast<APoliceCar>(*It) && Dist < NearestDist)
		{
			Victim = *It;
			NearestDist = Dist;
		}
	}
	if (Victim)
	{
		for (TActorIterator<ATrafficManager> It(GetWorld()); It; ++It)
		{
			Nearest = It->StealCar(Victim);
			break;
		}
	}
	if (!Nearest)
	{
		return;
	}

	SetInWorld(false);
	Nearest->TakeDriver(this);
	PC->Possess(Nearest);
}

bool APestiPerson::TryStepOutAt(const FVector& At, double Yaw)
{
	FCollisionQueryParams Params(SCENE_QUERY_STAT(PestiPersonStepOut), false, this);
	FHitResult Hit;
	if (!GetWorld()->LineTraceSingleByChannel(Hit, At + FVector(0.f, 0.f, 200.f), At - FVector(0.f, 0.f, 1000.f), ECC_Visibility, Params))
	{
		return false;
	}
	const FVector Location = Hit.ImpactPoint + FVector(0.f, 0.f, PestiPerson::CapsuleHalfHeight + 5.f);
	const FCollisionShape Capsule = FCollisionShape::MakeCapsule(PestiPerson::CapsuleRadius, PestiPerson::CapsuleHalfHeight);
	if (GetWorld()->OverlapAnyTestByChannel(Location, FQuat::Identity, ECC_Pawn, Capsule, Params))
	{
		return false;
	}

	SetActorLocationAndRotation(Location, FRotator(0.f, Yaw, 0.f), false, nullptr, ETeleportType::TeleportPhysics);
	SetInWorld(true);
	return true;
}

void APestiPerson::SetInWorld(bool bInWorld)
{
	SetActorHiddenInGame(!bInWorld);
	SetActorEnableCollision(bInWorld);
	SetActorTickEnabled(bInWorld);
	if (bInWorld)
	{
		OutOfCarTime = GetWorld()->GetTimeSeconds();
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}
	else
	{
		GetCharacterMovement()->StopMovementImmediately();
		GetCharacterMovement()->DisableMovement();
	}
}

void APestiPerson::SetCarrying(bool bInCarrying)
{
	bCarrying = bInCarrying;
}
