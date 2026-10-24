// Fill out your copyright notice in the Description page of Project Settings.

#include "PestiCar.h"

#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace PestiCar
{
	// Box half size in cm: a 4.2 m long, 1.7 m wide period saloon.
	const FVector BoxExtent(210.f, 85.f, 70.f);

	constexpr float ChaseArmLength = 700.f;
	constexpr float ChasePitch = -12.f;
	constexpr float OverheadArmLength = 2600.f;
	constexpr float OverheadPitch = -70.f;

	const FLinearColor BodyColor(0.03f, 0.07f, 0.05f);
	const FLinearColor TyreColor(0.01f, 0.01f, 0.01f);
}

APestiCar::APestiCar()
{
	PrimaryActorTick.bCanEverTick = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	CollisionBox->SetBoxExtent(PestiCar::BoxExtent);
	CollisionBox->SetCollisionProfileName(TEXT("Pawn"));
	RootComponent = CollisionBox;

	// The box bottom floats HoverHeight above the road; the body sits a
	// little lower so the car looks grounded. Engine shapes are 100 cm.
	auto MakePart = [this](const TCHAR* PartName, UStaticMesh* Shape, const FVector& Location, const FRotator& Rotation, const FVector& Scale)
	{
		UStaticMeshComponent* Part = CreateDefaultSubobject<UStaticMeshComponent>(PartName);
		Part->SetupAttachment(CollisionBox);
		Part->SetStaticMesh(Shape);
		Part->SetRelativeLocationAndRotation(Location, Rotation);
		Part->SetRelativeScale3D(Scale);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		return Part;
	};

	Chassis = MakePart(TEXT("Chassis"), CubeMesh.Object, FVector(0.f, 0.f, -35.f), FRotator::ZeroRotator, FVector(4.2f, 1.7f, 0.6f));
	Cabin = MakePart(TEXT("Cabin"), CubeMesh.Object, FVector(-30.f, 0.f, 25.f), FRotator::ZeroRotator, FVector(2.0f, 1.5f, 0.6f));

	const FVector WheelPositions[] = {
		FVector(130.f, 78.f, -55.f),
		FVector(130.f, -78.f, -55.f),
		FVector(-130.f, 78.f, -55.f),
		FVector(-130.f, -78.f, -55.f),
	};
	int32 WheelIndex = 0;
	for (const FVector& WheelPosition : WheelPositions)
	{
		const FString PartName = FString::Printf(TEXT("Wheel%d"), WheelIndex++);
		Wheels.Add(MakePart(*PartName, CylinderMesh.Object, WheelPosition, FRotator(0.f, 0.f, 90.f), FVector(0.7f, 0.7f, 0.25f)));
	}

	CameraArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraArm"));
	CameraArm->SetupAttachment(CollisionBox);
	CameraArm->TargetArmLength = PestiCar::ChaseArmLength;
	CameraArm->SetRelativeRotation(FRotator(PestiCar::ChasePitch, 0.f, 0.f));
	CameraArm->TargetOffset = FVector(0.f, 0.f, 150.f);
	CameraArm->bUsePawnControlRotation = false;
	CameraArm->bInheritRoll = false;
	CameraArm->bEnableCameraLag = true;
	CameraArm->CameraLagSpeed = 8.f;
	CameraArm->bEnableCameraRotationLag = true;
	CameraArm->CameraRotationLagSpeed = 6.f;

	ChaseCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ChaseCamera"));
	ChaseCamera->SetupAttachment(CameraArm, USpringArmComponent::SocketName);
}

void APestiCar::BeginPlay()
{
	Super::BeginPlay();

	auto Paint = [this](UStaticMeshComponent* Part, const FLinearColor& Color)
	{
		if (UMaterialInterface* BaseMaterial = Part->GetMaterial(0))
		{
			UMaterialInstanceDynamic* Paintwork = UMaterialInstanceDynamic::Create(BaseMaterial, this);
			Paintwork->SetVectorParameterValue(TEXT("Color"), Color);
			Part->SetMaterial(0, Paintwork);
		}
	};
	Paint(Chassis, PestiCar::BodyColor);
	Paint(Cabin, PestiCar::BodyColor);
	for (UStaticMeshComponent* Wheel : Wheels)
	{
		Paint(Wheel, PestiCar::TyreColor);
	}

	SnapToGround();
}

void APestiCar::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		return;
	}

	auto KeyAxis = [PC](const FKey& Positive, const FKey& Negative)
	{
		return (PC->IsInputKeyDown(Positive) ? 1.f : 0.f) - (PC->IsInputKeyDown(Negative) ? 1.f : 0.f);
	};

	const float Throttle = FMath::Clamp(
		KeyAxis(EKeys::W, EKeys::S) + KeyAxis(EKeys::Up, EKeys::Down)
			+ PC->GetInputAnalogKeyState(EKeys::Gamepad_RightTriggerAxis)
			- PC->GetInputAnalogKeyState(EKeys::Gamepad_LeftTriggerAxis),
		-1.f, 1.f);
	const float Steer = FMath::Clamp(
		KeyAxis(EKeys::D, EKeys::A) + KeyAxis(EKeys::Right, EKeys::Left)
			+ PC->GetInputAnalogKeyState(EKeys::Gamepad_LeftX),
		-1.f, 1.f);
	const bool bHandbrake = PC->IsInputKeyDown(EKeys::SpaceBar) || PC->IsInputKeyDown(EKeys::Gamepad_FaceButton_Bottom);

	if (PC->WasInputKeyJustPressed(EKeys::C) || PC->WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Top))
	{
		ToggleCamera();
	}

	UpdateSpeed(Throttle, bHandbrake, DeltaSeconds);

	// Turning needs some speed, and gets gentler near top speed.
	const float AbsSpeed = FMath::Abs(ForwardSpeed);
	const float SpeedFactor = FMath::Clamp(AbsSpeed / FullTurnSpeed, 0.f, 1.f);
	const float HighSpeedDamping = 1.f - 0.45f * FMath::Clamp(AbsSpeed / MaxSpeed, 0.f, 1.f);
	const float HandbrakeBoost = bHandbrake ? 1.6f : 1.f;
	const float YawDelta = Steer * MaxTurnRate * SpeedFactor * HighSpeedDamping * HandbrakeBoost
		* FMath::Sign(ForwardSpeed) * DeltaSeconds;
	AddActorWorldRotation(FRotator(0.f, YawDelta, 0.f));

	MoveWithSlide(GetActorForwardVector() * ForwardSpeed * DeltaSeconds);
	SnapToGround();

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(1, 0.f, FColor::White,
			FString::Printf(TEXT("%d km/h   (C: camera)"), FMath::RoundToInt(AbsSpeed * 0.036f)));
	}
}

void APestiCar::UpdateSpeed(float Throttle, bool bHandbrake, float DeltaSeconds)
{
	if (bHandbrake)
	{
		ForwardSpeed = FMath::FInterpConstantTo(ForwardSpeed, 0.f, DeltaSeconds, HandbrakeDeceleration);
	}
	else if (Throttle > 0.05f)
	{
		const float Rate = ForwardSpeed < 0.f
			? BrakeDeceleration
			: Acceleration * (1.f - 0.6f * FMath::Clamp(ForwardSpeed / MaxSpeed, 0.f, 1.f));
		ForwardSpeed += Rate * Throttle * DeltaSeconds;
	}
	else if (Throttle < -0.05f)
	{
		const float Rate = ForwardSpeed > 0.f ? BrakeDeceleration : Acceleration * 0.6f;
		ForwardSpeed += Rate * Throttle * DeltaSeconds;
	}
	else
	{
		ForwardSpeed = FMath::FInterpConstantTo(ForwardSpeed, 0.f, DeltaSeconds, RollingDrag);
	}
	ForwardSpeed = FMath::Clamp(ForwardSpeed, -MaxReverseSpeed, MaxSpeed);
}

void APestiCar::MoveWithSlide(const FVector& Delta)
{
	FHitResult Hit;
	AddActorWorldOffset(Delta, true, &Hit);

	if (Hit.bStartPenetrating)
	{
		// Turning on the spot can push a corner into a wall; step back out.
		AddActorWorldOffset(Hit.Normal * (Hit.PenetrationDepth + 1.f), false);
		return;
	}
	if (!Hit.bBlockingHit)
	{
		return;
	}

	// A head-on hit nearly stops the car; a glancing one lets it scrape along.
	const FVector WallNormal = FVector(Hit.Normal.X, Hit.Normal.Y, 0.f).GetSafeNormal();
	const float Impact = static_cast<float>(FMath::Abs(FVector::DotProduct(GetActorForwardVector(), WallNormal)));
	ForwardSpeed *= 1.f - 0.8f * Impact;
	AddActorWorldOffset(FVector::VectorPlaneProject(Delta * (1.f - Hit.Time), WallNormal), true);
}

void APestiCar::SnapToGround()
{
	const FVector Start = GetActorLocation();
	const FVector End = Start - FVector(0.f, 0.f, 2000.f);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(PestiCarGround), false, this);
	FHitResult Hit;
	if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
	{
		FVector Location = Start;
		Location.Z = Hit.ImpactPoint.Z + HoverHeight + CollisionBox->GetScaledBoxExtent().Z;
		SetActorLocation(Location, false);
	}
}

void APestiCar::ToggleCamera()
{
	bOverheadCamera = !bOverheadCamera;
	CameraArm->TargetArmLength = bOverheadCamera ? PestiCar::OverheadArmLength : PestiCar::ChaseArmLength;
	CameraArm->SetRelativeRotation(FRotator(bOverheadCamera ? PestiCar::OverheadPitch : PestiCar::ChasePitch, 0.f, 0.f));
	CameraArm->bDoCollisionTest = !bOverheadCamera;
}
