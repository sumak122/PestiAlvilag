// Fill out your copyright notice in the Description page of Project Settings.

#include "SoundSpots.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/SceneComponent.h"
#include "DayNight.h"
#include "Engine/World.h"
#include "KeletiStation.h"
#include "Kismet/GameplayStatics.h"
#include "MarketHall.h"
#include "SpotSound.h"

namespace SoundSpots
{
	/** How many spots are heard at once. */
	constexpr int32 MaxHeard = 6;
	/** How long a spot fades out before its sound is free for another (s). */
	constexpr float FadeOut = 0.6f;

	// Places (cm, X east, Y south).
	// The parish church on Horváth Mihály tér, just south of the map: its bell carries far.
	const FVector2D ChurchAt(40000.0, 78000.0);
	// The Teleki tér market.
	const FVector2D TelekiAt(99969.0, 28889.0);
	// Tisza Kálmán tér, the park.
	const FVector2D ParkAt(60000.0, 500.0);
	// The café on Tisza Kálmán tér (PestiStart::CafeAt, the middle of its terrace).
	const FVector2D CafeAt(62685.0, 14408.0);
	// The workshops of Garay utca.
	const FVector2D GarayAt(74888.0, -51702.0);
	// The squares where the pigeon flocks keep (as in Pigeons.cpp).
	const FVector2D PigeonsAt[] = {
		FVector2D(-6500.0, 1500.0),
		FVector2D(60000.0, 500.0),
		FVector2D(99969.0, 28889.0),
		FVector2D(90531.0, -35706.0),
		FVector2D(91000.0, -41600.0),
	};
	// Courtyards inside the blocks, at least 140 m apart (from the inner rings of the buildings in OpenStreetMap).
	const FVector2D YardsAt[] = {
		FVector2D(84694.0, 15716.0),
		FVector2D(16285.0, 24653.0),
		FVector2D(83865.0, -29194.0),
		FVector2D(25946.0, -23142.0),
		FVector2D(32578.0, 10313.0),
		FVector2D(39503.0, -27091.0),
		FVector2D(57925.0, -25566.0),
		FVector2D(-18178.0, 4656.0),
		FVector2D(20346.0, -3110.0),
		FVector2D(-7691.0, 24706.0),
		FVector2D(-2057.0, 37844.0),
		FVector2D(83836.0, 63637.0),
		FVector2D(8776.0, -27690.0),
		FVector2D(59800.0, -57382.0),
		FVector2D(-24499.0, 52581.0),
		FVector2D(64762.0, 69722.0),
		FVector2D(3808.0, -13161.0),
		FVector2D(90670.0, 29497.0),
		FVector2D(-14309.0, -36143.0),
		FVector2D(-28436.0, -12654.0),
		FVector2D(97948.0, -62917.0),
		FVector2D(49955.0, 29729.0),
		FVector2D(25936.0, -39610.0),
		FVector2D(-15.0, 55678.0),
		FVector2D(102546.0, 71036.0),
		FVector2D(-25511.0, -49264.0),
		FVector2D(125475.0, 56662.0),
		FVector2D(103567.0, 43006.0),
		FVector2D(94028.0, 5056.0),
		FVector2D(-4290.0, -46368.0),
		FVector2D(-14325.0, -70147.0),
		FVector2D(-1453.0, 12072.0),
		FVector2D(76198.0, 46245.0),
		FVector2D(910.0, -68308.0),
		FVector2D(28476.0, 64929.0),
		FVector2D(-29842.0, 36104.0),
		FVector2D(62088.0, 39926.0),
		FVector2D(69214.0, -40321.0),
		FVector2D(76646.0, -58906.0),
		FVector2D(69921.0, 22748.0),
		FVector2D(40860.0, -1039.0),
		FVector2D(8797.0, 68543.0),
		FVector2D(-10522.0, 65606.0),
		FVector2D(-13975.0, -18758.0),
		FVector2D(31842.0, -70264.0),
		FVector2D(43775.0, 66704.0),
		FVector2D(-21180.0, 18387.0),
		FVector2D(-24790.0, 67736.0),
		FVector2D(136848.0, -61700.0),
		FVector2D(117198.0, 70095.0),
		FVector2D(89117.0, -51733.0),
		FVector2D(10525.0, -46190.0),
		FVector2D(123137.0, -67178.0),
		FVector2D(101822.0, 56948.0),
		FVector2D(46086.0, -41652.0),
		FVector2D(44386.0, -60730.0),
		FVector2D(15272.0, -70842.0),
		FVector2D(44480.0, 51586.0),
		FVector2D(-30656.0, -63408.0),
	};
}

ASoundSpots::ASoundSpots()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	for (int32 p = 0; p < SoundSpots::MaxHeard; ++p)
	{
		USpotSound* Player = CreateDefaultSubobject<USpotSound>(FName(*FString::Printf(TEXT("Spot%d"), p)));
		Player->SetupAttachment(RootComponent);
		Players.Add(Player);
	}
}

void ASoundSpots::AddSpot(CitySynth::ESpot Kind, const FVector2D& At, double Reach, float Loud, double Height)
{
	FSpot Spot;
	Spot.Kind = Kind;
	Spot.At = At;
	Spot.Reach = Reach;
	Spot.Loud = Loud;
	Spot.Height = Height;
	Spots.Add(Spot);
}

void ASoundSpots::BeginPlay()
{
	Super::BeginPlay();
	using CitySynth::ESpot;

	AddSpot(ESpot::Church, SoundSpots::ChurchAt, 45000.0, 1.f, 3500.0);

	// Keleti: along the tracks behind the front.
	AddSpot(ESpot::Station, AKeletiStation::ToCity(FVector2D(0.0, -8000.0)), 12000.0, 0.9f, 400.0);
	AddSpot(ESpot::Station, AKeletiStation::ToCity(FVector2D(2500.0, -18000.0)), 12000.0, 0.9f, 400.0);

	AddSpot(ESpot::Market, SoundSpots::TelekiAt, 6000.0, 1.f);
	AddSpot(ESpot::Market, AMarketHall::ToCity(FVector2D(3555.0, 0.0)), 5000.0, 1.f, 300.0);

	for (const FVector2D& At : SoundSpots::PigeonsAt)
	{
		AddSpot(ESpot::Pigeons, At, 2500.0, 0.8f, 50.0);
	}

	// The park: sparrows in the trees, crickets in the grass.
	const FVector2D Around[4] = { FVector2D(2500.0, 0.0), FVector2D(-2500.0, 0.0), FVector2D(0.0, 2500.0), FVector2D(0.0, -2500.0) };
	for (const FVector2D& Offset : Around)
	{
		AddSpot(ESpot::Sparrows, SoundSpots::ParkAt + Offset, 3500.0, 0.8f, 600.0);
		AddSpot(ESpot::Crickets, SoundSpots::ParkAt + Offset * 0.6, 3000.0, 0.8f, 20.0);
	}

	AddSpot(ESpot::Gramophone, SoundSpots::CafeAt, 2000.0, 0.8f);
	AddSpot(ESpot::Smith, SoundSpots::GarayAt, 3500.0, 0.8f);

	// The courtyards, in turn: a dog, a carpet being beaten, a smith or a gramophone at an open window.
	int32 Yard = 0;
	for (const FVector2D& At : SoundSpots::YardsAt)
	{
		switch (Yard % 6)
		{
		case 0:
		case 3:
			AddSpot(ESpot::Dog, At, 4500.0, 0.8f, 50.0);
			break;
		case 1:
		case 4:
			AddSpot(ESpot::Carpet, At, 3500.0, 0.8f, 300.0);
			break;
		case 2:
			AddSpot(ESpot::Smith, At, 3000.0, 0.7f);
			break;
		default:
			AddSpot(ESpot::Gramophone, At, 1800.0, 0.7f, 800.0);
			break;
		}
		++Yard;
	}

	Slots.SetNum(Players.Num());
	for (USpotSound* Player : Players)
	{
		Player->Start();
	}
}

bool ASoundSpots::IsAwake(CitySynth::ESpot Kind, float Hour, float Darkness)
{
	using CitySynth::ESpot;
	switch (Kind)
	{
	case ESpot::Market:
		return Hour >= 6.f && Hour < 19.f;
	case ESpot::Pigeons:
		return Darkness < 0.5f;
	case ESpot::Sparrows:
		return Hour >= 5.f && Hour < 20.f && Darkness < 0.4f;
	case ESpot::Crickets:
		return Darkness > 0.5f;
	case ESpot::Gramophone:
		return Hour >= 9.f;
	case ESpot::Carpet:
		return Hour >= 8.f && Hour < 17.f;
	case ESpot::Smith:
		return Hour >= 7.f && Hour < 18.f;
	default:
		return true;
	}
}

void ASoundSpots::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0);
	if (!Camera || Slots.Num() != Players.Num())
	{
		return;
	}
	const FVector Ear = Camera->GetCameraLocation();
	const FVector2D From(Ear.X, Ear.Y);
	float Hour = 12.f;
	float Darkness = 0.f;
	if (const ADayNight* Clock = ADayNight::Get(GetWorld()))
	{
		Hour = Clock->GetHour();
		Darkness = FMath::Clamp(Clock->GetDarkness(), 0.f, 1.f);
	}

	// The spots in reach, loudest first.
	TArray<TPair<float, int32>> InReach;
	for (int32 s = 0; s < Spots.Num(); ++s)
	{
		const FSpot& Spot = Spots[s];
		const double Distance = FVector2D::Distance(From, Spot.At);
		if (Distance >= Spot.Reach || !IsAwake(Spot.Kind, Hour, Darkness))
		{
			continue;
		}
		InReach.Add(TPair<float, int32>(Spot.Loud * static_cast<float>(1.0 - Distance / Spot.Reach), s));
	}
	InReach.Sort([](const TPair<float, int32>& L, const TPair<float, int32>& R) { return L.Key > R.Key; });
	TArray<int32> Wanted;
	for (int32 w = 0; w < InReach.Num() && w < SoundSpots::MaxHeard; ++w)
	{
		Wanted.Add(InReach[w].Value);
	}

	// Spots no longer wanted fade out; once quiet their sound is free.
	for (int32 p = 0; p < Slots.Num(); ++p)
	{
		FSlot& Slot = Slots[p];
		if (Slot.Spot == INDEX_NONE)
		{
			continue;
		}
		if (Slot.Fading > 0.f || !Wanted.Contains(Slot.Spot))
		{
			if (Slot.Fading <= 0.f)
			{
				Slot.Fading = SoundSpots::FadeOut;
				Players[p]->SetHeard(0.f, 1.f);
			}
			Slot.Fading -= DeltaSeconds;
			if (Slot.Fading <= 0.f)
			{
				Slot.Spot = INDEX_NONE;
			}
			continue;
		}
		Wanted.Remove(Slot.Spot);
	}

	// Newly wanted spots take a free sound.
	for (const int32 Spot : Wanted)
	{
		for (int32 p = 0; p < Slots.Num(); ++p)
		{
			if (Slots[p].Spot == INDEX_NONE)
			{
				Slots[p].Spot = Spot;
				Slots[p].Fading = 0.f;
				Players[p]->SetSpot(Spots[Spot].Kind, NextGeneration++);
				break;
			}
		}
	}

	// Place each playing sound at its spot and say how loud it reaches the listener.
	for (int32 p = 0; p < Slots.Num(); ++p)
	{
		const FSlot& Slot = Slots[p];
		USpotSound* Player = Players[p];
		Player->SetHour(Hour);
		if (Slot.Spot == INDEX_NONE || Slot.Fading > 0.f)
		{
			continue;
		}
		const FSpot& Spot = Spots[Slot.Spot];
		// The streets are near level, so the ground is taken as just under the camera.
		Player->SetWorldLocation(FVector(Spot.At.X, Spot.At.Y, Ear.Z - 200.0 + Spot.Height));
		const double Toward = FMath::Clamp(FVector2D::Distance(From, Spot.At) / Spot.Reach, 0.0, 1.0);
		const float Near = static_cast<float>(1.0 - Toward);
		Player->SetHeard(Spot.Loud * Near * Near, static_cast<float>(Toward));
	}
}
