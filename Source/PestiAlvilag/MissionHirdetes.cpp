// Fill out your copyright notice in the Description page of Project Settings.

#include "MissionHirdetes.h"

#include "CarShowModel.h"
#include "Engine/TargetPoint.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "LaciYard.h"
#include "LaciYardModel.h"
#include "PersonModel.h"
#include "PestiAlvilagGameModeBase.h"
#include "PestiHud.h"
#include "PestiPerson.h"
#include "StoryPerson.h"
#include "Tailor.h"
#include "TailorModel.h"
#include "Tattersall.h"

namespace HirdetesPlan
{
	constexpr int32 TalkStep = 0;
	constexpr int32 SuitStep = 1;
	constexpr int32 ShowStep = 2;
	constexpr int32 SalesmanStep = 3;
	constexpr int32 OverhearStep = 4;
	constexpr int32 DoorStep = 5;
	constexpr int32 BackStep = 6;

	// How near the hall's doors counts as there (cm).
	constexpr float ShowRadius = 600.f;
	constexpr float TalkRadius = 250.f;
	constexpr float CounterRadius = 150.f;
	constexpr float DoorRadius = 200.f;
	// Overhearing: the player stays this near Weisz (cm).
	constexpr float OverhearRadius = 900.f;
	// The side door is looked at from this far out in the lane (cm).
	constexpr double DoorOut = 150.0;
	// How often the "shut" note shows (s).
	constexpr double ShutHintEvery = 10.0;
	// Kálmán starts talking this near Weisz, or this long after coming in (cm, s);
	// he is gone this near the way out (cm).
	constexpr double KalmanAtWeisz = 320.0;
	constexpr double KalmanWalkMost = 14.0;
	constexpr double KalmanGone = 250.0;
	// The talk between Weisz and Kálmán ends this long after its last line (s).
	constexpr double TalkTail = 5.0;

	constexpr int32 HintKey = 39;
	constexpr float HintSeconds = 6.f;

	const TCHAR* const LaciName = TEXT("Laci");
	const TCHAR* const WeiszName = TEXT("Weisz \u00far");
	const TCHAR* const KalmanName = TEXT("K\u00e1lm\u00e1n");
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

	FMissionStep Suit;
	Suit.GoalHu = TEXT("Vegy\u00e9l \u00f6lt\u00f6nyt Kert\u00e9sz szab\u00f3n\u00e1l, B\u00e9rkocsis utca");
	Suit.GoalEn = TEXT("Buy a suit at Kert\u00e9sz the tailor's on B\u00e9rkocsis utca");
	Suit.Target = ATailor::BuySpot(World);
	Suit.Radius = HirdetesPlan::CounterRadius;
	Suit.bPressE = true;
	Suit.bManual = true;
	Suit.Place = TEXT("B\u00e9rkocsis utca");
	Steps.Add(Suit);

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
	Salesman.SceneSeconds = 12.f;
	Steps.Add(Salesman);

	FMissionStep Overhear;
	Overhear.GoalHu = TEXT("N\u00e9zegesd az aut\u00f3kat, \u00e9s hallgasd, mit besz\u00e9lnek");
	Overhear.GoalEn = TEXT("Look at the cars and listen to what they say");
	Overhear.Target = Salesman.Target;
	Overhear.Radius = HirdetesPlan::OverhearRadius;
	Overhear.bManual = true;
	Overhear.bGuide = false;
	Overhear.Place = TEXT("Kerepesi \u00fat");
	Steps.Add(Overhear);

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
	Back.GoalHu = TEXT("Mondd el Lacinak, mit hallott\u00e1l");
	Back.GoalEn = TEXT("Tell Laci what you heard");
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

AStoryPerson* AMissionHirdetes::FindWeisz() const
{
	for (TActorIterator<ATattersall> It(GetWorld()); It; ++It)
	{
		return It->GetSalesman();
	}
	return nullptr;
}

void AMissionHirdetes::OnStepStarted(int32 Step)
{
	if (Step == HirdetesPlan::SuitStep)
	{
		// The tailor may not have been there when the steps were made.
		const FVector Counter = ATailor::BuySpot(GetWorld());
		if (!Counter.IsZero())
		{
			SetStepTarget(Counter);
		}
		return;
	}
	if (Step == HirdetesPlan::SalesmanStep)
	{
		if (AStoryPerson* Weisz = FindWeisz())
		{
			Steps[Step].SceneWith = Weisz;
			SetStepTarget(Weisz->GetActorLocation());
		}
		return;
	}
	if (Step == HirdetesPlan::OverhearStep)
	{
		// Kálmán, the importer's chauffeur, walks in from the doors for the spare key.
		const UWorld* World = GetWorld();
		const FTransform Hall = ATattersall::Frame(World);
		const FVector Doors = ATattersall::ToCity(World, CarShowModel::Spots::OutOfDoors);
		Kalman = SpawnForMission<AStoryPerson>(Doors + FVector(0.0, 0.0, PersonModel::MiddleHeight), Hall.Rotator().Yaw + 180.0);
		WayOut = SpawnForMission<ATargetPoint>(Doors + FVector(0.0, 0.0, PersonModel::MiddleHeight), 0.0);
		if (Kalman)
		{
			Kalman->SetOutfit(CarShowModel::Chauffeur());
			if (AStoryPerson* Weisz = FindWeisz())
			{
				Kalman->Follow(Weisz);
				SetStepTarget(Weisz->GetActorLocation());
			}
		}
		KalmanSince = GetWorld()->GetTimeSeconds();
		TalkOverAt = -1.0;
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
		const APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this);
		if (Game && Game->HasSuit())
		{
			SayLater(6.f, HirdetesPlan::LaciName,
				TEXT("Menj el, n\u00e9zd meg j\u00f3l. Vedd fel a sz\u00e9p \u00f6lt\u00f6ny\u00f6det, \u00e9s k\u00e9rdezz, mint egy \u00far, aki venni akar."),
				TEXT("Go and have a good look. Put on that fine suit of yours and ask like a gentleman who wants to buy."));
		}
		else
		{
			SayLater(6.f, HirdetesPlan::LaciName,
				TEXT("De ebben a kab\u00e1tban be sem engednek. Kert\u00e9sz, a szab\u00f3, a gar\u00e1zsoddal szemben varr az uraknak. Vegy\u00e9l t\u0151le egy \u00f6lt\u00f6nyt."),
				TEXT("But they won't even let you in in that coat. Kert\u00e9sz the tailor, across from your garage, sews for gentlemen. Buy a suit from him."));
		}
		return;
	}
	if (Step == HirdetesPlan::SuitStep)
	{
		Hint(TEXT("Most m\u00e1r \u00fagy festesz, mint egy \u00far. Ir\u00e1ny a Tattersall!  (Now you look like a gentleman. Off to the Tattersall!)"));
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
			TEXT("Sajnos ez a darab nem elad\u00f3: Vogel \u00far\u00e9, a gy\u00e1r pesti k\u00e9pvisel\u0151j\u00e9\u00e9. De rendelhet\u00fcnk \u00f6nnek egyet Stuttgartb\u00f3l."),
			TEXT("I'm afraid this one is not for sale: it's Mr. Vogel's, the works' man in Pest. But we can order you one from Stuttgart."));
		return;
	}
	if (Step == HirdetesPlan::OverhearStep)
	{
		Hint(TEXT("A sof\u0151r a Bauer-kocsm\u00e1ban iszik, a Baross t\u00e9ren. N\u00e9zz k\u00f6r\u00fcl odakint is  (The chauffeur drinks at Bauer's tavern on Baross t\u00e9r. Have a look round outside too)"));
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
			TEXT("A p\u00f3tkulcs egy sof\u0151r zseb\u00e9ben, aki a Bauern\u00e9l iszik? Enn\u00e9l jobb h\u00edrt nem is hozhatt\u00e1l."),
			TEXT("The spare key in the pocket of a chauffeur who drinks at Bauer's? You couldn't have brought better news."), 6.f);
		SayLater(6.f, HirdetesPlan::LaciName,
			TEXT("\u00dclj le mell\u00e9, fizess neki p\u00e1r k\u00f6rt, am\u00edg el nem alszik. Itt a p\u00e9nzed a f\u00e1rads\u00e1g\u00e9rt, meg a kocsm\u00e1ra is."),
			TEXT("Sit down with him and stand him a few rounds till he falls asleep. Here's your money for the trouble, and for the tavern too."));
	}
}

void AMissionHirdetes::TickChauffeur()
{
	if (!Kalman)
	{
		return;
	}
	const double Now = GetWorld()->GetTimeSeconds();
	AStoryPerson* Weisz = FindWeisz();
	if (TalkOverAt < 0.0)
	{
		const bool bThere = !Weisz || FVector::Dist2D(Kalman->GetActorLocation(), Weisz->GetActorLocation()) < HirdetesPlan::KalmanAtWeisz;
		if (bThere || Now - KalmanSince > HirdetesPlan::KalmanWalkMost)
		{
			// The spare key changes hands, and the player hears where it goes
			// (after whatever Weisz was still saying to the player).
			if (Weisz)
			{
				Weisz->FaceTowards(Kalman->GetActorLocation());
				Kalman->FaceTowards(Weisz->GetActorLocation());
			}
			SayLater(1.f, HirdetesPlan::KalmanName,
				TEXT("J\u00f3 napot, Weisz \u00far. A p\u00f3tkulcsot k\u00e9rn\u00e9m, Vogel \u00far ma este a kocsival menne a Ligetbe."),
				TEXT("Good day, Mr. Weisz. The spare key, please; Mr. Vogel wants the car for the Liget tonight."));
			SayLater(6.f, HirdetesPlan::WeiszName,
				TEXT("Tess\u00e9k, K\u00e1lm\u00e1n. De holnap reggelre hozza vissza, \u00e9s ez\u00fattal j\u00f3zanon!"),
				TEXT("Here you are, K\u00e1lm\u00e1n. But bring it back by tomorrow morning, and sober this time!"));
			SayLater(6.f, HirdetesPlan::KalmanName,
				TEXT("Ugyan, Weisz \u00far, egy-k\u00e9t kupica... Ha keresnek, a Bauern\u00e9l vagyok a Baross t\u00e9ren, mint mindig."),
				TEXT("Come now, Mr. Weisz, a glass or two... If anyone wants me, I'm at Bauer's on Baross t\u00e9r, as always."));
			TalkOverAt = Lines.Last().At + HirdetesPlan::TalkTail;
		}
		return;
	}
	if (Now >= TalkOverAt)
	{
		// Off he goes with the key; gone once at the doors.
		if (WayOut)
		{
			Kalman->Follow(WayOut.Get());
			if (FVector::Dist2D(Kalman->GetActorLocation(), WayOut->GetActorLocation()) < HirdetesPlan::KalmanGone + Kalman->FollowDistance)
			{
				Kalman->Destroy();
				Kalman = nullptr;
			}
		}
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
	if (Step >= HirdetesPlan::OverhearStep)
	{
		TickChauffeur();
	}

	const APawn* Player = GetPlayer();
	if (!Player)
	{
		return;
	}
	const APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this);
	const double Distance = FVector::Dist2D(Player->GetActorLocation(), Steps[Step].Target);
	const bool bThere = Distance < Steps[Step].Radius;
	if (Step == HirdetesPlan::SuitStep)
	{
		// Bought at Kertész's (ATailor), or his already.
		if (Game && Game->HasSuit())
		{
			FinishStep();
		}
		return;
	}
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
		return;
	}
	if (Step == HirdetesPlan::OverhearStep)
	{
		// Heard all over the hall, wherever in it the player was.
		if (TalkOverAt >= 0.0 && Now >= TalkOverAt)
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
