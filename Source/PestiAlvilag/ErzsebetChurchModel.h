// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ChurchModel.h"

/**
 * The Árpád-házi Szent Erzsébet church on Rózsák tere (Steindl, 1901), as it
 * stood free in the middle of the square in 1932: red brick with pale stone
 * trim, two 76 m towers with stone spires either side of a gabled front with
 * three portals and a big rose window, a nave with lower aisles, transepts
 * with rounded ends, a rounded apse at the back, and the roofs laid with
 * glazed Zsolnay tiles in coloured bands, a slim spire over the crossing.
 *
 * Header-only so it can be built and looked at outside the engine. Frame as
 * ChurchModel's: the middle of the front at the foot, +X out of the front,
 * Y along it, the church running back to -X for Depth.
 */
namespace ErzsebetChurchModel
{
	using MarketModel::FShape;
	using BozsiHouseModel::Add;
	using BozsiHouseModel::FLamp;

	namespace Colors
	{
		const FLinearColor Brick(0.40f, 0.17f, 0.11f);
		const FLinearColor BrickDark(0.30f, 0.12f, 0.08f);
		const FLinearColor Stone(0.70f, 0.64f, 0.52f);
		const FLinearColor TileBrown(0.30f, 0.17f, 0.09f);
		const FLinearColor TileGreen(0.10f, 0.30f, 0.18f);
		const FLinearColor TileYellow(0.72f, 0.52f, 0.14f);
		const FLinearColor SpireStone(0.52f, 0.48f, 0.40f);
	}

	// Sizes (cm), from the map block and the real church.
	constexpr double FrontHalf = 1400.0;
	constexpr double TowerSize = 760.0;
	constexpr double TowerShaft = 4700.0;
	constexpr double BelfryTop = 5500.0;
	constexpr double SpireTop = 7400.0;
	constexpr double NaveHalf = 650.0;
	constexpr double NaveEaves = 2100.0;
	constexpr double NaveRise = 1100.0;
	constexpr double AisleOut = 600.0;
	constexpr double AisleEaves = 1100.0;
	constexpr double TransHalf = 650.0;
	constexpr double TransReach = 1450.0;
	constexpr double ApseRadius = 620.0;

	/**
	 * A pitched roof over X0..X1 (ridge along X), -Half..Half across, eaves at
	 * Z, Rise high, in glazed tiles: a brown field with green and yellow bands.
	 */
	inline void TiledRoof(TArray<FShape>& Out, double X0, double X1, double Half, double Z, double Rise)
	{
		using namespace Colors;
		ChurchModel::RidgeRoof(Out, TileBrown, X0, X1, Half, Z, Rise);
		const double Slope = FMath::RadiansToDegrees(FMath::Atan2(Rise, Half));
		const double Run = FMath::Sqrt(Half * Half + Rise * Rise);
		// Bands across the slope, a little proud of it, at three heights.
		for (const double Share : { 0.25, 0.5, 0.75 })
		{
			const FLinearColor& Band = Share == 0.5 ? TileYellow : TileGreen;
			for (const double Side : { -1.0, 1.0 })
			{
				const double Across = Half * (1.0 - Share);
				const double Up = Z + Rise * Share;
				const double Lift = 9.0;
				Add(Out, MarketModel::Cube, Band, FVector((X0 + X1) * 0.5, Side * (Across + Lift * FMath::Sin(FMath::DegreesToRadians(Slope))), Up + Lift),
					FVector(X1 - X0 + 30.0, Run * 0.09, 6.0), 0.0, false, 0.0, Side * Slope);
			}
		}
	}

	/** A gilt cross standing at X, Y, Z, Size tall (ChurchModel::Cross stands on the middle line only). */
	inline void CrossAt(TArray<FShape>& Out, double X, double Y, double Z, double Size)
	{
		Add(Out, MarketModel::Cube, ChurchModel::Colors::Gilt, FVector(X, Y, Z + Size * 0.5), FVector(Size * 0.08, Size * 0.08, Size), 0.0, false);
		Add(Out, MarketModel::Cube, ChurchModel::Colors::Gilt, FVector(X, Y, Z + Size * 0.68), FVector(Size * 0.08, Size * 0.55, Size * 0.08), 0.0, false);
	}

	/** A pointed portal on the front at Y, Width wide: stone frame, door, a gable (wimperg) over it. */
	inline void Portal(TArray<FShape>& Out, double X, double Y, double Width, double Height)
	{
		using namespace Colors;
		Add(Out, MarketModel::Cube, Stone, FVector(X + 20, Y, Height * 0.5), FVector(60, Width + 80, Height), 0.0, false);
		Add(Out, MarketModel::Cube, ChurchModel::Colors::Door, FVector(X + 52, Y, Height * 0.42), FVector(6, Width, Height * 0.84), 0.0, false);
		Add(Out, MarketModel::Cone, Stone, FVector(X + 24, Y, Height + Width * 0.45), FVector(60, Width + 80, Width * 0.9), 0.0, false);
		Add(Out, MarketModel::Cone, ChurchModel::Colors::Door, FVector(X + 54, Y, Height * 0.84 + Width * 0.22), FVector(4, Width, Width * 0.5), 0.0, false);
		for (const double S : { -1.0, 1.0 })
		{
			Add(Out, MarketModel::Cube, Stone, FVector(X + 40, Y + S * (Width * 0.5 + 55.0), (Height + Width) * 0.5), FVector(40, 30, Height + Width), 0.0, false);
			Add(Out, MarketModel::Cone, Stone, FVector(X + 40, Y + S * (Width * 0.5 + 55.0), Height + Width + 60.0), FVector(40, 30, 120), 0.0, false);
		}
	}

	/** A round window (rose) at X facing +X, Size across. */
	inline void Rose(TArray<FShape>& Out, double X, double Y, double Z, double Size, double Yaw = 0.0)
	{
		const FRotator Turn(0.0, Yaw, 0.0);
		auto P = [&](double DX, double DY, double DZ) { return FVector(X, Y, Z) + Turn.RotateVector(FVector(DX, DY, DZ)); };
		Add(Out, MarketModel::Cylinder, Colors::Stone, P(4, 0, 0), FVector(Size + 60, Size + 60, 8), Yaw, false, 90.0);
		Add(Out, MarketModel::Cylinder, ChurchModel::Colors::Glass, P(9, 0, 0), FVector(Size, Size, 6), Yaw, false, 90.0);
		Add(Out, MarketModel::Cylinder, Colors::Stone, P(12, 0, 0), FVector(Size * 0.28, Size * 0.28, 6), Yaw, false, 90.0);
		for (int32 k = 0; k < 6; ++k)
		{
			Add(Out, MarketModel::Cube, Colors::Stone, P(12, 0, 0), FVector(4, Size, 10), Yaw, false, 0.0, k * 30.0);
		}
	}

	/** One front tower, its middle at Y: square brick shaft with stone corners and bands, an eight-sided belfry, the stone spire. */
	inline void Tower(TArray<FShape>& Out, TArray<FLamp>& Lamps, double Y)
	{
		using namespace Colors;
		const double TX = -TowerSize * 0.5;
		const double Half = TowerSize * 0.5;
		Add(Out, MarketModel::Cube, Brick, FVector(TX, Y, TowerShaft * 0.5), FVector(TowerSize, TowerSize, TowerShaft));
		for (const double Z : { 60.0, 1100.0, 2100.0, 3200.0, TowerShaft - 30.0 })
		{
			Add(Out, MarketModel::Cube, Stone, FVector(TX, Y, Z), FVector(TowerSize + 40, TowerSize + 40, 50), 0.0, false);
		}
		for (const double CX : { -1.0, 1.0 })
		{
			for (const double CY : { -1.0, 1.0 })
			{
				// Stepped corner buttresses, a pinnacle on top.
				Add(Out, MarketModel::Cube, Stone, FVector(TX + CX * (Half + 15.0), Y + CY * (Half + 15.0), TowerShaft * 0.46), FVector(90, 90, TowerShaft * 0.92));
				Add(Out, MarketModel::Cube, Stone, FVector(TX + CX * (Half + 5.0), Y + CY * (Half + 5.0), TowerShaft + 150.0), FVector(70, 70, 300), 0.0, false);
				Add(Out, MarketModel::Cone, SpireStone, FVector(TX + CX * (Half + 5.0), Y + CY * (Half + 5.0), TowerShaft + 500.0), FVector(80, 80, 400), 0.0, false);
			}
		}
		// Tall pointed windows up the faces, the bell openings and clock at the top.
		for (int32 Face = 0; Face < 4; ++Face)
		{
			const double FaceYaw = Face * 90.0;
			const FVector Mid = FVector(TX, Y, 0) + FRotator(0.0, FaceYaw, 0.0).RotateVector(FVector(Half, 0, 0));
			ChurchModel::TallWindow(Out, Mid + FVector(0, 0, 2300), FaceYaw, 120.0, 700.0, Stone, true);
			for (const double S : { -150.0, 150.0 })
			{
				ChurchModel::TallWindow(Out, Mid + FRotator(0.0, FaceYaw, 0.0).RotateVector(FVector(0, S, 3500)), FaceYaw, 110.0, 800.0, Stone, true);
			}
		}
		Add(Out, MarketModel::Cylinder, ChurchModel::Colors::Gilt, FVector(TX + Half + 6.0, Y, 3000), FVector(190, 190, 4), 0.0, false, 90.0);
		Add(Out, MarketModel::Cylinder, ChurchModel::Colors::ClockFace, FVector(TX + Half + 9.0, Y, 3000), FVector(170, 170, 4), 0.0, false, 90.0);
		// The eight-sided belfry and the spire with its crockets.
		Add(Out, MarketModel::Cylinder, Brick, FVector(TX, Y, (TowerShaft + BelfryTop) * 0.5), FVector(TowerSize * 0.86, TowerSize * 0.86, BelfryTop - TowerShaft), 22.5);
		Add(Out, MarketModel::Cylinder, Stone, FVector(TX, Y, BelfryTop), FVector(TowerSize * 0.94, TowerSize * 0.94, 50), 22.5, false);
		for (int32 k = 0; k < 8; ++k)
		{
			const double A = 22.5 + k * 45.0;
			const FVector Slot = FVector(TX, Y, 0) + FRotator(0.0, A, 0.0).RotateVector(FVector(TowerSize * 0.40, 0, 0));
			Add(Out, MarketModel::Cube, ChurchModel::Colors::Dark, Slot + FVector(0, 0, TowerShaft + 400.0), FVector(10, 110, 480), A, false);
		}
		Add(Out, MarketModel::Cone, SpireStone, FVector(TX, Y, BelfryTop + (SpireTop - BelfryTop) * 0.5), FVector(TowerSize * 0.80, TowerSize * 0.80, SpireTop - BelfryTop), 22.5, false);
		for (const double Share : { 0.3, 0.6 })
		{
			const double Z = BelfryTop + (SpireTop - BelfryTop) * Share;
			Add(Out, MarketModel::Cylinder, Stone, FVector(TX, Y, Z), FVector(TowerSize * 0.80 * (1.0 - Share) + 20.0, TowerSize * 0.80 * (1.0 - Share) + 20.0, 24), 22.5, false);
		}
		Add(Out, MarketModel::Sphere, ChurchModel::Colors::Gilt, FVector(TX, Y, SpireTop + 10.0), FVector(50, 50, 50), 0.0, false);
		CrossAt(Out, TX, Y, SpireTop + 30.0, 260.0);
		Lamps.Add({ FVector(60.0, Y * 0.5, 650), 700.f });
	}

	/** Tall pointed windows and (unless bPlain) buttresses with pinnacles down one long wall from X0 to X1 at Y, facing Yaw. */
	inline void SideWindows(TArray<FShape>& Out, double X0, double X1, double Y, double Yaw, double Z, double Height, double Eaves, bool bPlain = false)
	{
		const int32 Bays = FMath::Max(1, FMath::RoundToInt((X1 - X0) / 520.0));
		const double Side = Yaw > 0.0 ? 1.0 : -1.0;
		for (int32 k = 0; k <= Bays; ++k)
		{
			const double X = X0 + (X1 - X0) * k / Bays;
			if (!bPlain)
			{
				Add(Out, MarketModel::Cube, Colors::Stone, FVector(X, Y + Side * 35.0, Eaves * 0.45), FVector(70, 70, Eaves * 0.9));
				Add(Out, MarketModel::Cone, Colors::SpireStone, FVector(X, Y + Side * 35.0, Eaves + 110.0), FVector(60, 60, 220), 0.0, false);
			}
			if (k < Bays)
			{
				ChurchModel::TallWindow(Out, FVector(X0 + (X1 - X0) * (k + 0.5) / Bays, Y, Z), Yaw, 140.0, Height, Colors::Stone, true);
			}
		}
	}

	/** The whole church: Depth back from the front (its front Width is fixed by the towers). */
	inline void Church(double Depth, TArray<FShape>& Out, TArray<FLamp>& Lamps)
	{
		using namespace Colors;
		const double Back = -FMath::Max(Depth, 4500.0);
		const double CrossX = Back * 0.62;
		const double ChancelEnd = Back + ApseRadius;
		const double NaveStart = -TowerSize + 40.0;

		// The nave and its aisles, front to the crossing.
		Add(Out, MarketModel::Cube, Brick, FVector((NaveStart + CrossX) * 0.5, 0, NaveEaves * 0.5), FVector(NaveStart - CrossX, NaveHalf * 2.0, NaveEaves));
		TiledRoof(Out, CrossX, NaveStart, NaveHalf + 40.0, NaveEaves, NaveRise);
		for (const double Side : { -1.0, 1.0 })
		{
			const double AisleY = Side * (NaveHalf + AisleOut * 0.5);
			Add(Out, MarketModel::Cube, Brick, FVector((NaveStart + CrossX) * 0.5, AisleY, AisleEaves * 0.5), FVector(NaveStart - CrossX, AisleOut, AisleEaves));
			// A lean-to roof from the aisle wall up to the nave.
			const double Tilt = FMath::RadiansToDegrees(FMath::Atan2(250.0, AisleOut));
			Add(Out, MarketModel::Cube, TileBrown, FVector((NaveStart + CrossX) * 0.5, AisleY, AisleEaves + 125.0), FVector(NaveStart - CrossX + 30.0, AisleOut / FMath::Cos(FMath::DegreesToRadians(Tilt)) + 20.0, 14.0), 0.0, true, 0.0, Side * Tilt);
			SideWindows(Out, CrossX + TransHalf, NaveStart, Side * (NaveHalf + AisleOut), Side * 90.0, 250.0, 600.0, AisleEaves);
			// The high windows over the aisle roofs (no buttresses: they would come up through the aisle roof).
			SideWindows(Out, CrossX + TransHalf, NaveStart, Side * NaveHalf, Side * 90.0, AisleEaves + 330.0, 480.0, NaveEaves, true);
		}

		// The transept with its rounded ends and a rose window in each.
		Add(Out, MarketModel::Cube, Brick, FVector(CrossX, 0, NaveEaves * 0.5), FVector(TransHalf * 2.0, TransReach * 2.0, NaveEaves));
		{
			// Its roof runs across the nave's: built along X, then turned.
			TArray<FShape> Roof;
			TiledRoof(Roof, -TransReach, TransReach, TransHalf + 40.0, NaveEaves, NaveRise);
			// Brick gables close the roof's ends over the rounded ends.
			ChurchModel::Gable(Roof, Brick, -TransReach + 20.0, 40.0, TransHalf, NaveEaves, NaveRise);
			ChurchModel::Gable(Roof, Brick, TransReach - 20.0, 40.0, TransHalf, NaveEaves, NaveRise);
			for (FShape Piece : Roof)
			{
				const FVector Local = Piece.At;
				Piece.At = FVector(CrossX - Local.Y, Local.X, Local.Z);
				Piece.Yaw += 90.0;
				Out.Add(Piece);
			}
		}
		for (const double Side : { -1.0, 1.0 })
		{
			const double EndY = Side * TransReach;
			Add(Out, MarketModel::Cylinder, Brick, FVector(CrossX, EndY, (NaveEaves - 100.0) * 0.5), FVector(TransHalf * 2.0, TransHalf * 2.0, NaveEaves - 100.0), 22.5);
			// The end's cone roof rises to the ridge, so it meets the gable.
			Add(Out, MarketModel::Cone, TileBrown, FVector(CrossX, EndY, NaveEaves - 100.0 + (NaveRise + 100.0) * 0.5), FVector(TransHalf * 2.0 + 60.0, TransHalf * 2.0 + 60.0, NaveRise + 100.0), 22.5, false);
			for (const double Turn : { -50.0, 0.0, 50.0 })
			{
				const double FaceYaw = Side * 90.0 + Turn;
				const FVector Foot = FVector(CrossX, EndY, 0) + FRotator(0.0, FaceYaw, 0.0).RotateVector(FVector(TransHalf - 15.0, 0, 0));
				ChurchModel::TallWindow(Out, Foot + FVector(0, 0, 400), FaceYaw, 140.0, 900.0, Stone, true);
			}
		}

		// The chancel and its rounded apse.
		Add(Out, MarketModel::Cube, Brick, FVector((CrossX + ChancelEnd) * 0.5, 0, NaveEaves * 0.5), FVector(CrossX - ChancelEnd, NaveHalf * 2.0, NaveEaves));
		TiledRoof(Out, ChancelEnd, CrossX, NaveHalf + 40.0, NaveEaves, NaveRise);
		Add(Out, MarketModel::Cylinder, Brick, FVector(ChancelEnd, 0, (NaveEaves - 100.0) * 0.5), FVector(ApseRadius * 2.0, ApseRadius * 2.0, NaveEaves - 100.0), 22.5);
		Add(Out, MarketModel::Cone, TileBrown, FVector(ChancelEnd, 0, NaveEaves - 100.0 + (NaveRise + 100.0) * 0.5), FVector(ApseRadius * 2.0 + 60.0, ApseRadius * 2.0 + 60.0, NaveRise + 100.0), 22.5, false);
		ChurchModel::Gable(Out, Brick, ChancelEnd + 20.0, 40.0, NaveHalf, NaveEaves, NaveRise);
		for (const double Side : { -1.0, 1.0 })
		{
			SideWindows(Out, ChancelEnd, CrossX - TransHalf, Side * NaveHalf, Side * 90.0, 450.0, 1000.0, NaveEaves);
		}
		for (const double Turn : { -60.0, -20.0, 20.0, 60.0 })
		{
			const double FaceYaw = 180.0 + Turn;
			const FVector Foot = FVector(ChancelEnd, 0, 0) + FRotator(0.0, FaceYaw, 0.0).RotateVector(FVector(ApseRadius - 15.0, 0, 0));
			ChurchModel::TallWindow(Out, Foot + FVector(0, 0, 450), FaceYaw, 130.0, 1000.0, Stone, true);
		}

		// The slim spire over the crossing.
		const double RidgeTop = NaveEaves + NaveRise;
		Add(Out, MarketModel::Cylinder, Stone, FVector(CrossX, 0, RidgeTop + 250.0), FVector(220, 220, 500), 22.5, false);
		Add(Out, MarketModel::Cone, SpireStone, FVector(CrossX, 0, RidgeTop + 500.0 + 700.0), FVector(240, 240, 1400), 22.5, false);
		ChurchModel::Cross(Out, CrossX, RidgeTop + 1900.0, 180.0);

		// The front: the two towers, the gable between them with the rose window, three portals.
		for (const double Side : { -1.0, 1.0 })
		{
			Tower(Out, Lamps, Side * (FrontHalf - TowerSize * 0.5));
		}
		const double GableHalf = FrontHalf - TowerSize;
		Add(Out, MarketModel::Cube, Brick, FVector(-TowerSize * 0.5, 0, NaveEaves * 0.5), FVector(TowerSize, GableHalf * 2.0, NaveEaves));
		ChurchModel::Gable(Out, Brick, -30.0, 60.0, GableHalf, NaveEaves, NaveRise + 200.0);
		Add(Out, MarketModel::Cube, Stone, FVector(5, 0, NaveEaves), FVector(50, GableHalf * 2.0, 50), 0.0, false);
		// A row of small pointed arches (a gallery) under the rose window.
		for (int32 k = 0; k < 7; ++k)
		{
			const double Y = -GableHalf * 0.75 + GableHalf * 1.5 * k / 6.0;
			Add(Out, MarketModel::Cube, ChurchModel::Colors::Dark, FVector(4, Y, 1180), FVector(6, 50, 160), 0.0, false);
			Add(Out, MarketModel::Cone, Stone, FVector(6, Y, 1290), FVector(10, 70, 60), 0.0, false);
		}
		Add(Out, MarketModel::Cube, Stone, FVector(8, 0, 1080), FVector(30, GableHalf * 1.7, 24), 0.0, false);
		Rose(Out, 0.0, 0.0, 1650.0, 640.0);
		ChurchModel::Cross(Out, -10.0, NaveEaves + NaveRise + 200.0, 220.0);
		Portal(Out, 0.0, 0.0, 260.0, 520.0);
		for (const double Side : { -1.0, 1.0 })
		{
			Portal(Out, 0.0, Side * (FrontHalf - TowerSize * 0.5), 180.0, 400.0);
		}
		// The steps across the whole front.
		Add(Out, MarketModel::Cube, Stone, FVector(80, 0, 15), FVector(160, FrontHalf * 2.0 + 100.0, 30));
		Add(Out, MarketModel::Cube, Stone, FVector(40, 0, 45), FVector(80, FrontHalf * 2.0 + 60.0, 30));
		Lamps.Add({ FVector(120.0, 0, 700), 900.f });
	}
}
