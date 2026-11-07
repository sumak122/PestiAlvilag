// Fill out your copyright notice in the Description page of Project Settings.

#include "ShapeSet.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/TextRenderComponent.h"
#include "DayNight.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace ShapeSetLook
{
	// The lamps' warm gas light, how bright at full dark, and how often their brightness is checked (s).
	const FLinearColor LampColor(1.f, 0.74f, 0.45f);
	constexpr float LampCandela = 40.f;
	constexpr float LampCheckEvery = 1.f;
}

AShapeSet::AShapeSet()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicShapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	BaseMaterial = BasicShapeMaterial.Object;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMesh(TEXT("/Engine/BasicShapes/Cone.Cone"));
	Meshes = { CubeMesh.Object, CylinderMesh.Object, SphereMesh.Object, ConeMesh.Object };
}

AShapeSet* AShapeSet::Find(const UWorld* World, FName InKey)
{
	if (!World)
	{
		return nullptr;
	}
	for (TActorIterator<AShapeSet> It(World); It; ++It)
	{
		if (It->Key == InKey)
		{
			return *It;
		}
	}
	return nullptr;
}

void AShapeSet::SetDrawDistance(float Distance)
{
	DrawDistance = Distance;
	for (UInstancedStaticMeshComponent* Batch : Batches)
	{
		Batch->SetCullDistances(0, DrawDistance);
	}
}

void AShapeSet::AddShapes(const TArray<MarketModel::FShape>& Shapes)
{
	for (const MarketModel::FShape& Piece : Shapes)
	{
		if (!Meshes.IsValidIndex(Piece.Shape) || !Meshes[Piece.Shape])
		{
			continue;
		}
		// One batch per shape, colour and collision; the instances are placed in the set's frame.
		const FString BatchKey = FString::Printf(TEXT("%d %s %d"), Piece.Shape, *Piece.Color.ToString(), Piece.bCollide ? 1 : 0);
		UInstancedStaticMeshComponent* Instances = nullptr;
		if (const int32* Found = BatchIndex.Find(BatchKey))
		{
			Instances = Batches[*Found];
		}
		else
		{
			Instances = NewObject<UInstancedStaticMeshComponent>(this, NAME_None, RF_Transient);
			Instances->SetupAttachment(RootComponent);
			Instances->SetStaticMesh(Meshes[Piece.Shape]);
			if (BaseMaterial)
			{
				UMaterialInstanceDynamic* Paint = UMaterialInstanceDynamic::Create(BaseMaterial, this);
				Paint->SetVectorParameterValue(TEXT("Color"), Piece.Color);
				Instances->SetMaterial(0, Paint);
			}
			Instances->SetCollisionProfileName(Piece.bCollide ? TEXT("BlockAll") : TEXT("NoCollision"));
			Instances->SetCullDistances(0, DrawDistance);
			Instances->SetCanEverAffectNavigation(false);
			Instances->RegisterComponent();
			BatchIndex.Add(BatchKey, Batches.Add(Instances));
		}
		Instances->AddInstance(FTransform(FRotator(Piece.Pitch, Piece.Yaw, Piece.Roll), Piece.At, Piece.Size / 100.0), false);
	}
}

void AShapeSet::AddSign(const FString& Text, const FVector& At, double Yaw, double Height, const FLinearColor& Color)
{
	UTextRenderComponent* Lettering = NewObject<UTextRenderComponent>(this, NAME_None, RF_Transient);
	Lettering->SetupAttachment(RootComponent);
	Lettering->SetRelativeLocationAndRotation(At, FRotator(0.0, Yaw, 0.0));
	Lettering->SetText(FText::FromString(Text));
	Lettering->SetHorizontalAlignment(EHTA_Center);
	Lettering->SetVerticalAlignment(EVRTA_TextCenter);
	Lettering->SetWorldSize(Height);
	Lettering->SetTextRenderColor(Color.ToFColor(true));
	Lettering->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Lettering->RegisterComponent();
}

void AShapeSet::AddLamp(const FVector& At, float Radius)
{
	UPointLightComponent* Light = NewObject<UPointLightComponent>(this, NAME_None, RF_Transient);
	Light->SetupAttachment(RootComponent);
	Light->SetMobility(EComponentMobility::Movable);
	Light->SetRelativeLocation(At);
	Light->SetCastShadows(false);
	Light->SetIntensityUnits(ELightUnits::Candelas);
	Light->SetIntensity(0.f);
	Light->SetAttenuationRadius(Radius);
	Light->SetLightColor(ShapeSetLook::LampColor);
	Light->SetVisibility(false);
	Light->RegisterComponent();
	Lamps.Add(Light);
	SetActorTickEnabled(true);
}

void AShapeSet::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	LampCheckIn -= DeltaSeconds;
	if (LampCheckIn > 0.f)
	{
		return;
	}
	LampCheckIn = ShapeSetLook::LampCheckEvery;
	const ADayNight* Sky = ADayNight::Get(GetWorld());
	const float Dark = Sky ? Sky->GetDarkness() : 0.f;
	for (UPointLightComponent* Light : Lamps)
	{
		Light->SetVisibility(Dark > 0.05f);
		Light->SetIntensity(ShapeSetLook::LampCandela * Dark);
	}
}
