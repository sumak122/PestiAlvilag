// Fill out your copyright notice in the Description page of Project Settings.

#include "ParkKeeper.h"

#include "PersonModel.h"
#include "Components/SceneComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace ParkKeeper
{
	const FLinearColor WorkCoat(0.20f, 0.22f, 0.20f);
	const FLinearColor RakeWood(0.40f, 0.26f, 0.12f);
	const FLinearColor RakeIron(0.05f, 0.05f, 0.05f);

	constexpr float Walk = 110.f;
	constexpr double Arrived = 20.0;
	constexpr float TurnRate = 240.f;
	constexpr float StrideLength = 110.f;
	constexpr float LegSwing = 25.f;

	// The hands, in front of the coat at waist height; the rake is 1.6 m
	// long and reaches the ground about 1.3 m ahead of him when tilted down.
	const FVector GripAt(25.f, 18.f, 10.f);
	constexpr double RakeLength = 160.0;
	constexpr float RakeDown = -38.f;
	constexpr float CarryDown = -15.f;
	constexpr float StrokeReach = 22.f;
	constexpr float StrokeRate = 3.5f;
	constexpr float ArmsRaking = 45.f;
	constexpr float ArmsCarrying = 30.f;
	constexpr float ArmStroke = 12.f;

	// He stands this far from the pile he rakes, on a side that changes
	// from pile to pile.
	constexpr double StandOff = 140.0;
}

AParkKeeper::AParkKeeper()
{
	CoatColor = ParkKeeper::WorkCoat;
	// A park keeper's work clothes: cap, jacket, boots.
	Outfit.Hat = PersonKit::EHat::FlatCap;
	Outfit.HatColor = ParkKeeper::WorkCoat * 0.8f;
	Outfit.Top = PersonKit::ETop::Jacket;
	Outfit.bTie = false;
	Outfit.bBoots = true;
	Outfit.bMoustache = true;
	Outfit.Trousers = FLinearColor(0.12f, 0.11f, 0.09f);
	WalkSpeed = ParkKeeper::Walk;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

	RakeGrip = CreateDefaultSubobject<USceneComponent>(TEXT("RakeGrip"));
	RakeGrip->SetupAttachment(Capsule);
	RakeGrip->SetRelativeLocationAndRotation(ParkKeeper::GripAt, FRotator(ParkKeeper::CarryDown, 0.f, 0.f));

	// The handle runs along the grip's forward axis (the cylinder stands along Z).
	RakeHandle = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RakeHandle"));
	RakeHandle->SetupAttachment(RakeGrip);
	RakeHandle->SetStaticMesh(CylinderMesh.Object);
	RakeHandle->SetRelativeLocationAndRotation(FVector(ParkKeeper::RakeLength * 0.5, 0.0, 0.0), FRotator(-90.f, 0.f, 0.f));
	RakeHandle->SetRelativeScale3D(FVector(0.035, 0.035, ParkKeeper::RakeLength / 100.0));
	RakeHandle->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	RakeHead = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RakeHead"));
	RakeHead->SetupAttachment(RakeGrip);
	RakeHead->SetStaticMesh(CubeMesh.Object);
	RakeHead->SetRelativeLocation(FVector(ParkKeeper::RakeLength, 0.0, 0.0));
	RakeHead->SetRelativeScale3D(FVector(0.05, 0.55, 0.05));
	RakeHead->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	RakeTeeth = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RakeTeeth"));
	RakeTeeth->SetupAttachment(RakeGrip);
	RakeTeeth->SetStaticMesh(CubeMesh.Object);
	RakeTeeth->SetRelativeLocation(FVector(ParkKeeper::RakeLength + 2.0, 0.0, -7.0));
	RakeTeeth->SetRelativeScale3D(FVector(0.015, 0.52, 0.1));
	RakeTeeth->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AParkKeeper::BeginPlay()
{
	Super::BeginPlay();

	if (BaseMaterial)
	{
		UMaterialInstanceDynamic* Wood = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		Wood->SetVectorParameterValue(TEXT("Color"), ParkKeeper::RakeWood);
		RakeHandle->SetMaterial(0, Wood);
		RakeHead->SetMaterial(0, Wood);
		UMaterialInstanceDynamic* Iron = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		Iron->SetVectorParameterValue(TEXT("Color"), ParkKeeper::RakeIron);
		RakeTeeth->SetMaterial(0, Iron);
	}
}

void AParkKeeper::SetWorkSpots(const TArray<FVector>& InSpots)
{
	Spots = InSpots;
	Next = 0;
	Raking = 0.f;
}

void AParkKeeper::PoseLimbs(float Legs, float Arms)
{
	// The leg out in front bends a little at the knee.
	PersonModel::PoseLegs(Pieces, Legs, -Legs, -0.5f * FMath::Max(Legs, 0.f), -0.5f * FMath::Max(-Legs, 0.f));
	for (const int32 Arm : { PersonKit::ArmLeft, PersonKit::ArmRight })
	{
		if (USceneComponent* Part = Limb(Arm))
		{
			Part->SetRelativeRotation(FRotator(Arms, 0.f, 0.f));
		}
	}
}

void AParkKeeper::Tick(float DeltaSeconds)
{
	// His own walking and raking instead of following someone.
	AActor::Tick(DeltaSeconds);

	if (Spots.IsEmpty())
	{
		PoseLimbs(0.f, ParkKeeper::ArmsCarrying);
		return;
	}

	const FVector Pile = Spots[Next % Spots.Num()];
	const double Side = 360.0 * Next / FMath::Max(Spots.Num(), 1) + 60.0;
	const FVector2D Stand = FVector2D(Pile) + FVector2D(FMath::Cos(FMath::DegreesToRadians(Side)), FMath::Sin(FMath::DegreesToRadians(Side))) * ParkKeeper::StandOff;

	const FVector Here = GetActorLocation();
	const FVector2D ToStand = Stand - FVector2D(Here);
	const double Gap = ToStand.Size();

	if (Raking <= 0.f && Gap > ParkKeeper::Arrived)
	{
		// Walking to the next pile, rake held out in front.
		const float Step = WalkSpeed * DeltaSeconds;
		FVector NextAt = Here + FVector(ToStand.GetSafeNormal() * FMath::Min(static_cast<double>(Step), Gap), 0.0);
		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(KeeperGround), false, this);
		if (GetWorld()->LineTraceSingleByChannel(Hit, NextAt + FVector(0.0, 0.0, 150.0), NextAt - FVector(0.0, 0.0, 500.0), ECC_Visibility, Params))
		{
			NextAt.Z = Hit.ImpactPoint.Z + Capsule->GetUnscaledCapsuleHalfHeight();
		}
		SetActorLocation(NextAt);

		const float WantYaw = static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(ToStand.Y, ToStand.X)));
		SetActorRotation(FRotator(0.f, FMath::FixedTurn(static_cast<float>(GetActorRotation().Yaw), WantYaw, ParkKeeper::TurnRate * DeltaSeconds), 0.f));

		Phase += DeltaSeconds * WalkSpeed / ParkKeeper::StrideLength * PI;
		PoseLimbs(FMath::Sin(Phase) * ParkKeeper::LegSwing, ParkKeeper::ArmsCarrying);
		RakeGrip->SetRelativeLocationAndRotation(ParkKeeper::GripAt, FRotator(ParkKeeper::CarryDown, 0.f, 0.f));
		return;
	}

	if (Raking <= 0.f)
	{
		// Just got there.
		Raking = RakeTime;
		Phase = 0.f;
	}

	// Raking: face the pile and draw the rake towards it, stroke after stroke.
	const FVector2D ToPile = FVector2D(Pile) - FVector2D(Here);
	const float WantYaw = static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(ToPile.Y, ToPile.X)));
	SetActorRotation(FRotator(0.f, FMath::FixedTurn(static_cast<float>(GetActorRotation().Yaw), WantYaw, ParkKeeper::TurnRate * DeltaSeconds), 0.f));

	Phase += DeltaSeconds * ParkKeeper::StrokeRate;
	const float Stroke = FMath::Sin(Phase);
	PoseLimbs(0.f, ParkKeeper::ArmsRaking + ParkKeeper::ArmStroke * Stroke);
	RakeGrip->SetRelativeLocationAndRotation(ParkKeeper::GripAt + FVector(ParkKeeper::StrokeReach * Stroke, 0.f, 0.f),
		FRotator(ParkKeeper::RakeDown - 4.f * Stroke, 0.f, 0.f));

	Raking -= DeltaSeconds;
	if (Raking <= 0.f)
	{
		Raking = 0.f;
		Phase = 0.f;
		Next = (Next + 1) % Spots.Num();
	}
}
