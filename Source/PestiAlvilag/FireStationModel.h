// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BozsiHouseModel.h"
#include "MarketModel.h"

/**
 * The looks of the VIII. district fire station on Dologház utca, as basic
 * shapes (MarketModel::FShape): a stone ground floor with three arched
 * engine doors painted ox-blood red (glazed at the top), the brigade's name
 * over them, its brass badge (a helmet on crossed axes), a red street fire
 * alarm box, the alarm bell on its bracket, red lanterns, a granite apron
 * on the pavement, and the tall brick drill tower rising over the roofs
 * with its lookout and flag.
 *
 * Header-only. The frame: the house wall along Y at X = 0, +X out across
 * the pavement to the street, Z up from the pavement, the middle door at
 * Y = 0. The fire engine waits parked on the apron (ParkAt, facing +Y).
 */
namespace FireStationModel
{
	using MarketModel::FShape;
	using BozsiHouseModel::FSign;
	using BozsiHouseModel::FLamp;
	using BozsiHouseModel::Add;
	using BozsiHouseModel::Span;

	/** Where the station stands on the map (cm): the middle of its front, and the way out to the street. */
	const FVector2D CityAt(84112.0, 6562.0);
	constexpr double CityFacing = -110.46;

	namespace Colors
	{
		const FLinearColor Stone(0.52f, 0.46f, 0.36f);
		const FLinearColor StoneDark(0.38f, 0.34f, 0.27f);
		const FLinearColor Granite(0.30f, 0.30f, 0.30f);
		const FLinearColor DoorRed(0.30f, 0.03f, 0.02f);
		const FLinearColor DoorDark(0.17f, 0.02f, 0.015f);
		const FLinearColor Glass(0.05f, 0.06f, 0.07f);
		const FLinearColor Mullion(0.78f, 0.75f, 0.66f);
		const FLinearColor Band(0.80f, 0.74f, 0.60f);
		const FLinearColor Letters(0.10f, 0.06f, 0.04f);
		const FLinearColor Brass(0.66f, 0.49f, 0.17f);
		const FLinearColor Iron(0.05f, 0.05f, 0.055f);
		const FLinearColor AlarmRed(0.55f, 0.03f, 0.02f);
		const FLinearColor White(0.85f, 0.83f, 0.76f);
		const FLinearColor Green(0.04f, 0.28f, 0.10f);
		const FLinearColor Brick(0.36f, 0.13f, 0.08f);
		const FLinearColor BrickDark(0.26f, 0.09f, 0.06f);
		const FLinearColor Roof(0.12f, 0.20f, 0.17f);
		const FLinearColor LampRed(0.95f, 0.12f, 0.06f);
		const FLinearColor Wood(0.30f, 0.20f, 0.11f);
	}

	/** The three engine doors: their middles along Y, half a door's width and its height to the arch's foot. */
	constexpr int32 NumDoors = 3;
	constexpr double DoorStep = 460.0;
	constexpr double DoorHalf = 180.0;
	constexpr double DoorTop = 380.0;
	/** Half the dressed stone front's length, and its height (the first floor's sill). */
	constexpr double FrontHalf = 1050.0;
	constexpr double FrontTop = 650.0;
	/** The drill tower: its middle (behind the front wall), half its width, the lookout's floor and the roof's foot. */
	const FVector TowerAt(-520.0, 1380.0, 0.0);
	constexpr double TowerHalf = 230.0;
	constexpr double LookoutFloor = 2750.0;
	constexpr double LookoutTop = 3130.0;

	/** Where the engine waits: its middle on the ground, and its heading (yaw in the frame). */
	const FVector ParkAt(200.0, -60.0, 0.0);
	constexpr double ParkYaw = 90.0;
	/** Two firemen on duty: one polishing the radiator's brass, one by the bell having a smoke. */
	const FVector PolisherAt(200.0, 300.0, 0.0);
	constexpr double PolisherYaw = -90.0;
	const FVector SmokerAt(40.0, 820.0, 0.0);
	constexpr double SmokerYaw = 20.0;

	inline double DoorY(int32 Index) { return (Index - (NumDoors - 1) * 0.5) * DoorStep; }

	/** One stone of an arch over a door: a block set on the ring at angle A (0 = right foot, 180 = left foot). */
	inline void ArchStone(TArray<FShape>& Out, const FLinearColor& Color, double MiddleY, double FootZ, double Radius, double A, double Depth, double Thick, double Length)
	{
		const double R = FMath::DegreesToRadians(A);
		const FVector At(Depth * 0.5, MiddleY + Radius * FMath::Cos(R), FootZ + Radius * FMath::Sin(R) * 0.55);
		Add(Out, MarketModel::Cube, Color, At, FVector(Depth, Length, Thick), 0.0, false, 0.0, -(A - 90.0) * 0.7);
	}

	/** The station's front and its tower. */
	inline void BuildFront(TArray<FShape>& Out)
	{
		using namespace Colors;
		// The stone ground floor over the plain house front, in courses, and its cornice.
		Span(Out, Stone, FVector(0, -FrontHalf, 0), FVector(12, FrontHalf, FrontTop), false);
		for (double Z = 60.0; Z < FrontTop - 20.0; Z += 60.0)
		{
			Span(Out, StoneDark, FVector(12, -FrontHalf, Z - 2.0), FVector(13, FrontHalf, Z + 1.0), false);
		}
		Span(Out, StoneDark, FVector(0, -FrontHalf - 6, 0), FVector(18, FrontHalf + 6, 45), false);
		Span(Out, StoneDark, FVector(0, -FrontHalf - 10, FrontTop), FVector(26, FrontHalf + 10, FrontTop + 16), false);
		Span(Out, Stone, FVector(0, -FrontHalf - 14, FrontTop + 16), FVector(32, FrontHalf + 14, FrontTop + 26), false);

		// The three engine doors: a dark opening, two red leaves with panels,
		// a glazed band at the top with white bars, a segmental stone arch with a keystone.
		for (int32 i = 0; i < NumDoors; ++i)
		{
			const double Y = DoorY(i);
			Span(Out, Glass, FVector(12, Y - DoorHalf, 0), FVector(13, Y + DoorHalf, DoorTop + 70), false);
			for (const double Side : { -1.0, 1.0 })
			{
				const double In = Y + Side * 2.0;
				const double Outer = Y + Side * DoorHalf;
				Span(Out, DoorRed, FVector(13, In, 0), FVector(19, Outer, DoorTop - 95), false);
				Span(Out, DoorRed, FVector(13, In, DoorTop - 10), FVector(19, Outer, DoorTop + 4), false);
				for (const double Z0 : { 20.0, 150.0 })
				{
					Span(Out, DoorDark, FVector(19, In + Side * 18, Z0), FVector(22, Outer - Side * 18, Z0 + 105), false);
				}
				// The glazed band: panes between white bars.
				Span(Out, Mullion, FVector(13, In, DoorTop - 95), FVector(20, Outer, DoorTop - 89), false);
				for (int32 k = 0; k <= 3; ++k)
				{
					const double BarY = In + (Outer - In) * k / 3.0;
					Span(Out, Mullion, FVector(13, BarY - 3, DoorTop - 89), FVector(20, BarY + 3, DoorTop - 10), false);
				}
				Add(Out, MarketModel::Sphere, Brass, FVector(22, Y + Side * 14, 130), FVector(7, 7, 7), 0.0, false);
			}
			// The arch over the door's head and its keystone.
			for (int32 k = 0; k <= 8; ++k)
			{
				ArchStone(Out, StoneDark, Y, DoorTop, DoorHalf + 18.0, k * 180.0 / 8.0, 22.0, 26.0, 52.0);
			}
			Span(Out, Stone, FVector(0, Y - 20, DoorTop + 82), FVector(26, Y + 20, DoorTop + 132), false);
		}
		// Rusticated piers between and beside the doors.
		for (int32 i = 0; i <= NumDoors; ++i)
		{
			const double Y = (i - NumDoors * 0.5) * DoorStep;
			for (double Z = 0.0; Z < DoorTop + 40.0; Z += 60.0)
			{
				Span(Out, (static_cast<int32>(Z / 60.0) % 2 == 0) ? Stone : StoneDark, FVector(12, Y - 44, Z + 2), FVector(22, Y + 44, Z + 58), false);
			}
		}

		// The name band over the doors.
		Span(Out, Band, FVector(12, -FrontHalf + 60, DoorTop + 150), FVector(16, FrontHalf - 60, FrontTop - 12), false);

		// The brigade's badge over the middle door: a brass helmet on two crossed axes.
		{
			const FVector At(28.0, 0.0, FrontTop + 120.0);
			for (const double Side : { -1.0, 1.0 })
			{
				Add(Out, MarketModel::Cube, Wood, At, FVector(4, 6, 140), 0.0, false, 0.0, Side * 38.0);
				const FVector Head = At + FVector(0.0, Side * 40.0, 50.0);
				Add(Out, MarketModel::Cube, Brass, Head, FVector(5, 30, 18), 0.0, false, 0.0, Side * 38.0);
			}
			Add(Out, MarketModel::Sphere, Brass, At + FVector(4, 0, 10), FVector(10, 46, 44), 0.0, false);
			Span(Out, Brass, At + FVector(2, -3, 30), At + FVector(10, 3, 46), false);
			Span(Out, Brass, At + FVector(0, -30, -12), At + FVector(8, 30, -8), false);
		}

		// Red lanterns on the outer piers, glowing at night.
		for (const double Side : { -1.0, 1.0 })
		{
			const double Y = Side * NumDoors * 0.5 * DoorStep;
			Span(Out, Iron, FVector(22, Y - 3, 330), FVector(56, Y + 3, 335), false);
			Span(Out, Iron, FVector(42, Y - 12, 324), FVector(70, Y + 12, 328), false);
			Span(Out, LampRed, FVector(45, Y - 10, 290), FVector(67, Y + 10, 324), false);
			Add(Out, MarketModel::Cone, Iron, FVector(56, Y, 334), FVector(30, 30, 14), 0.0, false);
		}

		// The alarm bell on a wall bracket, right of the doors.
		{
			const double Y = NumDoors * 0.5 * DoorStep + 150.0;
			Span(Out, Iron, FVector(12, Y - 10, 360), FVector(16, Y + 10, 420), false);
			Span(Out, Iron, FVector(12, Y - 3, 405), FVector(58, Y + 3, 411), false);
			Add(Out, MarketModel::Cone, Brass, FVector(52, Y, 380), FVector(40, 40, 34), 0.0, false);
			Add(Out, MarketModel::Sphere, Brass, FVector(52, Y, 398), FVector(14, 14, 12), 0.0, false);
			Span(Out, Iron, FVector(51, Y - 1, 330), FVector(53, Y + 1, 362), false);
		}

		// The street fire alarm box on its post, left of the doors: red with a
		// white glass window, a little roof.
		{
			const double Y = -NumDoors * 0.5 * DoorStep - 170.0;
			Add(Out, MarketModel::Cylinder, AlarmRed, FVector(60, Y, 55), FVector(14, 14, 110), 0.0, true);
			Span(Out, AlarmRed, FVector(46, Y - 18, 110), FVector(74, Y + 18, 160), true);
			Span(Out, White, FVector(74, Y - 11, 125), FVector(75, Y + 11, 148), false);
			Add(Out, MarketModel::Cone, AlarmRed, FVector(60, Y, 168), FVector(42, 42, 18), 0.0, false);
			Add(Out, MarketModel::Sphere, Brass, FVector(60, Y, 180), FVector(7, 7, 7), 0.0, false);
		}

		// The granite apron the engines roll out over.
		Span(Out, Granite, FVector(0, -NumDoors * 0.5 * DoorStep - 40, 0), FVector(330, NumDoors * 0.5 * DoorStep + 40, 2), false);

		// The drill tower: brick, stone bands, narrow windows facing the street,
		// an open lookout at the top with a balcony, a green copper roof, the flag.
		{
			const FVector T = TowerAt;
			const double H = TowerHalf;
			Span(Out, Brick, FVector(T.X - H, T.Y - H, 0), FVector(T.X + H, T.Y + H, LookoutFloor), false);
			for (double Z = 1600.0; Z < LookoutFloor; Z += 380.0)
			{
				Span(Out, Stone, FVector(T.X - H - 6, T.Y - H - 6, Z), FVector(T.X + H + 6, T.Y + H + 6, Z + 18), false);
			}
			for (double Z = 1700.0; Z < LookoutFloor - 200.0; Z += 380.0)
			{
				Span(Out, Glass, FVector(T.X + H, T.Y - 22, Z), FVector(T.X + H + 2, T.Y + 22, Z + 150), false);
				Span(Out, Stone, FVector(T.X + H, T.Y - 30, Z + 150), FVector(T.X + H + 6, T.Y + 30, Z + 162), false);
			}
			for (const double Sx : { -1.0, 1.0 })
			{
				for (const double Sy : { -1.0, 1.0 })
				{
					Span(Out, BrickDark, FVector(T.X + Sx * H - 18, T.Y + Sy * H - 18, 1500), FVector(T.X + Sx * H + 18, T.Y + Sy * H + 18, LookoutFloor), false);
				}
			}
			// The lookout: a floor slab, four corner piers, a railing round it, the cornice.
			Span(Out, Stone, FVector(T.X - H - 40, T.Y - H - 40, LookoutFloor), FVector(T.X + H + 40, T.Y + H + 40, LookoutFloor + 20), false);
			for (const double Sx : { -1.0, 1.0 })
			{
				for (const double Sy : { -1.0, 1.0 })
				{
					Span(Out, Brick, FVector(T.X + Sx * H - 32, T.Y + Sy * H - 32, LookoutFloor + 20), FVector(T.X + Sx * H + 32, T.Y + Sy * H + 32, LookoutTop), false);
				}
			}
			Span(Out, Glass, FVector(T.X - H + 30, T.Y - H + 30, LookoutFloor + 20), FVector(T.X + H - 30, T.Y + H - 30, LookoutTop), false);
			for (const double Side : { -1.0, 1.0 })
			{
				Span(Out, Iron, FVector(T.X - H - 38, T.Y + Side * (H + 36), LookoutFloor + 100), FVector(T.X + H + 38, T.Y + Side * (H + 40), LookoutFloor + 104), false);
				Span(Out, Iron, FVector(T.X + Side * (H + 36), T.Y - H - 38, LookoutFloor + 100), FVector(T.X + Side * (H + 40), T.Y + H + 38, LookoutFloor + 104), false);
			}
			Span(Out, Stone, FVector(T.X - H - 30, T.Y - H - 30, LookoutTop), FVector(T.X + H + 30, T.Y + H + 30, LookoutTop + 26), false);
			// The roof: a pyramid of shrinking slabs, a lantern of brass on top.
			for (int32 k = 0; k < 6; ++k)
			{
				const double Half = (H + 20.0) * (1.0 - k / 6.0);
				Span(Out, Roof, FVector(T.X - Half, T.Y - Half, LookoutTop + 26 + k * 40.0), FVector(T.X + Half, T.Y + Half, LookoutTop + 66 + k * 40.0), false);
			}
			const double Peak = LookoutTop + 26 + 6 * 40.0;
			Add(Out, MarketModel::Sphere, Brass, FVector(T.X, T.Y, Peak + 10), FVector(22, 22, 22), 0.0, false);
			// The flagpole and a red, white and green flag.
			Add(Out, MarketModel::Cylinder, Iron, FVector(T.X, T.Y, Peak + 170), FVector(6, 6, 320), 0.0, false);
			const FLinearColor Bands[] = { Colors::AlarmRed, Colors::White, Colors::Green };
			for (int32 i = 0; i < 3; ++i)
			{
				Span(Out, Bands[i], FVector(T.X - 1, T.Y + 4, Peak + 300 - (i + 1) * 30.0), FVector(T.X + 1, T.Y + 154, Peak + 300 - i * 30.0), false);
			}
		}
	}

	/** The name over the doors, and the alarm box's word. */
	inline void Signs(TArray<FSign>& Out)
	{
		using namespace Colors;
		Out.Add({ TEXT("SZ\u00c9KESF\u0150V\u00c1ROSI T\u0170ZOLT\u00d3S\u00c1G"), FVector(16.5, 0.0, DoorTop + 228.0), 0.0, 44.0, Letters });
		Out.Add({ TEXT("VIII. KER\u00dcLETI T\u0170Z\u0150RS\u00c9G"), FVector(16.5, 0.0, DoorTop + 178.0), 0.0, 28.0, Letters });
		Out.Add({ TEXT("T\u0170ZJELZ\u0150"), FVector(75.5, -NumDoors * 0.5 * DoorStep - 170.0, 115.0), 0.0, 6.0, White });
	}

	inline void Lamps(TArray<FLamp>& Out)
	{
		for (const double Side : { -1.0, 1.0 })
		{
			Out.Add({ FVector(56.0, Side * NumDoors * 0.5 * DoorStep, 305.0), 380.f });
		}
	}
}
