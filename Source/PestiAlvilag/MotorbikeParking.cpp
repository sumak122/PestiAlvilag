// Fill out your copyright notice in the Description page of Project Settings.

#include "MotorbikeParking.h"

#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Motorbike.h"
#include "MotorbikeModel.h"
#include "TrafficManager.h"

namespace MotorbikeParkingRun
{
	constexpr int32 NumParked = 3;
	// Parked this far from the camera, so it is never seen appearing, and not further, so it is found (cm).
	constexpr double NearestSpot = 6000.0;
	constexpr double FarthestSpot = 30000.0;
	// Apart from each other (cm).
	constexpr double Spacing = 8000.0;
	// From the lane's middle to the kerb, and the box's middle over the road (cm).
	constexpr double KerbOffset = 240.0;
	constexpr double Lift = 120.0;
}

AMotorbikeParking::AMotorbikeParking()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 2.f;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void AMotorbikeParking::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	ATrafficManager* Traffic = TrafficRef.Get();
	if (!Traffic)
	{
		TActorIterator<ATrafficManager> It(GetWorld());
		if (!It)
		{
			return;
		}
		Traffic = *It;
		TrafficRef = Traffic;
	}
	const APlayerCameraManager* Viewer = UGameplayStatics::GetPlayerCameraManager(this, 0);
	if (!Viewer || Traffic->NumLanes() == 0)
	{
		return;
	}
	// Those taken by the player are his now.
	Parked.RemoveAll([](const TObjectPtr<AMotorbike>& Bike) { return !IsValid(Bike) || Bike->WasTaken(); });
	if (Parked.Num() < MotorbikeParkingRun::NumParked)
	{
		ParkOne(Traffic, Viewer->GetCameraLocation());
	}
}

bool AMotorbikeParking::ParkOne(ATrafficManager* Traffic, const FVector& Eye)
{
	UWorld* World = GetWorld();
	FCollisionQueryParams Params(SCENE_QUERY_STAT(MotorbikeParkingSpot), false, this);
	const FVector Half = MotorbikeModel::OutfitHalf(true);
	const FCollisionShape BikeBox = FCollisionShape::MakeBox(FVector(Half.X + 30.0, Half.Y + 20.0, 50.0));
	for (int32 Try = 0; Try < 40; ++Try)
	{
		const int32 LaneIndex = FMath::RandRange(0, Traffic->NumLanes() - 1);
		const TArray<FVector>& Points = Traffic->GetLane(LaneIndex).Points;
		if (Points.Num() < 2)
		{
			continue;
		}
		const int32 p = FMath::RandRange(1, Points.Num() - 1);
		const double Away = FVector::Dist2D(Points[p], Eye);
		if (Away < MotorbikeParkingRun::NearestSpot || Away > MotorbikeParkingRun::FarthestSpot)
		{
			continue;
		}
		bool bCrowded = false;
		for (const AMotorbike* Other : Parked)
		{
			bCrowded = bCrowded || (Other && FVector::Dist2D(Other->GetActorLocation(), Points[p]) < MotorbikeParkingRun::Spacing);
		}
		if (bCrowded)
		{
			continue;
		}
		// The kerb is on the left when keeping left, the sidecar on that side; it faces along the lane.
		const FVector Ahead = (Points[p] - Points[p - 1]).GetSafeNormal2D();
		if (Ahead.IsNearlyZero())
		{
			continue;
		}
		const FVector Left(Ahead.Y, -Ahead.X, 0.0);
		const FVector Spot = Points[p] + Left * MotorbikeParkingRun::KerbOffset;
		FHitResult Ground;
		if (!World->LineTraceSingleByChannel(Ground, Spot + FVector(0.0, 0.0, 5000.0), Spot - FVector(0.0, 0.0, 5000.0), ECC_Visibility, Params))
		{
			continue;
		}
		const FQuat Facing(FRotator(0.0, FMath::RadiansToDegrees(FMath::Atan2(Ahead.Y, Ahead.X)), 0.0));
		const FVector Middle = Ground.ImpactPoint + FVector(0.0, 0.0, MotorbikeParkingRun::Lift);
		if (World->OverlapAnyTestByChannel(Middle, Facing, ECC_WorldStatic, BikeBox, Params)
			|| World->OverlapAnyTestByChannel(Middle, Facing, ECC_Pawn, BikeBox, Params))
		{
			continue;
		}
		const FTransform Place(Facing, Middle);
		AMotorbike* Bike = World->SpawnActorDeferred<AMotorbike>(AMotorbike::StaticClass(), Place, nullptr, nullptr,
			ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
		if (!Bike)
		{
			return false;
		}
		Bike->SetLook(true, FMath::RandRange(0, MotorbikeModel::NumPaints - 1));
		Bike->SetOwned(true);
		Bike->FinishSpawning(Place);
		Parked.Add(Bike);
		return true;
	}
	return false;
}
