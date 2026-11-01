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
#include "PestiHud.h"
#include "PestiPerson.h"
#include "ProceduralMeshComponent.h"
#include "StoryPerson.h"

namespace Zsibvasar
{
	// Pista bácsi's junk stall at the flea market, in case the market
	// can't be found (the street furniture file puts him here).
	const FVector2D PistaFallback(101337.0, 30370.0);
	// Where the game mode leaves his cart, by Tisza Kálmán tér, in case it
	// isn't there: then the mission brings one.
	const FVector2D CartFallback(53292.0, 17629.0);
	constexpr double CartFallbackYaw = 159.2;

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

	// A one-time tip when the first crate falls off.
	constexpr int32 TipKey = 34;
	constexpr float TipSeconds = 10.f;

	const TCHAR* const PistaName = TEXT("Pista b\u00e1csi");
	const TCHAR* const PorterName = TEXT("Feri, a hord\u00e1r");

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
	MissionId = TEXT("Zsibvasar");
	Reward = 760;
	TitleHu = TEXT("A zsibv\u00e1s\u00e1r");
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
		const FVector At(Zsibvasar::CartFallback, GroundZ(Zsibvasar::CartFallback) + 150.0);
		Cart = SpawnForMission<AHorseCart>(At, Zsibvasar::CartFallbackYaw);
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
	Steps.Add(Talk);

	FMissionStep GetOn;
	GetOn.GoalHu = TEXT("\u00dclj fel Pista b\u00e1csi kocsij\u00e1ra a Tisza K\u00e1lm\u00e1n t\u00e9rn\u00e9l (F)");
	GetOn.GoalEn = TEXT("Get on Pista b\u00e1csi's cart by Tisza K\u00e1lm\u00e1n t\u00e9r (F)");
	GetOn.Target = Cart ? Cart->GetBenchLocation() : FVector(Zsibvasar::CartFallback, 0.0);
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
}

void AMissionZsibvasar::OnStepStarted(int32 Step)
{
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
			TEXT("A lovam, Csillag, a kocsival a Tisza K\u00e1lm\u00e1n t\u00e9rn\u00e9l \u00e1ll. Hozd el nekem a l\u00e1d\u00e1kat!"),
			TEXT("My horse, Csillag, stands with the cart by Tisza K\u00e1lm\u00e1n t\u00e9r. Fetch me the crates!"));
		SayLater(7.f, Zsibvasar::PistaName,
			TEXT("\u00c9s ne v\u00e1gtass, mert leesnek a l\u00e1d\u00e1k!"),
			TEXT("And don't gallop, or the crates fall off!"));
	}
	else if (Step == 3)
	{
		Say(Zsibvasar::PorterName,
			TEXT("Na, mind fent van. Lassan hajts, fiam, mert leesnek!"),
			TEXT("Right, they're all on. Drive slowly, son, or they'll fall off!"), 6.f);
	}
	else if (Step == 4)
	{
		Lines.Reset();
		if (Pista && Player)
		{
			Pista->FaceTowards(Player->GetActorLocation());
		}
		Say(Zsibvasar::PistaName,
			TEXT("Mind a hat? Der\u00e9k fi\u00fa vagy! Itt a fuvard\u00edj."),
			TEXT("All six? Good lad! Here's your fare."), 7.f);
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
		Announce(TEXT("Leesett egy l\u00e1da!"), TEXT("A crate fell off!"), 3.f);
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
