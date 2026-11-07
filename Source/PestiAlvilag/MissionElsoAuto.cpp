// Fill out your copyright notice in the Description page of Project Settings.

#include "MissionElsoAuto.h"

#include "CarModel.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "LaciYard.h"
#include "LaciYardModel.h"
#include "PersonModel.h"
#include "PestiAlvilagGameModeBase.h"
#include "PestiCar.h"
#include "PestiHud.h"
#include "PestiPerson.h"
#include "PoliceModel.h"
#include "PolicePatrols.h"
#include "StoryPerson.h"

namespace ElsoAuto
{
	constexpr int32 TalkStep = 0;
	constexpr int32 StealStep = 1;
	constexpr int32 LoseStep = 2;
	constexpr int32 DeliverStep = 3;

	// Kiss József utca, the wide end towards Népszínház utca: the street's
	// line (from the south end to the north end) and the side of the wide
	// pavement, where the doctor leaves his car.
	const FVector2D StreetFrom(35885.0, 9590.0);
	const FVector2D StreetTo(37009.0, 12752.0);
	// How far along the street (0 to 1) and out from its middle towards the
	// wide pavement the Steyr stands (cm); it faces up the street.
	constexpr double CarAlong = 0.5;
	constexpr double CarOut = 430.0;
	// The constable's beat on the pavement: from and to (along), how far out.
	constexpr double BeatStart = 0.2;
	constexpr double BeatEnd = 0.8;
	constexpr double BeatOut = 700.0;
	constexpr float BeatSpeed = 110.f;
	constexpr float RunSpeed = 520.f;
	constexpr float ChaseSeconds = 6.f;
	// Cars are spawned this high over the ground and settle by themselves (cm).
	constexpr double CarLift = 120.0;

	// What the constable makes of it: within this distance he notices (cm),
	// and he sees it happen when the car is within this angle of where he faces.
	constexpr double NoticeRange = 4000.0;
	constexpr double SeeHalfAngle = 70.0;
	constexpr int32 StarsBehind = 1;
	constexpr int32 StarsInFront = 2;

	// A Steyr this battered gets a warning.
	constexpr float WearWarning = 0.6f;
	// How close to the pit the Steyr must stand, and the player to it (cm).
	constexpr double ParkRadius = 350.0;
	constexpr double PlayerNear = 900.0;
	// How far from the yard a "lose them first" note shows, and how often (s).
	constexpr double YardNear = 3000.0;
	constexpr double WantedHintEvery = 8.0;

	constexpr float TalkRadius = 250.f;
	constexpr int32 HintKey = 39;
	constexpr float HintSeconds = 6.f;

	const TCHAR* const LaciName = TEXT("Laci");
	const TCHAR* const ConstableName = TEXT("Rend\u0151r");
}

AMissionElsoAuto::AMissionElsoAuto()
{
	// Must match PestiStart::ElsoAuto in the game mode.
	MissionId = TEXT("ElsoAuto");
	Reward = 1010;
	TitleHu = TEXT("Az els\u0151 aut\u00f3");
	TitleEn = TEXT("The first car");
	Number = 7;
}

void AMissionElsoAuto::SetUpMission()
{
	for (TActorIterator<ALaciYard> It(GetWorld()); It; ++It)
	{
		Laci = It->GetLaci();
		break;
	}
	const double YardGround = GroundZ(FVector2D(ALaciYard::ToCity(LaciYardModel::Spots::Park, 0.0)), 0.0);
	const FVector LaciAt = Laci ? Laci->GetActorLocation() : ALaciYard::ToCity(LaciYardModel::Spots::Laci, YardGround);
	const FVector ParkAt = ALaciYard::ToCity(LaciYardModel::Spots::Park, YardGround);

	const FVector2D Along = (ElsoAuto::StreetTo - ElsoAuto::StreetFrom);
	const FVector2D Kerb = FVector2D(Along.Y, -Along.X).GetSafeNormal();
	const FVector2D CarSpot = ElsoAuto::StreetFrom + Along * ElsoAuto::CarAlong + Kerb * ElsoAuto::CarOut;

	FMissionStep Talk;
	Talk.GoalHu = TEXT("Menj Lacihoz az aut\u00f3jav\u00edt\u00f3ba, Szerdahelyi utca");
	Talk.GoalEn = TEXT("Go to Laci's motor workshop on Szerdahelyi utca");
	Talk.Target = LaciAt;
	Talk.Radius = ElsoAuto::TalkRadius;
	Talk.bPressE = true;
	Talk.Place = TEXT("Szerdahelyi utca");
	Talk.SceneSeconds = 20.f;
	Talk.SceneWith = Laci.Get();
	Steps.Add(Talk);
	bStartsAtRing = true;
	StartLetter = TEXT("L");

	FMissionStep Steal;
	Steal.GoalHu = TEXT("Vidd el Hal\u00e1sz doktor z\u00f6ld Steyrj\u00e9t a Kiss J\u00f3zsef utc\u00e1b\u00f3l");
	Steal.GoalEn = TEXT("Take Dr. Hal\u00e1sz's green Steyr from Kiss J\u00f3zsef utca");
	Steal.Target = FVector(CarSpot, GroundZ(CarSpot, YardGround));
	Steal.Radius = 400.f;
	Steal.bOnFoot = false;
	Steal.bManual = true;
	Steal.Place = TEXT("Kiss J\u00f3zsef utca");
	Steps.Add(Steal);

	FMissionStep Lose;
	Lose.GoalHu = TEXT("R\u00e1zd le a rend\u0151r\u00f6ket");
	Lose.GoalEn = TEXT("Shake off the police");
	Lose.Target = ParkAt;
	Lose.bOnFoot = false;
	Lose.bManual = true;
	Lose.bGuide = false;
	Lose.Place = TEXT("Szerdahelyi utca");
	Steps.Add(Lose);

	FMissionStep Deliver;
	Deliver.GoalHu = TEXT("\u00c1ll\u00edtsd a Steyrt Laci akn\u00e1ja f\u00f6l\u00e9, \u00e9s sz\u00e1llj ki");
	Deliver.GoalEn = TEXT("Leave the Steyr over the pit in Laci's yard and get out");
	Deliver.Target = ParkAt;
	Deliver.Radius = static_cast<float>(ElsoAuto::ParkRadius);
	Deliver.bOnFoot = false;
	Deliver.bManual = true;
	Deliver.Place = TEXT("Szerdahelyi utca");
	Deliver.SceneSeconds = 13.f;
	Deliver.SceneWith = Laci.Get();
	Steps.Add(Deliver);

	Whistle = NewObject<UPoliceWhistle>(this);
	Whistle->SetupAttachment(GetRootComponent());
	Whistle->bAllowSpatialization = true;
	Whistle->bOverrideAttenuation = true;
	Whistle->AttenuationOverrides.bAttenuate = true;
	Whistle->AttenuationOverrides.bSpatialize = true;
	Whistle->AttenuationOverrides.FalloffDistance = 6000.f;
	Whistle->RegisterComponent();
	Whistle->SetVolumeMultiplier(0.5f);
}

void AMissionElsoAuto::SpawnCarAndConstable()
{
	const FVector2D Along = (ElsoAuto::StreetTo - ElsoAuto::StreetFrom);
	const FVector2D Kerb = FVector2D(Along.Y, -Along.X).GetSafeNormal();
	const double Yaw = FMath::RadiansToDegrees(FMath::Atan2(Along.Y, Along.X));

	// The doctor's Steyr, pulled up on the wide pavement, facing up the street.
	const FVector2D CarSpot = ElsoAuto::StreetFrom + Along * ElsoAuto::CarAlong + Kerb * ElsoAuto::CarOut;
	const FTransform Place(FRotator(0.0, Yaw, 0.0), FVector(CarSpot, GroundZ(CarSpot, 0.0) + ElsoAuto::CarLift));
	Steyr = GetWorld()->SpawnActorDeferred<APestiCar>(APestiCar::StaticClass(), Place, nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (Steyr)
	{
		const CarModel::FStyle Look = CarModel::SteyrStyle();
		Steyr->BodyColor = Look.PaintColor;
		Steyr->SetStyle(Look);
		Steyr->FinishSpawning(Place);
		AddMissionActor(Steyr.Get());
	}

	// The constable on his beat beside it.
	const FVector2D From = ElsoAuto::StreetFrom + Along * ElsoAuto::BeatStart + Kerb * ElsoAuto::BeatOut;
	const FVector2D To = ElsoAuto::StreetFrom + Along * ElsoAuto::BeatEnd + Kerb * ElsoAuto::BeatOut;
	BeatFrom = FVector(From, GroundZ(From, 0.0) + PersonModel::MiddleHeight);
	BeatTo = FVector(To, GroundZ(To, 0.0) + PersonModel::MiddleHeight);
	BeatAlong = 0.3;
	BeatDir = 1;
	Constable = SpawnForMission<APoliceman>(FMath::Lerp(BeatFrom, BeatTo, BeatAlong), Yaw);
	if (Constable)
	{
		FRandomStream Random(1932);
		Constable->PutOnUniform(PoliceModel::Outfit(Random));
	}
}

void AMissionElsoAuto::SayLater(float Seconds, const FString& Who, const FString& LineHu, const FString& LineEn)
{
	// Seconds after the line before it (or after now, if none is waiting).
	const double Now = GetWorld()->GetTimeSeconds();
	const double Last = Lines.Num() > 0 ? Lines.Last().At : Now;
	Lines.Add({ FMath::Max(Last, Now) + Seconds, Who, LineHu, LineEn });
}

void AMissionElsoAuto::Hint(const FString& Text) const
{
	APestiHUD::Show(this, ElsoAuto::HintKey, ElsoAuto::HintSeconds, FColor(255, 210, 60), Text);
}

void AMissionElsoAuto::OnStepStarted(int32 Step)
{
	if (Step == ElsoAuto::StealStep)
	{
		SpawnCarAndConstable();
		if (Steyr)
		{
			SetStepTarget(Steyr->GetActorLocation());
		}
		Hint(TEXT("Tipp: v\u00e1rd meg, m\u00edg a rend\u0151r h\u00e1tat ford\u00edt  (Tip: wait till the constable turns his back)"));
	}
}

void AMissionElsoAuto::OnStepDone(int32 Step)
{
	const APawn* Player = GetPlayer();
	if (Step == ElsoAuto::TalkStep)
	{
		if (Laci && Player)
		{
			Laci->FaceTowards(Player->GetActorLocation());
		}
		Say(ElsoAuto::LaciName,
			TEXT("Sz\u00f3val te vagy az, akit a n\u00e9nik\u00e9m a nyakamra k\u00fcld\u00f6tt. Azt mondja, \u00e9rtesz a korm\u00e1nyhoz."),
			TEXT("So you're the one my aunt sent my way. She says you can handle a wheel."), 6.f);
		SayLater(6.f, ElsoAuto::LaciName,
			TEXT("Hal\u00e1sz doktornak van egy z\u00f6ld Steyrje, a Kiss J\u00f3zsef utc\u00e1ban \u00e1ll. Egy \u00far a Lip\u00f3tv\u00e1rosb\u00f3l sz\u00e9p p\u00e9nzt adna \u00e9rte."),
			TEXT("Dr. Hal\u00e1sz has a green Steyr standing on Kiss J\u00f3zsef utca. A gentleman in Lip\u00f3tv\u00e1ros would pay good money for it."));
		SayLater(7.f, ElsoAuto::LaciName,
			TEXT("Egy rend\u0151r j\u00e1r ott fel-al\u00e1. V\u00e1rd meg, m\u00edg h\u00e1tat ford\u00edt. Azt\u00e1n r\u00e1zd le a hekusokat, \u00e9s hozd ide, egy karcol\u00e1s n\u00e9lk\u00fcl."),
			TEXT("A constable walks up and down there. Wait till his back is turned. Then shake off the coppers and bring it here without a scratch."));
		return;
	}
	if (Step == ElsoAuto::DeliverStep)
	{
		Lines.Reset();
		if (Laci && Player)
		{
			Laci->FaceTowards(Player->GetActorLocation());
		}
		// The Steyr stays over the pit; Laci has work to do on it.
		if (Steyr)
		{
			KeepAfterMission(Steyr.Get());
		}
		Say(ElsoAuto::LaciName,
			TEXT("Sz\u00e9p darab! Egy h\u00e9t, \u00e9s a saj\u00e1t gazd\u00e1ja sem ismer r\u00e1. Itt a r\u00e9szed."),
			TEXT("A beauty! Give me a week and its own master won't know it. Here's your share."), 6.f);
		SayLater(6.f, ElsoAuto::LaciName,
			TEXT("\u00c9jjel a fi\u00fak n\u00e9ha versenyt aut\u00f3znak a M\u00e1ty\u00e1s t\u00e9ren. Majd sz\u00f3lok."),
			TEXT("At night the lads sometimes race on M\u00e1ty\u00e1s t\u00e9r. I'll let you know."));
	}
}

void AMissionElsoAuto::TickStep(int32 Step, float DeltaSeconds)
{
	// Lines said a little later, in turn.
	const double Now = GetWorld()->GetTimeSeconds();
	if (Lines.Num() > 0 && Now >= Lines[0].At)
	{
		Say(Lines[0].Who, Lines[0].Hu, Lines[0].En, 7.f);
		Lines.RemoveAt(0);
	}

	if (Step < ElsoAuto::StealStep)
	{
		return;
	}
	TickConstable(DeltaSeconds);

	const APawn* Player = GetPlayer();
	if (!Steyr)
	{
		return;
	}
	const bool bInSteyr = Player == Steyr.Get();
	if (Step == ElsoAuto::StealStep)
	{
		if (bInSteyr)
		{
			OnCarTaken();
			FinishStep();
		}
		return;
	}
	if (TickCarTrouble())
	{
		return;
	}

	const APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this);
	const int32 Wanted = Game ? Game->GetWantedLevel() : 0;
	if (Step == ElsoAuto::LoseStep)
	{
		if (Wanted > 0)
		{
			bChased = true;
			return;
		}
		if (bChased)
		{
			Hint(TEXT("Ler\u00e1ztad \u0151ket. Most vidd a Steyrt Lacihoz  (You've lost them. Now take the Steyr to Laci)"));
		}
		FinishStep();
		return;
	}
	if (Step == ElsoAuto::DeliverStep)
	{
		const FVector ParkAt = Steps[Step].Target;
		const bool bParked = FVector::Dist2D(Steyr->GetActorLocation(), ParkAt) < ElsoAuto::ParkRadius;
		if (Wanted > 0)
		{
			if (FVector::Dist2D(Steyr->GetActorLocation(), ParkAt) < ElsoAuto::YardNear && Now - LastWantedHint > ElsoAuto::WantedHintEvery)
			{
				LastWantedHint = Now;
				Hint(TEXT("Laci nem nyit kaput, am\u00edg a rend\u0151r\u00f6k a nyomodban vannak  (Laci won't open up with the police on your tail)"));
			}
			return;
		}
		if (bParked && Player && Player->IsA<APestiPerson>()
			&& FVector::Dist2D(Player->GetActorLocation(), ParkAt) < ElsoAuto::PlayerNear)
		{
			FinishStep();
		}
	}
}

void AMissionElsoAuto::OnCarTaken()
{
	if (bCarTaken)
	{
		return;
	}
	bCarTaken = true;
	APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this);
	ArrestsAtStart = Game ? Game->GetArrests() : 0;
	if (!Constable || !Steyr)
	{
		return;
	}

	// Did he see it, or only hear the engine behind him?
	const FVector ToCar = Steyr->GetActorLocation() - Constable->GetActorLocation();
	const double Distance = ToCar.Size2D();
	if (Distance > ElsoAuto::NoticeRange)
	{
		return;
	}
	const double Facing = FVector::DotProduct(Constable->GetActorForwardVector().GetSafeNormal2D(), ToCar.GetSafeNormal2D());
	const bool bSaw = Facing > FMath::Cos(FMath::DegreesToRadians(ElsoAuto::SeeHalfAngle));
	if (Game)
	{
		Game->ReportCrime(bSaw ? ElsoAuto::StarsInFront : ElsoAuto::StarsBehind);
	}
	if (Whistle)
	{
		Whistle->SetWorldLocation(Constable->GetActorLocation());
		Whistle->Blow();
	}
	ChaseLeft = ElsoAuto::ChaseSeconds;
	if (bSaw)
	{
		Say(ElsoAuto::ConstableName, TEXT("\u00c1llj! Meg\u00e1llni! Rend\u0151rs\u00e9g!"), TEXT("Stop! Halt! Police!"), 4.f);
	}
	else
	{
		Say(ElsoAuto::ConstableName, TEXT("H\u00e9! Az a doktor \u00far aut\u00f3ja!"), TEXT("Hey! That's the doctor's car!"), 4.f);
	}
}

void AMissionElsoAuto::TickConstable(float DeltaSeconds)
{
	if (!Constable)
	{
		return;
	}
	if (!bCarTaken)
	{
		// Up and down the pavement, turning at each end.
		const double Length = FMath::Max(1.0, FVector::Dist2D(BeatFrom, BeatTo));
		BeatAlong += BeatDir * ElsoAuto::BeatSpeed * DeltaSeconds / Length;
		if (BeatAlong >= 1.0 || BeatAlong <= 0.0)
		{
			BeatAlong = FMath::Clamp(BeatAlong, 0.0, 1.0);
			BeatDir = -BeatDir;
		}
		const FVector Heading = (BeatTo - BeatFrom) * BeatDir;
		const double Yaw = FMath::RadiansToDegrees(FMath::Atan2(Heading.Y, Heading.X));
		Constable->WalkOn(FMath::Lerp(BeatFrom, BeatTo, BeatAlong), Yaw, ElsoAuto::BeatSpeed, DeltaSeconds);
		return;
	}
	const FVector At = Constable->GetActorLocation();
	if (ChaseLeft > 0.f && Steyr)
	{
		// A few steps after the car, then he gives up and stands.
		ChaseLeft -= DeltaSeconds;
		const FVector ToCar = Steyr->GetActorLocation() - At;
		const double Yaw = FMath::RadiansToDegrees(FMath::Atan2(ToCar.Y, ToCar.X));
		const FVector Move = ToCar.GetSafeNormal2D() * ElsoAuto::RunSpeed * DeltaSeconds;
		const FVector2D Next(At + Move);
		Constable->RunTo(FVector(Next, GroundZ(Next, At.Z - PersonModel::MiddleHeight) + PersonModel::MiddleHeight), Yaw, ElsoAuto::RunSpeed, DeltaSeconds);
		return;
	}
	Constable->Stand(At, Constable->GetActorRotation().Yaw);
}

bool AMissionElsoAuto::TickCarTrouble()
{
	APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this);
	if (Game && Game->GetArrests() > ArrestsAtStart)
	{
		Fail(TEXT("Elkaptak a rend\u0151r\u00f6k"), TEXT("The police caught you"));
		return true;
	}
	if (!IsValid(Steyr.Get()) || Steyr->IsWrecked())
	{
		Fail(TEXT("\u00d6sszet\u00f6rted a Steyrt"), TEXT("You wrecked the Steyr"));
		return true;
	}
	if (!bWarnedWear && Steyr->GetWear() > ElsoAuto::WearWarning)
	{
		bWarnedWear = true;
		Hint(TEXT("Vigy\u00e1zz a Steyrre! Ha \u00f6sszet\u00f6r\u00f6d, Laci nem fizet  (Careful with the Steyr! Wreck it and Laci won't pay)"));
	}
	return false;
}

bool AMissionElsoAuto::GetRetrySpot(FVector& OutAt, double& OutYaw) const
{
	const FVector Gate = LaciYardModel::Spots::Gate + FVector(300.0, 0.0, 0.0);
	const FVector2D Flat(ALaciYard::ToCity(Gate, 0.0));
	OutAt = FVector(Flat, GroundZ(Flat, 0.0) + PersonModel::MiddleHeight + 10.0);
	OutYaw = LaciYardModel::CityYaw;
	return true;
}

void AMissionElsoAuto::OnFailed()
{
	Lines.Reset();
	// Out of the car, so that trying again finds him on his feet; if he
	// cannot get out, the car stays rather than vanish under him.
	if (IsValid(Steyr.Get()) && GetPlayer() == Steyr.Get() && !Steyr->ForceDriverOut())
	{
		KeepAfterMission(Steyr.Get());
	}
}
