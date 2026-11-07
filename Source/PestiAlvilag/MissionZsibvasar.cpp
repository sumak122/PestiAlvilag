// Fill out your copyright notice in the Description page of Project Settings.

#include "MissionZsibvasar.h"

#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "FleaMarket.h"
#include "GameFramework/PlayerController.h"
#include "HorseCart.h"
#include "HorseCartModel.h"
#include "InputCoreTypes.h"
#include "MarketHall.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PersonKit.h"
#include "PersonModel.h"
#include "PestiAlvilagGameModeBase.h"
#include "PestiHud.h"
#include "PestiPerson.h"
#include "ProceduralMeshComponent.h"
#include "Ruffian.h"
#include "StableYard.h"
#include "StableYardModel.h"
#include "StoryPerson.h"

namespace Zsibvasar
{
	// Pista bácsi's junk stall at the flea market, in case the market
	// can't be found (the street furniture file puts him here).
	const FVector2D PistaFallback(101337.0, 30370.0);
	// By the market hall's side door on Vásár utca, in the hall's own
	// frame (AMarketHall::ToCity): the cart stops on the road, the crates
	// stand stacked on the pavement, the porter beside them.
	const FVector2D DoorStopLocal(4300.0, -4560.0);
	const FVector2D StackLocal(4300.0, -4230.0);
	const FVector2D PorterLocal(4050.0, -4180.0);
	// The stack: three crates along the wall, two deep.
	constexpr double StackAlong = 66.0;
	constexpr double StackDeep = 52.0;

	constexpr float TalkRadius = 250.f;
	constexpr float CartRadius = 400.f;
	constexpr float DoorRadius = 700.f;
	constexpr float LoadRadius = 200.f;
	constexpr float DeliverRadius = 1500.f;
	// Near enough to a crate on the ground to pick it up (cm, flat).
	constexpr double PickUpReach = 170.0;
	// The crate in his arms, in his frame (his middle is 90 cm over his feet).
	const FVector HeldAt(45.0, 0.0, -12.0);
	// The marker only moves when its target has moved this far.
	constexpr double MarkerMoves = 40.0;

	// Pista bácsi's bonus (fillér) for all six crates there quickly: the
	// time allowed is the straight way from the hall times this for the
	// turns, at this pace (cm/s, between trot and canter), plus a little (s).
	constexpr int32 BonusPay = 150;
	constexpr double BonusWinding = 1.45;
	constexpr double BonusPace = 400.0;
	constexpr double BonusSlack = 20.0;

	// A one-time tip when the first crate falls off.
	constexpr int32 TipKey = 34;
	constexpr float TipSeconds = 10.f;

	const TCHAR* const PistaName = TEXT("Pista b\u00e1csi");
	const TCHAR* const PorterName = TEXT("Feri, a hord\u00e1r");

	// The ruffians crowd round Pista bácsi (he stands behind his stall,
	// facing it): round his back and sides, at these angles from the way he
	// faces (degrees) and this far from him (cm), in ERuffian order; where
	// a stall or another of them is in the way, a little further round.
	const double RuffianAngles[] = { 180.0, 130.0, -130.0, 160.0 };
	const double RuffianDistances[] = { 125.0, 165.0, 165.0, 245.0 };
	constexpr double TryAround = 30.0;
	constexpr int32 TriesAround = 12;
	constexpr double FurtherOut = 80.0;
	constexpr double KeepFromEachOther = 95.0;
	constexpr float BodyRadius = 40.f;
	constexpr float BodyHalfHeight = 88.f;
	// They go for the player in this order, two at a time; Pöttöm, last, runs.
	const ERuffian FightOrder[] = { ERuffian::Svihak, ERuffian::Csontos, ERuffian::Bika };
	constexpr int32 AtOnce = 2;
	// How keen each is (below 1: thinks slower, no grabbing, no double jabs):
	// the first man in, and the second, who holds back more.
	constexpr float FirstEager = 0.8f;
	constexpr float SecondEager = 0.45f;
	// Those waiting their turn stand round him this far off (cm), at these
	// angles from the way he faces (degrees).
	constexpr double RingRadius = 380.0;
	const double RingAngles[] = { 75.0, -75.0, 125.0, -125.0 };
	// The fight starts with him on foot this close to Pista bácsi (cm);
	// walking this far off leaves it, and they wait for him again.
	constexpr double FightStarts = 700.0;
	constexpr double FightLeft = 3500.0;
	// Another man coming at him this much closer than the one he faces: he turns to him.
	constexpr double TurnShare = 0.6;
	constexpr float FightRadius = 600.f;
	// Pista bácsi thanks him this long after the last one runs (s).
	constexpr double ThanksAfter = 4.0;

	/** A mesh of the mission's own from a model piece, painted. */
	UProceduralMeshComponent* MakeMesh(AActor* InOwner, UMaterialInterface* Material, const PersonKit::FPiece& Piece)
	{
		UProceduralMeshComponent* Mesh = NewObject<UProceduralMeshComponent>(InOwner, NAME_None, RF_Transient);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetCanEverAffectNavigation(false);
		for (int32 i = 0; i < Piece.Sections.Num(); ++i)
		{
			const PersonKit::FSection& Section = Piece.Sections[i];
			Mesh->CreateMeshSection_LinearColor(i, Section.Vertices, Section.Triangles, Section.Normals,
				TArray<FVector2D>(), TArray<FLinearColor>(), TArray<FProcMeshTangent>(), false);
			if (Material)
			{
				UMaterialInstanceDynamic* Paint = UMaterialInstanceDynamic::Create(Material, InOwner);
				Paint->SetVectorParameterValue(TEXT("Color"), Section.Color);
				Mesh->SetMaterial(i, Paint);
			}
		}
		return Mesh;
	}
}

AMissionZsibvasar::AMissionZsibvasar()
{
	// Must match PestiStart::Zsibvasar in the game mode.
	MissionId = TEXT("Zsibvasar2");
	Reward = 760;
	TitleHu = TEXT("A zsibv\u00e1s\u00e1r");
	Number = 4;
	TitleEn = TEXT("The flea market");
}

void AMissionZsibvasar::SetUpMission()
{
	Pista = AFleaMarket::FindKeeper(GetWorld(), Zsibvasar::PistaName);
	PistaAt = Pista
		? Pista->GetActorLocation()
		: FVector(Zsibvasar::PistaFallback, GroundZ(Zsibvasar::PistaFallback) + 90.0);

	// His cart, empty to start with.
	for (TActorIterator<AHorseCart> It(GetWorld()); It; ++It)
	{
		Cart = *It;
		break;
	}
	if (!Cart)
	{
		// No cart about (the game mode leaves it in the yard on Auróra utca): the mission brings one there.
		const FVector2D Yard(AStableYard::ToCity(StableYardModel::Spots::Cart, 0.0));
		const FVector At(Yard, GroundZ(Yard) + 150.0);
		Cart = SpawnForMission<AHorseCart>(At, StableYardModel::CityYaw + StableYardModel::Spots::CartYaw);
		KeepAfterMission(Cart);
	}
	if (Cart)
	{
		Cart->SetCrates(0);
		FVector Stale;
		while (Cart->TakeFallenCrate(Stale))
		{
		}
	}

	// By the side door: the crates and the porter who minds them.
	const double HallYaw = AMarketHall::Yaw();
	const FVector2D Door = AMarketHall::ToCity(Zsibvasar::DoorStopLocal);
	DoorStop = FVector(Door, GroundZ(Door));
	const FVector Along = FRotator(0.0, HallYaw, 0.0).RotateVector(FVector::ForwardVector);
	const FVector Deep = FRotator(0.0, HallYaw, 0.0).RotateVector(FVector::RightVector);
	const FVector2D Stack = AMarketHall::ToCity(Zsibvasar::StackLocal);
	for (int32 c = 0; c < HorseCartModel::MaxCrates; ++c)
	{
		const FVector2D At = Stack
			+ FVector2D(Along * ((c % 3) - 1) * Zsibvasar::StackAlong)
			+ FVector2D(Deep * ((c / 3) - 0.5) * Zsibvasar::StackDeep);
		AddGroundCrate(FVector(At, GroundZ(At, DoorStop.Z)), HallYaw + FMath::FRandRange(-6.0, 6.0));
	}

	const FVector2D PorterSpot = AMarketHall::ToCity(Zsibvasar::PorterLocal);
	Porter = SpawnForMission<AStoryPerson>(FVector(PorterSpot, GroundZ(PorterSpot, DoorStop.Z) + 90.0), HallYaw - 90.0);
	if (Porter)
	{
		PersonKit::FOutfit Heavy;
		Heavy.Height = 172.f;
		Heavy.Build = 1.15f;
		Heavy.Hat = PersonKit::EHat::FlatCap;
		Heavy.Top = PersonKit::ETop::Waistcoat;
		Heavy.bTie = false;
		Heavy.bApron = true;
		Heavy.bMoustache = true;
		Heavy.Waistcoat = FLinearColor(0.16f, 0.12f, 0.09f);
		Heavy.Apron = FLinearColor(0.36f, 0.30f, 0.22f);
		Heavy.HatColor = FLinearColor(0.20f, 0.18f, 0.15f);
		Porter->SetOutfit(Heavy);
	}

	FMissionStep Talk;
	Talk.GoalHu = TEXT("Besz\u00e9lj Pista b\u00e1csival a Teleki t\u00e9ri piacon");
	Talk.GoalEn = TEXT("Talk to Pista b\u00e1csi at the Teleki t\u00e9r market");
	Talk.Target = PistaAt;
	Talk.Radius = Zsibvasar::TalkRadius;
	Talk.bPressE = true;
	Talk.SceneSeconds = 14.f;
	Talk.SceneWith = Pista.Get();
	Steps.Add(Talk);
	bStartsAtRing = true;
	StartLetter = TEXT("P");

	FMissionStep GetOn;
	GetOn.GoalHu = TEXT("\u00dclj fel Pista b\u00e1csi kocsij\u00e1ra az Aur\u00f3ra utcai fuvarosudvarban (F)");
	GetOn.GoalEn = TEXT("Get on Pista b\u00e1csi's cart in the carters' yard on Aur\u00f3ra utca (F)");
	GetOn.Target = Cart ? Cart->GetBenchLocation() : AStableYard::ToCity(StableYardModel::Spots::Cart, 0.0);
	GetOn.Radius = Zsibvasar::CartRadius;
	GetOn.bManual = true;
	Steps.Add(GetOn);

	FMissionStep Drive;
	Drive.GoalHu = TEXT("Hajts a v\u00e1s\u00e1rcsarnok V\u00e1s\u00e1r utcai kapuj\u00e1hoz");
	Drive.GoalEn = TEXT("Drive to the market hall's door on V\u00e1s\u00e1r utca");
	Drive.Target = DoorStop;
	Drive.Radius = Zsibvasar::DoorRadius;
	Drive.bOnFoot = false;
	Drive.bManual = true;
	Steps.Add(Drive);

	FMissionStep Load;
	Load.Target = DoorStop;
	Load.Radius = Zsibvasar::LoadRadius;
	Load.bGuide = false;
	Load.bManual = true;
	Steps.Add(Load);

	DeliverHu = TEXT("Vidd a l\u00e1d\u00e1kat Pista b\u00e1csinak a Teleki t\u00e9rre");
	DeliverEn = TEXT("Take the crates to Pista b\u00e1csi at Teleki t\u00e9r");
	FMissionStep Deliver;
	Deliver.GoalHu = DeliverHu;
	Deliver.GoalEn = DeliverEn;
	Deliver.Target = PistaAt;
	Deliver.Radius = Zsibvasar::DeliverRadius;
	Deliver.bOnFoot = false;
	Deliver.bManual = true;
	Steps.Add(Deliver);

	FMissionStep Defend;
	Defend.GoalHu = TEXT("V\u00e9dd meg Pista b\u00e1csit a jasszokt\u00f3l");
	Defend.GoalEn = TEXT("Protect Pista b\u00e1csi from the ruffians");
	Defend.Target = PistaAt;
	Defend.Radius = Zsibvasar::FightRadius;
	Defend.bGuide = false;
	Defend.bManual = true;
	Defend.SceneSeconds = 8.f;
	Defend.SceneWith = Pista.Get();
	Steps.Add(Defend);
}

void AMissionZsibvasar::OnStepStarted(int32 Step)
{
	// Out of sight while he fetches the crates, the ruffians come to Pista bácsi's stall.
	if (Step == 4 && Ruffians.Num() == 0)
	{
		SpawnRuffians();
	}
	if (Step == 4)
	{
		bBonus = true;
		BonusUntil = GetWorld()->GetTimeSeconds()
			+ FVector::Dist2D(DoorStop, PistaAt) * Zsibvasar::BonusWinding / Zsibvasar::BonusPace + Zsibvasar::BonusSlack;
	}
	if (Step == 3)
	{
		if (Porter)
		{
			if (const APawn* Player = GetPlayer())
			{
				Porter->FaceTowards(Player->GetActorLocation());
			}
		}
		Say(Zsibvasar::PorterName,
			TEXT("Pista b\u00e1csi k\u00fcld\u00f6tt? Ott a hat l\u00e1da. Rakd fel magad, fiam, az \u00e9n h\u00e1tam is f\u00e1j."),
			TEXT("Pista b\u00e1csi sent you? There are the six crates. Load them yourself, son, my back hurts too."), 7.f);
		SayLater(7.f, Zsibvasar::PorterName,
			TEXT("E-vel felveszed, a kocsi h\u00e1tulj\u00e1hoz viszed, E-vel felrakod."),
			TEXT("Pick one up with E, carry it to the back of the cart, put it on with E."));
	}
	UpdateGoal(Step);
}

void AMissionZsibvasar::OnStepDone(int32 Step)
{
	APawn* Player = GetPlayer();
	if (Step == 0)
	{
		if (Pista && Player)
		{
			Pista->FaceTowards(Player->GetActorLocation());
		}
		Say(Zsibvasar::PistaName,
			TEXT("Fiam, megint a der\u00e9kam... Hat l\u00e1da \u00e1ru v\u00e1r r\u00e1m a v\u00e1s\u00e1rcsarnokn\u00e1l, a V\u00e1s\u00e1r utcai kapun\u00e1l."),
			TEXT("Son, my back again... Six crates of goods are waiting for me at the market hall, at the V\u00e1s\u00e1r utca door."), 7.f);
		SayLater(7.f, Zsibvasar::PistaName,
			TEXT("A lovam, Csillag, a kocsival az Aur\u00f3ra utcai fuvarosudvarban \u00e1ll, J\u00f3ska vigy\u00e1z r\u00e1. Hozd el nekem a l\u00e1d\u00e1kat!"),
			TEXT("My horse, Csillag, stands with the cart in the carters' yard on Aur\u00f3ra utca; J\u00f3ska looks after him. Fetch me the crates!"));
		SayLater(7.f, Zsibvasar::PistaName,
			TEXT("\u00c9s ne v\u00e1gtass, mert leesnek a l\u00e1d\u00e1k!"),
			TEXT("And don't gallop, or the crates fall off!"));
	}
	else if (Step == 3)
	{
		Say(Zsibvasar::PorterName,
			TEXT("Na, mind fent van. Lassan hajts, fiam, mert leesnek!"),
			TEXT("Right, they're all on. Drive slowly, son, or they'll fall off!"), 6.f);
		SayLater(6.f, Zsibvasar::PorterName,
			TEXT("Pista b\u00e1csi r\u00e1ad\u00e1st fizet, ha mind a hat gyorsan \u00e9s eg\u00e9szben oda\u00e9r."),
			TEXT("Pista b\u00e1csi pays extra if all six get there quickly and in one piece."));
	}
	else if (Step == 4)
	{
		Lines.Reset();
		if (bBonus && GetWorld()->GetTimeSeconds() <= BonusUntil)
		{
			Reward += Zsibvasar::BonusPay;
			Announce(TEXT("R\u00c1AD\u00c1S"), FString::Printf(TEXT("All six crates, in good time: Pista b\u00e1csi adds %s"),
				*APestiAlvilagGameModeBase::FormatMoney(Zsibvasar::BonusPay)), 4.f);
		}
		bBonus = false;
		if (Pista && Player)
		{
			Pista->FaceTowards(Player->GetActorLocation());
		}
		Say(TEXT("Bika"),
			TEXT("Na, \u00f6reg, hol a hetip\u00e9nz? Vagy sz\u00e9tszedj\u00fck a k\u00f3cer\u00e1jt?"),
			TEXT("Well, old man, where's this week's money? Or shall we take your junk shop apart?"), 6.f);
		SayLater(6.f, Zsibvasar::PistaName,
			TEXT("Fiam, ezek a jasszok minden h\u00e9ten p\u00e9nzt szednek t\u0151lem. Nem tudn\u00e1l tenni valamit?"),
			TEXT("Son, these ruffians take money off me every week. Couldn't you do something?"));
	}
	else if (Step == 5)
	{
		Lines.Reset();
		if (Pista && Player)
		{
			Pista->FaceTowards(Player->GetActorLocation());
		}
		Say(Zsibvasar::PistaName,
			TEXT("K\u00f6sz\u00f6n\u00f6m, fiam! Ezek t\u00f6bbet nem j\u00f6nnek ide. Itt a fuvard\u00edj, meg egy kis r\u00e1ad\u00e1s."),
			TEXT("Thank you, son! They won't come round here again. Here's your fare, and a little extra."), 8.f);
	}
}

void AMissionZsibvasar::TickStep(int32 Step, float DeltaSeconds)
{
	// Lines said a little later, in turn.
	const double Now = GetWorld()->GetTimeSeconds();
	if (Lines.Num() > 0 && Now >= Lines[0].At)
	{
		Say(Lines[0].Who, Lines[0].Hu, Lines[0].En, 7.f);
		Lines.RemoveAt(0);
	}
	if (!Cart)
	{
		return;
	}

	APawn* Player = GetPlayer();
	const bool bOnCart = Player == Cart;
	if (Step == 1 && bOnCart)
	{
		FinishStep();
		return;
	}
	if (Step == 2 && FVector::Dist2D(Cart->GetActorLocation(), DoorStop) < Zsibvasar::DoorRadius)
	{
		FinishStep();
		return;
	}
	if (Step >= 2)
	{
		TickCrates(Step);
	}
	if (Step == 3 && Cart->GetCrates() >= HorseCartModel::MaxCrates)
	{
		FinishStep();
		return;
	}
	if (Step == 5)
	{
		TickFight();
		return;
	}
	if (Step == 4 && bBonus && Now > BonusUntil)
	{
		bBonus = false;
		Announce(TEXT("Lek\u00e9sted a r\u00e1ad\u00e1st"), TEXT("Too slow for Pista b\u00e1csi's bonus"), 3.f);
	}
	if (Step == 4 && Cart->GetCrates() >= HorseCartModel::MaxCrates
		&& FVector::Dist2D(Cart->GetActorLocation(), PistaAt) < Zsibvasar::DeliverRadius)
	{
		FinishStep();
		return;
	}
	UpdateGoal(Step);
}

void AMissionZsibvasar::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopCarrying();
	for (ARuffian* Tough : Ruffians)
	{
		if (IsValid(Tough))
		{
			Tough->Destroy();
		}
	}
	Ruffians.Reset();
	for (UProceduralMeshComponent* Crate : GroundCrates)
	{
		if (Crate)
		{
			Crate->DestroyComponent();
		}
	}
	GroundCrates.Reset();
	Super::EndPlay(EndPlayReason);
}

void AMissionZsibvasar::SayLater(float Seconds, const FString& Who, const FString& LineHu, const FString& LineEn)
{
	const double Last = Lines.Num() > 0 ? Lines.Last().At : GetWorld()->GetTimeSeconds();
	Lines.Add(FLater{ FMath::Max(Last + Seconds, GetWorld()->GetTimeSeconds() + Seconds), Who, LineHu, LineEn });
}

void AMissionZsibvasar::AddGroundCrate(const FVector& At, double Yaw)
{
	PersonKit::FPiece Piece;
	HorseCartModel::BuildCrate(Piece, FVector::ZeroVector);
	UProceduralMeshComponent* Crate = Zsibvasar::MakeMesh(this, BaseMaterial, Piece);
	Crate->SetupAttachment(GetRootComponent());
	Crate->RegisterComponent();
	Crate->SetWorldLocationAndRotation(At, FRotator(0.0, Yaw, 0.0));
	GroundCrates.Add(Crate);
}

void AMissionZsibvasar::TickCrates(int32 Step)
{
	// What fell off the cart lies on the road until someone picks it up.
	FVector Landed;
	while (Cart->TakeFallenCrate(Landed))
	{
		AddGroundCrate(Landed, Cart->GetActorRotation().Yaw + FMath::FRandRange(-40.0, 40.0));
		if (Step == 4 && bBonus)
		{
			bBonus = false;
			Announce(TEXT("Leesett egy l\u00e1da!"), TEXT("A crate fell off! No bonus from Pista b\u00e1csi now"), 3.f);
		}
		else
		{
			Announce(TEXT("Leesett egy l\u00e1da!"), TEXT("A crate fell off!"), 3.f);
		}
		if (!bWarned)
		{
			bWarned = true;
			APestiHUD::Show(this, Zsibvasar::TipKey, Zsibvasar::TipSeconds, FColor(255, 210, 60),
				TEXT("Tipp: ne menj neki semminek, \u00e9s v\u00e1gt\u00e1ban ne kanyarodj \u00e9lesen  (Tip: don't hit things, and don't turn hard at a canter)"));
		}
	}

	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	APawn* Player = GetPlayer();

	// Climbing onto the bench with a crate: it goes on the bed on the way up.
	if (HeldCrate && Player != Carrier)
	{
		if (Player == Cart && Cart->GetCrates() < HorseCartModel::MaxCrates)
		{
			Cart->SetCrates(Cart->GetCrates() + 1);
		}
		else if (IsValid(Carrier))
		{
			// Into a car or a tram: he puts it down where he stood.
			const FVector Feet = Carrier->GetActorLocation();
			AddGroundCrate(FVector(Feet.X, Feet.Y, GroundZ(FVector2D(Feet), Feet.Z - 90.0)), Carrier->GetActorRotation().Yaw);
		}
		StopCarrying();
		return;
	}

	APestiPerson* Walker = Cast<APestiPerson>(Player);
	if (!Walker || !PC || !PC->WasInputKeyJustPressed(EKeys::E))
	{
		return;
	}
	const FVector Feet = Walker->GetActorLocation();
	if (HeldCrate)
	{
		if (Cart->IsByBed(Feet) && Cart->GetCrates() < HorseCartModel::MaxCrates)
		{
			Cart->SetCrates(Cart->GetCrates() + 1);
			StopCarrying();
		}
		return;
	}

	// Picking up the nearest crate in reach.
	int32 Nearest = INDEX_NONE;
	double NearestDistance = Zsibvasar::PickUpReach;
	for (int32 c = 0; c < GroundCrates.Num(); ++c)
	{
		if (!GroundCrates[c])
		{
			continue;
		}
		const double Distance = FVector::Dist2D(GroundCrates[c]->GetComponentLocation(), Feet);
		if (Distance < NearestDistance)
		{
			Nearest = c;
			NearestDistance = Distance;
		}
	}
	if (Nearest == INDEX_NONE)
	{
		return;
	}
	GroundCrates[Nearest]->DestroyComponent();
	GroundCrates.RemoveAt(Nearest);

	PersonKit::FPiece Piece;
	HorseCartModel::BuildCrate(Piece, FVector::ZeroVector, true);
	HeldCrate = Zsibvasar::MakeMesh(this, BaseMaterial, Piece);
	HeldCrate->SetupAttachment(Walker->GetRootComponent());
	HeldCrate->SetRelativeLocation(Zsibvasar::HeldAt);
	HeldCrate->RegisterComponent();
	Carrier = Walker;
	Carrier->SetHoldingInFront(true);
}

void AMissionZsibvasar::StopCarrying()
{
	if (HeldCrate)
	{
		HeldCrate->DestroyComponent();
		HeldCrate = nullptr;
	}
	if (IsValid(Carrier))
	{
		Carrier->SetHoldingInFront(false);
	}
	Carrier = nullptr;
}

void AMissionZsibvasar::UpdateGoal(int32 Step)
{
	if (!Cart || !Steps.IsValidIndex(Step))
	{
		return;
	}
	FMissionStep& Current = Steps[Step];
	const bool bOnCart = GetPlayer() == Cart;
	FVector Want = Current.Target;

	// The nearest crate on the ground, if any.
	FVector Loose = FVector::ZeroVector;
	bool bLoose = false;
	if (const APawn* Player = GetPlayer())
	{
		double Best = TNumericLimits<double>::Max();
		for (const UProceduralMeshComponent* Crate : GroundCrates)
		{
			if (Crate)
			{
				const double Distance = FVector::Dist2D(Crate->GetComponentLocation(), Player->GetActorLocation());
				if (Distance < Best)
				{
					Best = Distance;
					Loose = Crate->GetComponentLocation();
					bLoose = true;
				}
			}
		}
	}

	if (Step == 1)
	{
		Want = Cart->GetBenchLocation();
	}
	else if (Step == 2)
	{
		if (bOnCart)
		{
			Current.GoalHu = TEXT("Hajts a v\u00e1s\u00e1rcsarnok V\u00e1s\u00e1r utcai kapuj\u00e1hoz");
			Current.GoalEn = TEXT("Drive to the market hall's door on V\u00e1s\u00e1r utca");
			Want = DoorStop;
		}
		else
		{
			Current.GoalHu = TEXT("\u00dclj vissza a kocsira (F)");
			Current.GoalEn = TEXT("Get back on the cart (F)");
			Want = Cart->GetBenchLocation();
		}
	}
	else if (Step == 3)
	{
		Current.GoalHu = FString::Printf(TEXT("Rakd fel a l\u00e1d\u00e1kat a kocsira (%d/%d)"), Cart->GetCrates(), HorseCartModel::MaxCrates);
		Current.GoalEn = FString::Printf(TEXT("Load the crates onto the cart (%d/%d)"), Cart->GetCrates(), HorseCartModel::MaxCrates);
		if (HeldCrate)
		{
			Want = Cart->GetTailLocation();
		}
		else if (bLoose)
		{
			Want = Loose;
		}
	}
	else if (Step == 4)
	{
		if (HeldCrate)
		{
			Current.GoalHu = TEXT("Tedd vissza a l\u00e1d\u00e1t a kocsira (E)");
			Current.GoalEn = TEXT("Put the crate back on the cart (E)");
			Want = Cart->GetTailLocation();
		}
		else if (bLoose)
		{
			Current.GoalHu = TEXT("Szedd fel a leesett l\u00e1d\u00e1t (E)");
			Current.GoalEn = TEXT("Pick up the crate that fell off (E)");
			Want = Loose;
		}
		else if (!bOnCart)
		{
			Current.GoalHu = TEXT("\u00dclj vissza a kocsira (F)");
			Current.GoalEn = TEXT("Get back on the cart (F)");
			Want = Cart->GetBenchLocation();
		}
		else if (bBonus)
		{
			// The time left for the bonus, counting down.
			const int32 Left = FMath::Max(0, FMath::CeilToInt(BonusUntil - GetWorld()->GetTimeSeconds()));
			Current.GoalHu = FString::Printf(TEXT("%s  \u00b7  r\u00e1ad\u00e1s m\u00e9g %d:%02d"), *DeliverHu, Left / 60, Left % 60);
			Current.GoalEn = FString::Printf(TEXT("%s  \u00b7  bonus for %d:%02d more"), *DeliverEn, Left / 60, Left % 60);
			Want = PistaAt;
		}
		else
		{
			Current.GoalHu = DeliverHu;
			Current.GoalEn = DeliverEn;
			Want = PistaAt;
		}
		// Close by for the crates, a wide stop for the cart at the market.
		Current.Radius = (HeldCrate || bLoose || !bOnCart) ? Zsibvasar::LoadRadius : Zsibvasar::DeliverRadius;
	}

	if (FVector::Dist(Want, Current.Target) > Zsibvasar::MarkerMoves)
	{
		SetStepTarget(Want);
	}
}

void AMissionZsibvasar::SpawnRuffians()
{
	const double PistaYaw = Pista ? Pista->GetActorRotation().Yaw : 0.0;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(RuffianSpot), false, this);
	if (Pista)
	{
		Params.AddIgnoredActor(Pista);
	}
	const FCollisionShape Body = FCollisionShape::MakeCapsule(Zsibvasar::BodyRadius, Zsibvasar::BodyHalfHeight);
	TArray<FVector2D> Taken;
	for (int32 r = 0; r < static_cast<int32>(UE_ARRAY_COUNT(Zsibvasar::RuffianAngles)); ++r)
	{
		// The first free spot from the one he would like, trying either way round.
		FVector2D Spot = FVector2D::ZeroVector;
		double Z = PistaAt.Z;
		bool bFound = false;
		for (int32 Round = 0; Round < 2 && !bFound; ++Round)
		{
			for (int32 t = 0; t <= Zsibvasar::TriesAround && !bFound; ++t)
			{
				const double Turn = (t % 2 == 0 ? 1.0 : -1.0) * ((t + 1) / 2) * Zsibvasar::TryAround;
				const double Angle = PistaYaw + Zsibvasar::RuffianAngles[r] + Turn;
				const double Distance = Zsibvasar::RuffianDistances[r] + Round * Zsibvasar::FurtherOut;
				const FVector2D Try = FVector2D(PistaAt) + FVector2D(FRotator(0.0, Angle, 0.0).RotateVector(FVector::ForwardVector)) * Distance;
				bool bCrowded = false;
				for (const FVector2D& Other : Taken)
				{
					bCrowded |= FVector2D::Distance(Other, Try) < Zsibvasar::KeepFromEachOther;
				}
				const double TryZ = GroundZ(Try, PistaAt.Z - PersonModel::MiddleHeight) + PersonModel::MiddleHeight + 10.0;
				if (!bCrowded && !GetWorld()->OverlapBlockingTestByChannel(FVector(Try, TryZ), FQuat::Identity, ECC_Pawn, Body, Params))
				{
					Spot = Try;
					Z = TryZ;
					bFound = true;
				}
			}
		}
		if (!bFound)
		{
			// Nowhere clear: where he would like, and the spawn nudges him out.
			Spot = FVector2D(PistaAt) + FVector2D(FRotator(0.0, PistaYaw + Zsibvasar::RuffianAngles[r], 0.0).RotateVector(FVector::ForwardVector)) * Zsibvasar::RuffianDistances[r];
			Z = GroundZ(Spot, PistaAt.Z - PersonModel::MiddleHeight) + PersonModel::MiddleHeight + 10.0;
		}
		Taken.Add(Spot);
		const FVector2D Look = FVector2D(PistaAt) - Spot;
		const double Yaw = FMath::RadiansToDegrees(FMath::Atan2(Look.Y, Look.X));
		const FTransform Place(FRotator(0.0, Yaw, 0.0), FVector(Spot, Z));
		ARuffian* Tough = GetWorld()->SpawnActorDeferred<ARuffian>(ARuffian::StaticClass(), Place, this, nullptr,
			ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
		if (!Tough)
		{
			continue;
		}
		Tough->Kind = static_cast<ERuffian>(r);
		Tough->FinishSpawning(Place);
		Ruffians.Add(Tough);
		RuffianHomes.Add(Place);
		SaidDown.Add(false);
	}
}

ARuffian* AMissionZsibvasar::GetRuffian(ERuffian Kind) const
{
	for (ARuffian* Tough : Ruffians)
	{
		if (Tough && Tough->Kind == Kind)
		{
			return Tough;
		}
	}
	return nullptr;
}

void AMissionZsibvasar::ResetFight()
{
	bFightOn = false;
	if (APestiPerson* Person = Cast<APestiPerson>(GetPlayer()))
	{
		if (Person->GetAct() != Brawl::EAct::None)
		{
			Person->EndFight();
		}
	}
	for (int32 r = 0; r < Ruffians.Num(); ++r)
	{
		if (Ruffians[r])
		{
			Ruffians[r]->StandAgain(RuffianHomes[r].GetLocation(), RuffianHomes[r].Rotator().Yaw);
		}
		SaidDown[r] = false;
	}
	Steps[5].GoalHu = TEXT("Menj vissza, \u00e9s v\u00e9dd meg Pista b\u00e1csit");
	Steps[5].GoalEn = TEXT("Go back and protect Pista b\u00e1csi");
	SetStepTarget(PistaAt);
}

void AMissionZsibvasar::TickFight()
{
	APestiPerson* Person = Cast<APestiPerson>(GetPlayer());
	const APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this);
	ARuffian* Boss = GetRuffian(ERuffian::Bika);
	ARuffian* Little = GetRuffian(ERuffian::Pottom);

	if (!bFightOn)
	{
		// They wait round the stall until he comes over on foot.
		const bool bNear = Person && !Person->IsRiding() && FVector::Dist2D(Person->GetActorLocation(), PistaAt) < Zsibvasar::FightStarts;
		if (!bNear || (Game && Game->IsFainted()))
		{
			return;
		}
		bFightOn = true;
		Lines.Reset();
		if (bFoughtBefore)
		{
			Say(TEXT("Bika"), TEXT("Visszaj\u00f6tt\u00e9l? H\u00e1t nem volt el\u00e9g?"), TEXT("Back again? Wasn't once enough?"), 5.f);
		}
		else
		{
			Say(TEXT("Bika"), TEXT("Ki ez a taknyos? Fi\u00fak, int\u00e9zz\u00e9tek el!"), TEXT("Who's this whelp? Lads, sort him out!"), 5.f);
		}
		bFoughtBefore = true;
		for (ARuffian* Tough : Ruffians)
		{
			if (Tough)
			{
				Tough->Circle(Person, Tough->GetActorLocation());
			}
		}
	}

	// Knocked out (he comes round at home), or walked off: they wait for him again.
	if (!Person || (Game && Game->IsFainted()) || FVector::Dist2D(Person->GetActorLocation(), PistaAt) > Zsibvasar::FightLeft)
	{
		if (Person && !(Game && Game->IsFainted()))
		{
			Say(TEXT("Bika"), TEXT("Na, ez is elszaladt!"), TEXT("Well, off he runs!"), 4.f);
		}
		ResetFight();
		return;
	}

	// Who is still on his feet, in the order they go for him.
	TArray<ARuffian*> Standing;
	for (const ERuffian Kind : Zsibvasar::FightOrder)
	{
		ARuffian* Tough = GetRuffian(Kind);
		if (Tough && !Tough->IsBeaten())
		{
			Standing.Add(Tough);
		}
	}
	// A word from each as he goes down.
	for (int32 r = 0; r < Ruffians.Num(); ++r)
	{
		if (Ruffians[r] && Ruffians[r]->IsBeaten() && !SaidDown[r])
		{
			SaidDown[r] = true;
			if (Ruffians[r] == Boss)
			{
				Say(TEXT("Bika"), TEXT("El\u00e9g, el\u00e9g! Megadom magam!"), TEXT("Enough, enough! I give up!"), 4.f);
			}
		}
	}

	// All three down: the little one runs for it, and it's over.
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
			if (Little)
			{
				Little->Flee(Person);
				Say(TEXT("P\u00f6tt\u00f6m"), TEXT("Megemlegeted ezt! Sz\u00f3lok a nagyb\u00e1ty\u00e1mnak!"), TEXT("You'll pay for this! I'm telling my uncle!"), 4.f);
			}
			Steps[5].GoalHu = TEXT("Elverted a jasszokat");
			Steps[5].GoalEn = TEXT("You beat the ruffians");
		}
		// Let him have his say before Pista bácsi has his.
		else if (Now - WonAt >= Zsibvasar::ThanksAfter)
		{
			FinishStep();
		}
		return;
	}

	// Two go for him; the rest stand round him with their fists up, waiting.
	const double PersonYaw = Person->GetActorRotation().Yaw;
	const FVector Middle = Person->GetActorLocation();
	int32 Round = 0;
	TArray<ARuffian*> Waiting;
	for (int32 k = Zsibvasar::AtOnce; k < Standing.Num(); ++k)
	{
		Waiting.Add(Standing[k]);
	}
	if (Little)
	{
		Waiting.Add(Little);
	}
	for (ARuffian* Tough : Waiting)
	{
		const double Angle = PersonYaw + Zsibvasar::RingAngles[Round % static_cast<int32>(UE_ARRAY_COUNT(Zsibvasar::RingAngles))];
		const FVector Spot = Middle + FRotator(0.0, Angle, 0.0).RotateVector(FVector::ForwardVector) * Zsibvasar::RingRadius;
		Tough->Circle(Person, Spot);
		++Round;
	}
	for (int32 k = 0; k < FMath::Min(Zsibvasar::AtOnce, Standing.Num()); ++k)
	{
		Standing[k]->Attack(Person, k == 0 ? Zsibvasar::FirstEager : Zsibvasar::SecondEager);
	}

	// He faces the nearest man coming at him, and turns to another who gets much closer.
	ARuffian* Nearest = nullptr;
	double NearestApart = TNumericLimits<double>::Max();
	for (int32 k = 0; k < FMath::Min(Zsibvasar::AtOnce, Standing.Num()); ++k)
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
		|| (Nearest != Facing && NearestApart < FVector::Dist2D(Facing->GetActorLocation(), Middle) * Zsibvasar::TurnShare))
	{
		const Brawl::EAct Doing = Person->GetAct();
		// Not in the middle of holding someone.
		if (Doing != Brawl::EAct::Holding && Doing != Brawl::EAct::Knee && Doing != Brawl::EAct::Held)
		{
			Person->SetFoe(Nearest);
		}
	}

	Steps[5].GoalHu = FString::Printf(TEXT("Verd el a jasszokat (%d maradt)"), Standing.Num());
	Steps[5].GoalEn = FString::Printf(TEXT("Beat the ruffians (%d left)"), Standing.Num());
	if (Nearest)
	{
		SetStepTarget(Nearest->GetActorLocation());
	}
}
