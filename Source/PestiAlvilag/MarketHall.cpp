// Fill out your copyright notice in the Description page of Project Settings.

#include "MarketHall.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/TextRenderComponent.h"
#include "DayNight.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture.h"
#include "Engine/World.h"
#include "FleaMarket.h"
#include "HallMarketModel.h"
#include "ChaseModel.h"
#include "MarketHallModel.h"
#include "MarketModel.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ProceduralMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

namespace MarketHallFill
{
	// The goods are drawn up to this far away (cm).
	constexpr int32 GoodsDrawDistance = 12000;
	// The stalls' goods stand this far above the floor (cm).
	constexpr double FloorUp = 3.0;
	// The hanging lamps.
	const FLinearColor LampIron(0.05f, 0.05f, 0.05f);
	const FLinearColor LampShade(0.08f, 0.18f, 0.12f);
	const FLinearColor LampGlobe(0.95f, 0.88f, 0.70f);
	constexpr float LampGlow = 25.f;

	const TCHAR* GoodsName(MarketModel::EGoods Goods)
	{
		switch (Goods)
		{
		case MarketModel::EGoods::Clothes: return TEXT("clothes");
		case MarketModel::EGoods::Shoes: return TEXT("shoes");
		case MarketModel::EGoods::Pots: return TEXT("pots");
		case MarketModel::EGoods::Produce: return TEXT("produce");
		default: return TEXT("crockery");
		}
	}
	// Looking for the ground under the things in the pickpocket's way, this far up and down (cm).
	constexpr double GroundLook = 300.0;
}

AMarketHall::AMarketHall()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Mesh"));
	Mesh->bUseComplexAsSimpleCollision = true;
	Mesh->SetCollisionProfileName(TEXT("BlockAll"));
	RootComponent = Mesh;

	Overhead = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Overhead"));
	Overhead->SetupAttachment(Mesh);
	Overhead->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Glazing = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Glazing"));
	Glazing->SetupAttachment(Mesh);
	Glazing->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Glazing->SetCastShadow(false);
	Glazing->bAffectDynamicIndirectLighting = false;
	Glazing->bAffectDistanceFieldLighting = false;
	Glazing->bVisibleInRayTracing = false;

	// Gold letters on the dark green board, facing the square (the front looks along -X).
	NameText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("NameText"));
	NameText->SetupAttachment(Mesh);
	NameText->SetRelativeLocationAndRotation(MarketHall::NameTextAt, FRotator(0.0, 180.0, 0.0));
	NameText->SetHorizontalAlignment(EHTA_Center);
	NameText->SetVerticalAlignment(EVRTA_TextCenter);
	NameText->SetWorldSize(70.f);
	NameText->SetTextRenderColor(FColor(216, 180, 90));
	NameText->SetText(FText::FromString(TEXT("V\u00c1S\u00c1RCSARNOK")));

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicShapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	BaseMaterial = BasicShapeMaterial.Object;
	// The engine's material for 3D widgets: unlit and see-through, its colour
	// and opacity set from code. Only the window panes use it.
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Translucent(TEXT("/Engine/EngineMaterials/Widget3DPassThrough_Translucent.Widget3DPassThrough_Translucent"));
	SeeThroughMaterial = Translucent.Object;
	static ConstructorHelpers::FObjectFinder<UTexture> White(TEXT("/Engine/EngineResources/WhiteSquareTexture.WhiteSquareTexture"));
	WhiteTexture = White.Object;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMesh(TEXT("/Engine/BasicShapes/Cone.Cone"));
	Shapes = { CubeMesh.Object, CylinderMesh.Object, SphereMesh.Object, ConeMesh.Object };
}

FVector2D AMarketHall::FrontAt()
{
	// The middle of the main front, from the hall's outline in OpenStreetMap (r4514296).
	return FVector2D(18109.0, 40941.0);
}

double AMarketHall::Yaw()
{
	// The hall stands square to Rákóczi tér, turned from the map's east towards north.
	return -19.7;
}

FVector2D AMarketHall::ToCity(const FVector2D& Local)
{
	const double A = FMath::DegreesToRadians(Yaw());
	return FrontAt() + FVector2D(Local.X * FMath::Cos(A) - Local.Y * FMath::Sin(A), Local.X * FMath::Sin(A) + Local.Y * FMath::Cos(A));
}

void AMarketHall::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Build();
}

void AMarketHall::BeginPlay()
{
	Super::BeginPlay();
	Fill();
}

void AMarketHall::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Market)
	{
		Market->Destroy();
		Market = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}

void AMarketHall::Build()
{
	using namespace MarketHall;

	FBuilder B;
	MarketHall::Build(B);

	Mesh->ClearAllMeshSections();
	Overhead->ClearAllMeshSections();
	Glazing->ClearAllMeshSections();
	int32 NumTriangles = 0;
	for (int32 i = 0; i < PartCount; ++i)
	{
		UProceduralMeshComponent* Target = Mesh;
		int32 Section = i;
		if (i >= FirstGlazingPart)
		{
			Target = Glazing;
			Section = i - FirstGlazingPart;
		}
		else if (i >= FirstOverheadPart)
		{
			Target = Overhead;
			Section = i - FirstOverheadPart;
		}
		const FPart& Part = B.Parts[i];
		Target->CreateMeshSection_LinearColor(Section, Part.Vertices, Part.Triangles, Part.Normals,
			TArray<FVector2D>(), TArray<FLinearColor>(), TArray<FProcMeshTangent>(), Target == Mesh);
		if (i >= FirstSeeThroughPart && SeeThroughMaterial)
		{
			UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(SeeThroughMaterial, this);
			if (WhiteTexture)
			{
				Material->SetTextureParameterValue(TEXT("SlateUI"), WhiteTexture);
			}
			Material->SetScalarParameterValue(TEXT("OpacityFromTexture"), 0.f);
			Material->SetVectorParameterValue(TEXT("TintColorAndOpacity"), PartColor(i));
			// Unlit, so it is darkened with the night instead of shining.
			if (ADayNight* Clock = ADayNight::Get(GetWorld()))
			{
				Clock->AddSeeThrough(Material, PartColor(i));
			}
			Target->SetMaterial(Section, Material);
		}
		else if (i >= FirstSeeThroughPart)
		{
			// No see-through material: leave the panes out, as clear glass.
			Target->ClearMeshSection(Section);
		}
		else if (BaseMaterial)
		{
			UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(BaseMaterial, this);
			Material->SetVectorParameterValue(TEXT("Color"), PartColor(i));
			Target->SetMaterial(Section, Material);
		}
		NumTriangles += Part.Triangles.Num() / 3;
	}
	UE_LOG(LogTemp, Log, TEXT("MarketHall: %d triangles"), NumTriangles);
}

void AMarketHall::Fill()
{
	// The market inside: the stalls, sellers and shoppers, brought to life
	// by a flea market of their own (as at Teleki tér) on the hall's floor.
	TArray<HallMarket::FStall> Stalls;
	TArray<HallMarket::FShopper> Shoppers;
	TArray<HallMarket::FLamp> Lamps;
	HallMarket::Layout(Stalls, Shoppers, Lamps);
	const FTransform& Frame = GetActorTransform();
	const double HallYaw = GetActorRotation().Yaw;
	const TSharedRef<FJsonObject> Inside = MakeShared<FJsonObject>();
	TArray<TSharedPtr<FJsonValue>> StallValues;
	for (const HallMarket::FStall& Stall : Stalls)
	{
		const FVector At = Frame.TransformPosition(Stall.At);
		const TSharedRef<FJsonObject> Object = MakeShared<FJsonObject>();
		TArray<TSharedPtr<FJsonValue>> Place;
		Place.Add(MakeShared<FJsonValueNumber>(At.X));
		Place.Add(MakeShared<FJsonValueNumber>(At.Y));
		Place.Add(MakeShared<FJsonValueNumber>(Stall.Yaw + HallYaw));
		Object->SetArrayField(TEXT("at"), Place);
		Object->SetStringField(TEXT("build"), Stall.Build == MarketModel::EBuild::Blanket ? TEXT("blanket") : TEXT("table"));
		Object->SetStringField(TEXT("goods"), MarketHallFill::GoodsName(Stall.Goods));
		Object->SetBoolField(TEXT("seller"), Stall.bSeller);
		StallValues.Add(MakeShared<FJsonValueObject>(Object));
	}
	TArray<TSharedPtr<FJsonValue>> ShopperValues;
	for (const HallMarket::FShopper& Shopper : Shoppers)
	{
		const FVector At = Frame.TransformPosition(Shopper.At);
		TArray<TSharedPtr<FJsonValue>> Place;
		Place.Add(MakeShared<FJsonValueNumber>(At.X));
		Place.Add(MakeShared<FJsonValueNumber>(At.Y));
		Place.Add(MakeShared<FJsonValueNumber>(Shopper.Yaw + HallYaw));
		ShopperValues.Add(MakeShared<FJsonValueArray>(Place));
	}
	Inside->SetArrayField(TEXT("stalls"), StallValues);
	Inside->SetArrayField(TEXT("shoppers"), ShopperValues);
	Inside->SetNumberField(TEXT("ground_z"), Frame.GetLocation().Z + MarketHallFill::FloorUp);
	Market = GetWorld()->SpawnActor<AFleaMarket>(AFleaMarket::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
	if (Market)
	{
		Market->Setup(*Inside);
	}

	// The lamps hanging from the roof: a rod, a green enamel shade and a
	// globe that glows at night, when the nearest of them light the hall.
	TArray<MarketModel::FShape> Pieces;
	ADayNight* Clock = ADayNight::Get(GetWorld());
	for (const HallMarket::FLamp& Lamp : Lamps)
	{
		const double Rod = Lamp.Top - Lamp.At.Z;
		Pieces.Add({ 1, MarketHallFill::LampIron, Lamp.At + FVector(0.0, 0.0, Rod / 2.0), FVector(3.0, 3.0, Rod), 0.0, 0.0, 0.0, false });
		Pieces.Add({ 3, MarketHallFill::LampShade, Lamp.At + FVector(0.0, 0.0, 28.0), FVector(70.0, 70.0, 30.0), 0.0, 0.0, 0.0, false });
		Pieces.Add({ 2, MarketHallFill::LampGlobe, Lamp.At, FVector(34.0, 34.0, 34.0), 0.0, 0.0, 0.0, false });
		if (Clock)
		{
			Clock->AddLamp(Frame.TransformPosition(Lamp.At));
		}
	}

	// Deliveries out on the square in front of the gateway, and things in
	// the pickpocket's way: to run over and under. Out on the streets and in
	// the garden each stands on the ground there, which may lie a little
	// higher or lower than the hall.
	TArray<ChaseModel::FSpot> Spots = ChaseModel::HallYard();
	Spots.Append(ChaseModel::RouteSpots());
	FCollisionQueryParams GroundParams(SCENE_QUERY_STAT(MarketHallGround), false);
	for (int32 k = 0; k < Spots.Num(); ++k)
	{
		FVector At = Spots[k].At;
		const FVector InCity = Frame.TransformPosition(At);
		FHitResult Hit;
		if (GetWorld()->LineTraceSingleByChannel(Hit, InCity + FVector(0.0, 0.0, MarketHallFill::GroundLook), InCity - FVector(0.0, 0.0, MarketHallFill::GroundLook),
				ECC_WorldStatic, GroundParams))
		{
			At.Z = Hit.ImpactPoint.Z - Frame.GetLocation().Z;
		}
		MarketModel::FRandom Random(static_cast<uint32>(k * 13 + 5));
		ChaseModel::Build(Pieces, At, Spots[k].Yaw, Spots[k].Kind, Random);
	}
	for (const MarketModel::FShape& Piece : Pieces)
	{
		AddShape(Piece.Shape, Piece.Color, Piece.At, Piece.Size, FRotator(Piece.Pitch, Piece.Yaw, Piece.Roll), Piece.bCollide);
	}
	const FString GlobeKey = FString::Printf(TEXT("%d %s %d"), 2, *MarketHallFill::LampGlobe.ToString(), 0);
	if (const int32* Globes = BatchIndex.Find(GlobeKey); Globes && Clock)
	{
		Clock->AddNightGlow(Batches[*Globes], 0, MarketHallFill::LampGlobe, MarketHallFill::LampGlow);
	}
}

void AMarketHall::AddShape(int32 Shape, const FLinearColor& Color, const FVector& At, const FVector& Size, const FRotator& Turn, bool bCollide)
{
	if (!Shapes.IsValidIndex(Shape) || !Shapes[Shape])
	{
		return;
	}
	// One batch per shape, colour and collision; the instances are placed in the hall's frame.
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
		Instances->SetCullDistances(0, MarketHallFill::GoodsDrawDistance);
		Instances->SetCanEverAffectNavigation(false);
		Instances->RegisterComponent();
		BatchIndex.Add(Key, Batches.Add(Instances));
	}
	Instances->AddInstance(FTransform(Turn, At, Size / 100.0), false);
}
