// Fill out your copyright notice in the Description page of Project Settings.

#include "FiakerStand.h"

#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HorseCartModel.h"
#include "Kismet/GameplayStatics.h"
#include "OsmGraybox.h"
#include "TrafficCart.h"
#include "TrafficManager.h"

namespace FiakerStandSpots
{
	// On the road in front of Keleti, behind the taxis' stand, facing west (cm, degrees).
	const FVector2D Spots[] = { FVector2D(100950.0, -37278.0), FVector2D(100300.0, -37278.0) };
	constexpr int32 NumSpots = 2;
	constexpr double Yaw = 180.0;
	// A new cab is put out only with the camera this far off (cm), and not onto a cart standing there.
	constexpr double SpawnAway = 7000.0;
	constexpr double KeepClear = 500.0;
	// Over the road like the traffic's carts (cm).
	constexpr double Lift = 20.0;
}

AFiakerStand::AFiakerStand()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 1.f;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void AFiakerStand::Tick(float DeltaSeconds)
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
	Waiting.SetNum(FiakerStandSpots::NumSpots);
	const APlayerCameraManager* Viewer = UGameplayStatics::GetPlayerCameraManager(this, 0);
	if (!Viewer)
	{
		return;
	}
	const FVector2D Eye(Viewer->GetCameraLocation());
	for (int32 i = 0; i < FiakerStandSpots::NumSpots; ++i)
	{
		ATrafficCart* Cab = Waiting[i];
		if (IsValid(Cab) && Cab->IsWaitingForHire())
		{
			continue;
		}
		// Hired off or taken: the stand's no longer its business.
		Waiting[i] = nullptr;
		const FVector2D Spot = FiakerStandSpots::Spots[i];
		if (FVector2D::Distance(Eye, Spot) < FiakerStandSpots::SpawnAway)
		{
			continue;
		}
		bool bClear = true;
		for (TActorIterator<ATrafficCart> It(GetWorld()); It; ++It)
		{
			if (FVector2D::Distance(FVector2D(It->GetActorLocation()), Spot) < FiakerStandSpots::KeepClear)
			{
				bClear = false;
				break;
			}
		}
		if (bClear)
		{
			PutCab(i, Traffic);
		}
	}
}

bool AFiakerStand::PutCab(int32 Index, ATrafficManager* Traffic)
{
	const FVector2D Spot = FiakerStandSpots::Spots[Index];
	FCollisionQueryParams Params(SCENE_QUERY_STAT(FiakerStandGround), false, this);
	TArray<FHitResult> Hits;
	GetWorld()->LineTraceMultiByObjectType(Hits, FVector(Spot, 20000.0), FVector(Spot, -20000.0),
		FCollisionObjectQueryParams(ECC_WorldStatic), Params);
	double GroundZ = 0.0;
	bool bGround = false;
	for (const FHitResult& Hit : Hits)
	{
		if (Cast<AOsmGraybox>(Hit.GetActor()))
		{
			GroundZ = Hit.ImpactPoint.Z;
			bGround = true;
			break;
		}
	}
	if (!bGround)
	{
		return false;
	}
	const FTransform Place(FRotator(0.0, FiakerStandSpots::Yaw, 0.0),
		FVector(Spot, GroundZ + HorseCartModel::RigHalf().Z + FiakerStandSpots::Lift));
	ATrafficCart* Cab = GetWorld()->SpawnActorDeferred<ATrafficCart>(ATrafficCart::StaticClass(), Place, nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Cab)
	{
		return false;
	}
	FRandomStream Dice(FMath::Rand());
	Cab->MakeFiaker(Dice, false);
	Cab->FinishSpawning(Place);
	Cab->WaitForHire(Traffic);
	Waiting[Index] = Cab;
	return true;
}
