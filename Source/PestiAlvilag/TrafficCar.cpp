// Fill out your copyright notice in the Description page of Project Settings.

#include "TrafficCar.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TrafficManager.h"
#include "UObject/ConstructorHelpers.h"

namespace TrafficCar
{
	// How far ahead a car looks for others (cm).
	constexpr float LookAhead = 2500.f;
	// The car moves on to the next point once it is this close (cm).
	constexpr float ReachRadius = 300.f;
	// Slowest fraction of lane speed in a sharp corner.
	constexpr float CornerSpeedFraction = 0.35f;
}

ATrafficCar::ATrafficCar()
{
	PrimaryActorTick.bCanEverTick = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

	// Same size and layout as the player's car (see APestiCar).
	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	CollisionBox->SetBoxExtent(FVector(210.f, 85.f, 70.f));
	CollisionBox->SetCollisionProfileName(TEXT("Pawn"));
	RootComponent = CollisionBox;

	struct FPart
	{
		const TCHAR* Name;
		bool bWheel;
		FVector Location;
		FVector Scale;
	};
	const FPart Parts[] = {
		{ TEXT("Chassis"), false, FVector(0.f, 0.f, -35.f), FVector(4.2f, 1.7f, 0.6f) },
		{ TEXT("Cabin"), false, FVector(-30.f, 0.f, 25.f), FVector(2.0f, 1.5f, 0.6f) },
		{ TEXT("Wheel0"), true, FVector(130.f, 78.f, -55.f), FVector(0.7f, 0.7f, 0.25f) },
		{ TEXT("Wheel1"), true, FVector(130.f, -78.f, -55.f), FVector(0.7f, 0.7f, 0.25f) },
		{ TEXT("Wheel2"), true, FVector(-130.f, 78.f, -55.f), FVector(0.7f, 0.7f, 0.25f) },
		{ TEXT("Wheel3"), true, FVector(-130.f, -78.f, -55.f), FVector(0.7f, 0.7f, 0.25f) },
	};
	for (const FPart& Spec : Parts)
	{
		UStaticMeshComponent* Part = CreateDefaultSubobject<UStaticMeshComponent>(Spec.Name);
		Part->SetupAttachment(CollisionBox);
		Part->SetStaticMesh(Spec.bWheel ? CylinderMesh.Object : CubeMesh.Object);
		Part->SetRelativeLocationAndRotation(Spec.Location, Spec.bWheel ? FRotator(0.f, 0.f, 90.f) : FRotator::ZeroRotator);
		Part->SetRelativeScale3D(Spec.Scale);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		BodyParts.Add(Part);
	}
}

void ATrafficCar::StartOnLane(ATrafficManager* InManager, int32 InLaneIndex, int32 InPointIndex, const FLinearColor& Paint)
{
	Manager = InManager;
	LaneIndex = InLaneIndex;
	PointIndex = InPointIndex;
	CurrentSpeed = 0.f;

	for (int32 i = 0; i < BodyParts.Num(); ++i)
	{
		UStaticMeshComponent* Part = BodyParts[i];
		UMaterialInterface* BaseMaterial = Part ? Part->GetMaterial(0) : nullptr;
		if (!BaseMaterial)
		{
			continue;
		}
		// The first two parts are the body; the rest are tyres.
		UMaterialInstanceDynamic* Paintwork = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		Paintwork->SetVectorParameterValue(TEXT("Color"), i < 2 ? Paint : FLinearColor(0.01f, 0.01f, 0.01f));
		Part->SetMaterial(0, Paintwork);
	}
}

void ATrafficCar::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!Manager || LaneIndex == INDEX_NONE)
	{
		return;
	}

	FVector Location = GetActorLocation();

	// Advance along the lane, switching lanes at the end of each street piece.
	const FTrafficLane* Lane = &Manager->GetLane(LaneIndex);
	FVector Target = Lane->Points[PointIndex];
	for (int32 Guard = 0; Guard < 4 && FVector::Dist2D(Location, Target) < TrafficCar::ReachRadius; ++Guard)
	{
		if (++PointIndex >= Lane->Points.Num())
		{
			LaneIndex = Manager->PickNextLane(LaneIndex);
			Lane = &Manager->GetLane(LaneIndex);
			PointIndex = 0;
		}
		Target = Lane->Points[PointIndex];
	}

	// Steer towards the target point at a limited turn rate.
	const FVector ToTarget = (Target - Location).GetSafeNormal2D();
	const float CurrentYaw = static_cast<float>(GetActorRotation().Yaw);
	const float WantedYaw = static_cast<float>(ToTarget.Rotation().Yaw);
	const float TurnNeeded = FMath::Abs(FRotator::NormalizeAxis(WantedYaw - CurrentYaw));
	const float NewYaw = FMath::FixedTurn(CurrentYaw, WantedYaw, TurnRate * DeltaSeconds);
	SetActorRotation(FRotator(0.f, NewYaw, 0.f));

	// Slow for corners and for anything in front.
	float WantedSpeed = Lane->Speed * FMath::Lerp(1.f, TrafficCar::CornerSpeedFraction, FMath::Clamp(TurnNeeded / 90.f, 0.f, 1.f));
	const float Clear = Manager->ClearDistanceAhead(this, TrafficCar::LookAhead);
	WantedSpeed *= FMath::Clamp((Clear - StopGap) / (TrafficCar::LookAhead - StopGap), 0.f, 1.f);

	const float Rate = WantedSpeed < CurrentSpeed ? BrakeDeceleration : Acceleration;
	CurrentSpeed = FMath::FInterpConstantTo(CurrentSpeed, WantedSpeed, DeltaSeconds, Rate);

	Location += GetActorForwardVector().GetSafeNormal2D() * CurrentSpeed * DeltaSeconds;
	SetActorLocation(Location);
}
