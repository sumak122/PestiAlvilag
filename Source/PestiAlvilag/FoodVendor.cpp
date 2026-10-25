// Fill out your copyright notice in the Description page of Project Settings.

#include "FoodVendor.h"

#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PersonModel.h"
#include "StoryPerson.h"
#include "UObject/ConstructorHelpers.h"

namespace FoodVendor
{
	enum EShape : int32 { Cube, Cylinder, Sphere, Cone };

	const FLinearColor Iron(0.04f, 0.04f, 0.045f);
	const FLinearColor Wood(0.32f, 0.19f, 0.09f);
	const FLinearColor DarkWood(0.18f, 0.10f, 0.05f);
	const FLinearColor Fire(1.0f, 0.38f, 0.05f);
	const FLinearColor Chestnut(0.22f, 0.10f, 0.05f);
	const FLinearColor Pumpkin(0.85f, 0.42f, 0.08f);
	const FLinearColor Copper(0.55f, 0.27f, 0.12f);
	const FLinearColor Cloth(0.86f, 0.84f, 0.78f);
	const FLinearColor Green(0.10f, 0.22f, 0.12f);
	const FLinearColor Basket(0.52f, 0.36f, 0.16f);
	const FLinearColor Pretzel(0.48f, 0.24f, 0.08f);
	const FLinearColor Bread(0.70f, 0.48f, 0.22f);
	const FLinearColor Mustard(0.80f, 0.62f, 0.08f);

	// The seller stands this far behind the cart's middle; customers this far in front (cm).
	constexpr double SellerBack = 70.0;
	constexpr double CustomerFront = 130.0;
	// A standing person's middle is this high above the ground.
	constexpr double PersonMiddle = 90.0;
}

AFoodVendor::AFoodVendor()
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

void AFoodVendor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Seller)
	{
		Seller->Destroy();
	}
	Super::EndPlay(EndPlayReason);
}

FVector AFoodVendor::GetOrderPoint() const
{
	return GetActorTransform().TransformPosition(FVector(FoodVendor::CustomerFront, 0.0, 0.0));
}

void AFoodVendor::Add(int32 Shape, const FLinearColor& Color, const FVector& At, const FVector& Size, const FRotator& Turn)
{
	if (!Shapes.IsValidIndex(Shape) || !Shapes[Shape])
	{
		return;
	}
	UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(this);
	Part->SetupAttachment(RootComponent);
	Part->SetStaticMesh(Shapes[Shape]);
	Part->SetRelativeLocationAndRotation(At, Turn);
	Part->SetRelativeScale3D(Size / 100.0);
	Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (BaseMaterial)
	{
		UMaterialInstanceDynamic* Paint = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		Paint->SetVectorParameterValue(TEXT("Color"), Color);
		Part->SetMaterial(0, Paint);
	}
	Part->RegisterComponent();
	AddInstanceComponent(Part);
}

void AFoodVendor::Sign(const FString& Words, const FVector& At)
{
	// A painted board facing the customers, the words on its front.
	Add(FoodVendor::Cube, FoodVendor::Cloth, At, FVector(2.0, 90.0, 24.0));
	UTextRenderComponent* Text = NewObject<UTextRenderComponent>(this);
	Text->SetupAttachment(RootComponent);
	Text->SetRelativeLocation(At + FVector(1.5, 0.0, 0.0));
	Text->SetText(FText::FromString(Words));
	Text->SetHorizontalAlignment(EHTA_Center);
	Text->SetVerticalAlignment(EVRTA_TextCenter);
	Text->SetWorldSize(15.f);
	Text->SetTextRenderColor(FColor(120, 20, 10));
	Text->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Text->RegisterComponent();
	AddInstanceComponent(Text);
}

void AFoodVendor::Setup(EFoodMenu InMenu)
{
	Menu = InMenu;
	FRandomStream Random(GetTypeHash(GetActorLocation()));
	PersonKit::FOutfit Outfit = PersonModel::RandomOutfit(Random);
	Outfit.Held = PersonKit::EHeld::None;
	Outfit.bApron = true;
	switch (Menu)
	{
	case EFoodMenu::Sausages:
		BuildSausages();
		Outfit.bWoman = false;
		Outfit.Hat = PersonKit::EHat::None;
		Outfit.Top = PersonKit::ETop::Jacket;
		Outfit.Coat = FLinearColor(0.80f, 0.78f, 0.72f);
		Outfit.bTie = false;
		Outfit.bMoustache = true;
		break;
	case EFoodMenu::Pretzels:
		BuildPretzels();
		Outfit.bWoman = true;
		Outfit.Height = 162.f;
		Outfit.Hat = PersonKit::EHat::None;
		Outfit.Top = PersonKit::ETop::Dress;
		Outfit.Coat = FLinearColor(0.16f, 0.12f, 0.18f);
		Outfit.bScarf = true;
		Outfit.Scarf = FLinearColor(0.45f, 0.10f, 0.08f);
		break;
	default:
		BuildChestnuts();
		Outfit.bWoman = false;
		Outfit.Hat = PersonKit::EHat::FlatCap;
		Outfit.Top = PersonKit::ETop::Jacket;
		Outfit.bTie = false;
		Outfit.bScarf = true;
		Outfit.bBoots = true;
		Outfit.bMoustache = true;
		Outfit.Apron = FLinearColor(0.30f, 0.26f, 0.20f);
		break;
	}

	const FVector At = GetActorTransform().TransformPosition(FVector(-FoodVendor::SellerBack, 0.0, FoodVendor::PersonMiddle));
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Seller = GetWorld()->SpawnActor<AStoryPerson>(AStoryPerson::StaticClass(), At, GetActorRotation(), Params);
	if (Seller)
	{
		Seller->SetOutfit(Outfit);
	}
}

void AFoodVendor::BuildChestnuts()
{
	using namespace FoodVendor;
	// The iron stove on three legs, a glowing fire door at the front, the
	// roasting pan on top heaped with chestnuts, and a stovepipe behind.
	for (int32 i = 0; i < 3; ++i)
	{
		const double A = 2.0 * PI * i / 3.0;
		Add(Cylinder, Iron, FVector(20.0 + 22.0 * FMath::Cos(A), 22.0 * FMath::Sin(A), 12.0), FVector(4.0, 4.0, 24.0));
	}
	Add(Cylinder, Iron, FVector(20.0, 0.0, 58.0), FVector(56.0, 56.0, 68.0));
	Add(Cube, Fire, FVector(47.0, 0.0, 45.0), FVector(4.0, 22.0, 16.0));
	Add(Cylinder, Iron, FVector(20.0, 0.0, 95.0), FVector(70.0, 70.0, 6.0));
	for (int32 i = 0; i < 14; ++i)
	{
		const double A = i * 2.4;
		const double R = 6.0 + 2.0 * i;
		Add(Sphere, Chestnut, FVector(20.0 + R * FMath::Cos(A), R * FMath::Sin(A), 100.0), FVector(8.0, 8.0, 6.0));
	}
	Add(Cylinder, Iron, FVector(0.0, -18.0, 140.0), FVector(9.0, 9.0, 80.0));
	Add(Cone, Iron, FVector(0.0, -18.0, 186.0), FVector(18.0, 18.0, 10.0));
	// A low table beside it with baked pumpkin and a stack of paper cones.
	Add(Cube, Wood, FVector(20.0, 62.0, 60.0), FVector(50.0, 50.0, 4.0));
	for (const double Y : { 50.0, 74.0 })
	{
		Add(Cube, DarkWood, FVector(20.0, Y, 29.0), FVector(4.0, 4.0, 58.0));
	}
	for (int32 i = 0; i < 3; ++i)
	{
		Add(Cube, Pumpkin, FVector(28.0 - i * 9.0, 58.0 + i * 5.0, 65.0), FVector(20.0, 8.0, 5.0), FRotator(0.0, 20.0, 0.0));
	}
	Add(Cone, Cloth, FVector(8.0, 76.0, 70.0), FVector(10.0, 10.0, 18.0), FRotator(180.0, 0.0, 0.0));
	Sign(TEXT("GESZTENYE"), FVector(45.0, 62.0, 80.0));

	// The fire glows.
	UPointLightComponent* Glow = NewObject<UPointLightComponent>(this);
	Glow->SetupAttachment(RootComponent);
	Glow->SetRelativeLocation(FVector(60.0, 0.0, 45.0));
	Glow->SetLightColor(FLinearColor(1.0f, 0.45f, 0.12f));
	Glow->SetIntensity(1500.f);
	Glow->SetAttenuationRadius(260.f);
	Glow->SetCastShadows(false);
	Glow->RegisterComponent();
	AddInstanceComponent(Glow);
}

void AFoodVendor::BuildSausages()
{
	using namespace FoodVendor;
	// A green handcart on two big wheels with a shelf on top: the copper
	// kettle of hot water for the sausages, a basket of rolls, mustard.
	Add(Cube, Green, FVector(10.0, 0.0, 60.0), FVector(70.0, 110.0, 50.0));
	Add(Cube, Wood, FVector(10.0, 0.0, 88.0), FVector(80.0, 120.0, 5.0));
	for (const double Y : { -62.0, 62.0 })
	{
		Add(Cylinder, DarkWood, FVector(10.0, Y, 38.0), FVector(76.0, 76.0, 5.0), FRotator(0.0, 0.0, 90.0));
		Add(Cylinder, Iron, FVector(10.0, Y, 38.0), FVector(12.0, 12.0, 8.0), FRotator(0.0, 0.0, 90.0));
	}
	Add(Cylinder, DarkWood, FVector(-45.0, 0.0, 80.0), FVector(4.0, 4.0, 110.0), FRotator(0.0, 0.0, 90.0));
	Add(Cylinder, Copper, FVector(10.0, -25.0, 108.0), FVector(40.0, 40.0, 36.0));
	Add(Cylinder, Copper, FVector(10.0, -25.0, 128.0), FVector(42.0, 42.0, 4.0));
	Add(Sphere, Iron, FVector(10.0, -25.0, 132.0), FVector(8.0, 8.0, 6.0));
	Add(Cylinder, Basket, FVector(15.0, 30.0, 98.0), FVector(36.0, 36.0, 14.0));
	for (int32 i = 0; i < 4; ++i)
	{
		Add(Sphere, Bread, FVector(10.0 + (i % 2) * 10.0, 24.0 + (i / 2) * 12.0, 108.0), FVector(14.0, 10.0, 8.0));
	}
	Add(Cylinder, Mustard, FVector(35.0, 5.0, 96.0), FVector(9.0, 9.0, 12.0));
	Sign(TEXT("VIRSLI"), FVector(46.0, 0.0, 60.0));
}

void AFoodVendor::BuildPretzels()
{
	using namespace FoodVendor;
	// A tall pole with a cross bar hung with pretzels, and a big basket
	// of them on a stool.
	Add(Cylinder, DarkWood, FVector(10.0, -40.0, 90.0), FVector(5.0, 5.0, 180.0));
	Add(Cylinder, DarkWood, FVector(10.0, -40.0, 170.0), FVector(4.0, 4.0, 80.0), FRotator(0.0, 0.0, 90.0));
	for (int32 i = 0; i < 6; ++i)
	{
		const double Y = -40.0 + (i % 2 ? 1.0 : -1.0) * (8.0 + 12.0 * (i / 2));
		Add(Cylinder, Pretzel, FVector(10.0, Y, 156.0 - (i % 3) * 18.0), FVector(16.0, 16.0, 3.0), FRotator(0.0, 0.0, 90.0));
	}
	for (const double X : { -5.0, 25.0 })
	{
		for (const double Y : { 15.0, 45.0 })
		{
			Add(Cube, DarkWood, FVector(X, Y, 22.0), FVector(4.0, 4.0, 44.0));
		}
	}
	Add(Cube, Wood, FVector(10.0, 30.0, 46.0), FVector(44.0, 44.0, 4.0));
	Add(Cylinder, Basket, FVector(10.0, 30.0, 58.0), FVector(52.0, 52.0, 20.0));
	for (int32 i = 0; i < 6; ++i)
	{
		const double A = i * 1.05;
		Add(Cylinder, Pretzel, FVector(10.0 + 12.0 * FMath::Cos(A), 30.0 + 12.0 * FMath::Sin(A), 70.0), FVector(16.0, 16.0, 3.0),
			FRotator(20.0, i * 60.0, 0.0));
	}
	Sign(TEXT("PEREC"), FVector(36.0, 30.0, 40.0));
}
