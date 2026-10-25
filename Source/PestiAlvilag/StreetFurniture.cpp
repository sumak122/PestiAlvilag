// Fill out your copyright notice in the Description page of Project Settings.

#include "StreetFurniture.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "OsmGraybox.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
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
};

namespace Furniture
{
	enum EShape : int32 { Cube, Cylinder, Sphere, Cone, NumShapes };

	const FLinearColor Iron(0.035f, 0.05f, 0.04f);
	const FLinearColor Lantern(0.95f, 0.85f, 0.55f);
	const FLinearColor Wood(0.30f, 0.17f, 0.08f);
	const FLinearColor Stone(0.42f, 0.40f, 0.36f);
	const FLinearColor Gravel(0.55f, 0.50f, 0.40f);
	const FLinearColor Bark(0.12f, 0.09f, 0.07f);
	const FLinearColor Leaves(0.10f, 0.20f, 0.05f);
	const FLinearColor LeavesLight(0.15f, 0.26f, 0.07f);
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

	const TArray<FFurniturePart> Bench = {
		{ Cube, Wood, FVector(0, 0, 45), FVector(0.45, 1.8, 0.06), true },
		{ Cube, Wood, FVector(-22, 0, 76), FVector(0.05, 1.8, 0.36), true },
		{ Cube, Iron, FVector(0, -80, 23), FVector(0.45, 0.06, 0.46), false },
		{ Cube, Iron, FVector(0, 80, 23), FVector(0.45, 0.06, 0.46), false },
	};

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

	// A broad park tree: a trunk and two overlapping crowns.
	const TArray<FFurniturePart> Tree = {
		{ Cylinder, Bark, FVector(0, 0, 170), FVector(0.4, 0.4, 3.4), true },
		{ Sphere, Leaves, FVector(0, 0, 560), FVector(6.0, 6.0, 5.0), false },
		{ Sphere, LeavesLight, FVector(90, 60, 650), FVector(4.4, 4.4, 3.8), false },
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
}

AStreetFurniture::AStreetFurniture()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	BaseMaterial = Material.Object;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMesh(TEXT("/Engine/BasicShapes/Cone.Cone"));
	ShapeMeshes = { CubeMesh.Object, CylinderMesh.Object, SphereMesh.Object, ConeMesh.Object };
}

TArray<FTransform>& AStreetFurniture::Batch(int32 Shape, const FLinearColor& Color, bool bCollide, float Cull)
{
	const FString Key = FString::Printf(TEXT("%d %s %d %.0f"), Shape, *Color.ToString(), bCollide ? 1 : 0, Cull);
	if (const int32* Found = BatchIndex.Find(Key))
	{
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

	BatchIndex.Add(Key, Batches.Add(Instances));
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

void AStreetFurniture::AddObject(const TArray<FFurniturePart>& Parts, const FVector& At, double Yaw, double Scale, float Cull)
{
	const FTransform Place(FRotator(0.0, Yaw, 0.0), At, FVector(Scale));
	for (const FFurniturePart& Part : Parts)
	{
		const FTransform Local(FRotator::ZeroRotator, Part.Location, Part.Scale);
		Batch(Part.Shape, Part.Color, Part.bCollide, Cull).Add(Local * Place);
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
		BuildTiszaKalmanTer(**Park);
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
			AddObject(Furniture::Bench, Ground, Row[2], 1.0, SmallCullDistance);
		}
	});
}

void AStreetFurniture::BuildTiszaKalmanTer(const FJsonObject& Park)
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
			AddObject(Furniture::Tree, Ground, Yaw, Row[2], TreeCullDistance);
		}
	});
	Furniture::ForEachRow(Park, TEXT("benches"), [&](const TArray<double>& Row)
	{
		if (Row.Num() >= 3 && GroundAt(FVector2D(Row[0], Row[1]), Ground))
		{
			AddObject(Furniture::Bench, Ground, Row[2], 1.0, SmallCullDistance);
		}
	});
	Furniture::ForEachRow(Park, TEXT("lamps"), [&](const TArray<double>& Row)
	{
		if (Row.Num() >= 3 && GroundAt(FVector2D(Row[0], Row[1]), Ground))
		{
			AddObject(Furniture::Lamp, Ground, Row[2], 1.0, CullDistance);
		}
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
}
