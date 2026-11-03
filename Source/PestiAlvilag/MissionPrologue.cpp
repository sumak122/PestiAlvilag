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
	Steps.Add(Home);
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
