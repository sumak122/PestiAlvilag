// Fill out your copyright notice in the Description page of Project Settings.

#include "OsmGraybox.h"

#include "Dom/JsonObject.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "ProceduralMeshComponent.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogOsmGraybox, Log, All);

namespace OsmGraybox
{
	// Mesh sections, one color each.
	enum ESection : int32
	{
		Ground,
		Road,
		Plaza,
		Park,
		Tram,
		Rail,
		Walls,
		Roofs,
		Count
	};

	// Small lifts keep the flat layers from flickering where they overlap.
	constexpr double GroundZ = 2.0;
	constexpr double ParkZ = 4.0;
	constexpr double PlazaZ = 6.0;
	constexpr double RoadZ = 8.0;
	constexpr double RailZ = 10.0;
	constexpr double TramZ = 12.0;

	struct FSection
	{
		TArray<FVector> Vertices;
		TArray<FVector> Normals;
		TArray<int32> Triangles;

		int32 AddVertex(const FVector& Position, const FVector& Normal)
		{
			Normals.Add(Normal);
			return Vertices.Add(Position);
		}

		// Unreal treats a triangle as facing (C - A) ^ (B - A), so order the
		// corners to face the wanted normal.
		void AddTriangle(int32 A, int32 B, int32 C, const FVector& Normal, bool bFlip)
		{
			const FVector Facing = FVector::CrossProduct(Vertices[C] - Vertices[A], Vertices[B] - Vertices[A]);
			const bool bFacesNormal = FVector::DotProduct(Facing, Normal) >= 0.0;
			Triangles.Add(A);
			if (bFacesNormal != bFlip)
			{
				Triangles.Add(B);
				Triangles.Add(C);
			}
			else
			{
				Triangles.Add(C);
				Triangles.Add(B);
			}
		}
	};

	// Rings come as flat [x0, y0, x1, y1, ...] lists; triangle indices count
	// through all rings in order.
	bool ReadPolygon(const FJsonObject& Polygon, TArray<TArray<FVector2D>>& OutRings, TArray<int32>& OutTriangles)
	{
		const TArray<TSharedPtr<FJsonValue>>* Rings = nullptr;
		const TArray<TSharedPtr<FJsonValue>>* Triangles = nullptr;
		if (!Polygon.TryGetArrayField(TEXT("rings"), Rings) || !Polygon.TryGetArrayField(TEXT("tris"), Triangles))
		{
			return false;
		}

		OutRings.Reset();
		int32 NumPoints = 0;
		for (const TSharedPtr<FJsonValue>& RingValue : *Rings)
		{
			const TArray<TSharedPtr<FJsonValue>>& Coords = RingValue->AsArray();
			TArray<FVector2D>& Ring = OutRings.AddDefaulted_GetRef();
			Ring.Reserve(Coords.Num() / 2);
			for (int32 i = 0; i + 1 < Coords.Num(); i += 2)
			{
				Ring.Emplace(Coords[i]->AsNumber(), Coords[i + 1]->AsNumber());
			}
			NumPoints += Ring.Num();
		}

		OutTriangles.Reset(Triangles->Num());
		for (const TSharedPtr<FJsonValue>& Index : *Triangles)
		{
			const int32 i = static_cast<int32>(Index->AsNumber());
			if (i < 0 || i >= NumPoints)
			{
				return false;
			}
			OutTriangles.Add(i);
		}
		return OutRings.Num() > 0 && OutTriangles.Num() % 3 == 0;
	}

	void AddFlatPolygon(FSection& Section, const TArray<TArray<FVector2D>>& Rings, const TArray<int32>& Triangles, double Z, bool bFlip)
	{
		const FVector Up = FVector::UpVector;
		const int32 First = Section.Vertices.Num();
		for (const TArray<FVector2D>& Ring : Rings)
		{
			for (const FVector2D& Point : Ring)
			{
				Section.AddVertex(FVector(Point.X, Point.Y, Z), Up);
			}
		}
		for (int32 i = 0; i + 2 < Triangles.Num(); i += 3)
		{
			Section.AddTriangle(First + Triangles[i], First + Triangles[i + 1], First + Triangles[i + 2], Up, bFlip);
		}
	}

	// The script orients rings so the solid is on the left of every edge,
	// which puts the outside on the right.
	void AddWalls(FSection& Section, const TArray<TArray<FVector2D>>& Rings, double Height, bool bFlip)
	{
		for (const TArray<FVector2D>& Ring : Rings)
		{
			for (int32 i = 0; i < Ring.Num(); ++i)
			{
				const FVector2D& P0 = Ring[i];
				const FVector2D& P1 = Ring[(i + 1) % Ring.Num()];
				const FVector2D Edge = P1 - P0;
				if (Edge.IsNearlyZero())
				{
					continue;
				}
				const FVector Outward = FVector(Edge.Y, -Edge.X, 0.0).GetSafeNormal();
				const int32 A = Section.AddVertex(FVector(P0.X, P0.Y, 0.0), Outward);
				const int32 B = Section.AddVertex(FVector(P1.X, P1.Y, 0.0), Outward);
				const int32 C = Section.AddVertex(FVector(P1.X, P1.Y, Height), Outward);
				const int32 D = Section.AddVertex(FVector(P0.X, P0.Y, Height), Outward);
				Section.AddTriangle(A, B, C, Outward, bFlip);
				Section.AddTriangle(A, C, D, Outward, bFlip);
			}
		}
	}
}

AOsmGraybox::AOsmGraybox()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Mesh"));
	Mesh->bUseAsyncCooking = true;
	Mesh->bUseComplexAsSimpleCollision = true;
	Mesh->SetCollisionProfileName(TEXT("BlockAll"));
	RootComponent = Mesh;

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicShapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	BaseMaterial = BasicShapeMaterial.Object;
}

void AOsmGraybox::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Building takes a moment, so only do it when there is nothing yet.
	if (Mesh && Mesh->GetNumSections() == 0)
	{
		Build();
	}
}

void AOsmGraybox::Rebuild()
{
	Build();
}

bool AOsmGraybox::Build()
{
	using namespace OsmGraybox;

	Mesh->ClearAllMeshSections();

	const FString Path = FPaths::Combine(FPaths::ProjectContentDir(), DataFile);
	FString Text;
	if (!FFileHelper::LoadFileToString(Text, *Path))
	{
		UE_LOG(LogOsmGraybox, Error, TEXT("Could not read %s"), *Path);
		return false;
	}

	TSharedPtr<FJsonObject> Root;
	if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) || !Root.IsValid())
	{
		UE_LOG(LogOsmGraybox, Error, TEXT("%s is not valid JSON"), *Path);
		return false;
	}

	TArray<FSection> Sections;
	Sections.SetNum(ESection::Count);
	TArray<TArray<FVector2D>> Rings;
	TArray<int32> Triangles;

	// Ground: one rectangle over the whole map area.
	const TSharedPtr<FJsonObject>* Bounds = nullptr;
	if (Root->TryGetObjectField(TEXT("bounds"), Bounds))
	{
		const double MinX = (*Bounds)->GetNumberField(TEXT("minX"));
		const double MinY = (*Bounds)->GetNumberField(TEXT("minY"));
		const double MaxX = (*Bounds)->GetNumberField(TEXT("maxX"));
		const double MaxY = (*Bounds)->GetNumberField(TEXT("maxY"));
		Rings.Reset();
		TArray<FVector2D>& Corners = Rings.AddDefaulted_GetRef();
		Corners.Add(FVector2D(MinX, MinY));
		Corners.Add(FVector2D(MaxX, MinY));
		Corners.Add(FVector2D(MaxX, MaxY));
		Corners.Add(FVector2D(MinX, MaxY));
		Triangles = { 0, 1, 2, 0, 2, 3 };
		AddFlatPolygon(Sections[Ground], Rings, Triangles, GroundZ, bFlipFaces);
	}

	struct FLayer
	{
		const TCHAR* Name;
		ESection Section;
		double Z;
	};
	const FLayer LayerTable[] = {
		{ TEXT("road"), Road, RoadZ },
		{ TEXT("plaza"), Plaza, PlazaZ },
		{ TEXT("park"), Park, ParkZ },
		{ TEXT("tram"), Tram, TramZ },
		{ TEXT("rail"), Rail, RailZ },
	};

	const TArray<TSharedPtr<FJsonValue>>* Areas = nullptr;
	if (Root->TryGetArrayField(TEXT("areas"), Areas))
	{
		for (const TSharedPtr<FJsonValue>& AreaValue : *Areas)
		{
			const TSharedPtr<FJsonObject>& Area = AreaValue->AsObject();
			const TArray<TSharedPtr<FJsonValue>>* Polygons = nullptr;
			if (!Area.IsValid() || !Area->TryGetArrayField(TEXT("polygons"), Polygons))
			{
				continue;
			}
			const FString LayerName = Area->GetStringField(TEXT("layer"));
			const FLayer* MatchedLayer = nullptr;
			for (const FLayer& Candidate : LayerTable)
			{
				if (LayerName == Candidate.Name)
				{
					MatchedLayer = &Candidate;
				}
			}
			if (!MatchedLayer)
			{
				continue;
			}
			for (const TSharedPtr<FJsonValue>& PolygonValue : *Polygons)
			{
				const TSharedPtr<FJsonObject>& Polygon = PolygonValue->AsObject();
				if (Polygon.IsValid() && ReadPolygon(*Polygon, Rings, Triangles))
				{
					AddFlatPolygon(Sections[MatchedLayer->Section], Rings, Triangles, MatchedLayer->Z, bFlipFaces);
				}
			}
		}
	}

	int32 NumBuildings = 0;
	const TArray<TSharedPtr<FJsonValue>>* Buildings = nullptr;
	if (Root->TryGetArrayField(TEXT("buildings"), Buildings))
	{
		for (const TSharedPtr<FJsonValue>& BuildingValue : *Buildings)
		{
			const TSharedPtr<FJsonObject>& Building = BuildingValue->AsObject();
			if (!Building.IsValid() || !ReadPolygon(*Building, Rings, Triangles))
			{
				continue;
			}
			const double Height = Building->GetNumberField(TEXT("height"));
			AddWalls(Sections[Walls], Rings, Height, bFlipFaces);
			AddFlatPolygon(Sections[Roofs], Rings, Triangles, Height, bFlipFaces);
			++NumBuildings;
		}
	}

	const FLinearColor Colors[ESection::Count] = {
		GroundColor, RoadColor, PlazaColor, ParkColor, TramColor, RailColor, WallColor, RoofColor
	};
	for (int32 i = 0; i < ESection::Count; ++i)
	{
		const FSection& Section = Sections[i];
		if (Section.Triangles.Num() == 0)
		{
			continue;
		}
		Mesh->CreateMeshSection_LinearColor(i, Section.Vertices, Section.Triangles, Section.Normals,
			TArray<FVector2D>(), TArray<FLinearColor>(), TArray<FProcMeshTangent>(), bCreateCollision);

		if (BaseMaterial)
		{
			UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(BaseMaterial, this);
			Material->SetVectorParameterValue(TEXT("Color"), Colors[i]);
			Mesh->SetMaterial(i, Material);
		}
	}

	UE_LOG(LogOsmGraybox, Log, TEXT("Built %d buildings from %s"), NumBuildings, *Path);
	return true;
}
