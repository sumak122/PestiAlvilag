// Fill out your copyright notice in the Description page of Project Settings.

#include "MissionPrologue.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "HomeDoor.h"
#include "KeletiStation.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PestiAlvilagGameModeBase.h"
#include "PestiPerson.h"
#include "PestiTaxi.h"
#include "StoryPerson.h"
#include "UObject/ConstructorHelpers.h"

namespace Prologue
{
	// Off the train: on the arrival platform inside Keleti, beside the
	// first carriage, facing the great arch and Baross tér beyond it
	// (in the station's own frame: X from the front into the hall, Y
	// towards the arrival side).
	const FVector2D StartAt = AKeletiStation::ToCity(FVector2D(5200.0, 1750.0));
	constexpr double StartYaw = 180.0;

	// The gentleman waits a few steps ahead on the platform with his suitcase.
	const FVector2D GentlemanAt = AKeletiStation::ToCity(FVector2D(4400.0, 1720.0));
	const FLinearColor GentlemanCoat(0.30f, 0.22f, 0.12f);

	// His taxi stands at the kerb on Baross tér, about 60 m away.
	const FVector2D TaxiAt(85836.f, -42980.f);
	constexpr double TaxiYaw = 140.0;
	constexpr float DriverBeside = 180.f;
	const FLinearColor DriverCoat(0.04f, 0.04f, 0.05f);

	// The flower bed in the middle of Tisza Kálmán tér; anywhere on the
	// round gravel place around it counts as there.
	const FVector2D ParkMiddle(59600.f, 700.f);
	constexpr float ParkRadius = 2400.f;

	// A brown leather case, carried in the right hand.
	const FVector SuitcaseScale(0.7f, 0.22f, 0.48f);
	const FVector SuitcaseInHand(0.f, 48.f, -52.f);
	const FVector SuitcaseBeside(30.f, 60.f, -66.f);
	const FLinearColor Leather(0.22f, 0.11f, 0.05f);

	constexpr int32 Tip = 50;
	constexpr float IntroFade = 4.f;
	constexpr int32 MessageLine = 3;
}

AMissionPrologue::AMissionPrologue()
{
	MissionId = TEXT("Prologue");
	Reward = 500;
	TitleHu = TEXT("Meg\u00e9rkez\u00e9s");
	TitleEn = TEXT("Arrival");

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	Suitcase = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Suitcase"));
	Suitcase->SetupAttachment(RootComponent);
	Suitcase->SetStaticMesh(CubeMesh.Object);
	Suitcase->SetRelativeScale3D(Prologue::SuitcaseScale);
	Suitcase->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

FVector2D AMissionPrologue::StartAt()
{
	return Prologue::StartAt;
}

double AMissionPrologue::StartYaw()
{
	return Prologue::StartYaw;
}

void AMissionPrologue::SetUpMission()
{
	if (BaseMaterial)
	{
		UMaterialInstanceDynamic* Paintwork = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		Paintwork->SetVectorParameterValue(TEXT("Color"), Prologue::Leather);
		Suitcase->SetMaterial(0, Paintwork);
	}

	// Off the train: the picture comes up slowly on the platform.
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		if (PC->PlayerCameraManager)
		{
			PC->PlayerCameraManager->StartCameraFade(1.f, 0.f, Prologue::IntroFade, FLinearColor::Black);
		}
	}
	Announce(TEXT("Budapest, 1932 \u0151sze. Keleti p\u00e1lyaudvar."), TEXT("Budapest, autumn 1932. Keleti station."), 8.f);

	// The gentleman, with his suitcase on the ground beside him.
	const FVector GentlemanLocation(Prologue::GentlemanAt, GroundZ(Prologue::GentlemanAt) + 90.0);
	const double GentlemanYaw = (FVector(Prologue::StartAt, 0.0) - FVector(Prologue::GentlemanAt, 0.0)).Rotation().Yaw;
	Gentleman = SpawnForMission<AStoryPerson>(GentlemanLocation, GentlemanYaw);
	if (Gentleman)
	{
		Gentleman->SetCoatColor(Prologue::GentlemanCoat);
		Suitcase->AttachToComponent(Gentleman->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
		Suitcase->SetRelativeLocation(Prologue::SuitcaseBeside);
	}

	// His taxi, with the driver waiting on the pavement beside it.
	const FVector TaxiLocation(Prologue::TaxiAt, GroundZ(Prologue::TaxiAt) + 150.0);
	Taxi = SpawnForMission<APestiTaxi>(TaxiLocation, Prologue::TaxiYaw);
	KeepAfterMission(Taxi);
	const FRotator TaxiFacing(0.0, Prologue::TaxiYaw, 0.0);
	const FVector2D DriverAt = Prologue::TaxiAt - FVector2D(TaxiFacing.RotateVector(FVector::RightVector)) * Prologue::DriverBeside;
	const FVector DriverLocation(DriverAt, GroundZ(DriverAt) + 90.0);
	Driver = SpawnForMission<AStoryPerson>(DriverLocation, Prologue::TaxiYaw);
	if (Driver)
	{
		Driver->SetCoatColor(Prologue::DriverCoat);
		Driver->FaceTowards(GentlemanLocation);
	}

	FMissionStep Help;
	Help.GoalHu = TEXT("Seg\u00edts az \u00farnak a b\u0151r\u00f6ndj\u00e9vel");
	Help.GoalEn = TEXT("Help the gentleman with his suitcase");
	Help.Target = GentlemanLocation;
	Help.Radius = 250.f;
	Help.bPressE = true;
	Steps.Add(Help);

	FMissionStep Carry;
	Carry.GoalHu = TEXT("Vidd a b\u0151r\u00f6nd\u00f6t a taxihoz a Baross t\u00e9ren");
	Carry.GoalEn = TEXT("Carry the suitcase to the taxi on Baross t\u00e9r");
	Carry.Target = TaxiLocation;
	Carry.Radius = 400.f;
	// Out through the arch and the taxi is in sight; street arrows would only confuse inside the hall.
	Carry.bGuide = false;
	Steps.Add(Carry);

	// The way home leads through the park on Tisza Kálmán tér.
	FMissionStep Park;
	Park.GoalHu = TEXT("Menj \u00e1t a Tisza K\u00e1lm\u00e1n t\u00e9ren");
	Park.GoalEn = TEXT("Walk through Tisza K\u00e1lm\u00e1n t\u00e9r");
	Park.Target = FVector(Prologue::ParkMiddle, GroundZ(Prologue::ParkMiddle) + 90.0);
	Park.Radius = Prologue::ParkRadius;
	Steps.Add(Park);

	FMissionStep Home;
	Home.GoalHu = TEXT("Keresd meg B\u00f6zsi n\u00e9ni h\u00e1z\u00e1t a N\u00e9psz\u00ednh\u00e1z utc\u00e1ban");
	Home.GoalEn = TEXT("Find Aunt B\u00f6zsi's house on N\u00e9psz\u00ednh\u00e1z utca");
	TActorIterator<AHomeDoor> Door(GetWorld());
	Home.Target = Door ? Door->GetStandSpot() : FVector::ZeroVector;
	Home.Radius = 300.f;
	Steps.Add(Home);
}

void AMissionPrologue::OnStepStarted(int32 Step)
{
	if (Step == 0)
	{
		Say(TEXT("\u00dariember"), TEXT("Fiatalember! Lenne sz\u00edves seg\u00edteni egy \u00f6regemberen?"),
			TEXT("Young man! Would you be so kind as to help an old man?"), 8.f);
	}
}

void AMissionPrologue::OnStepDone(int32 Step)
{
	APestiPerson* Person = Cast<APestiPerson>(GetPlayer());

	if (Step == 0)
	{
		// Take the case; the gentleman walks along behind.
		if (Person)
		{
			Suitcase->AttachToComponent(Person->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
			Suitcase->SetRelativeLocationAndRotation(Prologue::SuitcaseInHand, FRotator::ZeroRotator);
			Person->SetCarrying(true);
		}
		if (Gentleman)
		{
			Gentleman->Follow(Person);
		}
		Say(TEXT("\u00dariember"), TEXT("H\u00e1l\u00e1s k\u00f6sz\u00f6net! A taxim ott v\u00e1r a Baross t\u00e9ren."),
			TEXT("Many thanks! My taxi is waiting over on Baross t\u00e9r."));
	}
	else if (Step == 1)
	{
		// The case goes in the boot, the gentleman into the back, and he pays.
		Suitcase->SetVisibility(false);
		if (Person)
		{
			Person->SetCarrying(false);
		}
		if (Gentleman)
		{
			Gentleman->Follow(nullptr);
			Gentleman->SetInWorld(false);
		}
		if (APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this))
		{
			Game->AddMoney(Prologue::Tip);
		}
		Say(TEXT("\u00dariember"), TEXT("Tess\u00e9k, \u00f6tven fill\u00e9r. Isten \u00e1ldja, fiam!"),
			TEXT("Here, fifty fill\u00e9r. God bless you, son!"), 8.f);
	}
	else if (Step == 3)
	{
		Say(TEXT("B\u00f6zsi n\u00e9ni"), TEXT("H\u00e1t megj\u00f6tt\u00e9l, fiam! Gyere csak, megmutatom a szob\u00e1dat."),
			TEXT("So you made it, son! Come in, I'll show you your room."), 8.f);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(Prologue::MessageLine, 12.f, FColor::Yellow,
				TEXT("Ez most az otthonod: az ajt\u00f3n\u00e1l E-vel alhatsz \u00e9s menthetsz.  (This is your home now: press E at the door to sleep and save.)"));
		}
	}
}
