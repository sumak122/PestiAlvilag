// Fill out your copyright notice in the Description page of Project Settings.

#include "MissionHirdetes.h"

#include "CarShowModel.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "LaciYard.h"
#include "LaciYardModel.h"
#include "PersonModel.h"
#include "PestiHud.h"
#include "PestiPerson.h"
#include "StoryPerson.h"
#include "Tattersall.h"

namespace HirdetesPlan
{
	constexpr int32 TalkStep = 0;
	constexpr int32 ShowStep = 1;
	constexpr int32 SalesmanStep = 2;
	constexpr int32 DoorStep = 3;
	constexpr int32 BackStep = 4;

	// How near the hall's doors counts as there (cm).
	constexpr float ShowRadius = 600.f;
	constexpr float TalkRadius = 250.f;
	constexpr float DoorRadius = 200.f;
	// The side door is looked at from this far out in the lane (cm).
	constexpr double DoorOut = 150.0;
	// How often the "shut" note shows (s).
	constexpr double ShutHintEvery = 10.0;

	constexpr int32 HintKey = 39;
	constexpr float HintSeconds = 6.f;

	const TCHAR* const LaciName = TEXT("Laci");
	const TCHAR* const WeiszName = TEXT("Weisz \u00far");
}

AMissionHirdetes::AMissionHirdetes()
{
	// Must match PestiStart::Hirdetes in the game mode.
	MissionId = TEXT("Hirdetes");
	Reward = 500;
	TitleHu = TEXT("A hirdet\u00e9s");
	TitleEn = TEXT("The advert");
	Number = 0;
}

void AMissionHirdetes::SetUpMission()
{
	for (TActorIterator<ALaciYard> It(GetWorld()); It; ++It)
	{
		Laci = It->GetLaci();
		break;
	}
	const double YardGround = GroundZ(FVector2D(ALaciYard::ToCity(LaciYardModel::Spots::Laci, 0.0)), 0.0);
	const FVector LaciAt = Laci ? Laci->GetActorLocation() : ALaciYard::ToCity(LaciYardModel::Spots::Laci, YardGround);
	const UWorld* World = GetWorld();

	FMissionStep Talk;
	Talk.GoalHu = TEXT("Menj Lacihoz, Szerdahelyi utca");
	Talk.GoalEn = TEXT("Go to Laci on Szerdahelyi utca");
	Talk.Target = LaciAt;
	Talk.Radius = HirdetesPlan::TalkRadius;
	Talk.bPressE = true;
	Talk.Place = TEXT("Szerdahelyi utca");
	Talk.SceneSeconds = 18.f;
	Talk.SceneWith = Laci.Get();
	Steps.Add(Talk);
	bStartsAtRing = true;
	StartLetter = TEXT("L");

	FMissionStep Show;
	Show.GoalHu = TEXT("Menj a Tattersallba, Kerepesi \u00fat");
	Show.GoalEn = TEXT("Go to the Tattersall on Kerepesi \u00fat");
	Show.Target = ATattersall::ToCity(World, CarShowModel::Spots::OutOfDoors);
	Show.Radius = HirdetesPlan::ShowRadius;
	Show.bOnFoot = false;
	Show.bManual = true;
	Show.Place = TEXT("Kerepesi \u00fat");
	Steps.Add(Show);

	FMissionStep Salesman;
	Salesman.GoalHu = TEXT("K\u00e9rdezd meg az elad\u00f3t a fekete Mercedesr\u0151l");
	Salesman.GoalEn = TEXT("Ask the salesman about the black Mercedes");
	Salesman.Target = ATattersall::ToCity(World, CarShowModel::SalesmanSpot);
	Salesman.Radius = HirdetesPlan::TalkRadius;
	Salesman.bPressE = true;
	Salesman.bManual = true;
	Salesman.bGuide = false;
	Salesman.Place = TEXT("Kerepesi \u00fat");
	Salesman.SceneSeconds = 16.f;
	Steps.Add(Salesman);

	FMissionStep Door;
	Door.GoalHu = TEXT("N\u00e9zd meg a csarnok oldalajtaj\u00e1t");
	Door.GoalEn = TEXT("Have a look at the hall's side door");
	Door.Target = ATattersall::ToCity(World, CarShowModel::Spots::SideDoor + FVector(0.0, -HirdetesPlan::DoorOut, 0.0));
	Door.Radius = HirdetesPlan::DoorRadius;
	Door.bPressE = true;
	Door.bGuide = false;
	Door.Place = TEXT("Kerepesi \u00fat");
	Steps.Add(Door);

	FMissionStep Back;
	Back.GoalHu = TEXT("Mondd el Lacinak, mit l\u00e1tt\u00e1l");
	Back.GoalEn = TEXT("Tell Laci what you saw");
	Back.Target = LaciAt;
	Back.Radius = HirdetesPlan::TalkRadius;
	Back.bPressE = true;
	Back.Place = TEXT("Szerdahelyi utca");
	Back.SceneSeconds = 14.f;
	Back.SceneWith = Laci.Get();
	Steps.Add(Back);
}

void AMissionHirdetes::SayLater(float Seconds, const FString& Who, const FString& LineHu, const FString& LineEn)
{
	// Seconds after the line before it (or after now, if none is waiting).
	const double Now = GetWorld()->GetTimeSeconds();
	const double Last = Lines.Num() > 0 ? Lines.Last().At : Now;
	Lines.Add({ FMath::Max(Last, Now) + Seconds, Who, LineHu, LineEn });
}

void AMissionHirdetes::Hint(const FString& Text) const
{
	APestiHUD::Show(this, HirdetesPlan::HintKey, HirdetesPlan::HintSeconds, FColor(255, 210, 60), Text);
}

void AMissionHirdetes::HintShut()
{
	const double Now = GetWorld()->GetTimeSeconds();
	if (Now - LastShutHint > HirdetesPlan::ShutHintEvery)
	{
		LastShutHint = Now;
		Hint(TEXT("A ki\u00e1ll\u00edt\u00e1s z\u00e1rva. Reggel 9-kor nyit, este 7-kor z\u00e1r  (The show is shut. It opens at 9 in the morning and shuts at 7 in the evening)"));
	}
}

void AMissionHirdetes::OnStepStarted(int32 Step)
{
	if (Step == HirdetesPlan::SalesmanStep)
	{
		// Mr. Weisz by the SSK.
		for (TActorIterator<ATattersall> It(GetWorld()); It; ++It)
		{
			if (AStoryPerson* Weisz = It->GetSalesman())
			{
				Steps[Step].SceneWith = Weisz;
				SetStepTarget(Weisz->GetActorLocation());
			}
			break;
		}
	}
}

void AMissionHirdetes::OnStepDone(int32 Step)
{
	const APawn* Player = GetPlayer();
	if (Step == HirdetesPlan::TalkStep)
	{
		if (Laci && Player)
		{
			Laci->FaceTowards(Player->GetActorLocation());
		}
		Say(HirdetesPlan::LaciName,
			TEXT("Na, megnyerted mind a n\u00e9gyet. N\u00e9zd csak, mit hoz ma a Pesti H\u00edrlap."),
			TEXT("So, you won all four. Look what the Pesti H\u00edrlap has today."), 6.f);
		SayLater(6.f, HirdetesPlan::LaciName,
			TEXT("\u00bbNemzetk\u00f6zi Aut\u00f3ki\u00e1ll\u00edt\u00e1s a Tattersallban.\u00ab \u00c9s ott a k\u00e9pen: egy fekete Mercedes SSK. Fekete, mint az \u00e9jszaka."),
			TEXT("\"International Motor Show at the Tattersall.\" And there in the picture: a black Mercedes SSK. Black as the night."));
		SayLater(6.f, HirdetesPlan::LaciName,
			TEXT("Menj el, n\u00e9zd meg j\u00f3l. K\u00e9rdezz, mint egy \u00far, aki venni akar. Nappal vannak nyitva."),
			TEXT("Go and have a good look. Ask about it like a gentleman who wants to buy. They're open by day."));
		return;
	}
	if (Step == HirdetesPlan::ShowStep)
	{
		Hint(TEXT("Menj be a csarnokba, az elad\u00f3 a fekete aut\u00f3n\u00e1l \u00e1ll  (Go into the hall, the salesman stands by the black car)"));
		return;
	}
	if (Step == HirdetesPlan::SalesmanStep)
	{
		AStoryPerson* Weisz = Cast<AStoryPerson>(Steps[Step].SceneWith.Get());
		if (Weisz && Player)
		{
			Weisz->FaceTowards(Player->GetActorLocation());
		}
		Say(HirdetesPlan::WeiszName,
			TEXT("A Mercedes-Benz SSK, uram. H\u00e9t liter, kompresszor. K\u00e9tsz\u00e1z kilom\u00e9ter \u00f3r\u00e1nk\u00e9nt, ha az ember mer."),
			TEXT("The Mercedes-Benz SSK, sir. Seven litres, supercharged. Two hundred kilometres an hour, if one dares."), 6.f);
		SayLater(6.f, HirdetesPlan::WeiszName,
			TEXT("K\u00e9t kulcsa van. Az egyik itt, az irod\u00e1ban, a szekr\u00e9nyben. A m\u00e1sik Vogel \u00far sof\u0151rj\u00e9n\u00e9l, K\u00e1lm\u00e1nn\u00e1l."),
			TEXT("It has two keys. One is here, in the office cabinet. The other is with Mr. Vogel's chauffeur, K\u00e1lm\u00e1n."));
		SayLater(6.f, HirdetesPlan::WeiszName,
			TEXT("K\u00e1lm\u00e1n? Az most is a Bauer kocsm\u00e1ja el\u0151tt \u00fcl a Baross t\u00e9ren, ha j\u00f3l ismerem. De k\u00e9rem, j\u00f6jj\u00f6n vissza Vogel \u00farral."),
			TEXT("K\u00e1lm\u00e1n? If I know him, he's sitting before Bauer's tavern on Baross t\u00e9r right now. But do come back with Mr. Vogel."));
		return;
	}
	if (Step == HirdetesPlan::DoorStep)
	{
		Say(TEXT("T\u00e1bla"),
			TEXT("\u00bb\u00c9jjeli\u0151r szolg\u00e1latban este 7 \u00f3r\u00e1t\u00f3l. Bel\u00e9pni tilos!\u00ab"),
			TEXT("\"Night watchman on duty from 7 in the evening. No entry!\""), 6.f);
		return;
	}
	if (Step == HirdetesPlan::BackStep)
	{
		Lines.Reset();
		if (Laci && Player)
		{
			Laci->FaceTowards(Player->GetActorLocation());
		}
		Say(HirdetesPlan::LaciName,
			TEXT("K\u00e9t kulcs, \u00e9s az egyik egy sof\u0151r zseb\u00e9ben \u00fcl a kocsm\u00e1ban? Enn\u00e9l jobb h\u00edrt nem is hozhatt\u00e1l."),
			TEXT("Two keys, and one sits in a chauffeur's pocket at the tavern? You couldn't have brought better news."), 6.f);
		SayLater(6.f, HirdetesPlan::LaciName,
			TEXT("Holnap szerezd meg azt a kulcsot. Csendben, ha lehet. Itt a p\u00e9nzed a f\u00e1rads\u00e1g\u00e9rt."),
			TEXT("Get that key tomorrow. Quietly, if you can. Here's your money for the trouble."));
	}
}

void AMissionHirdetes::TickStep(int32 Step, float DeltaSeconds)
{
	// Lines said a little later, in turn.
	const double Now = GetWorld()->GetTimeSeconds();
	if (Lines.Num() > 0 && Now >= Lines[0].At)
	{
		Say(Lines[0].Who, Lines[0].Hu, Lines[0].En, 7.f);
		Lines.RemoveAt(0);
	}

	const APawn* Player = GetPlayer();
	if (!Player)
	{
		return;
	}
	const double Distance = FVector::Dist2D(Player->GetActorLocation(), Steps[Step].Target);
	const bool bThere = Distance < Steps[Step].Radius;
	if (Step == HirdetesPlan::ShowStep)
	{
		if (!bThere)
		{
			return;
		}
		if (ATattersall::IsOpen(GetWorld()))
		{
			FinishStep();
		}
		else
		{
			HintShut();
		}
		return;
	}
	if (Step == HirdetesPlan::SalesmanStep)
	{
		// Mr. Weisz goes home at closing time; the step waits for the morning.
		if (!ATattersall::IsOpen(GetWorld()))
		{
			if (Distance < HirdetesPlan::ShowRadius * 4.0)
			{
				HintShut();
			}
			return;
		}
		const APlayerController* PC = Cast<APlayerController>(Player->GetController());
		if (bThere && Player->IsA<APestiPerson>() && PC && PC->WasInputKeyJustPressed(EKeys::E))
		{
			FinishStep();
		}
	}
}

bool AMissionHirdetes::GetRetrySpot(FVector& OutAt, double& OutYaw) const
{
	const FVector Gate = LaciYardModel::Spots::Gate + FVector(300.0, 0.0, 0.0);
	const FVector2D Flat(ALaciYard::ToCity(Gate, 0.0));
	OutAt = FVector(Flat, GroundZ(Flat, 0.0) + PersonModel::MiddleHeight + 10.0);
	OutYaw = LaciYardModel::CityYaw;
	return true;
}
