// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MarketModel.h"

/**
 * Things to run over and under in a chase (FreeRun), as basic shapes in the
 * manner of MarketModel: a handcart, a stack of crates and a row of barrels
 * to vault, a carpet hung on a beating rack and a road menders' plank on two
 * trestles to slide under. Each stands across its +X, so running along X
 * meets it broadside.
 *
 * Header-only so the offline preview builds the same shapes.
 */
namespace ChaseModel
{
	enum class EObstacle : uint8 { Handcart, Crates, Barrels, CarpetRack, Trestles, Count };

	/** An obstacle in some frame (for the market hall's yard, the hall's). */
	struct FSpot
	{
		EObstacle Kind;
		FVector At;
		double Yaw;
	};

	namespace Paint
	{
		const FLinearColor Wood(0.36f, 0.25f, 0.13f);
		const FLinearColor Weathered(0.42f, 0.36f, 0.27f);
		const FLinearColor Sacking(0.55f, 0.47f, 0.32f);
		const FLinearColor Stave(0.40f, 0.24f, 0.11f);
		const FLinearColor Hoop(0.10f, 0.09f, 0.08f);
		const FLinearColor Red(0.55f, 0.07f, 0.05f);
		const FLinearColor White(0.82f, 0.80f, 0.74f);
	}

	// The heights that make each one a vault or a slide (cm): see FreeRun's limits.
	constexpr double CartBed = 62.0;
	constexpr double CartSide = 28.0;
	constexpr double CrateHeight = 42.0;
	constexpr double BarrelHeight = 95.0;
	constexpr double RackBar = 170.0;
	constexpr double CarpetDrop = 85.0;
	constexpr double PlankTop = 110.0;

	inline void Build(TArray<MarketModel::FShape>& Out, const FVector& Middle, double Yaw, EObstacle Kind, MarketModel::FRandom& Random)
	{
		using namespace MarketModel;
		using namespace MarketModel::Colors;
		using namespace ChaseModel::Paint;
		FBuilder B{ Out, Middle, Yaw };
		switch (Kind)
		{
		case EObstacle::Handcart:
		{
			// A two-wheeled handcart broadside on, its handles off to one side, a few sacks in it.
			B.Add(Cube, Wood, FVector(0.0, 0.0, CartBed), FVector(100.0, 170.0, 6.0), 0.0, 0.0, 0.0, true);
			for (const double X : { -48.0, 48.0 })
			{
				B.Add(Cube, Weathered, FVector(X, 0.0, CartBed + CartSide * 0.5), FVector(4.0, 170.0, CartSide), 0.0, 0.0, 0.0, true);
			}
			for (const double Y : { -83.0, 83.0 })
			{
				B.Add(Cube, Weathered, FVector(0.0, Y, CartBed + CartSide * 0.5), FVector(100.0, 4.0, CartSide), 0.0, 0.0, 0.0, true);
			}
			for (const double X : { -58.0, 58.0 })
			{
				B.Add(Cylinder, Wood, FVector(X, 0.0, 46.0), FVector(92.0, 92.0, 7.0), 90.0, 0.0, 0.0, true);
				B.Add(Cylinder, Hoop, FVector(X * 1.04, 0.0, 46.0), FVector(16.0, 16.0, 6.0), 90.0);
			}
			B.Add(Cylinder, Hoop, FVector(0.0, 0.0, 46.0), FVector(5.0, 5.0, 122.0), 90.0);
			for (const double X : { -36.0, 36.0 })
			{
				B.Add(Cube, Wood, FVector(X, 145.0, CartBed - 4.0), FVector(5.0, 130.0, 5.0), -6.0, 0.0, 0.0);
			}
			B.Add(Cube, Wood, FVector(0.0, 205.0, CartBed - 10.0), FVector(80.0, 5.0, 5.0));
			for (int32 i = 0; i < 3; ++i)
			{
				B.Add(Sphere, Sacking, FVector(Random.Range(-20.0, 20.0), -50.0 + 50.0 * i, CartBed + 20.0),
					FVector(60.0, 45.0, 34.0), 0.0, Random.Range(-30.0, 30.0));
			}
			break;
		}
		case EObstacle::Crates:
		{
			// Two layers of crates: three below, two on top.
			for (int32 i = 0; i < 3; ++i)
			{
				B.Add(Cube, Crate, FVector(Random.Range(-4.0, 4.0), -76.0 + 76.0 * i, CrateHeight * 0.5),
					FVector(52.0, 72.0, CrateHeight), 0.0, Random.Range(-4.0, 4.0), 0.0, true);
			}
			for (int32 i = 0; i < 2; ++i)
			{
				B.Add(Cube, Crate, FVector(Random.Range(-4.0, 4.0), -38.0 + 76.0 * i, CrateHeight * 1.5),
					FVector(52.0, 72.0, CrateHeight), 0.0, Random.Range(-6.0, 6.0), 0.0, true);
			}
			// The slats, and apples heaped in one.
			for (int32 i = 0; i < 3; ++i)
			{
				B.Add(Cube, Weathered, FVector(-26.5, -76.0 + 76.0 * i, CrateHeight * 0.5), FVector(1.0, 74.0, 6.0));
			}
			for (int32 k = 0; k < 6; ++k)
			{
				B.Add(Sphere, Red, FVector(Random.Range(-15.0, 15.0), 38.0 + Random.Range(-25.0, 25.0), CrateHeight * 2.0 + 4.0),
					FVector(9.0, 9.0, 9.0));
			}
			break;
		}
		case EObstacle::Barrels:
		{
			for (int32 i = 0; i < 3; ++i)
			{
				const FVector At(Random.Range(-6.0, 6.0), -68.0 + 68.0 * i, BarrelHeight * 0.5);
				B.Add(Cylinder, Stave, At, FVector(64.0, 64.0, BarrelHeight), 0.0, 0.0, 0.0, true);
				for (const double Z : { 14.0, BarrelHeight - 14.0 })
				{
					B.Add(Cylinder, Hoop, FVector(At.X, At.Y, Z), FVector(66.0, 66.0, 4.0));
				}
				B.Add(Cylinder, Wood, FVector(At.X, At.Y, BarrelHeight + 0.5), FVector(58.0, 58.0, 1.0));
			}
			break;
		}
		case EObstacle::CarpetRack:
		{
			// An iron carpet-beating rack with a carpet hung over its bar, down to hip height.
			for (const double Y : { -165.0, 165.0 })
			{
				B.Add(Cylinder, Hoop, FVector(0.0, Y, RackBar * 0.5), FVector(7.0, 7.0, RackBar), 0.0, 0.0, 0.0, true);
			}
			B.Add(Cylinder, Hoop, FVector(0.0, 0.0, RackBar), FVector(5.0, 5.0, 336.0), 0.0, 0.0, 90.0);
			const FLinearColor& Field = Pick(Dyes, Random);
			const FLinearColor& Edge = Random.Unit() < 0.5 ? Indigo : Madder;
			const double Hang = RackBar - CarpetDrop;
			B.Add(Cube, Field, FVector(0.0, 0.0, CarpetDrop + Hang * 0.5), FVector(10.0, 260.0, Hang), 0.0, 0.0, 0.0, true);
			B.Add(Cube, Field, FVector(0.0, 0.0, RackBar + 3.0), FVector(12.0, 260.0, 4.0));
			for (const double X : { -5.5, 5.5 })
			{
				B.Add(Cube, Edge, FVector(X, 0.0, CarpetDrop + Hang * 0.5), FVector(1.0, 230.0, Hang - 18.0));
				B.Add(Cube, Wool, FVector(X * 1.1, 0.0, CarpetDrop + Hang * 0.5), FVector(1.0, 80.0, Hang * 0.55), 0.0, 0.0, 45.0);
				B.Add(Cube, Field, FVector(X * 1.2, 0.0, CarpetDrop + Hang * 0.5), FVector(1.0, 30.0, Hang * 0.25), 0.0, 0.0, 45.0);
			}
			// The fringe.
			for (int32 k = 0; k < 14; ++k)
			{
				B.Add(Cube, Wool, FVector(0.0, -120.0 + k * (240.0 / 13.0), CarpetDrop - 5.0), FVector(11.0, 6.0, 10.0));
			}
			break;
		}
		case EObstacle::Trestles:
		{
			// Road menders' barrier: a red and white plank laid across two trestles.
			for (const double Y : { -135.0, 135.0 })
			{
				B.Add(Cube, Wood, FVector(0.0, Y, PlankTop - 12.0), FVector(50.0, 12.0, 10.0));
				for (const double X : { -1.0, 1.0 })
				{
					for (const double Side : { -1.0, 1.0 })
					{
						// Splayed legs, out to the sides and to front and back.
						B.Add(Cube, Wood, FVector(X * 18.0, Y + Side * 10.0, (PlankTop - 17.0) * 0.5), FVector(6.0, 6.0, PlankTop - 10.0),
							X * 10.0, 0.0, Side * 8.0, true);
					}
				}
			}
			B.Add(Cube, White, FVector(0.0, 0.0, PlankTop - 2.5), FVector(25.0, 340.0, 5.0), 0.0, 0.0, 0.0, true);
			for (int32 k = 0; k < 6; ++k)
			{
				// Red bands slanting across the white.
				B.Add(Cube, Red, FVector(-12.8, -140.0 + k * 56.0, PlankTop - 2.5), FVector(0.6, 28.0, 5.2));
				B.Add(Cube, Red, FVector(12.8, -140.0 + k * 56.0, PlankTop - 2.5), FVector(0.6, 28.0, 5.2));
			}
			// A hurricane lamp hanging from it.
			B.Add(Cube, Red, FVector(0.0, 0.0, PlankTop - 22.0), FVector(12.0, 12.0, 16.0));
			B.Add(Cylinder, Hoop, FVector(0.0, 0.0, PlankTop - 10.0), FVector(1.0, 1.0, 10.0));
			break;
		}
		default:
			break;
		}
	}

	/**
	 * Deliveries in front of the market hall's gateway, in the hall's frame
	 * (+X into the hall, so these lie out on the square): a line of them
	 * from the gate towards the garden's east path, over and under in turn.
	 */
	inline TArray<FSpot> HallYard()
	{
		return {
			{ EObstacle::Handcart, FVector(-625.0, -67.0, 0.0), -168.0 },
			{ EObstacle::CarpetRack, FVector(-950.0, -134.0, 0.0), -168.0 },
			{ EObstacle::Crates, FVector(-1276.0, -201.0, 0.0), -168.0 },
			{ EObstacle::Trestles, FVector(-1601.0, -269.0, 0.0), -168.0 },
			{ EObstacle::Barrels, FVector(-1890.0, -328.0, 0.0), -168.0 },
		};
	}

	/**
	 * The pickpocket's way out, in the hall's frame (cm): from the stalls
	 * where he snatches the purse, down the nave and out of the side door on
	 * Vásár utca, along its pavement and round the corner onto Rákóczi tér,
	 * past the front over the deliveries (HallYard), into the garden by its
	 * east path, round the promenade's north side and the fountain, and out
	 * onto József körút, where he is gone if not caught.
	 */
	inline TArray<FVector2D> ThiefRoute()
	{
		return {
			FVector2D(1300.0, 560.0), FVector2D(1700.0, 250.0), FVector2D(2300.0, 0.0), FVector2D(3000.0, 0.0),
			FVector2D(3555.0, -350.0), FVector2D(3555.0, -1500.0), FVector2D(3555.0, -2600.0), FVector2D(3555.0, -3400.0),
			FVector2D(3555.0, -3900.0), FVector2D(3300.0, -4150.0), FVector2D(1800.0, -4150.0), FVector2D(300.0, -4150.0),
			FVector2D(-450.0, -3950.0), FVector2D(-650.0, -3300.0), FVector2D(-450.0, -1200.0), FVector2D(-300.0, 0.0),
			FVector2D(-2107.0, -373.0), FVector2D(-3721.0, -696.0), FVector2D(-3879.0, -1514.0), FVector2D(-4601.0, -2228.0),
			FVector2D(-5693.0, -2646.0), FVector2D(-6861.0, -2655.0), FVector2D(-7794.0, -2254.0), FVector2D(-8179.0, -1758.0),
			FVector2D(-8610.0, -2347.0), FVector2D(-8827.0, -2805.0), FVector2D(-9317.0, -2934.0), FVector2D(-9672.0, -2727.0),
			FVector2D(-10150.0, -2828.0), FVector2D(-10792.0, -3111.0), FVector2D(-10949.0, -3858.0), FVector2D(-10592.0, -5004.0),
		};
	}

	/**
	 * Things in his way along it, in the hall's frame, each standing across
	 * the way: crates in the nave, barrels and a porters' plank in the
	 * cross hall, a handcart and a carpet being beaten on the pavement of
	 * Vásár utca, crates by the front, and in the garden a mended path, a
	 * gardener's cart, a flower seller's crates and barrels at the west gate.
	 */
	inline TArray<FSpot> RouteSpots()
	{
		return {
			{ EObstacle::Crates, FVector(2685.0, 0.0, 0.0), 0.0 },
			{ EObstacle::Barrels, FVector(3555.0, -2050.0, 0.0), -90.0 },
			{ EObstacle::Trestles, FVector(3555.0, -3080.0, 0.0), -90.0 },
			{ EObstacle::Handcart, FVector(2700.0, -4150.0, 0.0), 180.0 },
			{ EObstacle::CarpetRack, FVector(1200.0, -4150.0, 0.0), 180.0 },
			{ EObstacle::Crates, FVector(-550.0, -2250.0, 0.0), 85.0 },
			{ EObstacle::Trestles, FVector(-4240.0, -1871.0, 0.0), -135.0 },
			{ EObstacle::Handcart, FVector(-6277.0, -2651.0, 0.0), -180.0 },
			{ EObstacle::Crates, FVector(-9072.0, -2870.0, 0.0), -165.0 },
			{ EObstacle::Barrels, FVector(-10471.0, -2969.0, 0.0), -156.0 },
		};
	}
}
