// Fill out your copyright notice in the Description page of Project Settings.

#include "OsmGraybox.h"

#include "Dom/JsonObject.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "Misc/Crc.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "ProceduralMeshComponent.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogOsmGraybox, Log, All);

namespace OsmGraybox
{
	// Main mesh sections, one color each; then one wall section per plaster color.
	enum ESection : int32
	{
		Ground,
		Road,
		Plaza,
		Park,
		Tram,
		Rail,
		TramEdge,
		TramGroove,
		TramRail,
		Roofs,
		FirstWalls
	};

	// Facade tile sections.
	enum EFacadeSection : int32
	{
		Stone,
		Glass,
		Wood,
		Sign,
		FacadeSectionCount
	};

	// Facade tiles: the map is cut into this many columns and rows.
	constexpr int32 FacadeTilesX = 8;
	constexpr int32 FacadeTilesY = 6;

	// Small lifts keep the flat layers from flickering where they overlap.
	constexpr double GroundZ = 2.0;
	constexpr double ParkZ = 4.0;
	constexpr double PlazaZ = 6.0;
	constexpr double RoadZ = 8.0;
	constexpr double RailZ = 10.0;
	constexpr double TramZ = 12.0;
	// The tram tracks' edge stones and rail grooves, then the rail heads on top.
	constexpr double TrackDetailZ = 14.0;
	constexpr double RailHeadZ = 16.0;

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

	void AddFlatPolygon(FSection& Section, const TArray<TArray<FVector2D>>& Rings, const TArray<int32>& Triangles, double Z, bool bFlip,
		const FVector& Up = FVector::UpVector)
	{
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
	void AddWalls(FSection& Section, const TArray<TArray<FVector2D>>& Rings, double Base, double Height, bool bFlip)
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
				const int32 A = Section.AddVertex(FVector(P0.X, P0.Y, Base), Outward);
				const int32 B = Section.AddVertex(FVector(P1.X, P1.Y, Base), Outward);
				const int32 C = Section.AddVertex(FVector(P1.X, P1.Y, Height), Outward);
				const int32 D = Section.AddVertex(FVector(P0.X, P0.Y, Height), Outward);
				Section.AddTriangle(A, B, C, Outward, bFlip);
				Section.AddTriangle(A, C, D, Outward, bFlip);
			}
		}
	}

	// Sizes of a Pest tenement facade (cm).
	namespace Facade
	{
		constexpr double StoreyHeight = 380.0;
		constexpr double GroundFloorHeight = 450.0;
		constexpr double CorniceZone = 60.0;
		constexpr double BayWidth = 320.0;
		constexpr double EdgeMargin = 60.0;
		constexpr double MinWindowedLength = 250.0;
		constexpr double WindowWidth = 130.0;
		constexpr double FrameBorder = 14.0;
		constexpr double FrameDepth = 8.0;
		constexpr double GlassDepth = 1.0;
		constexpr double ShopWidth = 240.0;
		constexpr double GateWidth = 280.0;
		constexpr double GateHeight = 400.0;
	}

	// Draws on one straight wall: S runs along it from P0, Z up from the
	// ground, D out from the wall face.
	struct FWallFrame
	{
		FVector2D P0;
		FVector2D Along;
		FVector Out;
		bool bFlip;

		FVector At(double S, double Z, double D) const
		{
			const FVector2D Flat = P0 + Along * S;
			return FVector(Flat.X, Flat.Y, Z) + Out * D;
		}

		// A rectangle on a plane, facing Normal, with corners in order.
		void Quad(FSection& Section, const FVector& A, const FVector& B, const FVector& C, const FVector& D, const FVector& Normal) const
		{
			const int32 IA = Section.AddVertex(A, Normal);
			const int32 IB = Section.AddVertex(B, Normal);
			const int32 IC = Section.AddVertex(C, Normal);
			const int32 ID = Section.AddVertex(D, Normal);
			Section.AddTriangle(IA, IB, IC, Normal, bFlip);
			Section.AddTriangle(IA, IC, ID, Normal, bFlip);
		}

		// A flat panel parallel to the wall at depth D.
		void Panel(FSection& Section, double S0, double S1, double Z0, double Z1, double D) const
		{
			Quad(Section, At(S0, Z0, D), At(S1, Z0, D), At(S1, Z1, D), At(S0, Z1, D), Out);
		}

		// A block standing out of the wall (no back face).
		void Block(FSection& Section, double S0, double S1, double Z0, double Z1, double D) const
		{
			const FVector Side(Along.X, Along.Y, 0.0);
			Panel(Section, S0, S1, Z0, Z1, D);
			Quad(Section, At(S0, Z1, 0.0), At(S1, Z1, 0.0), At(S1, Z1, D), At(S0, Z1, D), FVector::UpVector);
			Quad(Section, At(S0, Z0, 0.0), At(S1, Z0, 0.0), At(S1, Z0, D), At(S0, Z0, D), FVector::DownVector);
			Quad(Section, At(S0, Z0, 0.0), At(S0, Z1, 0.0), At(S0, Z1, D), At(S0, Z0, D), -Side);
			Quad(Section, At(S1, Z0, 0.0), At(S1, Z1, 0.0), At(S1, Z1, D), At(S1, Z0, D), Side);
		}

		// An opening: a raised frame of width Border around an inner panel
		// that sits back near the wall, with the frame's inner sides showing.
		void Opening(FSection& FrameSection, FSection& InnerSection, double S0, double S1, double Z0, double Z1, double Border, double Depth) const
		{
			const double IS0 = S0 + Border, IS1 = S1 - Border, IZ0 = Z0 + Border, IZ1 = Z1 - Border;
			const FVector Side(Along.X, Along.Y, 0.0);
			// Frame front.
			Panel(FrameSection, S0, S1, IZ1, Z1, Depth);
			Panel(FrameSection, S0, S1, Z0, IZ0, Depth);
			Panel(FrameSection, S0, IS0, IZ0, IZ1, Depth);
			Panel(FrameSection, IS1, S1, IZ0, IZ1, Depth);
			// Frame inner sides, from the panel out to the frame front.
			Quad(FrameSection, At(IS0, IZ1, Facade::GlassDepth), At(IS1, IZ1, Facade::GlassDepth), At(IS1, IZ1, Depth), At(IS0, IZ1, Depth), FVector::DownVector);
			Quad(FrameSection, At(IS0, IZ0, Facade::GlassDepth), At(IS1, IZ0, Facade::GlassDepth), At(IS1, IZ0, Depth), At(IS0, IZ0, Depth), FVector::UpVector);
			Quad(FrameSection, At(IS0, IZ0, Facade::GlassDepth), At(IS0, IZ1, Facade::GlassDepth), At(IS0, IZ1, Depth), At(IS0, IZ0, Depth), Side);
			Quad(FrameSection, At(IS1, IZ0, Facade::GlassDepth), At(IS1, IZ1, Facade::GlassDepth), At(IS1, IZ1, Depth), At(IS1, IZ0, Depth), -Side);
			// The glass, door or shop window itself.
			Panel(InnerSection, IS0, IS1, IZ0, IZ1, Facade::GlassDepth);
		}
	};

	// A Pest tenement front on one wall: shops or windows and a gate on the
	// ground floor, framed windows with sills on the floors above, ledges
	// between floors and a cornice on top. Seed varies the details.
	void AddFacade(TArray<FSection>& Sections, const FVector2D& P0, const FVector2D& P1, double Height, int32 Levels, uint32 Seed, bool bFlip)
	{
		using namespace Facade;

		const double Length = FVector2D::Distance(P0, P1);
		const FVector2D Along = (P1 - P0) / Length;
		// Rings keep the solid on the left of each edge, so out is to the right.
		const FWallFrame Wall{ P0, Along, FVector(Along.Y, -Along.X, 0.0), bFlip };
		FSection& StoneS = Sections[Stone];
		FSection& GlassS = Sections[Glass];
		FSection& WoodS = Sections[Wood];
		FSection& SignS = Sections[Sign];

		const int32 Storeys = Levels > 0 ? Levels : FMath::Clamp(FMath::RoundToInt(Height / StoreyHeight), 1, 8);
		const double GroundTop = FMath::Min(GroundFloorHeight, Height - CorniceZone);
		const double UpperHeight = Storeys > 1 ? (Height - CorniceZone - GroundTop) / (Storeys - 1) : 0.0;

		// Plinth, the ledge over the ground floor, and the cornice.
		Wall.Block(StoneS, 0.0, Length, 0.0, 50.0, 6.0);
		if (GroundTop > 100.0)
		{
			Wall.Block(StoneS, 0.0, Length, GroundTop - 25.0, GroundTop, 18.0);
		}
		Wall.Block(StoneS, 0.0, Length, Height - CorniceZone, Height - 25.0, 22.0);
		Wall.Block(StoneS, 0.0, Length, Height - 25.0, Height, 42.0);

		if (Length < MinWindowedLength || GroundTop < 300.0)
		{
			return;
		}

		const int32 Bays = FMath::Max(1, FMath::FloorToInt((Length - 2.0 * EdgeMargin) / BayWidth));
		const double Bay = (Length - 2.0 * EdgeMargin) / Bays;
		const bool bShops = Seed % 10 < 7;
		const int32 GateBay = Bays >= 3 && (Seed / 10) % 2 == 0 ? Bays / 2 : INDEX_NONE;

		for (int32 k = 0; k < Bays; ++k)
		{
			const double Mid = EdgeMargin + Bay * (k + 0.5);

			// Ground floor: a gate, a shop, or a window.
			if (k == GateBay)
			{
				const double Half = FMath::Min(GateWidth, Bay - 20.0) / 2.0;
				Wall.Opening(StoneS, WoodS, Mid - Half, Mid + Half, 0.0, FMath::Min(GateHeight, GroundTop - 30.0), 30.0, 10.0);
			}
			else if (bShops)
			{
				const double Half = FMath::Min(ShopWidth, Bay - 40.0) / 2.0;
				Wall.Opening(WoodS, GlassS, Mid - Half, Mid + Half, 50.0, GroundTop - 120.0, 18.0, 8.0);
				Wall.Block(SignS, Mid - Half - 10.0, Mid + Half + 10.0, GroundTop - 105.0, GroundTop - 45.0, 8.0);
			}
			else
			{
				Wall.Opening(StoneS, GlassS, Mid - WindowWidth / 2.0, Mid + WindowWidth / 2.0, 130.0, GroundTop - 80.0, FrameBorder, FrameDepth);
			}

			// Floors above: framed windows with a sill; a hood over the
			// first floor's windows, which were the grandest.
			for (int32 Floor = 1; Floor < Storeys; ++Floor)
			{
				const double Base = GroundTop + (Floor - 1) * UpperHeight;
				const double Z0 = Base + UpperHeight * 0.22;
				const double Z1 = Z0 + FMath::Min(230.0, UpperHeight * 0.6);
				const double S0 = Mid - WindowWidth / 2.0;
				const double S1 = Mid + WindowWidth / 2.0;
				Wall.Opening(StoneS, GlassS, S0, S1, Z0, Z1, FrameBorder, FrameDepth);
				Wall.Block(StoneS, S0 - 15.0, S1 + 15.0, Z0 - 12.0, Z0, 16.0);
				if (Floor == 1)
				{
					Wall.Block(StoneS, S0 - 20.0, S1 + 20.0, Z1 + 8.0, Z1 + 30.0, 14.0);
				}
			}
		}

		// Thin ledges between the upper floors.
		for (int32 Floor = 2; Floor < Storeys; ++Floor)
		{
			const double Base = GroundTop + (Floor - 1) * UpperHeight;
			Wall.Block(StoneS, 0.0, Length, Base - 8.0, Base + 4.0, 8.0);
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

	// Building takes a moment, so only do it when there is nothing yet or the
	// map file has changed since the last build (e.g. after pulling an update).
	if (!Mesh)
	{
		return;
	}
	const FDateTime FileTime = IFileManager::Get().GetTimeStamp(*DataPath());
	const bool bFileChanged = FileTime != FDateTime::MinValue() && FileTime != BuiltFromFileTime;
	if (Mesh->GetNumSections() == 0 || bFileChanged)
	{
		Build();
	}
}

void AOsmGraybox::BeginPlay()
{
	Super::BeginPlay();

	// Facades exist only while the game runs: kept in the editor's level they
	// were copied whenever Play was pressed, which ran out of memory.
	if (!HasFacades())
	{
		BuildFacades();
	}
}

bool AOsmGraybox::HasFacades() const
{
	if (!bBuildFacades)
	{
		return true;
	}
	for (const UProceduralMeshComponent* Tile : FacadeTiles)
	{
		if (Tile && Tile->GetNumSections() > 0)
		{
			return true;
		}
	}
	return false;
}

TSharedPtr<FJsonObject> AOsmGraybox::LoadData() const
{
	const FString Path = DataPath();
	FString Text;
	if (!FFileHelper::LoadFileToString(Text, *Path))
	{
		UE_LOG(LogOsmGraybox, Error, TEXT("Could not read %s"), *Path);
		return nullptr;
	}
	TSharedPtr<FJsonObject> Root;
	if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) || !Root.IsValid())
	{
		UE_LOG(LogOsmGraybox, Error, TEXT("%s is not valid JSON"), *Path);
		return nullptr;
	}
	return Root;
}

void AOsmGraybox::BuildFacades()
{
	if (const TSharedPtr<FJsonObject> Root = LoadData())
	{
		BuildFacades(*Root);
	}
}

FString AOsmGraybox::DataPath() const
{
	return FPaths::Combine(FPaths::ProjectDir(), DataFile);
}

void AOsmGraybox::Rebuild()
{
	Build();
}

bool AOsmGraybox::Build()
{
	using namespace OsmGraybox;

	Mesh->ClearAllMeshSections();

	const FString Path = DataPath();
	const TSharedPtr<FJsonObject> Root = LoadData();
	if (!Root)
	{
		return false;
	}

	const int32 NumWallColors = FMath::Max(1, WallColors.Num());
	TArray<FSection> Sections;
	Sections.SetNum(FirstWalls + NumWallColors);

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
		{ TEXT("tram_edge"), TramEdge, TrackDetailZ },
		{ TEXT("tram_groove"), TramGroove, TrackDetailZ },
		{ TEXT("tram_rail"), TramRail, RailHeadZ },
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
			// Bridges between buildings start above the street, so traffic passes under them.
			double Base = 0.0;
			Building->TryGetNumberField(TEXT("base"), Base);
			const uint32 Seed = FCrc::StrCrc32(*Building->GetStringField(TEXT("id")));
			FSection& WallSection = Sections[FirstWalls + Seed % NumWallColors];
			AddWalls(WallSection, Rings, Base, Height, bFlipFaces);
			AddFlatPolygon(Sections[Roofs], Rings, Triangles, Height, bFlipFaces);
			if (Base > 0.0)
			{
				AddFlatPolygon(WallSection, Rings, Triangles, Base, bFlipFaces, FVector::DownVector);
			}
			++NumBuildings;
		}
	}

	TArray<FLinearColor> Colors = { GroundColor, RoadColor, PlazaColor, ParkColor, TramColor, RailColor,
		TrackEdgeColor, RailGrooveColor, RailHeadColor, RoofColor };
	for (int32 i = 0; i < NumWallColors; ++i)
	{
		Colors.Add(WallColors.IsValidIndex(i) ? WallColors[i] : FLinearColor(0.62f, 0.55f, 0.45f));
	}
	for (int32 i = 0; i < Sections.Num(); ++i)
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

	BuiltFromFileTime = IFileManager::Get().GetTimeStamp(*Path);
	UE_LOG(LogOsmGraybox, Log, TEXT("Built %d buildings from %s"), NumBuildings, *Path);

	BuildFacades(*Root);
	return true;
}

void AOsmGraybox::BuildFacades(const FJsonObject& Root)
{
	using namespace OsmGraybox;

	if (!GetWorld() || !GetWorld()->IsGameWorld())
	{
		return;
	}
	if (FacadeTiles.Num() == 0)
	{
		for (int32 i = 0; i < FacadeTilesX * FacadeTilesY; ++i)
		{
			UProceduralMeshComponent* Tile = NewObject<UProceduralMeshComponent>(this, *FString::Printf(TEXT("FacadeTile%02d"), i), RF_Transient | RF_DuplicateTransient);
			Tile->SetupAttachment(Mesh);
			Tile->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Tile->RegisterComponent();
			FacadeTiles.Add(Tile);
		}
	}
	for (UProceduralMeshComponent* Tile : FacadeTiles)
	{
		if (Tile)
		{
			Tile->ClearAllMeshSections();
		}
	}
	const TSharedPtr<FJsonObject>* Bounds = nullptr;
	const TArray<TSharedPtr<FJsonValue>>* Buildings = nullptr;
	if (!bBuildFacades || !Root.TryGetObjectField(TEXT("bounds"), Bounds) || !Root.TryGetArrayField(TEXT("buildings"), Buildings))
	{
		return;
	}
	const double MinX = (*Bounds)->GetNumberField(TEXT("minX"));
	const double MinY = (*Bounds)->GetNumberField(TEXT("minY"));
	const double MaxX = (*Bounds)->GetNumberField(TEXT("maxX"));
	const double MaxY = (*Bounds)->GetNumberField(TEXT("maxY"));

	TArray<TArray<FSection>> TileSections;
	TileSections.SetNum(FacadeTiles.Num());
	for (TArray<FSection>& Tile : TileSections)
	{
		Tile.SetNum(FacadeSectionCount);
	}

	TArray<TArray<FVector2D>> Rings;
	TArray<int32> Triangles;
	for (const TSharedPtr<FJsonValue>& BuildingValue : *Buildings)
	{
		const TSharedPtr<FJsonObject>& Building = BuildingValue->AsObject();
		const TArray<TSharedPtr<FJsonValue>>* Front = nullptr;
		if (!Building.IsValid() || !Building->TryGetArrayField(TEXT("front"), Front) || !ReadPolygon(*Building, Rings, Triangles))
		{
			continue;
		}
		const double Height = Building->GetNumberField(TEXT("height"));
		const uint32 Seed = FCrc::StrCrc32(*Building->GetStringField(TEXT("id")));

		// "front" lists the street-facing wall edges, counted through all rings.
		double Levels = 0.0;
		Building->TryGetNumberField(TEXT("levels"), Levels);
		for (const TSharedPtr<FJsonValue>& EdgeValue : *Front)
		{
			int32 Edge = static_cast<int32>(EdgeValue->AsNumber());
			for (const TArray<FVector2D>& Ring : Rings)
			{
				if (Edge >= Ring.Num())
				{
					Edge -= Ring.Num();
					continue;
				}
				const FVector2D& P0 = Ring[Edge];
				const FVector2D& P1 = Ring[(Edge + 1) % Ring.Num()];
				const FVector2D Mid = (P0 + P1) * 0.5;
				const int32 TileX = FMath::Clamp(FMath::FloorToInt((Mid.X - MinX) / (MaxX - MinX) * FacadeTilesX), 0, FacadeTilesX - 1);
				const int32 TileY = FMath::Clamp(FMath::FloorToInt((Mid.Y - MinY) / (MaxY - MinY) * FacadeTilesY), 0, FacadeTilesY - 1);
				const int32 TileIndex = TileY * FacadeTilesX + TileX;
				if (TileSections.IsValidIndex(TileIndex) && !P0.Equals(P1, 1.0))
				{
					AddFacade(TileSections[TileIndex], P0, P1, Height, static_cast<int32>(Levels), Seed ^ (static_cast<uint32>(Edge) * 2654435761u), bFlipFaces);
				}
				break;
			}
		}
	}

	// Facade tiles share one material per part.
	const FLinearColor FacadeColors[FacadeSectionCount] = { StoneColor, GlassColor, WoodColor, SignColor };
	UMaterialInstanceDynamic* FacadeMaterials[FacadeSectionCount] = {};
	if (BaseMaterial)
	{
		for (int32 j = 0; j < FacadeSectionCount; ++j)
		{
			FacadeMaterials[j] = UMaterialInstanceDynamic::Create(BaseMaterial, this);
			FacadeMaterials[j]->SetVectorParameterValue(TEXT("Color"), FacadeColors[j]);
		}
	}
	int32 FacadeTriangles = 0;
	for (int32 t = 0; t < FacadeTiles.Num(); ++t)
	{
		UProceduralMeshComponent* Tile = FacadeTiles[t];
		if (!Tile)
		{
			continue;
		}
		Tile->LDMaxDrawDistance = FacadeDrawDistance;
		Tile->SetCachedMaxDrawDistance(FacadeDrawDistance);
		for (int32 j = 0; j < FacadeSectionCount; ++j)
		{
			const FSection& Section = TileSections[t][j];
			if (Section.Triangles.Num() == 0)
			{
				continue;
			}
			Tile->CreateMeshSection_LinearColor(j, Section.Vertices, Section.Triangles, Section.Normals,
				TArray<FVector2D>(), TArray<FLinearColor>(), TArray<FProcMeshTangent>(), false);
			Tile->SetMaterial(j, FacadeMaterials[j]);
			FacadeTriangles += Section.Triangles.Num() / 3;
		}
	}

	UE_LOG(LogOsmGraybox, Log, TEXT("Built facades: %d triangles"), FacadeTriangles);
}
