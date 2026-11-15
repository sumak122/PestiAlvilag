// Fill out your copyright notice in the Description page of Project Settings.

#include "MissionDzsokiUdvarlo.h"

#include "Brawl.h"
#include "MusicBarModel.h"
#include "MusicBars.h"
#include "PersonModel.h"
#include "PestiAlvilagGameModeBase.h"
#include "PestiHud.h"
#include "PestiPerson.h"
#include "Ruffian.h"
#include "StartRingComponent.h"
#include "StoryPerson.h"

namespace DzsokiUdvarlo
{
	// The pair's way (cm): from just past the tavern west along the street,
	// then north up to the Rózsa kávéház on József körút.
	const FVector2D Route[] = {
		FVector2D(20995.0, 35410.0), FVector2D(18092.0, 36448.0), FVector2D(16075.0, 37377.0), FVector2D(15716.0, 36607.0),
		FVector2D(15082.0, 36907.0), FVector2D(13728.0, 37294.0), FVector2D(6619.0, 38996.0), FVector2D(6053.0, 32340.0),
		FVector2D(4707.0, 13579.0), FVector2D(4867.0, 13566.0) };

	constexpr float TalkRadius = 250.f;
	// They stroll this fast (cm/s), side by side this far apart, each a step behind an unseen pacer (cm).
	constexpr float StrollSpeed = 150.f;
	constexpr double SideApart = 40.0;
	constexpr double PacerAhead = 100.0;
	// The toughs stop them this far along (cm), standing this far ahead of them.
	constexpr double AmbushAt = 18500.0;
	constexpr double ToughsAhead = 550.0;
	const double ToughsAcross[] = { -170.0, 0.0, 170.0 };
	// Following: further than this for this long loses them; closer than
	// this for this long and they see him; a warning closer than that (cm, s).
	constexpr double LostDistance = 4500.0;
	constexpr double LostSeconds = 4.0;
	constexpr double SeenDistance = 380.0;
	constexpr double SeenSeconds = 3.0;
	constexpr double WarnDistance = 700.0;
	constexpr double WarnEvery = 6.0;
	// The fight: it starts with him on foot this close to Gyuszi; walking
	// this far off leaves the pair to it (cm).
	constexpr double FightStarts = 1100.0;
	constexpr double FightLeft = 6000.0;
	constexpr int32 AtOnce = 2;
	constexpr float FirstEager = 0.8f;
	constexpr float SecondEager = 0.45f;
	constexpr double RingRadius = 380.0;
	const double RingAngles[] = { 75.0, -75.0, 125.0, -125.0 };
	constexpr double TurnShare = 0.6;
	// Gyuszi has his say this long after the last one goes down (s).
	constexpr double ThanksAfter = 5.0;
	// Trying again puts the player this far beyond the ring along the wall (cm).
	constexpr double RetryOut = 250.0;

	constexpr int32 TipKey = 38;

	const TCHAR* const DzsokiName = TEXT("Dzsoki");
	const TCHAR* const GyusziName = TEXT("Berki Gyuszi");
	const TCHAR* const RozsiName = TEXT("Rózsi");
	const TCHAR* const ToughName = TEXT("Verőlegény");

	/** The toughs' clothes: flat caps, rough jackets, neckerchiefs. */
	inline PersonKit::FOutfit ToughLook(int32 Index)
	{
		PersonKit::FOutfit O;
		O.Height = 176.f + Index * 6.f;
		O.Build = Index == 1 ? 1.25f : 1.1f;
		O.Hat = Index == 2 ? PersonKit::EHat::Bowler : PersonKit::EHat::FlatCap;
		O.Top = PersonKit::ETop::Jacket;
		O.bTie = false;
		O.bScarf = Index != 2;
		O.bBoots = true;
		O.Beard = PersonKit::EBeard::Stubble;
		O.bMoustache = Index == 1;
		const FLinearColor Coats[] = { FLinearColor(0.16f, 0.12f, 0.08f), FLinearColor(0.08f, 0.09f, 0.08f), FLinearColor(0.10f, 0.07f, 0.06f) };
		O.Coat = Coats[Index % 3];
		O.Trousers = FLinearColor(0.07f, 0.07f, 0.06f);
		O.Scarf = FLinearColor(0.40f, 0.10f, 0.06f);
		O.HatColor = FLinearColor(0.10f, 0.09f, 0.08f);
		return O;
	}
}

AMissionDzsokiUdvarlo::AMissionDzsokiUdvarlo()
{
	MissionId = FName(Id);
	Reward = 1200;
	TitleHu = TEXT("Az udvarló");
	TitleEn = TEXT("The suitor");
	Number = 0;
	bSideStory = true;
}

APestiPerson* AMissionDzsokiUdvarlo::PlayerOnFoot() const
{
	return Cast<APestiPerson>(GetPlayer());
}

FVector2D AMissionDzsokiUdvarlo::PointAlong(double Distance, FVector2D& OutAhead) const
{
	OutAhead = FVector2D(1.0, 0.0);
	if (Way.Num() == 0)
	{
		return FVector2D::ZeroVector;
	}
	double Left = FMath::Max(Distance, 0.0);
	for (int32 i = 0; i + 1 < Way.Num(); ++i)
	{
		const FVector2D Leg = Way[i + 1] - Way[i];
		const double LegLength = Leg.Size();
		if (LegLength < 1.0)
		{
			continue;
		}
		OutAhead = Leg / LegLength;
		if (Left <= LegLength)
		{
			return Way[i] + OutAhead * Left;
		}
		Left -= LegLength;
	}
	return Way.Last();
}

void AMissionDzsokiUdvarlo::SetUpMission()
{
	AMusicBars* Bars = AMusicBars::Find(GetWorld());
	if (Bars)
	{
		// Dzsoki is out here fuming, Rózsi out walking.
		Bars->SetDzsokiOut(true);
		Bars->SetRozsiOut(true);
		GiverAt = Bars->GetGiverSpot();
		GiverYaw = Bars->GetStreetYaw(AMusicBars::BarTavern);
	}
	else
	{
		GiverAt = FVector(DzsokiUdvarlo::Route[0], GroundZ(DzsokiUdvarlo::Route[0]));
	}

	Dzsoki = SpawnForMission<AStoryPerson>(GiverAt + FVector(0.0, 0.0, PersonModel::MiddleHeight), GiverYaw);
	if (Dzsoki)
	{
		Dzsoki->SetOutfit(MusicBarModel::DzsokiStreetLook());
	}

	for (const FVector2D& Point : DzsokiUdvarlo::Route)
	{
		if (Way.Num() > 0)
		{
			WayLength += FVector2D::Distance(Way.Last(), Point);
		}
		Way.Add(Point);
	}

	FMissionStep Talk;
	Talk.GoalHu = TEXT("Beszélj Dzsokival a Fehér Galamb előtt");
	Talk.GoalEn = TEXT("Talk to Dzsoki outside the Fehér Galamb");
	Talk.Target = GiverAt;
	Talk.Radius = DzsokiUdvarlo::TalkRadius;
	Talk.bPressE = true;
	Talk.SceneSeconds = 9.f;
	Talk.SceneWith = Dzsoki.Get();
	Talk.Place = TEXT("Vásár utca");
	Steps.Add(Talk);
	bStartsAtRing = true;
	StartLetter = TEXT("D");

	FMissionStep Follow;
	Follow.GoalHu = TEXT("Kövesd Rózsit és Gyuszit, de ne menj túl közel!");
	Follow.GoalEn = TEXT("Follow Rózsi and Gyuszi, but don't get too close!");
	Follow.Target = FVector(DzsokiUdvarlo::Route[0], GroundZ(DzsokiUdvarlo::Route[0]));
	Follow.bManual = true;
	Follow.bGuide = false;
	Steps.Add(Follow);

	FMissionStep Fight;
	Fight.GoalHu = TEXT("Siess Gyuszi segítségére!");
	Fight.GoalEn = TEXT("Hurry to help Gyuszi!");
	Fight.bManual = true;
	Fight.bGuide = false;
	Steps.Add(Fight);

	FMissionStep Back;
	Back.GoalHu = TEXT("Menj vissza Dzsokihoz a Fehér Galambhoz");
	Back.GoalEn = TEXT("Go back to Dzsoki at the Fehér Galamb");
	Back.Target = GiverAt;
	Back.Radius = DzsokiUdvarlo::TalkRadius;
	Back.bPressE = true;
	Back.SceneSeconds = 9.f;
	Back.SceneWith = Dzsoki.Get();
	Back.Place = TEXT("Vásár utca");
	Steps.Add(Back);
}

void AMissionDzsokiUdvarlo::OnStepStarted(int32 Step)
{
	if (Step == 1)
	{
		// The pair, a little way off down the street already, and their pacers.
		Along = 0.0;
		FVector2D Ahead;
		const FVector2D Start = PointAlong(Along, Ahead);
		const double Yaw = FMath::RadiansToDegrees(FMath::Atan2(Ahead.Y, Ahead.X));
		const FVector2D Across(-Ahead.Y, Ahead.X);
		for (int32 k = 0; k < 2; ++k)
		{
			const FVector2D Spot = Start + Across * (k == 0 ? -DzsokiUdvarlo::SideApart : DzsokiUdvarlo::SideApart);
			AStoryPerson* Walker = SpawnForMission<AStoryPerson>(FVector(Spot, GroundZ(Spot) + PersonModel::MiddleHeight), Yaw);
			AStoryPerson* Pacer = SpawnForMission<AStoryPerson>(FVector(Spot + Ahead * DzsokiUdvarlo::PacerAhead, GroundZ(Spot) + PersonModel::MiddleHeight), Yaw);
			if (!Walker || !Pacer)
			{
				continue;
			}
			Pacer->SetInWorld(false);
			Pacers.Add(Pacer);
			if (k == 0)
			{
				PersonKit::FOutfit Look = MusicBarModel::GyusziLook();
				Look.Hat = PersonKit::EHat::Fedora;
				Look.HatColor = FLinearColor(0.03f, 0.03f, 0.03f);
				Walker->SetOutfit(Look);
				Gyuszi = Walker;
			}
			else
			{
				Walker->SetOutfit(MusicBarModel::RozsiLook());
				Rozsi = Walker;
			}
			Walker->WalkSpeed = DzsokiUdvarlo::StrollSpeed;
			Walker->FollowDistance = static_cast<float>(DzsokiUdvarlo::PacerAhead);
			Walker->Follow(Pacer);
		}
	}
	else if (Step == 2)
	{
		Say(DzsokiUdvarlo::ToughName, TEXT("Hé, hegedűs! Üzenünk valakitől."), TEXT("Hey, fiddler! We've a message for you from someone."), 5.f);
	}
	else if (Step == 3)
	{
		// On they go to the Rózsa, arm in arm.
		for (int32 k = 0; k < Pacers.Num(); ++k)
		{
			AStoryPerson* Walker = k == 0 ? Gyuszi.Get() : Rozsi.Get();
			if (Walker)
			{
				Walker->Follow(Pacers[k]);
			}
		}
	}
}

void AMissionDzsokiUdvarlo::OnStepDone(int32 Step)
{
	APawn* Player = GetPlayer();
	if (Step == 0)
	{
		if (Dzsoki && Player)
		{
			Dzsoki->FaceTowards(Player->GetActorLocation());
		}
		Say(DzsokiUdvarlo::DzsokiName,
			TEXT("Rózsi elment sétálni azzal a Berki Gyuszival! Menj utánuk, fiam, nézd meg, mit akar tőle. De észre ne vegyenek!"),
			TEXT("Rózsi's gone walking with that Berki Gyuszi! Go after them, son, see what he's after. But don't let them see you!"), 9.f);
	}
	else if (Step == 2)
	{
		if (Gyuszi && Player)
		{
			Gyuszi->FaceTowards(Player->GetActorLocation());
		}
		Say(DzsokiUdvarlo::GyusziName,
			TEXT("Köszönöm, barátom. Mondd meg Dzsoki bácsinak: tisztességgel udvarlok a lányának."),
			TEXT("Thank you, my friend. Tell Dzsoki bácsi: I'm courting his daughter honestly."), 6.f);
	}
	else if (Step == 3)
	{
		if (Dzsoki && Player)
		{
			Dzsoki->FaceTowards(Player->GetActorLocation());
		}
		Say(DzsokiUdvarlo::DzsokiName,
			TEXT("A lányom elé állt? Hm... Akkor talán nem egészen hitvány az a fiú. De aranyfog... Csak egy embert ismerek aranyfoggal ebben a szakmában."),
			TEXT("He stood in front of my girl? Hm... Then maybe the boy isn't all bad. But a gold tooth... I know only one man in this trade with a gold tooth."), 9.f);
	}
}

void AMissionDzsokiUdvarlo::TickStep(int32 Step, float DeltaSeconds)
{
	if (Step == 1)
	{
		WalkPair(DeltaSeconds);
		TickFollow();
		if (!HasFailed() && Along >= DzsokiUdvarlo::AmbushAt)
		{
			// Stopped: three men step out in front of them.
			for (AStoryPerson* Walker : { Gyuszi.Get(), Rozsi.Get() })
			{
				if (Walker)
				{
					Walker->Follow(nullptr);
				}
			}
			SpawnToughs();
			FinishStep();
		}
	}
	else if (Step == 2)
	{
		TickFight();
	}
	else if (Step == 3)
	{
		WalkPair(DeltaSeconds);
		// At the café door, and caught up with their pacers: in they go.
		const bool bArrived = Gyuszi && Pacers.Num() > 0 && Pacers[0]
			&& FVector::Dist2D(Gyuszi->GetActorLocation(), Pacers[0]->GetActorLocation()) <= DzsokiUdvarlo::PacerAhead + 20.0;
		if (!bPairGone && Along >= WayLength && bArrived)
		{
			bPairGone = true;
			for (AStoryPerson* Walker : { Gyuszi.Get(), Rozsi.Get() })
			{
				if (Walker)
				{
					Walker->SetInWorld(false);
				}
			}
		}
	}
}

void AMissionDzsokiUdvarlo::WalkPair(float DeltaSeconds)
{
	if (bPairGone || Pacers.Num() < 2)
	{
		return;
	}
	Along = FMath::Min(Along + DzsokiUdvarlo::StrollSpeed * DeltaSeconds, WayLength);
	FVector2D Ahead;
	const FVector2D Middle = PointAlong(Along + DzsokiUdvarlo::PacerAhead, Ahead);
	const FVector2D Across(-Ahead.Y, Ahead.X);
	for (int32 k = 0; k < 2; ++k)
	{
		if (Pacers[k])
		{
			const FVector2D Spot = Middle + Across * (k == 0 ? -DzsokiUdvarlo::SideApart : DzsokiUdvarlo::SideApart);
			Pacers[k]->SetActorLocation(FVector(Spot, Pacers[k]->GetActorLocation().Z));
		}
	}
}

void AMissionDzsokiUdvarlo::TickFollow()
{
	const APawn* Player = GetPlayer();
	if (!Gyuszi || !Player)
	{
		return;
	}
	const double Now = GetWorld()->GetTimeSeconds();
	SetStepTarget(Gyuszi->GetActorLocation() - FVector(0.0, 0.0, PersonModel::MiddleHeight));
	const double Apart = FVector::Dist2D(Player->GetActorLocation(), Gyuszi->GetActorLocation());

	FarSince = Apart > DzsokiUdvarlo::LostDistance ? (FarSince < 0.0 ? Now : FarSince) : -1.0;
	if (FarSince >= 0.0 && Now - FarSince > DzsokiUdvarlo::LostSeconds)
	{
		Fail(TEXT("Szem elől tévesztetted őket"), TEXT("You lost sight of them"));
		return;
	}
	NearSince = Apart < DzsokiUdvarlo::SeenDistance ? (NearSince < 0.0 ? Now : NearSince) : -1.0;
	if (NearSince >= 0.0 && Now - NearSince > DzsokiUdvarlo::SeenSeconds)
	{
		Say(DzsokiUdvarlo::RozsiName, TEXT("Te meg mit leskelődsz utánunk? Apa küldött, ugye?"), TEXT("And what are you sneaking after us for? Father sent you, didn't he?"), 5.f);
		Fail(TEXT("Észrevettek"), TEXT("They saw you"));
		return;
	}
	if (Apart < DzsokiUdvarlo::WarnDistance && Now - WarnedAt > DzsokiUdvarlo::WarnEvery)
	{
		WarnedAt = Now;
		APestiHUD::Show(this, DzsokiUdvarlo::TipKey, 3.f, FColor(255, 200, 60), TEXT("Túl közel vagy, maradj le!  (Too close, hang back!)"));
	}
}

void AMissionDzsokiUdvarlo::SpawnToughs()
{
	FVector2D Ahead;
	const FVector2D Middle = PointAlong(Along + DzsokiUdvarlo::ToughsAhead, Ahead);
	const FVector2D Across(-Ahead.Y, Ahead.X);
	const double Yaw = FMath::RadiansToDegrees(FMath::Atan2(-Ahead.Y, -Ahead.X));
	for (int32 k = 0; k < static_cast<int32>(UE_ARRAY_COUNT(DzsokiUdvarlo::ToughsAcross)); ++k)
	{
		const FVector2D Spot = Middle + Across * DzsokiUdvarlo::ToughsAcross[k] - Ahead * (k == 1 ? 60.0 : 0.0);
		const FTransform Place(FRotator(0.0, Yaw, 0.0), FVector(Spot, GroundZ(Spot) + PersonModel::MiddleHeight + 10.0));
		ARuffian* Tough = GetWorld()->SpawnActorDeferred<ARuffian>(ARuffian::StaticClass(), Place, this, nullptr,
			ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
		if (!Tough)
		{
			continue;
		}
		Tough->Kind = ERuffian::Hired;
		Tough->StreetDress = DzsokiUdvarlo::ToughLook(k);
		Tough->FinishSpawning(Place);
		AddMissionActor(Tough);
		Toughs.Add(Tough);
	}
	// Gyuszi steps in front of Rózsi, facing them.
	if (Gyuszi)
	{
		Gyuszi->FaceTowards(FVector(Middle, Gyuszi->GetActorLocation().Z));
	}
	if (Rozsi)
	{
		Rozsi->FaceTowards(FVector(Middle, Rozsi->GetActorLocation().Z));
	}
}

void AMissionDzsokiUdvarlo::TickFight()
{
	APestiPerson* Person = PlayerOnFoot();
	const APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this);
	const double Now = GetWorld()->GetTimeSeconds();
	if (!Gyuszi)
	{
		return;
	}
	SetStepTarget(Gyuszi->GetActorLocation() - FVector(0.0, 0.0, PersonModel::MiddleHeight));
	const double FromPair = Person ? FVector::Dist2D(Person->GetActorLocation(), Gyuszi->GetActorLocation()) : TNumericLimits<double>::Max();
	const APawn* Player = GetPlayer();
	const double PlayerFromPair = Player ? FVector::Dist2D(Player->GetActorLocation(), Gyuszi->GetActorLocation()) : TNumericLimits<double>::Max();

	if (Game && (Game->IsFainted() || Game->GetHealth() <= 0.f))
	{
		Fail(TEXT("Kiütöttek"), TEXT("You were knocked out"));
		return;
	}
	if (PlayerFromPair > DzsokiUdvarlo::FightLeft)
	{
		Fail(TEXT("Magukra hagytad őket"), TEXT("You left them to it"));
		return;
	}

	if (!bFightOn)
	{
		// The toughs wait for him to come up, squaring up to Gyuszi.
		if (!Person || Person->IsRiding() || FromPair > DzsokiUdvarlo::FightStarts)
		{
			return;
		}
		bFightOn = true;
		Say(DzsokiUdvarlo::ToughName, TEXT("Ki ez? Fiúk, ezt is intézzétek el!"), TEXT("Who's this? Lads, sort him out too!"), 4.f);
		Steps[2].GoalHu = TEXT("Verd el a verőlegényeket");
		Steps[2].GoalEn = TEXT("Beat the toughs");
		for (ARuffian* Tough : Toughs)
		{
			if (Tough)
			{
				Tough->Circle(Person, Tough->GetActorLocation());
			}
		}
	}
	if (!Person)
	{
		return;
	}

	TArray<ARuffian*> Standing;
	for (ARuffian* Tough : Toughs)
	{
		if (Tough && !Tough->IsBeaten())
		{
			Standing.Add(Tough);
		}
	}

	// All down: the last one talks, and it's over.
	if (Standing.Num() == 0)
	{
		if (WonAt < 0.0)
		{
			WonAt = Now;
			if (Person->GetAct() != Brawl::EAct::None)
			{
				Person->EndFight();
			}
			Say(DzsokiUdvarlo::ToughName,
				TEXT("Elég! Egy aranyfogú úr fizetett minket... ugyanaz, aki a kölyökkel az ütőket is elvitette!"),
				TEXT("Enough! A man with a gold tooth paid us... the same one who had the lad take the hammers!"), 6.f);
			Steps[2].GoalHu = TEXT("Elverted a verőlegényeket");
			Steps[2].GoalEn = TEXT("You beat the toughs");
		}
		else if (Now - WonAt >= DzsokiUdvarlo::ThanksAfter)
		{
			FinishStep();
		}
		return;
	}

	// Two go for him; the rest stand round him with their fists up.
	const double PersonYaw = Person->GetActorRotation().Yaw;
	const FVector Middle = Person->GetActorLocation();
	for (int32 k = DzsokiUdvarlo::AtOnce; k < Standing.Num(); ++k)
	{
		const int32 Round = k - DzsokiUdvarlo::AtOnce;
		const double Angle = PersonYaw + DzsokiUdvarlo::RingAngles[Round % static_cast<int32>(UE_ARRAY_COUNT(DzsokiUdvarlo::RingAngles))];
		Standing[k]->Circle(Person, Middle + FRotator(0.0, Angle, 0.0).RotateVector(FVector::ForwardVector) * DzsokiUdvarlo::RingRadius);
	}
	ARuffian* Nearest = nullptr;
	double NearestApart = TNumericLimits<double>::Max();
	for (int32 k = 0; k < FMath::Min(DzsokiUdvarlo::AtOnce, Standing.Num()); ++k)
	{
		Standing[k]->Attack(Person, k == 0 ? DzsokiUdvarlo::FirstEager : DzsokiUdvarlo::SecondEager);
		const double Apart = FVector::Dist2D(Standing[k]->GetActorLocation(), Middle);
		if (Apart < NearestApart)
		{
			NearestApart = Apart;
			Nearest = Standing[k];
		}
	}

	// He faces the nearest man coming at him, and turns to another who gets much closer.
	ARuffian* Facing = Cast<ARuffian>(Person->GetFoe());
	const bool bFacingAttacker = Facing && Facing->IsAttacking() && !Facing->IsBeaten();
	if (!Person->IsFighting())
	{
		Person->StartFight(Nearest);
	}
	else if (Nearest && (!bFacingAttacker
		|| (Nearest != Facing && NearestApart < FVector::Dist2D(Facing->GetActorLocation(), Middle) * DzsokiUdvarlo::TurnShare)))
	{
		const Brawl::EAct Doing = Person->GetAct();
		if (Doing != Brawl::EAct::Holding && Doing != Brawl::EAct::Knee && Doing != Brawl::EAct::Held)
		{
			Person->SetFoe(Nearest);
		}
	}
	Steps[2].GoalHu = FString::Printf(TEXT("Verd el a verőlegényeket (%d maradt)"), Standing.Num());
	Steps[2].GoalEn = FString::Printf(TEXT("Beat the toughs (%d left)"), Standing.Num());

	// Gyuszi keeps himself between them and Rózsi, watching the fight.
	Gyuszi->FaceTowards(Middle);
}

void AMissionDzsokiUdvarlo::OnFailed()
{
	if (APestiPerson* Person = PlayerOnFoot())
	{
		if (Person->GetAct() != Brawl::EAct::None)
		{
			Person->EndFight();
		}
	}
	for (ARuffian* Tough : Toughs)
	{
		if (Tough && Tough->IsFighting())
		{
			Tough->EndFight();
		}
	}
}

bool AMissionDzsokiUdvarlo::GetRetrySpot(FVector& OutAt, double& OutYaw) const
{
	// Along the wall beyond Dzsoki's ring, looking at him; walking in starts it again.
	const FVector Along2 = FRotator(0.0, GiverYaw, 0.0).RotateVector(FVector(0.0, 1.0, 0.0));
	const FVector2D Spot = FVector2D(GiverAt + Along2 * (UStartRingComponent::Radius() + DzsokiUdvarlo::RetryOut));
	OutAt = FVector(Spot, GroundZ(Spot, GiverAt.Z) + PersonModel::MiddleHeight + 10.0);
	OutYaw = GiverYaw - 90.0;
	return true;
}

void AMissionDzsokiUdvarlo::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (APestiPerson* Person = PlayerOnFoot())
	{
		if (Person->GetAct() != Brawl::EAct::None)
		{
			Person->EndFight();
		}
	}
	// Back inside, both of them.
	if (AMusicBars* Bars = AMusicBars::Find(GetWorld()))
	{
		Bars->SetDzsokiOut(false);
		Bars->SetRozsiOut(false);
	}
	Super::EndPlay(EndPlayReason);
}
