// Fill out your copyright notice in the Description page of Project Settings.

#include "PestiCar.h"

#include "CarBody.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "PoliceCar.h"
#include "TrafficCar.h"
#include "Tram.h"

namespace PestiCar
{
	// Box half size in cm: a 4.2 m long, 1.7 m wide period saloon.
	const FVector BoxExtent(210.f, 85.f, 70.f);

	constexpr float ChaseArmLength = 700.f;
	constexpr float ChasePitch = -12.f;
	constexpr float OverheadArmLength = 2600.f;
	constexpr float OverheadPitch = -70.f;

	const FLinearColor BodyColor(0.03f, 0.07f, 0.05f);

	// Ramming something faster than this is a crime (cm/s; 500 is 18 km/h).
	constexpr float CrimeImpactSpeed = 500.f;
	// Below this speed next to a police car counts as being caught (cm/s).
	constexpr float CaughtSpeed = 300.f;
	constexpr int32 MaxWantedLevel = 3;
}

APestiCar::APestiCar()
{
	PrimaryActorTick.bCanEverTick = true;

	BaseMaterial = CarBody::LoadPaintMaterial();

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	CollisionBox->SetBoxExtent(PestiCar::BoxExtent);
	CollisionBox->SetCollisionProfileName(TEXT("Pawn"));
	RootComponent = CollisionBox;

	// The box bottom floats HoverHeight above the road; the body reaches a
	// little lower so the car looks grounded.
	CarBody::Build(this, CollisionBox, BodyParts, BodyPaints);

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

	CarBody::Paint(this, BaseMaterial, BodyParts, BodyPaints, PestiCar::BodyColor);

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
	if (PC->WasInputKeyJustPressed(EKeys::P))
	{
		if (WantedLevel > 0)
		{
			ClearWanted(TEXT("Police called off (P)"));
		}
		else
		{
			ReportCrime(1);
		}
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

	UpdateWanted(DeltaSeconds);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(1, 0.f, FColor::White,
			FString::Printf(TEXT("%d km/h   (C: camera, N: traffic view, P: police test)"), FMath::RoundToInt(AbsSpeed * 0.036f)));
		if (WantedLevel > 0)
		{
			GEngine->AddOnScreenDebugMessage(2, 0.f, FColor::Red,
				FString::Printf(TEXT("WANTED %s   lose them: %d s%s"), *FString::ChrN(WantedLevel, TEXT('*')),
					FMath::CeilToInt(FMath::Max(0.f, EscapeTime - TimeUnseen)),
					ArrestProgress > 0.f ? *FString::Printf(TEXT("   ARREST %d%%"), FMath::RoundToInt(100.f * ArrestProgress / ArrestTime)) : TEXT("")));
		}
	}
}

void APestiCar::ReportCrime(int32 CrimeLevel)
{
	if (WantedLevel == 0 && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(3, 4.f, FColor::Red, TEXT("The police are after you!"));
	}
	WantedLevel = FMath::Clamp(FMath::Max(WantedLevel, CrimeLevel), 0, PestiCar::MaxWantedLevel);
	TimeUnseen = 0.f;
}

void APestiCar::NotifySeenByPolice()
{
	TimeUnseen = 0.f;
}

void APestiCar::NotifyPoliceNear()
{
	LastPoliceNearTime = GetWorld()->GetTimeSeconds();
}

void APestiCar::UpdateWanted(float DeltaSeconds)
{
	if (WantedLevel == 0)
	{
		return;
	}

	TimeUnseen += DeltaSeconds;
	if (TimeUnseen > EscapeTime)
	{
		ClearWanted(TEXT("You lost the police!"));
		return;
	}

	const bool bPoliceNear = GetWorld()->GetTimeSeconds() - LastPoliceNearTime < 0.2;
	if (bPoliceNear && FMath::Abs(ForwardSpeed) < PestiCar::CaughtSpeed)
	{
		ArrestProgress += DeltaSeconds;
		if (ArrestProgress > ArrestTime)
		{
			ForwardSpeed = 0.f;
			ClearWanted(TEXT("BUSTED! The police caught you."));
		}
	}
	else
	{
		ArrestProgress = FMath::Max(0.f, ArrestProgress - DeltaSeconds);
	}
}

void APestiCar::ClearWanted(const TCHAR* Message)
{
	WantedLevel = 0;
	TimeUnseen = 0.f;
	ArrestProgress = 0.f;
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(2, 0.f, FColor::Red, TEXT(""));
		GEngine->AddOnScreenDebugMessage(3, 4.f, FColor::Yellow, Message);
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

	// Ramming traffic brings the police; ramming the police more so.
	const AActor* Rammed = Hit.GetActor();
	if (Rammed && FMath::Abs(ForwardSpeed) > PestiCar::CrimeImpactSpeed && (Rammed->IsA<ATrafficCar>() || Rammed->IsA<ATram>()))
	{
		ReportCrime(Rammed->IsA<APoliceCar>() ? 2 : 1);
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
