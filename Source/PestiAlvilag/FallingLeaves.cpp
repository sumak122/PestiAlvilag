// Fill out your copyright notice in the Description page of Project Settings.

#include "FallingLeaves.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace FallingLeaves
{
	// Leaf colours: gold, orange, rust and a late yellow-green.
	const FLinearColor LeafColours[] = {
		FLinearColor(0.55f, 0.30f, 0.03f),
		FLinearColor(0.50f, 0.14f, 0.02f),
		FLinearColor(0.28f, 0.08f, 0.02f),
		FLinearColor(0.42f, 0.36f, 0.05f),
	};
	constexpr int32 NumColours = static_cast<int32>(UE_ARRAY_COUNT(LeafColours));

	// A leaf is a card about 9 x 6 cm.
	const FVector LeafScale(0.09, 0.065, 0.004);

	// Share of new leaves that come off a tree when there is one near.
	constexpr float FromTreeChance = 0.85f;
	constexpr float CrownRadius = 280.f;
	constexpr float CrownLow = 350.f;
	constexpr float CrownHigh = 750.f;
	// Leaves blown through the streets start this high (cm).
	constexpr float StreetLow = 250.f;
	constexpr float StreetHigh = 650.f;

	constexpr float FallSpeed = 70.f;
	constexpr float SwaySpeed = 45.f;
	constexpr float CatchWind = 1.5f;
	constexpr float MinLie = 4.f;
	constexpr float MaxLie = 9.f;
	// Above this share of a full gust, lying leaves skitter along.
	constexpr float SkitterGust = 0.7f;
	constexpr float SkitterShare = 0.3f;
	constexpr double RefindTrees = 500.0;
}

AFallingLeaves::AFallingLeaves()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	BaseMaterial = Material.Object;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));

	for (int32 c = 0; c < FallingLeaves::NumColours; ++c)
	{
		UInstancedStaticMeshComponent* Instances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(*FString::Printf(TEXT("Leaves%d"), c));
		Instances->SetupAttachment(RootComponent);
		Instances->SetStaticMesh(CubeMesh.Object);
		Instances->SetMobility(EComponentMobility::Movable);
		Instances->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Instances->SetCastShadow(false);
		Colours.Add(Instances);
	}
}

void AFallingLeaves::SetTrees(const TArray<FVector4>& InTrees)
{
	Trees = InTrees;
	NearTreesFrom = FVector(1e9);
}

void AFallingLeaves::BeginPlay()
{
	Super::BeginPlay();

	Random.Initialize(1932);
	Leaves.SetNum(LeafCount);
	Frames.SetNum(Colours.Num());
	for (int32 c = 0; c < Colours.Num(); ++c)
	{
		if (BaseMaterial)
		{
			UMaterialInstanceDynamic* Paint = UMaterialInstanceDynamic::Create(BaseMaterial, this);
			Paint->SetVectorParameterValue(TEXT("Color"), FallingLeaves::LeafColours[c]);
			Colours[c]->SetMaterial(0, Paint);
		}
		// Leaf i is drawn by colour i % NumColours, as instance i / NumColours.
		const int32 Count = (LeafCount + Colours.Num() - 1 - c) / Colours.Num();
		Frames[c].Init(FTransform(FRotator::ZeroRotator, FVector::ZeroVector, FVector::ZeroVector), Count);
		Colours[c]->AddInstances(Frames[c], false, true);
	}
}

bool AFallingLeaves::CameraLocation(FVector& OutLocation) const
{
	const APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC || !PC->PlayerCameraManager)
	{
		return false;
	}
	OutLocation = PC->PlayerCameraManager->GetCameraLocation();
	return true;
}

void AFallingLeaves::Release(FLeaf& Leaf, const FVector& Around, bool bScatter)
{
	using namespace FallingLeaves;

	double Top;
	if (NearTrees.Num() > 0 && Random.FRand() < FromTreeChance)
	{
		const FVector4& Tree = Trees[NearTrees[Random.RandRange(0, NearTrees.Num() - 1)]];
		const FVector2D Off = FVector2D(Random.FRandRange(-1.f, 1.f), Random.FRandRange(-1.f, 1.f)) * CrownRadius * Tree.W;
		Leaf.GroundZ = Tree.Z;
		Top = Tree.Z + Random.FRandRange(CrownLow, CrownHigh) * Tree.W;
		Leaf.Location = FVector(Tree.X + Off.X, Tree.Y + Off.Y, Top);
	}
	else
	{
		const FVector2D Off = FVector2D(Random.FRandRange(-1.f, 1.f), Random.FRandRange(-1.f, 1.f)) * Range * 0.8;
		const FVector Spot(Around.X + Off.X, Around.Y + Off.Y, Around.Z);
		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(LeafGround), false, this);
		Leaf.GroundZ = GetWorld()->LineTraceSingleByChannel(Hit, Spot + FVector(0.0, 0.0, 3000.0), Spot - FVector(0.0, 0.0, 5000.0), ECC_Visibility, Params)
			? Hit.ImpactPoint.Z : Around.Z - 170.0;
		Top = Leaf.GroundZ + Random.FRandRange(StreetLow, StreetHigh);
		Leaf.Location = FVector(Spot.X, Spot.Y, Top);
	}

	Leaf.Velocity = FVector::ZeroVector;
	Leaf.Phase = Random.FRandRange(0.f, 2.f * PI);
	Leaf.Flutter = Random.FRandRange(0.7f, 1.3f);
	Leaf.Spin = Random.FRandRange(-90.f, 90.f);
	Leaf.Yaw = Random.FRandRange(0.f, 360.f);
	Leaf.bLanded = false;

	// At the very start, spread them out: some already down, the rest on their way.
	if (bScatter)
	{
		if (Random.FRand() < 0.4f)
		{
			Leaf.bLanded = true;
			Leaf.Location.Z = Leaf.GroundZ + 1.0;
			Leaf.LieTime = Random.FRandRange(0.f, MaxLie);
		}
		else
		{
			Leaf.Location.Z = FMath::Lerp(Leaf.GroundZ + 20.0, Top, static_cast<double>(Random.FRand()));
		}
	}
}

void AFallingLeaves::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	using namespace FallingLeaves;

	FVector Camera;
	if (!CameraLocation(Camera) || Leaves.IsEmpty())
	{
		return;
	}
	Time += DeltaSeconds;

	if (FVector::Dist2D(Camera, NearTreesFrom) > RefindTrees)
	{
		NearTreesFrom = Camera;
		NearTrees.Reset();
		for (int32 i = 0; i < Trees.Num(); ++i)
		{
			if (FVector::Dist2D(FVector(Trees[i]), Camera) < Range)
			{
				NearTrees.Add(i);
			}
		}
	}

	// The wind turns slowly and comes in gusts.
	const float Heading = 0.6f + 0.5f * FMath::Sin(Time * 0.05f);
	const float Gust = FMath::Clamp(0.5f + 0.5f * FMath::Sin(Time * 0.7f) * FMath::Sin(Time * 0.23f + 1.f), 0.f, 1.f);
	const FVector WindDir(FMath::Cos(Heading), FMath::Sin(Heading), 0.0);
	const FVector Wind = WindDir * (WindSpeed * (0.6f + Gust));
	const FVector Across(-WindDir.Y, WindDir.X, 0.0);

	for (int32 i = 0; i < Leaves.Num(); ++i)
	{
		FLeaf& Leaf = Leaves[i];
		if (!bStarted)
		{
			Release(Leaf, Camera, true);
		}
		else if (FVector::Dist2D(Leaf.Location, Camera) > Range * 1.3)
		{
			Release(Leaf, Camera, false);
		}

		FRotator Turn;
		if (Leaf.bLanded)
		{
			Leaf.LieTime -= DeltaSeconds;
			if (Gust > SkitterGust)
			{
				Leaf.Location += Wind * (SkitterShare * DeltaSeconds);
				Leaf.Yaw += Leaf.Spin * DeltaSeconds;
			}
			if (Leaf.LieTime <= 0.f)
			{
				Release(Leaf, Camera, false);
			}
			Turn = FRotator(0.0, Leaf.Yaw, 0.0);
		}
		else
		{
			Leaf.Phase += DeltaSeconds * (2.5f + 2.f * Leaf.Flutter);
			Leaf.Velocity.X += (Wind.X - Leaf.Velocity.X) * FMath::Min(1.f, DeltaSeconds * CatchWind);
			Leaf.Velocity.Y += (Wind.Y - Leaf.Velocity.Y) * FMath::Min(1.f, DeltaSeconds * CatchWind);
			Leaf.Velocity.Z = -FallSpeed * Leaf.Flutter + SwaySpeed * FMath::Sin(Leaf.Phase);
			Leaf.Location += (Leaf.Velocity + Across * (SwaySpeed * FMath::Cos(Leaf.Phase * 0.8f))) * DeltaSeconds;
			Leaf.Yaw += Leaf.Spin * DeltaSeconds;
			if (Leaf.Location.Z <= Leaf.GroundZ + 1.0)
			{
				Leaf.Location.Z = Leaf.GroundZ + 1.0;
				Leaf.bLanded = true;
				Leaf.LieTime = Random.FRandRange(MinLie, MaxLie);
			}
			Turn = FRotator(50.f * FMath::Sin(Leaf.Phase), Leaf.Yaw, 60.f * FMath::Cos(Leaf.Phase * 0.7f));
		}

		Frames[i % Colours.Num()][i / Colours.Num()] = FTransform(Turn, Leaf.Location, LeafScale);
	}
	bStarted = true;

	for (int32 c = 0; c < Colours.Num(); ++c)
	{
		Colours[c]->BatchUpdateInstancesTransforms(0, Frames[c], true, true, true);
	}
}
