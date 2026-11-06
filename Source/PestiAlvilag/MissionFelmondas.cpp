// Fill out your copyright notice in the Description page of Project Settings.

#include "MissionFelmondas.h"

#include "Camera/PlayerCameraManager.h"
#include "DayNight.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "FleaMarket.h"
#include "GarayModel.h"
#include "GarayYard.h"
#include "InputCoreTypes.h"
#include "PersonModel.h"
#include "PestiAlvilagGameModeBase.h"
#include "PestiHud.h"
#include "PestiPerson.h"
#include "Ruffian.h"
#include "StoryPerson.h"

namespace Felmondas
{
	// Pista bácsi's junk stall at the flea market, in case the market
	// can't be found (the street furniture file puts him here).
	const FVector2D PistaFallback(101337.0, 30370.0);

	constexpr float TalkRadius = 250.f;
	constexpr float DoorRadius = 220.f;
	constexpr float CounterRadius = 300.f;
	constexpr float FightRadius = 600.f;
	// He squares up to the old man from this close (cm, flat).
	constexpr double HermannReach = 280.0;
	// Waiting outside the shop until the bells ring for noon (s).
	constexpr double NoonAfter = 9.0;
	// The clock jumps to then on arriving; more than this (hours) fades in from black (s).
	constexpr float FadeJump = 0.5f;
	constexpr float NoonFade = 1.5f;
	// Further than this from the shop during the fight, he has walked out of it (cm).
	constexpr double FightLeft = 2500.0;
	// Schwimmer waits for him behind Pista bácsi's stall, this far back (cm).
	constexpr double MeetBehind = 170.0;
	// After Schwimmer's thanks, the paper comes out, and stays up this long (s).
	constexpr double HeadlineAfter = 7.0;
	/** The cutting stays up until E, or this long at most (s). */
	constexpr double HeadlineFor = 120.0;
	/** E only puts the cutting away after this long, so a press meant for Schwimmer can't (s). */
	constexpr double HeadlineMin = 1.0;
	// Spawning a person a little above the floor (cm).
	constexpr double SpawnLift = 10.0;

	// Géza and the clerks go for him in this order, two at a time.
	const ERuffian FightOrder[] = { ERuffian::Geza, ERuffian::ClerkOne, ERuffian::ClerkTwo };
	constexpr int32 AtOnce = 2;
	// How keen each is (below 1: thinks slower, no grabbing, no double jabs):
	// the first man in, and the second, who holds back more. Made easier
	// after Daniel's test.
	constexpr float FirstEager = 0.75f;
	constexpr float SecondEager = 0.4f;
	// Another man coming at him this much closer than the one he faces: he turns to him.
	constexpr double TurnShare = 0.6;
	// The fight is over this long after the last one goes down (s).
	constexpr double OverAfter = 2.5;

	const TCHAR* const PistaName = TEXT("Pista b\u00e1csi");
	const TCHAR* const SchwimmerName = TEXT("Schwimmer Mih\u00e1ly");
	const TCHAR* const HermannName = TEXT("Friedrich Hermann");
	const TCHAR* const GezaName = TEXT("Friedrich G\u00e9za");

	// Steps, in order.
	enum EStep : int32 { TalkPista, FindSchwimmer, WaitNoon, HitHermann, Fight, Getaway, Meet, Paper };

	// Failing in the shop or after it: trying again starts at the shop
	// door. Kept for the next mission of the same world only.
	TWeakObjectPtr<UWorld> ResumeWorld;
}

AMissionFelmondas::AMissionFelmondas()
{
	// Must match PestiStart::Felmondas in the game mode.
	MissionId = TEXT("Felmondas");
	Reward = 820;
	TitleHu = TEXT("A felmond\u00e1s");
	TitleEn = TEXT("The notice");
}

FVector AMissionFelmondas::YardPoint(const FVector& Local) const
{
	return YardFrame.TransformPosition(Local);
}

void AMissionFelmondas::SetUpMission()
{
	bResuming = Felmondas::ResumeWorld.IsValid() && Felmondas::ResumeWorld.Get() == GetWorld();
	Felmondas::ResumeWorld = nullptr;

	YardFrame = AGarayYard::Frame(GetWorld());
	Pista = AFleaMarket::FindKeeper(GetWorld(), Felmondas::PistaName);
	PistaAt = Pista
		? Pista->GetActorLocation()
		: FVector(Felmondas::PistaFallback, GroundZ(Felmondas::PistaFallback) + PersonModel::MiddleHeight);
	DoorOutside = YardPoint(GarayModel::Spots::ShopDoorOutside);

	// Schwimmer meets him later behind Pista bácsi's stall, standing as high as him.
	const FVector Back = Pista ? -Pista->GetActorForwardVector() : FVector::ForwardVector;
	const FVector2D Meet2D = FVector2D(PistaAt + Back * Felmondas::MeetBehind);
	// (Pista bácsi's own height: a look down from above could land on a stall's awning.)
	MeetAt = FVector(Meet2D, PistaAt.Z);

	// Schwimmer, by his hut in the timber yard (or at the shop door, trying again).
	const FVector SchwimmerSpot = bResuming
		? DoorOutside + YardFrame.TransformVector(FVector(-40.0, 120.0, 0.0))
		: YardPoint(GarayModel::Spots::Schwimmer);
	const double SchwimmerYaw = GarayModel::CityYaw + 180.0;
	Schwimmer = SpawnForMission<AStoryPerson>(SchwimmerSpot + FVector(0.0, 0.0, PersonModel::MiddleHeight), SchwimmerYaw);
	if (Schwimmer)
	{
		// Schwimmer with his bamboo baton, as the paper tells it.
		PersonKit::FOutfit Dress = GarayModel::Schwimmer();
		Dress.Held = PersonKit::EHeld::Baton;
		Dress.HeldColor = PersonKit::Bamboo();
		Schwimmer->SetOutfit(Dress);
	}
	SpawnShopPeople();

	FMissionStep Talk;
	Talk.GoalHu = TEXT("Besz\u00e9lj Pista b\u00e1csival a Teleki t\u00e9ri piacon");
	Talk.GoalEn = TEXT("Talk to Pista b\u00e1csi at the Teleki t\u00e9r market");
	Talk.Target = PistaAt;
	Talk.Radius = Felmondas::TalkRadius;
	Talk.bPressE = true;
	Steps.Add(Talk);

	FMissionStep Find;
	Find.GoalHu = TEXT("Keresd meg Schwimmer Mih\u00e1lyt a fatelep\u00e9n, Garay utca 13.");
	Find.GoalEn = TEXT("Find Schwimmer Mih\u00e1ly at his timber yard, Garay utca 13");
	Find.Target = SchwimmerSpot;
	Find.Radius = Felmondas::TalkRadius;
	Find.bPressE = true;
	Steps.Add(Find);

	FMissionStep Wait;
	Wait.GoalHu = TEXT("Menj Schwimmerrel Friedrich Hermann boltja el\u00e9");
	Wait.GoalEn = TEXT("Go with Schwimmer to the front of Friedrich Hermann's shop");
	Wait.Target = DoorOutside;
	Wait.Radius = Felmondas::DoorRadius;
	Wait.bManual = true;
	Steps.Add(Wait);

	FMissionStep Hit;
	Hit.GoalHu = TEXT("Menj be a boltba, \u00e9s \u00fcsd meg Friedrich Hermannt (bal eg\u00e9rgomb)");
	Hit.GoalEn = TEXT("Go into the shop and hit Friedrich Hermann (left mouse button)");
	Hit.Target = YardPoint(GarayModel::Spots::Hermann);
	Hit.Radius = Felmondas::CounterRadius;
	Hit.bGuide = false;
	Hit.bManual = true;
	Steps.Add(Hit);

	FMissionStep FightStep;
	FightStep.GoalHu = TEXT("Verekedj meg G\u00e9z\u00e1val \u00e9s a seg\u00e9dekkel");
	FightStep.GoalEn = TEXT("Fight G\u00e9za and the clerks");
	FightStep.Target = YardPoint(GarayModel::Spots::Floor);
	FightStep.Radius = Felmondas::FightRadius;
	FightStep.bGuide = false;
	FightStep.bManual = true;
	Steps.Add(FightStep);

	FMissionStep Away;
	Away.GoalHu = TEXT("R\u00e1zd le a rend\u0151r\u00f6ket");
	Away.GoalEn = TEXT("Lose the police");
	Away.Target = MeetAt;
	Away.Radius = Felmondas::TalkRadius;
	Away.bGuide = false;
	Away.bManual = true;
	Steps.Add(Away);

	FMissionStep MeetStep;
	MeetStep.GoalHu = TEXT("Tal\u00e1lkozz Schwimmerrel a Teleki t\u00e9ri piacon");
	MeetStep.GoalEn = TEXT("Meet Schwimmer at the Teleki t\u00e9r market");
	MeetStep.Target = MeetAt;
	MeetStep.Radius = Felmondas::TalkRadius;
	MeetStep.bPressE = true;
	Steps.Add(MeetStep);

	FMissionStep Read;
	Read.GoalHu = TEXT("M\u00e1snap reggel az \u00fajs\u00e1gban...");
	Read.GoalEn = TEXT("The next morning, in the paper...");
	Read.Target = MeetAt;
	Read.Radius = Felmondas::TalkRadius;
	Read.bGuide = false;
	Read.bManual = true;
	Steps.Add(Read);
}

void AMissionFelmondas::SpawnShopPeople()
{
	struct FPlace
	{
		ERuffian Kind;
		FVector Local;
		double Yaw;
	};
	// Each stands at his place facing into the shop (yaws in the yard's frame).
	const FPlace Places[] = {
		{ ERuffian::Hermann, GarayModel::Spots::Hermann, 135.0 },
		{ ERuffian::Geza, GarayModel::Spots::Geza, 180.0 },
		{ ERuffian::ClerkOne, GarayModel::Spots::ClerkOne, 0.0 },
		{ ERuffian::ClerkTwo, GarayModel::Spots::ClerkTwo, 180.0 },
	};
	for (const FPlace& Place : Places)
	{
		const FVector At = YardPoint(Place.Local) + FVector(0.0, 0.0, PersonModel::MiddleHeight + Felmondas::SpawnLift);
		const FTransform Spot(FRotator(0.0, GarayModel::CityYaw + Place.Yaw, 0.0), At);
		ARuffian* Person = GetWorld()->SpawnActorDeferred<ARuffian>(ARuffian::StaticClass(), Spot, this, nullptr,
			ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
		if (!Person)
		{
			continue;
		}
		Person->Kind = Place.Kind;
		Person->FinishSpawning(Spot);
		ShopPeople.Add(Person);
	}
}

ARuffian* AMissionFelmondas::GetShopPerson(ERuffian Kind) const
{
	for (ARuffian* Person : ShopPeople)
	{
		if (Person && Person->Kind == Kind)
		{
			return Person;
		}
	}
	return nullptr;
}

void AMissionFelmondas::OnStepStarted(int32 Step)
{
	Reached = FMath::Max(Reached, Step);
	APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this);

	// Trying again: straight back to the shop door.
	if (bResuming && Step < Felmondas::WaitNoon)
	{
		FinishStep();
		return;
	}
	if (Step == Felmondas::WaitNoon)
	{
		bResuming = false;
	}
	else if (Step == Felmondas::Fight)
	{
		Say(Felmondas::GezaName,
			TEXT("Ap\u00e1m! Lajos, Bandi, seg\u00edtsetek!"),
			TEXT("Father! Lajos, Bandi, help!"), 5.f);
	}
	else if (Step == Felmondas::Getaway)
	{
		// Out on the street people shout for the police; Schwimmer slips away.
		if (Game)
		{
			ArrestsBefore = Game->GetArrests();
			Game->ReportCrime(2);
		}
		if (Schwimmer)
		{
			Schwimmer->Follow(nullptr);
			Schwimmer->SetInWorld(false);
		}
	}
	else if (Step == Felmondas::Meet)
	{
		// Schwimmer is there before him, behind Pista bácsi's stall.
		if (Schwimmer)
		{
			Schwimmer->SetActorLocation(MeetAt, false, nullptr, ETeleportType::TeleportPhysics);
			Schwimmer->SetInWorld(true);
			Schwimmer->FaceTowards(PistaAt);
		}
	}
}

void AMissionFelmondas::OnStepDone(int32 Step)
{
	APawn* Player = GetPlayer();
	if (bResuming)
	{
		return;
	}
	if (Step == Felmondas::TalkPista)
	{
		if (Pista && Player)
		{
			Pista->FaceTowards(Player->GetActorLocation());
		}
		Say(Felmondas::PistaName,
			TEXT("Fiam, van egy r\u00e9gi cimbor\u00e1m, Schwimmer Mih\u00e1ly, fakeresked\u0151 a Garay utc\u00e1ban."),
			TEXT("Son, I have an old friend, Schwimmer Mih\u00e1ly, a timber merchant on Garay utca."), 7.f);
		SayLater(7.f, Felmondas::PistaName,
			TEXT("A h\u00e1ziura kirakn\u00e1 a telep\u00e9r\u0151l. Elkelne neki egy er\u0151s k\u00e9z. Keresd meg a Garay utca 13-ban!"),
			TEXT("His landlord wants him off the yard. He could use a strong hand. Find him at Garay utca 13!"));
	}
	else if (Step == Felmondas::FindSchwimmer)
	{
		if (Schwimmer && Player)
		{
			Schwimmer->FaceTowards(Player->GetActorLocation());
		}
		Say(Felmondas::SchwimmerName,
			TEXT("Pista b\u00e1csi k\u00fcld\u00f6tt? J\u00f3. Az a Friedrich, a h\u00e1zi\u00far, felmondott nekem, \u00e9s m\u00e9g \u0151 h\u0151zong!"),
			TEXT("Pista b\u00e1csi sent you? Good. That Friedrich, the landlord, gave me notice, and he's the one making a fuss!"), 7.f);
		SayLater(7.f, Felmondas::SchwimmerName,
			TEXT("A s\u00f3gorom, Erd\u0151s, ma nem \u00e9r r\u00e1, h\u00e1t te j\u00f6ssz velem helyette. D\u00e9lben a boltja el\u0151tt v\u00e1runk."),
			TEXT("My brother-in-law Erd\u0151s can't come today, so you come with me in his place. At noon we wait outside his shop."));
		if (Schwimmer)
		{
			Schwimmer->Follow(Player);
		}
		// He hands over a baton like his own: the first weapon.
		if (APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this))
		{
			if (!Game->HasBaton())
			{
				Game->GiveBaton();
				SayLater(7.f, Felmondas::SchwimmerName,
					TEXT("Fogd ezt a n\u00e1dp\u00e1lc\u00e1t, olyan, mint az eny\u00e9m. Ha baj van, j\u00f3 szolg\u00e1latot tesz."),
					TEXT("Take this bamboo baton, same as mine. If there's trouble, it'll serve you well."));
				Announce(TEXT("N\u00c1DP\u00c1LCA"), TEXT("You got a bamboo baton: your blows land harder"), 5.f);
			}
		}
	}
	else if (Step == Felmondas::WaitNoon)
	{
		Announce(TEXT("DELET HARANGOZNAK"), TEXT("Noon. The church bells ring."), 4.f);
		Say(Felmondas::SchwimmerName,
			TEXT("Most! Bent van az \u00f6reg. Menj el\u0151re, \u00e9n j\u00f6v\u00f6k ut\u00e1nad."),
			TEXT("Now! The old man's in. You go first, I'm right behind you."), 6.f);
	}
	else if (Step == Felmondas::HitHermann)
	{
		Lines.Reset();
	}
	else if (Step == Felmondas::Fight)
	{
		Lines.Reset();
		Say(Felmondas::SchwimmerName,
			TEXT("J\u00f6nnek a rend\u0151r\u00f6k! Sz\u00e9tv\u00e1lunk, fiam: a Teleki t\u00e9ren tal\u00e1lkozunk!"),
			TEXT("The police are coming! We split up, son: meet me at Teleki t\u00e9r!"), 6.f);
	}
	else if (Step == Felmondas::Meet)
	{
		if (Schwimmer && Player)
		{
			Schwimmer->FaceTowards(Player->GetActorLocation());
		}
		Say(Felmondas::SchwimmerName,
			TEXT("Meg\u00fasztuk! Az \u00f6reg most k\u00e9tszer is meggondolja. Tess\u00e9k, a f\u00e1radts\u00e1god\u00e9rt."),
			TEXT("We got away with it! The old man will think twice now. Here, for your trouble."), 7.f);
		SayLater(7.f, Felmondas::PistaName,
			TEXT("Csak azt\u00e1n ebb\u0151l baj ne legyen, fiam..."),
			TEXT("Just don't let this bring trouble, son..."));
		ThanksAt = GetWorld()->GetTimeSeconds();
	}
}

void AMissionFelmondas::TickStep(int32 Step, float DeltaSeconds)
{
	const double Now = GetWorld()->GetTimeSeconds();
	// Lines said a little later, in turn.
	if (Lines.Num() > 0 && Now >= Lines[0].At)
	{
		Say(Lines[0].Who, Lines[0].Hu, Lines[0].En, 7.f);
		Lines.RemoveAt(0);
	}

	APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this);
	APestiPerson* Person = Cast<APestiPerson>(GetPlayer());
	const bool bFainted = Game && (Game->IsFainted() || Game->GetHealth() <= 0.f);
	if (bFainted && Step >= Felmondas::HitHermann && Step <= Felmondas::Getaway)
	{
		Fail(TEXT("El\u00e1jult\u00e1l"), TEXT("You fainted"));
		return;
	}

	if (Step == Felmondas::WaitNoon)
	{
		// At the door on foot: Schwimmer stands beside him, and they wait for the bells.
		const bool bThere = Person && !Person->IsRiding() && FVector::Dist2D(Person->GetActorLocation(), DoorOutside) < Felmondas::DoorRadius;
		if (bThere && WaitingSince < 0.0)
		{
			WaitingSince = Now;
			// Whatever the hour he gets here, the bells ring for noon after
			// a short wait: the clock jumps to just before it, behind a
			// moment of black if that is a big jump.
			if (ADayNight* Sky = ADayNight::Get(GetWorld()))
			{
				const float Before = Sky->GetHour();
				Sky->ReachHourIn(12.f, static_cast<float>(Felmondas::NoonAfter));
				const float Jump = FMath::Abs(Sky->GetHour() - Before);
				APlayerController* PC = Cast<APlayerController>(Person->GetController());
				if (FMath::Min(Jump, 24.f - Jump) > Felmondas::FadeJump && PC && PC->PlayerCameraManager)
				{
					PC->PlayerCameraManager->StartCameraFade(1.f, 0.f, Felmondas::NoonFade, FLinearColor::Black);
				}
			}
			if (Schwimmer)
			{
				Schwimmer->Follow(nullptr);
				Schwimmer->FaceTowards(YardPoint(GarayModel::Spots::ShopDoorInside));
			}
			Say(Felmondas::SchwimmerName,
				TEXT("V\u00e1rjunk d\u00e9lig. Akkor m\u00e1r bent lesz az \u00f6reg."),
				TEXT("Let's wait till noon. The old man will be in by then."), 6.f);
			Steps[Felmondas::WaitNoon].GoalHu = TEXT("V\u00e1rj Schwimmerrel d\u00e9lig");
			Steps[Felmondas::WaitNoon].GoalEn = TEXT("Wait with Schwimmer until noon");
		}
		if (WaitingSince >= 0.0 && Now - WaitingSince >= Felmondas::NoonAfter)
		{
			FinishStep();
		}
		return;
	}
	if (Step == Felmondas::HitHermann)
	{
		TickHermann();
		return;
	}
	if (Step == Felmondas::Fight)
	{
		TickFight();
		return;
	}
	if (Step == Felmondas::Getaway)
	{
		if (Game && Game->GetArrests() > ArrestsBefore)
		{
			Fail(TEXT("Elkaptak a rend\u0151r\u00f6k"), TEXT("The police caught you"));
			return;
		}
		if (!Game || Game->GetWantedLevel() == 0)
		{
			FinishStep();
		}
		return;
	}
	if (Step == Felmondas::Paper)
	{
		if (HeadlineAt < 0.0)
		{
			if (Now - ThanksAt >= Felmondas::HeadlineAfter)
			{
				HeadlineAt = Now;
				ShowClipping();
			}
			return;
		}
		const APawn* Player = GetPlayer();
		const APlayerController* PC = Player ? Cast<APlayerController>(Player->GetController()) : nullptr;
		const bool bPutAway = Now - HeadlineAt >= Felmondas::HeadlineMin && PC && PC->WasInputKeyJustPressed(EKeys::E);
		if (bPutAway || Now - HeadlineAt >= Felmondas::HeadlineFor)
		{
			HideClipping();
			FinishStep();
		}
	}
}

void AMissionFelmondas::TickHermann()
{
	ARuffian* Old = GetShopPerson(ERuffian::Hermann);
	APestiPerson* Person = Cast<APestiPerson>(GetPlayer());
	if (!Old)
	{
		FinishStep();
		return;
	}
	if (Old->IsBeaten())
	{
		FinishStep();
		return;
	}
	if (!bHermannUp && Person && !Person->IsRiding()
		&& FVector::Dist2D(Person->GetActorLocation(), Old->GetActorLocation()) < Felmondas::HermannReach)
	{
		// The old man puts his fists up where he stands; he faces him.
		bHermannUp = true;
		Old->Circle(Person, Old->GetActorLocation());
		Person->StartFight(Old);
		Say(Felmondas::HermannName,
			TEXT("Maguk megint? Takarodjanak a boltomb\u00f3l!"),
			TEXT("You again? Get out of my shop!"), 4.f);
	}
}

void AMissionFelmondas::TickFight()
{
	APestiPerson* Person = Cast<APestiPerson>(GetPlayer());
	const FVector Shop = YardPoint(GarayModel::Spots::Floor);
	if (!Person || FVector::Dist2D(Person->GetActorLocation(), Shop) > Felmondas::FightLeft)
	{
		Fail(TEXT("Otthagytad a vereked\u00e9st"), TEXT("You walked out of the fight"));
		return;
	}

	// Who is still on his feet, in the order they go for him.
	TArray<ARuffian*> Standing;
	for (const ERuffian Kind : Felmondas::FightOrder)
	{
		ARuffian* Man = GetShopPerson(Kind);
		if (Man && !Man->IsBeaten())
		{
			Standing.Add(Man);
		}
	}

	// All down: a moment, and they are out of the door.
	if (Standing.Num() == 0)
	{
		const double Now = GetWorld()->GetTimeSeconds();
		if (WonAt < 0.0)
		{
			WonAt = Now;
			if (Person->GetAct() != Brawl::EAct::None)
			{
				Person->EndFight();
			}
		}
		else if (Now - WonAt >= Felmondas::OverAfter)
		{
			FinishStep();
		}
		return;
	}

	// Two go for him; the third stands back with his fists up, waiting.
	const FVector Middle = Person->GetActorLocation();
	for (int32 k = 0; k < Standing.Num(); ++k)
	{
		if (k < Felmondas::AtOnce)
		{
			Standing[k]->Attack(Person, k == 0 ? Felmondas::FirstEager : Felmondas::SecondEager);
		}
		else
		{
			Standing[k]->Circle(Person, Standing[k]->GetActorLocation());
		}
	}

	// He faces the nearest man coming at him, and turns to another who gets much closer.
	ARuffian* Nearest = nullptr;
	double NearestApart = TNumericLimits<double>::Max();
	for (int32 k = 0; k < FMath::Min(Felmondas::AtOnce, Standing.Num()); ++k)
	{
		const double Apart = FVector::Dist2D(Standing[k]->GetActorLocation(), Middle);
		if (Apart < NearestApart)
		{
			NearestApart = Apart;
			Nearest = Standing[k];
		}
	}
	ARuffian* Facing = Cast<ARuffian>(Person->GetFoe());
	const bool bFacingAttacker = Facing && Facing->IsAttacking() && !Facing->IsBeaten();
	if (!Person->IsFighting())
	{
		Person->StartFight(Nearest);
	}
	else if (!bFacingAttacker
		|| (Nearest != Facing && NearestApart < FVector::Dist2D(Facing->GetActorLocation(), Middle) * Felmondas::TurnShare))
	{
		const Brawl::EAct Doing = Person->GetAct();
		// Not in the middle of holding someone.
		if (Doing != Brawl::EAct::Holding && Doing != Brawl::EAct::Knee && Doing != Brawl::EAct::Held)
		{
			Person->SetFoe(Nearest);
		}
	}

	Steps[Felmondas::Fight].GoalHu = FString::Printf(TEXT("Verekedj meg G\u00e9z\u00e1val \u00e9s a seg\u00e9dekkel (%d maradt)"), Standing.Num());
	Steps[Felmondas::Fight].GoalEn = FString::Printf(TEXT("Fight G\u00e9za and the clerks (%d left)"), Standing.Num());
	if (Nearest)
	{
		SetStepTarget(Nearest->GetActorLocation());
	}
}

bool AMissionFelmondas::GetRetrySpot(FVector& OutAt, double& OutYaw) const
{
	if (Reached < Felmondas::HitHermann)
	{
		return false;
	}
	// On the pavement outside the shop, facing its door.
	OutAt = DoorOutside + FVector(0.0, 0.0, PersonModel::MiddleHeight + Felmondas::SpawnLift);
	OutYaw = GarayModel::CityYaw;
	return true;
}

void AMissionFelmondas::ShowClipping()
{
	APestiHUD* Hud = APestiHUD::Get(this);
	if (!Hud)
	{
		return;
	}
	Hud->SetClipping(
		TEXT("Keresked\u0151k v\u00e9res vereked\u00e9se a Garay-utc\u00e1ban"),
		TEXT("Friedrich Hermann 67 \u00e9ves nagykeresked\u0151, a Garay-utca 13. sz\u00e1m\u00fa telek tulajdonosa, felmondott b\u00e9rl\u0151j\u00e9nek, Schwimmer Mih\u00e1ly fakeresked\u0151nek, aki az\u00f3ta t\u00f6bbsz\u00f6r is fenyegette a csal\u00e1dot.\n\n")
		TEXT("Tegnap d\u00e9lben Schwimmer \u00e9s egy ismeretlen fiatalember behatolt Hermann \u00fczlet\u00e9be, \u00e9s \u00fctlegelni kezdt\u00e9k az id\u0151s keresked\u0151t, a seg\u00edts\u00e9g\u00e9re siet\u0151 fi\u00e1t, Friedrich G\u00e9za m\u00e9rn\u00f6k\u00f6t, \u00e9s az alkalmazottakat. A j\u00e1r\u00f3kel\u0151k rend\u0151r\u00e9rt ki\u00e1ltottak, a t\u00e1mad\u00f3k elmenek\u00fcltek.\n\n")
		TEXT("Hermannt a ment\u0151k bek\u00f6t\u00f6zt\u00e9k. S\u00falyos testi s\u00e9rt\u00e9s miatt feljelent\u00e9st tett a f\u0151kapit\u00e1nys\u00e1gon."),
		TEXT("Merchants' bloody brawl in Garay utca.\n\n")
		TEXT("Friedrich Hermann, 67, wholesale merchant and owner of the yard at Garay utca 13, had given notice to his tenant, timber merchant Schwimmer Mih\u00e1ly, who has since threatened the family more than once.\n\n")
		TEXT("Yesterday at noon Schwimmer and an unknown young man burst into Hermann's shop and set upon the old merchant, his son G\u00e9za, an engineer, who rushed to help, and the clerks. Passers-by shouted for the police; the attackers fled.\n\n")
		TEXT("Ambulance men bandaged Hermann. He has reported them at police headquarters for serious bodily harm."));
}

void AMissionFelmondas::HideClipping()
{
	if (APestiHUD* Hud = APestiHUD::Get(this))
	{
		Hud->ClearClipping();
	}
}

void AMissionFelmondas::OnFailed()
{
	HideClipping();
	if (Reached >= Felmondas::HitHermann)
	{
		Felmondas::ResumeWorld = GetWorld();
	}
	if (APestiPerson* Person = Cast<APestiPerson>(GetPlayer()))
	{
		if (Person->GetAct() != Brawl::EAct::None)
		{
			Person->EndFight();
		}
	}
	for (ARuffian* Man : ShopPeople)
	{
		if (IsValid(Man) && Man->IsFighting())
		{
			Man->EndFight();
		}
	}
}

void AMissionFelmondas::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	HideClipping();
	for (ARuffian* Man : ShopPeople)
	{
		if (IsValid(Man))
		{
			Man->Destroy();
		}
	}
	ShopPeople.Reset();
	Super::EndPlay(EndPlayReason);
}

void AMissionFelmondas::SayLater(float Seconds, const FString& Who, const FString& LineHu, const FString& LineEn)
{
	const double Last = Lines.Num() > 0 ? Lines.Last().At : GetWorld()->GetTimeSeconds();
	Lines.Add(FLater{ FMath::Max(Last + Seconds, GetWorld()->GetTimeSeconds() + Seconds), Who, LineHu, LineEn });
}
