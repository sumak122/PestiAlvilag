// Fill out your copyright notice in the Description page of Project Settings.

#include "HandCarts.h"

#include "Camera/PlayerCameraManager.h"
#include "DayNight.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HandCart.h"
#include "HandCartModel.h"
#include "Kismet/GameplayStatics.h"
#include "TrafficManager.h"
#include "Weather.h"

namespace HandCartsRun
{
	// One at a time, a few minutes apart (s): porters are a rare sight.
	constexpr int32 MostOut = 1;
	constexpr float FirstLow = 90.f;
	constexpr float FirstHigh = 240.f;
	constexpr float NextLow = 180.f;
	constexpr float NextHigh = 420.f;
	// Only by day (hours), and not in more than a drizzle.
	constexpr float FromHour = 7.f;
	constexpr float ToHour = 18.f;
	constexpr float MostRain = 0.3f;
	// Started this far from the camera (cm), out of its view unless asked; near is for the test spawn menu.
	constexpr double NearestStart = 4000.0;
	constexpr double FarthestStart = 12000.0;
	constexpr double TestNearest = 1500.0;
	constexpr double TestFarthest = 4000.0;
	constexpr double OutOfView = 0.2;
	// The box's middle over the road when it starts (it settles onto it).
	constexpr double Lift = 120.0;
}

AHandCarts::AHandCarts()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 2.f;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Wait = FMath::FRandRange(HandCartsRun::FirstLow, HandCartsRun::FirstHigh);
}

void AHandCarts::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Out.RemoveAll([](const TObjectPtr<AHandCart>& Cart) { return !IsValid(Cart) || !Cart->IsInTraffic(); });
	if (Out.Num() >= HandCartsRun::MostOut)
	{
		return;
	}
	Wait -= DeltaSeconds;
	if (Wait > 0.f)
	{
		return;
	}
	UWorld* World = GetWorld();
	const ADayNight* Sky = ADayNight::Get(World);
	const float Hour = Sky ? Sky->GetHour() : 12.f;
	const AWeather* Spell = AWeather::Get(World);
	if (Hour < HandCartsRun::FromHour || Hour > HandCartsRun::ToHour || (Spell && Spell->GetRain() > HandCartsRun::MostRain))
	{
		Wait = 30.f;
		return;
	}
	const APlayerCameraManager* Viewer = UGameplayStatics::GetPlayerCameraManager(this, 0);
	if (!Viewer)
	{
		return;
	}
	if (SpawnNear(World, Viewer->GetCameraLocation(), false))
	{
		for (TActorIterator<AHandCart> It(World); It; ++It)
		{
			if (It->IsInTraffic() && !Out.Contains(*It))
			{
				Out.Add(*It);
			}
		}
		Wait = FMath::FRandRange(HandCartsRun::NextLow, HandCartsRun::NextHigh);
	}
	else
	{
		Wait = 10.f;
	}
}

bool AHandCarts::SpawnNear(UWorld* World, const FVector& Near, bool bInView)
{
	if (!World)
	{
		return false;
	}
	TActorIterator<ATrafficManager> Found(World);
	if (!Found || Found->NumLanes() == 0)
	{
		return false;
	}
	ATrafficManager* Traffic = *Found;
	const APlayerCameraManager* Viewer = UGameplayStatics::GetPlayerCameraManager(World, 0);
	const double Nearest = bInView ? HandCartsRun::TestNearest : HandCartsRun::NearestStart;
	const double Farthest = bInView ? HandCartsRun::TestFarthest : HandCartsRun::FarthestStart;
	for (int32 Try = 0; Try < 60; ++Try)
	{
		const int32 LaneIndex = FMath::RandRange(0, Traffic->NumLanes() - 1);
		const TArray<FVector>& Points = Traffic->GetLane(LaneIndex).Points;
		if (Points.Num() < 3)
		{
			continue;
		}
		const int32 p = FMath::RandRange(1, Points.Num() - 2);
		const FVector ToSpot = Points[p] - Near;
		const double Away = ToSpot.Size2D();
		if (Away < Nearest || Away > Farthest)
		{
			continue;
		}
		if (!bInView && Viewer && FVector::DotProduct(ToSpot.GetSafeNormal2D(), Viewer->GetCameraRotation().Vector().GetSafeNormal2D()) > HandCartsRun::OutOfView)
		{
			continue;
		}
		FHitResult Ground;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(HandCartsSpot), false);
		if (!World->LineTraceSingleByChannel(Ground, Points[p] + FVector(0.0, 0.0, 5000.0), Points[p] - FVector(0.0, 0.0, 5000.0), ECC_Visibility, Params))
		{
			continue;
		}
		const FVector Ahead = (Points[p + 1] - Points[p]).GetSafeNormal2D();
		const FTransform Place(FRotator(0.0, Ahead.Rotation().Yaw, 0.0), Ground.ImpactPoint + FVector(0.0, 0.0, HandCartsRun::Lift));
		AHandCart* Cart = World->SpawnActorDeferred<AHandCart>(AHandCart::StaticClass(), Place, nullptr, nullptr,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!Cart)
		{
			return false;
		}
		Cart->SetLoad(static_cast<HandCartModel::ELoad>(FMath::RandRange(0, static_cast<int32>(HandCartModel::ELoad::Count) - 1)));
		Cart->FinishSpawning(Place);
		Cart->StartInTraffic(Traffic, LaneIndex, p + 1);
		return true;
	}
	return false;
}
