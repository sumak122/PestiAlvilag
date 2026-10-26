// Fill out your copyright notice in the Description page of Project Settings.

#include "MissionUzenet.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "FleaMarket.h"
#include "HomeDoor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PestiHud.h"
#include "PestiPerson.h"
#include "StoryPerson.h"
#include "UObject/ConstructorHelpers.h"

namespace Uzenet
{
	// Bözsi néni waits on the pavement a few steps along from her door,
	// on the side away from the café.
	constexpr double BesideDoor = 550.0;

	// Pista bácsi's junk stall at the flea market, in case the market
	// can't be found (the street furniture file puts him here).
	const FVector2D PistaAt(101337.0, 30370.0);

	constexpr float TalkRadius = 250.f;

	// Where things go on him, in his own frame (his middle is 90 cm over
	// his feet; +X ahead, +Y to his right). The letter: its top out of the
	// left breast pocket of his overcoat, which turns a little toward his
	// side there. The parcel: held out between his hands, or on his lap
	// when he sits on the tram.
	const FVector PocketAt(13.9, -9.0, 36.0);
	constexpr double PocketYaw = -20.0;
	const FVector ParcelAt(40.0, 0.0, 18.0);
	const FVector ParcelOnLap(32.0, 0.0, 21.0);
	const FVector ParcelSize(26.0, 44.0, 16.0);
	constexpr double TwineWidth = 0.8;
	const FLinearColor Envelope(0.85f, 0.82f, 0.74f);
	const FLinearColor PocketWelt(0.21f, 0.18f, 0.14f);
	const FLinearColor BrownPaper(0.42f, 0.30f, 0.18f);
	const FLinearColor Twine(0.62f, 0.54f, 0.38f);
	const FLinearColor Wax(0.55f, 0.10f, 0.08f);

	// A one-time hint about the tram, under the goal.
	constexpr int32 HintKey = 33;
	constexpr float HintSeconds = 12.f;

	const TCHAR* const BozsiName = TEXT("B\u00f6zsi n\u00e9ni");
	const TCHAR* const PistaName = TEXT("Pista b\u00e1csi");
}

AMissionUzenet::AMissionUzenet()
{
	MissionId = TEXT("Uzenet");
	Reward = 580;
	TitleHu = TEXT("\u00dczenet");
	TitleEn = TEXT("The message");

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	CubeMesh = Cube.Object;
}

void AMissionUzenet::SetUpMission()
{
	// Bözsi néni comes home from the market for this.
	BozsiAtStall = AFleaMarket::FindKeeper(GetWorld(), Uzenet::BozsiName);
	if (BozsiAtStall)
	{
		BozsiAtStall->SetInWorld(false);
	}

	TActorIterator<AHomeDoor> Door(GetWorld());
	const FVector Stand = Door ? Door->GetStandSpot() : FVector::ZeroVector;
	const double DoorYaw = Door ? Door->GetStandYaw() : 0.0;
	const FVector Along = FRotator(0.0, DoorYaw, 0.0).RotateVector(FVector::RightVector);
	const FVector2D BozsiAt = FVector2D(Stand - Along * Uzenet::BesideDoor);
	const FVector BozsiLocation(BozsiAt, GroundZ(BozsiAt, Stand.Z) + 90.0);
	Bozsi = SpawnForMission<AStoryPerson>(BozsiLocation, DoorYaw);
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
	}

	Pista = AFleaMarket::FindKeeper(GetWorld(), Uzenet::PistaName);
	const FVector PistaLocation = Pista
		? Pista->GetActorLocation()
		: FVector(Uzenet::PistaAt, GroundZ(Uzenet::PistaAt) + 90.0);

	FMissionStep Talk;
	Talk.GoalHu = TEXT("Besz\u00e9lj B\u00f6zsi n\u00e9nivel a kapu el\u0151tt");
	Talk.GoalEn = TEXT("Talk to Aunt B\u00f6zsi outside the front door");
	Talk.Target = BozsiLocation;
	Talk.Radius = Uzenet::TalkRadius;
	Talk.bPressE = true;
	Steps.Add(Talk);

	FMissionStep Letter;
	Letter.GoalHu = TEXT("Vidd el a levelet Pista b\u00e1csinak a Teleki t\u00e9ri piacra");
	Letter.GoalEn = TEXT("Take the letter to Pista b\u00e1csi at the Teleki t\u00e9r market");
	Letter.Target = PistaLocation;
	Letter.Radius = Uzenet::TalkRadius;
	Letter.bPressE = true;
	Steps.Add(Letter);

	FMissionStep Back;
	Back.GoalHu = TEXT("Vidd vissza a csomagot B\u00f6zsi n\u00e9ninek");
	Back.GoalEn = TEXT("Take the parcel back to Aunt B\u00f6zsi");
	Back.Target = BozsiLocation;
	Back.Radius = Uzenet::TalkRadius;
	Back.bPressE = true;
	Steps.Add(Back);
}

void AMissionUzenet::OnStepStarted(int32 Step)
{
	if (Step == 1)
	{
		APestiHUD::Show(this, Uzenet::HintKey, Uzenet::HintSeconds, FColor(255, 210, 60),
			TEXT("Tipp: a meg\u00e1ll\u00f3ban T-vel felsz\u00e1llhatsz a villamosra  (Tip: press T at a stop to ride the tram)"));
	}
}

void AMissionUzenet::OnStepDone(int32 Step)
{
	APawn* Player = GetPlayer();
	if (Step == 0)
	{
		if (Bozsi && Player)
		{
			Bozsi->FaceTowards(Player->GetActorLocation());
		}
		Carrier = Cast<APestiPerson>(Player);
		PocketLetter();
		Say(Uzenet::BozsiName,
			TEXT("J\u00f3kor j\u00f6ssz, fiam. Vidd el ezt a levelet Pista b\u00e1csinak a Teleki t\u00e9ri zsibv\u00e1s\u00e1rra."),
			TEXT("Good timing, son. Take this letter to Pista b\u00e1csi at the Teleki t\u00e9r flea market."), 7.f);
		SayLater(7.f, Uzenet::BozsiName,
			TEXT("Ad majd egy csomagot, azt hozd vissza nekem. A villamos gyorsabb, a meg\u00e1ll\u00f3 itt van a h\u00e1z el\u0151tt."),
			TEXT("He'll give you a parcel; bring it back to me. The tram is quicker, the stop is right outside the house."));
	}
	else if (Step == 1)
	{
		if (Pista && Player)
		{
			Pista->FaceTowards(Player->GetActorLocation());
		}
		Carrier = Cast<APestiPerson>(Player);
		HoldParcel();
		Say(Uzenet::PistaName, TEXT("B\u00f6zsit\u0151l? L\u00e1ssuk csak..."), TEXT("From B\u00f6zsi? Let's see..."), 4.f);
		SayLater(4.f, Uzenet::PistaName,
			TEXT("Na, ezt vidd vissza neki. \u00c9s bele ne n\u00e9zz, fiam!"),
			TEXT("Right, take this back to her. And don't you look inside, son!"));
		SayLater(10.f, Uzenet::PistaName,
			TEXT("Furcsa... nem is hallottalak j\u00f6nni."),
			TEXT("Funny... I never heard you coming."));
	}
	else if (Step == 2)
	{
		if (Bozsi && Player)
		{
			Bozsi->FaceTowards(Player->GetActorLocation());
		}
		PutAway();
		Carrier = nullptr;
		Lines.Reset();
		Say(Uzenet::BozsiName,
			TEXT("\u00dcgyes fi\u00fa vagy! Egy ilyen fi\u00fanak mindig akad munka."),
			TEXT("Good lad! There's always work for a boy like you."), 8.f);
	}
}

void AMissionUzenet::TickStep(int32 Step, float DeltaSeconds)
{
	// Lines said a little later, in turn.
	const double Now = GetWorld()->GetTimeSeconds();
	if (Lines.Num() > 0 && Now >= Lines[0].At)
	{
		Say(Lines[0].Who, Lines[0].Hu, Lines[0].En, 7.f);
		Lines.RemoveAt(0);
	}

	// What he has goes out of sight with him, for example in a car, and
	// the parcel goes on his lap when he sits down.
	if (Carrier && Holder)
	{
		Holder->SetVisibility(!Carrier->IsHidden(), true);
		PlaceHolder();
	}
}

void AMissionUzenet::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	PutAway();
	// Back to her stall.
	if (BozsiAtStall)
	{
		BozsiAtStall->SetInWorld(true);
	}
	Super::EndPlay(EndPlayReason);
}

void AMissionUzenet::SayLater(float Seconds, const FString& Who, const FString& LineHu, const FString& LineEn)
{
	const double Last = Lines.Num() > 0 ? Lines.Last().At : GetWorld()->GetTimeSeconds();
	Lines.Add(FLater{ FMath::Max(Last, GetWorld()->GetTimeSeconds() + Seconds), Who, LineHu, LineEn });
}

void AMissionUzenet::PutAway()
{
	for (UStaticMeshComponent* Part : Parts)
	{
		if (Part)
		{
			Part->DestroyComponent();
		}
	}
	Parts.Reset();
	if (Holder)
	{
		Holder->DestroyComponent();
		Holder = nullptr;
	}
	if (IsValid(Carrier))
	{
		Carrier->SetHoldingInFront(false);
	}
	bParcel = false;
}

void AMissionUzenet::AddPart(const FLinearColor& Color, const FVector& At, const FVector& Size, const FRotator& Turn)
{
	if (!Holder || !CubeMesh)
	{
		return;
	}
	UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(this, NAME_None, RF_Transient);
	Part->SetupAttachment(Holder);
	Part->SetStaticMesh(CubeMesh);
	Part->SetRelativeLocationAndRotation(At, Turn);
	Part->SetRelativeScale3D(Size / 100.0);
	Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Part->SetCanEverAffectNavigation(false);
	if (BaseMaterial)
	{
		UMaterialInstanceDynamic* Paint = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		Paint->SetVectorParameterValue(TEXT("Color"), Color);
		Part->SetMaterial(0, Paint);
	}
	Part->RegisterComponent();
	Parts.Add(Part);
}

void AMissionUzenet::PlaceHolder()
{
	if (!Holder || !Carrier)
	{
		return;
	}
	if (bParcel)
	{
		Holder->SetRelativeLocationAndRotation(Carrier->IsRiding() ? Uzenet::ParcelOnLap : Uzenet::ParcelAt, FRotator::ZeroRotator);
	}
	else
	{
		Holder->SetRelativeLocationAndRotation(Uzenet::PocketAt, FRotator(0.0, Uzenet::PocketYaw, 0.0));
	}
}

void AMissionUzenet::PocketLetter()
{
	PutAway();
	if (!Carrier)
	{
		return;
	}
	Holder = NewObject<USceneComponent>(this, NAME_None, RF_Transient);
	Holder->SetupAttachment(Carrier->GetRootComponent());
	Holder->RegisterComponent();
	bParcel = false;
	PlaceHolder();
	// The envelope's top over the pocket's welt, a little askew.
	AddPart(Uzenet::Envelope, FVector(0.0, 0.0, 0.25), FVector(0.5, 7.5, 8.5), FRotator(0.0, 0.0, 6.0));
	AddPart(Uzenet::PocketWelt, FVector(0.4, 0.0, -2.0), FVector(1.0, 9.5, 5.0));
}

void AMissionUzenet::HoldParcel()
{
	PutAway();
	if (!Carrier)
	{
		return;
	}
	Holder = NewObject<USceneComponent>(this, NAME_None, RF_Transient);
	Holder->SetupAttachment(Carrier->GetRootComponent());
	Holder->RegisterComponent();
	bParcel = true;
	PlaceHolder();
	Carrier->SetHoldingInFront(true);
	// Brown paper, string tied both ways with a knot on top, and a wax seal on the front.
	const FVector& S = Uzenet::ParcelSize;
	const double T = Uzenet::TwineWidth;
	AddPart(Uzenet::BrownPaper, FVector::ZeroVector, S);
	AddPart(Uzenet::Twine, FVector::ZeroVector, FVector(S.X + 0.4, T, S.Z + 0.4));
	AddPart(Uzenet::Twine, FVector::ZeroVector, FVector(T, S.Y + 0.4, S.Z + 0.4));
	AddPart(Uzenet::Twine, FVector(0.0, 0.0, S.Z * 0.5 + 0.6), FVector(2.2, 2.2, 1.4), FRotator(0.0, 45.0, 0.0));
	AddPart(Uzenet::Wax, FVector(S.X * 0.5 + 0.15, 8.0, 2.0), FVector(0.3, 6.0, 4.0));
}
