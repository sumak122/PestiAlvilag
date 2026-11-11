// Fill out your copyright notice in the Description page of Project Settings.

#include "NightRaces.h"

#include "CarModel.h"
#include "Components/SceneComponent.h"
#include "DayNight.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GoalMarkerComponent.h"
#include "LaciOrders.h"
#include "LaciYard.h"
#include "LaciYardModel.h"
#include "Mission.h"
#include "PestiAlvilagGameModeBase.h"
#include "PestiCar.h"
#include "PestiHud.h"
#include "PestiText.h"
#include "RaceModel.h"
#include "RaceRival.h"
#include "RouteGuideComponent.h"
#include "ShapeSet.h"
#include "StartRingComponent.h"
#include "StreetName.h"
#include "TrafficManager.h"

namespace NightRace
{
	// The mission that opens the races.
	const FName OpenedBy(TEXT("ElsoAuto"));
	// On-screen note keys: the race line, and the big count.
	constexpr int32 NoteKey = 73;
	constexpr int32 CountKey = 74;
	// Races from this hour of the evening till this hour of the morning.
	constexpr float NightFrom = 21.f;
	constexpr float NightTill = 4.5f;
	// The ring is this far out from the gate, in the street (yard frame, cm).
	const FVector RingOut(-500.0, LaciYardModel::GateMid, 0.0);
	// Stopped this close to the ring's middle (cm), this still (cm/s), this long (s), a race starts.
	constexpr double InRing = 450.0;
	constexpr float StillSpeed = 60.f;
	constexpr float StillTime = 0.6f;
	// Away from the ring this far, it starts races again (cm); the day-time hint this close, this often (s).
	constexpr double RearmAway = 1500.0;
	constexpr double HintNear = 1500.0;
	constexpr float HintEvery = 12.f;
	// The rivals line up this far back along the street (cm); the player stands this far along.
	constexpr double LineBack = 1800.0;
	const double RivalAlong[2] = { 1000.0, 300.0 };
	const double RivalSide[2] = { -70.0, 70.0 };
	// The count before the start (s), and how far the car may roll before it's a false start (cm).
	constexpr float CountFrom = 3.5f;
	constexpr double FalseStart = 600.0;
	// A gate stands this far past its corner, in the street the line turns into, and at least this far
	// before the next corner and before the finish (cm).
	constexpr double GatePastCorner = 1500.0;
	constexpr double GateBeforeCorner = 300.0;
	constexpr double GateBeforeFinish = 6000.0;
	// A gate's half width, from the walls either side (cm), and how near its middle counts as through.
	constexpr double SideLook = 1500.0;
	constexpr double MinHalf = 250.0;
	constexpr double MaxHalf = 600.0;
	constexpr double ThroughExtra = 250.0;
	// The race is off: out of a car this long (s), or this much further from the next gate than the gate before it is (cm).
	constexpr float MaxOutOfCar = 8.f;
	constexpr double MaxAway = 40000.0;
	// The Mercedes word comes this long after the last first win (s).
	constexpr float MercedesAfter = 8.f;

	/** A street corner on a race's way: where it is (a corner of the traffic map), and the name of the gate past it (none: the line only goes by it). */
	struct FCorner
	{
		double X;
		double Y;
		const TCHAR* Gate;
	};

	struct FRace
	{
		const TCHAR* Hu;
		const TCHAR* En;
		const TCHAR* LaciHu;
		const TCHAR* LaciEn;
		FCorner Corners[24];
		int32 NumCorners;
		float TopSpeed;
		int32 Prize;
	};
	// Each race is a round from the ring (heading off towards Mátyás tér) through these corners in turn,
	// on to Teleki tér and home along Szerdahelyi utca; no street twice, no dead ends.
	const FRace Races[] = {
		{ TEXT("kis k\u00F6r a Teleki t\u00E9rig"), TEXT("a little round to Teleki t\u00E9r"),
			TEXT("Kezdj\u00FCk k\u00F6nnyen: egy kis k\u00F6r a M\u00E1ty\u00E1s t\u00E9rt\u0151l a Teleki t\u00E9rig, \u00E9s vissza ide. A l\u00E1mp\u00E1s kapukon \u00E1t, ahogy a fi\u00FAk kiaggatt\u00E1k!"),
			TEXT("Let's start easy: a little round from M\u00E1ty\u00E1s t\u00E9r to Teleki t\u00E9r and back here. Through the lantern gates, the way the boys hung them!"),
			{ { 70785.0, 44383.0, TEXT("M\u00E1ty\u00E1s t\u00E9r") }, { 77562.0, 40380.0, TEXT("Homok utca") }, { 70457.0, 31070.0, nullptr },
				{ 74096.0, 27813.0, TEXT("N\u00E9psz\u00EDnh\u00E1z utca") }, { 90901.0, 33381.0, TEXT("Teleki t\u00E9r") }, { 97026.0, 43893.0, nullptr } },
			6, 1700.f, 300 },
		{ TEXT("Tisza K\u00E1lm\u00E1n t\u00E9r \u00E9s a Fiumei \u00FAt"), TEXT("Tisza K\u00E1lm\u00E1n t\u00E9r and Fiumei \u00FAt"),
			TEXT("Ma este a Tisza K\u00E1lm\u00E1n t\u00E9r, a Dologh\u00E1z utca, azt\u00E1n le a Fiumei \u00FAton, \u00E9s haza. Ezek a fi\u00FAk m\u00E1r nem kezd\u0151k."),
			TEXT("Tonight it's Tisza K\u00E1lm\u00E1n t\u00E9r, Dologh\u00E1z utca, then down Fiumei \u00FAt and home. These lads aren't beginners."),
			{ { 63159.0, 48058.0, TEXT("Nagy Fuvaros utca") }, { 58722.0, 39851.0, nullptr }, { 50960.0, 20191.0, TEXT("Tisza K\u00E1lm\u00E1n t\u00E9r") },
				{ 64313.0, 13377.0, nullptr }, { 77196.0, 8623.0, TEXT("Dologh\u00E1z utca") }, { 95175.0, 1810.0, TEXT("Fiumei \u00FAt") },
				{ 109519.0, 21237.0, TEXT("Teleki t\u00E9r") }, { 90901.0, 33381.0, nullptr }, { 97026.0, 43893.0, nullptr } },
			9, 1850.f, 400 },
		{ TEXT("R\u00E1k\u00F3czi \u00FAt \u00E9s a Keleti"), TEXT("R\u00E1k\u00F3czi \u00FAt and Keleti"),
			TEXT("Hossz\u00FA k\u00F6r: a Luther utc\u00E1n ki a R\u00E1k\u00F3czi \u00FAtra, k\u00F6rbe a Baross t\u00E9ren a Keleti el\u0151tt, le a Fiumei \u00FAton, \u00E9s haza. Vigy\u00E1zz a villamosokra!"),
			TEXT("A long one: up Luther utca onto R\u00E1k\u00F3czi \u00FAt, round Baross t\u00E9r past Keleti, down Fiumei \u00FAt and home. Mind the trams!"),
			{ { 63159.0, 48058.0, TEXT("Nagy Fuvaros utca") }, { 58722.0, 39851.0, nullptr }, { 50960.0, 20191.0, TEXT("Tisza K\u00E1lm\u00E1n t\u00E9r") },
				{ 42233.0, -7817.0, TEXT("Luther utca") }, { 36938.0, -21294.0, TEXT("R\u00E1k\u00F3czi \u00FAt") }, { 53859.0, -27763.0, nullptr },
				{ 67866.0, -35178.0, nullptr }, { 75099.0, -35311.0, TEXT("Keleti p\u00E1lyaudvar") }, { 81726.0, -21461.0, TEXT("Fiumei \u00FAt") },
				{ 89090.0, -6306.0, nullptr }, { 95175.0, 1810.0, TEXT("Dologh\u00E1z utca") }, { 109519.0, 21237.0, TEXT("Teleki t\u00E9r") },
				{ 90901.0, 33381.0, nullptr }, { 97026.0, 43893.0, nullptr } },
			14, 2000.f, 500 },
		{ TEXT("a nagy k\u00F6r J\u00F3zsefv\u00E1rosban"), TEXT("the big round of J\u00F3zsefv\u00E1ros"),
			TEXT("A nagy k\u00F6r: a Magdolna utca, a Fiumei \u00FAt, a Keleti, a R\u00E1k\u00F3czi \u00FAt a Blah\u00E1ig, a v\u00E1s\u00E1rcsarnok, \u00E9s v\u00E9gig a N\u00E9psz\u00EDnh\u00E1z utc\u00E1n. Ezt csak a legjobbak b\u00EDrj\u00E1k."),
			TEXT("The big round: Magdolna utca, Fiumei \u00FAt, Keleti, R\u00E1k\u00F3czi \u00FAt to Blaha, the market hall, and all the way down N\u00E9psz\u00EDnh\u00E1z utca. Only the best can do it."),
			{ { 96044.0, 69320.0, TEXT("Dank\u00F3 utca") }, { 108599.0, 61865.0, TEXT("Magdolna utca") }, { 115580.0, 57153.0, TEXT("Lujza utca") },
				{ 104588.0, 39459.0, nullptr }, { 111497.0, 35303.0, nullptr }, { 126059.0, 37605.0, TEXT("Fiumei \u00FAt") },
				{ 119961.0, 29322.0, nullptr }, { 95175.0, 1810.0, TEXT("Dologh\u00E1z utca") }, { 81726.0, -21461.0, nullptr },
				{ 75099.0, -35311.0, TEXT("Keleti p\u00E1lyaudvar") }, { 67866.0, -35178.0, nullptr }, { 62062.0, -30619.0, TEXT("R\u00E1k\u00F3czi \u00FAt") },
				{ 36938.0, -21294.0, nullptr }, { 292.0, -5356.0, TEXT("Blaha Lujza t\u00E9r") }, { 6321.0, 39386.0, TEXT("V\u00E1s\u00E1rcsarnok") },
				{ 15930.0, 37064.0, nullptr }, { 23670.0, 33900.0, TEXT("V\u00EDg utca") }, { 19882.0, 9733.0, TEXT("N\u00E9psz\u00EDnh\u00E1z utca") },
				{ 50960.0, 20191.0, TEXT("Tisza K\u00E1lm\u00E1n t\u00E9r") }, { 74096.0, 27813.0, nullptr }, { 90901.0, 33381.0, TEXT("Teleki t\u00E9r") },
				{ 97026.0, 43893.0, nullptr } },
			22, 2150.f, 800 },
	};
	constexpr int32 NumRaces = static_cast<int32>(UE_ARRAY_COUNT(Races));

	// The rivals' cars: a coupé and a tourer, in these paints.
	const FLinearColor RivalPaints[] = {
		FLinearColor(0.30f, 0.03f, 0.03f),
		FLinearColor(0.62f, 0.55f, 0.40f),
		FLinearColor(0.03f, 0.05f, 0.13f),
		FLinearColor(0.25f, 0.26f, 0.27f),
	};
	constexpr int32 NumRivalPaints = static_cast<int32>(UE_ARRAY_COUNT(RivalPaints));
}

ANightRaces::ANightRaces()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	StartRing = CreateDefaultSubobject<UStartRingComponent>(TEXT("StartRing"));
	StartRing->SetupAttachment(RootComponent);
	StartRing->SetOnMap(false);
	Marker = CreateDefaultSubobject<UGoalMarkerComponent>(TEXT("Marker"));
	Marker->SetupAttachment(RootComponent);
	Guide = CreateDefaultSubobject<URouteGuideComponent>(TEXT("Guide"));
	Guide->SetupAttachment(RootComponent);
}

void ANightRaces::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	for (ARaceRival* Rival : Rivals)
	{
		if (IsValid(Rival))
		{
			Rival->Destroy();
		}
	}
	Rivals.Reset();
	ClearGates();
	Super::EndPlay(EndPlayReason);
}

bool ANightRaces::IsMissionUnderway() const
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

bool ANightRaces::IsNight() const
{
	const ADayNight* Sky = ADayNight::Get(GetWorld());
	if (!Sky)
	{
		return false;
	}
	const float Hour = Sky->GetHour();
	return Hour >= NightRace::NightFrom || Hour < NightRace::NightTill;
}

ALaciYard* ANightRaces::GetYard() const
{
	TActorIterator<ALaciYard> It(GetWorld());
	return It ? *It : nullptr;
}

double ANightRaces::GroundAt(const FVector& At) const
{
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(NightRaceGround), false, this);
	return GetWorld()->LineTraceSingleByObjectType(Hit, FVector(At.X, At.Y, At.Z + 5000.0), FVector(At.X, At.Y, At.Z - 5000.0),
		FCollisionObjectQueryParams(ECC_WorldStatic), Params) ? Hit.ImpactPoint.Z : At.Z;
}

void ANightRaces::Note(const FString& Line, float Seconds) const
{
	APestiHUD::Show(this, NightRace::NoteKey, Seconds, FColor(255, 200, 60), Line);
}

void ANightRaces::LaciSays(const FString& Hu, const FString& En) const
{
	if (APestiHUD* Hud = APestiHUD::Get(this))
	{
		Hud->Say(TEXT("Laci"), Hu, En, 8.f);
	}
}

bool ANightRaces::FindStartSpot(FVector& OutAt, FVector& OutDir) const
{
	const ALaciYard* Yard = GetYard();
	const ATrafficManager* Traffic = nullptr;
	{
		TActorIterator<ATrafficManager> It(GetWorld());
		Traffic = It ? *It : nullptr;
	}
	if (!Yard || !Traffic)
	{
		return false;
	}
	const FVector Out = ALaciYard::ToCity(NightRace::RingOut, Yard->GetActorLocation().Z);
	// The street runs along the yard's front (its Y axis).
	const FVector Along = FRotator(0.0, LaciYardModel::CityYaw + 90.0, 0.0).Vector();
	int32 Lane = INDEX_NONE;
	int32 Point = INDEX_NONE;
	if (!Traffic->FindLanePointNear(Out, Along, Lane, Point))
	{
		return false;
	}
	const TArray<FVector>& Points = Traffic->GetLane(Lane).Points;
	if (!Points.IsValidIndex(Point))
	{
		return false;
	}
	// Right in front of the gate: the nearest spot on that bit of the lane.
	const FVector LaneEnd = Points[Point];
	const FVector LaneStart = Points.IsValidIndex(Point - 1) ? Points[Point - 1] : LaneEnd;
	const FVector Span = LaneEnd - LaneStart;
	const double SpanSquared = Span.SizeSquared2D();
	const double Share = SpanSquared > 1.0 ? FMath::Clamp(((Out.X - LaneStart.X) * Span.X + (Out.Y - LaneStart.Y) * Span.Y) / SpanSquared, 0.0, 1.0) : 1.0;
	OutAt = LaneStart + Span * Share;
	OutDir = Span.GetSafeNormal2D();
	if (OutDir.IsNearlyZero())
	{
		OutDir = Along;
	}
	OutAt.Z = GroundAt(OutAt);
	return true;
}

int32 ANightRaces::PickRace() const
{
	const APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this);
	for (int32 i = 0; i < NightRace::NumRaces; ++i)
	{
		if (!Game || !Game->HasWonRace(i))
		{
			return i;
		}
	}
	// All won: each in turn, from the one after the last run.
	return (Race + 1) % NightRace::NumRaces;
}

void ANightRaces::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this);
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	APawn* Player = PC ? PC->GetPawn() : nullptr;
	if (!Game || !Player || !Game->IsMissionDone(NightRace::OpenedBy))
	{
		return;
	}

	if (MercedesLineIn > 0.f)
	{
		MercedesLineIn -= DeltaSeconds;
		if (MercedesLineIn <= 0.f)
		{
			MercedesLineIn = 0.f;
			LaciSays(TEXT("Figyelj ide. A Tattersallban, a Kerepesi \u00FAton aut\u00F3ki\u00E1ll\u00EDt\u00E1s lesz, \u00E9s ott \u00E1ll egy fekete Mercedes SSK. Olyan kocsi kell neked. Majd megbesz\u00E9lj\u00FCk..."),
				TEXT("Listen. There's a motor show at the Tattersall on Kerepesi \u00FAt, and there's a black Mercedes SSK standing in it. That's the car for you. We'll talk..."));
		}
	}

	const bool bMission = IsMissionUnderway();
	if (State != ENightRace::None && bMission)
	{
		StopRace(TEXT("A verseny elmaradt.  (The race is off.)"));
		return;
	}
	if (State == ENightRace::Countdown)
	{
		TickCountdown(Cast<APestiCar>(Player), DeltaSeconds);
		return;
	}
	if (State == ENightRace::Racing)
	{
		TickRacing(Player, DeltaSeconds);
		return;
	}

	// Waiting: the ring in the street, after dark.
	if (StartAt.IsZero() && !FindStartSpot(StartAt, StartDir))
	{
		return;
	}
	const FVector At = Player->GetActorLocation();
	const double FromRing = FVector::Dist2D(At, StartAt);
	if (FromRing > NightRace::RearmAway)
	{
		bRingArmed = true;
	}
	const bool bNight = IsNight();
	if (!bNight || bMission)
	{
		if (StartRing->IsShown())
		{
			StartRing->Hide();
		}
		NightHintIn -= DeltaSeconds;
		if (!bNight && !bMission && FromRing < NightRace::HintNear && NightHintIn <= 0.f)
		{
			NightHintIn = NightRace::HintEvery;
			Note(TEXT("Laci versenyei \u00E9jjel vannak, este 9 ut\u00E1n.  (Laci's races are at night, after 9 pm.)"), 5.f);
		}
		return;
	}
	if (!StartRing->IsShown())
	{
		StartRing->ShowAt(StartAt, TEXT("V"));
	}
	if (FromRing > 3000.0)
	{
		StoodFor = 0.f;
		return;
	}

	// Close by: what tonight's race is, and stopping a car in the ring starts it.
	const NightRace::FRace& Next = NightRace::Races[PickRace()];
	APestiCar* Car = Cast<APestiCar>(Player);
	const APestiCar* OrderCar = nullptr;
	for (TActorIterator<ALaciOrders> It(GetWorld()); It; ++It)
	{
		OrderCar = It->GetOrderCar();
	}
	if (!Car || Car == OrderCar)
	{
		Note(FString::Printf(TEXT("Laci \u00E9jszakai versenye: %s. Gyere aut\u00F3val, \u00E9s \u00E1llj meg a k\u00F6rben.  (Laci's night race: %s. Come in a car and stop in the ring.)"), Next.Hu, Next.En), 0.5f);
		StoodFor = 0.f;
		return;
	}
	if (Game->GetWantedLevel() > 0)
	{
		Note(TEXT("Rend\u0151r\u00F6kkel a nyomodban nincs verseny.  (No racing with the police on your tail.)"), 0.5f);
		StoodFor = 0.f;
		return;
	}
	Note(FString::Printf(TEXT("Laci \u00E9jszakai versenye: %s. \u00C1llj meg a k\u00F6rben.  (Laci's night race: %s. Stop in the ring.)"), Next.Hu, Next.En), 0.5f);
	const bool bStill = FMath::Abs(Car->ForwardSpeed) < NightRace::StillSpeed;
	StoodFor = bRingArmed && bStill && !Car->IsWrecked() && FromRing < NightRace::InRing ? StoodFor + DeltaSeconds : 0.f;
	if (StoodFor >= NightRace::StillTime)
	{
		StoodFor = 0.f;
		StartRace(Car);
	}
}

bool ANightRaces::StartRace(APestiCar* Car)
{
	const ATrafficManager* Traffic = nullptr;
	{
		TActorIterator<ATrafficManager> It(GetWorld());
		Traffic = It ? *It : nullptr;
	}
	if (!Traffic || !FindStartSpot(StartAt, StartDir))
	{
		return false;
	}
	Race = PickRace();
	const NightRace::FRace& Def = NightRace::Races[Race];

	// The line: from behind the start along the street, through each corner in turn, and back to the ring.
	RaceLine.Reset();
	RaceLine.Add(StartAt - StartDir * NightRace::LineBack);
	RaceLine.Add(StartAt);
	TArray<int32> CornerEnds;
	auto AddPoint = [this](const FVector& P)
	{
		if (FVector::Dist2D(RaceLine.Last(), P) > 50.0)
		{
			RaceLine.Add(P);
		}
	};
	auto FlatDot = [](const FVector& A, const FVector& B)
	{
		return A.X * B.X + A.Y * B.Y;
	};
	FVector From = StartAt;
	for (int32 c = 0; c <= Def.NumCorners; ++c)
	{
		const bool bHome = c == Def.NumCorners;
		const FVector To = bHome ? StartAt : FVector(Def.Corners[c].X, Def.Corners[c].Y, StartAt.Z);
		TArray<FVector> Leg;
		if (!Traffic->FindPath(From, To, Leg) || Leg.Num() == 0)
		{
			Note(TEXT("Laci fiai ma nem tal\u00E1lnak utat.  (Laci's lads can't find a way tonight.)"), 4.f);
			return false;
		}
		if (c == 0)
		{
			// Off the ring: never back to a street corner behind it.
			while (Leg.Num() > 1 && FlatDot(Leg[0] - StartAt, StartDir) < 100.0)
			{
				Leg.RemoveAt(0);
			}
		}
		if (bHome)
		{
			// Home: stop at the ring, not at the street corner past it.
			while (Leg.Num() > 1 && FlatDot(Leg.Last() - StartAt, StartAt - Leg[Leg.Num() - 2]) > 0.0)
			{
				Leg.Pop();
			}
		}
		for (const FVector& P : Leg)
		{
			AddPoint(P);
		}
		if (bHome)
		{
			AddPoint(StartAt);
		}
		CornerEnds.Add(RaceLine.Num() - 1);
		From = Leg.Last();
	}
	RaceAlong.SetNum(RaceLine.Num());
	RaceAlong[0] = 0.0;
	for (int32 i = 1; i < RaceLine.Num(); ++i)
	{
		RaceAlong[i] = RaceAlong[i - 1] + FVector::Dist2D(RaceLine[i - 1], RaceLine[i]);
	}
	const double Total = RaceAlong.Last();

	// The gates: one just past each named corner, across the street the line turns into, then the finish at the ring.
	GateAt.Reset();
	GateDir.Reset();
	GateHalf.Reset();
	GateAlong.Reset();
	GateName.Reset();
	for (int32 c = 0; c < Def.NumCorners; ++c)
	{
		if (!Def.Corners[c].Gate)
		{
			continue;
		}
		const double Corner = RaceAlong[CornerEnds[c]];
		const double Ahead = RaceAlong[CornerEnds[c + 1]];
		const double S = FMath::Max(Corner, FMath::Min3(Corner + NightRace::GatePastCorner, Ahead - NightRace::GateBeforeCorner, Total - NightRace::GateBeforeFinish));
		if (S <= NightRace::LineBack || (GateAlong.Num() > 0 && S <= GateAlong.Last() + 100.0))
		{
			continue;
		}
		GateAlong.Add(S);
		GateName.Add(Def.Corners[c].Gate);
	}
	GateAlong.Add(Total);
	GateName.Add(TEXT("Laci kapuja"));
	FCollisionQueryParams Params(SCENE_QUERY_STAT(NightRaceGate), false, this);
	Params.AddIgnoredActor(Car);
	for (const double S : GateAlong)
	{
		FVector Dir = FVector::ForwardVector;
		FVector P = ARaceRival::PointAt(RaceLine, RaceAlong, FMath::Min(S, Total - 1.0), &Dir);
		P.Z = GroundAt(P);
		// As wide as the street: to the walls either side, the gate in the middle.
		const FVector Right(-Dir.Y, Dir.X, 0.0);
		const FVector Eye = P + FVector(0.0, 0.0, 150.0);
		double Spans[2] = { NightRace::SideLook, NightRace::SideLook };
		for (int32 s = 0; s < 2; ++s)
		{
			FHitResult Hit;
			const FVector Reach = Eye + Right * (s == 0 ? -NightRace::SideLook : NightRace::SideLook);
			if (GetWorld()->LineTraceSingleByObjectType(Hit, Eye, Reach, FCollisionObjectQueryParams(ECC_WorldStatic), Params))
			{
				Spans[s] = FVector::Dist2D(Eye, Hit.ImpactPoint);
			}
		}
		const double Half = FMath::Clamp((Spans[0] + Spans[1]) * 0.5 - 80.0, NightRace::MinHalf, NightRace::MaxHalf);
		const double Shift = FMath::Clamp((Spans[1] - Spans[0]) * 0.5, -NightRace::MaxHalf, NightRace::MaxHalf);
		GateAt.Add(P + Right * Shift);
		GateDir.Add(Dir);
		GateHalf.Add(Half);
	}

	// The rivals, lined up behind the player.
	for (ARaceRival* Rival : Rivals)
	{
		if (IsValid(Rival))
		{
			Rival->Destroy();
		}
	}
	Rivals.Reset();
	FRandomStream Random(FMath::Rand());
	const CarModel::EBody Bodies[2] = { CarModel::EBody::Coupe, CarModel::EBody::Tourer };
	for (int32 r = 0; r < 2; ++r)
	{
		CarModel::FStyle Look = CarModel::RandomStyle(Random, NightRace::RivalPaints[Random.RandRange(0, NightRace::NumRivalPaints - 1)]);
		Look.Body = Race == NightRace::NumRaces - 1 && r == 0 ? CarModel::EBody::Steyr : Bodies[r];
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		const FVector SpawnAt = ARaceRival::PointAt(RaceLine, RaceAlong, NightRace::RivalAlong[r]) + FVector(0.0, 0.0, 200.0);
		ARaceRival* Rival = GetWorld()->SpawnActor<ARaceRival>(ARaceRival::StaticClass(), SpawnAt, FRotator::ZeroRotator, SpawnParams);
		if (Rival)
		{
			Rival->Setup(RaceLine, RaceAlong, Def.TopSpeed * (r == 0 ? 1.f : 0.96f), NightRace::RivalSide[r], Look, NightRace::RivalAlong[r]);
			Rivals.Add(Rival);
		}
	}

	// The player's car on the start line, facing the way the race goes.
	const FVector CarAt = Car->GetActorLocation();
	Car->SetActorLocationAndRotation(FVector(StartAt.X, StartAt.Y, CarAt.Z), FRotator(0.0, StartDir.Rotation().Yaw, 0.0),
		false, nullptr, ETeleportType::TeleportPhysics);

	StartRing->Hide();
	NextGate = 0;
	ShowGates(0);
	Clock = NightRace::CountFrom;
	OutOfCar = 0.f;
	State = ENightRace::Countdown;
	LaciSays(Def.LaciHu, Def.LaciEn);
	Note(FString::Printf(TEXT("Laci \u00E9jszakai versenye: %s  (Laci's night race: %s)"), Def.Hu, Def.En), 0.f);
	return true;
}

void ANightRaces::TickCountdown(APestiCar* Car, float DeltaSeconds)
{
	if (!Car || FVector::Dist2D(Car->GetActorLocation(), StartAt) > NightRace::FalseStart)
	{
		StopRace(TEXT("Korai rajt! A fi\u00FAk hazamentek.  (False start! The lads went home.)"));
		return;
	}
	const int32 Before = FMath::CeilToInt(Clock);
	Clock -= DeltaSeconds;
	const int32 Now = FMath::CeilToInt(Clock);
	if (Clock <= 0.f)
	{
		APestiHUD::Show(this, NightRace::CountKey, 1.5f, FColor(255, 230, 120), TEXT("RAJT!  (GO!)"));
		for (ARaceRival* Rival : Rivals)
		{
			if (IsValid(Rival))
			{
				Rival->Go();
			}
		}
		Clock = 0.f;
		State = ENightRace::Racing;
		return;
	}
	if (Now != Before && Now <= 3)
	{
		APestiHUD::Show(this, NightRace::CountKey, 1.f, FColor(255, 230, 120), FString::FromInt(Now));
	}
}

double ANightRaces::PlayerAlong(const FVector& At) const
{
	if (!GateAlong.IsValidIndex(NextGate))
	{
		return RaceAlong.Num() > 0 ? RaceAlong.Last() : 0.0;
	}
	const double Prev = NextGate > 0 ? GateAlong[NextGate - 1] : NightRace::LineBack;
	return FMath::Max(Prev, GateAlong[NextGate] - FVector::Dist2D(At, GateAt[NextGate]));
}

void ANightRaces::TickRacing(APawn* Player, float DeltaSeconds)
{
	Clock += DeltaSeconds;
	APestiCar* Car = Cast<APestiCar>(Player);
	const FVector At = Player->GetActorLocation();
	if (!Car)
	{
		OutOfCar += DeltaSeconds;
		if (OutOfCar > NightRace::MaxOutOfCar)
		{
			StopRace(TEXT("Kisz\u00E1llt\u00E1l, a verseny neked v\u00E9get \u00E9rt.  (You got out; the race is over for you.)"));
			return;
		}
		Note(TEXT("\u00DClj vissza egy aut\u00F3ba!  (Get back in a car!)"), 0.f);
		return;
	}
	OutOfCar = 0.f;
	if (Car->IsWrecked())
	{
		StopRace(TEXT("Az aut\u00F3 t\u00F6nkrement, kiest\u00E9l a versenyb\u0151l.  (The car is wrecked; you're out of the race.)"));
		return;
	}

	// Through the next gate?
	const double Through = FMath::Max(GateHalf[NextGate], 300.0) + NightRace::ThroughExtra;
	if (FVector::Dist2D(At, GateAt[NextGate]) < Through)
	{
		++NextGate;
		if (NextGate >= GateAt.Num())
		{
			int32 Home = 0;
			for (const ARaceRival* Rival : Rivals)
			{
				Home += IsValid(Rival) && Rival->HasFinished() ? 1 : 0;
			}
			Finish(Home + 1);
			return;
		}
		ShowGates(NextGate);
	}
	const FVector Target = GateAt[NextGate];
	const FVector Before = NextGate > 0 ? GateAt[NextGate - 1] : StartAt;
	if (FVector::Dist2D(At, Target) > FVector::Dist2D(Before, Target) + NightRace::MaxAway)
	{
		StopRace(TEXT("T\u00FAl messzire ker\u00FClt\u00E9l a p\u00E1ly\u00E1t\u00F3l.  (You strayed too far from the course.)"));
		return;
	}

	// The others: where the player is, and the place he holds.
	const double Mine = PlayerAlong(At);
	int32 Place = 1;
	for (ARaceRival* Rival : Rivals)
	{
		if (IsValid(Rival))
		{
			Rival->SetPlayerAlong(Mine);
			Place += Rival->HasFinished() || Rival->GetAlong() > Mine ? 1 : 0;
		}
	}

	Marker->ShowAt(Target);
	Guide->Guide(At, Target);
	const bool bFinish = NextGate == GateAt.Num() - 1;
	const int32 Tenths = FMath::FloorToInt(Clock * 10.f);
	const FString Time = FString::Printf(TEXT("%d:%02d.%d"), Tenths / 600, (Tenths / 10) % 60, Tenths % 10);
	if (bFinish)
	{
		Note(FString::Printf(TEXT("Vissza Laci kapuj\u00E1hoz, a c\u00E9lba!  (Back to Laci's gate, the finish!)   %d. hely / %d   %s   %s %s"),
			Place, Rivals.Num() + 1, *Time, *PestiText::Metres(FVector::Dist2D(At, Target)), PestiText::Compass(At, Target)), 0.f);
	}
	else
	{
		Note(FString::Printf(TEXT("Kapu %d / %d: %s  (Gate)   %d. hely / %d  (place)   %s   %s %s"),
			NextGate + 1, GateAt.Num() - 1, *GateName[NextGate], Place, Rivals.Num() + 1, *Time, *PestiText::Metres(FVector::Dist2D(At, Target)), PestiText::Compass(At, Target)), 0.f);
	}
}

void ANightRaces::Finish(int32 Place)
{
	const NightRace::FRace& Def = NightRace::Races[Race];
	const int32 Tenths = FMath::FloorToInt(Clock * 10.f);
	const FString Time = FString::Printf(TEXT("%d:%02d.%d"), Tenths / 600, (Tenths / 10) % 60, Tenths % 10);
	APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this);
	StopRace(FString::Printf(TEXT("C\u00E9lban: %d. hely, %s  (Finished: place %d, %s)"), Place, *Time, Place, *Time));
	bRingArmed = false;
	if (!Game)
	{
		return;
	}
	if (Place > 1)
	{
		LaciSays(TEXT("Ez most nem j\u00F6tt \u00F6ssze, \u00F6csi. Gyere vissza egy m\u00E1sik este, \u00E9s mutasd meg nekik!"),
			TEXT("Not this time, kid. Come back another night and show them!"));
		return;
	}
	const bool bFirstWin = !Game->HasWonRace(Race);
	const int32 Prize = bFirstWin ? Def.Prize : Def.Prize / 2;
	Game->AddMoney(Prize);
	const FString Money = APestiAlvilagGameModeBase::FormatMoney(Prize);
	if (!bFirstWin)
	{
		LaciSays(FString::Printf(TEXT("Megint te lett\u00E9l az els\u0151! Itt a %s, a fi\u00FAk adt\u00E1k \u00F6ssze."), *Money),
			FString::Printf(TEXT("First again! Here's %s, the lads chipped in."), *Money));
		return;
	}
	Game->MarkRaceWon(Race);
	Game->AddRenown(StreetName::ForRaceWin, TEXT("\u00E9jszakai verseny megnyer\u00E9se"), TEXT("winning a night race"));
	bool bAll = true;
	for (int32 i = 0; i < NightRace::NumRaces; ++i)
	{
		bAll = bAll && Game->HasWonRace(i);
	}
	if (bAll)
	{
		LaciSays(FString::Printf(TEXT("Mind a n\u00E9gyet megnyerted! Itt a %s. Nincs ma Pesten jobb sof\u0151r n\u00E1lad."), *Money),
			FString::Printf(TEXT("You've won all four! Here's %s. There's no better driver in Pest tonight."), *Money));
		MercedesLineIn = NightRace::MercedesAfter;
		return;
	}
	LaciSays(FString::Printf(TEXT("Els\u0151 lett\u00E9l! Itt a %s. Holnap este j\u00F6het a k\u00F6vetkez\u0151."), *Money),
		FString::Printf(TEXT("You came first! Here's %s. The next one tomorrow night."), *Money));
}

void ANightRaces::ShowGates(int32 First)
{
	// Take down the ones no longer wanted.
	for (int32 i = GateSets.Num() - 1; i >= 0; --i)
	{
		if (GateShown[i] < First || GateShown[i] > First + 1)
		{
			if (IsValid(GateSets[i].Get()))
			{
				GateSets[i]->Destroy();
			}
			GateSets.RemoveAt(i);
			GateShown.RemoveAt(i);
		}
	}
	// Put up this one and the next.
	for (int32 g = First; g <= First + 1 && g < GateAt.Num(); ++g)
	{
		if (GateShown.Contains(g))
		{
			continue;
		}
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AShapeSet* Set = GetWorld()->SpawnActor<AShapeSet>(AShapeSet::StaticClass(), GateAt[g], FRotator(0.0, GateDir[g].Rotation().Yaw, 0.0), SpawnParams);
		if (!Set)
		{
			continue;
		}
		TArray<MarketModel::FShape> Shapes;
		TArray<BozsiHouseModel::FLamp> Lamps;
		TArray<BozsiHouseModel::FSign> Lettering;
		const bool bFinish = g == GateAt.Num() - 1;
		if (bFinish)
		{
			RaceModel::Finish(Shapes, Lamps, GateHalf[g]);
		}
		else
		{
			RaceModel::Checkpoint(Shapes, Lamps, GateHalf[g]);
		}
		RaceModel::Signs(Lettering, GateHalf[g], bFinish ? 0 : g + 1);
		Set->AddShapes(Shapes);
		for (const BozsiHouseModel::FSign& Sign : Lettering)
		{
			Set->AddSign(Sign.Text, Sign.At, Sign.Yaw, Sign.Height, Sign.Color);
		}
		for (const BozsiHouseModel::FLamp& Lamp : Lamps)
		{
			Set->AddLamp(Lamp.At, Lamp.Radius);
		}
		GateSets.Add(Set);
		GateShown.Add(g);
	}
}

void ANightRaces::ClearGates()
{
	for (AShapeSet* Set : GateSets)
	{
		if (IsValid(Set))
		{
			Set->Destroy();
		}
	}
	GateSets.Reset();
	GateShown.Reset();
}

void ANightRaces::StopRace(const FString& Why)
{
	for (ARaceRival* Rival : Rivals)
	{
		if (IsValid(Rival))
		{
			Rival->Destroy();
		}
	}
	Rivals.Reset();
	ClearGates();
	State = ENightRace::None;
	StoodFor = 0.f;
	Marker->Hide();
	Guide->Stop();
	APestiHUD::Show(this, NightRace::CountKey, 0.1f, FColor(255, 230, 120), FString());
	Note(Why, 5.f);
}
