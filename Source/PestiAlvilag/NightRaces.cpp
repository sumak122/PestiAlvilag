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
	// Gates: about this far apart along the line, never closer, none this near the start or the finish (cm).
	constexpr double GateEvery = 25000.0;
	constexpr double GateGap = 8000.0;
	constexpr double GateFromStart = 4000.0;
	constexpr double GateBeforeFinish = 6000.0;
	// A gate's half width, from the walls either side (cm), and how near its middle counts as through.
	constexpr double SideLook = 1500.0;
	constexpr double MinHalf = 250.0;
	constexpr double MaxHalf = 600.0;
	constexpr double ThroughExtra = 250.0;
	// The race is off: out of a car this long (s), or this far from the next gate (cm).
	constexpr float MaxOutOfCar = 8.f;
	constexpr double MaxAway = 60000.0;
	// The Mercedes word comes this long after the last first win (s).
	constexpr float MercedesAfter = 8.f;

	struct FRace
	{
		const TCHAR* Hu;
		const TCHAR* En;
		const TCHAR* LaciHu;
		const TCHAR* LaciEn;
		FVector2D Ways[5];
		int32 NumWays;
		float TopSpeed;
		int32 Prize;
	};
	const FRace Races[] = {
		{ TEXT("Teleki t\u00E9r \u00E9s az Oppenheim k\u00E1v\u00E9h\u00E1z"), TEXT("Teleki t\u00E9r and Caf\u00E9 Oppenheim"),
			TEXT("Kezdj\u00FCk k\u00F6nnyen: a Teleki t\u00E9rig, onnan az Oppenheim el\u00E9, \u00E9s vissza ide. A kapukon \u00E1t, ahogy a fi\u00FAk kiaggatt\u00E1k!"),
			TEXT("Let's start easy: to Teleki t\u00E9r, then past Caf\u00E9 Oppenheim and back here. Through the gates, the way the boys hung them!"),
			{ FVector2D(101337.0, 30370.0), FVector2D(74990.0, 27101.0) }, 2, 1700.f, 300 },
		{ TEXT("a Keleti \u00E9s a Teleki t\u00E9r"), TEXT("Keleti and Teleki t\u00E9r"),
			TEXT("Ma este a Keletiig mentek, azt\u00E1n a Teleki t\u00E9rre, \u00E9s vissza. Ezek a fi\u00FAk m\u00E1r nem kezd\u0151k."),
			TEXT("Tonight it's down to Keleti, then Teleki t\u00E9r, and back. These lads aren't beginners."),
			{ FVector2D(101266.0, -37678.0), FVector2D(101337.0, 30370.0) }, 2, 1850.f, 400 },
		{ TEXT("N\u00E9psz\u00EDnh\u00E1z utca, R\u00E1k\u00F3czi \u00FAt, Keleti"), TEXT("N\u00E9psz\u00EDnh\u00E1z utca, R\u00E1k\u00F3czi \u00FAt, Keleti"),
			TEXT("Hossz\u00FA k\u00F6r: B\u00F6zsi n\u00E9ni h\u00E1za, a R\u00E1k\u00F3czi \u00FAt, a Keleti, \u00E9s haza. Vigy\u00E1zz a villamosokra!"),
			TEXT("A long one: B\u00F6zsi n\u00E9ni's house, R\u00E1k\u00F3czi \u00FAt, Keleti, and home. Mind the trams!"),
			{ FVector2D(58771.0, 21395.0), FVector2D(41134.0, -21532.0), FVector2D(101266.0, -37678.0) }, 3, 2000.f, 500 },
		{ TEXT("a nagy k\u00F6r J\u00F3zsefv\u00E1rosban"), TEXT("the big round of J\u00F3zsefv\u00E1ros"),
			TEXT("A nagy k\u00F6r: a N\u00E9psz\u00EDnh\u00E1z utca eleje, a v\u00E1s\u00E1rcsarnok, a R\u00E1k\u00F3czi \u00FAt, a Keleti, a Teleki t\u00E9r. Ezt csak a legjobbak b\u00EDrj\u00E1k."),
			TEXT("The big round: the top of N\u00E9psz\u00EDnh\u00E1z utca, the market hall, R\u00E1k\u00F3czi \u00FAt, Keleti, Teleki t\u00E9r. Only the best can do it."),
			{ FVector2D(21760.0, 9062.0), FVector2D(18109.0, 40941.0), FVector2D(41134.0, -21532.0), FVector2D(101266.0, -37678.0), FVector2D(101337.0, 30370.0) },
			5, 2150.f, 800 },
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
	OutAt = Points[Point];
	const FVector Ahead = Points.IsValidIndex(Point + 1) ? Points[Point + 1] - Points[Point]
		: Points.IsValidIndex(Point - 1) ? Points[Point] - Points[Point - 1] : Along;
	OutDir = Ahead.GetSafeNormal2D();
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

	// The line: from behind the start along the street, through each place in turn, and back to the ring.
	RaceLine.Reset();
	RaceLine.Add(StartAt - StartDir * NightRace::LineBack);
	RaceLine.Add(StartAt);
	TArray<int32> WayEnds;
	auto AddPoint = [this](const FVector& P)
	{
		if (FVector::Dist2D(RaceLine.Last(), P) > 50.0)
		{
			RaceLine.Add(P);
		}
	};
	FVector From = StartAt;
	for (int32 w = 0; w <= Def.NumWays; ++w)
	{
		const FVector To = w < Def.NumWays ? FVector(Def.Ways[w], StartAt.Z) : StartAt;
		TArray<FVector> Leg;
		if (!Traffic->FindPath(From, To, Leg) || Leg.Num() == 0)
		{
			Note(TEXT("Laci fiai ma nem tal\u00E1lnak utat.  (Laci's lads can't find a way tonight.)"), 4.f);
			return false;
		}
		for (const FVector& P : Leg)
		{
			AddPoint(P);
		}
		if (w == Def.NumWays)
		{
			AddPoint(StartAt);
		}
		WayEnds.Add(RaceLine.Num() - 1);
		From = Leg.Last();
	}
	RaceAlong.SetNum(RaceLine.Num());
	RaceAlong[0] = 0.0;
	for (int32 i = 1; i < RaceLine.Num(); ++i)
	{
		RaceAlong[i] = RaceAlong[i - 1] + FVector::Dist2D(RaceLine[i - 1], RaceLine[i]);
	}
	const double Total = RaceAlong.Last();
	const double StartAlong = NightRace::LineBack;

	// The gates: one at each place on the way, and others in between, then the finish at the ring.
	TArray<double> Wanted;
	for (int32 w = 0; w + 1 < WayEnds.Num(); ++w)
	{
		Wanted.Add(RaceAlong[WayEnds[w]]);
	}
	for (double S = StartAlong + NightRace::GateEvery; S < Total; S += NightRace::GateEvery)
	{
		Wanted.Add(S);
	}
	Wanted.Sort();
	GateAt.Reset();
	GateDir.Reset();
	GateHalf.Reset();
	GateAlong.Reset();
	double Last = StartAlong;
	for (const double S : Wanted)
	{
		if (S - Last < NightRace::GateGap || S < StartAlong + NightRace::GateFromStart || S > Total - NightRace::GateBeforeFinish)
		{
			continue;
		}
		GateAlong.Add(S);
		Last = S;
	}
	GateAlong.Add(Total);
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
	if (FVector::Dist2D(At, Target) > NightRace::MaxAway)
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
		Note(FString::Printf(TEXT("Kapu %d / %d  (Gate)   %d. hely / %d  (place)   %s   %s %s"),
			NextGate + 1, GateAt.Num() - 1, Place, Rivals.Num() + 1, *Time, *PestiText::Metres(FVector::Dist2D(At, Target)), PestiText::Compass(At, Target)), 0.f);
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
