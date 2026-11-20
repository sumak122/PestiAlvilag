// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MarketModel.h"

/**
 * The Kerepesi út trotting track (ügetőpálya) of 1932 at the east end of the
 * map, as basic shapes: the sand oval with its white rails, the grandstand
 * facing Kerepesi út's side of the track, the betting windows
 * (totalizatőr), the judges' box at the finish, the board fence with its
 * gate, the stables, and nearby the Keleti engine shed and water tower in
 * the rail yard and the old stand left on the gallop course (races moved
 * out of town in 1925). The infield and the gallop course's grass, the inn
 * and the jockeys' houses are in the map data (osm_to_graybox.py
 * east_1932).
 *
 * Frame: the track's middle at ground level, +X east, +Y south (the city's
 * axes, no turn). ATrotTrack builds it; header only so the preview kit can
 * draw it outside the engine.
 */
namespace TrotTrackModel
{
	using MarketModel::FShape;
	using MarketModel::FBuilder;
	using MarketModel::Cube;
	using MarketModel::Cylinder;

	// The track's middle in the city (cm), and its shape: half the length of
	// the straights, the outer radius and the width of the sand. Keep in
	// step with osm_to_graybox.py TROT_MIDDLE, TROT_STRAIGHT, TROT_OUTER,
	// TROT_SAND (there in metres, y north).
	const FVector2D Middle(170000.0, -19500.0);
	constexpr double Straight = 20500.0;
	constexpr double Outer = 9500.0;
	constexpr double Sand = 1800.0;
	constexpr double Inner = Outer - Sand;

	// The grandstand on the north straight: its length, the front of its
	// lowest tier (Y) and how many tiers it has.
	constexpr double StandLength = 8000.0;
	constexpr double StandFront = -(Outer + 800.0);
	constexpr int32 StandTiers = 8;
	constexpr double TierDepth = 250.0;
	constexpr double TierRise = 55.0;
	// The board fence along Kerepesi út (Y) from its west end to its east end
	// (X), with the gate between GateWest and GateEast.
	constexpr double FenceNorth = -12900.0;
	constexpr double FenceWest = -35000.0;
	constexpr double FenceEast = 40000.0;
	constexpr double FenceSouth = 10850.0;
	constexpr double GateWest = -6200.0;
	constexpr double GateEast = -4600.0;
	// Things nearby in the track's frame: the engine shed's middle (long
	// side east-west) and the water tower in the rail yard, and the old
	// stand on the gallop course.
	const FVector2D EngineShed(10500.0, -30100.0);
	const FVector2D WaterTower(-17300.0, -36200.0);
	const FVector2D OldStand(27500.0, -52000.0);
	// The railway to Rákosrendező crosses Kerepesi út and Aréna út on two
	// iron bridges at the east edge: their middles (X, the line's run is
	// north-south), and each road's middle under them (Y) and half its width
	// from kerb to kerb.
	constexpr double BridgeX = 66000.0;
	constexpr double BridgeRoads[][2] = { { -14320.0, 1300.0 }, { -24260.0, 820.0 } };

	struct FSign
	{
		FString Text;
		FVector At;
		double Yaw;
		double Height;
		FLinearColor Color;
	};

	namespace Colors
	{
		const FLinearColor SandTrack(0.50f, 0.40f, 0.27f);
		const FLinearColor RailWhite(0.82f, 0.80f, 0.74f);
		const FLinearColor StandWood(0.62f, 0.56f, 0.44f);
		const FLinearColor StandGreen(0.16f, 0.24f, 0.17f);
		const FLinearColor Roof(0.26f, 0.27f, 0.26f);
		const FLinearColor Brick(0.42f, 0.20f, 0.13f);
		const FLinearColor Plaster(0.72f, 0.66f, 0.54f);
		const FLinearColor Dark(0.05f, 0.05f, 0.05f);
		const FLinearColor Fence(0.30f, 0.23f, 0.15f);
		const FLinearColor Soot(0.12f, 0.11f, 0.10f);
		const FLinearColor Lettering(0.85f, 0.70f, 0.30f);
		const FLinearColor OldWood(0.36f, 0.33f, 0.29f);
	}

	/** Steps round the oval at a distance R from its middle line: calls Step(From, To) for pieces about Piece cm long. */
	template <typename FStep>
	inline void AroundOval(double R, double Piece, FStep Step)
	{
		const int32 StraightSteps = FMath::Max(1, FMath::RoundToInt(2.0 * Straight / Piece));
		const int32 TurnSteps = FMath::Max(4, FMath::RoundToInt(PI * R / Piece));
		for (int32 Side = 0; Side < 2; ++Side)
		{
			// North straight runs west to east, south straight east to west.
			const double Y = Side == 0 ? -R : R;
			const double Dir = Side == 0 ? 1.0 : -1.0;
			for (int32 I = 0; I < StraightSteps; ++I)
			{
				const double X0 = -Dir * Straight + Dir * 2.0 * Straight * I / StraightSteps;
				const double X1 = -Dir * Straight + Dir * 2.0 * Straight * (I + 1) / StraightSteps;
				Step(FVector2D(X0, Y), FVector2D(X1, Y));
			}
			// Then the turn at that end, round to the other straight.
			const double CX = Dir * Straight;
			const double Start = Side == 0 ? -HALF_PI : HALF_PI;
			for (int32 I = 0; I < TurnSteps; ++I)
			{
				const double A0 = Start + PI * I / TurnSteps;
				const double A1 = Start + PI * (I + 1) / TurnSteps;
				Step(FVector2D(CX + R * FMath::Cos(A0), R * FMath::Sin(A0)), FVector2D(CX + R * FMath::Cos(A1), R * FMath::Sin(A1)));
			}
		}
	}

	/** A plank or bar from A to B (ground plan), Size.X added to its length, at height Z. */
	inline void Bar(FBuilder& B, const FLinearColor& Color, const FVector2D& A, const FVector2D& To, double Z, double Wide, double Tall, double Extra, bool bCollide)
	{
		const FVector2D D = To - A;
		const double Heading = FMath::RadiansToDegrees(FMath::Atan2(D.Y, D.X));
		const FVector2D M = (A + To) * 0.5;
		B.Add(Cube, Color, FVector(M.X, M.Y, Z), FVector(D.Size() + Extra, Wide, Tall), 0.0, Heading, 0.0, bCollide);
	}

	inline void BuildOval(FBuilder& B)
	{
		// The sand, a little above the ground and the infield's grass.
		AroundOval(Outer - Sand * 0.5, 500.0, [&B](const FVector2D& A, const FVector2D& To)
		{
			Bar(B, Colors::SandTrack, A, To, 3.0, Sand, 6.0, 60.0, false);
		});
		// White rails on both edges: a post every few metres and a top bar.
		for (const double R : { Inner - 30.0, Outer + 30.0 })
		{
			AroundOval(R, 400.0, [&B](const FVector2D& A, const FVector2D& To)
			{
				B.Add(Cylinder, Colors::RailWhite, FVector(A.X, A.Y, 50.0), FVector(10.0, 10.0, 100.0));
				Bar(B, Colors::RailWhite, A, To, 98.0, 8.0, 12.0, 8.0, true);
				Bar(B, Colors::RailWhite, A, To, 55.0, 5.0, 8.0, 8.0, false);
			});
		}
		// The judges' box on the infield at the finish, across from the stand.
		const double JudgeY = -(Inner - 400.0);
		B.Add(Cube, Colors::Plaster, FVector(0.0, JudgeY, 250.0), FVector(300.0, 300.0, 500.0), 0.0, 0.0, 0.0, true);
		B.Add(Cube, Colors::Dark, FVector(0.0, JudgeY - 151.0, 380.0), FVector(220.0, 4.0, 110.0));
		B.Add(Cube, Colors::Roof, FVector(0.0, JudgeY, 530.0), FVector(380.0, 380.0, 25.0));
		// The finish post on the outer rail.
		B.Add(Cylinder, Colors::RailWhite, FVector(0.0, -(Outer + 30.0), 160.0), FVector(16.0, 16.0, 320.0));
		B.Add(Cube, Colors::Dark, FVector(0.0, -(Outer + 30.0), 300.0), FVector(6.0, 60.0, 60.0));
	}

	inline void BuildStand(FBuilder& B)
	{
		const double Half = StandLength * 0.5;
		const double Back = StandFront - StandTiers * TierDepth;
		// Tiers, each a step higher towards the back, with a bench edge.
		for (int32 I = 0; I < StandTiers; ++I)
		{
			const double Y = StandFront - (I + 0.5) * TierDepth;
			const double Top = (I + 1) * TierRise;
			B.Add(Cube, Colors::StandWood, FVector(0.0, Y, Top * 0.5), FVector(StandLength, TierDepth, Top), 0.0, 0.0, 0.0, true);
			B.Add(Cube, Colors::StandGreen, FVector(0.0, Y + TierDepth * 0.5 - 20.0, Top + 2.0), FVector(StandLength, 40.0, 4.0));
		}
		// Back and end walls, the roof on columns, a low front rail.
		const double RoofZ = StandTiers * TierRise + 380.0;
		B.Add(Cube, Colors::StandGreen, FVector(0.0, Back - 15.0, RoofZ * 0.5), FVector(StandLength, 30.0, RoofZ), 0.0, 0.0, 0.0, true);
		for (const double X : { -Half, Half })
		{
			B.Add(Cube, Colors::StandGreen, FVector(X, (StandFront + Back) * 0.5, RoofZ * 0.5), FVector(30.0, StandFront - Back, RoofZ), 0.0, 0.0, 0.0, true);
		}
		for (double X = -Half; X <= Half + 1.0; X += 1000.0)
		{
			B.Add(Cylinder, Colors::StandGreen, FVector(X, StandFront + 60.0, RoofZ * 0.5), FVector(24.0, 24.0, RoofZ), 0.0, 0.0, 0.0, true);
		}
		B.Add(Cube, Colors::Roof, FVector(0.0, (StandFront + Back) * 0.5 + 100.0, RoofZ + 60.0), FVector(StandLength + 200.0, StandFront - Back + 400.0, 20.0), 0.0, 0.0, -5.0);
		B.Add(Cube, Colors::StandGreen, FVector(0.0, StandFront + 290.0, RoofZ + 20.0), FVector(StandLength + 200.0, 10.0, 70.0));
		B.Add(Cube, Colors::StandGreen, FVector(0.0, StandFront + 10.0, 50.0), FVector(StandLength, 12.0, 100.0), 0.0, 0.0, 0.0, true);
		// A middle box for the gentry, with a flag pole on the roof.
		B.Add(Cube, Colors::Plaster, FVector(0.0, StandFront - 100.0, TierRise * 4.0 + 60.0), FVector(900.0, 30.0, 120.0));
		B.Add(Cylinder, Colors::RailWhite, FVector(0.0, (StandFront + Back) * 0.5, RoofZ + 350.0), FVector(8.0, 8.0, 600.0));
		B.Add(Cube, FLinearColor(0.45f, 0.06f, 0.05f), FVector(70.0, (StandFront + Back) * 0.5, RoofZ + 580.0), FVector(130.0, 2.0, 80.0));

		// The betting windows east of the stand: a plastered hut with a row
		// of hatches facing the track.
		const double HutX = Half + 1300.0;
		const double HutY = StandFront - 500.0;
		B.Add(Cube, Colors::Plaster, FVector(HutX, HutY, 200.0), FVector(1600.0, 900.0, 400.0), 0.0, 0.0, 0.0, true);
		B.Add(Cube, Colors::Roof, FVector(HutX, HutY + 50.0, 420.0), FVector(1700.0, 1100.0, 30.0));
		for (int32 I = 0; I < 5; ++I)
		{
			B.Add(Cube, Colors::Dark, FVector(HutX - 600.0 + I * 300.0, HutY + 452.0, 150.0), FVector(120.0, 4.0, 90.0));
			B.Add(Cube, Colors::StandWood, FVector(HutX - 600.0 + I * 300.0, HutY + 480.0, 100.0), FVector(150.0, 50.0, 6.0));
		}
	}

	inline void BuildFence(FBuilder& B)
	{
		// Boards along Kerepesi út with a gap for the gate, and the west and
		// east sides down to the cemetery wall.
		auto Run = [&B](const FVector2D& A, const FVector2D& To)
		{
			const int32 Pieces = FMath::Max(1, FMath::RoundToInt((To - A).Size() / 600.0));
			for (int32 I = 0; I < Pieces; ++I)
			{
				const FVector2D P0 = A + (To - A) * (double(I) / Pieces);
				const FVector2D P1 = A + (To - A) * (double(I + 1) / Pieces);
				Bar(B, Colors::Fence, P0, P1, 110.0, 6.0, 220.0, 0.0, true);
				B.Add(Cube, Colors::Fence, FVector(P0.X, P0.Y, 120.0), FVector(14.0, 14.0, 240.0));
			}
		};
		const double StandWest = -StandLength * 0.5;
		Run(FVector2D(FenceWest, FenceNorth), FVector2D(GateWest, FenceNorth));
		Run(FVector2D(GateEast, FenceNorth), FVector2D(StandWest, FenceNorth));
		Run(FVector2D(StandLength * 0.5, FenceNorth), FVector2D(FenceEast, FenceNorth));
		Run(FVector2D(FenceWest, FenceNorth), FVector2D(FenceWest, FenceSouth));
		Run(FVector2D(FenceEast, FenceNorth), FVector2D(FenceEast, FenceSouth));
		// The gate: two brick pillars and an iron arch with the name board.
		for (const double X : { GateWest, GateEast })
		{
			B.Add(Cube, Colors::Brick, FVector(X, FenceNorth, 200.0), FVector(70.0, 70.0, 400.0), 0.0, 0.0, 0.0, true);
			B.Add(Cube, Colors::Plaster, FVector(X, FenceNorth, 410.0), FVector(90.0, 90.0, 20.0));
		}
		B.Add(Cube, MarketModel::Colors::Iron, FVector((GateWest + GateEast) * 0.5, FenceNorth, 470.0), FVector(GateEast - GateWest, 8.0, 10.0));
		B.Add(Cube, Colors::StandGreen, FVector((GateWest + GateEast) * 0.5, FenceNorth, 520.0), FVector(1100.0, 6.0, 90.0));
	}

	inline void BuildStables(FBuilder& B)
	{
		// Four long brick stables by the east fence, gable ends to the yard,
		// a row of stable doors down each side.
		for (int32 I = 0; I < 4; ++I)
		{
			const double X = 31000.0 + I * 2400.0;
			const double Y0 = -10500.0;
			const double Y1 = -3500.0;
			const double MidY = (Y0 + Y1) * 0.5;
			const double Long = Y1 - Y0;
			B.Add(Cube, Colors::Brick, FVector(X, MidY, 200.0), FVector(1100.0, Long, 400.0), 0.0, 0.0, 0.0, true);
			for (const double Side : { -1.0, 1.0 })
			{
				B.Add(Cube, Colors::Roof, FVector(X + Side * 300.0, MidY, 520.0), FVector(700.0, Long + 100.0, 16.0), -Side * 22.0, 0.0, 0.0);
			}
			for (double Y = Y0 + 400.0; Y < Y1 - 200.0; Y += 500.0)
			{
				for (const double Side : { -1.0, 1.0 })
				{
					B.Add(Cube, Colors::StandGreen, FVector(X + Side * 552.0, Y, 130.0), FVector(4.0, 160.0, 250.0));
				}
			}
		}
		// Hay, a water trough and a cart in the yard between them.
		B.Add(Cube, FLinearColor(0.58f, 0.48f, 0.22f), FVector(32200.0, -2600.0, 70.0), FVector(300.0, 200.0, 140.0), 0.0, 10.0, 0.0, true);
		B.Add(Cube, MarketModel::Colors::Iron, FVector(34600.0, -2700.0, 35.0), FVector(250.0, 70.0, 70.0), 0.0, 0.0, 0.0, true);
	}

	inline void BuildRailYard(FBuilder& B)
	{
		// The engine shed: sooty brick, tall arched doors on its east end, a
		// lantern roof with smoke vents.
		const FVector2D S = EngineShed;
		B.Add(Cube, Colors::Brick, FVector(S.X, S.Y, 450.0), FVector(6800.0, 2000.0, 900.0), 0.0, 0.0, 0.0, true);
		B.Add(Cube, Colors::Soot, FVector(S.X, S.Y, 960.0), FVector(6900.0, 2100.0, 120.0));
		B.Add(Cube, Colors::Roof, FVector(S.X, S.Y, 1100.0), FVector(6600.0, 700.0, 200.0));
		for (int32 I = 0; I < 3; ++I)
		{
			B.Add(Cube, Colors::Soot, FVector(S.X + 3402.0, S.Y - 600.0 + I * 600.0, 330.0), FVector(4.0, 450.0, 660.0));
			B.Add(Cube, Colors::Soot, FVector(S.X - 2000.0 + I * 2000.0, S.Y, 1260.0), FVector(80.0, 80.0, 160.0));
		}
		for (double X = S.X - 3000.0; X <= S.X + 3000.0; X += 750.0)
		{
			for (const double Side : { -1.0, 1.0 })
			{
				B.Add(Cube, Colors::Dark, FVector(X, S.Y + Side * 1002.0, 550.0), FVector(260.0, 4.0, 380.0));
			}
		}
		// The water tower: a brick shaft with an iron tank on top.
		const FVector2D T = WaterTower;
		B.Add(Cylinder, Colors::Brick, FVector(T.X, T.Y, 600.0), FVector(500.0, 500.0, 1200.0), 0.0, 0.0, 0.0, true);
		B.Add(Cylinder, MarketModel::Colors::Iron, FVector(T.X, T.Y, 1450.0), FVector(900.0, 900.0, 500.0));
		B.Add(MarketModel::Cone, Colors::Roof, FVector(T.X, T.Y, 1820.0), FVector(980.0, 980.0, 250.0));
	}

	inline void BuildBridges(FBuilder& B)
	{
		// Each: stone abutments either side of the road, a short earth bank
		// behind each, and an iron girder deck with the track on top.
		const FLinearColor Stone(0.50f, 0.47f, 0.42f);
		const FLinearColor Bank(0.30f, 0.27f, 0.20f);
		for (const auto& Under : BridgeRoads)
		{
			const double Road = Under[0];
			const double Span = 2.0 * (Under[1] + 250.0);
			for (const double Side : { -1.0, 1.0 })
			{
				const double Face = Road + Side * Span * 0.5;
				B.Add(Cube, Stone, FVector(BridgeX, Face + Side * 300.0, 330.0), FVector(1400.0, 600.0, 660.0), 0.0, 0.0, 0.0, true);
				B.Add(Cube, Bank, FVector(BridgeX, Face + Side * 2100.0, 280.0), FVector(1400.0, 3000.0, 560.0), 0.0, 0.0, 0.0, true);
				B.Add(Cube, Bank, FVector(BridgeX, Face + Side * 4300.0, 140.0), FVector(1400.0, 1400.0, 280.0), 0.0, 0.0, 0.0, true);
			}
			B.Add(Cube, MarketModel::Colors::Iron, FVector(BridgeX, Road, 700.0), FVector(1100.0, Span + 200.0, 80.0));
			for (const double Edge : { -1.0, 1.0 })
			{
				B.Add(Cube, MarketModel::Colors::Iron, FVector(BridgeX + Edge * 560.0, Road, 780.0), FVector(30.0, Span + 200.0, 240.0));
			}
			B.Add(Cube, Colors::OldWood, FVector(BridgeX, Road, 750.0), FVector(260.0, Span + 300.0, 20.0));
		}
	}

	inline void BuildOldStand(FBuilder& B)
	{
		// The gallop course's old wooden stand, left to rot since 1925: bare
		// tiers, half its roof gone, posts leaning.
		const FVector2D O = OldStand;
		for (int32 I = 0; I < 6; ++I)
		{
			const double Top = (I + 1) * 60.0;
			B.Add(Cube, Colors::OldWood, FVector(O.X, O.Y - I * 250.0, Top * 0.5), FVector(5000.0, 250.0, Top), 0.0, 0.0, 0.0, true);
		}
		for (int32 I = 0; I < 6; ++I)
		{
			const double X = O.X - 2500.0 + I * 1000.0;
			B.Add(Cylinder, Colors::OldWood, FVector(X, O.Y + 100.0, 400.0), FVector(22.0, 22.0, 800.0), 0.0, 0.0, (I % 2 ? 4.0 : -3.0), true);
		}
		B.Add(Cube, Colors::Roof, FVector(O.X - 1200.0, O.Y - 600.0, 820.0), FVector(2500.0, 1700.0, 15.0), 0.0, 3.0, -7.0);
		B.Add(Cube, Colors::OldWood, FVector(O.X + 1800.0, O.Y - 200.0, 30.0), FVector(900.0, 400.0, 20.0), 0.0, 25.0, 6.0);
	}

	/** Everything, in the track's frame. */
	inline void Build(TArray<FShape>& Out)
	{
		FBuilder B{ Out, FVector::ZeroVector, 0.0 };
		BuildOval(B);
		BuildStand(B);
		BuildFence(B);
		BuildStables(B);
		BuildRailYard(B);
		BuildOldStand(B);
		BuildBridges(B);
	}

	inline void Signs(TArray<FSign>& Out)
	{
		const double RoofZ = StandTiers * TierRise + 380.0;
		// On the stand's roof edge, facing the track, and over the gate.
		Out.Add({ TEXT("KEREPESI \u00daTI \u00dcGET\u0150P\u00c1LYA"), FVector(0.0, StandFront + 306.0, RoofZ + 20.0), 90.0, 50.0, Colors::Lettering });
		Out.Add({ TEXT("\u00dcGET\u0150P\u00c1LYA"), FVector((GateWest + GateEast) * 0.5, FenceNorth - 4.0, 520.0), -90.0, 55.0, Colors::Lettering });
		Out.Add({ TEXT("TOTALIZAT\u0150R"), FVector(StandLength * 0.5 + 1300.0, StandFront - 500.0 + 452.0, 330.0), 90.0, 45.0, Colors::Dark });
	}
}
