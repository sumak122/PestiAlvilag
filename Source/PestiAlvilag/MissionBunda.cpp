// Fill out your copyright notice in the Description page of Project Settings.

#include "MissionBunda.h"

#include "BozsiHouseModel.h"
#include "Camera/PlayerCameraManager.h"
#include "Cutscene.h"
#include "DayNight.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "FleaMarket.h"
#include "FurrierModel.h"
#include "GameFramework/PlayerController.h"
#include "HomeDoor.h"
#include "PersonModel.h"
#include "PestiAlvilagGameModeBase.h"
#include "PestiHud.h"
#include "PestiPerson.h"
#include "PoliceModel.h"
#include "PolicePatrols.h"
#include "ShapeSet.h"
#include "StoryPerson.h"

namespace BundaPlay
{
	// Trying the raid again, after being caught in it, in this world.
	TWeakObjectPtr<UWorld> ResumeWorld;

	const TCHAR* const BozsiName = TEXT("B\u00f6zsi n\u00e9ni");
	const TCHAR* const FurrierName = TEXT("Friedmann");
	const TCHAR* const LadyName = TEXT("Kardosn\u00e9");
	const TCHAR* const ConstableName = TEXT("Rend\u0151r");
	const TCHAR* const VoiceName = TEXT("Hang a kagyl\u00f3ban");

	// Bözsi stands this far along the wall from her door (cm).
	constexpr double BesideDoor = 120.0;
	// The lady stands beside her, a little out on the pavement.
	constexpr double LadyAlong = 110.0;
	constexpr double LadyOut = 60.0;
	constexpr float TalkRadius = 250.f;
	constexpr float GrabRadius = 170.f;
	constexpr float DoorRadius = 150.f;
	// The pavement is measured this far out from the house's and the shop's front (cm).
	constexpr double GroundOut = 150.0;

	// The coat over his arms, in front of his chest (on a bicycle, on his lap).
	const FVector CoatInArms(40.0, 0.0, 14.0);
	const FVector CoatOnLap(34.0, 0.0, 20.0);

	// The raid: how fast the constables run (cm/s), how close counts as caught,
	// how near and how level he must be for them to make straight for him.
	constexpr float RaidSpeed = 500.f;
	constexpr double CatchReach = 130.0;
	constexpr double ChaseNear = 450.0;
	constexpr double ChaseLevel = 80.0;
	// The second constable follows the first this much later (s); the chase waits this long after the scene.
	constexpr double SecondAfter = 2.5;
	constexpr double ChaseDelay = 1.0;
	// Night falls (and morning comes) over this many seconds.
	constexpr float NightHour = 23.f;
	constexpr float MorningHour = 8.f;
	constexpr float ClockSeconds = 2.f;
	constexpr float FadeSeconds = 1.5f;
	// The newspaper stays up this long (s).
	constexpr double PaperSeconds = 14.0;
	// The lady and Bözsi leave the street door this long after the sale (s).
	constexpr double LadyStays = 27.0;
	constexpr int32 StarsForCoat = 1;
	constexpr int32 StarsForRaid = 2;

	constexpr int32 HintKey = 39;
	constexpr float HintSeconds = 6.f;
	constexpr int32 EndKey = 40;
	constexpr float EndSeconds = 12.f;

	// The way the constables run through the house (its own frame, Z the floor):
	// from the gateway across the courtyard, up the stair, along the balconies,
	// over the wash-house roof and down into the back yard.
	const FVector RaidWay[] = {
		BozsiHouseModel::Spots::Gateway,
		BozsiHouseModel::Spots::StairFoot,
		BozsiHouseModel::Spots::StairHead,
		BozsiHouseModel::Spots::GangCorner,
		BozsiHouseModel::Spots::GangEnd,
		FVector(BozsiHouseModel::CourtX1 + 60.0, BozsiHouseModel::Spots::GangEnd.Y, BozsiHouseModel::WashTop),
		BozsiHouseModel::Spots::WashRoof,
		BozsiHouseModel::Spots::Crates,
		BozsiHouseModel::Spots::BackYard,
		BozsiHouseModel::Spots::BackDoor,
	};
	// The third stands guard in the courtyard.
	const FVector GuardAt(1250.0, 0.0, 0.0);
}

AMissionBunda::AMissionBunda()
{
	// Must match PestiStart::Bunda in the game mode.
	MissionId = TEXT("Bunda");
	Reward = 1160;
	TitleHu = TEXT("A bunda");
	TitleEn = TEXT("The fur coat");
	Number = 8;
}

FVector AMissionBunda::HouseAt(const FVector& Local) const
{
	const FRotator Turn(0.0, BozsiHouseModel::CityYaw, 0.0);
	return FVector(BozsiHouseModel::CityAt, HouseGround) + Turn.RotateVector(Local);
}

FVector AMissionBunda::ShopAt(const FVector& Local) const
{
	const FRotator Turn(0.0, FurrierModel::CityYaw, 0.0);
	return FVector(FurrierModel::CityAt, ShopGround) + Turn.RotateVector(Local);
}

void AMissionBunda::SetUpMission()
{
	bResuming = BundaPlay::ResumeWorld.IsValid() && BundaPlay::ResumeWorld.Get() == GetWorld();
	BundaPlay::ResumeWorld = nullptr;

	// The pavement in front of the house and of the shop (not under a roof or an awning).
	const FVector2D HouseOut(HouseAt(FVector(-BundaPlay::GroundOut, 0.0, 0.0)));
	HouseGround = GroundZ(HouseOut, 0.0);
	const FVector2D ShopOut(ShopAt(FVector(FurrierModel::FrontDepth + 400.0, 0.0, 0.0)));
	ShopGround = GroundZ(ShopOut, HouseGround);

	// Bözsi néni beside her street door.
	TActorIterator<AHomeDoor> Door(GetWorld());
	const FVector Stand = Door ? Door->GetStandSpot() : HouseAt(FVector(-BundaPlay::GroundOut, -400.0, 0.0)) + FVector(0.0, 0.0, PersonModel::MiddleHeight);
	BozsiYaw = Door ? Door->GetStandYaw() : BozsiHouseModel::CityYaw + 180.0;
	const FVector Along = FRotator(0.0, BozsiYaw, 0.0).RotateVector(FVector::RightVector);
	const FVector2D BozsiAt = FVector2D(Stand - Along * BundaPlay::BesideDoor);
	BozsiStreet = FVector(BozsiAt, GroundZ(BozsiAt, Stand.Z) + PersonModel::MiddleHeight);

	BozsiAtStall = AFleaMarket::FindKeeper(GetWorld(), BundaPlay::BozsiName);
	if (BozsiAtStall)
	{
		BozsiAtStall->SetInWorld(false);
	}
	Bozsi = SpawnForMission<AStoryPerson>(bResuming ? HouseAt(BozsiHouseModel::Spots::BozsiDoor) + FVector(0.0, 0.0, PersonModel::MiddleHeight) : BozsiStreet, BozsiYaw);
	if (Bozsi)
	{
		if (BozsiAtStall)
		{
			Bozsi->SetOutfit(BozsiAtStall->GetOutfit());
		}
		else
		{
			PersonKit::FOutfit Landlady;
			Landlady.bWoman = true;
			Landlady.Height = 160.f;
			Landlady.Hat = PersonKit::EHat::None;
			Landlady.Top = PersonKit::ETop::Dress;
			Landlady.bTie = false;
			Bozsi->SetOutfit(Landlady);
		}
		if (bResuming)
		{
			Bozsi->SetInWorld(false);
		}
	}

	Whistle = NewObject<UPoliceWhistle>(this);
	Whistle->SetupAttachment(GetRootComponent());
	Whistle->bAllowSpatialization = true;
	Whistle->bOverrideAttenuation = true;
	Whistle->AttenuationOverrides.bAttenuate = true;
	Whistle->AttenuationOverrides.bSpatialize = true;
	Whistle->AttenuationOverrides.FalloffDistance = 6000.f;
	Whistle->RegisterComponent();
	Whistle->SetVolumeMultiplier(0.5f);

	const FVector FlatDoor = HouseAt(BozsiHouseModel::Spots::OnBalcony) + FVector(0.0, 0.0, PersonModel::MiddleHeight);

	if (bResuming)
	{
		// Trying again: back at the house's gateway, then straight into the raid.
		FMissionStep Back;
		Back.GoalHu = TEXT("Menj vissza B\u00f6zsi n\u00e9nihez, N\u00e9psz\u00ednh\u00e1z utca 29.");
		Back.GoalEn = TEXT("Go back to Aunt B\u00f6zsi's house, N\u00e9psz\u00ednh\u00e1z utca 29");
		Back.Target = HouseAt(FVector(-BundaPlay::GroundOut, 0.0, 0.0));
		Back.Radius = BundaPlay::TalkRadius;
		Back.Place = TEXT("N\u00e9psz\u00ednh\u00e1z utca");
		Back.SceneSeconds = 11.f;
		Steps.Add(Back);
		Kinds.Add(EKind::Night);
		bStartsAtRing = true;
		StartLetter = TEXT("B");
	}
	else
	{
		// Friedmann behind his table, and the coat on it.
		Furrier = SpawnForMission<AStoryPerson>(ShopAt(FurrierModel::Spots::Furrier) + FVector(0.0, 0.0, PersonModel::MiddleHeight),
			FurrierModel::CityYaw + FurrierModel::Spots::FurrierYaw);
		if (Furrier)
		{
			Furrier->SetOutfit(FurrierModel::Furrier());
		}
		TableCoat = SpawnForMission<AShapeSet>(ShopAt(FVector::ZeroVector), FurrierModel::CityYaw);
		if (TableCoat)
		{
			TArray<MarketModel::FShape> Folded;
			FurrierModel::FoldedCoat(Folded, FurrierModel::Spots::Coat, 10.0);
			TableCoat->AddShapes(Folded);
		}
		// The lady waits out of sight until the coat is brought.
		const FVector2D LadyAt = FVector2D(BozsiStreet + Along * BundaPlay::LadyAlong + FRotator(0.0, BozsiYaw, 0.0).Vector() * BundaPlay::LadyOut);
		Lady = SpawnForMission<AStoryPerson>(FVector(LadyAt, BozsiStreet.Z), BozsiYaw + 120.0);
		if (Lady)
		{
			Lady->SetOutfit(FurrierModel::Lady(false));
			Lady->SetInWorld(false);
		}

		FMissionStep Talk;
		Talk.GoalHu = TEXT("Besz\u00e9lj B\u00f6zsi n\u00e9nivel a kapu el\u0151tt");
		Talk.GoalEn = TEXT("Talk to Aunt B\u00f6zsi outside the front door");
		Talk.Target = BozsiStreet;
		Talk.Radius = BundaPlay::TalkRadius;
		Talk.bPressE = true;
		Talk.Place = TEXT("N\u00e9psz\u00ednh\u00e1z utca");
		Talk.SceneSeconds = 21.f;
		Talk.SceneWith = Bozsi.Get();
		Steps.Add(Talk);
		Kinds.Add(EKind::Talk);
		bStartsAtRing = true;
		StartLetter = TEXT("B");

		FMissionStep Grab;
		Grab.GoalHu = TEXT("Vidd el a bund\u00e1t Friedmann sz\u0171cs asztal\u00e1r\u00f3l, R\u00e1k\u00f3czi \u00fat");
		Grab.GoalEn = TEXT("Take the fur coat off Friedmann the furrier's table on R\u00e1k\u00f3czi \u00fat");
		Grab.Target = ShopAt(FurrierModel::Spots::Coat);
		Grab.Radius = BundaPlay::GrabRadius;
		Grab.bPressE = true;
		Grab.Place = TEXT("R\u00e1k\u00f3czi \u00fat");
		Steps.Add(Grab);
		Kinds.Add(EKind::Grab);

		FMissionStep Lose;
		Lose.GoalHu = TEXT("R\u00e1zd le a rend\u0151r\u00f6ket");
		Lose.GoalEn = TEXT("Shake off the police");
		Lose.Target = BozsiStreet;
		Lose.bOnFoot = false;
		Lose.bManual = true;
		Lose.bGuide = false;
		Steps.Add(Lose);
		Kinds.Add(EKind::LoseCoat);

		FMissionStep Deliver;
		Deliver.GoalHu = TEXT("Vidd a bund\u00e1t B\u00f6zsi n\u00e9ninek");
		Deliver.GoalEn = TEXT("Take the coat to Aunt B\u00f6zsi");
		Deliver.Target = BozsiStreet;
		Deliver.Radius = BundaPlay::TalkRadius;
		Deliver.bPressE = true;
		Deliver.Place = TEXT("N\u00e9psz\u00ednh\u00e1z utca");
		Deliver.SceneSeconds = 26.f;
		Deliver.SceneWith = Lady.Get();
		Steps.Add(Deliver);
		Kinds.Add(EKind::Deliver);

		FMissionStep Night;
		Night.GoalHu = TEXT("Menj fel B\u00f6zsi n\u00e9ni lak\u00e1s\u00e1ba az udvari gangon, \u00e9s pihenj le");
		Night.GoalEn = TEXT("Go up to Aunt B\u00f6zsi's flat on the courtyard balcony and get some rest");
		Night.Target = FlatDoor;
		Night.Radius = BundaPlay::DoorRadius;
		Night.bPressE = true;
		Night.Place = TEXT("N\u00e9psz\u00ednh\u00e1z utca");
		Night.SceneSeconds = 11.f;
		Night.SceneWith = Bozsi.Get();
		Steps.Add(Night);
		Kinds.Add(EKind::Night);
	}

	FMissionStep Escape;
	Escape.GoalHu = TEXT("Menek\u00fclj! A gangon, a mos\u00f3konyha tetej\u00e9n \u00e1t a h\u00e1ts\u00f3 udvarba, ki a h\u00e1ts\u00f3 kiskapun");
	Escape.GoalEn = TEXT("Run! Along the balcony, over the wash-house roof into the back yard, out of the small back door");
	Escape.Target = HouseAt(BozsiHouseModel::Spots::BackDoor) + FVector(0.0, 0.0, PersonModel::MiddleHeight);
	Escape.Radius = BundaPlay::DoorRadius;
	Escape.bPressE = true;
	Escape.bGuide = false;
	Steps.Add(Escape);
	Kinds.Add(EKind::Escape);

	FMissionStep LoseRaid;
	LoseRaid.GoalHu = TEXT("R\u00e1zd le a rend\u0151r\u00f6ket");
	LoseRaid.GoalEn = TEXT("Shake off the police");
	LoseRaid.Target = FVector(BozsiHouseModel::ExitAt, HouseGround);
	LoseRaid.bOnFoot = false;
	LoseRaid.bManual = true;
	LoseRaid.bGuide = false;
	Steps.Add(LoseRaid);
	Kinds.Add(EKind::LoseRaid);

	const FVector KioskFront = FVector(FurrierModel::KioskAt, GroundZ(FurrierModel::KioskAt, HouseGround))
		+ FRotator(0.0, FurrierModel::KioskYaw, 0.0).RotateVector(FVector(110.0, 0.0, PersonModel::MiddleHeight));
	FMissionStep Phone;
	Phone.GoalHu = TEXT("Cs\u00f6r\u00f6g a telefon a Tisza K\u00e1lm\u00e1n t\u00e9ri f\u00fclk\u00e9ben. Vedd fel!");
	Phone.GoalEn = TEXT("The telephone is ringing in the kiosk on Tisza K\u00e1lm\u00e1n t\u00e9r. Answer it!");
	Phone.Target = KioskFront;
	Phone.Radius = BundaPlay::DoorRadius;
	Phone.bPressE = true;
	Phone.Place = TEXT("Tisza K\u00e1lm\u00e1n t\u00e9r");
	Phone.SceneSeconds = 19.f;
	Steps.Add(Phone);
	Kinds.Add(EKind::Phone);

	const FVector2D AtticFront = BozsiHouseModel::AtticDoorAt + FVector2D(FRotator(0.0, BozsiHouseModel::AtticDoorYaw, 0.0).Vector()) * BundaPlay::GroundOut;
	FMissionStep Room;
	Room.GoalHu = TEXT("Menj az \u00faj szob\u00e1dba, B\u00e9rkocsis utca");
	Room.GoalEn = TEXT("Go to your new room on B\u00e9rkocsis utca");
	Room.Target = FVector(AtticFront, GroundZ(AtticFront, HouseGround) + PersonModel::MiddleHeight);
	Room.Radius = BundaPlay::TalkRadius;
	Room.bPressE = true;
	Room.Place = TEXT("B\u00e9rkocsis utca");
	Steps.Add(Room);
	Kinds.Add(EKind::Room);
}

void AMissionBunda::SayLater(float Seconds, const FString& Who, const FString& LineHu, const FString& LineEn)
{
	// Seconds after the line before it (or after now, if none is waiting).
	const double Now = GetWorld()->GetTimeSeconds();
	const double Last = Lines.Num() > 0 ? Lines.Last().At : Now;
	Lines.Add({ FMath::Max(Last, Now) + Seconds, Who, LineHu, LineEn });
}

void AMissionBunda::Hint(const FString& Text) const
{
	APestiHUD::Show(this, BundaPlay::HintKey, BundaPlay::HintSeconds, FColor(255, 210, 60), Text);
}

void AMissionBunda::PutPlayer(const FVector& At, double Yaw)
{
	APestiPerson* Person = Cast<APestiPerson>(GetPlayer());
	if (!Person)
	{
		return;
	}
	Person->SetActorLocationAndRotation(At, FRotator(0.0, Yaw, 0.0), false, nullptr, ETeleportType::TeleportPhysics);
	if (APlayerController* PC = Cast<APlayerController>(Person->GetController()))
	{
		PC->SetControlRotation(FRotator(-10.0, Yaw, 0.0));
		if (PC->PlayerCameraManager)
		{
			PC->PlayerCameraManager->StartCameraFade(1.f, 0.f, BundaPlay::FadeSeconds, FLinearColor::Black);
		}
	}
}

void AMissionBunda::HoldCoat(bool bHold)
{
	if (CarriedCoat)
	{
		CarriedCoat->Destroy();
		CarriedCoat = nullptr;
	}
	if (IsValid(Carrier))
	{
		Carrier->SetHoldingInFront(false);
	}
	Carrier = nullptr;
	if (!bHold)
	{
		return;
	}
	Carrier = Cast<APestiPerson>(GetPlayer());
	if (!Carrier)
	{
		return;
	}
	CarriedCoat = SpawnForMission<AShapeSet>(Carrier->GetActorLocation(), Carrier->GetActorRotation().Yaw);
	if (!CarriedCoat)
	{
		return;
	}
	TArray<MarketModel::FShape> Bundle;
	FurrierModel::CarriedCoat(Bundle);
	CarriedCoat->AddShapes(Bundle);
	CarriedCoat->AttachToActor(Carrier, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	Carrier->SetHoldingInFront(true);
	PlaceCoat();
}

void AMissionBunda::PlaceCoat()
{
	if (!CarriedCoat || !IsValid(Carrier))
	{
		return;
	}
	CarriedCoat->SetActorRelativeLocation(Carrier->IsRiding() ? BundaPlay::CoatOnLap : BundaPlay::CoatInArms);
	// Out of sight while he sits in a car.
	CarriedCoat->SetActorHiddenInGame(GetPlayer() != Carrier.Get() && !Carrier->IsRiding());
}

bool AMissionBunda::WasArrested() const
{
	const APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this);
	return Game && Game->GetArrests() > ArrestsAtStart;
}

void AMissionBunda::OnStepStarted(int32 Step)
{
	const EKind Kind = KindOf(Step);
	if (static_cast<uint8>(Kind) > static_cast<uint8>(Reached))
	{
		Reached = Kind;
	}
	const double Now = GetWorld()->GetTimeSeconds();
	switch (Kind)
	{
	case EKind::Grab:
		Hint(TEXT("Tipp: E-vel kapd fel a bund\u00e1t, azt\u00e1n fuss!  (Tip: grab the coat with E, then run!)"));
		break;
	case EKind::Deliver:
		if (Lady)
		{
			Lady->SetInWorld(true);
		}
		break;
	case EKind::Escape:
		bChaseOn = false;
		break;
	case EKind::Phone:
	{
		// The next morning: the paper, and the telephone in the kiosk ringing.
		if (ADayNight* Sky = ADayNight::Get(GetWorld()))
		{
			Sky->ReachHourIn(BundaPlay::MorningHour, BundaPlay::ClockSeconds);
		}
		ShowPaper(true);
		PaperUntil = Now + BundaPlay::PaperSeconds;
		const FVector KioskFoot(FurrierModel::KioskAt, GroundZ(FurrierModel::KioskAt, HouseGround));
		Kiosk = SpawnForMission<AShapeSet>(KioskFoot, FurrierModel::KioskYaw);
		if (Kiosk)
		{
			TArray<MarketModel::FShape> Shapes;
			TArray<BozsiHouseModel::FLamp> Lamps;
			FurrierModel::Kiosk(Shapes, Lamps);
			Kiosk->AddShapes(Shapes);
			for (const BozsiHouseModel::FLamp& Lamp : Lamps)
			{
				Kiosk->AddLamp(Lamp.At, Lamp.Radius);
			}
			TArray<BozsiHouseModel::FSign> Signs;
			FurrierModel::KioskSigns(Signs);
			for (const BozsiHouseModel::FSign& Sign : Signs)
			{
				Kiosk->AddSign(Sign.Text, Sign.At, Sign.Yaw, Sign.Height, Sign.Color);
			}
			Steps[Step].SceneWith = Kiosk.Get();
		}
		break;
	}
	default:
		break;
	}
}

void AMissionBunda::OnStepDone(int32 Step)
{
	const APawn* Player = GetPlayer();
	APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this);
	switch (KindOf(Step))
	{
	case EKind::Talk:
		if (Bozsi && Player)
		{
			Bozsi->FaceTowards(Player->GetActorLocation());
		}
		Say(BundaPlay::BozsiName,
			TEXT("Na v\u00e9gre, hogy el\u0151ker\u00fclt\u00e9l. Van egy vev\u0151m, egy \u00fariasszony. Bund\u00e1t akar, de nem boltit, \u00e9s olcs\u00f3n."),
			TEXT("Well, there you are at last. I've a buyer, a fine lady. She wants a fur coat, not from a shop, and cheap."), 7.f);
		SayLater(7.f, BundaPlay::BozsiName,
			TEXT("A R\u00e1k\u00f3czi \u00faton Friedmann sz\u0171cs kint szell\u0151zteti a bund\u00e1kat az asztal\u00e1n. Hozz el egyet, de sz\u00e9pet!"),
			TEXT("On R\u00e1k\u00f3czi \u00fat, Friedmann the furrier airs his furs out on his table. Bring me one, and a nice one!"));
		SayLater(7.f, BundaPlay::BozsiName,
			TEXT("Ha ez \u00f6sszej\u00f6n, megnyitom a saj\u00e1t boltomat a K\u00f6r\u00faton. Nem \u00fcl\u00f6k \u00f6r\u00f6kk\u00e9 ebben a lyukban."),
			TEXT("If this comes off, I'll open my own shop on the K\u00f6r\u00fat. I won't sit in this hole forever."));
		break;

	case EKind::Grab:
	{
		if (TableCoat)
		{
			TableCoat->Destroy();
			TableCoat = nullptr;
		}
		HoldCoat(true);
		ArrestsAtStart = Game ? Game->GetArrests() : 0;
		if (Game)
		{
			Game->ReportCrime(BundaPlay::StarsForCoat);
		}
		if (Furrier)
		{
			if (Player)
			{
				Furrier->FaceTowards(Player->GetActorLocation());
			}
			if (Whistle)
			{
				Whistle->SetWorldLocation(Furrier->GetActorLocation());
				Whistle->Blow();
			}
		}
		Say(BundaPlay::FurrierName, TEXT("Tolvaj! Meg\u00e1llni! Rend\u0151r! Rend\u0151r!"), TEXT("Thief! Stop! Police! Police!"), 4.f);
		break;
	}

	case EKind::LoseCoat:
		Announce(TEXT("Valami neh\u00e9z van a bunda zseb\u00e9ben: egy Frommer Stop pisztoly!"),
			TEXT("Something heavy in the coat's pocket: a Frommer Stop pistol! You keep it."), 6.f);
		break;

	case EKind::Deliver:
	{
		HoldCoat(false);
		if (Lady)
		{
			// She puts it on there and then.
			Lady->SetOutfit(FurrierModel::Lady(true));
			if (Bozsi)
			{
				Lady->FaceTowards(Bozsi->GetActorLocation());
			}
		}
		if (Bozsi && Lady)
		{
			Bozsi->FaceTowards(Lady->GetActorLocation());
		}
		Say(BundaPlay::BozsiName,
			TEXT("N\u00e9zze csak, nagys\u00e1gos asszony! Val\u00f3di nerc, alig hordott."),
			TEXT("Just look, madam! Real mink, hardly worn."), 5.f);
		SayLater(5.f, BundaPlay::LadyName,
			TEXT("K\u00fcl\u00f6n\u00f6s... pont olyan, mint az eny\u00e9m volt. Mennyi?"),
			TEXT("Strange... exactly like the one I had. How much?"));
		SayLater(4.f, BundaPlay::BozsiName,
			TEXT("Mag\u00e1nak csak k\u00e9tsz\u00e1z peng\u0151."),
			TEXT("For you, only two hundred peng\u0151."));
		SayLater(5.f, BundaPlay::LadyName,
			TEXT("Megveszem. Majd a f\u00e9rjem is megn\u00e9zi... alaposan."),
			TEXT("I'll take it. My husband will want a look at it too... a good look."));
		SayLater(6.f, BundaPlay::BozsiName,
			TEXT("Kardos f\u0151fel\u00fcgyel\u0151 feles\u00e9ge volt. Na mindegy, a p\u00e9nz az p\u00e9nz. Gyere fel este, aludj n\u00e1lam."),
			TEXT("That was Inspector Kardos's wife. Well, never mind, money is money. Come up tonight, you can sleep at mine."));
		LadyGoesAt = GetWorld()->GetTimeSeconds() + BundaPlay::LadyStays;
		break;
	}

	case EKind::Night:
		StartRaid();
		Steps[Step].SceneWith = Bozsi.Get();
		if (Bozsi)
		{
			Steps[Step].Target = Bozsi->GetActorLocation();
		}
		Say(BundaPlay::ConstableName, TEXT("Rend\u0151rs\u00e9g! Senki ne mozduljon!"), TEXT("Police! Nobody move!"), 4.f);
		SayLater(4.f, BundaPlay::BozsiName,
			TEXT("J\u00e9zusm\u00e1ria, a bunda! Mit \u00e1llsz ott? Fuss h\u00e1tra, a mos\u00f3konyha tetej\u00e9n \u00e1t!"),
			TEXT("Lord, the coat! What are you standing there for? Run out the back, over the wash-house roof!"));
		break;

	case EKind::Escape:
	{
		EndRaid();
		// Through the next house and out onto the square.
		const FVector2D Out = BozsiHouseModel::ExitAt;
		PutPlayer(FVector(Out, GroundZ(Out, HouseGround) + PersonModel::MiddleHeight + 10.0), BozsiHouseModel::ExitYaw);
		ArrestsAtStart = Game ? Game->GetArrests() : 0;
		if (Game)
		{
			Game->ReportCrime(BundaPlay::StarsForRaid);
		}
		if (Whistle)
		{
			Whistle->SetWorldLocation(HouseAt(BozsiHouseModel::Spots::BackYard));
			Whistle->Blow();
		}
		Hint(TEXT("Kijutott\u00e1l a Tisza K\u00e1lm\u00e1n t\u00e9rre, de a rend\u0151r\u00f6k a nyomodban vannak!  (You're out on Tisza K\u00e1lm\u00e1n t\u00e9r, but the police are after you!)"));
		break;
	}

	case EKind::LoseRaid:
		Hint(TEXT("Ler\u00e1ztad \u0151ket. Kih\u00fazod magad reggelig...  (You've lost them. You lie low till morning...)"));
		break;

	case EKind::Phone:
		ShowPaper(false);
		PaperUntil = -1.0;
		Say(BundaPlay::VoiceName,
			TEXT("Hallottam, mi t\u00f6rt\u00e9nt az \u00f6regasszonnyal. Maga legal\u00e1bb nem bukott le. \u00dcgyes."),
			TEXT("I heard what happened to the old woman. At least you didn't get caught. Well done."), 6.f);
		SayLater(6.f, BundaPlay::VoiceName,
			TEXT("Kib\u00e9reltem mag\u00e1nak egy padl\u00e1sszob\u00e1t a B\u00e9rkocsis utc\u00e1ban. A kulcs a l\u00e1bt\u00f6rl\u0151 alatt, a p\u00e9nze ott v\u00e1r."),
			TEXT("I've taken an attic room for you on B\u00e9rkocsis utca. The key is under the mat, your money is waiting there."));
		SayLater(7.f, BundaPlay::VoiceName,
			TEXT("Majd jelentkezem. Addig ne keressen."),
			TEXT("I'll be in touch. Until then, don't look for me."));
		break;

	case EKind::Room:
		APestiHUD::Show(this, BundaPlay::EndKey, BundaPlay::EndSeconds, FColor(255, 220, 140),
			TEXT("AZ ELS\u0150 FELVON\u00c1S V\u00c9GE  \u00b7  End of act one. Your home is now the attic room: sleep and save at its door."));
		break;

	default:
		break;
	}
}

void AMissionBunda::StartRaid()
{
	// Night falls; he is at Bözsi's door on the balcony, the police come in at the gateway.
	if (ADayNight* Sky = ADayNight::Get(GetWorld()))
	{
		Sky->ReachHourIn(BundaPlay::NightHour, BundaPlay::ClockSeconds);
	}
	HoldCoat(false);
	if (Lady)
	{
		Lady->SetInWorld(false);
	}
	LadyGoesAt = -1.0;
	const FVector OnBalcony = HouseAt(BozsiHouseModel::Spots::OnBalcony) + FVector(0.0, 0.0, PersonModel::MiddleHeight + 10.0);
	PutPlayer(OnBalcony, BozsiHouseModel::CityYaw + BozsiHouseModel::Spots::OnBalconyYaw);
	if (Bozsi)
	{
		const FVector AtDoor = HouseAt(BozsiHouseModel::Spots::BozsiDoor + FVector(0.0, 50.0, 0.0)) + FVector(0.0, 0.0, PersonModel::MiddleHeight);
		Bozsi->SetActorLocation(AtDoor);
		Bozsi->SetInWorld(true);
		Bozsi->FaceTowards(OnBalcony);
	}

	EndRaid();
	FRandomStream Random(1932);
	const double Now = GetWorld()->GetTimeSeconds();
	for (int32 i = 0; i < 3; ++i)
	{
		const bool bGuard = i == 2;
		const FVector Start = bGuard ? BundaPlay::GuardAt : BundaPlay::RaidWay[0] - FVector(80.0 * i, 0.0, 0.0);
		APoliceman* Man = SpawnForMission<APoliceman>(HouseAt(Start) + FVector(0.0, 0.0, PersonModel::MiddleHeight), BozsiHouseModel::CityYaw);
		if (!Man)
		{
			continue;
		}
		Man->PutOnUniform(PoliceModel::Outfit(Random));
		FRaider Raider;
		Raider.Man = Man;
		Raider.Next = 1;
		Raider.StartAt = Now + i * BundaPlay::SecondAfter;
		Raider.bGuard = bGuard;
		Raiders.Add(Raider);
	}
	if (Whistle)
	{
		Whistle->SetWorldLocation(HouseAt(BozsiHouseModel::Spots::Gateway));
		Whistle->Blow();
	}
}

void AMissionBunda::EndRaid()
{
	for (FRaider& Raider : Raiders)
	{
		if (APoliceman* Man = Raider.Man.Get())
		{
			Man->Destroy();
		}
	}
	Raiders.Reset();
	if (Bozsi)
	{
		// Taken away.
		Bozsi->SetInWorld(false);
	}
}

bool AMissionBunda::TickRaiders(float DeltaSeconds)
{
	const APawn* Player = GetPlayer();
	if (!Player)
	{
		return false;
	}
	const double Now = GetWorld()->GetTimeSeconds();
	// The chase waits for the scene to end, and a moment more.
	if (!bChaseOn)
	{
		if (ACutscene::IsPlaying(this))
		{
			for (FRaider& Raider : Raiders)
			{
				Raider.StartAt = Now + BundaPlay::ChaseDelay;
			}
			return false;
		}
		bChaseOn = true;
		for (int32 i = 0; i < Raiders.Num(); ++i)
		{
			Raiders[i].StartAt = Now + BundaPlay::ChaseDelay + i * BundaPlay::SecondAfter;
		}
	}
	const FVector PlayerAt = Player->GetActorLocation();
	const int32 WayCount = static_cast<int32>(UE_ARRAY_COUNT(BundaPlay::RaidWay));
	for (FRaider& Raider : Raiders)
	{
		APoliceman* Man = Raider.Man.Get();
		if (!Man)
		{
			continue;
		}
		const FVector At = Man->GetActorLocation();
		const FVector ToPlayer = PlayerAt - At;
		if (ToPlayer.Size() < BundaPlay::CatchReach)
		{
			return true;
		}
		const double Look = FMath::RadiansToDegrees(FMath::Atan2(ToPlayer.Y, ToPlayer.X));
		if (Raider.bGuard || Now < Raider.StartAt)
		{
			Man->Stand(At, Look);
			continue;
		}
		// Straight for him when he is close by on the same level, along the way through the house otherwise.
		FVector Goal;
		if (ToPlayer.Size2D() < BundaPlay::ChaseNear && FMath::Abs(ToPlayer.Z) < BundaPlay::ChaseLevel)
		{
			Goal = PlayerAt;
		}
		else if (Raider.Next < WayCount)
		{
			Goal = HouseAt(BundaPlay::RaidWay[Raider.Next]) + FVector(0.0, 0.0, PersonModel::MiddleHeight);
		}
		else
		{
			Man->Stand(At, Look);
			continue;
		}
		const FVector ToGoal = Goal - At;
		const double Stride = BundaPlay::RaidSpeed * DeltaSeconds;
		FVector Next = Goal;
		if (ToGoal.Size() > Stride)
		{
			Next = At + ToGoal.GetSafeNormal() * Stride;
		}
		else if (Goal != PlayerAt)
		{
			++Raider.Next;
		}
		const double Yaw = FMath::RadiansToDegrees(FMath::Atan2(ToGoal.Y, ToGoal.X));
		Man->RunTo(Next, Yaw, BundaPlay::RaidSpeed, DeltaSeconds);
	}
	return false;
}

void AMissionBunda::ShowPaper(bool bShow)
{
	APestiHUD* Hud = APestiHUD::Get(this);
	if (!Hud)
	{
		return;
	}
	if (!bShow)
	{
		Hud->ClearClipping();
		return;
	}
	Hud->SetClipping(
		TEXT("Orgazdan\u0151t fogtak el a N\u00e9psz\u00ednh\u00e1z utc\u00e1ban"),
		TEXT("Kardos \u00d6d\u00f6n rend\u0151rfel\u00fcgyel\u0151 neje tegnap d\u00e9lut\u00e1n bund\u00e1t v\u00e1s\u00e1rolt egy N\u00e9psz\u00ednh\u00e1z utcai asszonyt\u00f3l, \u00e9s csod\u00e1lkozva ismert r\u00e1 saj\u00e1t nercbund\u00e1j\u00e1ra, amelyet ny\u00e1rra Friedmann B\u00e9la R\u00e1k\u00f3czi \u00fati sz\u0171csmestern\u00e9l helyezett meg\u0151rz\u00e9sre. A bund\u00e1t aznap d\u00e9lel\u0151tt egy ismeretlen fiatalember lopta el az \u00fczlet el\u0151l.\n\n")
		TEXT("A rend\u0151rs\u00e9g az \u00e9jjel rajta\u00fct\u00f6tt a N\u00e9psz\u00ednh\u00e1z utca 29. sz\u00e1m\u00fa b\u00e9rh\u00e1zon, \u00e9s \u0151rizetbe vette B. Erzs\u00e9bet \u00f6zvegyasszonyt, akit r\u00e9g\u00f3ta orgazdak\u00e9nt ismernek. Tettest\u00e1rsa a h\u00e1ts\u00f3 udvaron \u00e1t elmenek\u00fclt; a nyomoz\u00e1s folyik."),
		TEXT("Receiver of stolen goods caught in N\u00e9psz\u00ednh\u00e1z utca.\n\n")
		TEXT("The wife of police inspector \u00d6d\u00f6n Kardos bought a fur coat yesterday afternoon from a woman in N\u00e9psz\u00ednh\u00e1z utca, and to her astonishment recognised her own mink, which she had left with the furrier B\u00e9la Friedmann of R\u00e1k\u00f3czi \u00fat for the summer. An unknown young man had stolen it from outside the shop that morning.\n\n")
		TEXT("Last night the police raided the tenement at N\u00e9psz\u00ednh\u00e1z utca 29 and took into custody the widow Mrs Erzs\u00e9bet B., long known as a receiver. Her accomplice escaped through the back yard; inquiries continue."));
}

void AMissionBunda::TickStep(int32 Step, float DeltaSeconds)
{
	// Lines said a little later, in turn.
	const double Now = GetWorld()->GetTimeSeconds();
	if (Lines.Num() > 0 && Now >= Lines[0].At)
	{
		Say(Lines[0].Who, Lines[0].Hu, Lines[0].En, 7.f);
		Lines.RemoveAt(0);
	}
	PlaceCoat();
	if (LadyGoesAt > 0.0 && Now >= LadyGoesAt)
	{
		LadyGoesAt = -1.0;
		if (Lady)
		{
			Lady->SetInWorld(false);
		}
		if (Bozsi)
		{
			Bozsi->SetInWorld(false);
		}
	}
	if (PaperUntil > 0.0 && Now >= PaperUntil)
	{
		PaperUntil = -1.0;
		ShowPaper(false);
		Hint(TEXT("Cs\u00f6r\u00f6g a telefon a f\u00fclk\u00e9ben!  (The telephone in the kiosk is ringing!)"));
	}

	const APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this);
	const int32 Wanted = Game ? Game->GetWantedLevel() : 0;
	switch (KindOf(Step))
	{
	case EKind::LoseCoat:
	case EKind::LoseRaid:
		if (WasArrested())
		{
			Fail(TEXT("Elkaptak a rend\u0151r\u00f6k"), TEXT("The police caught you"));
			return;
		}
		if (Wanted == 0)
		{
			FinishStep();
		}
		break;
	case EKind::Escape:
		if (TickRaiders(DeltaSeconds))
		{
			Fail(TEXT("Elkaptak a rend\u0151r\u00f6k"), TEXT("The police caught you"));
		}
		break;
	default:
		break;
	}
}

bool AMissionBunda::GetRetrySpot(FVector& OutAt, double& OutYaw) const
{
	if (static_cast<uint8>(Reached) < static_cast<uint8>(EKind::Night))
	{
		return false;
	}
	// On the pavement outside the house's gateway, facing in.
	OutAt = HouseAt(FVector(-2.0 * BundaPlay::GroundOut, 0.0, 0.0)) + FVector(0.0, 0.0, PersonModel::MiddleHeight + 10.0);
	OutYaw = BozsiHouseModel::CityYaw;
	return true;
}

void AMissionBunda::OnFailed()
{
	Lines.Reset();
	HoldCoat(false);
	ShowPaper(false);
	PaperUntil = -1.0;
	EndRaid();
	if (static_cast<uint8>(Reached) >= static_cast<uint8>(EKind::Night))
	{
		BundaPlay::ResumeWorld = GetWorld();
	}
}

void AMissionBunda::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ShowPaper(false);
	if (IsValid(Carrier))
	{
		Carrier->SetHoldingInFront(false);
	}
	// Bözsi back at her stall, unless she has been taken away.
	const APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this);
	if (IsValid(BozsiAtStall) && !(Game && Game->IsMissionDone(MissionId)) && !BundaPlay::ResumeWorld.IsValid())
	{
		BozsiAtStall->SetInWorld(true);
	}
	Super::EndPlay(EndPlayReason);
}
