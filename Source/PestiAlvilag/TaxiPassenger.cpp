// Fill out your copyright notice in the Description page of Project Settings.

#include "TaxiPassenger.h"

#include "CarBody.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PersonModel.h"
#include "ProceduralMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

namespace TaxiPassenger
{
	// Hailing: the right arm raised up and forward, waving to and fro.
	constexpr int32 RightArm = PersonKit::ArmRight;
	constexpr float ArmUp = 150.f;
	constexpr float WaveRate = 6.f;
	constexpr float WaveAngle = 15.f;
	const FLinearColor BeaconColor(1.f, 0.75f, 0.1f);
}

ATaxiPassenger::ATaxiPassenger()
{
	PrimaryActorTick.bCanEverTick = true;

	BaseMaterial = CarBody::LoadPaintMaterial();

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	// A tall thin column of light, starting well above the head.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	Beacon = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Beacon"));
	Beacon->SetupAttachment(Root);
	Beacon->SetStaticMesh(CylinderMesh.Object);
	Beacon->SetRelativeLocation(FVector(0.f, 0.f, 1800.f));
	Beacon->SetRelativeScale3D(FVector(0.3f, 0.3f, 30.f));
	Beacon->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Beacon->SetCastShadow(false);
}

void ATaxiPassenger::BeginPlay()
{
	Super::BeginPlay();

	FRandomStream Random(FMath::Rand());
	Outfit = PersonModel::RandomOutfit(Random);
	// A free hand to wave with.
	Outfit.Held = PersonKit::EHeld::None;
	PersonModel::Dress(this, RootComponent, BaseMaterial, Outfit, Pieces);
	if (BaseMaterial)
	{
		UMaterialInstanceDynamic* Light = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		Light->SetVectorParameterValue(TEXT("Color"), TaxiPassenger::BeaconColor);
		Beacon->SetMaterial(0, Light);
	}
	SetWaiting(true);
}

void ATaxiPassenger::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bIsWaiting || !Pieces.IsValidIndex(TaxiPassenger::RightArm) || !Pieces[TaxiPassenger::RightArm])
	{
		return;
	}
	WavePhase += DeltaSeconds * TaxiPassenger::WaveRate;
	Pieces[TaxiPassenger::RightArm]->SetRelativeRotation(FRotator(TaxiPassenger::ArmUp + FMath::Sin(WavePhase) * TaxiPassenger::WaveAngle, 0.f, 0.f));
}

void ATaxiPassenger::SetWaiting(bool bWaiting)
{
	bIsWaiting = bWaiting;
	Beacon->SetVisibility(bWaiting);
	if (Pieces.IsValidIndex(TaxiPassenger::RightArm) && Pieces[TaxiPassenger::RightArm])
	{
		Pieces[TaxiPassenger::RightArm]->SetRelativeRotation(FRotator(bWaiting ? TaxiPassenger::ArmUp : 0.f, 0.f, 0.f));
	}
}

void ATaxiPassenger::SetInWorld(bool bInWorld)
{
	SetActorHiddenInGame(!bInWorld);
	SetActorTickEnabled(bInWorld);
}
