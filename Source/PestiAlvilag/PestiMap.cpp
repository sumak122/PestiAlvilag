// Fill out your copyright notice in the Description page of Project Settings.

#include "PestiMap.h"

#include "Dom/JsonObject.h"
#include "Engine/Texture2D.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "TextureResource.h"

namespace PestiMapBake
{
	// The same file the graybox city is built from, and the files that dress it.
	const TCHAR* const DataFile = TEXT("Data/OsmGraybox/blaha_test_area.json");
	const TCHAR* const DetailsFile = TEXT("Data/OsmGraybox/city_details.json");
	const TCHAR* const FurnitureFile = TEXT("Data/OsmGraybox/street_furniture.json");

	// The picture is this many pixels square (about 44 cm a pixel).
	constexpr int32 Size = 4096;

	// The printed plan's inks.
	const FColor Ground(43, 37, 28);
	const FColor Road(203, 186, 146);
	const FColor Plaza(214, 200, 165);
	const FColor Park(84, 96, 56);
	const FColor Building(92, 76, 55);
	const FColor BuildingEdge(66, 53, 37);
	const FColor Landmark(140, 104, 62);
	const FColor LandmarkEdge(212, 176, 104);
	const FColor Tram(150, 62, 48);
	const FColor Rail(78, 70, 58);
	const FColor PathInk(176, 160, 118);
	const FColor Tree(52, 66, 34);
	const FColor Hedge(44, 58, 30);
	const FColor Bed(124, 58, 48);
	const FColor Kerb(170, 152, 112);
	const FColor Grave(132, 126, 112);
	const FColor CemeteryWall(150, 138, 112);

	// Sizes drawn on the plan (cm): a hedge's width, a tree's crown for size 1, a grave, the cemetery wall.
	constexpr double HedgeWidth = 90.0;
	constexpr double CrownRadius = 330.0;
	constexpr double GraveRadius = 45.0;
	constexpr double WallWidth = 60.0;
	constexpr double TramWidth = 90.0;

	// The Nepszinhaz's walls on the plan, as ANepszinhaz builds them at PestiStart::TheatreAt facing west (cm).
	constexpr double TheatreX = -3600.0;
	constexpr double TheatreY = 200.0;
	constexpr double TheatreFront = 2900.0;
	constexpr double TheatreBack = 2200.0;
	constexpr double TheatreHalfWidth = 2200.0;
	constexpr double TheatreApse = 1300.0;

	// A street further than this from the player is not named (cm).
	constexpr double NameDistance = 2500.0;
	const TCHAR* const District = TEXT("J\u00f3zsefv\u00e1ros");

	// Today's names that were different in 1932, and names of things that
	// did not exist yet (these show the district's name instead).
	const TCHAR* const Renamed[][2] = {
		{ TEXT("II. J\u00e1nos P\u00e1l p\u00e1pa t\u00e9r"), TEXT("Tisza K\u00e1lm\u00e1n t\u00e9r") },
		{ TEXT("Teleki L\u00e1szl\u00f3 t\u00e9r"), TEXT("Teleki t\u00e9r") },
		{ TEXT("Baross t\u00e9ri fel\u00fcl\u00e1r\u00f3"), TEXT("Baross t\u00e9r") },
	};
	const TCHAR* const NotYet[] = {
		TEXT("48-asok \u00fatja"),
		TEXT("56-osok \u00fatja"),
		TEXT("Bacs\u00f3 B\u00e9la utca"),
		TEXT("Mindszenty J\u00f3zsef t\u00e9r"),
		TEXT("Szeles Erika t\u00e9r"),
		TEXT("Szerb Antal \u00fat"),
		TEXT("Scheiber S\u00e1ndor utca"),
		TEXT("Kar\u00e1csony S\u00e1ndor utca"),
	};

	FString PeriodName(const FString& Today)
	{
		for (const auto& Pair : Renamed)
		{
			if (Today == Pair[0])
			{
				return Pair[1];
			}
		}
		for (const TCHAR* Name : NotYet)
		{
			if (Today == Name)
			{
				return FString();
			}
		}
		return Today;
	}

	/** The picture being painted, one colour a pixel. */
	struct FCanvas
	{
		TArray<FColor> Pixels;
		FVector2D Corner = FVector2D::ZeroVector;
		double PixelsPerCm = 1.0;

		FVector2D ToPixel(double X, double Y) const
		{
			return FVector2D((X - Corner.X) * PixelsPerCm, (Y - Corner.Y) * PixelsPerCm);
		}

		void Span(int32 Row, int32 From, int32 To, const FColor& Color)
		{
			if (Row < 0 || Row >= Size)
			{
				return;
			}
			From = FMath::Max(From, 0);
			To = FMath::Min(To, Size - 1);
			for (int32 X = From; X <= To; ++X)
			{
				Pixels[Row * Size + X] = Color;
			}
		}

		/** A filled triangle, one row of pixels at a time. */
		void Triangle(const FVector2D& A, const FVector2D& B, const FVector2D& C, const FColor& Color)
		{
			const double Top = FMath::Min3(A.Y, B.Y, C.Y);
			const double Bottom = FMath::Max3(A.Y, B.Y, C.Y);
			const int32 FirstRow = FMath::Max(FMath::FloorToInt(Top), 0);
			const int32 LastRow = FMath::Min(FMath::FloorToInt(Bottom), Size - 1);
			const FVector2D Corners[3] = { A, B, C };
			for (int32 Row = FirstRow; Row <= LastRow; ++Row)
			{
				// Where the edges cross the middle of this row of pixels.
				const double Y = Row + 0.5;
				double Left = TNumericLimits<double>::Max();
				double Right = -TNumericLimits<double>::Max();
				for (int32 i = 0; i < 3; ++i)
				{
					const FVector2D& P = Corners[i];
					const FVector2D& Q = Corners[(i + 1) % 3];
					const double Low = FMath::Min(P.Y, Q.Y);
					const double High = FMath::Max(P.Y, Q.Y);
					if (High - Low < UE_DOUBLE_SMALL_NUMBER || Y < Low || Y > High)
					{
						continue;
					}
					const double X = P.X + (Y - P.Y) * (Q.X - P.X) / (Q.Y - P.Y);
					Left = FMath::Min(Left, X);
					Right = FMath::Max(Right, X);
				}
				// A row the triangle only grazes (it starts or ends inside the row): the part of each edge inside the row.
				if (Left > Right)
				{
					const double RowTop = Row;
					const double RowBottom = Row + 1.0;
					for (int32 i = 0; i < 3; ++i)
					{
						const FVector2D& P = Corners[i];
						const FVector2D& Q = Corners[(i + 1) % 3];
						const double Low = FMath::Min(P.Y, Q.Y);
						const double High = FMath::Max(P.Y, Q.Y);
						if (High < RowTop || Low > RowBottom)
						{
							continue;
						}
						for (const double Edge : { FMath::Max(Low, RowTop), FMath::Min(High, RowBottom) })
						{
							const double X = High - Low < UE_DOUBLE_SMALL_NUMBER ? P.X : P.X + (Edge - P.Y) * (Q.X - P.X) / (Q.Y - P.Y);
							Left = FMath::Min(Left, X);
							Right = FMath::Max(Right, X);
						}
						if (High - Low < UE_DOUBLE_SMALL_NUMBER)
						{
							Left = FMath::Min(Left, FMath::Min(P.X, Q.X));
							Right = FMath::Max(Right, FMath::Max(P.X, Q.X));
						}
					}
				}
				if (Left <= Right)
				{
					Span(Row, FMath::FloorToInt(Left), FMath::FloorToInt(Right), Color);
				}
			}
		}

		/** A line one pixel wide. */
		void Line(const FVector2D& A, const FVector2D& B, const FColor& Color)
		{
			const int32 Steps = FMath::Max(1, FMath::CeilToInt(FMath::Max(FMath::Abs(B.X - A.X), FMath::Abs(B.Y - A.Y))));
			for (int32 i = 0; i <= Steps; ++i)
			{
				const FVector2D P = A + (B - A) * (static_cast<double>(i) / Steps);
				const int32 X = FMath::FloorToInt(P.X);
				const int32 Y = FMath::FloorToInt(P.Y);
				if (X >= 0 && X < Size && Y >= 0 && Y < Size)
				{
					Pixels[Y * Size + X] = Color;
				}
			}
		}

		/** A line Width cm wide between two world spots, at least a pixel. */
		void Band(double X0, double Y0, double X1, double Y1, double Width, const FColor& Color)
		{
			const FVector2D A = ToPixel(X0, Y0);
			const FVector2D B = ToPixel(X1, Y1);
			const double HalfWide = Width * PixelsPerCm * 0.5;
			if (HalfWide < 0.75)
			{
				Line(A, B, Color);
				return;
			}
			const FVector2D Along = (B - A).GetSafeNormal();
			const FVector2D Across(-Along.Y * HalfWide, Along.X * HalfWide);
			Triangle(A + Across, B + Across, B - Across, Color);
			Triangle(A + Across, B - Across, A - Across, Color);
		}

		/** A round spot of Radius cm at a world spot, at least a pixel. */
		void Disc(double X, double Y, double Radius, const FColor& Color)
		{
			const FVector2D Middle = ToPixel(X, Y);
			const double R = FMath::Max(Radius * PixelsPerCm, 0.6);
			const int32 FirstRow = FMath::FloorToInt(Middle.Y - R);
			const int32 LastRow = FMath::FloorToInt(Middle.Y + R);
			for (int32 Row = FirstRow; Row <= LastRow; ++Row)
			{
				const double Dy = FMath::Clamp(Row + 0.5 - Middle.Y, -R, R);
				const double Dx = FMath::Sqrt(FMath::Max(R * R - Dy * Dy, 0.0));
				Span(Row, FMath::FloorToInt(Middle.X - Dx), FMath::FloorToInt(Middle.X + Dx), Color);
			}
		}

		/** A polygon as the map data has it: rings of [x, y, ...] and triangles counted through all rings; with Edge, its rings drawn round it. */
		void Polygon(const FJsonObject& Shape, const FColor& Color, const FColor* Edge = nullptr)
		{
			const TArray<TSharedPtr<FJsonValue>>* Rings = nullptr;
			const TArray<TSharedPtr<FJsonValue>>* Corners = nullptr;
			if (!Shape.TryGetArrayField(TEXT("rings"), Rings) || !Shape.TryGetArrayField(TEXT("tris"), Corners))
			{
				return;
			}
			Fill(*Rings, *Corners, Color, Edge);
		}

		/** Rings of [x, y, ...] filled by triangles counted through all rings. */
		void Fill(const TArray<TSharedPtr<FJsonValue>>& Rings, const TArray<TSharedPtr<FJsonValue>>& Corners, const FColor& Color, const FColor* Edge)
		{
			TArray<FVector2D> Points;
			TArray<int32> RingStarts;
			for (const TSharedPtr<FJsonValue>& RingValue : Rings)
			{
				RingStarts.Add(Points.Num());
				const TArray<TSharedPtr<FJsonValue>>& Coords = RingValue->AsArray();
				for (int32 i = 0; i + 1 < Coords.Num(); i += 2)
				{
					Points.Add(ToPixel(Coords[i]->AsNumber(), Coords[i + 1]->AsNumber()));
				}
			}
			for (int32 i = 0; i + 2 < Corners.Num(); i += 3)
			{
				const int32 A = static_cast<int32>(Corners[i]->AsNumber());
				const int32 B = static_cast<int32>(Corners[i + 1]->AsNumber());
				const int32 C = static_cast<int32>(Corners[i + 2]->AsNumber());
				if (Points.IsValidIndex(A) && Points.IsValidIndex(B) && Points.IsValidIndex(C))
				{
					Triangle(Points[A], Points[B], Points[C], Color);
				}
			}
			if (!Edge)
			{
				return;
			}
			RingStarts.Add(Points.Num());
			for (int32 r = 0; r + 1 < RingStarts.Num(); ++r)
			{
				const int32 First = RingStarts[r];
				const int32 End = RingStarts[r + 1];
				for (int32 i = First; i < End; ++i)
				{
					Line(Points[i], Points[i + 1 < End ? i + 1 : First], *Edge);
				}
			}
		}

		/** Every polygon of a list of objects that each hold one (buildings, landmarks). */
		void Polygons(const FJsonObject& Root, const TCHAR* Field, const FColor& Color, const FColor* Edge = nullptr)
		{
			const TArray<TSharedPtr<FJsonValue>>* List = nullptr;
			if (Root.TryGetArrayField(Field, List))
			{
				for (const TSharedPtr<FJsonValue>& Value : *List)
				{
					if (const TSharedPtr<FJsonObject> Shape = Value->AsObject())
					{
						Polygon(*Shape, Color, Edge);
					}
				}
			}
		}
	};

	/** The JSON file at a path under the project, or null. */
	TSharedPtr<FJsonObject> LoadJson(const TCHAR* File)
	{
		FString Text;
		TSharedPtr<FJsonObject> Root;
		if (FFileHelper::LoadFileToString(Text, *FPaths::Combine(FPaths::ProjectDir(), File)))
		{
			FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root);
		}
		return Root;
	}

	/** Each row of numbers of a list field of an object (rows shorter than MinCount left out). */
	void ForEachRow(const FJsonObject& Object, const TCHAR* Field, int32 MinCount, TFunctionRef<void(const TArray<TSharedPtr<FJsonValue>>&)> Use)
	{
		const TArray<TSharedPtr<FJsonValue>>* Rows = nullptr;
		if (!Object.TryGetArrayField(Field, Rows))
		{
			return;
		}
		for (const TSharedPtr<FJsonValue>& RowValue : *Rows)
		{
			const TArray<TSharedPtr<FJsonValue>>* Numbers = nullptr;
			if (RowValue.IsValid() && RowValue->TryGetArray(Numbers) && Numbers->Num() >= MinCount)
			{
				Use(*Numbers);
			}
		}
	}

	/** A park's or square's paths, beds, hedges and trees (street_furniture.json). */
	void PaintGarden(FCanvas& Canvas, const FJsonObject& Garden)
	{
		ForEachRow(Garden, TEXT("paths"), 5, [&Canvas](const TArray<TSharedPtr<FJsonValue>>& E)
		{
			Canvas.Band(E[0]->AsNumber(), E[1]->AsNumber(), E[2]->AsNumber(), E[3]->AsNumber(), E[4]->AsNumber(), PathInk);
		});
		ForEachRow(Garden, TEXT("islands"), 5, [&Canvas](const TArray<TSharedPtr<FJsonValue>>& E)
		{
			Canvas.Band(E[0]->AsNumber(), E[1]->AsNumber(), E[2]->AsNumber(), E[3]->AsNumber(), E[4]->AsNumber(), Plaza);
		});
		ForEachRow(Garden, TEXT("rounds"), 4, [&Canvas](const TArray<TSharedPtr<FJsonValue>>& E)
		{
			Canvas.Disc(E[0]->AsNumber(), E[1]->AsNumber(), E[2]->AsNumber(), PathInk);
			Canvas.Disc(E[0]->AsNumber(), E[1]->AsNumber(), E[3]->AsNumber(), Bed);
		});
		ForEachRow(Garden, TEXT("hedges"), 4, [&Canvas](const TArray<TSharedPtr<FJsonValue>>& E)
		{
			Canvas.Band(E[0]->AsNumber(), E[1]->AsNumber(), E[2]->AsNumber(), E[3]->AsNumber(), HedgeWidth, Hedge);
		});
		ForEachRow(Garden, TEXT("trees"), 3, [&Canvas](const TArray<TSharedPtr<FJsonValue>>& E)
		{
			Canvas.Disc(E[0]->AsNumber(), E[1]->AsNumber(), E[2]->AsNumber() * CrownRadius, Tree);
		});
	}

	/** Points of a flat [x, y, ...] list. */
	TArray<FVector2D> ReadPoints(const FJsonObject& Object)
	{
		TArray<FVector2D> Points;
		const TArray<TSharedPtr<FJsonValue>>* Coords = nullptr;
		if (Object.TryGetArrayField(TEXT("points"), Coords))
		{
			for (int32 i = 0; i + 1 < Coords->Num(); i += 2)
			{
				Points.Emplace((*Coords)[i]->AsNumber(), (*Coords)[i + 1]->AsNumber());
			}
		}
		return Points;
	}

	/** The texture from the pixels, with its smaller copies for when the map is zoomed out. */
	UTexture2D* MakeTexture(const TArray<FColor>& Pixels)
	{
		UTexture2D* Texture = UTexture2D::CreateTransient(Size, Size, PF_B8G8R8A8);
		if (!Texture)
		{
			return nullptr;
		}
		Texture->SRGB = true;
		Texture->LODGroup = TEXTUREGROUP_UI;
		Texture->Filter = TF_Trilinear;
		Texture->AddressX = TA_Clamp;
		Texture->AddressY = TA_Clamp;
		Texture->NeverStream = true;

		// FColor is laid out blue, green, red, alpha, as the texture wants.
		TArray<FColor> Level = Pixels;
		FTexturePlatformData* Data = Texture->GetPlatformData();
		int32 LevelSize = Size;
		for (int32 MipIndex = 0;; ++MipIndex)
		{
			FTexture2DMipMap* Mip = nullptr;
			if (MipIndex == 0)
			{
				Mip = &Data->Mips[0];
			}
			else
			{
				Mip = new FTexture2DMipMap(LevelSize, LevelSize, 1);
				Data->Mips.Add(Mip);
			}
			const int64 Bytes = static_cast<int64>(Level.Num()) * sizeof(FColor);
			Mip->BulkData.Lock(LOCK_READ_WRITE);
			void* Dest = Mip->BulkData.Realloc(Bytes);
			FMemory::Memcpy(Dest, Level.GetData(), Bytes);
			Mip->BulkData.Unlock();

			if (LevelSize == 1)
			{
				break;
			}
			// Each pixel of the next one is the average of four.
			const int32 Half = LevelSize / 2;
			TArray<FColor> Smaller;
			Smaller.SetNumUninitialized(Half * Half);
			for (int32 Y = 0; Y < Half; ++Y)
			{
				for (int32 X = 0; X < Half; ++X)
				{
					const FColor& P00 = Level[(2 * Y) * LevelSize + 2 * X];
					const FColor& P01 = Level[(2 * Y) * LevelSize + 2 * X + 1];
					const FColor& P10 = Level[(2 * Y + 1) * LevelSize + 2 * X];
					const FColor& P11 = Level[(2 * Y + 1) * LevelSize + 2 * X + 1];
					Smaller[Y * Half + X] = FColor(
						static_cast<uint8>((P00.R + P01.R + P10.R + P11.R + 2) / 4),
						static_cast<uint8>((P00.G + P01.G + P10.G + P11.G + 2) / 4),
						static_cast<uint8>((P00.B + P01.B + P10.B + P11.B + 2) / 4),
						255);
				}
			}
			Level = MoveTemp(Smaller);
			LevelSize = Half;
		}

		Texture->UpdateResource();
		Texture->AddToRoot();
		return Texture;
	}

	bool Bake(PestiMap::FPlan& Plan)
	{
		const FString Path = FPaths::Combine(FPaths::ProjectDir(), DataFile);
		FString Text;
		if (!FFileHelper::LoadFileToString(Text, *Path))
		{
			return false;
		}
		TSharedPtr<FJsonObject> Root;
		if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) || !Root.IsValid())
		{
			return false;
		}
		const TSharedPtr<FJsonObject>* Bounds = nullptr;
		if (!Root->TryGetObjectField(TEXT("bounds"), Bounds) || !Bounds || !Bounds->IsValid())
		{
			return false;
		}
		const double MinX = (*Bounds)->GetNumberField(TEXT("minX"));
		const double MinY = (*Bounds)->GetNumberField(TEXT("minY"));
		const double MaxX = (*Bounds)->GetNumberField(TEXT("maxX"));
		const double MaxY = (*Bounds)->GetNumberField(TEXT("maxY"));
		Plan.Side = FMath::Max3(MaxX - MinX, MaxY - MinY, 1.0);
		Plan.Corner = FVector2D((MinX + MaxX - Plan.Side) * 0.5, (MinY + MaxY - Plan.Side) * 0.5);

		FCanvas Canvas;
		Canvas.Pixels.Init(Ground, Size * Size);
		Canvas.Corner = Plan.Corner;
		Canvas.PixelsPerCm = Size / Plan.Side;

		// The dressing files: blocks the game builds differently, the cemetery's lawn and graves, the gardens.
		const TSharedPtr<FJsonObject> Details = LoadJson(DetailsFile);
		const TSharedPtr<FJsonObject> Furniture = LoadJson(FurnitureFile);

		// Roads, squares, parks and rails first, then the gardens, then the blocks over them.
		TArray<TSharedPtr<FJsonValue>> AllAreas;
		const TArray<TSharedPtr<FJsonValue>>* Areas = nullptr;
		if (Root->TryGetArrayField(TEXT("areas"), Areas))
		{
			AllAreas.Append(*Areas);
		}
		if (Details.IsValid() && Details->TryGetArrayField(TEXT("areas"), Areas))
		{
			AllAreas.Append(*Areas);
		}
		{
			for (const TSharedPtr<FJsonValue>& Value : AllAreas)
			{
				const TSharedPtr<FJsonObject> Area = Value->AsObject();
				if (!Area.IsValid())
				{
					continue;
				}
				const FString Layer = Area->GetStringField(TEXT("layer"));
				const FColor* Color = Layer == TEXT("road") ? &Road : Layer == TEXT("plaza") ? &Plaza : Layer == TEXT("park") ? &Park
					: Layer == TEXT("rail") ? &Rail : nullptr;
				const TArray<TSharedPtr<FJsonValue>>* Shapes = nullptr;
				if (Color && Area->TryGetArrayField(TEXT("polygons"), Shapes))
				{
					for (const TSharedPtr<FJsonValue>& ShapeValue : *Shapes)
					{
						if (const TSharedPtr<FJsonObject> Shape = ShapeValue->AsObject())
						{
							Canvas.Polygon(*Shape, *Color);
						}
					}
				}
			}
		}
		if (Furniture.IsValid())
		{
			const TSharedPtr<FJsonObject>* Market = nullptr;
			const TArray<TSharedPtr<FJsonValue>>* MarketGround = nullptr;
			const TArray<TSharedPtr<FJsonValue>>* MarketCorners = nullptr;
			if (Furniture->TryGetObjectField(TEXT("market"), Market) && (*Market)->TryGetArrayField(TEXT("ground"), MarketGround)
				&& (*Market)->TryGetArrayField(TEXT("tris"), MarketCorners))
			{
				const TArray<TSharedPtr<FJsonValue>> GroundRings = { MakeShared<FJsonValueArray>(*MarketGround) };
				Canvas.Fill(GroundRings, *MarketCorners, Plaza, nullptr);
			}
			for (const TCHAR* GardenName : { TEXT("tisza"), TEXT("baross"), TEXT("rakoczi"), TEXT("kerepesi"), TEXT("rozsak") })
			{
				const TSharedPtr<FJsonObject>* Garden = nullptr;
				if (Furniture->TryGetObjectField(GardenName, Garden))
				{
					PaintGarden(Canvas, **Garden);
				}
			}
			ForEachRow(*Furniture, TEXT("sidewalks"), 4, [&Canvas](const TArray<TSharedPtr<FJsonValue>>& E)
			{
				Canvas.Line(Canvas.ToPixel(E[0]->AsNumber(), E[1]->AsNumber()), Canvas.ToPixel(E[2]->AsNumber(), E[3]->AsNumber()), Kerb);
			});
		}

		// The cemetery's graves and wall.
		const TSharedPtr<FJsonObject>* Cemetery = nullptr;
		if (Details.IsValid() && Details->TryGetObjectField(TEXT("cemetery"), Cemetery))
		{
			ForEachRow(**Cemetery, TEXT("graves"), 2, [&Canvas](const TArray<TSharedPtr<FJsonValue>>& E)
			{
				Canvas.Disc(E[0]->AsNumber(), E[1]->AsNumber(), GraveRadius, Grave);
			});
			ForEachRow(**Cemetery, TEXT("walls"), 4, [&Canvas](const TArray<TSharedPtr<FJsonValue>>& E)
			{
				Canvas.Band(E[0]->AsNumber(), E[1]->AsNumber(), E[2]->AsNumber(), E[3]->AsNumber(), WallWidth, CemeteryWall);
			});
		}

		// The blocks: the game's own landmarks lighter with a gilt edge, blocks it builds differently as it builds them.
		TSet<FString> LandmarkIds;
		TMap<FString, TArray<TSharedPtr<FJsonObject>>> Replaced;
		if (Details.IsValid())
		{
			const TArray<TSharedPtr<FJsonValue>>* Ids = nullptr;
			if (Details->TryGetArrayField(TEXT("map_landmarks"), Ids))
			{
				for (const TSharedPtr<FJsonValue>& Id : *Ids)
				{
					LandmarkIds.Add(Id->AsString());
				}
			}
			const TArray<TSharedPtr<FJsonValue>>* Pieces = nullptr;
			if (Details->TryGetArrayField(TEXT("buildings"), Pieces))
			{
				for (const TSharedPtr<FJsonValue>& PieceValue : *Pieces)
				{
					const TSharedPtr<FJsonObject> Piece = PieceValue->AsObject();
					FString Replaces;
					if (Piece.IsValid() && Piece->TryGetStringField(TEXT("replaces"), Replaces))
					{
						Replaced.FindOrAdd(Replaces).Add(Piece);
					}
				}
			}
		}
		const TArray<TSharedPtr<FJsonValue>>* Blocks = nullptr;
		if (Root->TryGetArrayField(TEXT("buildings"), Blocks))
		{
			for (const TSharedPtr<FJsonValue>& BlockValue : *Blocks)
			{
				const TSharedPtr<FJsonObject> Block = BlockValue->AsObject();
				if (!Block.IsValid())
				{
					continue;
				}
				const FString Id = Block->GetStringField(TEXT("id"));
				if (LandmarkIds.Contains(Id))
				{
					Canvas.Polygon(*Block, Landmark, &LandmarkEdge);
				}
				else if (const TArray<TSharedPtr<FJsonObject>>* Instead = Replaced.Find(Id))
				{
					for (const TSharedPtr<FJsonObject>& Piece : *Instead)
					{
						bool bGone = false;
						if (!Piece->TryGetBoolField(TEXT("gone"), bGone) || !bGone)
						{
							Canvas.Polygon(*Piece, Building, &BuildingEdge);
						}
					}
				}
				else
				{
					Canvas.Polygon(*Block, Building, &BuildingEdge);
				}
			}
		}
		Canvas.Polygons(*Root, TEXT("landmarks"), Landmark, &LandmarkEdge);

		// The Nepszinhaz (an actor of its own, not in the map data): the rounded back first, then the walls over it.
		{
			const double FrontX = TheatreX - TheatreFront;
			const double BackX = TheatreX + TheatreBack;
			const double North = TheatreY - TheatreHalfWidth;
			const double South = TheatreY + TheatreHalfWidth;
			Canvas.Disc(BackX, TheatreY, TheatreApse, LandmarkEdge);
			Canvas.Disc(BackX, TheatreY, TheatreApse - 60.0, Landmark);
			Canvas.Band(FrontX, TheatreY, BackX, TheatreY, 2.0 * TheatreHalfWidth, Landmark);
			Canvas.Line(Canvas.ToPixel(FrontX, North), Canvas.ToPixel(BackX, North), LandmarkEdge);
			Canvas.Line(Canvas.ToPixel(FrontX, South), Canvas.ToPixel(BackX, South), LandmarkEdge);
			Canvas.Line(Canvas.ToPixel(FrontX, North), Canvas.ToPixel(FrontX, South), LandmarkEdge);
			Canvas.Line(Canvas.ToPixel(BackX, North), Canvas.ToPixel(BackX, TheatreY - TheatreApse), LandmarkEdge);
			Canvas.Line(Canvas.ToPixel(BackX, TheatreY + TheatreApse), Canvas.ToPixel(BackX, South), LandmarkEdge);
		}

		const TArray<TSharedPtr<FJsonValue>>* Trams = nullptr;
		if (Root->TryGetArrayField(TEXT("trams"), Trams))
		{
			for (const TSharedPtr<FJsonValue>& Value : *Trams)
			{
				if (const TSharedPtr<FJsonObject> Line = Value->AsObject())
				{
					const TArray<FVector2D> Points = ReadPoints(*Line);
					for (int32 i = 0; i + 1 < Points.Num(); ++i)
					{
						Canvas.Band(Points[i].X, Points[i].Y, Points[i + 1].X, Points[i + 1].Y, TramWidth, Tram);
					}
				}
			}
		}

		// The named streets, for the name under the minimap.
		const TArray<TSharedPtr<FJsonValue>>* Centrelines = nullptr;
		if (Root->TryGetArrayField(TEXT("centerlines"), Centrelines))
		{
			for (const TSharedPtr<FJsonValue>& Value : *Centrelines)
			{
				const TSharedPtr<FJsonObject> Way = Value->AsObject();
				FString Name;
				if (!Way.IsValid() || !Way->TryGetStringField(TEXT("name"), Name) || Name.IsEmpty())
				{
					continue;
				}
				PestiMap::FStreet& Street = Plan.Streets.AddDefaulted_GetRef();
				Street.Name = PeriodName(Name);
				Street.Points = ReadPoints(*Way);
				for (const FVector2D& Point : Street.Points)
				{
					Street.Bounds += Point;
				}
			}
		}

		UTexture2D* Texture = MakeTexture(Canvas.Pixels);
		if (!Texture)
		{
			return false;
		}
		Plan.Brush.DrawAs = ESlateBrushDrawType::Image;
		Plan.Brush.ImageSize = FVector2D(Size, Size);
		Plan.Brush.SetResourceObject(Texture);
		return true;
	}
}

FString PestiMap::FPlan::StreetAt(const FVector2D& At) const
{
	double Best = PestiMapBake::NameDistance;
	const FStreet* Found = nullptr;
	for (const FStreet& Street : Streets)
	{
		if (!Street.Bounds.ExpandBy(Best).IsInside(At))
		{
			continue;
		}
		for (int32 i = 0; i + 1 < Street.Points.Num(); ++i)
		{
			const FVector2D Closest = FMath::ClosestPointOnSegment2D(At, Street.Points[i], Street.Points[i + 1]);
			const double Distance = FVector2D::Distance(Closest, At);
			if (Distance < Best)
			{
				Best = Distance;
				Found = &Street;
			}
		}
	}
	return Found && !Found->Name.IsEmpty() ? Found->Name : FString(PestiMapBake::District);
}

const PestiMap::FPlan* PestiMap::Get()
{
	// Baked once and kept for as long as the game (or the editor) runs.
	static FPlan* Plan = nullptr;
	static bool bTried = false;
	if (!bTried)
	{
		bTried = true;
		FPlan* Baked = new FPlan();
		if (PestiMapBake::Bake(*Baked))
		{
			Plan = Baked;
		}
		else
		{
			delete Baked;
		}
	}
	return Plan;
}
