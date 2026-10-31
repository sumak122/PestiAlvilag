// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * The pictures laid over the ground: small granite setts in arcs on the
 * roads, big setts in rows on the squares and along the tram tracks, square
 * paving slabs on the pavements, gravel on the park paths and autumn grass
 * in the parks. Each is a square that repeats without a seam, drawn from a
 * list of stones: every stone is a rounded box with its own colour and a
 * domed top, laid on sand. Header-only and free of engine objects, so the
 * offline preview draws the same pictures.
 *
 * A picture has a colour (with how rough the surface is in alpha) and a
 * normal map for the bumps (X, Y, Z of the surface's slope in red, green,
 * blue, Z up), both TileCm wide in the world.
 */
namespace SurfaceModel
{
	enum class EKind : uint8 { Setts, BigSetts, Slabs, Gravel, Grass, Count };

	struct FImage
	{
		int32 Size = 0;
		double TileCm = 100.0;
		/** Size x Size pixels of R, G, B and roughness, row by row. */
		TArray<uint8> Colour;
		/** Size x Size pixels of the bump normal's X, Y and Z (0..255 for -1..1), and 255. */
		TArray<uint8> Normal;
	};

	/** One stone: a box with rounded corners, turned by Angle (radians), Height cm proud of the sand. */
	struct FStone
	{
		double X = 0.0;
		double Y = 0.0;
		double HalfLength = 5.0;
		double HalfWidth = 5.0;
		double Angle = 0.0;
		double Corner = 1.0;
		double Height = 1.0;
		FLinearColor Colour;
		float Roughness = 0.6f;
	};

	/** A repeatable random series. */
	struct FDice
	{
		uint32 State;
		explicit FDice(uint32 Seed) : State(Seed * 2654435761u + 12345u) {}
		double Unit()
		{
			State = State * 1664525u + 1013904223u;
			return (State >> 8) / 16777216.0;
		}
		double Range(double Min, double Max) { return Min + (Max - Min) * Unit(); }
	};

	/** Smooth noise that repeats every Period cells (value noise on a grid). */
	inline double Noise(double X, double Y, int32 Period, uint32 Seed)
	{
		auto Corner = [Period, Seed](int32 I, int32 J)
		{
			I = ((I % Period) + Period) % Period;
			J = ((J % Period) + Period) % Period;
			uint32 H = static_cast<uint32>(I) * 374761393u + static_cast<uint32>(J) * 668265263u + Seed * 2246822519u;
			H = (H ^ (H >> 13)) * 1274126177u;
			return ((H ^ (H >> 16)) & 0xffffu) / 65535.0;
		};
		const double FX = FMath::Floor(X);
		const double FY = FMath::Floor(Y);
		const int32 I = static_cast<int32>(FX);
		const int32 J = static_cast<int32>(FY);
		double U = X - FX;
		double V = Y - FY;
		U = U * U * (3.0 - 2.0 * U);
		V = V * V * (3.0 - 2.0 * V);
		const double A = FMath::Lerp(Corner(I, J), Corner(I + 1, J), U);
		const double B = FMath::Lerp(Corner(I, J + 1), Corner(I + 1, J + 1), U);
		return FMath::Lerp(A, B, V);
	}

	/** A few octaves of Noise across a tile of the given size in pixels (0..1). */
	inline double Speckle(double PX, double PY, int32 Size, int32 Cells, uint32 Seed)
	{
		double Sum = 0.0;
		double Weight = 0.5;
		for (int32 Octave = 0; Octave < 3; ++Octave)
		{
			const int32 Period = Cells << Octave;
			Sum += Weight * Noise(PX * Period / Size, PY * Period / Size, Period, Seed + Octave * 31u);
			Weight *= 0.5;
		}
		return Sum / 0.875;
	}

	/** Granite: greys with a little blue or brown in them. */
	inline FLinearColor Granite(FDice& Dice, double Light)
	{
		const double Grey = Light * Dice.Range(0.75, 1.2);
		const double Warm = Dice.Range(-0.02, 0.025);
		return FLinearColor(static_cast<float>(Grey + Warm), static_cast<float>(Grey + Warm * 0.4), static_cast<float>(Grey - Warm * 0.6 + 0.008));
	}

	/** What lies between the stones, and how much fine grain each stone shows. */
	struct FBed
	{
		FLinearColor Sand;
		float SandRoughness = 0.95f;
		double Grain = 0.25;
		int32 GrainCells = 32;
	};

	/**
	 * Small granite cubes (Budapest's "kiskockako") laid in arcs, a metre
	 * across, row after row: the roads.
	 */
	inline void LaySetts(TArray<FStone>& Stones, FBed& Bed, double Tile)
	{
		FDice Dice(7);
		constexpr double Cube = 10.0;
		constexpr double ArcWidth = 100.0;
		constexpr double Radius = 85.0;
		for (double Column = 0.0; Column < Tile; Column += ArcWidth)
		{
			const double Middle = Column + ArcWidth * 0.5;
			// Rows a little over a stone apart, a whole number of them to the tile.
			const double RowStep = Tile / FMath::RoundToInt(Tile / (Cube + 1.0));
			for (double Row = 0.0; Row < Tile - 1e-6; Row += RowStep)
			{
				// The arc's top is at Row; its ends drop to the sides.
				const double Half = FMath::Asin((ArcWidth * 0.5) / Radius);
				const double Step = (Cube + 1.2) / Radius;
				for (double A = -Half; A <= Half + 1e-6; A += Step)
				{
					FStone S;
					S.X = Middle + FMath::Sin(A) * Radius + Dice.Range(-0.8, 0.8);
					S.Y = Row + Radius - FMath::Cos(A) * Radius + Dice.Range(-0.8, 0.8);
					S.Angle = A + Dice.Range(-0.12, 0.12);
					S.HalfLength = Cube * 0.5 * Dice.Range(0.85, 1.0);
					S.HalfWidth = Cube * 0.5 * Dice.Range(0.85, 1.0);
					S.Corner = 1.6;
					S.Height = Dice.Range(0.7, 1.2);
					S.Colour = Granite(Dice, 0.13);
					S.Roughness = static_cast<float>(Dice.Range(0.45, 0.7));
					Stones.Add(S);
				}
			}
		}
		Bed.Sand = FLinearColor(0.06f, 0.055f, 0.045f);
		Bed.Grain = 0.35;
	}

	/** Big granite setts in rows, each row half a stone along from the last: the squares and the tram tracks. */
	inline void LayBigSetts(TArray<FStone>& Stones, FBed& Bed, double Tile)
	{
		FDice Dice(11);
		constexpr double Long = 22.0;
		constexpr double Wide = 16.0;
		int32 RowIndex = 0;
		for (double Y = Wide * 0.5; Y < Tile; Y += Wide, ++RowIndex)
		{
			for (double X = (RowIndex % 2) * Long * 0.5; X < Tile; X += Long)
			{
				FStone S;
				S.X = X + Dice.Range(-0.6, 0.6);
				S.Y = Y + Dice.Range(-0.6, 0.6);
				S.Angle = Dice.Range(-0.03, 0.03);
				S.HalfLength = Long * 0.5 - Dice.Range(0.8, 1.5);
				S.HalfWidth = Wide * 0.5 - Dice.Range(0.8, 1.4);
				S.Corner = 2.0;
				S.Height = Dice.Range(0.8, 1.4);
				S.Colour = Granite(Dice, 0.19);
				S.Roughness = static_cast<float>(Dice.Range(0.4, 0.65));
				Stones.Add(S);
			}
		}
		Bed.Sand = FLinearColor(0.07f, 0.065f, 0.055f);
		Bed.Grain = 0.3;
	}

	/** Square cement slabs, 40 cm, a little worn and uneven: the pavements. */
	inline void LaySlabs(TArray<FStone>& Stones, FBed& Bed, double Tile)
	{
		FDice Dice(5);
		constexpr double Slab = 40.0;
		for (double Y = Slab * 0.5; Y < Tile; Y += Slab)
		{
			for (double X = Slab * 0.5; X < Tile; X += Slab)
			{
				FStone S;
				S.X = X;
				S.Y = Y;
				S.HalfLength = Slab * 0.5 - 0.5;
				S.HalfWidth = Slab * 0.5 - 0.5;
				S.Corner = 0.8;
				S.Height = Dice.Range(0.25, 0.5);
				const double Light = Dice.Range(0.27, 0.34);
				S.Colour = FLinearColor(static_cast<float>(Light), static_cast<float>(Light * 0.97), static_cast<float>(Light * 0.92));
				S.Roughness = static_cast<float>(Dice.Range(0.75, 0.9));
				Stones.Add(S);
			}
		}
		Bed.Sand = FLinearColor(0.12f, 0.11f, 0.09f);
		Bed.Grain = 0.16;
		Bed.GrainCells = 16;
	}

	/** Pebbles packed on a sandy path: the park paths. */
	inline void LayGravel(TArray<FStone>& Stones, FBed& Bed, double Tile)
	{
		FDice Dice(3);
		const int32 Count = static_cast<int32>(Tile * Tile / 9.0);
		for (int32 i = 0; i < Count; ++i)
		{
			FStone S;
			S.X = Dice.Range(0.0, Tile);
			S.Y = Dice.Range(0.0, Tile);
			S.Angle = Dice.Range(0.0, 3.14159);
			S.HalfLength = Dice.Range(0.6, 1.5);
			S.HalfWidth = S.HalfLength * Dice.Range(0.6, 1.0);
			S.Corner = S.HalfWidth;
			S.Height = Dice.Range(0.3, 0.7);
			const double Light = Dice.Range(0.18, 0.40);
			S.Colour = FLinearColor(static_cast<float>(Light), static_cast<float>(Light * 0.9), static_cast<float>(Light * 0.75));
			S.Roughness = 0.8f;
			Stones.Add(S);
		}
		Bed.Sand = FLinearColor(0.32f, 0.27f, 0.19f);
		Bed.Grain = 0.3;
	}

	/** Fallen leaves on autumn grass: the parks (the grass itself is drawn in Build). */
	inline void LayLeaves(TArray<FStone>& Stones, FBed& Bed, double Tile)
	{
		FDice Dice(13);
		const FLinearColor Leaves[] = { FLinearColor(0.45f, 0.20f, 0.04f), FLinearColor(0.50f, 0.36f, 0.06f), FLinearColor(0.30f, 0.12f, 0.04f) };
		const int32 Count = static_cast<int32>(Tile * Tile / 260.0);
		for (int32 i = 0; i < Count; ++i)
		{
			FStone S;
			S.X = Dice.Range(0.0, Tile);
			S.Y = Dice.Range(0.0, Tile);
			S.Angle = Dice.Range(0.0, 3.14159);
			S.HalfLength = Dice.Range(1.5, 2.6);
			S.HalfWidth = S.HalfLength * 0.55;
			S.Corner = S.HalfWidth;
			S.Height = 0.15;
			S.Colour = Leaves[static_cast<int32>(Dice.Unit() * 2.999)];
			S.Roughness = 0.75f;
			Stones.Add(S);
		}
		Bed.Sand = FLinearColor(0.065f, 0.095f, 0.035f);
		Bed.Grain = 0.55;
		Bed.GrainCells = 64;
	}

	/** How wide one picture is in the world (cm). */
	inline double TileCm(EKind Kind)
	{
		switch (Kind)
		{
		case EKind::Setts: return 200.0;
		case EKind::BigSetts: return 176.0;
		case EKind::Slabs: return 240.0;
		case EKind::Gravel: return 90.0;
		default: return 150.0;
		}
	}

	/** Draws one kind of ground, Size pixels square. */
	inline void Build(EKind Kind, int32 Size, FImage& Out)
	{
		const double Tile = TileCm(Kind);
		TArray<FStone> Stones;
		FBed Bed;
		switch (Kind)
		{
		case EKind::Setts: LaySetts(Stones, Bed, Tile); break;
		case EKind::BigSetts: LayBigSetts(Stones, Bed, Tile); break;
		case EKind::Slabs: LaySlabs(Stones, Bed, Tile); break;
		case EKind::Gravel: LayGravel(Stones, Bed, Tile); break;
		default: LayLeaves(Stones, Bed, Tile); break;
		}

		// Stones by grid cell (with the ones near an edge also in the cells
		// across it), so each pixel only looks at the few around it.
		constexpr int32 Cells = 16;
		const double CellCm = Tile / Cells;
		TArray<TArray<int32>> Grid;
		Grid.SetNum(Cells * Cells);
		for (int32 s = 0; s < Stones.Num(); ++s)
		{
			const FStone& S = Stones[s];
			const double Reach = FMath::Max(S.HalfLength, S.HalfWidth) * 1.5;
			const int32 X0 = static_cast<int32>(FMath::Floor((S.X - Reach) / CellCm));
			const int32 X1 = static_cast<int32>(FMath::Floor((S.X + Reach) / CellCm));
			const int32 Y0 = static_cast<int32>(FMath::Floor((S.Y - Reach) / CellCm));
			const int32 Y1 = static_cast<int32>(FMath::Floor((S.Y + Reach) / CellCm));
			for (int32 CY = Y0; CY <= Y1; ++CY)
			{
				for (int32 CX = X0; CX <= X1; ++CX)
				{
					Grid[((CY % Cells + Cells) % Cells) * Cells + (CX % Cells + Cells) % Cells].Add(s);
				}
			}
		}

		const double PixelCm = Tile / Size;
		TArray<double> Height;
		TArray<float> Colour;
		Height.SetNumZeroed(Size * Size);
		Colour.SetNumZeroed(Size * Size * 4);
		for (int32 PY = 0; PY < Size; ++PY)
		{
			for (int32 PX = 0; PX < Size; ++PX)
			{
				const double X = (PX + 0.5) * PixelCm;
				const double Y = (PY + 0.5) * PixelCm;
				const double Grain = Speckle(PX, PY, Size, Bed.GrainCells, 101u) - 0.5;
				const double Fine = Speckle(PX, PY, Size, 128, 202u) - 0.5;

				// The stone this pixel lies deepest inside, if any.
				int32 Best = -1;
				double BestInside = 0.0;
				const TArray<int32>& Near = Grid[FMath::Min(Cells - 1, static_cast<int32>(Y / CellCm)) * Cells + FMath::Min(Cells - 1, static_cast<int32>(X / CellCm))];
				for (const int32 s : Near)
				{
					const FStone& S = Stones[s];
					// Nearest copy of the stone across the tile's edges.
					double DX = X - S.X;
					double DY = Y - S.Y;
					DX -= Tile * FMath::Floor(DX / Tile + 0.5);
					DY -= Tile * FMath::Floor(DY / Tile + 0.5);
					const double C = FMath::Cos(S.Angle);
					const double Sn = FMath::Sin(S.Angle);
					const double LX = FMath::Abs(DX * C + DY * Sn) - (S.HalfLength - S.Corner);
					const double LY = FMath::Abs(-DX * Sn + DY * C) - (S.HalfWidth - S.Corner);
					const double Out = FMath::Sqrt(FMath::Square(FMath::Max(LX, 0.0)) + FMath::Square(FMath::Max(LY, 0.0)))
						+ FMath::Min(FMath::Max(LX, LY), 0.0) - S.Corner;
					if (-Out > BestInside)
					{
						BestInside = -Out;
						Best = s;
					}
				}

				FLinearColor Here = Bed.Sand;
				float Rough = Bed.SandRoughness;
				double H = 0.0;
				double Shade = 1.0 + Grain * 0.6 + Fine * 0.5;
				if (Kind == EKind::Grass && Best < 0)
				{
					// Blades: light and dark streaks, with bare earth showing through.
					const double Blades = Speckle(PX * 3.0, PY * 0.8, Size * 3, 96, 303u);
					const double Bare = Speckle(PX, PY, Size, 6, 404u);
					Shade = 0.6 + Blades * 0.8;
					H = Blades * 0.4;
					if (Bare > 0.68)
					{
						const double T = FMath::Clamp((Bare - 0.68) * 6.0, 0.0, 1.0);
						Here = FLinearColor(static_cast<float>(FMath::Lerp(static_cast<double>(Here.R), 0.12, T)), static_cast<float>(FMath::Lerp(static_cast<double>(Here.G), 0.09, T)), static_cast<float>(FMath::Lerp(static_cast<double>(Here.B), 0.05, T)));
					}
				}
				if (Best >= 0)
				{
					const FStone& S = Stones[Best];
					// A domed top, with the edges worn round over the last cm or two.
					const double Edge = FMath::Clamp(BestInside / FMath::Max(S.Corner, 0.8), 0.0, 1.0);
					H = S.Height * (Edge * (2.0 - Edge));
					Here = S.Colour;
					Rough = S.Roughness;
					Shade = 1.0 + Grain * Bed.Grain + Fine * 0.25 + (1.0 - Edge) * -0.25;
				}
				else if (Kind != EKind::Grass)
				{
					H = Grain * 0.3;
				}
				const int32 At = (PY * Size + PX) * 4;
				Colour[At + 0] = static_cast<float>(Here.R * Shade);
				Colour[At + 1] = static_cast<float>(Here.G * Shade);
				Colour[At + 2] = static_cast<float>(Here.B * Shade);
				Colour[At + 3] = Rough;
				Height[PY * Size + PX] = H;
			}
		}

		// Colour to 8-bit sRGB, and the bumps from the slope of the heights.
		auto ToByte = [](double Linear)
		{
			const double C = FMath::Clamp(Linear, 0.0, 1.0);
			const double S = C <= 0.0031308 ? C * 12.92 : 1.055 * FMath::Pow(C, 1.0 / 2.4) - 0.055;
			return static_cast<uint8>(FMath::Clamp(static_cast<int32>(S * 255.0 + 0.5), 0, 255));
		};
		Out.Size = Size;
		Out.TileCm = Tile;
		Out.Colour.SetNumZeroed(Size * Size * 4);
		Out.Normal.SetNumZeroed(Size * Size * 4);
		for (int32 PY = 0; PY < Size; ++PY)
		{
			for (int32 PX = 0; PX < Size; ++PX)
			{
				const int32 i = PY * Size + PX;
				Out.Colour[i * 4 + 0] = ToByte(Colour[i * 4 + 0]);
				Out.Colour[i * 4 + 1] = ToByte(Colour[i * 4 + 1]);
				Out.Colour[i * 4 + 2] = ToByte(Colour[i * 4 + 2]);
				Out.Colour[i * 4 + 3] = static_cast<uint8>(FMath::Clamp(static_cast<int32>(Colour[i * 4 + 3] * 255.f), 0, 255));
				auto HeightAt = [&Height, Size](int32 X, int32 Y) { return Height[((Y + Size) % Size) * Size + (X + Size) % Size]; };
				const double DX = (HeightAt(PX + 1, PY) - HeightAt(PX - 1, PY)) / (2.0 * PixelCm);
				const double DY = (HeightAt(PX, PY + 1) - HeightAt(PX, PY - 1)) / (2.0 * PixelCm);
				const FVector N = FVector(-DX, -DY, 1.0).GetSafeNormal();
				Out.Normal[i * 4 + 0] = static_cast<uint8>(FMath::Clamp(static_cast<int32>((N.X * 0.5 + 0.5) * 255.0 + 0.5), 0, 255));
				Out.Normal[i * 4 + 1] = static_cast<uint8>(FMath::Clamp(static_cast<int32>((N.Y * 0.5 + 0.5) * 255.0 + 0.5), 0, 255));
				Out.Normal[i * 4 + 2] = static_cast<uint8>(FMath::Clamp(static_cast<int32>((N.Z * 0.5 + 0.5) * 255.0 + 0.5), 0, 255));
				Out.Normal[i * 4 + 3] = 255;
			}
		}
	}
}
