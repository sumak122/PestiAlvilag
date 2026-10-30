// Fill out your copyright notice in the Description page of Project Settings.

#include "MarketHall.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture.h"
#include "Engine/World.h"
#include "ChaseModel.h"
#include "MarketHallModel.h"
#include "MarketModel.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PersonModel.h"
#include "ProceduralMeshComponent.h"
#include "StoryPerson.h"
#include "UObject/ConstructorHelpers.h"

namespace MarketHallFill
{
	// People are drawn up to this far away, the goods further (cm).
	constexpr float PeopleDrawDistance = 9000.f;
	constexpr int32 GoodsDrawDistance = 12000;
	// Most stalls have their seller behind them; some have a shopper in front.
	constexpr float SellerChance = 0.75f;
	constexpr float ShopperChance = 0.3f;
	constexpr double ShopperOut = 150.0;
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
	for (AStoryPerson* Person : People)
	{
		if (Person)
		{
			Person->Destroy();
		}
	}
	People.Reset();
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
	TArray<MarketHall::FStall> Stalls;
	MarketHall::StallSpots(Stalls);
	TArray<MarketModel::FShape> Pieces;
	FRandomStream PeopleRandom(1897);
	for (int32 k = 0; k < Stalls.Num(); ++k)
	{
		const MarketHall::FStall& Stall = Stalls[k];
		MarketModel::FRandom Random(static_cast<uint32>(k * 7 + 3));
		// Mostly vegetables and fruit; here and there pots and pans or crockery.
		const double Kind = Random.Unit();
		const MarketModel::EGoods Goods = Kind < 0.75 ? MarketModel::EGoods::Produce : Kind < 0.88 ? MarketModel::EGoods::Crockery : MarketModel::EGoods::Pots;
		const FVector Middle = Stall.At + FVector(0.0, 0.0, 3.0);
		const double Back = MarketModel::BuildStall(Pieces, Middle, Stall.Yaw, MarketModel::EBuild::Table, Goods, Random);
		const double A = FMath::DegreesToRadians(Stall.Yaw);
		const FVector Forward(FMath::Cos(A), FMath::Sin(A), 0.0);
		const FVector Side(-Forward.Y, Forward.X, 0.0);
		if (PeopleRandom.FRand() < MarketHallFill::SellerChance)
		{
			SpawnPerson(Middle - Forward * Back + Side * PeopleRandom.FRandRange(-60.f, 60.f), Stall.Yaw, PeopleRandom, true);
		}
		if (PeopleRandom.FRand() < MarketHallFill::ShopperChance)
		{
			SpawnPerson(Middle + Forward * MarketHallFill::ShopperOut + Side * PeopleRandom.FRandRange(-80.f, 80.f),
				Stall.Yaw + 180.0 + PeopleRandom.FRandRange(-25.f, 25.f), PeopleRandom, false);
		}
	}
	// Deliveries out on the square in front of the gateway, and things in
	// the pickpocket's way: to run over and under. Out on the streets and in
	// the garden each stands on the ground there, which may lie a little
	// higher or lower than the hall.
	TArray<ChaseModel::FSpot> Spots = ChaseModel::HallYard();
	Spots.Append(ChaseModel::RouteSpots());
	const FTransform& Frame = GetActorTransform();
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

AStoryPerson* AMarketHall::SpawnPerson(const FVector& LocalFeet, double LocalYaw, FRandomStream& Random, bool bSeller)
{
	PersonKit::FOutfit Outfit = PersonModel::RandomOutfit(Random);
	if (bSeller)
	{
		// Sellers in working clothes: aprons, caps or headscarves, nothing in hand.
		Outfit.Held = PersonKit::EHeld::None;
		Outfit.bApron = Random.FRand() < 0.6;
		if (!Outfit.bWoman && Random.FRand() < 0.6)
		{
			Outfit.Hat = PersonKit::EHat::FlatCap;
		}
	}
	else if (Random.FRand() < 0.5)
	{
		Outfit.Held = PersonKit::EHeld::Basket;
	}

	const FTransform& Hall = GetActorTransform();
	const FVector Feet = Hall.TransformPosition(LocalFeet);
	const FTransform Spawn(FRotator(0.0, LocalYaw + GetActorRotation().Yaw, 0.0), Feet + FVector(0.0, 0.0, PersonModel::MiddleHeight));
	AStoryPerson* Person = GetWorld()->SpawnActorDeferred<AStoryPerson>(AStoryPerson::StaticClass(), Spawn, this, nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Person)
	{
		return nullptr;
	}
	Person->SetOutfit(Outfit);
	Person->FinishSpawning(Spawn);
	// They stand at the stalls: no need to think every frame, nor to be
	// drawn from the other end of the district.
	Person->SetActorTickEnabled(false);
	TInlineComponentArray<UPrimitiveComponent*> Parts(Person);
	for (UPrimitiveComponent* Part : Parts)
	{
		Part->SetCachedMaxDrawDistance(MarketHallFill::PeopleDrawDistance);
	}
	People.Add(Person);
	return Person;
}
