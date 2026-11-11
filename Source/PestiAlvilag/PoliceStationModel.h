// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BozsiHouseModel.h"
#include "MarketModel.h"

/**
 * The looks of the police's own places, as basic shapes (MarketModel::FShape):
 * - the VIII. district police station's front door on Víg utca: a stone
 *   doorway, two oak door leaves, granite steps, a dark green board with
 *   gilt letters, the coat of arms, a flag on a slanting pole and two wall
 *   lanterns (where the player is let go after an arrest);
 * - the wooden road barrier of a road block: a trestle with a red and white
 *   striped plank, a small "\u00c1LLJ!" board hung under it and a red oil lamp.
 *
 * Header-only. The station's frame: the house wall along Y at X = 0, +X out
 * across the pavement to the street, Z up from the pavement, the door in
 * the middle. A barrier's frame: the plank along Y, +X towards the traffic
 * it stops, Z up from the road. A barrier has no collision of its own:
 * APoliceChase knocks it over when a car touches it.
 */
namespace PoliceStationModel
{
	using MarketModel::FShape;
	using BozsiHouseModel::FSign;
	using BozsiHouseModel::FLamp;
	using BozsiHouseModel::Add;
	using BozsiHouseModel::Span;

	namespace Colors
	{
		const FLinearColor Stone(0.46f, 0.43f, 0.38f);
		const FLinearColor StoneDark(0.33f, 0.31f, 0.28f);
		const FLinearColor Granite(0.30f, 0.30f, 0.30f);
		const FLinearColor Oak(0.14f, 0.08f, 0.04f);
		const FLinearColor OakDark(0.08f, 0.045f, 0.025f);
		const FLinearColor Brass(0.62f, 0.45f, 0.16f);
		const FLinearColor Board(0.04f, 0.12f, 0.08f);
		const FLinearColor Gilt(0.78f, 0.58f, 0.20f);
		const FLinearColor Red(0.55f, 0.04f, 0.04f);
		const FLinearColor White(0.82f, 0.80f, 0.74f);
		const FLinearColor Green(0.04f, 0.28f, 0.10f);
		const FLinearColor Iron(0.05f, 0.05f, 0.055f);
		const FLinearColor LampGlass(0.95f, 0.80f, 0.50f);
		const FLinearColor Wood(0.30f, 0.20f, 0.11f);
		const FLinearColor RedGlass(0.85f, 0.08f, 0.05f);
		const FLinearColor Lettering(0.06f, 0.05f, 0.04f);
	}

	/** Half the doorway's width, and where the two sentries stand (in the station's frame). */
	constexpr double DoorHalf = 125.0;
	const FVector SentryLeft(55.0, -205.0, 0.0);
	const FVector SentryRight(55.0, 205.0, 0.0);
	/** Where the player comes out after a night in the cells: on the pavement in front of the steps. */
	const FVector LetOutAt(190.0, 0.0, 0.0);

	/** The station's front door and all round it. */
	inline void BuildStationFront(TArray<FShape>& Out)
	{
		using namespace Colors;
		const double Pier = 28.0;
		const double Top = 330.0;
		// The stone doorway: two piers, a lintel with a cornice, a keystone.
		for (const double Side : { -1.0, 1.0 })
		{
			Span(Out, Stone, FVector(0, Side * DoorHalf, 0), FVector(16, Side * (DoorHalf + Pier), Top));
			Span(Out, StoneDark, FVector(0, Side * (DoorHalf + Pier - 4), 0), FVector(20, Side * (DoorHalf + Pier + 4), 40), false);
		}
		Span(Out, Stone, FVector(0, -DoorHalf - Pier - 10, Top), FVector(18, DoorHalf + Pier + 10, Top + 30));
		Span(Out, StoneDark, FVector(0, -DoorHalf - Pier - 16, Top + 30), FVector(26, DoorHalf + Pier + 16, Top + 40), false);
		Span(Out, StoneDark, FVector(0, -18, Top - 14), FVector(22, 18, Top + 34), false);

		// Two oak leaves, panelled, with brass handles; a fanlight over them.
		for (const double Side : { -1.0, 1.0 })
		{
			Span(Out, Oak, FVector(0, Side * 2, 0), FVector(6, Side * DoorHalf, 255));
			for (const double Z : { 30.0, 140.0 })
			{
				Span(Out, OakDark, FVector(6, Side * 16, Z), FVector(9, Side * (DoorHalf - 16), Z + 90), false);
			}
			Add(Out, MarketModel::Sphere, Brass, FVector(11, Side * 14, 120), FVector(7, 7, 7), 0.0, false);
		}
		Span(Out, Iron, FVector(0, -DoorHalf, 255), FVector(4, DoorHalf, Top - 14), false);
		for (int32 i = -2; i <= 2; ++i)
		{
			Add(Out, MarketModel::Cube, Brass, FVector(5, i * 42.0, (255 + Top - 14) * 0.5), FVector(2, 3, Top - 14 - 255), 0.0, false);
		}

		// Granite steps.
		Span(Out, Granite, FVector(0, -DoorHalf - 50, 0), FVector(70, DoorHalf + 50, 13));
		Span(Out, Granite, FVector(0, -DoorHalf - 30, 13), FVector(38, DoorHalf + 30, 26));

		// The name board over the door, in a gilt frame.
		const double BoardLow = Top + 52;
		const double BoardHigh = BoardLow + 74;
		const double BoardHalf = 290;
		Span(Out, Board, FVector(0, -BoardHalf, BoardLow), FVector(7, BoardHalf, BoardHigh), false);
		Span(Out, Gilt, FVector(0, -BoardHalf - 4, BoardLow - 4), FVector(8, BoardHalf + 4, BoardLow), false);
		Span(Out, Gilt, FVector(0, -BoardHalf - 4, BoardHigh), FVector(8, BoardHalf + 4, BoardHigh + 4), false);
		for (const double Side : { -1.0, 1.0 })
		{
			Span(Out, Gilt, FVector(0, Side * BoardHalf, BoardLow), FVector(8, Side * (BoardHalf + 4), BoardHigh), false);
		}

		// The coat of arms over the board: a shield, red and white bars on the
		// left as one looks at it (+Y), the green hills and white cross on red
		// on the right, a crown.
		const double ShieldLow = BoardHigh + 16;
		Span(Out, Gilt, FVector(0, -34, ShieldLow - 3), FVector(5, 34, ShieldLow + 73), false);
		for (int32 i = 0; i < 6; ++i)
		{
			Span(Out, i % 2 == 0 ? Red : White, FVector(5, 1, ShieldLow + i * 11.5), FVector(7, 30, ShieldLow + (i + 1) * 11.5), false);
		}
		Span(Out, Red, FVector(5, -30, ShieldLow), FVector(7, -1, ShieldLow + 69), false);
		Span(Out, Green, FVector(6, -27, ShieldLow), FVector(8, -4, ShieldLow + 16), false);
		Span(Out, White, FVector(6, -17, ShieldLow + 16), FVector(8, -14, ShieldLow + 52), false);
		Span(Out, White, FVector(6, -24, ShieldLow + 38), FVector(8, -7, ShieldLow + 42), false);
		Add(Out, MarketModel::Sphere, Gilt, FVector(4, 0, ShieldLow + 84), FVector(16, 30, 20), 0.0, false);
		Add(Out, MarketModel::Cube, Gilt, FVector(4, 0, ShieldLow + 98), FVector(3, 3, 12), 0.0, false);

		// The flag on a pole slanting out from the wall, right of the door:
		// red, white and green bands hanging under the pole.
		const double PoleUp = 35.0;
		const FVector PoleFoot(0, BoardHalf + 70, BoardLow + 20);
		const FVector Along(FMath::Cos(FMath::DegreesToRadians(PoleUp)), 0, FMath::Sin(FMath::DegreesToRadians(PoleUp)));
		const FVector Down(-Along.Z, 0, Along.X);
		const double PoleLength = 260.0;
		Add(Out, MarketModel::Cube, Iron, PoleFoot + Along * (PoleLength * 0.5), FVector(PoleLength, 6, 6), 0.0, false, PoleUp);
		Add(Out, MarketModel::Sphere, Gilt, PoleFoot + Along * (PoleLength + 4), FVector(11, 11, 11), 0.0, false);
		Span(Out, Iron, PoleFoot + FVector(0, -12, -12), PoleFoot + FVector(6, 12, 12), false);
		const FLinearColor Bands[] = { Red, White, Green };
		for (int32 i = 0; i < 3; ++i)
		{
			const FVector Middle = PoleFoot + Along * (PoleLength * 0.55) - Down * (12.0 + 25.0 * i);
			Add(Out, MarketModel::Cube, Bands[i], Middle, FVector(150, 2, 25), 0.0, false, PoleUp);
		}

		// Two wall lanterns, one each side of the doorway.
		for (const double Side : { -1.0, 1.0 })
		{
			const double Y = Side * (DoorHalf + Pier + 48);
			Span(Out, Iron, FVector(0, Y - 3, 300), FVector(40, Y + 3, 305), false);
			Span(Out, Iron, FVector(0, Y - 8, 285), FVector(4, Y + 8, 315), false);
			Span(Out, Iron, FVector(28, Y - 14, 296), FVector(52, Y + 14, 300), false);
			Span(Out, LampGlass, FVector(30, Y - 11, 262), FVector(50, Y + 11, 296), false);
			Add(Out, MarketModel::Cone, Iron, FVector(40, Y, 303), FVector(32, 32, 14), 0.0, false);
		}
	}

	/** The board's lettering, gilt on green, and the brass plate by the door. */
	inline void StationSigns(TArray<FSign>& Out)
	{
		using namespace Colors;
		const double BoardLow = 330.0 + 52.0;
		Out.Add({ TEXT("M. KIR. \u00c1LLAMREND\u0150RS\u00c9G"), FVector(8.5, 0.0, BoardLow + 52.0), 0.0, 25.0, Gilt });
		Out.Add({ TEXT("VIII. KER\u00dcLETI KAPIT\u00c1NYS\u00c1G"), FVector(8.5, 0.0, BoardLow + 22.0), 0.0, 21.0, Gilt });
	}

	inline void StationLamps(TArray<FLamp>& Out)
	{
		for (const double Side : { -1.0, 1.0 })
		{
			Out.Add({ FVector(40.0, Side * (DoorHalf + 28.0 + 48.0), 279.0), 450.f });
		}
	}

	/** Half a barrier's length along its plank (cm), and its height. */
	constexpr double BarrierHalf = 125.0;
	constexpr double BarrierTop = 108.0;

	/** One wooden road barrier, in its own frame (see above). */
	inline void BuildBarrier(TArray<FShape>& Out)
	{
		using namespace Colors;
		// Two trestles: each two legs splayed along X, a foot board, a cross piece.
		for (const double Side : { -1.0, 1.0 })
		{
			const double Y = Side * (BarrierHalf - 22.0);
			for (const double Lean : { -1.0, 1.0 })
			{
				Add(Out, MarketModel::Cube, Wood, FVector(Lean * 16.0, Y, 50.0), FVector(6, 7, 104), 0.0, false, Lean * 17.0);
			}
			Span(Out, Wood, FVector(-38, Y - 4, 0), FVector(38, Y + 4, 6), false);
			Span(Out, Wood, FVector(-14, Y - 4, 60), FVector(14, Y + 4, 66), false);
		}
		// The striped plank on top, both faces the same.
		const int32 Stripes = 8;
		const double Step = 2.0 * BarrierHalf / Stripes;
		for (int32 i = 0; i < Stripes; ++i)
		{
			const double Y0 = -BarrierHalf + i * Step;
			Span(Out, i % 2 == 0 ? Red : White, FVector(-3, Y0, BarrierTop - 22), FVector(3, Y0 + Step, BarrierTop), false);
		}
		// The small board hung under the plank, white with black letters.
		Span(Out, White, FVector(-1.5, -62, 62), FVector(1.5, 62, 84), false);
		Span(Out, Iron, FVector(-1, -30, 84), FVector(1, -28, BarrierTop - 22), false);
		Span(Out, Iron, FVector(-1, 28, 84), FVector(1, 30, BarrierTop - 22), false);
		// A red oil lamp at one end of the plank.
		const double LampY = BarrierHalf - 14.0;
		Span(Out, Iron, FVector(-6, LampY - 6, BarrierTop), FVector(6, LampY + 6, BarrierTop + 3), false);
		Span(Out, RedGlass, FVector(-5, LampY - 5, BarrierTop + 3), FVector(5, LampY + 5, BarrierTop + 17), false);
		Add(Out, MarketModel::Cone, Iron, FVector(0, LampY, BarrierTop + 22), FVector(14, 14, 9), 0.0, false);
		Span(Out, Iron, FVector(-1, LampY - 1, BarrierTop + 26), FVector(1, LampY + 1, BarrierTop + 34), false);
	}

	/** "\u00c1LLJ! REND\u0150RS\u00c9G" (Stop! Police) on the board, both faces. */
	inline void BarrierSigns(TArray<FSign>& Out)
	{
		Out.Add({ TEXT("\u00c1LLJ!  REND\u0150RS\u00c9G"), FVector(2.0, 0.0, 73.0), 0.0, 12.0, Colors::Lettering });
		Out.Add({ TEXT("\u00c1LLJ!  REND\u0150RS\u00c9G"), FVector(-2.0, 0.0, 73.0), 180.0, 12.0, Colors::Lettering });
	}

	/** Where the red lamp's glow sits. */
	inline FVector BarrierLampAt()
	{
		return FVector(0.0, BarrierHalf - 14.0, BarrierTop + 10.0);
	}
}
