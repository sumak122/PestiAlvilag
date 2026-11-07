// Fill out your copyright notice in the Description page of Project Settings.

#include "LaciYard.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PersonModel.h"
#include "LaciYardModel.h"
#include "StoryPerson.h"
#include "UObject/ConstructorHelpers.h"

namespace LaciYardPlace
{
	// Things in the yard are not drawn from further off than this (cm).
	constexpr float DrawDistance = 20000.f;
}

ALaciYard::ALaciYard()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicShapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	BaseMaterial = BasicShapeMaterial.Object;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMesh(TEXT("/Engine/BasicShapes/Cone.Cone"));
	Shapes = { CubeMesh.Object, CylinderMesh.Object, SphereMesh.Object, ConeMesh.Object };
}

FVector ALaciYard::ToCity(const FVector& Local, double Ground)
{
	const FRotator Turn(0.0, LaciYardModel::CityYaw, 0.0);
	return FVector(LaciYardModel::CityAt, Ground) + Turn.RotateVector(Local);
}

AStoryPerson* ALaciYard::GetLaci() const
{
	return Laci;
}

void ALaciYard::BeginPlay()
{
	Super::BeginPlay();

	TArray<MarketModel::FShape> Pieces;
	LaciYardModel::Build(Pieces);
	for (const MarketModel::FShape& Piece : Pieces)
	{
		AddShape(Piece.Shape, Piece.Color, Piece.At, Piece.Size, FRotator(Piece.Pitch, Piece.Yaw, Piece.Roll), Piece.bCollide);
	}

	// The painted board over the gate, the lettering on the wall.
	TArray<LaciYardModel::FSign> Signs;
	LaciYardModel::Signs(Signs);
	for (const LaciYardModel::FSign& Sign : Signs)
	{
		UTextRenderComponent* Text = NewObject<UTextRenderComponent>(this, NAME_None, RF_Transient);
		Text->SetupAttachment(RootComponent);
		Text->SetRelativeLocationAndRotation(Sign.At, FRotator(0.0, Sign.Yaw, 0.0));
		Text->SetText(FText::FromString(Sign.Text));
		Text->SetHorizontalAlignment(EHTA_Center);
		Text->SetVerticalAlignment(EVRTA_TextCenter);
		Text->SetWorldSize(Sign.Height);
		Text->SetTextRenderColor(Sign.Color.ToFColor(true));
		Text->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Text->RegisterComponent();
	}

	// Laci, by the workshop.
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	const FVector MechanicAt = GetActorTransform().TransformPosition(LaciYardModel::Spots::Laci) + FVector(0.0, 0.0, PersonModel::MiddleHeight);
	const FRotator MechanicTurn(0.0, GetActorRotation().Yaw + LaciYardModel::Spots::LaciYaw, 0.0);
	Laci = GetWorld()->SpawnActor<AStoryPerson>(AStoryPerson::StaticClass(), MechanicAt, MechanicTurn, Params);
	if (Laci)
	{
		Laci->SetOutfit(LaciYardModel::Laci());
	}
}

void ALaciYard::AddShape(int32 Shape, const FLinearColor& Color, const FVector& At, const FVector& Size, const FRotator& Turn, bool bCollide)
{
	if (!Shapes.IsValidIndex(Shape) || !Shapes[Shape])
	{
		return;
	}
	// One batch per shape, colour and collision; the instances are placed in the yard's frame.
	const FString Key = FString::Printf(TEXT("%d %s %d"), Shape, *Color.ToString(), bCollide ? 1 : 0);
	UInstancedStaticMeshComponent* Instances = nullptr;
	if (const int32* Found = BatchIndex.Find(Key))
	{
		Instances = Batches[*Found];
	}
	else
	{
		Instances = NewObject<UInstancedStaticMeshComponent>(this, NAME_None, RF_Transient);
		Instances->SetupAttachment(RootComponent);
		Instances->SetStaticMesh(Shapes[Shape]);
		if (BaseMaterial)
		{
			UMaterialInstanceDynamic* Paint = UMaterialInstanceDynamic::Create(BaseMaterial, this);
			Paint->SetVectorParameterValue(TEXT("Color"), Color);
			Instances->SetMaterial(0, Paint);
		}
		Instances->SetCollisionProfileName(bCollide ? TEXT("BlockAll") : TEXT("NoCollision"));
		Instances->SetCullDistances(0, LaciYardPlace::DrawDistance);
		Instances->SetCanEverAffectNavigation(false);
		Instances->RegisterComponent();
		BatchIndex.Add(Key, Batches.Add(Instances));
	}
	Instances->AddInstance(FTransform(Turn, At, Size / 100.0), false);
}
