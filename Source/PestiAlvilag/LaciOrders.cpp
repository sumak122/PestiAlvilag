// Fill out your copyright notice in the Description page of Project Settings.

#include "LaciOrders.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GoalMarkerComponent.h"
#include "LaciYard.h"
#include "Mission.h"
#include "PestiAlvilagGameModeBase.h"
#include "PestiCar.h"
#include "PestiHud.h"
#include "PestiPerson.h"
#include "PestiText.h"
#include "RouteGuideComponent.h"
#include "StoryPerson.h"
#include "TaxiCompany.h"
#include "TrafficManager.h"

namespace LaciOrder
{
	// The mission that opens the job.
	const FName OpenedBy(TEXT("ElsoAuto"));
	// On-screen note key (under the goal).
	constexpr int32 NoteKey = 70;
	// This share of the ordered cars are locked.
	constexpr float LockedChance = 0.7f;
	// How close to Laci the player stands to talk to him (cm).
	constexpr double TalkRange = 260.0;
	// The car stands this far from the yard, in a straight line (cm).
	constexpr double MinAway = 20000.0;
	constexpr double MaxAway = 80000.0;
	// Parked this far to the left of its lane, by the kerb (cm), and lifted to land on its wheels.
	constexpr double KerbOffset = 240.0;
	constexpr double CarLift = 120.0;
	// Over the pit: the bay round it (yard frame, cm), standing still this long (s).
	constexpr double BayX0 = LaciYardModel::PitX0 - 80.0;
	constexpr double BayX1 = LaciYardModel::PitX1 + 80.0;
	constexpr double BayHalf = 150.0;
	constexpr float StillSpeed = 40.f;
	constexpr float StillTime = 0.8f;
	// Somebody sees the theft and calls the police, one time in this many hundred.
	constexpr int32 SeenChance = 30;
	// A wanted hint by the yard at most this often (s), and within this distance (cm).
	constexpr double HintEvery = 8.0;
	constexpr double HintNear = 3000.0;
	// The handover: fade to black (s), the car goes, the screen clears.
	constexpr float FadeOut = 0.6f;
	constexpr double SwapAfter = 0.7;
	constexpr double ClearAfter = 2.0;
	constexpr float FadeIn = 0.8f;
	// A battered car pays less: up to this share off for a wreck.
	constexpr float WearCut = 0.7f;

	struct FKind
	{
		CarModel::EBody Body;
		const TCHAR* Hu;     // in the accusative: "Hozz nekem egy zöld ..."
		const TCHAR* HuName; // plain, for the note on screen
		const TCHAR* En;
		int32 Price;         // fillér, for a car in one piece
	};
	// Laci wants the common ones more often; now and then a big one.
	const FKind Kinds[] = {
		{ CarModel::EBody::Balilla, TEXT("Fiat Balill\u00e1t"), TEXT("Fiat Balilla"), TEXT("Fiat Balilla"), 300 },
		{ CarModel::EBody::Opel, TEXT("Opelt"), TEXT("Opel"), TEXT("Opel"), 350 },
		{ CarModel::EBody::Saloon, TEXT("csukott aut\u00f3t"), TEXT("csukott aut\u00f3"), TEXT("saloon"), 400 },
		{ CarModel::EBody::Tourer, TEXT("nyitott t\u00farakocsit"), TEXT("nyitott t\u00farakocsi"), TEXT("open tourer"), 400 },
		{ CarModel::EBody::Coupe, TEXT("kup\u00e9t"), TEXT("kup\u00e9"), TEXT("coup\u00e9"), 450 },
		{ CarModel::EBody::Van, TEXT("sz\u00e1ll\u00edt\u00f3kocsit"), TEXT("sz\u00e1ll\u00edt\u00f3kocsi"), TEXT("delivery van"), 300 },
		{ CarModel::EBody::Opel, TEXT("Opelt"), TEXT("Opel"), TEXT("Opel"), 350 },
		{ CarModel::EBody::Balilla, TEXT("Fiat Balill\u00e1t"), TEXT("Fiat Balilla"), TEXT("Fiat Balilla"), 300 },
		{ CarModel::EBody::Limousine, TEXT("nagy limuzint"), TEXT("nagy limuzin"), TEXT("big limousine"), 600 },
		{ CarModel::EBody::Steyr, TEXT("Steyrt"), TEXT("Steyr"), TEXT("Steyr"), 700 },
	};
	constexpr int32 NumKinds = static_cast<int32>(UE_ARRAY_COUNT(Kinds));

	struct FOrderPaint
	{
		const TCHAR* Hu;
		const TCHAR* En;
		FLinearColor Color;
	};
	const FOrderPaint OrderPaints[] = {
		{ TEXT("z\u00f6ld"), TEXT("green"), FLinearColor(0.05f, 0.13f, 0.07f) },
		{ TEXT("bord\u00f3"), TEXT("maroon"), FLinearColor(0.20f, 0.03f, 0.03f) },
		{ TEXT("fekete"), TEXT("black"), FLinearColor(0.02f, 0.02f, 0.022f) },
		{ TEXT("kr\u00e9msz\u00edn\u0171"), TEXT("cream"), FLinearColor(0.62f, 0.55f, 0.40f) },
		{ TEXT("s\u00f6t\u00e9tk\u00e9k"), TEXT("dark blue"), FLinearColor(0.03f, 0.05f, 0.13f) },
		{ TEXT("piros"), TEXT("red"), FLinearColor(0.35f, 0.04f, 0.03f) },
		{ TEXT("sz\u00fcrke"), TEXT("grey"), FLinearColor(0.25f, 0.26f, 0.27f) },
	};
	constexpr int32 NumOrderPaints = static_cast<int32>(UE_ARRAY_COUNT(OrderPaints));

	const FLinearColor BlackGuards(0.012f, 0.012f, 0.012f);
	const FLinearColor BlackRims(0.02f, 0.02f, 0.02f);
	const FLinearColor RedRims(0.30f, 0.03f, 0.02f);
	const FLinearColor BrassSpokes(0.55f, 0.48f, 0.33f);
	const FLinearColor VanPanel(0.70f, 0.62f, 0.42f);

	CarModel::EBody BodyOf(int32 Packed)
	{
		return static_cast<CarModel::EBody>(FMath::Clamp(Packed / 100, 0, static_cast<int32>(CarModel::EBody::Count) - 1));
	}

	const FOrderPaint& PaintOf(int32 Packed)
	{
		return OrderPaints[FMath::Clamp(Packed % 100, 0, NumOrderPaints - 1)];
	}

	const FKind& KindOf(int32 Packed)
	{
		const CarModel::EBody Wanted = BodyOf(Packed);
		for (const FKind& Kind : Kinds)
		{
			if (Kind.Body == Wanted)
			{
				return Kind;
			}
		}
		return Kinds[0];
	}
}

ALaciOrders::ALaciOrders()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	// The mark over the car, then over the pit; it lives in the world.
	Marker = CreateDefaultSubobject<UGoalMarkerComponent>(TEXT("Marker"));
	Marker->SetupAttachment(RootComponent);
	Guide = CreateDefaultSubobject<URouteGuideComponent>(TEXT("Guide"));
	Guide->SetupAttachment(RootComponent);
}

CarModel::FStyle ALaciOrders::OrderStyle(int32 Packed)
{
	CarModel::FStyle Look;
	Look.Body = LaciOrder::BodyOf(Packed);
	Look.PaintColor = LaciOrder::PaintOf(Packed).Color;
	Look.GuardColor = LaciOrder::BlackGuards;
	switch (Look.Body)
	{
	case CarModel::EBody::Steyr:
		Look.Wheel = CarModel::EWheel::Disc;
		Look.SpokeColor = Look.PaintColor;
		break;
	case CarModel::EBody::Balilla:
		Look.Wheel = CarModel::EWheel::Disc;
		Look.SpokeColor = LaciOrder::RedRims;
		break;
	case CarModel::EBody::Limousine:
		Look.Wheel = CarModel::EWheel::Wire;
		Look.SpokeColor = LaciOrder::BrassSpokes;
		break;
	default:
		Look.Wheel = CarModel::EWheel::Artillery;
		Look.SpokeColor = LaciOrder::BlackRims;
		break;
	}
	if (Look.Body == CarModel::EBody::Van)
	{
		Look.AccentColor = LaciOrder::VanPanel;
	}
	return Look;
}

LaciYardModel::FStrippedCar ALaciOrders::Leftovers(int32 Packed)
{
	const CarModel::FStyle Look = OrderStyle(Packed);
	LaciYardModel::FStrippedCar Car;
	Car.Paint = Look.PaintColor;
	Car.Rim = Look.SpokeColor;
	Car.bBig = Look.Body == CarModel::EBody::Steyr || Look.Body == CarModel::EBody::Limousine;
	Car.bVan = Look.Body == CarModel::EBody::Van;
	Car.Panel = Look.AccentColor;
	return Car;
}

APestiCar* ALaciOrders::GetOrderCar() const
{
	return State == ELaciOrder::None ? nullptr : OrderCar.Get();
}

ALaciYard* ALaciOrders::GetYard() const
{
	TActorIterator<ALaciYard> It(GetWorld());
	return It ? *It : nullptr;
}

bool ALaciOrders::IsMissionUnderway() const
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

void ALaciOrders::Note(const FString& Line, float Seconds) const
{
	APestiHUD::Show(this, LaciOrder::NoteKey, Seconds, FColor(255, 200, 60), Line);
}

void ALaciOrders::LaciSays(const FString& Hu, const FString& En) const
{
	if (APestiHUD* Hud = APestiHUD::Get(this))
	{
		Hud->Say(TEXT("Laci"), Hu, En, 7.f);
	}
}

void ALaciOrders::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (State == ELaciOrder::Handover)
	{
		TickHandover();
		return;
	}
	const APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this);
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	APawn* Player = PC ? PC->GetPawn() : nullptr;
	ALaciYard* Yard = GetYard();
	if (!Game || !Player || !Yard || !Game->IsMissionDone(LaciOrder::OpenedBy))
	{
		return;
	}

	// Talking to Laci, on foot: E for an order (or to hear it again).
	const APestiPerson* Person = Cast<APestiPerson>(Player);
	const AStoryPerson* Mechanic = Yard->GetLaci();
	const bool bByLaci = Person && Mechanic && !Person->IsRiding() && !Person->IsCarrying()
		&& FVector::Dist2D(Person->GetActorLocation(), Mechanic->GetActorLocation()) < LaciOrder::TalkRange;
	if (bByLaci && State != ELaciOrder::Deliver && !IsMissionUnderway())
	{
		if (APestiHUD* Hud = APestiHUD::Get(this))
		{
			if (State == ELaciOrder::None)
			{
				Hud->Prompt(TEXT("E"), TEXT("Laci megrendel\u00e9se"), TEXT("Laci's order"), false);
			}
			else
			{
				Hud->Prompt(TEXT("E"), TEXT("Mit is k\u00e9rt Laci?"), TEXT("What did Laci want?"), false);
			}
		}
		if (PC->WasInputKeyJustPressed(EKeys::E))
		{
			if (State == ELaciOrder::None)
			{
				StartOrder();
			}
			else
			{
				SayOrder();
			}
		}
	}

	if (State == ELaciOrder::None)
	{
		return;
	}
	if (!IsValid(OrderCar.Get()) || OrderCar->IsWrecked())
	{
		DropOrder(TEXT("A kocsi t\u00f6nkrement, Laci ilyet nem vesz \u00e1t.  (The car is wrecked; Laci won't take one like that.)"));
		return;
	}
	if (Player == OrderCar.Get())
	{
		TickDeliver(Player, DeltaSeconds);
	}
	else
	{
		TickSteal(Player);
	}
}

void ALaciOrders::StartOrder()
{
	ALaciYard* Yard = GetYard();
	const ATrafficManager* Traffic = nullptr;
	{
		TActorIterator<ATrafficManager> It(GetWorld());
		Traffic = It ? *It : nullptr;
	}
	if (!Yard || !Traffic || Traffic->NumLanes() == 0)
	{
		return;
	}

	// What he wants: a kind and a colour.
	const int32 KindIndex = FMath::RandRange(0, LaciOrder::NumKinds - 1);
	const int32 PaintIndex = FMath::RandRange(0, LaciOrder::NumOrderPaints - 1);
	const int32 Packed = static_cast<int32>(LaciOrder::Kinds[KindIndex].Body) * 100 + PaintIndex;

	// Where one stands: by the kerb of a named street, some way off.
	const FVector From = Yard->GetActorLocation();
	FCollisionQueryParams Params(SCENE_QUERY_STAT(LaciOrderSpot), false, this);
	const FCollisionShape CarBox = FCollisionShape::MakeBox(FVector(230.0, 100.0, 60.0));
	for (int32 Try = 0; Try < 200; ++Try)
	{
		const int32 LaneIndex = FMath::RandRange(0, Traffic->NumLanes() - 1);
		const FString Street = ATaxiCompany::PeriodStreetName(Traffic->FindStreetName(LaneIndex));
		const TArray<FVector>& Points = Traffic->GetLane(LaneIndex).Points;
		if (Street.IsEmpty() || Points.Num() < 2)
		{
			continue;
		}
		const int32 p = FMath::RandRange(1, Points.Num() - 1);
		const double Away = FVector::Dist2D(Points[p], From);
		if (Away < LaciOrder::MinAway || Away > LaciOrder::MaxAway)
		{
			continue;
		}
		// The kerb is on the left when keeping left; the car faces along the lane.
		const FVector Ahead = (Points[p] - Points[p - 1]).GetSafeNormal2D();
		const FVector Left(Ahead.Y, -Ahead.X, 0.0);
		const FVector Spot = Points[p] + Left * LaciOrder::KerbOffset;
		FHitResult Ground;
		if (!GetWorld()->LineTraceSingleByChannel(Ground, Spot + FVector(0.0, 0.0, 5000.0), Spot - FVector(0.0, 0.0, 5000.0), ECC_Visibility, Params))
		{
			continue;
		}
		const double CarYaw = FMath::RadiansToDegrees(FMath::Atan2(Ahead.Y, Ahead.X));
		const FQuat Facing(FRotator(0.0, CarYaw, 0.0));
		const FVector Middle = Ground.ImpactPoint + FVector(0.0, 0.0, LaciOrder::CarLift);
		if (GetWorld()->OverlapAnyTestByChannel(Middle, Facing, ECC_WorldStatic, CarBox, Params))
		{
			continue;
		}
		const FTransform Place(Facing, Middle);
		APestiCar* Car = GetWorld()->SpawnActorDeferred<APestiCar>(APestiCar::StaticClass(), Place, nullptr, nullptr,
			ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
		if (!Car)
		{
			return;
		}
		const CarModel::FStyle Look = OrderStyle(Packed);
		Car->BodyColor = Look.PaintColor;
		Car->SetStyle(Look);
		// Most owners lock up: smash a window (loud) or pick the lock (slow).
		Car->SetLocked(FMath::FRand() < LaciOrder::LockedChance);
		Car->FinishSpawning(Place);

		OrderCar = Car;
		OrderPacked = Packed;
		OrderStreet = Street;
		State = ELaciOrder::Steal;
		bTaken = false;
		StoodFor = 0.f;
		Marker->ShowAt(Ground.ImpactPoint);
		SayOrder();
		return;
	}
	LaciSays(TEXT("Most nincs semmi, fiam. N\u00e9zz be k\u00e9s\u0151bb!"), TEXT("Nothing just now, son. Look in later!"));
}

void ALaciOrders::SayOrder() const
{
	const LaciOrder::FKind& Kind = LaciOrder::KindOf(OrderPacked);
	const LaciOrder::FOrderPaint& Paint = LaciOrder::PaintOf(OrderPacked);
	LaciSays(FString::Printf(TEXT("Hozz nekem egy %s %s! L\u00e1ttam egyet: %s. Ha \u00e9pen hozod, j\u00f3l fizetek."), Paint.Hu, Kind.Hu, *OrderStreet),
		FString::Printf(TEXT("Bring me a %s %s! I saw one on %s. Bring it in one piece and I'll pay well."), Paint.En, Kind.En, *OrderStreet));
}

void ALaciOrders::TickSteal(APawn* Player)
{
	// Out of the car (or not in it yet): the way to it.
	const FVector At = Player->GetActorLocation();
	const FVector CarAt = OrderCar->GetActorLocation();
	Marker->ShowAt(CarAt);
	Guide->Guide(At, CarAt);
	const LaciOrder::FKind& Kind = LaciOrder::KindOf(OrderPacked);
	const LaciOrder::FOrderPaint& Paint = LaciOrder::PaintOf(OrderPacked);
	Note(FString::Printf(TEXT("Laci rendel\u00e9se: %s %s, %s  (Laci's order: the %s %s, %s)   %s %s"),
		Paint.Hu, Kind.HuName, *OrderStreet, Paint.En, Kind.En, *OrderStreet, *PestiText::Metres(FVector::Dist2D(At, CarAt)), PestiText::Compass(At, CarAt)), 0.f);
}

void ALaciOrders::TickDeliver(APawn* Player, float DeltaSeconds)
{
	APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this);
	ALaciYard* Yard = GetYard();
	if (!Game || !Yard)
	{
		return;
	}
	const double Now = GetWorld()->GetTimeSeconds();

	// Taking it: now and then somebody sees, and calls the police.
	if (!bTaken)
	{
		bTaken = true;
		if (FMath::RandRange(0, 99) < LaciOrder::SeenChance)
		{
			Game->ReportCrime(1);
			Note(TEXT("Valaki megl\u00e1tott!  (Somebody saw you!)"), 4.f);
		}
	}

	const FVector ParkAt = ALaciYard::ToCity(LaciYardModel::Spots::Park, Yard->GetActorLocation().Z);
	const FVector At = OrderCar->GetActorLocation();
	Marker->ShowAt(ParkAt);
	Guide->Guide(At, ParkAt);
	const int32 Wanted = Game->GetWantedLevel();
	if (Wanted > 0)
	{
		StoodFor = 0.f;
		if (FVector::Dist2D(At, ParkAt) < LaciOrder::HintNear && Now - LastHint > LaciOrder::HintEvery)
		{
			LastHint = Now;
			Note(TEXT("Laci nem nyit kaput, am\u00edg a rend\u0151r\u00f6k a nyomodban vannak  (Laci won't open up with the police on your tail)"), 5.f);
		}
		return;
	}
	Note(FString::Printf(TEXT("Vidd Laci m\u0171hely\u00e9be, \u00e1llj meg az akna f\u00f6l\u00f6tt  (Take it to Laci's yard, stop over the pit)   %s %s"),
		*PestiText::Metres(FVector::Dist2D(At, ParkAt)), PestiText::Compass(At, ParkAt)), 0.f);

	const FVector Local = Yard->GetActorTransform().InverseTransformPosition(At);
	const bool bOverPit = Local.X > LaciOrder::BayX0 && Local.X < LaciOrder::BayX1
		&& FMath::Abs(Local.Y - LaciYardModel::GateMid) < LaciOrder::BayHalf;
	StoodFor = bOverPit && OrderCar->GetVelocity().Size() < LaciOrder::StillSpeed ? StoodFor + DeltaSeconds : 0.f;
	if (StoodFor < LaciOrder::StillTime)
	{
		return;
	}

	// Handed over: the screen goes black while Laci takes it in.
	StoodFor = 0.f;
	State = ELaciOrder::Handover;
	OrderCar->ForwardSpeed = 0.f;
	OrderCar->SideSpeed = 0.f;
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		PC->SetIgnoreMoveInput(true);
		if (PC->PlayerCameraManager)
		{
			PC->PlayerCameraManager->StartCameraFade(0.f, 1.f, LaciOrder::FadeOut, FLinearColor::Black, false, true);
		}
	}
	SwapAt = Now + LaciOrder::SwapAfter;
	ClearAt = Now + LaciOrder::ClearAfter;
	Guide->Stop();
	Marker->Hide();
}

void ALaciOrders::TickHandover()
{
	const double Now = GetWorld()->GetTimeSeconds();
	APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this);

	// Behind the black: the player steps out, the car is gone, the money paid.
	if (SwapAt > 0.0 && Now >= SwapAt)
	{
		APestiCar* Car = OrderCar.Get();
		const bool bOut = !Car || GetWorld()->GetFirstPlayerController() == nullptr
			|| GetWorld()->GetFirstPlayerController()->GetPawn() != Car || Car->ForceDriverOut();
		if (bOut)
		{
			SwapAt = -1.0;
			const LaciOrder::FKind& Kind = LaciOrder::KindOf(OrderPacked);
			const float Wear = Car ? FMath::Clamp(Car->GetWear(), 0.f, 1.f) : 0.f;
			Pay = FMath::RoundToInt(Kind.Price * (1.f - LaciOrder::WearCut * Wear) / 10.f) * 10;
			if (Car)
			{
				Car->Destroy();
			}
			OrderCar = nullptr;
			if (Game)
			{
				Game->AddMoney(Pay);
				Game->AddLaciCar(OrderPacked);
			}
		}
	}
	if (ClearAt < 0.0 || Now < ClearAt)
	{
		return;
	}
	ClearAt = -1.0;
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		if (PC->PlayerCameraManager)
		{
			PC->PlayerCameraManager->StartCameraFade(1.f, 0.f, LaciOrder::FadeIn, FLinearColor::Black, false, false);
		}
		PC->SetIgnoreMoveInput(false);
	}
	if (SwapAt > 0.0)
	{
		// He could not get out (something in the way): the car stays, try again.
		SwapAt = -1.0;
		State = ELaciOrder::Deliver;
		return;
	}
	State = ELaciOrder::None;
	const FString Money = APestiAlvilagGameModeBase::FormatMoney(Pay);
	LaciSays(FString::Printf(TEXT("\u00dcgyes. Itt a %s. A t\u00f6bbi az \u00e9n dolgom."), *Money),
		FString::Printf(TEXT("Nice work. Here's %s. The rest is my business."), *Money));
}

void ALaciOrders::DropOrder(const FString& Line)
{
	State = ELaciOrder::None;
	OrderCar = nullptr;
	StoodFor = 0.f;
	Marker->Hide();
	Guide->Stop();
	Note(Line, 6.f);
}
