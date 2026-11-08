// Fill out your copyright notice in the Description page of Project Settings.

#include "PistaRuns.h"

#include "Components/SceneComponent.h"
#include "DeliveryModel.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "FleaMarket.h"
#include "GameFramework/PlayerController.h"
#include "GarayModel.h"
#include "GoalMarkerComponent.h"
#include "HorseCart.h"
#include "MarketHall.h"
#include "Mission.h"
#include "PestiAlvilagGameModeBase.h"
#include "PestiHud.h"
#include "PestiPerson.h"
#include "PestiText.h"
#include "RouteGuideComponent.h"
#include "StoryPerson.h"
#include "StreetName.h"

namespace PistaRun
{
	// The mission that opens the job.
	const FName OpenedBy(TEXT("Zsibvasar2"));
	const TCHAR* const PistaName = TEXT("Pista b\u00E1csi");
	// On-screen note key (under the goal).
	constexpr int32 NoteKey = 71;
	// How close to him the player stands to talk (cm).
	constexpr double TalkRange = 260.0;
	// The cart stands this close to the place to be loaded, and to his stall to be paid (cm).
	constexpr double PickupRadius = 700.0;
	constexpr double BackRadius = 1500.0;
	// Four crates, one every this many seconds while the cart stands still.
	constexpr int32 Load = 4;
	constexpr float LoadEvery = 1.2f;
	constexpr float StillTime = 0.6f;
	// What he pays for each crate that comes back (fillér).
	constexpr int32 PayPerCrate = 40;
	// The market hall's side door on Vásár utca (the hall's frame, as in mission 4).
	const FVector2D HallDoorLocal(4300.0, -4560.0);
	// Before Keleti's taxi stand, and in front of Schwimmer's timber yard (their frames).
	const FVector StationLocal(200.0, -300.0, 0.0);
	constexpr double TimberOut = -400.0;

	struct FPickup
	{
		const TCHAR* WhereHu;
		const TCHAR* WhereEn;
		const TCHAR* WhatHu;
		const TCHAR* WhatEn;
	};
	const FPickup Pickups[] = {
		{ TEXT("a v\u00E1s\u00E1rcsarnok oldals\u00F3 kapuj\u00E1hoz, a V\u00E1s\u00E1r utc\u00E1ba"), TEXT("the market hall's side door on V\u00E1s\u00E1r utca"),
			TEXT("n\u00E9gy l\u00E1da alma"), TEXT("four crates of apples") },
		{ TEXT("a Keleti p\u00E1lyaudvarhoz, a taxi\u00E1ll\u00E1s mell\u00E9"), TEXT("Keleti station, by the taxi stand"),
			TEXT("n\u00E9gy l\u00E1da vid\u00E9ki holmi"), TEXT("four crates of goods off the country train") },
		{ TEXT("Schwimmer fatelep\u00E9re, a Garay utc\u00E1ba"), TEXT("Schwimmer's timber yard on Garay utca"),
			TEXT("n\u00E9gy l\u00E1da r\u00E9gi szersz\u00E1m"), TEXT("four crates of old tools") },
	};
	constexpr int32 NumPickups = static_cast<int32>(UE_ARRAY_COUNT(Pickups));
}

APistaRuns::APistaRuns()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Marker = CreateDefaultSubobject<UGoalMarkerComponent>(TEXT("Marker"));
	Marker->SetupAttachment(RootComponent);
	Guide = CreateDefaultSubobject<URouteGuideComponent>(TEXT("Guide"));
	Guide->SetupAttachment(RootComponent);
}

AHorseCart* APistaRuns::FindCart() const
{
	for (TActorIterator<AHorseCart> It(GetWorld()); It; ++It)
	{
		// (Not one of the carriers' carts in the traffic.)
		if (!It->IsCityCart())
		{
			return *It;
		}
	}
	return nullptr;
}

bool APistaRuns::IsMissionUnderway() const
{
	for (TActorIterator<AMission> It(GetWorld()); It; ++It)
	{
		if (It->IsUnderway())
		{
			return true;
		}
	}
	return false;
}

double APistaRuns::GroundAt(const FVector2D& At) const
{
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(PistaRunGround), false, this);
	return GetWorld()->LineTraceSingleByObjectType(Hit, FVector(At, 10000.0), FVector(At, -10000.0), FCollisionObjectQueryParams(ECC_WorldStatic), Params)
		? Hit.ImpactPoint.Z : 0.0;
}

void APistaRuns::Note(const FString& Line, float Seconds) const
{
	APestiHUD::Show(this, PistaRun::NoteKey, Seconds, FColor(255, 200, 60), Line);
}

void APistaRuns::PistaSays(const FString& Hu, const FString& En) const
{
	if (APestiHUD* Hud = APestiHUD::Get(this))
	{
		Hud->Say(PistaRun::PistaName, Hu, En, 7.f);
	}
}

void APistaRuns::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this);
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	APawn* Player = PC ? PC->GetPawn() : nullptr;
	if (!Game || !Player || !Game->IsMissionDone(PistaRun::OpenedBy))
	{
		return;
	}
	const bool bMission = IsMissionUnderway();
	if (State != EPistaRun::None && bMission)
	{
		// A mission takes over: the run is off.
		StopRun();
		return;
	}

	// Talking to him, on foot: E for a run (or to hear it again).
	AStoryPerson* Pista = AFleaMarket::FindKeeper(GetWorld(), PistaRun::PistaName);
	const APestiPerson* Person = Cast<APestiPerson>(Player);
	const bool bByPista = Pista && Person && !Person->IsRiding() && !Person->IsCarrying()
		&& FVector::Dist2D(Person->GetActorLocation(), Pista->GetActorLocation()) < PistaRun::TalkRange;
	if (bByPista && !bMission)
	{
		if (APestiHUD* Hud = APestiHUD::Get(this))
		{
			if (State == EPistaRun::None)
			{
				Hud->Prompt(TEXT("E"), TEXT("Fuvar Pista b\u00E1csinak"), TEXT("A cart run for Pista b\u00E1csi"), false);
			}
			else
			{
				Hud->Prompt(TEXT("E"), TEXT("Hov\u00E1 is kell menni?"), TEXT("Where was it again?"), false);
			}
		}
		if (PC->WasInputKeyJustPressed(EKeys::E))
		{
			if (State == EPistaRun::None)
			{
				PistaAt = Pista->GetActorLocation();
				StartRun();
			}
			else
			{
				SayRun();
			}
		}
	}

	if (State != EPistaRun::None)
	{
		TickRun(Player, Pista, DeltaSeconds);
	}
}

void APistaRuns::StartRun()
{
	Cart = FindCart();
	if (!Cart)
	{
		PistaSays(TEXT("Csillag ma pihen, fiam. Gyere vissza k\u00E9s\u0151bb!"), TEXT("Csillag is resting today, son. Come back later!"));
		return;
	}
	Pickup = FMath::RandRange(0, PistaRun::NumPickups - 1);
	FVector2D At;
	switch (Pickup)
	{
	case 0:
		At = AMarketHall::ToCity(PistaRun::HallDoorLocal);
		break;
	case 1:
		At = FVector2D(DeliveryModel::ToCity(DeliveryModel::EPlace::Station, PistaRun::StationLocal));
		break;
	default:
		At = GarayModel::CityAt + FVector2D(FRotator(0.0, GarayModel::CityYaw, 0.0).Vector()) * PistaRun::TimberOut;
		break;
	}
	PickupAt = FVector(At, GroundAt(At));
	Cart->SetCrates(0);
	FVector Stale;
	while (Cart->TakeFallenCrate(Stale))
	{
	}
	LoadClock = 0.f;
	StoodFor = 0.f;
	State = EPistaRun::ToCart;
	SayRun();
}

void APistaRuns::SayRun() const
{
	const PistaRun::FPickup& Place = PistaRun::Pickups[Pickup];
	PistaSays(FString::Printf(TEXT("Menj el a kocsival %s, ott v\u00E1r %s. Hozd ide \u00E9pen, l\u00E1d\u00E1nk\u00E9nt fizetek!"), Place.WhereHu, Place.WhatHu),
		FString::Printf(TEXT("Take the cart to %s; %s are waiting there. Bring them back in one piece, I pay by the crate!"), Place.WhereEn, Place.WhatEn));
}

void APistaRuns::Lead(const FVector& From, const FVector& Target, const FString& GoalHu, const FString& GoalEn)
{
	Marker->ShowAt(Target);
	Guide->Guide(From, Target);
	Note(FString::Printf(TEXT("%s  (%s)   %s %s"), *GoalHu, *GoalEn, *PestiText::Metres(FVector::Dist2D(From, Target)), PestiText::Compass(From, Target)), 0.f);
}

void APistaRuns::TickRun(APawn* Player, const AStoryPerson* Pista, float DeltaSeconds)
{
	if (!IsValid(Cart.Get()))
	{
		StopRun();
		return;
	}
	// Crates thrown off on the way are lost.
	FVector Fell;
	while (Cart->TakeFallenCrate(Fell))
	{
		Note(TEXT("Leesett egy l\u00E1da!  (A crate fell off!)"), 3.f);
	}

	const FVector At = Player->GetActorLocation();
	const bool bOnCart = Player == Cart.Get();
	if (State != EPistaRun::ToCart && !bOnCart)
	{
		Lead(At, Cart->GetBenchLocation(), TEXT("\u00DClj vissza a bakra"), TEXT("Climb back on the cart"));
		return;
	}
	const FVector CartAt = Cart->GetActorLocation();
	const bool bStill = Cart->GetPace() == 0;
	switch (State)
	{
	case EPistaRun::ToCart:
		if (bOnCart)
		{
			State = EPistaRun::ToPickup;
			return;
		}
		Lead(At, Cart->GetBenchLocation(), TEXT("Menj Pista b\u00E1csi kocsij\u00E1hoz"), TEXT("Go to Pista b\u00E1csi's cart"));
		return;

	case EPistaRun::ToPickup:
		Lead(CartAt, PickupAt, TEXT("Hajts a rakod\u00E1shoz"), TEXT("Drive to the pick-up"));
		if (bStill && FVector::Dist2D(CartAt, PickupAt) < PistaRun::PickupRadius)
		{
			State = EPistaRun::Loading;
			LoadClock = 0.f;
		}
		return;

	case EPistaRun::Loading:
		if (!bStill || FVector::Dist2D(CartAt, PickupAt) > PistaRun::PickupRadius)
		{
			State = EPistaRun::ToPickup;
			return;
		}
		Marker->Hide();
		Guide->Stop();
		Note(FString::Printf(TEXT("A hord\u00E1r rakodik: %d / %d  (The porter is loading)"), Cart->GetCrates(), PistaRun::Load), 0.f);
		LoadClock += DeltaSeconds;
		if (LoadClock >= PistaRun::LoadEvery)
		{
			LoadClock = 0.f;
			Cart->SetCrates(Cart->GetCrates() + 1);
			if (Cart->GetCrates() >= PistaRun::Load)
			{
				State = EPistaRun::Back;
				StoodFor = 0.f;
				Note(TEXT("Megrakva! Vissza a Teleki t\u00E9rre.  (Loaded! Back to Teleki t\u00E9r.)"), 4.f);
			}
		}
		return;

	case EPistaRun::Back:
	{
		const FVector Stall = Pista ? Pista->GetActorLocation() : PistaAt;
		Lead(CartAt, Stall, TEXT("Vidd a l\u00E1d\u00E1kat Pista b\u00E1csinak a Teleki t\u00E9rre"), TEXT("Take the crates to Pista b\u00E1csi at Teleki t\u00E9r"));
		StoodFor = bStill && FVector::Dist2D(CartAt, Stall) < PistaRun::BackRadius ? StoodFor + DeltaSeconds : 0.f;
		if (StoodFor < PistaRun::StillTime)
		{
			return;
		}
		// Unloaded: paid by the crate.
		APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this);
		const int32 Crates = Cart->GetCrates();
		const int32 Pay = Crates * PistaRun::PayPerCrate;
		Cart->SetCrates(0);
		StopRun();
		if (Game)
		{
			Game->AddMoney(Pay);
			Game->AddCartRun();
			if (Crates > 0)
			{
				Game->AddRenown(StreetName::ForCartRun, TEXT("fuvar Pista b\u00E1csinak"), TEXT("a cart run for Pista b\u00E1csi"));
			}
		}
		const FString Money = APestiAlvilagGameModeBase::FormatMoney(Pay);
		if (Crates == PistaRun::Load)
		{
			PistaSays(FString::Printf(TEXT("Mind a n\u00E9gy megvan! Itt a %s, fiam."), *Money),
				FString::Printf(TEXT("All four of them! Here's %s, son."), *Money));
		}
		else if (Crates > 0)
		{
			PistaSays(FString::Printf(TEXT("Csak %d l\u00E1da? No, itt a %s, de vigy\u00E1zz jobban Csillagra!"), Crates, *Money),
				FString::Printf(TEXT("Only %d crates? Well, here's %s, but go easier on Csillag!"), Crates, *Money));
		}
		else
		{
			PistaSays(TEXT("\u00DCres a kocsi? Ezt nem fizetem ki, fiam."), TEXT("The cart's empty? I'm not paying for that, son."));
		}
		return;
	}

	default:
		return;
	}
}

void APistaRuns::StopRun()
{
	State = EPistaRun::None;
	Marker->Hide();
	Guide->Stop();
	Note(FString(), 0.1f);
}
