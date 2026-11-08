// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BozsiHouseModel.h"
#include "MarketModel.h"

/**
 * Laci's night races: the gates the cars must pass through, set up by his
 * pals across a street after dark.
 * - a checkpoint: two iron poles either side of the street with a rope
 *   across hung with glowing paper lanterns, a paraffin flare burning on
 *   the cobbles at each foot, and a board on one pole chalked with the
 *   gate's number;
 * - the finish: the same, with a chequered cloth across instead of lanterns.
 *
 * Header-only, as basic shapes (MarketModel::FShape). The gate's frame: the
 * middle of the street at the origin, the cars coming along +X, the poles
 * at Y = -Half and +Half, Z up from the road.
 */
namespace RaceModel
{
	using MarketModel::FShape;
	using BozsiHouseModel::FSign;
	using BozsiHouseModel::FLamp;
	using BozsiHouseModel::Add;
	using BozsiHouseModel::Span;

	namespace Colors
	{
		const FLinearColor Iron(0.06f, 0.06f, 0.065f);
		const FLinearColor Rope(0.42f, 0.34f, 0.22f);
		const FLinearColor Lantern(1.00f, 0.62f, 0.22f);
		const FLinearColor LanternRed(0.95f, 0.22f, 0.10f);
		const FLinearColor Flame(1.00f, 0.70f, 0.25f);
		const FLinearColor Tin(0.30f, 0.30f, 0.30f);
		const FLinearColor Board(0.06f, 0.06f, 0.06f);
		const FLinearColor Chalk(0.90f, 0.88f, 0.82f);
		const FLinearColor White(0.88f, 0.86f, 0.80f);
		const FLinearColor Black(0.03f, 0.03f, 0.03f);
		const FLinearColor Stone(0.36f, 0.34f, 0.31f);
	}

	constexpr double PoleTop = 420.0;
	constexpr double RopeZ = 400.0;
	constexpr double Sag = 50.0;

	/** The rope's height a fraction F (0..1) of the way across. */
	inline double RopeAt(double F)
	{
		return RopeZ - Sag * 4.0 * F * (1.0 - F);
	}

	/** A pole with its stone foot and, at the foot, a paraffin flare in a tin. */
	inline void Pole(TArray<FShape>& Out, TArray<FLamp>& Lamps, double Y)
	{
		using namespace Colors;
		Add(Out, MarketModel::Cylinder, Stone, FVector(0, Y, 15), FVector(50, 50, 30));
		Add(Out, MarketModel::Cylinder, Iron, FVector(0, Y, PoleTop * 0.5), FVector(10, 10, PoleTop));
		Add(Out, MarketModel::Sphere, Iron, FVector(0, Y, PoleTop + 4.0), FVector(16, 16, 16), 0.0, false);
		const double Side = Y < 0.0 ? -1.0 : 1.0;
		const FVector Flare(60.0, Y - Side * 40.0, 0.0);
		Add(Out, MarketModel::Cylinder, Tin, Flare + FVector(0, 0, 14), FVector(26, 26, 28), 0.0, false);
		Add(Out, MarketModel::Cone, Flame, Flare + FVector(0, 0, 50), FVector(22, 22, 46), 0.0, false);
		Lamps.Add({ Flare + FVector(0, 0, 60), 700.f });
	}

	/** One gate: poles, the lantern rope across, the chalked number board on the left pole. */
	inline void Checkpoint(TArray<FShape>& Out, TArray<FLamp>& Lamps, double Half)
	{
		using namespace Colors;
		Pole(Out, Lamps, -Half);
		Pole(Out, Lamps, Half);
		const int32 Bits = 16;
		for (int32 i = 0; i < Bits; ++i)
		{
			const double F0 = static_cast<double>(i) / Bits;
			const double F1 = static_cast<double>(i + 1) / Bits;
			const FVector A(0, FMath::Lerp(-Half, Half, F0), RopeAt(F0));
			const FVector B(0, FMath::Lerp(-Half, Half, F1), RopeAt(F1));
			const double Roll = FMath::RadiansToDegrees(FMath::Atan2(B.Z - A.Z, B.Y - A.Y));
			Add(Out, MarketModel::Cube, Rope, (A + B) * 0.5, FVector(2, (B - A).Size() + 1.0, 2), 0.0, false, 0.0, Roll);
		}
		// The paper lanterns hanging from it, red and orange in turn.
		const int32 Count = 7;
		for (int32 i = 1; i <= Count; ++i)
		{
			const double F = static_cast<double>(i) / (Count + 1);
			const FVector At(0, FMath::Lerp(-Half, Half, F), RopeAt(F) - 34.0);
			Add(Out, MarketModel::Cylinder, Black, At + FVector(0, 0, 24), FVector(1, 1, 20), 0.0, false);
			Add(Out, MarketModel::Cylinder, Black, At + FVector(0, 0, 15), FVector(16, 16, 3), 0.0, false);
			Add(Out, MarketModel::Sphere, i % 2 ? Lantern : LanternRed, At, FVector(30, 30, 34), 0.0, false);
			Add(Out, MarketModel::Cylinder, Black, At - FVector(0, 0, 16), FVector(14, 14, 3), 0.0, false);
			if (i % 2 == 0)
			{
				Lamps.Add({ At, 600.f });
			}
		}
		// The board on the left pole, facing the cars.
		Span(Out, Board, FVector(-8, -Half + 6, 200), FVector(-4, -Half + 66, 260), false);
	}

	/** The finish: the same gate, with a chequered cloth across under the rope instead of lanterns. */
	inline void Finish(TArray<FShape>& Out, TArray<FLamp>& Lamps, double Half)
	{
		using namespace Colors;
		Pole(Out, Lamps, -Half);
		Pole(Out, Lamps, Half);
		const int32 Squares = 20;
		for (int32 i = 0; i < Squares; ++i)
		{
			const double F0 = static_cast<double>(i) / Squares;
			const double F1 = static_cast<double>(i + 1) / Squares;
			const double Y0 = FMath::Lerp(-Half, Half, F0);
			const double Y1 = FMath::Lerp(-Half, Half, F1);
			const double Z = RopeAt((F0 + F1) * 0.5);
			for (int32 Row = 0; Row < 2; ++Row)
			{
				Span(Out, (i + Row) % 2 ? White : Black, FVector(-1, Y0, Z - 30.0 * (Row + 1)), FVector(1, Y1, Z - 30.0 * Row), false);
			}
		}
		Span(Out, Board, FVector(-8, -Half + 6, 200), FVector(-4, -Half + 66, 260), false);
	}

	/** The number chalked on a checkpoint's board ("1", "2", ...); the finish's says CÉL. */
	inline void Signs(TArray<FSign>& Out, double Half, int32 Number)
	{
		const FString Text = Number > 0 ? FString::FromInt(Number) : FString(TEXT("C\u00C9L"));
		Out.Add({ Text, FVector(-9, -Half + 36, 230), 180.0, 22.0, Colors::Chalk });
	}
}
