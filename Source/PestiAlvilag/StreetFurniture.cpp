// Fill out your copyright notice in the Description page of Project Settings.

#include "StreetFurniture.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/TextRenderComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/StaticMesh.h"
#include "DayNight.h"
#include "Engine/World.h"
#include "FallingLeaves.h"
#include "FleaMarket.h"
#include "FoodVendor.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/PlayerCameraManager.h"
#include "Lighters.h"
#include "Math/RotationMatrix.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "OsmGraybox.h"
#include "ParkKeeper.h"
#include "ParkModel.h"
#include "PersonModel.h"
#include "PestiTaxi.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "ShopModel.h"
#include "StoryPerson.h"
#include "Tram.h"
#include "PestiCar.h"
#include "Wreck.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogFurniture, Log, All);

/** One engine shape of a piece of furniture (cm, X forward, Z up from the ground). */
struct FFurniturePart
{
	int32 Shape;
	FLinearColor Color;
	FVector Location;
	FVector Scale;
	bool bCollide;
	FRotator Rotation = FRotator::ZeroRotator;
};

namespace Furniture
{
	enum EShape : int32 { Cube, Cylinder, Sphere, Cone, NumShapes };

	const FLinearColor Iron(0.035f, 0.05f, 0.04f);
	const FLinearColor Lantern(0.95f, 0.85f, 0.55f);
	// At night the lanterns burn with a warm gas flame, and the shops behind
	// their windows are lit (as bright as this many lit grey walls).
	const FLinearColor GasFlame(1.f, 0.74f, 0.40f);
	constexpr float LanternGlow = 25.f;
	const FLinearColor ShopLight(1.f, 0.70f, 0.38f);
	constexpr float ShopGlow = 5.f;
	const FLinearColor Wood(0.30f, 0.17f, 0.08f);
	const FLinearColor Stone(0.42f, 0.40f, 0.36f);
	const FLinearColor Gravel(0.55f, 0.50f, 0.40f);
	const FLinearColor Bark(0.12f, 0.09f, 0.07f);
	const FLinearColor Flowers(0.45f, 0.06f, 0.05f);
	const FLinearColor Lawn(0.09f, 0.22f, 0.05f);
	// Faded posters on the advertising columns.
	const FLinearColor Posters[] = {
		FLinearColor(0.72f, 0.66f, 0.50f),
		FLinearColor(0.55f, 0.22f, 0.16f),
		FLinearColor(0.68f, 0.54f, 0.22f),
	};

	// The period street lamp: a slim pole on a heavier base, a scrolled
	// crook reaching over the road and a bell lantern hanging from it.
	const TArray<FFurniturePart> Lamp = {
		{ Cylinder, Iron, FVector(0, 0, 35), FVector(0.28, 0.28, 0.7), true },
		{ Cylinder, Iron, FVector(0, 0, 350), FVector(0.1, 0.1, 5.6), true },
		{ Cone, Iron, FVector(0, 0, 650), FVector(0.1, 0.1, 0.2), false },
		{ Cube, Iron, FVector(28, 0, 615), FVector(0.6, 0.05, 0.05), false },
		{ Cube, Iron, FVector(58, 0, 628), FVector(0.05, 0.05, 0.28), false },
		{ Cube, Iron, FVector(48, 0, 642), FVector(0.2, 0.05, 0.05), false },
		{ Cube, Iron, FVector(50, 0, 598), FVector(0.03, 0.03, 0.3), false },
		{ Cone, Iron, FVector(50, 0, 570), FVector(0.36, 0.36, 0.28), false },
		{ Cube, Lantern, FVector(50, 0, 552), FVector(0.18, 0.18, 0.14), false },
	};

	// The park bench: two cast-iron side frames, each a front leg, a back
	// leg that carries on up as the leaning back support, a rail under the
	// seat and an armrest, with wooden slats laid across them, four in the
	// seat and three screwed to the front of the back supports.
	constexpr double BenchBackLean = 12.8;

	TArray<FFurniturePart> MakeBench()
	{
		TArray<FFurniturePart> Parts;
		const double Lean = FMath::Tan(FMath::DegreesToRadians(BenchBackLean));
		const FRotator BackTilt(BenchBackLean, 0.0, 0.0);
		for (double Y : { -80.0, 80.0 })
		{
			Parts.Add({ Cube, Iron, FVector(17, Y, 22), FVector(0.05, 0.05, 0.44), true });
			Parts.Add({ Cube, Iron, FVector(-17, Y, 22), FVector(0.05, 0.05, 0.44), true });
			Parts.Add({ Cube, Iron, FVector(18, Y, 1.5), FVector(0.1, 0.08, 0.03), false });
			Parts.Add({ Cube, Iron, FVector(-18, Y, 1.5), FVector(0.1, 0.08, 0.03), false });
			Parts.Add({ Cube, Iron, FVector(0, Y, 42), FVector(0.46, 0.05, 0.05), false });
			FFurniturePart Upright{ Cube, Iron, FVector(-17 - 22 * Lean, Y, 66), FVector(0.05, 0.05, 0.46), false };
			Upright.Rotation = BackTilt;
			Parts.Add(Upright);
			Parts.Add({ Cube, Iron, FVector(20, Y, 55), FVector(0.04, 0.04, 0.22), false });
			Parts.Add({ Cube, Iron, FVector(1, Y, 67), FVector(0.46, 0.06, 0.04), false });
			Parts.Add({ Sphere, Iron, FVector(23, Y, 67), FVector(0.07, 0.07, 0.06), false });
		}
		Parts.Add({ Cube, Iron, FVector(-17, 0, 12), FVector(0.04, 1.6, 0.04), false });
		Parts.Add({ Cube, Iron, FVector(0, 0, 42), FVector(0.44, 0.04, 0.04), false });
		for (double X : { -15.0, -5.0, 5.0, 15.0 })
		{
			Parts.Add({ Cube, Wood, FVector(X, 0, 46), FVector(0.09, 1.86, 0.03), X == -5.0 });
		}
		for (double Z : { 56.0, 68.0, 80.0 })
		{
			FFurniturePart Slat{ Cube, Wood, FVector(-17 - (Z - 44) * Lean + 3.75, 0, Z), FVector(0.025, 1.86, 0.09), Z == 68.0 };
			Slat.Rotation = BackTilt;
			Parts.Add(Slat);
		}
		return Parts;
	}

	const TArray<FFurniturePart> Bench = MakeBench();

	// Baross tér's lamp (Fortepan 139746): the street lamp's pole with a
	// crook and a lantern reaching out to each side.
	TArray<FFurniturePart> MakeTwinLamp()
	{
		TArray<FFurniturePart> Parts = Lamp;
		constexpr int32 FirstArmPart = 3;
		for (int32 i = FirstArmPart; i < Lamp.Num(); ++i)
		{
			FFurniturePart Other = Lamp[i];
			Other.Location.X = -Other.Location.X;
			Parts.Add(Other);
		}
		return Parts;
	}

	const TArray<FFurniturePart> TwinLamp = MakeTwinLamp();

	// A litter bin: a round sheet-metal basket in dark green, hung on the
	// side of a short iron post, open at the top.
	const FLinearColor BinPaint(0.05f, 0.13f, 0.07f);
	const FLinearColor BinInside(0.02f, 0.02f, 0.02f);
	const TArray<FFurniturePart> Bin = {
		{ Cylinder, Iron, FVector(0, 0, 50), FVector(0.07, 0.07, 1.0), true },
		{ Sphere, Iron, FVector(0, 0, 101), FVector(0.1, 0.1, 0.1), false },
		{ Cylinder, BinPaint, FVector(24, 0, 63), FVector(0.4, 0.4, 0.56), true },
		{ Cylinder, Iron, FVector(24, 0, 34), FVector(0.42, 0.42, 0.03), false },
		{ Cylinder, Iron, FVector(24, 0, 90), FVector(0.44, 0.44, 0.04), false },
		{ Cylinder, BinInside, FVector(24, 0, 91.5), FVector(0.36, 0.36, 0.02), false },
		{ Cube, Iron, FVector(12, 0, 80), FVector(0.24, 0.05, 0.05), false },
	};

	// A taxi stand sign: a post with a yellow plate, chequered black at the
	// ends like the taxis' band; "TAXI" is written on it separately.
	const FLinearColor TaxiYellow(0.85f, 0.62f, 0.05f);
	const FLinearColor TaxiBlack(0.01f, 0.01f, 0.01f);
	const TArray<FFurniturePart> TaxiSign = {
		{ Cylinder, Iron, FVector(0, 0, 130), FVector(0.07, 0.07, 2.6), true },
		{ Sphere, Iron, FVector(0, 0, 262), FVector(0.1, 0.1, 0.1), false },
		{ Cube, TaxiYellow, FVector(0, 0, 230), FVector(0.03, 0.8, 0.32), false },
		{ Cube, TaxiBlack, FVector(0, -34, 238), FVector(0.035, 0.12, 0.08), false },
		{ Cube, TaxiBlack, FVector(0, -46, 222), FVector(0.035, 0.12, 0.08), false },
		{ Cube, TaxiBlack, FVector(0, 34, 222), FVector(0.035, 0.12, 0.08), false },
		{ Cube, TaxiBlack, FVector(0, 46, 238), FVector(0.035, 0.12, 0.08), false },
	};
	constexpr double TaxiSignTextZ = 230.0;

	// A tram stop sign ("megallo"): a post with a white enamel plate on top
	// edged in red, the word and the stop's name written on it separately.
	const FLinearColor Enamel(0.85f, 0.85f, 0.80f);
	const FLinearColor StopRed(0.45f, 0.03f, 0.02f);
	const TArray<FFurniturePart> TramStopSign = {
		{ Cylinder, Iron, FVector(0, 0, 110), FVector(0.07, 0.07, 2.2), true },
		{ Cube, Iron, FVector(0, 0, 221), FVector(0.06, 0.3, 0.03), false },
		{ Cube, StopRed, FVector(0, 0, 245), FVector(0.03, 0.9, 0.5), false },
		{ Cube, Enamel, FVector(0, 0, 245), FVector(0.036, 0.84, 0.44), false },
	};
	constexpr double TramStopWordZ = 256.0;
	constexpr double TramStopNameZ = 234.0;
	constexpr double TaxiLift = 150.0;

	// The round advertising column; the poster drum comes in three colours.
	TArray<FFurniturePart> Column(int32 Variant)
	{
		return {
			{ Cylinder, Iron, FVector(0, 0, 15), FVector(1.4, 1.4, 0.3), true },
			{ Cylinder, Posters[Variant % UE_ARRAY_COUNT(Posters)], FVector(0, 0, 165), FVector(1.15, 1.15, 2.7), true },
			{ Cylinder, Iron, FVector(0, 0, 307), FVector(1.4, 1.4, 0.14), false },
			{ Sphere, Iron, FVector(0, 0, 314), FVector(1.3, 1.3, 0.7), false },
			{ Cylinder, Iron, FVector(0, 0, 365), FVector(0.08, 0.08, 0.4), false },
			{ Sphere, Iron, FVector(0, 0, 388), FVector(0.16, 0.16, 0.16), false },
		};
	}

	// Autumn leaves, on the trees and fallen.
	const FLinearColor LeafGreen(0.12f, 0.19f, 0.04f);
	const FLinearColor LeafYellow(0.40f, 0.32f, 0.04f);
	const FLinearColor LeafGold(0.50f, 0.26f, 0.03f);
	const FLinearColor LeafOrange(0.45f, 0.12f, 0.02f);
	const FLinearColor LeafRust(0.24f, 0.07f, 0.02f);
	const FLinearColor Fallen(0.26f, 0.11f, 0.03f);
	const FLinearColor FallenGold(0.38f, 0.21f, 0.03f);

	// A round shape stretched from From along Dir (a branch or a trunk).
	FFurniturePart Limb(const FVector& From, const FVector& Dir, double Length, double Radius, bool bCollide = false)
	{
		const FVector Along = Dir.GetSafeNormal();
		FFurniturePart Part{ Cylinder, Bark, From + Along * (Length * 0.5), FVector(Radius / 50.0, Radius / 50.0, Length / 100.0), bCollide };
		Part.Rotation = FRotationMatrix::MakeFromZ(Along).Rotator();
		return Part;
	}

	// Leaning out at Tilt degrees from upright, towards Yaw.
	FVector Leaning(double Yaw, double Tilt)
	{
		const double Y = FMath::DegreesToRadians(Yaw);
		const double T = FMath::DegreesToRadians(Tilt);
		return FVector(FMath::Sin(T) * FMath::Cos(Y), FMath::Sin(T) * FMath::Sin(Y), FMath::Cos(T));
	}

	// The park trees in autumn, in four kinds: still full with yellowing
	// leaves, gone golden, thinning out (the branches show through a few
	// clumps of orange and rust), and already bare. Each stands on a carpet
	// of its fallen leaves; the barer the tree, the wider the carpet.
	enum ETree : int32 { TreeFull, TreeGolden, TreeThinning, TreeBare, NumTrees };

	TArray<FFurniturePart> MakeTree(int32 Kind)
	{
		TArray<FFurniturePart> Parts;
		Parts.Add({ Cylinder, Bark, FVector(0, 0, 12), FVector(0.62, 0.62, 0.24), true });
		Parts.Add(Limb(FVector(0, 0, 0), FVector::UpVector, 380.0, 20.0, true));
		Parts.Add(Limb(FVector(0, 0, 370), Leaning(20.0, 8.0), 230.0, 13.0));

		// Four main boughs, each forking once near its end.
		const double Twist = 25.0 * Kind;
		TArray<FVector> Tips;
		for (int32 i = 0; i < 4; ++i)
		{
			const double Yaw = Twist + 90.0 * i + (i % 2 ? 15.0 : -10.0);
			const double Tilt = 42.0 + 8.0 * (i % 3);
			const FVector From(0, 0, 330.0 + 25.0 * i);
			const FVector Dir = Leaning(Yaw, Tilt);
			const double Length = 250.0 - 20.0 * (i % 2);
			Parts.Add(Limb(From, Dir, Length, 8.0));
			const FVector Fork = From + Dir * (Length * 0.7);
			const FVector Tip = From + Dir * Length;
			Tips.Add(Tip);
			if (Kind >= TreeThinning)
			{
				// The twigs show only where the leaves are gone.
				Parts.Add(Limb(Fork, Leaning(Yaw + 35.0, Tilt - 18.0), 150.0, 4.0));
				Parts.Add(Limb(Fork, Leaning(Yaw - 35.0, Tilt - 12.0), 130.0, 3.5));
				Parts.Add(Limb(Tip, Leaning(Yaw, Tilt - 25.0), 90.0, 2.5));
			}
		}

		switch (Kind)
		{
		case TreeFull:
			Parts.Add({ Sphere, LeafYellow, FVector(0, 0, 620), FVector(5.4, 5.4, 4.3), false });
			Parts.Add({ Sphere, LeafGold, FVector(150, 80, 690), FVector(3.4, 3.4, 2.8), false });
			Parts.Add({ Sphere, LeafOrange, FVector(-170, 60, 600), FVector(3.2, 3.2, 2.6), false });
			Parts.Add({ Sphere, LeafGreen, FVector(40, -180, 570), FVector(3.0, 3.0, 2.4), false });
			Parts.Add({ Sphere, LeafGold, FVector(-40, 40, 770), FVector(2.8, 2.8, 2.2), false });
			Parts.Add({ Cylinder, Fallen, FVector(0, 0, 1), FVector(3.6, 3.0, 0.01), false });
			break;
		case TreeGolden:
			Parts.Add({ Sphere, LeafGold, FVector(0, 0, 610), FVector(5.2, 5.2, 4.1), false });
			Parts.Add({ Sphere, LeafYellow, FVector(160, -40, 680), FVector(3.4, 3.4, 2.8), false });
			Parts.Add({ Sphere, LeafOrange, FVector(-150, 120, 590), FVector(2.6, 2.6, 2.2), false });
			Parts.Add({ Sphere, LeafGold, FVector(0, -60, 770), FVector(2.8, 2.8, 2.2), false });
			Parts.Add({ Cylinder, FallenGold, FVector(0, 0, 1), FVector(4.0, 3.4, 0.01), false });
			break;
		case TreeThinning:
			Parts.Add({ Sphere, LeafOrange, Tips[0] + FVector(0, 0, 30), FVector(2.4, 2.4, 1.8), false });
			Parts.Add({ Sphere, LeafRust, Tips[1] + FVector(0, 0, 20), FVector(2.0, 2.0, 1.6), false });
			Parts.Add({ Sphere, LeafGold, Tips[3] + FVector(0, 0, 40), FVector(2.2, 2.2, 1.7), false });
			Parts.Add({ Cylinder, Fallen, FVector(0, 0, 1), FVector(5.0, 4.2, 0.01), false });
			Parts.Add({ Cylinder, FallenGold, FVector(90, -60, 1.2), FVector(3.0, 2.6, 0.01), false });
			break;
		default:
			Parts.Add({ Cylinder, Fallen, FVector(0, 0, 1), FVector(5.6, 4.8, 0.01), false });
			Parts.Add({ Cylinder, LeafRust, FVector(-80, 70, 1.2), FVector(3.4, 3.0, 0.01), false });
			break;
		}
		return Parts;
	}

	const TArray<FFurniturePart>& Tree(int32 Kind)
	{
		static TArray<TArray<FFurniturePart>> Kinds;
		if (Kinds.IsEmpty())
		{
			for (int32 i = 0; i < NumTrees; ++i)
			{
				Kinds.Add(MakeTree(i));
			}
		}
		return Kinds[FMath::Clamp(Kind, 0, NumTrees - 1)];
	}

	// Which kind a tree at this spot is: about a third still full, a quarter
	// golden, a quarter thinning, the rest bare.
	int32 TreeKindAt(const FVector2D& At)
	{
		const uint32 Roll = GetTypeHash(FIntPoint(FMath::RoundToInt(At.X), FMath::RoundToInt(At.Y))) % 100;
		return Roll < 35 ? TreeFull : Roll < 60 ? TreeGolden : Roll < 85 ? TreeThinning : TreeBare;
	}

	// A raked heap of leaves.
	const TArray<FFurniturePart> LeafPile = {
		{ Sphere, Fallen, FVector(0, 0, 8), FVector(1.7, 1.4, 0.55), false },
		{ Sphere, FallenGold, FVector(22, -15, 16), FVector(1.1, 1.0, 0.45), false },
		{ Sphere, LeafOrange, FVector(-25, 20, 12), FVector(0.9, 0.8, 0.4), false },
		{ Sphere, LeafRust, FVector(10, 32, 6), FVector(0.7, 0.7, 0.3), false },
	};

	// The low iron railing round the park.
	constexpr double RailingPostSpacing = 250.0;
	constexpr double RailingHeight = 90.0;

	// The wire fences along the lawns and the hedges round the round places.
	const FLinearColor Hedge(0.05f, 0.15f, 0.04f);
	constexpr double LawnFencePostSpacing = 200.0;
	constexpr double LawnFenceHeight = 45.0;
	constexpr double HedgeHeight = 70.0;
	constexpr double HedgeWidth = 55.0;

	/** A statue or playground piece from ParkModel, as furniture parts. */
	TArray<FFurniturePart> FromParkModel(const TArray<ParkModel::FPart>& Model)
	{
		TArray<FFurniturePart> Parts;
		Parts.Reserve(Model.Num());
		for (const ParkModel::FPart& Piece : Model)
		{
			FFurniturePart Part{ Piece.Shape, Piece.Color, Piece.At, Piece.Size / 100.0, Piece.bCollide };
			Part.Rotation = FRotator(Piece.Pitch, 0.0, Piece.Roll);
			Parts.Add(Part);
		}
		return Parts;
	}

	// Children and nannies are not drawn beyond this distance (cm).
	constexpr float PlaygroundPeopleDistance = 9000.f;
	// A tram stands this far above the rails: the half height of its box (ATram).
	constexpr double ParkedTramLift = 167.0;
	// Tram islands: low stone platforms beside the tracks (cm).
	constexpr double TramIslandHeight = 20.0;

	// Shop fronts: the pavement height is found this far out from the wall (cm).
	constexpr double ShopGroundProbe = 150.0;
	// Shop parts smaller than this (goods, handles) go with the small things (cm).
	constexpr double ShopSmallPart = 45.0;
	// Shop lettering is not drawn beyond this distance (cm).
	constexpr float ShopLetteringDistance = 12000.f;
	// A capital letter of the lettering is about this much of its height wide.
	constexpr double LetterWidth = 0.62;
	// Shopkeepers are not drawn beyond this distance (cm).
	constexpr float ShopkeeperDistance = 9000.f;

	// Benches and bins: a car faster than this smashes one it drives into
	// (cm/s). The pieces lie still after a while, and no more than so many
	// lie about. A smashed one stands again a minute later at the earliest,
	// once the camera is this far away (cm) and nobody sees it.
	constexpr double SmashSpeed = 250.0;
	constexpr double CarHalfLength = 215.0;
	constexpr double CarHalfWidth = 95.0;
	constexpr float CarSlowOnSmash = 0.9f;
	constexpr double SettleAfter = 8.0;
	constexpr int32 MaxWreckage = 600;
	constexpr double ResetDistance = 15000.0;
	constexpr double ResetAfter = 60.0;
	constexpr float SeenWithin = 1.f;
	constexpr int32 ResetChecksPerFrame = 4;
	// A bench this close to someone sitting stays whole.
	constexpr double SeatedNear = 150.0;
	// What a lighter lay on, within this far (cm).
	constexpr double LighterNear = 150.0;

	constexpr double BreakCellSize = 1000.0;
	FIntPoint BreakCell(const FVector2D& At)
	{
		return FIntPoint(FMath::FloorToInt(At.X / BreakCellSize), FMath::FloorToInt(At.Y / BreakCellSize));
	}
}

AStreetFurniture::AStreetFurniture()
{
	// Ticks to let a car smash the benches and bins.
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	BaseMaterial = Material.Object;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMesh(TEXT("/Engine/BasicShapes/Cone.Cone"));
	ShapeMeshes = { CubeMesh.Object, CylinderMesh.Object, SphereMesh.Object, ConeMesh.Object };
}

TArray<FTransform>& AStreetFurniture::Batch(int32 Shape, const FLinearColor& Color, bool bCollide, float Cull, int32* OutBatch)
{
	const FString Key = FString::Printf(TEXT("%d %s %d %.0f"), Shape, *Color.ToString(), bCollide ? 1 : 0, Cull);
	if (const int32* Found = BatchIndex.Find(Key))
	{
		if (OutBatch)
		{
			*OutBatch = *Found;
		}
		return Pending[*Found];
	}

	UHierarchicalInstancedStaticMeshComponent* Instances = NewObject<UHierarchicalInstancedStaticMeshComponent>(this, NAME_None, RF_Transient);
	Instances->SetupAttachment(RootComponent);
	Instances->SetStaticMesh(ShapeMeshes[Shape]);
	if (BaseMaterial)
	{
		UMaterialInstanceDynamic* Paintwork = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		Paintwork->SetVectorParameterValue(TEXT("Color"), Color);
		Instances->SetMaterial(0, Paintwork);
	}
	Instances->SetCollisionProfileName(bCollide ? TEXT("BlockAll") : TEXT("NoCollision"));
	Instances->SetCullDistances(0, static_cast<int32>(Cull));
	Instances->RegisterComponent();

	// Lanterns and the insides of the shops light up at dusk.
	if (Color.Equals(Furniture::Lantern) || Color.Equals(ShopModel::Colors::Inside))
	{
		if (ADayNight* Clock = ADayNight::Get(GetWorld()))
		{
			const bool bLantern = Color.Equals(Furniture::Lantern);
			Clock->AddNightGlow(Instances, 0, bLantern ? Furniture::GasFlame : Furniture::ShopLight, bLantern ? Furniture::LanternGlow : Furniture::ShopGlow);
		}
	}

	const int32 Made = Batches.Add(Instances);
	BatchIndex.Add(Key, Made);
	if (OutBatch)
	{
		*OutBatch = Made;
	}
	return Pending.AddDefaulted_GetRef();
}

bool AStreetFurniture::GroundAt(const FVector2D& At, FVector& OutGround) const
{
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(FurnitureGround), false, this);
	if (!GetWorld()->LineTraceSingleByChannel(Hit, FVector(At, 10000.0), FVector(At, -10000.0), ECC_Visibility, Params))
	{
		return false;
	}
	// Only the open ground of the map: not on top of a hero building.
	if (!Cast<AOsmGraybox>(Hit.GetActor()))
	{
		return false;
	}
	OutGround = Hit.ImpactPoint;
	return true;
}

void AStreetFurniture::AddObject(const TArray<FFurniturePart>& Parts, const FVector& At, double Yaw, double Scale, float Cull, bool bBreakable)
{
	const FTransform Place(FRotator(0.0, Yaw, 0.0), At, FVector(Scale));
	FBreakable* Thing = nullptr;
	if (bBreakable)
	{
		Thing = &Breakables.AddDefaulted_GetRef();
		Thing->At = At;
		Thing->Yaw = Yaw;
		BreakableCells.Add(Furniture::BreakCell(FVector2D(At)), Breakables.Num() - 1);
	}
	for (const FFurniturePart& Part : Parts)
	{
		const FTransform Local(Part.Rotation, Part.Location, Part.Scale);
		int32 BatchNumber = INDEX_NONE;
		TArray<FTransform>& Instances = Batch(Part.Shape, Part.Color, Part.bCollide, Cull, &BatchNumber);
		Instances.Add(Local * Place);
		if (Thing)
		{
			Thing->Parts.Add({ BatchNumber, Instances.Num() - 1, Part.Shape, Local * Place });
			const FVector Reach = (Part.Location.GetAbs() + Part.Scale * 50.0) * Scale;
			Thing->Half = FVector2D(FMath::Max(Thing->Half.X, Reach.X), FMath::Max(Thing->Half.Y, Reach.Y));
		}
		if (Part.Color.Equals(Furniture::Lantern))
		{
			if (ADayNight* Clock = ADayNight::Get(GetWorld()))
			{
				Clock->AddLamp((Local * Place).GetLocation());
			}
		}
	}
}

void AStreetFurniture::AddStrip(const FVector2D& A, const FVector2D& B, double Width, double Thickness, double Lift, const FLinearColor& Color)
{
	const FVector2D Mid = (A + B) * 0.5;
	FVector Ground;
	if (!GroundAt(Mid, Ground))
	{
		return;
	}
	// Longer by its width, so pieces meeting at an angle close the gap.
	const double Length = FVector2D::Distance(A, B) + Width;
	const double Yaw = FMath::RadiansToDegrees(FMath::Atan2(B.Y - A.Y, B.X - A.X));
	const FTransform Piece(FRotator(0.0, Yaw, 0.0), Ground + FVector(0.0, 0.0, Lift), FVector(Length / 100.0, Width / 100.0, Thickness / 100.0));
	Batch(Furniture::Cube, Color, false, 0.f).Add(Piece);
}

void AStreetFurniture::AddRailing(const FVector2D& A, const FVector2D& B)
{
	const double Length = FVector2D::Distance(A, B);
	if (Length < 1.0)
	{
		return;
	}
	const FVector2D Along = (B - A) / Length;
	const double Yaw = FMath::RadiansToDegrees(FMath::Atan2(Along.Y, Along.X));
	const int32 Posts = FMath::Max(1, FMath::CeilToInt32(Length / Furniture::RailingPostSpacing));
	FVector Ground;
	for (int32 i = 0; i <= Posts; ++i)
	{
		const FVector2D At = A + Along * (Length * i / Posts);
		if (GroundAt(At, Ground))
		{
			Batch(Furniture::Cube, Furniture::Iron, true, SmallCullDistance).Add(
				FTransform(FRotator(0.0, Yaw, 0.0), Ground + FVector(0.0, 0.0, Furniture::RailingHeight / 2.0), FVector(0.05, 0.05, Furniture::RailingHeight / 100.0)));
		}
	}
	// The stone kerb under it and the top and bottom rails, in one piece each between posts.
	for (int32 i = 0; i < Posts; ++i)
	{
		const FVector2D P0 = A + Along * (Length * i / Posts);
		const FVector2D P1 = A + Along * (Length * (i + 1) / Posts);
		const FVector2D Mid = (P0 + P1) * 0.5;
		if (!GroundAt(Mid, Ground))
		{
			continue;
		}
		const double Piece = FVector2D::Distance(P0, P1) / 100.0;
		const FRotator Turn(0.0, Yaw, 0.0);
		Batch(Furniture::Cube, Furniture::Stone, true, SmallCullDistance).Add(FTransform(Turn, Ground + FVector(0.0, 0.0, 6.0), FVector(Piece, 0.25, 0.12)));
		Batch(Furniture::Cube, Furniture::Iron, false, SmallCullDistance).Add(FTransform(Turn, Ground + FVector(0.0, 0.0, Furniture::RailingHeight - 2.0), FVector(Piece, 0.04, 0.04)));
		Batch(Furniture::Cube, Furniture::Iron, false, SmallCullDistance).Add(FTransform(Turn, Ground + FVector(0.0, 0.0, 25.0), FVector(Piece, 0.03, 0.03)));
	}
}

void AStreetFurniture::AddLawnFence(const FVector2D& A, const FVector2D& B)
{
	const double Length = FVector2D::Distance(A, B);
	if (Length < 1.0)
	{
		return;
	}
	const FVector2D Along = (B - A) / Length;
	const FRotator Turn(0.0, FMath::RadiansToDegrees(FMath::Atan2(Along.Y, Along.X)), 0.0);
	const int32 Posts = FMath::Max(1, FMath::CeilToInt32(Length / Furniture::LawnFencePostSpacing));
	FVector Ground;
	for (int32 i = 0; i <= Posts; ++i)
	{
		if (GroundAt(A + Along * (Length * i / Posts), Ground))
		{
			Batch(Furniture::Cube, Furniture::Wood, false, SmallCullDistance).Add(
				FTransform(Turn, Ground + FVector(0.0, 0.0, Furniture::LawnFenceHeight / 2.0), FVector(0.04, 0.04, Furniture::LawnFenceHeight / 100.0)));
		}
	}
	// One wire along the top, in one piece.
	if (GroundAt((A + B) * 0.5, Ground))
	{
		Batch(Furniture::Cube, Furniture::Iron, false, SmallCullDistance).Add(
			FTransform(Turn, Ground + FVector(0.0, 0.0, Furniture::LawnFenceHeight - 4.0), FVector(Length / 100.0, 0.012, 0.012)));
	}
}

void AStreetFurniture::AddHedge(const FVector2D& A, const FVector2D& B)
{
	FVector Ground;
	if (!GroundAt((A + B) * 0.5, Ground))
	{
		return;
	}
	// Longer by its width, so pieces round a curve close up.
	const double Length = FVector2D::Distance(A, B) + Furniture::HedgeWidth;
	const FRotator Turn(0.0, FMath::RadiansToDegrees(FMath::Atan2(B.Y - A.Y, B.X - A.X)), 0.0);
	Batch(Furniture::Cube, Furniture::Hedge, true, SmallCullDistance).Add(
		FTransform(Turn, Ground + FVector(0.0, 0.0, Furniture::HedgeHeight / 2.0), FVector(Length / 100.0, Furniture::HedgeWidth / 100.0, Furniture::HedgeHeight / 100.0)));
}

namespace Furniture
{
	/** Each entry of an array field as numbers. */
	template <typename FEach>
	void ForEachRow(const FJsonObject& Object, const TCHAR* Field, FEach Each)
	{
		const TArray<TSharedPtr<FJsonValue>>* Rows = nullptr;
		if (!Object.TryGetArrayField(Field, Rows))
		{
			return;
		}
		TArray<double> Numbers;
		for (const TSharedPtr<FJsonValue>& Row : *Rows)
		{
			Numbers.Reset();
			for (const TSharedPtr<FJsonValue>& Value : Row->AsArray())
			{
				Numbers.Add(Value->AsNumber());
			}
			Each(Numbers);
		}
	}
}

void AStreetFurniture::BeginPlay()
{
	Super::BeginPlay();

	const FString Path = FPaths::Combine(FPaths::ProjectDir(), DataFile);
	FString Text;
	TSharedPtr<FJsonObject> Root;
	if (!FFileHelper::LoadFileToString(Text, *Path)
		|| !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) || !Root.IsValid())
	{
		UE_LOG(LogFurniture, Error, TEXT("Could not read %s"), *Path);
		return;
	}

	BuildStreets(*Root);
	const TSharedPtr<FJsonObject>* Park = nullptr;
	if (Root->TryGetObjectField(TEXT("tisza"), Park))
	{
		BuildPark(**Park);
	}
	if (Root->TryGetObjectField(TEXT("baross"), Park))
	{
		BuildPark(**Park);
	}
	if (Root->TryGetObjectField(TEXT("rakoczi"), Park))
	{
		BuildPark(**Park);
	}

	// Hand each batch all its instances at once; one by one is slow.
	int32 Count = 0;
	for (int32 i = 0; i < Batches.Num(); ++i)
	{
		Batches[i]->AddInstances(Pending[i], false, true);
		Count += Pending[i].Num();
	}
	Pending.Empty();
	UE_LOG(LogFurniture, Log, TEXT("Street furniture: %d shapes in %d batches"), Count, Batches.Num());

	// A bench someone sits on stays whole.
	for (FBreakable& Thing : Breakables)
	{
		for (const FVector& Seat : SeatedAt)
		{
			Thing.bSturdy = Thing.bSturdy || FVector2D::Distance(FVector2D(Seat), FVector2D(Thing.At)) < Furniture::SeatedNear;
		}
	}
	WreckRandom.Initialize(1932);

	// Leaves blowing about, most of them off the trees that still have some.
	if (AFallingLeaves* Blowing = GetWorld()->SpawnActor<AFallingLeaves>())
	{
		Blowing->SetTrees(LeafyTrees);
	}
}

void AStreetFurniture::BuildStreets(const FJsonObject& Root)
{
	FVector Ground;
	Furniture::ForEachRow(Root, TEXT("lamps"), [&](const TArray<double>& Row)
	{
		if (Row.Num() >= 3 && GroundAt(FVector2D(Row[0], Row[1]), Ground))
		{
			AddObject(Furniture::Lamp, Ground, Row[2], 1.0, CullDistance);
		}
	});
	Furniture::ForEachRow(Root, TEXT("columns"), [&](const TArray<double>& Row)
	{
		if (Row.Num() >= 4 && GroundAt(FVector2D(Row[0], Row[1]), Ground))
		{
			AddObject(Furniture::Column(static_cast<int32>(Row[3])), Ground, Row[2], 1.0, CullDistance);
		}
	});
	Furniture::ForEachRow(Root, TEXT("benches"), [&](const TArray<double>& Row)
	{
		if (Row.Num() >= 3 && GroundAt(FVector2D(Row[0], Row[1]), Ground))
		{
			AddObject(Furniture::Bench, Ground, Row[2], 1.0, SmallCullDistance, true);
		}
	});
	Furniture::ForEachRow(Root, TEXT("bins"), [&](const TArray<double>& Row)
	{
		if (Row.Num() >= 3 && GroundAt(FVector2D(Row[0], Row[1]), Ground))
		{
			AddObject(Furniture::Bin, Ground, Row[2], 1.0, SmallCullDistance, true);
		}
	});
	BuildTaxiStands(Root);
	BuildTramStops(Root);
	BuildFoodVendors(Root);
	BuildMarket(Root);
	BuildShops(Root);
}

void AStreetFurniture::BuildFoodVendors(const FJsonObject& Root)
{
	const TArray<TSharedPtr<FJsonValue>>* Vendors = nullptr;
	if (!Root.TryGetArrayField(TEXT("food_vendors"), Vendors))
	{
		return;
	}
	for (const TSharedPtr<FJsonValue>& Value : *Vendors)
	{
		const TSharedPtr<FJsonObject>* Vendor = nullptr;
		const TArray<TSharedPtr<FJsonValue>>* At = nullptr;
		if (!Value->TryGetObject(Vendor) || !(*Vendor)->TryGetArrayField(TEXT("at"), At) || At->Num() < 3)
		{
			continue;
		}
		FVector Ground;
		if (!GroundAt(FVector2D((*At)[0]->AsNumber(), (*At)[1]->AsNumber()), Ground))
		{
			continue;
		}
		const FString Kind = (*Vendor)->GetStringField(TEXT("kind"));
		const EFoodMenu Menu = Kind == TEXT("sausages") ? EFoodMenu::Sausages
			: Kind == TEXT("pretzels") ? EFoodMenu::Pretzels
			: EFoodMenu::Chestnuts;
		if (AFoodVendor* Seller = GetWorld()->SpawnActor<AFoodVendor>(AFoodVendor::StaticClass(), Ground, FRotator(0.0, (*At)[2]->AsNumber(), 0.0)))
		{
			Seller->Setup(Menu);
		}
	}
}

void AStreetFurniture::BuildMarket(const FJsonObject& Root)
{
	// The Teleki tér market, and the market women on the square in front of the market hall.
	for (const TCHAR* Field : { TEXT("market"), TEXT("hall_market") })
	{
		const TSharedPtr<FJsonObject>* Market = nullptr;
		if (!Root.TryGetObjectField(Field, Market))
		{
			continue;
		}
		if (AFleaMarket* Made = GetWorld()->SpawnActor<AFleaMarket>(AFleaMarket::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator))
		{
			Made->Setup(**Market);
		}
	}
}

void AStreetFurniture::BuildShops(const FJsonObject& Root)
{
	// Where a shop or gate stands: the foot of the wall at the pavement's height, facing out.
	auto Place = [this](const FJsonObject& Item, FVector& OutFoot, double& OutYaw)
	{
		const TArray<TSharedPtr<FJsonValue>>* At = nullptr;
		if (!Item.TryGetArrayField(TEXT("at"), At) || At->Num() < 3)
		{
			return false;
		}
		OutYaw = (*At)[2]->AsNumber();
		const FVector2D Foot((*At)[0]->AsNumber(), (*At)[1]->AsNumber());
		const double Turn = FMath::DegreesToRadians(OutYaw);
		FVector Pavement;
		if (!GroundAt(Foot + FVector2D(FMath::Cos(Turn), FMath::Sin(Turn)) * Furniture::ShopGroundProbe, Pavement))
		{
			return false;
		}
		OutFoot = FVector(Foot, Pavement.Z);
		return true;
	};
	// The shapes go in two batches: the front itself, seen from far down the street, and the small goods.
	auto Put = [this](const TArray<ParkModel::FPart>& Model, const FVector& Foot, double Yaw)
	{
		TArray<FFurniturePart> Large;
		TArray<FFurniturePart> Small;
		for (const FFurniturePart& Part : Furniture::FromParkModel(Model))
		{
			const bool bSmall = Part.Scale.GetMax() * 100.0 < Furniture::ShopSmallPart;
			(bSmall ? Small : Large).Add(Part);
		}
		AddObject(Large, Foot, Yaw, 1.0, CullDistance);
		AddObject(Small, Foot, Yaw, 1.0, SmallCullDistance);
	};
	auto Letter = [this](const ShopModel::FLettering& Lettering, const FString& Words, const FVector& Foot, double Yaw)
	{
		if (Words.IsEmpty())
		{
			return;
		}
		const FRotator Turn(0.0, Yaw, 0.0);
		UTextRenderComponent* Text = NewObject<UTextRenderComponent>(this);
		Text->SetupAttachment(RootComponent);
		Text->SetWorldLocationAndRotation(Foot + Turn.RotateVector(Lettering.At), FRotator(0.0, Yaw + Lettering.Yaw, 0.0));
		Text->SetText(FText::FromString(Words));
		Text->SetHorizontalAlignment(EHTA_Center);
		Text->SetVerticalAlignment(EVRTA_TextCenter);
		// Long names are lettered smaller, so they fit the board.
		Text->SetWorldSize(static_cast<float>(FMath::Min(Lettering.Size, Lettering.Room / (Furniture::LetterWidth * FMath::Max(1, Words.Len())))));
		Text->SetTextRenderColor(Lettering.Color.ToFColor(true));
		Text->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Text->SetCastShadow(false);
		Text->SetCullDistance(Furniture::ShopLetteringDistance);
		Text->RegisterComponent();
	};

	FRandomStream Random(1932);
	const TArray<TSharedPtr<FJsonValue>>* Shops = nullptr;
	if (Root.TryGetArrayField(TEXT("shops"), Shops))
	{
		for (const TSharedPtr<FJsonValue>& Value : *Shops)
		{
			const TSharedPtr<FJsonObject> Shop = Value->AsObject();
			FVector Foot;
			double Yaw = 0.0;
			if (!Shop.IsValid() || !Place(*Shop, Foot, Yaw))
			{
				continue;
			}
			const double Width = Shop->GetNumberField(TEXT("width"));
			const uint32 Seed = static_cast<uint32>(Shop->GetNumberField(TEXT("seed")));
			TArray<ParkModel::FPart> Model;
			TArray<ShopModel::FLettering> Lettering;
			ShopModel::BuildShop(Model, Lettering, Width, ShopModel::TradeOf(*Shop->GetStringField(TEXT("trade"))), Seed,
				Shop->GetBoolField(TEXT("closed")), Shop->GetBoolField(TEXT("outside")));
			Put(Model, Foot, Yaw);
			for (const ShopModel::FLettering& Line : Lettering)
			{
				const TCHAR* Field = Line.Line == ShopModel::ELine::Fascia ? TEXT("fascia") : Line.Line == ShopModel::ELine::Glass ? TEXT("glass") : TEXT("blade");
				Letter(Line, Shop->GetStringField(Field), Foot, Yaw);
			}

			// The shopkeeper in the doorway, looking out at the street.
			if (!Shop->GetBoolField(TEXT("keeper")))
			{
				continue;
			}
			const double Door = ShopModel::DoorMiddle(Width, Seed);
			const FVector Local(ShopModel::KeeperOut, Door + (Door > 0.0 ? 20.0 : -20.0), PersonModel::MiddleHeight);
			PersonKit::FOutfit Clothes = PersonModel::RandomOutfit(Random);
			Clothes.Held = PersonKit::EHeld::None;
			Clothes.bApron = Random.FRand() < 0.6f;
			if (!Clothes.bWoman && Random.FRand() < 0.4f)
			{
				Clothes.Hat = PersonKit::EHat::None;
			}
			if (Shop->GetStringField(TEXT("trade")) == TEXT("hats"))
			{
				// The hatter wears the best of the stock: a smart felt hat with a contrasting band, and no apron.
				ShopModel::DressHatter(Clothes, Random.FRand());
			}
			const FTransform Spawn(FRotator(0.0, Yaw + Random.FRandRange(-30.f, 30.f), 0.0), Foot + FRotator(0.0, Yaw, 0.0).RotateVector(Local));
			AStoryPerson* Keeper = GetWorld()->SpawnActorDeferred<AStoryPerson>(AStoryPerson::StaticClass(), Spawn, this, nullptr,
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
			if (!Keeper)
			{
				continue;
			}
			Keeper->SetOutfit(Clothes);
			Keeper->FinishSpawning(Spawn);
			Keeper->SetActorTickEnabled(false);
			TInlineComponentArray<UPrimitiveComponent*> Shapes(Keeper);
			for (UPrimitiveComponent* Shape : Shapes)
			{
				Shape->SetCachedMaxDrawDistance(Furniture::ShopkeeperDistance);
			}
		}
	}

	const TArray<TSharedPtr<FJsonValue>>* Gates = nullptr;
	if (Root.TryGetArrayField(TEXT("gates"), Gates))
	{
		for (const TSharedPtr<FJsonValue>& Value : *Gates)
		{
			const TSharedPtr<FJsonObject> Gate = Value->AsObject();
			FVector Foot;
			double Yaw = 0.0;
			if (!Gate.IsValid() || !Place(*Gate, Foot, Yaw))
			{
				continue;
			}
			TArray<ParkModel::FPart> Model;
			TArray<ShopModel::FLettering> Lettering;
			ShopModel::BuildGate(Model, Lettering, Gate->GetNumberField(TEXT("width")));
			Put(Model, Foot, Yaw);
			for (const ShopModel::FLettering& Line : Lettering)
			{
				Letter(Line, Gate->GetStringField(TEXT("number")), Foot, Yaw);
			}
		}
	}
}

void AStreetFurniture::BuildTramStops(const FJsonObject& Root)
{
	const TArray<TSharedPtr<FJsonValue>>* Stops = nullptr;
	if (!Root.TryGetArrayField(TEXT("tram_stops"), Stops))
	{
		return;
	}
	for (const TSharedPtr<FJsonValue>& Value : *Stops)
	{
		const TSharedPtr<FJsonObject>* Stop = nullptr;
		const TArray<TSharedPtr<FJsonValue>>* Sign = nullptr;
		if (!Value->TryGetObject(Stop) || !(*Stop)->TryGetArrayField(TEXT("sign"), Sign) || Sign->Num() < 3)
		{
			continue;
		}
		const FVector2D At((*Sign)[0]->AsNumber(), (*Sign)[1]->AsNumber());
		const double Yaw = (*Sign)[2]->AsNumber();
		FVector Ground;
		if (!GroundAt(At, Ground))
		{
			continue;
		}
		AddObject(Furniture::TramStopSign, Ground, Yaw, 1.0, CullDistance);
		// The word and the name on both faces of the plate.
		const FString Name = (*Stop)->GetStringField(TEXT("name"));
		for (const double Face : { 1.0, -1.0 })
		{
			auto Write = [&](const FString& Words, double Z, float Size)
			{
				UTextRenderComponent* Text = NewObject<UTextRenderComponent>(this);
				Text->SetupAttachment(RootComponent);
				Text->SetWorldLocationAndRotation(Ground + FRotator(0.0, Yaw, 0.0).RotateVector(FVector(4.0 * Face, 0.0, Z)),
					FRotator(0.0, Face > 0.0 ? Yaw : Yaw + 180.0, 0.0));
				Text->SetText(FText::FromString(Words));
				Text->SetHorizontalAlignment(EHTA_Center);
				Text->SetVerticalAlignment(EVRTA_TextCenter);
				Text->SetWorldSize(Size);
				Text->SetTextRenderColor(FColor(110, 10, 8));
				Text->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				Text->SetCullDistance(CullDistance);
				Text->RegisterComponent();
			};
			Write(TEXT("MEG\u00c1LL\u00d3"), Furniture::TramStopWordZ, 14.f);
			Write(Name, Furniture::TramStopNameZ, Name.Len() > 16 ? 6.f : 8.f);
		}
	}
}

void AStreetFurniture::BuildTaxiStands(const FJsonObject& Root)
{
	const TArray<TSharedPtr<FJsonValue>>* Stands = nullptr;
	if (!Root.TryGetArrayField(TEXT("taxi_stands"), Stands))
	{
		return;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	for (const TSharedPtr<FJsonValue>& Value : *Stands)
	{
		const TSharedPtr<FJsonObject>* Stand = nullptr;
		if (!Value->TryGetObject(Stand))
		{
			continue;
		}
		FVector Ground;
		// The taxis wait in a rank by the kerb (they settle onto the road themselves).
		Furniture::ForEachRow(**Stand, TEXT("taxis"), [&](const TArray<double>& Row)
		{
			if (Row.Num() >= 3 && GroundAt(FVector2D(Row[0], Row[1]), Ground))
			{
				GetWorld()->SpawnActor<APestiTaxi>(APestiTaxi::StaticClass(), Ground + FVector(0.0, 0.0, Furniture::TaxiLift),
					FRotator(0.0, Row[2], 0.0), Params);
			}
		});
		const TArray<TSharedPtr<FJsonValue>>* Sign = nullptr;
		if (!(*Stand)->TryGetArrayField(TEXT("sign"), Sign) || Sign->Num() < 3)
		{
			continue;
		}
		const FVector2D At((*Sign)[0]->AsNumber(), (*Sign)[1]->AsNumber());
		const double Yaw = (*Sign)[2]->AsNumber();
		if (!GroundAt(At, Ground))
		{
			continue;
		}
		AddObject(Furniture::TaxiSign, Ground, Yaw, 1.0, CullDistance);
		// "TAXI" on both faces of the plate.
		for (const double Face : { 1.0, -1.0 })
		{
			UTextRenderComponent* Text = NewObject<UTextRenderComponent>(this);
			Text->SetupAttachment(RootComponent);
			Text->SetWorldLocationAndRotation(Ground + FRotator(0.0, Yaw, 0.0).RotateVector(FVector(3.5 * Face, 0.0, Furniture::TaxiSignTextZ)),
				FRotator(0.0, Face > 0.0 ? Yaw : Yaw + 180.0, 0.0));
			Text->SetText(FText::FromString(TEXT("TAXI")));
			Text->SetHorizontalAlignment(EHTA_Center);
			Text->SetVerticalAlignment(EVRTA_TextCenter);
			Text->SetWorldSize(18.f);
			Text->SetTextRenderColor(FColor(10, 10, 10));
			Text->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Text->RegisterComponent();
		}
	}
}

void AStreetFurniture::BuildPark(const FJsonObject& Park)
{
	// Gravel paths and the round places, just above the grass.
	Furniture::ForEachRow(Park, TEXT("paths"), [&](const TArray<double>& Row)
	{
		if (Row.Num() >= 5)
		{
			AddStrip(FVector2D(Row[0], Row[1]), FVector2D(Row[2], Row[3]), Row[4], 4.0, 1.0, Furniture::Gravel);
		}
	});
	FVector Ground;
	Furniture::ForEachRow(Park, TEXT("rounds"), [&](const TArray<double>& Row)
	{
		if (Row.Num() < 4 || !GroundAt(FVector2D(Row[0], Row[1]), Ground))
		{
			return;
		}
		const double Round = Row[2] * 2.0 / 100.0;
		const double Bed = Row[3] * 2.0 / 100.0;
		Batch(Furniture::Cylinder, Furniture::Gravel, false, 0.f).Add(FTransform(FRotator::ZeroRotator, Ground + FVector(0.0, 0.0, 1.5), FVector(Round, Round, 0.04)));
		// A raised bed with a stone edge, a ring of flowers and a green middle.
		Batch(Furniture::Cylinder, Furniture::Stone, true, 0.f).Add(FTransform(FRotator::ZeroRotator, Ground + FVector(0.0, 0.0, 12.0), FVector(Bed, Bed, 0.24)));
		Batch(Furniture::Cylinder, Furniture::Flowers, false, 0.f).Add(FTransform(FRotator::ZeroRotator, Ground + FVector(0.0, 0.0, 16.0), FVector(Bed - 0.3, Bed - 0.3, 0.26)));
		Batch(Furniture::Cylinder, Furniture::Lawn, false, 0.f).Add(FTransform(FRotator::ZeroRotator, Ground + FVector(0.0, 0.0, 18.0), FVector(Bed * 0.6, Bed * 0.6, 0.28)));
	});
	Furniture::ForEachRow(Park, TEXT("trees"), [&](const TArray<double>& Row)
	{
		if (Row.Num() >= 3 && GroundAt(FVector2D(Row[0], Row[1]), Ground))
		{
			// Turn each tree a little so the crowns do not all lean one way.
			const double Yaw = FMath::Fmod(Row[0] * 0.37 + Row[1] * 0.11, 360.0);
			const int32 Kind = Furniture::TreeKindAt(FVector2D(Row[0], Row[1]));
			AddObject(Furniture::Tree(Kind), Ground, Yaw, Row[2], TreeCullDistance);
			if (Kind != Furniture::TreeBare)
			{
				LeafyTrees.Add(FVector4(Ground, Row[2]));
			}
		}
	});
	Furniture::ForEachRow(Park, TEXT("benches"), [&](const TArray<double>& Row)
	{
		if (Row.Num() >= 3 && GroundAt(FVector2D(Row[0], Row[1]), Ground))
		{
			AddObject(Furniture::Bench, Ground, Row[2], 1.0, SmallCullDistance, true);
		}
	});
	Furniture::ForEachRow(Park, TEXT("bins"), [&](const TArray<double>& Row)
	{
		if (Row.Num() >= 3 && GroundAt(FVector2D(Row[0], Row[1]), Ground))
		{
			AddObject(Furniture::Bin, Ground, Row[2], 1.0, SmallCullDistance, true);
		}
	});
	Furniture::ForEachRow(Park, TEXT("lamps"), [&](const TArray<double>& Row)
	{
		if (Row.Num() >= 3 && GroundAt(FVector2D(Row[0], Row[1]), Ground))
		{
			AddObject(Furniture::Lamp, Ground, Row[2], 1.0, CullDistance);
		}
	});
	Furniture::ForEachRow(Park, TEXT("twin_lamps"), [&](const TArray<double>& Row)
	{
		if (Row.Num() >= 3 && GroundAt(FVector2D(Row[0], Row[1]), Ground))
		{
			AddObject(Furniture::TwinLamp, Ground, Row[2], 1.0, CullDistance);
		}
	});
	// Trams waiting on a side track: no route, so they stand still.
	Furniture::ForEachRow(Park, TEXT("trams"), [&](const TArray<double>& Row)
	{
		if (Row.Num() < 3 || !GroundAt(FVector2D(Row[0], Row[1]), Ground))
		{
			return;
		}
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (ATram* Waiting = GetWorld()->SpawnActor<ATram>(ATram::StaticClass(), Ground + FVector(0.0, 0.0, Furniture::ParkedTramLift),
			FRotator(0.0, Row[2], 0.0), Params))
		{
			Waiting->SetActorTickEnabled(false);
		}
	});
	Furniture::ForEachRow(Park, TEXT("islands"), [&](const TArray<double>& Row)
	{
		if (Row.Num() < 5)
		{
			return;
		}
		const FVector2D From(Row[0], Row[1]);
		const FVector2D To(Row[2], Row[3]);
		if (!GroundAt((From + To) * 0.5, Ground))
		{
			return;
		}
		const double Yaw = FMath::RadiansToDegrees(FMath::Atan2(To.Y - From.Y, To.X - From.X));
		Batch(Furniture::Cube, Furniture::Stone, true, 0.f).Add(FTransform(FRotator(0.0, Yaw, 0.0),
			Ground + FVector(0.0, 0.0, Furniture::TramIslandHeight * 0.5),
			FVector(FVector2D::Distance(From, To) / 100.0, Row[4] / 100.0, Furniture::TramIslandHeight / 100.0)));
	});
	Furniture::ForEachRow(Park, TEXT("fence"), [&](const TArray<double>& Row)
	{
		if (Row.Num() >= 4)
		{
			AddRailing(FVector2D(Row[0], Row[1]), FVector2D(Row[2], Row[3]));
		}
	});
	Furniture::ForEachRow(Park, TEXT("lawn_fences"), [&](const TArray<double>& Row)
	{
		if (Row.Num() >= 4)
		{
			AddLawnFence(FVector2D(Row[0], Row[1]), FVector2D(Row[2], Row[3]));
		}
	});
	Furniture::ForEachRow(Park, TEXT("hedges"), [&](const TArray<double>& Row)
	{
		if (Row.Num() >= 4)
		{
			AddHedge(FVector2D(Row[0], Row[1]), FVector2D(Row[2], Row[3]));
		}
	});

	const TArray<TSharedPtr<FJsonValue>>* Statue = nullptr;
	if (Park.TryGetArrayField(TEXT("statue"), Statue) && Statue->Num() >= 3)
	{
		const FVector2D At((*Statue)[0]->AsNumber(), (*Statue)[1]->AsNumber());
		if (GroundAt(At, Ground))
		{
			AddObject(Furniture::FromParkModel(ParkModel::Statue()), Ground, (*Statue)[2]->AsNumber(), 1.0, TreeCullDistance);
		}
	}
	const TArray<TSharedPtr<FJsonValue>>* Fountain = nullptr;
	if (Park.TryGetArrayField(TEXT("fountain"), Fountain) && Fountain->Num() >= 3)
	{
		const FVector2D At((*Fountain)[0]->AsNumber(), (*Fountain)[1]->AsNumber());
		if (GroundAt(At, Ground))
		{
			AddObject(Furniture::FromParkModel(ParkModel::FourSeasonsFountain()), Ground, (*Fountain)[2]->AsNumber(), 1.0, TreeCullDistance);
		}
	}
	const TSharedPtr<FJsonObject>* Playground = nullptr;
	if (Park.TryGetObjectField(TEXT("playground"), Playground))
	{
		BuildPlayground(**Playground);
	}

	// Raked leaf piles, and the keepers who go from pile to pile.
	TArray<FVector> Piles;
	Furniture::ForEachRow(Park, TEXT("leaf_piles"), [&](const TArray<double>& Row)
	{
		if (Row.Num() < 3)
		{
			return;
		}
		if (GroundAt(FVector2D(Row[0], Row[1]), Ground))
		{
			AddObject(Furniture::LeafPile, Ground, FMath::Fmod(Row[0] * 0.21 + Row[1] * 0.43, 360.0), Row[2], SmallCullDistance);
			Piles.Add(Ground);
		}
		else
		{
			// Keeps the numbering the keepers use.
			Piles.Add(FVector(Row[0], Row[1], 0.0));
		}
	});
	Furniture::ForEachRow(Park, TEXT("keepers"), [&](const TArray<double>& Row)
	{
		if (Row.Num() < 3 || !GroundAt(FVector2D(Row[0], Row[1]), Ground))
		{
			return;
		}
		TArray<FVector> Spots;
		for (int32 i = 2; i < Row.Num(); ++i)
		{
			const int32 Pile = static_cast<int32>(Row[i]);
			if (Piles.IsValidIndex(Pile) && Piles[Pile].Z != 0.0)
			{
				Spots.Add(Piles[Pile]);
			}
		}
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (AParkKeeper* Keeper = GetWorld()->SpawnActor<AParkKeeper>(AParkKeeper::StaticClass(), Ground + FVector(0.0, 0.0, 90.0), FRotator::ZeroRotator, Params))
		{
			Keeper->SetWorkSpots(Spots);
		}
	});
}

void AStreetFurniture::BuildPlayground(const FJsonObject& Playground)
{
	// The sand, just under the paths that cross it.
	const TArray<TSharedPtr<FJsonValue>>* Sand = nullptr;
	if (Playground.TryGetArrayField(TEXT("sand"), Sand) && Sand->Num() >= 5)
	{
		AddStrip(FVector2D((*Sand)[0]->AsNumber(), (*Sand)[1]->AsNumber()), FVector2D((*Sand)[2]->AsNumber(), (*Sand)[3]->AsNumber()),
			(*Sand)[4]->AsNumber(), 3.0, 0.5, ParkModel::Colors::Sand);
	}

	TArray<TArray<FFurniturePart>> Kinds;
	for (int32 Kind = 0; Kind < ParkModel::NumPlay; ++Kind)
	{
		Kinds.Add(Furniture::FromParkModel(ParkModel::Play(Kind)));
	}
	FVector Ground;
	Furniture::ForEachRow(Playground, TEXT("items"), [&](const TArray<double>& Row)
	{
		const int32 Kind = Row.Num() >= 4 ? static_cast<int32>(Row[0]) : -1;
		if (Kinds.IsValidIndex(Kind) && GroundAt(FVector2D(Row[1], Row[2]), Ground))
		{
			AddObject(Kinds[Kind], Ground, Row[3], 1.0, CullDistance);
		}
	});

	// The children at play and the grown-ups minding them. They stand or sit
	// still, so they need no ticking.
	FRandomStream Random(1932);
	Furniture::ForEachRow(Playground, TEXT("people"), [&](const TArray<double>& Row)
	{
		if (Row.Num() < 5 || !GroundAt(FVector2D(Row[1], Row[2]), Ground))
		{
			return;
		}
		const int32 Kind = static_cast<int32>(Row[0]);
		const bool bChild = Kind == ParkModel::ChildStanding || Kind == ParkModel::ChildSeated;
		const bool bSeated = Kind == ParkModel::ChildSeated || Kind == ParkModel::GrownUpSeated;
		PersonKit::FOutfit Clothes = bChild ? ParkModel::ChildOutfit(Random) : ParkModel::GrownUpOutfit(Random);
		Clothes.bSeated = bSeated;
		const double Middle = bSeated ? ParkModel::SeatedMiddle(Clothes, Row[3], PersonModel::MiddleHeight) : Row[3] + PersonModel::MiddleHeight;
		const FTransform Spawn(FRotator(0.0, Row[4], 0.0), Ground + FVector(0.0, 0.0, Middle));
		AStoryPerson* Person = GetWorld()->SpawnActorDeferred<AStoryPerson>(AStoryPerson::StaticClass(), Spawn, this, nullptr,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!Person)
		{
			return;
		}
		Person->SetOutfit(Clothes);
		Person->FinishSpawning(Spawn);
		Person->SetActorTickEnabled(false);
		if (bSeated)
		{
			SeatedAt.Add(Spawn.GetLocation());
		}
		TInlineComponentArray<UPrimitiveComponent*> Shapes(Person);
		for (UPrimitiveComponent* Shape : Shapes)
		{
			Shape->SetCachedMaxDrawDistance(Furniture::PlaygroundPeopleDistance);
		}
	});
}

void AStreetFurniture::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (DeltaSeconds <= 0.f)
	{
		return;
	}
	const double Now = GetWorld()->GetTimeSeconds();

	// How fast the player's car goes (it moves itself, without physics, so from how far it went).
	APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	const bool bDriving = Player && Player->IsA<APestiCar>() && !Player->IsHidden();
	if (Player)
	{
		const FVector Here = Player->GetActorLocation();
		if (LastPlayer.Get() != Player)
		{
			LastPlayer = Player;
			LastPlayerAt = Here;
		}
		PlayerVelocity = FMath::Lerp(PlayerVelocity, (Here - LastPlayerAt) / DeltaSeconds, 0.5);
		LastPlayerAt = Here;
	}

	// Driven fast into a bench or a bin: it goes to pieces, just before the car touches it.
	const double Speed = PlayerVelocity.Size2D();
	if (bDriving && Speed > Furniture::SmashSpeed)
	{
		const FVector2D Car(Player->GetActorLocation());
		const FVector2D Going = FVector2D(PlayerVelocity).GetSafeNormal();
		const FVector2D Points[] = { Car, Car + Going * (Furniture::CarHalfLength * 0.9) };
		const double Margin = Furniture::CarHalfWidth * 0.6 + Speed * DeltaSeconds * 2.0 + 20.0;
		const FIntPoint Cell = Furniture::BreakCell(Car);
		TArray<int32> Near;
		for (int32 dx = -1; dx <= 1; ++dx)
		{
			for (int32 dy = -1; dy <= 1; ++dy)
			{
				BreakableCells.MultiFind(Cell + FIntPoint(dx, dy), Near);
			}
		}
		for (const int32 Index : Near)
		{
			const FBreakable& Thing = Breakables[Index];
			if (Thing.bBroken || Thing.bSturdy || FMath::Abs(Thing.At.Z - Player->GetActorLocation().Z) > 300.0)
			{
				continue;
			}
			const double A = FMath::DegreesToRadians(Thing.Yaw);
			const FVector2D Forward(FMath::Cos(A), FMath::Sin(A));
			const FVector2D Right(-Forward.Y, Forward.X);
			for (const FVector2D& Point : Points)
			{
				const FVector2D Offset = Point - FVector2D(Thing.At);
				if (FMath::Abs(FVector2D::DotProduct(Offset, Forward)) < Thing.Half.X + Margin
					&& FMath::Abs(FVector2D::DotProduct(Offset, Right)) < Thing.Half.Y + Margin)
				{
					Smash(Index, Now);
					break;
				}
			}
		}
	}

	if (Smashed.Num() == 0)
	{
		return;
	}
	// The pieces come to rest; a few smashed ones a frame are checked to stand again.
	for (const int32 Index : Smashed)
	{
		FBreakable& Thing = Breakables[Index];
		if (!Thing.bSettled && Now - Thing.BrokenAt > Furniture::SettleAfter)
		{
			Thing.bSettled = true;
			for (UStaticMeshComponent* Piece : Thing.Pieces)
			{
				Wreck::Settle(Piece);
			}
		}
	}
	const APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(this, 0);
	if (!CameraManager)
	{
		return;
	}
	const FVector2D Camera(CameraManager->GetCameraLocation());
	for (int32 n = 0; n < Furniture::ResetChecksPerFrame && Smashed.Num() > 0; ++n)
	{
		NextReset = (NextReset + 1) % Smashed.Num();
		FBreakable& Thing = Breakables[Smashed[NextReset]];
		if (Now - Thing.BrokenAt < Furniture::ResetAfter)
		{
			continue;
		}
		bool bSeen = FVector2D::Distance(FVector2D(Thing.At), Camera) < Furniture::ResetDistance;
		for (int32 p = 0; p < Thing.Pieces.Num() && !bSeen; ++p)
		{
			bSeen = IsValid(Thing.Pieces[p]) && Thing.Pieces[p]->WasRecentlyRendered(Furniture::SeenWithin);
		}
		if (!bSeen)
		{
			PutBack(Thing);
			Smashed.RemoveAtSwap(NextReset);
		}
	}
}

void AStreetFurniture::Smash(int32 Index, double Now)
{
	FBreakable& Thing = Breakables[Index];

	// Too much wreckage about already: the oldest that nobody sees is cleared first.
	while (Wreckage.Num() + Thing.Parts.Num() > Furniture::MaxWreckage)
	{
		int32 Oldest = INDEX_NONE;
		for (int32 i = 0; i < Smashed.Num(); ++i)
		{
			const FBreakable& Other = Breakables[Smashed[i]];
			bool bSeen = false;
			for (int32 p = 0; p < Other.Pieces.Num() && !bSeen; ++p)
			{
				bSeen = IsValid(Other.Pieces[p]) && Other.Pieces[p]->WasRecentlyRendered(Furniture::SeenWithin);
			}
			if (!bSeen && (Oldest == INDEX_NONE || Other.BrokenAt < Breakables[Smashed[Oldest]].BrokenAt))
			{
				Oldest = i;
			}
		}
		if (Oldest == INDEX_NONE)
		{
			return;
		}
		PutBack(Breakables[Smashed[Oldest]]);
		Smashed.RemoveAtSwap(Oldest);
	}

	Thing.bBroken = true;
	Thing.bSettled = false;
	Thing.BrokenAt = Now;
	Smashed.Add(Index);
	TSet<int32> Touched;
	for (const FBreakPart& Part : Thing.Parts)
	{
		if (!Batches.IsValidIndex(Part.Batch) || !Batches[Part.Batch] || !ShapeMeshes.IsValidIndex(Part.Shape))
		{
			continue;
		}
		UHierarchicalInstancedStaticMeshComponent* Instances = Batches[Part.Batch];
		FTransform Gone = Part.Transform;
		Gone.SetScale3D(FVector(0.001));
		Instances->UpdateInstanceTransform(Part.Instance, Gone, true, false, true);
		Touched.Add(Part.Batch);

		FVector Velocity;
		FVector Spin;
		Wreck::Fling(PlayerVelocity, WreckRandom, Velocity, Spin);
		UStaticMeshComponent* Piece = Wreck::Throw(this, ShapeMeshes[Part.Shape], Instances->GetMaterial(0), Part.Transform, Velocity, Spin, SmallCullDistance);
		Thing.Pieces.Add(Piece);
		Wreckage.Add(Piece);
	}
	for (const int32 Touch : Touched)
	{
		Batches[Touch]->MarkRenderStateDirty();
	}
	if (APestiCar* Car = Cast<APestiCar>(LastPlayer.Get()))
	{
		Car->ForwardSpeed *= Furniture::CarSlowOnSmash;
	}
	// A lighter left on the bench falls to the ground, still there to be found.
	ALighters::DropNear(GetWorld(), FVector2D(Thing.At), Furniture::LighterNear, Thing.At.Z);
}

void AStreetFurniture::PutBack(FBreakable& Thing)
{
	for (UStaticMeshComponent* Piece : Thing.Pieces)
	{
		if (IsValid(Piece))
		{
			Wreckage.RemoveSwap(Piece);
			Piece->DestroyComponent();
		}
	}
	Thing.Pieces.Reset();
	TSet<int32> Touched;
	for (const FBreakPart& Part : Thing.Parts)
	{
		if (Batches.IsValidIndex(Part.Batch) && Batches[Part.Batch])
		{
			Batches[Part.Batch]->UpdateInstanceTransform(Part.Instance, Part.Transform, true, false, true);
			Touched.Add(Part.Batch);
		}
	}
	for (const int32 Touch : Touched)
	{
		Batches[Touch]->MarkRenderStateDirty();
	}
	Thing.bBroken = false;
	Thing.bSettled = false;
}
