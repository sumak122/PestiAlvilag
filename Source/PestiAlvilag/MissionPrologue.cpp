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
#include "TrafficCar.h"
#include "TrafficManager.h"
#include "UObject/ConstructorHelpers.h"
#include "PestiHud.h"

namespace Prologue
{
	// Off the train: on the arrival platform inside Keleti, beside the
	// first carriage, facing the great arch and Baross tér beyond it
	// (in the station's own frame: X from the front into the hall, Y
	// towards the arrival side).
	const FVector2D StartAt = AKeletiStation::ToCity(FVector2D(5200.0, 1750.0));
	constexpr double StartYaw = 180.0;

	// An old gentleman waits a few steps ahead on the platform with his
	// suitcase on the ground beside him. He is only part of the scene.
	const FVector2D GentlemanAt = AKeletiStation::ToCity(FVector2D(4400.0, 1720.0));
	const FLinearColor GentlemanCoat(0.30f, 0.22f, 0.12f);

	// A brown leather case.
	const FVector SuitcaseScale(0.7f, 0.22f, 0.48f);
	const FVector SuitcaseBeside(30.f, 60.f, -66.f);
	const FLinearColor Leather(0.22f, 0.11f, 0.05f);

	constexpr float IntroFade = 4.f;
	constexpr int32 MessageLine = 3;
	// The keys for the letter and for asking the way, shown once the
	// station's name has gone (s).
	constexpr float KeysFor = 20.f;
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

	// The gentleman, waiting with his suitcase on the ground beside him.
	const FVector GentlemanLocation(Prologue::GentlemanAt, GroundZ(Prologue::GentlemanAt) + 90.0);
	const double GentlemanYaw = (FVector(Prologue::StartAt, 0.0) - FVector(Prologue::GentlemanAt, 0.0)).Rotation().Yaw;
	Gentleman = SpawnForMission<AStoryPerson>(GentlemanLocation, GentlemanYaw);
	if (Gentleman)
	{
		Gentleman->SetCoatColor(Prologue::GentlemanCoat);
		Suitcase->AttachToComponent(Gentleman->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
		Suitcase->SetRelativeLocation(Prologue::SuitcaseBeside);
	}
	else
	{
		Suitcase->SetVisibility(false);
	}

	// The only goal: find the way to Bözsi néni.
	FMissionStep Home;
	Home.GoalHu = TEXT("Menj B\u00f6zsi n\u00e9nihez a N\u00e9psz\u00ednh\u00e1z utc\u00e1ba");
	Home.GoalEn = TEXT("Get to Aunt B\u00f6zsi on N\u00e9psz\u00ednh\u00e1z utca");
	TActorIterator<AHomeDoor> Door(GetWorld());
	Home.Target = Door ? Door->GetStandSpot() : FVector::ZeroVector;
	Home.Radius = 300.f;
	Home.Place = TEXT("N\u00e9psz\u00ednh\u00e1z utca");
	Steps.Add(Home);
}

void AMissionPrologue::OnStepStarted(int32 Step)
{
	if (Step == 0)
	{
		APestiHUD::Show(this, Prologue::MessageLine, Prologue::KeysFor, FColor::Yellow,
			TEXT("R: B\u00f6zsi n\u00e9ni level\u00e9t olvasod,  Q: utat k\u00e9rdezel egy j\u00e1r\u00f3kel\u0151t\u0151l  (R: read Aunt B\u00f6zsi's letter,  Q: ask a passer-by the way)"));
	}
}

bool AMissionPrologue::GetLetter(FString& OutHu, FString& OutEn) const
{
	OutHu = TEXT("Kedves Fiam!\n\n")
		TEXT("Ha lesz\u00e1llsz a vonatr\u00f3l, gyere ki a Keleti nagy kapuj\u00e1n a Baross t\u00e9rre. ")
		TEXT("Menj a R\u00e1k\u00f3czi \u00faton a Belv\u00e1ros fel\u00e9, \u00e9s fordulj balra a Keny\u00e9rmez\u0151 utc\u00e1ba. ")
		TEXT("Az a Tisza K\u00e1lm\u00e1n t\u00e9rre visz, ott v\u00e1gj \u00e1t a parkon. ")
		TEXT("A t\u00e9r t\u00fals\u00f3 v\u00e9g\u00e9n \u00e9red el a N\u00e9psz\u00ednh\u00e1z utc\u00e1t: fordulj balra, \u00e9s p\u00e1r h\u00e1z ut\u00e1n ott v\u00e1rlak a kapuban.\n\n")
		TEXT("Ha m\u00e9gis elt\u00e9vedn\u00e9l, k\u00e9rdezz meg b\u00e1rkit az utc\u00e1n, a pesti ember sz\u00edvesen \u00fatba igaz\u00edt.\n\n")
		TEXT(">Cs\u00f3kol:\n>B\u00f6zsi n\u00e9ni");
	OutEn = TEXT("Dear boy,\n\n")
		TEXT("When you get off the train, come out through the big gate of Keleti onto Baross t\u00e9r. ")
		TEXT("Walk along R\u00e1k\u00f3czi \u00fat towards the city centre and turn left into Keny\u00e9rmez\u0151 utca. ")
		TEXT("It brings you to Tisza K\u00e1lm\u00e1n t\u00e9r: cut across the park. ")
		TEXT("At the far end of the square you reach N\u00e9psz\u00ednh\u00e1z utca: turn left, and a few houses on I'll be waiting at the gate.\n\n")
		TEXT("If you get lost, ask anyone in the street: Pest folk are glad to show the way.\n\n")
		TEXT(">Love,\n>Aunt B\u00f6zsi");
	return true;
}

void AMissionPrologue::OnStepDone(int32 Step)
{
	if (Step == 0)
	{
		Say(TEXT("B\u00f6zsi n\u00e9ni"), TEXT("H\u00e1t megj\u00f6tt\u00e9l, fiam! Gyere csak, megmutatom a szob\u00e1dat."),
			TEXT("So you made it, son! Come in, I'll show you your room."), 8.f);
		if (GEngine)
		{
			APestiHUD::Show(this, Prologue::MessageLine, 12.f, FColor::Yellow,
				TEXT("Ez most az otthonod: az ajt\u00f3n\u00e1l E-vel alhatsz \u00e9s menthetsz.  (This is your home now: press E at the door to sleep and save.)"));
		}
	}
}
