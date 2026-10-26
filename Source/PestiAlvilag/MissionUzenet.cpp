// Fill out your copyright notice in the Description page of Project Settings.

#include "MissionUzenet.h"

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

	// The letter is a white envelope, the parcel brown paper, both held
	// at his side in the right hand.
	const FVector LetterSize(0.22f, 0.02f, 0.12f);
	const FLinearColor LetterPaper(0.85f, 0.82f, 0.74f);
	const FVector ParcelSize(0.30f, 0.18f, 0.14f);
	const FLinearColor BrownPaper(0.42f, 0.30f, 0.18f);
	const FVector InHand(4.f, 32.f, -34.f);

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

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	InHand = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("InHand"));
	InHand->SetupAttachment(RootComponent);
	InHand->SetStaticMesh(CubeMesh.Object);
	InHand->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	InHand->SetVisibility(false);
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
		Hold(Uzenet::LetterSize, Uzenet::LetterPaper);
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
		Hold(Uzenet::ParcelSize, Uzenet::BrownPaper);
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
		InHand->SetVisibility(false);
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

	// What he holds goes out of sight with him, for example in a car.
	if (Carrier)
	{
		InHand->SetVisibility(!Carrier->IsHidden());
	}
}

void AMissionUzenet::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
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

void AMissionUzenet::Hold(const FVector& Size, const FLinearColor& Color)
{
	if (!Carrier)
	{
		return;
	}
	if (BaseMaterial)
	{
		UMaterialInstanceDynamic* Paper = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		Paper->SetVectorParameterValue(TEXT("Color"), Color);
		InHand->SetMaterial(0, Paper);
	}
	InHand->AttachToComponent(Carrier->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
	InHand->SetRelativeLocationAndRotation(Uzenet::InHand, FRotator::ZeroRotator);
	InHand->SetRelativeScale3D(Size);
	InHand->SetVisibility(true);
}
