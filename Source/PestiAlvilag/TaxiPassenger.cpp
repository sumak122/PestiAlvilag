// Fill out your copyright notice in the Description page of Project Settings.

#include "TaxiPassenger.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace TaxiPassenger
{
	// The right arm is the fourth body part (see CarBody::BuildPerson).
	constexpr int32 RightArm = 3;
	const FVector ArmDown(0.f, 29.f, 22.f);
	const FVector ArmUp(0.f, 33.f, 72.f);
	constexpr float WaveRate = 6.f;
	constexpr float WaveAngle = 18.f;

	// Coats of the period: browns, greys, a dark blue, a camel.
	const FLinearColor Coats[] = {
		FLinearColor(0.08f, 0.05f, 0.03f),
		FLinearColor(0.10f, 0.10f, 0.10f),
		FLinearColor(0.02f, 0.03f, 0.08f),
		FLinearColor(0.35f, 0.25f, 0.12f),
		FLinearColor(0.05f, 0.05f, 0.05f),
	};
	const FLinearColor BeaconColor(1.f, 0.75f, 0.1f);
}

ATaxiPassenger::ATaxiPassenger()
{
	PrimaryActorTick.bCanEverTick = true;

	BaseMaterial = CarBody::LoadPaintMaterial();

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;
	CarBody::BuildPerson(this, Root, BodyParts, BodyPaints);

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

	const FLinearColor Coat = TaxiPassenger::Coats[FMath::RandRange(0, UE_ARRAY_COUNT(TaxiPassenger::Coats) - 1)];
	CarBody::Paint(this, BaseMaterial, BodyParts, BodyPaints, Coat);
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

	if (!bIsWaiting || !BodyParts.IsValidIndex(TaxiPassenger::RightArm) || !BodyParts[TaxiPassenger::RightArm])
	{
		return;
	}
	WavePhase += DeltaSeconds * TaxiPassenger::WaveRate;
	BodyParts[TaxiPassenger::RightArm]->SetRelativeRotation(FRotator(0.f, 0.f, FMath::Sin(WavePhase) * TaxiPassenger::WaveAngle));
}

void ATaxiPassenger::SetWaiting(bool bWaiting)
{
	bIsWaiting = bWaiting;
	Beacon->SetVisibility(bWaiting);
	if (BodyParts.IsValidIndex(TaxiPassenger::RightArm) && BodyParts[TaxiPassenger::RightArm])
	{
		BodyParts[TaxiPassenger::RightArm]->SetRelativeLocationAndRotation(bWaiting ? TaxiPassenger::ArmUp : TaxiPassenger::ArmDown, FRotator::ZeroRotator);
	}
}

void ATaxiPassenger::SetInWorld(bool bInWorld)
{
	SetActorHiddenInGame(!bInWorld);
	SetActorTickEnabled(bInWorld);
}
