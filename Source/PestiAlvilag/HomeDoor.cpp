// Fill out your copyright notice in the Description page of Project Settings.

#include "HomeDoor.h"

#include "CarBody.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PestiAlvilagGameModeBase.h"
#include "PestiPerson.h"
#include "UObject/ConstructorHelpers.h"

namespace HomeDoor
{
	// A tall double door of dark wood in a stone frame, with a lamp over it
	// (cm, X out to the street, Z up from the pavement).
	struct FDoorPart
	{
		const TCHAR* Name;
		FVector Location;
		FVector Scale;
		FLinearColor Color;
	};
	const FLinearColor Wood(0.10f, 0.06f, 0.035f);
	const FLinearColor Stone(0.45f, 0.42f, 0.36f);
	const FLinearColor Iron(0.03f, 0.03f, 0.03f);
	const FLinearColor Lamp(1.f, 0.8f, 0.45f);
	const FDoorPart DoorSpecs[] = {
		{ TEXT("Door"), FVector(7.f, 0.f, 150.f), FVector(0.12f, 1.6f, 3.0f), Wood },
		{ TEXT("FrameLeft"), FVector(12.f, -95.f, 160.f), FVector(0.2f, 0.3f, 3.2f), Stone },
		{ TEXT("FrameRight"), FVector(12.f, 95.f, 160.f), FVector(0.2f, 0.3f, 3.2f), Stone },
		{ TEXT("FrameTop"), FVector(12.f, 0.f, 330.f), FVector(0.2f, 2.2f, 0.4f), Stone },
		{ TEXT("Step"), FVector(30.f, 0.f, 5.f), FVector(0.6f, 2.0f, 0.1f), Stone },
		{ TEXT("LampArm"), FVector(30.f, 0.f, 370.f), FVector(0.6f, 0.04f, 0.04f), Iron },
		{ TEXT("LampGlass"), FVector(55.f, 0.f, 355.f), FVector(0.2f, 0.2f, 0.3f), Lamp },
	};

	constexpr float StandOut = 150.f;
	constexpr float PersonMiddle = 90.f;
	constexpr double SleepFade = 1.0;
}

AHomeDoor::AHomeDoor()
{
	PrimaryActorTick.bCanEverTick = true;

	BaseMaterial = CarBody::LoadPaintMaterial();

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	for (const HomeDoor::FDoorPart& Spec : HomeDoor::DoorSpecs)
	{
		UStaticMeshComponent* Part = CreateDefaultSubobject<UStaticMeshComponent>(Spec.Name);
		Part->SetupAttachment(Root);
		Part->SetStaticMesh(CubeMesh.Object);
		Part->SetRelativeLocation(Spec.Location);
		Part->SetRelativeScale3D(Spec.Scale);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Parts.Add(Part);
	}
}

void AHomeDoor::BeginPlay()
{
	Super::BeginPlay();

	if (!BaseMaterial)
	{
		return;
	}
	for (int32 i = 0; i < Parts.Num() && i < static_cast<int32>(UE_ARRAY_COUNT(HomeDoor::DoorSpecs)); ++i)
	{
		UMaterialInstanceDynamic* Paintwork = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		Paintwork->SetVectorParameterValue(TEXT("Color"), HomeDoor::DoorSpecs[i].Color);
		Parts[i]->SetMaterial(0, Paintwork);
	}
}

FVector AHomeDoor::GetStandSpot() const
{
	return GetActorLocation() + GetActorForwardVector() * HomeDoor::StandOut + FVector(0.0, 0.0, HomeDoor::PersonMiddle);
}

double AHomeDoor::GetStandYaw() const
{
	return GetActorRotation().Yaw;
}

void AHomeDoor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this);
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	const APawn* Player = PC ? PC->GetPawn() : nullptr;
	if (!Game || !Player || !Player->IsA<APestiPerson>() || !Game->CanSaveAtHome())
	{
		return;
	}

	// Coming back out of the door after the fade.
	const double Now = GetWorld()->GetTimeSeconds();
	if (FadeDoneTime > 0.0 && Now >= FadeDoneTime)
	{
		FadeDoneTime = -1.0;
		if (PC->PlayerCameraManager)
		{
			PC->PlayerCameraManager->StartCameraFade(1.f, 0.f, HomeDoor::SleepFade, FLinearColor::Black);
		}
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(9, 4.f, FColor(255, 230, 160),
				Game->SaveToDisk() ? TEXT("J\u00e1t\u00e9k mentve  (Game saved)") : TEXT("Nem siker\u00fclt menteni  (Could not save)"));
		}
	}

	if (FadeDoneTime > 0.0 || FVector::Dist2D(Player->GetActorLocation(), GetActorLocation()) > UseRange)
	{
		return;
	}
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(4, 0.f, FColor(255, 200, 60),
			TEXT("Otthon  (Home)   E: alv\u00e1s \u00e9s ment\u00e9s  (sleep and save)"));
	}
	if (PC->WasInputKeyJustPressed(EKeys::E))
	{
		if (PC->PlayerCameraManager)
		{
			PC->PlayerCameraManager->StartCameraFade(0.f, 1.f, HomeDoor::SleepFade, FLinearColor::Black, false, true);
		}
		FadeDoneTime = Now + HomeDoor::SleepFade + 0.5;
	}
}
