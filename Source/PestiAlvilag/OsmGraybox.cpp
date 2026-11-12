// Fill out your copyright notice in the Description page of Project Settings.

#include "OsmGraybox.h"

#include "BozsiHouseModel.h"

#include "DayNight.h"
#include "FacadeLifeModel.h"
#include "GroundSurfaces.h"
#include "Dom/JsonObject.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "Misc/Crc.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "ProceduralMeshComponent.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/UObjectGlobals.h"

DEFINE_LOG_CATEGORY_STATIC(LogOsmGraybox, Log, All);

namespace OsmGraybox
{
	// Main mesh sections, one color each; then one wall section per plaster color
	// for the ground floors, then one per plaster color for the floors above.
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
		Kerbs,
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
		/** Windows with someone at home: glass by day, lit at night. */
		LitGlass,
		/** Flower boxes and washing on the windows (FacadeLifeModel's parts, in its order). */
		FirstLife,
		FacadeSectionCount = FirstLife + static_cast<int32>(FacadeLifeModel::NumParts)
	};

	// Facade tiles: the map is cut into this many columns and rows.
	constexpr int32 FacadeTilesX = 8;
	constexpr int32 FacadeTilesY = 6;

	// Small lifts keep the flat layers from flickering where they overlap.
	constexpr double GroundZ = 2.0;
	constexpr double ParkZ = 4.0;
	constexpr double PlazaZ = 6.0;
	constexpr double RoadZ = 8.0;
	// Kerbs sit on the pavement just above the road, under the tram tracks.
	constexpr double KerbZ = 9.0;
	constexpr double KerbWidth = 18.0;
	constexpr double RailZ = 10.0;
	constexpr double TramZ = 12.0;
	// The tram tracks' edge stones and rail grooves, then the rail heads on top.
	constexpr double TrackDetailZ = 14.0;
	constexpr double RailHeadZ = 16.0;

	// Bump when the mesh layout changes, so levels saved with an older mesh rebuild it.
	// 2: picture coordinates (UVs) and tangents for the pack materials.
	// 3: walls split into ground floor and upper floor sections.
	// 4: Bözsi néni's block left out (BozsiHouseModel builds it with its courtyard).
	// 5: the city details' blocks (open gateways, the taller Corvin) from city_details.json.
	// 6: flower boxes and washing on the windows; the churches' blocks left out (city_details.json "gone").
	// 7: the New York palace's dressed street walls lose their plain facades (city_details.json).
	constexpr int32 MeshVersion = 7;

	/** The city details' data, next to the map data (Tools/OsmGraybox/city_details.py). */
	const TCHAR* const DetailsFile = TEXT("city_details.json");

	// Walls below this height (cm) are the ground floor, which gets stone blocks;
	// above it smooth plaster. Matches the facades' ground floor.
	constexpr double GroundFloorTop = 450.0;

	struct FSection
	{
		TArray<FVector> Vertices;
		TArray<FVector> Normals;
		TArray<int32> Triangles;
		TArray<FVector2D> UVs;
		TArray<FProcMeshTangent> Tangents;

		int32 AddVertex(const FVector& Position, const FVector& Normal)
		{
			// Pictures are laid by the metre: flat faces take them from above,
			// walls from the front with up being up.
			FVector Across = FVector::XAxisVector;
			FVector2D UV(Position.X, Position.Y);
			if (FMath::Abs(Normal.Z) < 0.7)
			{
				Across = FVector(-Normal.Y, Normal.X, 0.0).GetSafeNormal();
				UV = FVector2D(FVector::DotProduct(Position, Across), -Position.Z);
			}
			UVs.Add(UV / 100.0);
			Tangents.Add(FProcMeshTangent(Across, false));
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

	// Kitbash3D Art Nouveau material instances. The pack lives only in Content on
	// Daniel's PC, never in git, so a missing pack just gives nullptr.
	UMaterialInterface* LoadPackMaterial(const TCHAR* Name)
	{
		const FString Package = FString::Printf(TEXT("/Game/kb3d_artnouveau/Materials/MI_KB3D_ANV_%s"), Name);
		if (!FPackageName::DoesPackageExist(Package))
		{
			return nullptr;
		}
		return LoadObject<UMaterialInterface>(nullptr, *FString::Printf(TEXT("%s.MI_KB3D_ANV_%s"), *Package, Name));
	}

	// One pack material, repeating every TileSize cm, its picture multiplied by Tint if given.
	UMaterialInstanceDynamic* MakePackMaterial(const TCHAR* Name, UObject* InOuter, float TileSize, const FLinearColor* Tint)
	{
		UMaterialInterface* Pack = LoadPackMaterial(Name);
		if (!Pack)
		{
			return nullptr;
		}
		UMaterialInstanceDynamic* PackMaterial = UMaterialInstanceDynamic::Create(Pack, InOuter);
		if (!PackMaterial)
		{
			return nullptr;
		}
		// Our picture coordinates count metres.
		PackMaterial->SetScalarParameterValue(TEXT("Tile_UV"), 100.f / FMath::Max(TileSize, 20.f));
		if (Tint)
		{
			PackMaterial->SetVectorParameterValue(TEXT("basecolor_mult"), *Tint);
		}
		return PackMaterial;
	}

	// The pack's walls are nearly white; our plaster colours, a little brightened, tint them.
	FLinearColor PackTint(const FLinearColor& Color)
	{
		return FLinearColor(FMath::Min(Color.R * 1.5f, 1.f), FMath::Min(Color.G * 1.5f, 1.f), FMath::Min(Color.B * 1.5f, 1.f), 1.f);
	}

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
	// Walls below Split go into Lower, the rest into Upper.
	void AddWalls(FSection& Lower, FSection& Upper, const TArray<TArray<FVector2D>>& Rings, double Base, double Height, double Split, bool bFlip)
	{
		auto AddWallQuad = [bFlip](FSection& Section, const FVector2D& P0, const FVector2D& P1, double Z0, double Z1, const FVector& Outward)
		{
			const int32 A = Section.AddVertex(FVector(P0.X, P0.Y, Z0), Outward);
			const int32 B = Section.AddVertex(FVector(P1.X, P1.Y, Z0), Outward);
			const int32 C = Section.AddVertex(FVector(P1.X, P1.Y, Z1), Outward);
			const int32 D = Section.AddVertex(FVector(P0.X, P0.Y, Z1), Outward);
			Section.AddTriangle(A, B, C, Outward, bFlip);
			Section.AddTriangle(A, C, D, Outward, bFlip);
		};

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
				if (Split <= Base)
				{
					AddWallQuad(Upper, P0, P1, Base, Height, Outward);
				}
				else if (Split >= Height)
				{
					AddWallQuad(Lower, P0, P1, Base, Height, Outward);
				}
				else
				{
					AddWallQuad(Lower, P0, P1, Base, Split, Outward);
					AddWallQuad(Upper, P0, P1, Split, Height, Outward);
				}
			}
		}
	}

	// Light granite kerbstones along the edge of the road, on the pavement
	// side (the outside of the road's rings).
	void AddKerbs(FSection& Section, const TArray<TArray<FVector2D>>& Rings, bool bFlip)
	{
		for (const TArray<FVector2D>& Ring : Rings)
		{
			for (int32 i = 0; i < Ring.Num(); ++i)
			{
				const FVector2D& P0 = Ring[i];
				const FVector2D& P1 = Ring[(i + 1) % Ring.Num()];
				const FVector2D Edge = P1 - P0;
				if (Edge.SizeSquared() < 1.0)
				{
					continue;
				}
				const FVector2D Out = FVector2D(Edge.Y, -Edge.X).GetSafeNormal() * KerbWidth;
				const int32 A = Section.AddVertex(FVector(P0, KerbZ), FVector::UpVector);
				const int32 B = Section.AddVertex(FVector(P1, KerbZ), FVector::UpVector);
				const int32 C = Section.AddVertex(FVector(P1 + Out, KerbZ), FVector::UpVector);
				const int32 D = Section.AddVertex(FVector(P0 + Out, KerbZ), FVector::UpVector);
				Section.AddTriangle(A, B, C, FVector::UpVector, bFlip);
				Section.AddTriangle(A, C, D, FVector::UpVector, bFlip);
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
		constexpr uint32 LitWindowPercent = 26;
		// A lit window's lamplight, as bright as this many lit grey walls.
		const FLinearColor Lamplight(1.f, 0.66f, 0.34f);
		constexpr float LamplightGlow = 3.f;
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

		// A box standing off the wall: S0..S1 along, Z0..Z1 up, D0..D1 out.
		void Box(FSection& Section, double S0, double S1, double Z0, double Z1, double D0, double D1) const
		{
			const FVector Side(Along.X, Along.Y, 0.0);
			Panel(Section, S0, S1, Z0, Z1, D1);
			// The back only shows when the box stands well off the wall.
			if (D0 > 10.0)
			{
				Quad(Section, At(S1, Z0, D0), At(S0, Z0, D0), At(S0, Z1, D0), At(S1, Z1, D0), -Out);
			}
			Quad(Section, At(S0, Z1, D0), At(S1, Z1, D0), At(S1, Z1, D1), At(S0, Z1, D1), FVector::UpVector);
			Quad(Section, At(S0, Z0, D0), At(S1, Z0, D0), At(S1, Z0, D1), At(S0, Z0, D1), FVector::DownVector);
			Quad(Section, At(S0, Z0, D0), At(S0, Z1, D0), At(S0, Z1, D1), At(S0, Z0, D1), -Side);
			Quad(Section, At(S1, Z0, D0), At(S1, Z1, D0), At(S1, Z1, D1), At(S1, Z0, D1), Side);
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
		FSection& LitS = Sections[LitGlass];

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
		TArray<FacadeLifeModel::FBox> LifeBoxes;
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
				Wall.Opening(WoodS, LitS, Mid - Half, Mid + Half, 50.0, GroundTop - 120.0, 18.0, 8.0);
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
				// About one window in four has its lamp lit in the evening.
				const uint32 Home = (Seed ^ (static_cast<uint32>(k) * 2654435761u) ^ (static_cast<uint32>(Floor) * 40503u)) >> 7;
				Wall.Opening(StoneS, Home % 100 < LitWindowPercent ? LitS : GlassS, S0, S1, Z0, Z1, FrameBorder, FrameDepth);
				Wall.Block(StoneS, S0 - 15.0, S1 + 15.0, Z0 - 12.0, Z0, 16.0);
				if (Floor == 1)
				{
					Wall.Block(StoneS, S0 - 20.0, S1 + 20.0, Z1 + 8.0, Z1 + 30.0, 14.0);
				}
				// Flower boxes and washing, picked by where the window is.
				const FVector2D WindowAt = P0 + Along * Mid;
				LifeBoxes.Reset();
				FacadeLifeModel::Window(S0, S1, Z0, Z1, FacadeLifeModel::WindowHash(WindowAt.X, WindowAt.Y, Floor), LifeBoxes);
				for (const FacadeLifeModel::FBox& Life : LifeBoxes)
				{
					Wall.Box(Sections[FirstLife + Life.Part], Life.S0, Life.S1, Life.Z0, Life.Z1, Life.D0, Life.D1);
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
	if (Mesh->GetNumSections() == 0 || bFileChanged || BuiltVersion != OsmGraybox::MeshVersion)
	{
		Build();
	}
}

void AOsmGraybox::BeginPlay()
{
	Super::BeginPlay();

	// Pack materials go on only while playing, so the saved level never needs the pack.
	ApplyPackMaterials();

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

	// The blocks the city details build differently (city_details.py): the
	// houses with an open gateway, cut in pieces with a bridge over the
	// gateway, and the taller Corvin department store. Their pieces take the
	// place of the blocks they replace.
	FString DetailsText;
	TSharedPtr<FJsonObject> Details;
	const FString DetailsPath = FPaths::Combine(FPaths::GetPath(Path), OsmGraybox::DetailsFile);
	const TArray<TSharedPtr<FJsonValue>>* Pieces = nullptr;
	const TArray<TSharedPtr<FJsonValue>>* Blocks = nullptr;
	if (FFileHelper::LoadFileToString(DetailsText, *DetailsPath)
		&& FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(DetailsText), Details) && Details.IsValid()
		&& Details->TryGetArrayField(TEXT("buildings"), Pieces) && Root->TryGetArrayField(TEXT("buildings"), Blocks))
	{
		TSet<FString> Replaced;
		for (const TSharedPtr<FJsonValue>& PieceValue : *Pieces)
		{
			const TSharedPtr<FJsonObject> Piece = PieceValue->AsObject();
			if (Piece.IsValid())
			{
				Replaced.Add(Piece->GetStringField(TEXT("replaces")));
			}
		}
		TArray<TSharedPtr<FJsonValue>> Merged;
		Merged.Reserve(Blocks->Num() + Pieces->Num());
		for (const TSharedPtr<FJsonValue>& BlockValue : *Blocks)
		{
			const TSharedPtr<FJsonObject> Block = BlockValue->AsObject();
			if (!Block.IsValid() || !Replaced.Contains(Block->GetStringField(TEXT("id"))))
			{
				Merged.Add(BlockValue);
			}
		}
		int32 PieceIndex = 0;
		for (const TSharedPtr<FJsonValue>& PieceValue : *Pieces)
		{
			const TSharedPtr<FJsonObject> Piece = PieceValue->AsObject();
			// A "gone" piece only takes its block away (the churches, which ACityDetails builds).
			if (Piece.IsValid() && !Piece->HasField(TEXT("gone")))
			{
				// Each piece its own id, so its colours and facade are its own.
				Piece->SetStringField(TEXT("id"), FString::Printf(TEXT("%s-%d"), *Piece->GetStringField(TEXT("replaces")), PieceIndex++));
				Merged.Add(PieceValue);
			}
		}
		Root->SetArrayField(TEXT("buildings"), Merged);
	}
	return Root;
}

void AOsmGraybox::ApplyPackMaterials()
{
	using namespace OsmGraybox;

	if (!bPackMaterials || !Mesh)
	{
		return;
	}
	if (UMaterialInstanceDynamic* RoofMaterial = MakePackMaterial(TEXT("RoofTiles"), this, PackTileSize, nullptr))
	{
		Mesh->SetMaterial(Roofs, RoofMaterial);
	}
	// Each plaster colour gets the pack's stone blocks on the ground floor and
	// smooth plaster above, both tinted to that colour.
	static const TCHAR* const BlockNames[] = { TEXT("DifferentWall"), TEXT("DifferentWall2"), TEXT("EustacheWall") };
	static const TCHAR* const PlasterNames[] = { TEXT("ConcreteOld"), TEXT("WhiteConcrete") };
	const int32 NumBlockNames = static_cast<int32>(UE_ARRAY_COUNT(BlockNames));
	const int32 NumPlasterNames = static_cast<int32>(UE_ARRAY_COUNT(PlasterNames));
	const int32 NumWallColors = FMath::Max(1, WallColors.Num());
	for (int32 i = 0; i < WallColors.Num(); ++i)
	{
		const FLinearColor Tint = PackTint(WallColors[i]);
		if (UMaterialInstanceDynamic* BlockMaterial = MakePackMaterial(BlockNames[i % NumBlockNames], this, PackTileSize, &Tint))
		{
			Mesh->SetMaterial(FirstWalls + i, BlockMaterial);
		}
		if (UMaterialInstanceDynamic* PlasterMaterial = MakePackMaterial(PlasterNames[i % NumPlasterNames], this, PackTileSize, &Tint))
		{
			Mesh->SetMaterial(FirstWalls + NumWallColors + i, PlasterMaterial);
		}
	}
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
	Sections.SetNum(FirstWalls + 2 * NumWallColors);

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
					if (MatchedLayer->Section == Road)
					{
						AddKerbs(Sections[Kerbs], Rings, bFlipFaces);
					}
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
			if (!Building.IsValid() || !ReadPolygon(*Building, Rings, Triangles)
				|| Building->GetStringField(TEXT("id")) == BozsiHouseModel::ReplacesBuilding)
			{
				continue;
			}
			const double Height = Building->GetNumberField(TEXT("height"));
			// Bridges between buildings start above the street, so traffic passes under them.
			double Base = 0.0;
			Building->TryGetNumberField(TEXT("base"), Base);
			const uint32 Seed = FCrc::StrCrc32(*Building->GetStringField(TEXT("id")));
			FSection& WallSection = Sections[FirstWalls + Seed % NumWallColors];
			FSection& UpperWallSection = Sections[FirstWalls + NumWallColors + Seed % NumWallColors];
			AddWalls(WallSection, UpperWallSection, Rings, Base, Height, GroundFloorTop, bFlipFaces);
			AddFlatPolygon(Sections[Roofs], Rings, Triangles, Height, bFlipFaces);
			if (Base > 0.0)
			{
				AddFlatPolygon(Base < GroundFloorTop ? WallSection : UpperWallSection, Rings, Triangles, Base, bFlipFaces, FVector::DownVector);
			}
			++NumBuildings;
		}
	}

	TArray<FLinearColor> Colors = { GroundColor, RoadColor, PlazaColor, ParkColor, TramColor, RailColor,
		TrackEdgeColor, RailGrooveColor, RailHeadColor, KerbColor, RoofColor };
	// Ground floor walls, then the same colours again for the floors above.
	for (int32 i = 0; i < 2 * NumWallColors; ++i)
	{
		const int32 k = i % NumWallColors;
		Colors.Add(WallColors.IsValidIndex(k) ? WallColors[k] : FLinearColor(0.62f, 0.55f, 0.45f));
	}
	for (int32 i = 0; i < Sections.Num(); ++i)
	{
		const FSection& Section = Sections[i];
		if (Section.Triangles.Num() == 0)
		{
			continue;
		}
		// Kerbs are only a picture; walking and driving over them stays smooth.
		Mesh->CreateMeshSection_LinearColor(i, Section.Vertices, Section.Triangles, Section.Normals,
			Section.UVs, TArray<FLinearColor>(), Section.Tangents, bCreateCollision && i != Kerbs);

		// Stone, slabs and grass where there's a picture for it, plain colour otherwise.
		UMaterialInstanceDynamic* Material = nullptr;
		if (bGroundPictures)
		{
			switch (i)
			{
			case Ground: Material = GroundSurfaces::Make(SurfaceModel::EKind::Slabs, this); break;
			case Road: Material = GroundSurfaces::Make(SurfaceModel::EKind::Setts, this); break;
			case Plaza:
			case Tram: Material = GroundSurfaces::Make(SurfaceModel::EKind::BigSetts, this); break;
			case Park: Material = GroundSurfaces::Make(SurfaceModel::EKind::Grass, this); break;
			default: break;
			}
		}
		if (!Material && BaseMaterial)
		{
			Material = UMaterialInstanceDynamic::Create(BaseMaterial, this);
			Material->SetVectorParameterValue(TEXT("Color"), Colors[i]);
		}
		if (Material)
		{
			Mesh->SetMaterial(i, Material);
		}
	}

	BuiltFromFileTime = IFileManager::Get().GetTimeStamp(*Path);
	BuiltVersion = MeshVersion;
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
		if (!Building.IsValid() || !Building->TryGetArrayField(TEXT("front"), Front) || !ReadPolygon(*Building, Rings, Triangles)
			|| Building->GetStringField(TEXT("id")) == BozsiHouseModel::ReplacesBuilding)
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
	FLinearColor FacadeColors[FacadeSectionCount] = { StoneColor, GlassColor, WoodColor, SignColor, GlassColor };
	for (int32 j = FirstLife; j < FacadeSectionCount; ++j)
	{
		FacadeColors[j] = FacadeLifeModel::PartColor(j - FirstLife);
	}
	ADayNight* Clock = ADayNight::Get(GetWorld());
	UMaterialInstanceDynamic* FacadeMaterials[FacadeSectionCount] = {};
	if (BaseMaterial)
	{
		for (int32 j = 0; j < FacadeSectionCount; ++j)
		{
			FacadeMaterials[j] = UMaterialInstanceDynamic::Create(BaseMaterial, this);
			FacadeMaterials[j]->SetVectorParameterValue(TEXT("Color"), FacadeColors[j]);
		}
	}
	// With the Art Nouveau pack: stone trim and wooden gates and shop fronts.
	// Windows keep their dark glass, which the night lamps also light.
	if (bPackMaterials)
	{
		const FLinearColor StoneTint = PackTint(StoneColor);
		if (UMaterialInstanceDynamic* StoneMaterial = MakePackMaterial(TEXT("WhiteConcrete"), this, PackTileSize, &StoneTint))
		{
			FacadeMaterials[Stone] = StoneMaterial;
		}
		if (UMaterialInstanceDynamic* WoodMaterial = MakePackMaterial(TEXT("Wood"), this, PackTileSize, nullptr))
		{
			FacadeMaterials[Wood] = WoodMaterial;
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
				Section.UVs, TArray<FLinearColor>(), Section.Tangents, false);
			Tile->SetMaterial(j, FacadeMaterials[j]);
			if (j == LitGlass && Clock)
			{
				Clock->AddNightGlow(Tile, j, Facade::Lamplight, Facade::LamplightGlow);
			}
			FacadeTriangles += Section.Triangles.Num() / 3;
		}
	}

	UE_LOG(LogOsmGraybox, Log, TEXT("Built facades: %d triangles"), FacadeTriangles);
}
