// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PersonKit.h"
#include "MarketModel.h"
#include "MusicBarModel.h"

/**
 * The nine places to eat of AEateries, each a room the player walks into
 * through its street door (the way the music bars work):
 *
 * - Hauer: the Hauer pastry shop on Rákóczi út, cream walls, gilt mirrors,
 *   a glass case of cakes, marble tables.
 * - Kifozde: a cheap cook-shop on Népszínház utca, a whitewashed vault,
 *   oilcloth on long tables with benches, the pots on the serving counter.
 * - Tejcsarnok: a milk bar on József utca, white tiles, a marble counter with
 *   milk churns, a shelf along the wall to stand at.
 * - Fuvaros: the carters' inn on Nagy Fuvaros utca, dark wood, long tables,
 *   harness and whips on the walls, beer from the barrel.
 * - Borozo: a wine cellar on Teleki tér, a brick vault with big barrels,
 *   barrel tables, card players.
 * - Kispipa: the Kispipa inn on Akácfa utca, checked cloths and an upright
 *   piano with its pianist.
 * - Bufe: a standing buffet on Baross tér by Keleti, a long counter with a
 *   glass case of sandwiches, high tables to stand at.
 * - Koser: a kosher eating house on Scheiber Sándor utca by the seminary,
 *   white cloths, a seven-branched candlestick on the dresser.
 * - Nemzeti: the restaurant of the Hotel Nemzeti, a tall hall with
 *   chandeliers, palms, mirrors and silver on white cloths.
 *
 * Each room is built in its own frame like MusicBarModel's: the street door
 * in the middle of the front wall at X = 0, the room along +X, the floor at
 * Z = 0. Build fills the shapes and an FLayout: where the lamps hang, the
 * seats and standing spots for guests, the waiter's round or the counter
 * where the staff stands and the player orders, and the sign.
 */
namespace EateryModel
{
	using MarketModel::FShape;
	using PersonKit::FOutfit;

	enum class EEatery : uint8 { Hauer, Kifozde, Tejcsarnok, Fuvaros, Borozo, Kispipa, Bufe, Koser, Nemzeti, Count };
	constexpr int32 NumEateries = static_cast<int32>(EEatery::Count);

	/** A spot on the floor and which way one faces there. */
	struct FSpot
	{
		FVector At = FVector::ZeroVector;
		double Yaw = 0.0;
	};

	struct FLayout
	{
		double Depth = 900.0;
		double HalfWidth = 500.0;
		double Height = 340.0;
		/** Where the lights hang. */
		TArray<FVector> Lamps;
		/** Chairs and bench places (the guest faces Yaw). */
		TArray<FSpot> Seats;
		/** Places to stand at a high table, shelf or the counter. */
		TArray<FSpot> Standers;
		/** A waiter goes round these (true), or the staff serve at the counter. */
		bool bWaiter = false;
		FVector WaiterFrom = FVector::ZeroVector;
		TArray<FVector> TableStops;
		/** Who stands behind the counter, and where the player orders from it. */
		FSpot Staff;
		FVector OrderAt = FVector::ZeroVector;
		/** A second one behind the counter (null spot: none). */
		bool bSecondStaff = false;
		FSpot SecondStaff;
		/** The piano and its player (Kispipa). */
		bool bPiano = false;
		FSpot Pianist;
		/** The name inside, on the back wall. */
		FVector SignAt = FVector::ZeroVector;
		double SignHeight = 30.0;
		FLinearColor SignColor = FLinearColor::White;
	};

	// Colours.
	inline FLinearColor Whitewash() { return FLinearColor(0.80f, 0.78f, 0.72f); }
	inline FLinearColor Brick() { return FLinearColor(0.42f, 0.18f, 0.10f); }
	inline FLinearColor Oak() { return FLinearColor(0.30f, 0.17f, 0.08f); }
	inline FLinearColor WhiteTile() { return FLinearColor(0.86f, 0.87f, 0.84f); }
	inline FLinearColor Oilcloth() { return FLinearColor(0.70f, 0.74f, 0.66f); }
	inline FLinearColor Cream() { return FLinearColor(0.86f, 0.80f, 0.64f); }
	inline FLinearColor Palm() { return FLinearColor(0.10f, 0.28f, 0.09f); }
	inline FLinearColor Silver() { return FLinearColor(0.75f, 0.76f, 0.78f); }

	/** The name inside each place (on its back wall), and on the maps. */
	inline const TCHAR* SignOf(EEatery Kind)
	{
		switch (Kind)
		{
		case EEatery::Hauer: return TEXT("HAUER");
		case EEatery::Kifozde: return TEXT("KIFŐZDE");
		case EEatery::Tejcsarnok: return TEXT("KÖZPONTI TEJCSARNOK");
		case EEatery::Fuvaros: return TEXT("FUVAROS VENDÉGLŐ");
		case EEatery::Borozo: return TEXT("BOROZÓ");
		case EEatery::Kispipa: return TEXT("KISPIPA");
		case EEatery::Bufe: return TEXT("ÁLLÓBÜFÉ");
		case EEatery::Koser: return TEXT("KÓSER ÉTKEZDE");
		default: return TEXT("NEMZETI ÉTTEREM");
		}
	}

	/** Floor, walls, ceiling, the front wall with the door and two windows. bVault: a whitewashed cellar vault instead of a flat ceiling. */
	struct FShell
	{
		FLinearColor FloorA;
		FLinearColor FloorB;
		bool bChecker = false;
		FLinearColor Low;
		FLinearColor High;
		FLinearColor Rail;
		double LowHeight = 120.0;
		FLinearColor Ceiling;
		bool bVault = false;
		FLinearColor Door;
	};

	inline void Shell(MarketModel::FBuilder& B, const FLayout& L, const FShell& S)
	{
		using namespace MarketModel;
		const double D = L.Depth, W = L.HalfWidth, H = L.Height;
		// The floor: boards along X, or tiles in a chequer.
		if (S.bChecker)
		{
			const int32 Rows = FMath::CeilToInt(D / 100.0), Cols = FMath::CeilToInt(2.0 * W / 100.0);
			for (int32 i = 0; i < Rows; ++i)
			{
				for (int32 j = 0; j < Cols; ++j)
				{
					const double X = FMath::Min(D - 50.0, 50.0 + i * 100.0), Y = FMath::Min(W - 50.0, -W + 50.0 + j * 100.0);
					B.Add(Cube, (i + j) % 2 ? S.FloorA : S.FloorB, FVector(X, Y, -1.0), FVector(100.0, 100.0, 2.0), 0.0, 0.0, 0.0, true);
				}
			}
		}
		else
		{
			MarketModel::FRandom Random(77);
			const int32 Strips = FMath::CeilToInt(2.0 * W / 100.0);
			for (int32 i = 0; i < Strips; ++i)
			{
				const double Y = FMath::Min(W - 50.0, -W + 50.0 + i * 100.0);
				B.Add(Cube, S.FloorA + (S.FloorB - S.FloorA) * float(Random.Unit()), FVector(D * 0.5, Y, -1.0), FVector(D, 98.0, 2.0), 0.0, 0.0, 0.0, true);
			}
		}
		// Back and side walls, with a darker band below and a rail on it.
		B.Add(Cube, S.High, FVector(D + 10.0, 0.0, H * 0.5), FVector(20.0, 2.0 * W + 40.0, H), 0.0, 0.0, 0.0, true);
		B.Add(Cube, S.High, FVector(D * 0.5, -W - 10.0, H * 0.5), FVector(D, 20.0, H), 0.0, 0.0, 0.0, true);
		B.Add(Cube, S.High, FVector(D * 0.5, W + 10.0, H * 0.5), FVector(D, 20.0, H), 0.0, 0.0, 0.0, true);
		B.Add(Cube, S.Low, FVector(D - 1.0, 0.0, S.LowHeight * 0.5), FVector(4.0, 2.0 * W, S.LowHeight));
		B.Add(Cube, S.Low, FVector(D * 0.5, -W + 1.0, S.LowHeight * 0.5), FVector(D, 4.0, S.LowHeight));
		B.Add(Cube, S.Low, FVector(D * 0.5, W - 1.0, S.LowHeight * 0.5), FVector(D, 4.0, S.LowHeight));
		B.Add(Cube, S.Rail, FVector(D - 3.0, 0.0, S.LowHeight + 2.0), FVector(8.0, 2.0 * W, 5.0));
		B.Add(Cube, S.Rail, FVector(D * 0.5, -W + 3.0, S.LowHeight + 2.0), FVector(D, 8.0, 5.0));
		B.Add(Cube, S.Rail, FVector(D * 0.5, W - 3.0, S.LowHeight + 2.0), FVector(D, 8.0, 5.0));
		// The front wall: the door in the middle, a window either side.
		const double Win = FMath::Min(170.0, W - 160.0);
		const double WinY = FMath::Min(W - Win * 0.5 - 40.0, 110.0 + Win * 0.5 + 40.0);
		B.Add(Cube, S.High, FVector(-10.0, -W * 0.5 - 50.0, H * 0.5), FVector(20.0, W - 100.0 + 40.0, H), 0.0, 0.0, 0.0, true);
		B.Add(Cube, S.High, FVector(-10.0, W * 0.5 + 50.0, H * 0.5), FVector(20.0, W - 100.0 + 40.0, H), 0.0, 0.0, 0.0, true);
		B.Add(Cube, S.High, FVector(-10.0, 0.0, (H + 230.0) * 0.5), FVector(20.0, 160.0, H - 230.0), 0.0, 0.0, 0.0, true);
		B.Add(Cube, S.Door, FVector(-4.0, 0.0, 115.0), FVector(8.0, 150.0, 230.0), 0.0, 0.0, 0.0, true);
		B.Add(Cube, MusicBarModel::Brass(), FVector(2.0, 50.0, 105.0), FVector(4.0, 4.0, 14.0));
		B.Add(Cube, FLinearColor(0.80f, 0.76f, 0.62f), FVector(0.5, 0.0, 200.0), FVector(2.0, 110.0, 40.0));
		for (int32 Side = -1; Side <= 1; Side += 2)
		{
			B.Add(Cube, S.Door, FVector(1.0, Side * WinY, 175.0), FVector(3.0, Win + 16.0, 166.0));
			B.Add(Cube, FLinearColor(0.85f, 0.80f, 0.66f), FVector(2.5, Side * WinY, 175.0), FVector(2.0, Win, 150.0));
			B.Add(Cube, MusicBarModel::Cloth(), FVector(4.0, Side * WinY, 225.0), FVector(2.0, Win, 46.0));
		}
		// The ceiling: flat, or a low vault of whitewashed brick on the side walls.
		if (!S.bVault)
		{
			B.Add(Cube, S.Ceiling, FVector(D * 0.5, 0.0, H + 5.0), FVector(D + 40.0, 2.0 * W + 40.0, 10.0));
			return;
		}
		const double Spring = H - W * 0.45;
		const int32 Steps = 7;
		for (int32 Side = -1; Side <= 1; Side += 2)
		{
			for (int32 i = 0; i < Steps; ++i)
			{
				// Arc of a flattened circle from the wall top up to the crown.
				const double A0 = (double(i) / Steps) * HALF_PI, A1 = (double(i + 1) / Steps) * HALF_PI;
				const FVector2D P0(W * FMath::Cos(A0), W * 0.45 * FMath::Sin(A0));
				const FVector2D P1(W * FMath::Cos(A1), W * 0.45 * FMath::Sin(A1));
				const FVector2D Mid = (P0 + P1) * 0.5;
				const double Len = (P1 - P0).Size() + 6.0;
				// A positive roll tips the slab's +Y end down, so roll by minus its rise along +Y.
				const double Tilt = FMath::RadiansToDegrees(FMath::Atan2(P1.Y - P0.Y, Side * (P1.X - P0.X)));
				B.Add(Cube, S.Ceiling, FVector(D * 0.5, Side * Mid.X, Spring + Mid.Y + 6.0), FVector(D + 40.0, Len, 12.0), 0.0, 0.0, -Tilt);
			}
		}
		// Brick arches across the vault every few metres.
		for (double X = 150.0; X < D; X += 300.0)
		{
			for (int32 Side = -1; Side <= 1; Side += 2)
			{
				B.Add(Cube, Brick(), FVector(X, Side * (W - 12.0), Spring * 0.5), FVector(36.0, 24.0, Spring));
			}
		}
	}

	/** A small round marble table on one iron foot, with chairs round it. */
	inline void MarbleTable(MarketModel::FBuilder& B, FLayout& L, const FVector2D& At, int32 Chairs, MarketModel::FRandom& Random, double Turn0 = 0.0)
	{
		using namespace MarketModel;
		const FVector T(At, 0.0);
		B.Add(Cylinder, MusicBarModel::BlackWood(), T + FVector(0.0, 0.0, 36.0), FVector(8.0, 8.0, 72.0), 0.0, 0.0, 0.0, true);
		B.Add(Cylinder, MusicBarModel::BlackWood(), T + FVector(0.0, 0.0, 2.0), FVector(44.0, 44.0, 4.0));
		B.Add(Cylinder, MusicBarModel::Marble(), T + FVector(0.0, 0.0, 74.0), FVector(70.0, 70.0, 4.0));
		for (int32 c = 0; c < Chairs; ++c)
		{
			const double Turn = Turn0 + c * (360.0 / Chairs) + 12.0 * (Random.Unit() - 0.5);
			const FVector Seat = T + FRotator(0.0, Turn, 0.0).RotateVector(FVector(-55.0, 0.0, 0.0));
			MusicBarModel::Chair(B, Seat, Turn);
			L.Seats.Add({ Seat, Turn });
		}
		L.TableStops.Add(T + FVector(-60.0, 60.0, 0.0));
	}

	/** A square table with a cloth (and a stripe or checks), four chairs. */
	inline void ClothTable(MarketModel::FBuilder& B, FLayout& L, const FVector2D& At, const FLinearColor& Cloth, const FLinearColor& Check, bool bChecked, MarketModel::FRandom& Random, bool bSilver = false)
	{
		using namespace MarketModel;
		const FVector T(At, 0.0);
		B.Add(Cube, MusicBarModel::Walnut(), T + FVector(0.0, 0.0, 37.0), FVector(70.0, 70.0, 74.0), 0.0, 0.0, 0.0, true);
		B.Add(Cube, Cloth, T + FVector(0.0, 0.0, 76.0), FVector(92.0, 92.0, 3.0));
		if (bChecked)
		{
			for (int32 i = -2; i <= 2; ++i)
			{
				B.Add(Cube, Check, T + FVector(i * 18.0, 0.0, 77.7), FVector(8.0, 92.4, 0.4));
				B.Add(Cube, Check, T + FVector(0.0, i * 18.0, 77.8), FVector(92.4, 8.0, 0.4));
			}
		}
		else
		{
			B.Add(Cube, Check, T + FVector(0.0, 0.0, 77.8), FVector(92.5, 16.0, 0.6));
		}
		if (bSilver)
		{
			// A silver vase with a flower, and two plates laid with knives and forks.
			B.Add(Cylinder, Silver(), T + FVector(0.0, 0.0, 88.0), FVector(7.0, 7.0, 20.0));
			B.Add(Sphere, FLinearColor(0.60f, 0.05f, 0.08f), T + FVector(0.0, 0.0, 101.0), FVector(8.0, 8.0, 8.0));
			for (int32 s = -1; s <= 1; s += 2)
			{
				B.Add(Cylinder, MusicBarModel::Cloth(), T + FVector(s * 28.0, 0.0, 79.0), FVector(26.0, 26.0, 1.5));
				B.Add(Cube, Silver(), T + FVector(s * 28.0, 17.0, 78.3), FVector(18.0, 2.0, 0.6));
				B.Add(Cube, Silver(), T + FVector(s * 28.0, -17.0, 78.3), FVector(18.0, 2.0, 0.6));
			}
		}
		else
		{
			B.Add(Cylinder, FLinearColor(0.60f, 0.62f, 0.55f), T + FVector(10.0, 12.0, 88.0), FVector(8.0, 8.0, 22.0));
			B.Add(Cylinder, MusicBarModel::Cloth(), T + FVector(-14.0, -10.0, 79.0), FVector(24.0, 24.0, 1.5));
		}
		for (int32 c = 0; c < 4; ++c)
		{
			const double Turn = c * 90.0 + 12.0 * (Random.Unit() - 0.5);
			const FVector Seat = T + FRotator(0.0, Turn, 0.0).RotateVector(FVector(-62.0, 0.0, 0.0));
			MusicBarModel::Chair(B, Seat, Turn);
			L.Seats.Add({ Seat, Turn });
		}
		L.TableStops.Add(T + FVector(-70.0, 60.0, 0.0));
	}

	/** A long table along X with a bench either side (Length cm), its top in Top. */
	inline void LongTable(MarketModel::FBuilder& B, FLayout& L, const FVector2D& At, double Length, const FLinearColor& Top)
	{
		using namespace MarketModel;
		const FVector T(At, 0.0);
		B.Add(Cube, Oak(), T + FVector(0.0, 0.0, 37.0), FVector(Length - 20.0, 60.0, 74.0), 0.0, 0.0, 0.0, true);
		B.Add(Cube, Top, T + FVector(0.0, 0.0, 75.5), FVector(Length, 80.0, 3.0));
		for (int32 Side = -1; Side <= 1; Side += 2)
		{
			B.Add(Cube, Oak(), T + FVector(0.0, Side * 70.0, 22.0), FVector(Length - 10.0, 28.0, 44.0), 0.0, 0.0, 0.0, true);
			const int32 Places = FMath::Max(2, int32(Length / 75.0));
			for (int32 i = 0; i < Places; ++i)
			{
				const double X = -Length * 0.5 + (i + 0.5) * Length / Places;
				// Facing the table: the bench at +Y faces -Y (yaw -90).
				L.Seats.Add({ T + FVector(X, Side * 70.0, 0.0), Side > 0 ? -90.0 : 90.0 });
				// Mugs and plates along the top.
				if ((i + (Side > 0 ? 1 : 0)) % 2 == 0)
				{
					B.Add(Cylinder, FLinearColor(0.82f, 0.80f, 0.72f), T + FVector(X, Side * 22.0, 78.0), FVector(24.0, 24.0, 1.5));
				}
				else
				{
					B.Add(Cylinder, FLinearColor(0.55f, 0.50f, 0.40f), T + FVector(X + 10.0, Side * 20.0, 83.0), FVector(8.0, 8.0, 12.0));
				}
			}
		}
		L.TableStops.Add(T + FVector(-Length * 0.5 - 40.0, 0.0, 0.0));
	}

	/** A barrel stood on end as a table, with three stools round it. */
	inline void BarrelTable(MarketModel::FBuilder& B, FLayout& L, const FVector2D& At, MarketModel::FRandom& Random)
	{
		using namespace MarketModel;
		const FVector T(At, 0.0);
		const FLinearColor Hoop(0.10f, 0.10f, 0.10f);
		B.Add(Cylinder, Oak(), T + FVector(0.0, 0.0, 47.0), FVector(66.0, 66.0, 94.0), 0.0, 0.0, 0.0, true);
		B.Add(Cylinder, Hoop, T + FVector(0.0, 0.0, 20.0), FVector(68.0, 68.0, 5.0));
		B.Add(Cylinder, Hoop, T + FVector(0.0, 0.0, 75.0), FVector(68.0, 68.0, 5.0));
		B.Add(Cylinder, FLinearColor(0.36f, 0.22f, 0.12f), T + FVector(0.0, 0.0, 95.0), FVector(62.0, 62.0, 2.0));
		// A jug of wine, glasses, and a few cards.
		B.Add(Cylinder, FLinearColor(0.80f, 0.78f, 0.70f), T + FVector(4.0, 6.0, 108.0), FVector(11.0, 11.0, 24.0));
		B.Add(Cylinder, FLinearColor(0.70f, 0.66f, 0.40f), T + FVector(-14.0, -10.0, 101.0), FVector(6.0, 6.0, 10.0));
		B.Add(Cube, FLinearColor(0.90f, 0.86f, 0.74f), T + FVector(14.0, -14.0, 96.3), FVector(9.0, 6.0, 0.4), 0.0, 25.0, 0.0);
		B.Add(Cube, FLinearColor(0.70f, 0.16f, 0.12f), T + FVector(-4.0, 18.0, 96.3), FVector(9.0, 6.0, 0.4), 0.0, -15.0, 0.0);
		for (int32 c = 0; c < 3; ++c)
		{
			const double Turn = c * 120.0 + 20.0 * (Random.Unit() - 0.5);
			const FVector Seat = T + FRotator(0.0, Turn, 0.0).RotateVector(FVector(-60.0, 0.0, 0.0));
			B.Add(Cylinder, Oak(), Seat + FVector(0.0, 0.0, 23.0), FVector(34.0, 34.0, 46.0), 0.0, 0.0, 0.0, true);
			L.Seats.Add({ Seat, Turn });
		}
		L.TableStops.Add(T + FVector(-70.0, 50.0, 0.0));
	}

	/** A high table on one leg to stand at, two places. */
	inline void HighTable(MarketModel::FBuilder& B, FLayout& L, const FVector2D& At)
	{
		using namespace MarketModel;
		const FVector T(At, 0.0);
		B.Add(Cylinder, MusicBarModel::BlackWood(), T + FVector(0.0, 0.0, 55.0), FVector(8.0, 8.0, 110.0), 0.0, 0.0, 0.0, true);
		B.Add(Cylinder, MusicBarModel::BlackWood(), T + FVector(0.0, 0.0, 2.0), FVector(46.0, 46.0, 4.0));
		B.Add(Cylinder, MusicBarModel::Marble(), T + FVector(0.0, 0.0, 111.0), FVector(64.0, 64.0, 4.0));
		B.Add(Cylinder, FLinearColor(0.80f, 0.60f, 0.20f), T + FVector(8.0, 4.0, 121.0), FVector(8.0, 8.0, 16.0));
		B.Add(Cylinder, FLinearColor(0.90f, 0.88f, 0.82f), T + FVector(-10.0, -8.0, 114.0), FVector(20.0, 20.0, 1.5));
		L.Standers.Add({ T + FVector(-48.0, 0.0, 0.0), 0.0 });
		L.Standers.Add({ T + FVector(48.0, 0.0, 0.0), 180.0 });
	}

	/** A counter along Y at X (its front, the customer's side, towards -X), from Y0 to Y1. */
	inline void Counter(MarketModel::FBuilder& B, double X, double Y0, double Y1, const FLinearColor& Body, const FLinearColor& Top)
	{
		using namespace MarketModel;
		const double Len = Y1 - Y0, Mid = (Y0 + Y1) * 0.5;
		B.Add(Cube, Body, FVector(X + 30.0, Mid, 51.0), FVector(60.0, Len, 102.0), 0.0, 0.0, 0.0, true);
		B.Add(Cube, Top, FVector(X + 28.0, Mid, 105.0), FVector(68.0, Len + 8.0, 6.0));
		B.Add(Cube, MusicBarModel::Brass(), FVector(X - 4.0, Mid, 14.0), FVector(4.0, Len, 4.0));
	}

	/** Shelves of bottles on the wall at X (facing -X), Y0 to Y1. */
	inline void BottleShelves(MarketModel::FBuilder& B, double X, double Y0, double Y1, MarketModel::FRandom& Random, int32 Shelves = 3)
	{
		using namespace MarketModel;
		for (int32 s = 0; s < Shelves; ++s)
		{
			const double Z = 140.0 + s * 40.0;
			B.Add(Cube, MusicBarModel::Walnut(), FVector(X - 15.0, (Y0 + Y1) * 0.5, Z), FVector(30.0, Y1 - Y0, 3.0));
			for (double Y = Y0 + 15.0; Y < Y1 - 10.0; Y += 24.0 + 6.0 * Random.Unit())
			{
				const int32 k = Random.Index(3);
				const FLinearColor Glass = k == 0 ? FLinearColor(0.06f, 0.16f, 0.06f) : k == 1 ? FLinearColor(0.25f, 0.12f, 0.03f) : FLinearColor(0.55f, 0.55f, 0.50f);
				const double Tall = 22.0 + 8.0 * Random.Unit();
				B.Add(Cylinder, Glass, FVector(X - 15.0, Y, Z + 1.5 + Tall * 0.5), FVector(7.0, 7.0, Tall));
			}
		}
	}

	/** A barrel lying on a cradle, its tap towards -X... turned by Yaw, Big for the cellar's casks. */
	inline void LyingBarrel(MarketModel::FBuilder& B, const FVector& At, double Yaw, double Big)
	{
		using namespace MarketModel;
		const FRotator R(0.0, Yaw, 0.0);
		B.Add(Cube, MusicBarModel::DarkPanel(), At - FVector(0.0, 0.0, Big * 0.5 + 8.0), FVector(Big * 1.1, Big * 0.9, 16.0), 0.0, Yaw, 0.0, true);
		B.Add(Cylinder, Oak(), At, FVector(Big, Big, Big * 1.25), 90.0, Yaw, 0.0, true);
		for (int32 h = -1; h <= 1; h += 2)
		{
			B.Add(Cylinder, FLinearColor(0.10f, 0.10f, 0.10f), At + R.RotateVector(FVector(h * Big * 0.4, 0.0, 0.0)), FVector(Big + 3.0, Big + 3.0, 5.0), 90.0, Yaw, 0.0);
		}
		B.Add(Cylinder, FLinearColor(0.36f, 0.22f, 0.12f), At + R.RotateVector(FVector(-Big * 0.63, 0.0, 0.0)), FVector(Big * 0.92, Big * 0.92, 2.0), 90.0, Yaw, 0.0);
		B.Add(Cylinder, MusicBarModel::Brass(), At + R.RotateVector(FVector(-Big * 0.66, 0.0, -Big * 0.3)), FVector(4.0, 4.0, 10.0), 90.0, Yaw, 0.0);
	}

	/** A glass case on the counter (cakes, sandwiches): its middle at X, Y, Len along Y. */
	inline void GlassCase(MarketModel::FBuilder& B, double X, double Y, double Len, const TArray<FLinearColor>& Goods, MarketModel::FRandom& Random)
	{
		using namespace MarketModel;
		const FLinearColor Glass(0.70f, 0.78f, 0.80f);
		B.Add(Cube, MusicBarModel::Brass(), FVector(X, Y, 109.0), FVector(56.0, Len, 2.0));
		B.Add(Cube, Glass, FVector(X - 27.0, Y, 132.0), FVector(1.0, Len, 44.0), 0.0, 0.0, 0.0);
		B.Add(Cube, Glass, FVector(X, Y, 154.0), FVector(56.0, Len, 1.0));
		B.Add(Cube, MusicBarModel::Brass(), FVector(X - 27.0, Y, 154.0), FVector(2.0, Len, 2.0));
		for (int32 Row = 0; Row < 2; ++Row)
		{
			for (double Off = -Len * 0.5 + 18.0; Off < Len * 0.5 - 12.0; Off += 26.0)
			{
				const FLinearColor C = Goods[Random.Index(Goods.Num())];
				const FVector P(X - 10.0 + Row * 20.0, Y + Off, 110.0 + Row * 20.0);
				B.Add(Cylinder, MusicBarModel::Cloth(), P, FVector(22.0, 22.0, 1.0));
				B.Add(Cube, C, P + FVector(0.0, 0.0, 5.0), FVector(14.0, 12.0, 9.0), 0.0, 20.0 * (Random.Unit() - 0.5), 0.0);
			}
			// A shelf for the back row.
			B.Add(Cube, Glass, FVector(X + 10.0, Y, 129.0), FVector(26.0, Len - 4.0, 1.0));
		}
	}

	/** A mirror in a gilt frame on the wall at Y = Side * W (or the back wall when Side is 0), its middle at X / Y. */
	inline void Mirror(MarketModel::FBuilder& B, const FLayout& L, int32 Side, double Along, double Z, double Wide, double Tall)
	{
		using namespace MarketModel;
		if (Side == 0)
		{
			B.Add(Cube, MusicBarModel::Gilt(), FVector(L.Depth - 3.0, Along, Z), FVector(3.0, Wide + 14.0, Tall + 14.0));
			B.Add(Cube, MusicBarModel::Mirror(), FVector(L.Depth - 5.0, Along, Z), FVector(3.0, Wide, Tall));
			return;
		}
		B.Add(Cube, MusicBarModel::Gilt(), FVector(Along, Side * (L.HalfWidth - 3.0), Z), FVector(Wide + 14.0, 3.0, Tall + 14.0));
		B.Add(Cube, MusicBarModel::Mirror(), FVector(Along, Side * (L.HalfWidth - 5.0), Z), FVector(Wide, 3.0, Tall));
	}

	/** A chandelier: a brass ring of glowing bulbs on a rod. */
	inline void Chandelier(MarketModel::FBuilder& B, const FVector& At, double Ceiling)
	{
		using namespace MarketModel;
		B.Add(Cylinder, MusicBarModel::Brass(), FVector(At.X, At.Y, (Ceiling + At.Z) * 0.5), FVector(3.0, 3.0, Ceiling - At.Z));
		B.Add(Sphere, MusicBarModel::Brass(), At, FVector(24.0, 24.0, 30.0));
		for (int32 i = 0; i < 8; ++i)
		{
			const FVector Arm = FRotator(0.0, i * 45.0, 0.0).RotateVector(FVector(45.0, 0.0, 0.0));
			B.Add(Cube, MusicBarModel::Brass(), At + Arm * 0.5, FVector(45.0, 3.0, 3.0), 0.0, i * 45.0, 0.0);
			B.Add(Sphere, MusicBarModel::LampGlow(), At + Arm + FVector(0.0, 0.0, 8.0), FVector(10.0, 10.0, 14.0));
		}
	}

	/** A palm in a big pot. */
	inline void PottedPalm(MarketModel::FBuilder& B, const FVector& At)
	{
		using namespace MarketModel;
		B.Add(Cylinder, FLinearColor(0.45f, 0.22f, 0.12f), At + FVector(0.0, 0.0, 30.0), FVector(56.0, 56.0, 60.0), 0.0, 0.0, 0.0, true);
		B.Add(Cylinder, FLinearColor(0.30f, 0.20f, 0.10f), At + FVector(0.0, 0.0, 90.0), FVector(8.0, 8.0, 60.0));
		for (int32 i = 0; i < 7; ++i)
		{
			const double Yaw = i * (360.0 / 7.0);
			const FVector Leaf = FRotator(0.0, Yaw, 0.0).RotateVector(FVector(40.0, 0.0, 0.0));
			B.Add(Cube, Palm(), At + FVector(0.0, 0.0, 130.0) + Leaf, FVector(90.0, 18.0, 2.0), -28.0, Yaw, 0.0);
		}
	}

	/** An upright piano against the wall at Y = W side, its keys facing -Y... built at At facing yaw (keys towards -X of its frame). */
	inline void Piano(MarketModel::FBuilder& B, const FVector& At, double Yaw)
	{
		using namespace MarketModel;
		const FRotator R(0.0, Yaw, 0.0);
		const FLinearColor Black(0.04f, 0.025f, 0.02f);
		B.Add(Cube, Black, At + R.RotateVector(FVector(0.0, 0.0, 65.0)), FVector(60.0, 150.0, 130.0), 0.0, Yaw, 0.0, true);
		B.Add(Cube, Black, At + R.RotateVector(FVector(-30.0, 0.0, 72.0)), FVector(30.0, 146.0, 6.0), 0.0, Yaw, 0.0);
		B.Add(Cube, FLinearColor(0.90f, 0.88f, 0.80f), At + R.RotateVector(FVector(-34.0, 0.0, 76.0)), FVector(16.0, 128.0, 2.0), 0.0, Yaw, 0.0);
		for (int32 k = 0; k < 18; ++k)
		{
			B.Add(Cube, Black, At + R.RotateVector(FVector(-30.0, -60.0 + k * 7.2, 78.0)), FVector(8.0, 2.5, 2.0), 0.0, Yaw, 0.0);
		}
		// Two brass candle arms and the music.
		for (int32 s = -1; s <= 1; s += 2)
		{
			B.Add(Cylinder, MusicBarModel::Brass(), At + R.RotateVector(FVector(-34.0, s * 60.0, 112.0)), FVector(5.0, 5.0, 12.0));
		}
		B.Add(Cube, FLinearColor(0.92f, 0.90f, 0.82f), At + R.RotateVector(FVector(-30.0, 0.0, 100.0)), FVector(2.0, 40.0, 28.0), -12.0, Yaw, 0.0);
		// The stool.
		B.Add(Cylinder, Black, At + R.RotateVector(FVector(-85.0, 0.0, 24.0)), FVector(36.0, 36.0, 48.0), 0.0, 0.0, 0.0, true);
	}

	/** Pictures in frames along a side wall. */
	inline void Pictures(MarketModel::FBuilder& B, const FLayout& L, int32 Side, double X0, double X1, int32 Count, const FLinearColor& Frame)
	{
		using namespace MarketModel;
		for (int32 i = 0; i < Count; ++i)
		{
			const double X = X0 + (X1 - X0) * (i + 0.5) / Count;
			B.Add(Cube, Frame, FVector(X, Side * (L.HalfWidth - 3.0), 190.0), FVector(52.0, 3.0, 40.0));
			B.Add(Cube, FLinearColor(0.35f, 0.30f, 0.22f) * (0.6f + 0.12f * float(i % 4)), FVector(X, Side * (L.HalfWidth - 5.0), 190.0), FVector(44.0, 3.0, 32.0));
		}
	}

	/** A milk churn (tall, tin, with a lid). */
	inline void Churn(MarketModel::FBuilder& B, const FVector& At)
	{
		using namespace MarketModel;
		B.Add(Cylinder, MusicBarModel::Steel(), At + FVector(0.0, 0.0, 30.0), FVector(34.0, 34.0, 60.0), 0.0, 0.0, 0.0, true);
		B.Add(Cone, MusicBarModel::Steel(), At + FVector(0.0, 0.0, 68.0), FVector(30.0, 30.0, 16.0));
		B.Add(Cylinder, MusicBarModel::Steel(), At + FVector(0.0, 0.0, 80.0), FVector(18.0, 18.0, 10.0));
		B.Add(Cylinder, FLinearColor(0.40f, 0.40f, 0.42f), At + FVector(0.0, 0.0, 86.0), FVector(20.0, 20.0, 3.0));
	}

	/** A big cooking pot with a lid on the counter top. */
	inline void Pot(MarketModel::FBuilder& B, const FVector& At, double Size, const FLinearColor& Enamel)
	{
		using namespace MarketModel;
		B.Add(Cylinder, Enamel, At + FVector(0.0, 0.0, Size * 0.4), FVector(Size, Size, Size * 0.8));
		B.Add(Cylinder, Enamel * 0.8f, At + FVector(0.0, 0.0, Size * 0.82), FVector(Size + 2.0, Size + 2.0, 3.0));
		B.Add(Sphere, MusicBarModel::BlackWood(), At + FVector(0.0, 0.0, Size * 0.86), FVector(8.0, 8.0, 6.0));
	}

	/** Builds a place's room into Out (frame at Origin, turned by Yaw) and fills L. */
	inline void Build(EEatery Kind, TArray<FShape>& Out, const FVector& Origin, double Yaw, FLayout& L)
	{
		using namespace MarketModel;
		using namespace MusicBarModel;
		FBuilder B{ Out, Origin, Yaw };
		FRandom Random(1932 + static_cast<uint32>(Kind) * 17u);
		L = FLayout();
		FShell S;
		S.FloorA = FLinearColor(0.17f, 0.10f, 0.05f);
		S.FloorB = FLinearColor(0.22f, 0.13f, 0.07f);
		S.Low = DarkPanel();
		S.High = Lime();
		S.Rail = Walnut();
		S.Ceiling = Lime();
		S.Door = Walnut();
		auto Lamp = [&B, &L](const FVector& At)
		{
			HangingLamp(B, At, L.Height - 2.0);
			L.Lamps.Add(At);
		};

		switch (Kind)
		{
		case EEatery::Hauer:
		{
			L.Depth = 1000.0; L.HalfWidth = 560.0; L.Height = 420.0;
			S.bChecker = true;
			S.FloorA = FLinearColor(0.85f, 0.84f, 0.80f);
			S.FloorB = FLinearColor(0.06f, 0.06f, 0.06f);
			S.Low = FLinearColor(0.42f, 0.26f, 0.14f);
			S.High = Cream();
			S.Rail = Gilt();
			S.Ceiling = FLinearColor(0.90f, 0.86f, 0.74f);
			S.Door = FLinearColor(0.30f, 0.16f, 0.08f);
			Shell(B, L, S);
			// The glass cake case on its counter along the right wall, the coffee machine behind it.
			Counter(B, 260.0, 300.0, 520.0, FLinearColor(0.42f, 0.26f, 0.14f), Marble());
			B.Add(Cube, FLinearColor(0.42f, 0.26f, 0.14f), FVector(500.0, 410.0, 51.0), FVector(560.0, 60.0, 102.0), 0.0, 0.0, 0.0, true);
			B.Add(Cube, Marble(), FVector(500.0, 410.0, 105.0), FVector(568.0, 68.0, 6.0));
			const TArray<FLinearColor> Cakes = { FLinearColor(0.28f, 0.14f, 0.06f), FLinearColor(0.85f, 0.70f, 0.42f), FLinearColor(0.90f, 0.86f, 0.74f),
				FLinearColor(0.60f, 0.12f, 0.16f), FLinearColor(0.20f, 0.10f, 0.05f), FLinearColor(0.80f, 0.55f, 0.25f) };
			GlassCase(B, 500.0, 400.0, 500.0, Cakes, Random);
			B.Add(Cube, Steel(), FVector(700.0, 520.0, 140.0), FVector(50.0, 40.0, 60.0));
			B.Add(Sphere, Brass(), FVector(700.0, 520.0, 182.0), FVector(30.0, 30.0, 30.0));
			Mirror(B, L, -1, 300.0, 210.0, 160.0, 180.0);
			Mirror(B, L, -1, 650.0, 210.0, 160.0, 180.0);
			Mirror(B, L, 0, -200.0, 220.0, 260.0, 200.0);
			// The staff: a girl behind the case, the player orders at its front.
			L.Staff = { FVector(500.0, 480.0, 0.0), -90.0 };
			L.OrderAt = FVector(500.0, 310.0, 0.0);
			const FVector2D Tables[7] = { FVector2D(180.0, -400.0), FVector2D(180.0, -150.0), FVector2D(180.0, 110.0), FVector2D(430.0, -400.0),
				FVector2D(430.0, -150.0), FVector2D(680.0, -400.0), FVector2D(680.0, -150.0) };
			for (const FVector2D& T : Tables)
			{
				MarbleTable(B, L, T, 3, Random);
			}
			L.bWaiter = true;
			L.WaiterFrom = FVector(820.0, 250.0, 0.0);
			for (int32 i = 0; i < 3; ++i)
			{
				Chandelier(B, FVector(250.0 + i * 270.0, -150.0, L.Height - 80.0), L.Height);
				L.Lamps.Add(FVector(250.0 + i * 270.0, -150.0, L.Height - 80.0));
			}
			PottedPalm(B, FVector(L.Depth - 60.0, -L.HalfWidth + 60.0, 0.0));
			L.SignAt = FVector(L.Depth - 8.0, 0.0, 345.0);
			L.SignHeight = 34.0;
			L.SignColor = Gilt();
			break;
		}
		case EEatery::Kifozde:
		{
			L.Depth = 900.0; L.HalfWidth = 450.0; L.Height = 320.0;
			S.FloorA = FLinearColor(0.45f, 0.24f, 0.16f);
			S.FloorB = FLinearColor(0.52f, 0.30f, 0.20f);
			S.bChecker = true;
			S.Low = FLinearColor(0.30f, 0.40f, 0.36f);
			S.High = Whitewash();
			S.Rail = FLinearColor(0.20f, 0.28f, 0.24f);
			S.Ceiling = Whitewash();
			S.bVault = true;
			S.Door = FLinearColor(0.20f, 0.28f, 0.24f);
			Shell(B, L, S);
			// The serving counter across the back with the pots, the kitchen hatch behind.
			Counter(B, 700.0, -300.0, 300.0, FLinearColor(0.30f, 0.40f, 0.36f), FLinearColor(0.75f, 0.74f, 0.70f));
			Pot(B, FVector(728.0, -200.0, 108.0), 44.0, FLinearColor(0.80f, 0.80f, 0.76f));
			Pot(B, FVector(728.0, -120.0, 108.0), 40.0, FLinearColor(0.18f, 0.30f, 0.50f));
			Pot(B, FVector(728.0, 0.0, 108.0), 48.0, FLinearColor(0.80f, 0.80f, 0.76f));
			Pot(B, FVector(728.0, 100.0, 108.0), 36.0, FLinearColor(0.60f, 0.15f, 0.10f));
			B.Add(Cube, FLinearColor(0.08f, 0.06f, 0.05f), FVector(L.Depth - 2.0, 160.0, 150.0), FVector(4.0, 160.0, 80.0));
			B.Add(Cube, FLinearColor(0.90f, 0.90f, 0.86f), FVector(L.Depth - 4.0, -160.0, 180.0), FVector(3.0, 120.0, 70.0));
			L.Staff = { FVector(790.0, -40.0, 0.0), 180.0 };
			L.bSecondStaff = true;
			L.SecondStaff = { FVector(800.0, 200.0, 0.0), 180.0 };
			L.OrderAt = FVector(620.0, -40.0, 0.0);
			LongTable(B, L, FVector2D(260.0, -250.0), 300.0, Oilcloth());
			LongTable(B, L, FVector2D(260.0, 230.0), 300.0, FLinearColor(0.66f, 0.56f, 0.52f));
			LongTable(B, L, FVector2D(530.0, -250.0), 160.0, Oilcloth());
			Lamp(FVector(250.0, -250.0, 230.0));
			Lamp(FVector(250.0, 230.0, 230.0));
			Lamp(FVector(650.0, 0.0, 240.0));
			L.SignAt = FVector(L.Depth - 8.0, -160.0, 180.0);
			L.SignHeight = 18.0;
			L.SignColor = FLinearColor(0.10f, 0.08f, 0.06f);
			break;
		}
		case EEatery::Tejcsarnok:
		{
			L.Depth = 700.0; L.HalfWidth = 400.0; L.Height = 330.0;
			S.bChecker = true;
			S.FloorA = WhiteTile();
			S.FloorB = FLinearColor(0.20f, 0.30f, 0.45f);
			S.Low = WhiteTile();
			S.High = FLinearColor(0.88f, 0.89f, 0.86f);
			S.LowHeight = 170.0;
			S.Rail = FLinearColor(0.20f, 0.30f, 0.45f);
			S.Ceiling = FLinearColor(0.92f, 0.92f, 0.90f);
			S.Door = FLinearColor(0.20f, 0.30f, 0.45f);
			Shell(B, L, S);
			// The marble counter across the back with churns, cans and a basket of kifli.
			Counter(B, 460.0, -330.0, 330.0, WhiteTile(), Marble());
			for (int32 i = 0; i < 4; ++i)
			{
				Churn(B, FVector(580.0, -300.0 + i * 70.0, 0.0));
			}
			for (int32 i = 0; i < 6; ++i)
			{
				B.Add(Cylinder, Steel(), FVector(495.0, -250.0 + i * 30.0, 118.0), FVector(14.0, 14.0, 20.0));
			}
			B.Add(Cylinder, FLinearColor(0.60f, 0.45f, 0.25f), FVector(495.0, 120.0, 116.0), FVector(50.0, 50.0, 16.0));
			for (int32 i = 0; i < 8; ++i)
			{
				B.Add(Cube, FLinearColor(0.85f, 0.62f, 0.30f), FVector(495.0 + 12.0 * (i % 3 - 1), 120.0 + 12.0 * (i % 2 ? 1 : -1), 126.0), FVector(16.0, 7.0, 6.0), 0.0, i * 40.0, 0.0);
			}
			// Cheese and butter under a glass bell.
			B.Add(Cylinder, FLinearColor(0.92f, 0.82f, 0.40f), FVector(495.0, 230.0, 116.0), FVector(30.0, 30.0, 14.0));
			B.Add(Sphere, FLinearColor(0.75f, 0.82f, 0.85f), FVector(495.0, 230.0, 124.0), FVector(42.0, 42.0, 36.0));
			L.Staff = { FVector(560.0, 60.0, 0.0), 180.0 };
			L.OrderAt = FVector(390.0, 60.0, 0.0);
			// A shelf along the left wall to stand at, and two little tables.
			B.Add(Cube, Marble(), FVector(250.0, -L.HalfWidth + 20.0, 110.0), FVector(360.0, 40.0, 4.0), 0.0, 0.0, 0.0, true);
			for (int32 i = 0; i < 3; ++i)
			{
				L.Standers.Add({ FVector(130.0 + i * 120.0, -L.HalfWidth + 75.0, 0.0), -90.0 });
			}
			MarbleTable(B, L, FVector2D(200.0, 180.0), 2, Random, 90.0);
			MarbleTable(B, L, FVector2D(330.0, 300.0), 2, Random, 0.0);
			Lamp(FVector(200.0, 0.0, 250.0));
			Lamp(FVector(450.0, 0.0, 250.0));
			L.SignAt = FVector(L.Depth - 8.0, 0.0, 265.0);
			L.SignHeight = 20.0;
			L.SignColor = FLinearColor(0.20f, 0.30f, 0.45f);
			break;
		}
		case EEatery::Fuvaros:
		{
			L.Depth = 1000.0; L.HalfWidth = 560.0; L.Height = 340.0;
			S.FloorA = FLinearColor(0.15f, 0.09f, 0.05f);
			S.FloorB = FLinearColor(0.20f, 0.12f, 0.06f);
			S.High = FLinearColor(0.70f, 0.62f, 0.48f);
			Shell(B, L, S);
			for (int32 i = 0; i < 4; ++i)
			{
				B.Add(Cube, DarkPanel(), FVector(130.0 + i * 240.0, 0.0, L.Height - 12.0), FVector(24.0, 2.0 * L.HalfWidth, 24.0));
			}
			// The counter along the right wall with a barrel on it, bottles behind.
			Counter(B, 300.0, L.HalfWidth - 200.0, L.HalfWidth - 20.0, DarkPanel(), Walnut());
			B.Add(Cube, DarkPanel(), FVector(560.0, L.HalfWidth - 110.0, 51.0), FVector(520.0, 60.0, 102.0), 0.0, 0.0, 0.0, true);
			B.Add(Cube, Walnut(), FVector(560.0, L.HalfWidth - 110.0, 105.0), FVector(528.0, 68.0, 6.0));
			LyingBarrel(B, FVector(560.0, L.HalfWidth - 110.0, 140.0), 90.0, 52.0);
			BottleShelves(B, L.Depth, L.HalfWidth - 300.0, L.HalfWidth - 20.0, Random);
			L.Staff = { FVector(650.0, L.HalfWidth - 40.0, 0.0), -90.0 };
			L.OrderAt = FVector(650.0, L.HalfWidth - 200.0, 0.0);
			// Long tables for the carters.
			LongTable(B, L, FVector2D(250.0, -300.0), 330.0, FLinearColor(0.36f, 0.22f, 0.12f));
			LongTable(B, L, FVector2D(650.0, -300.0), 330.0, FLinearColor(0.36f, 0.22f, 0.12f));
			LongTable(B, L, FVector2D(250.0, 120.0), 240.0, FLinearColor(0.36f, 0.22f, 0.12f));
			// Harness, whips and horseshoes on the left wall; a stove in the corner.
			for (int32 i = 0; i < 3; ++i)
			{
				const double X = 200.0 + i * 280.0;
				B.Add(Cylinder, FLinearColor(0.12f, 0.07f, 0.03f), FVector(X, -L.HalfWidth + 8.0, 200.0), FVector(70.0, 70.0, 10.0), 90.0, 90.0, 0.0);
				B.Add(Cylinder, MusicBarModel::Brass(), FVector(X, -L.HalfWidth + 10.0, 230.0), FVector(10.0, 10.0, 6.0), 90.0, 90.0, 0.0);
				B.Add(Cylinder, FLinearColor(0.08f, 0.06f, 0.04f), FVector(X + 110.0, -L.HalfWidth + 8.0, 190.0), FVector(2.0, 2.0, 140.0), 0.0, 0.0, 25.0);
				B.Add(Cylinder, Steel() * 0.6f, FVector(X + 60.0, -L.HalfWidth + 6.0, 250.0), FVector(16.0, 16.0, 3.0), 90.0, 90.0, 0.0);
			}
			B.Add(Cube, FLinearColor(0.10f, 0.26f, 0.18f), FVector(70.0, L.HalfWidth - 60.0, 95.0), FVector(90.0, 90.0, 190.0), 0.0, 0.0, 0.0, true);
			L.bWaiter = true;
			L.WaiterFrom = FVector(800.0, L.HalfWidth - 220.0, 0.0);
			Lamp(FVector(250.0, -300.0, 240.0));
			Lamp(FVector(650.0, -300.0, 240.0));
			Lamp(FVector(250.0, 120.0, 240.0));
			Lamp(FVector(560.0, 250.0, 240.0));
			L.SignAt = FVector(L.Depth - 8.0, -200.0, 250.0);
			L.SignHeight = 26.0;
			L.SignColor = FLinearColor(0.20f, 0.10f, 0.04f);
			break;
		}
		case EEatery::Borozo:
		{
			L.Depth = 1000.0; L.HalfWidth = 450.0; L.Height = 310.0;
			S.FloorA = FLinearColor(0.30f, 0.26f, 0.22f);
			S.FloorB = FLinearColor(0.36f, 0.31f, 0.26f);
			S.bChecker = true;
			S.Low = Brick();
			S.High = FLinearColor(0.66f, 0.60f, 0.52f);
			S.Rail = Brick() * 0.7f;
			S.LowHeight = 80.0;
			S.Ceiling = FLinearColor(0.62f, 0.56f, 0.48f);
			S.bVault = true;
			S.Door = Oak();
			Shell(B, L, S);
			// Big casks along both walls at the back.
			for (int32 i = 0; i < 3; ++i)
			{
				for (int32 Side = -1; Side <= 1; Side += 2)
				{
					LyingBarrel(B, FVector(600.0 + i * 140.0, Side * (L.HalfWidth - 70.0), 70.0), 90.0 * Side, 110.0);
				}
			}
			// The host's little counter by the door, glasses and a jug.
			Counter(B, 180.0, L.HalfWidth - 170.0, L.HalfWidth - 20.0, Oak(), FLinearColor(0.36f, 0.22f, 0.12f));
			for (int32 i = 0; i < 6; ++i)
			{
				B.Add(Cylinder, FLinearColor(0.70f, 0.72f, 0.66f), FVector(205.0, L.HalfWidth - 150.0 + i * 20.0, 115.0), FVector(6.0, 6.0, 12.0));
			}
			L.Staff = { FVector(250.0, L.HalfWidth - 80.0, 0.0), 180.0 };
			L.OrderAt = FVector(120.0, L.HalfWidth - 90.0, 0.0);
			const FVector2D Tables[5] = { FVector2D(250.0, -200.0), FVector2D(420.0, 60.0), FVector2D(520.0, -220.0), FVector2D(700.0, 0.0), FVector2D(860.0, -160.0) };
			for (const FVector2D& T : Tables)
			{
				BarrelTable(B, L, T, Random);
			}
			Lamp(FVector(250.0, -100.0, 220.0));
			Lamp(FVector(520.0, -100.0, 230.0));
			Lamp(FVector(800.0, -60.0, 230.0));
			L.SignAt = FVector(L.Depth - 8.0, 0.0, 180.0);
			L.SignHeight = 26.0;
			L.SignColor = FLinearColor(0.85f, 0.78f, 0.60f);
			break;
		}
		case EEatery::Kispipa:
		{
			L.Depth = 1000.0; L.HalfWidth = 520.0; L.Height = 340.0;
			S.High = FLinearColor(0.74f, 0.66f, 0.50f);
			Shell(B, L, S);
			// The piano by the back wall, its keys to the room; the counter along the right.
			L.bPiano = true;
			Piano(B, FVector(L.Depth - 40.0, -280.0, 0.0), 0.0);
			L.Pianist = { FVector(L.Depth - 125.0, -280.0, 0.0), 0.0 };
			Counter(B, 300.0, L.HalfWidth - 180.0, L.HalfWidth - 20.0, DarkPanel(), Walnut());
			B.Add(Cube, DarkPanel(), FVector(520.0, L.HalfWidth - 100.0, 51.0), FVector(440.0, 60.0, 102.0), 0.0, 0.0, 0.0, true);
			B.Add(Cube, Walnut(), FVector(520.0, L.HalfWidth - 100.0, 105.0), FVector(448.0, 68.0, 6.0));
			BottleShelves(B, L.Depth, L.HalfWidth - 300.0, L.HalfWidth - 20.0, Random);
			L.Staff = { FVector(600.0, L.HalfWidth - 35.0, 0.0), -90.0 };
			L.OrderAt = FVector(600.0, L.HalfWidth - 190.0, 0.0);
			// The little pipe the place is named for, over the counter.
			B.Add(Cylinder, Walnut(), FVector(420.0, L.HalfWidth - 4.0, 260.0), FVector(26.0, 26.0, 30.0));
			B.Add(Cylinder, Walnut(), FVector(370.0, L.HalfWidth - 4.0, 250.0), FVector(8.0, 8.0, 90.0), 0.0, 0.0, 80.0);
			const FVector2D Tables[6] = { FVector2D(160.0, -330.0), FVector2D(160.0, -60.0), FVector2D(160.0, 210.0), FVector2D(420.0, -330.0), FVector2D(420.0, -60.0), FVector2D(680.0, -40.0) };
			for (const FVector2D& T : Tables)
			{
				ClothTable(B, L, T, Cloth(), FLinearColor(0.55f, 0.08f, 0.07f), true, Random);
			}
			Pictures(B, L, -1, 100.0, 800.0, 5, Gilt());
			L.bWaiter = true;
			L.WaiterFrom = FVector(760.0, L.HalfWidth - 200.0, 0.0);
			Lamp(FVector(160.0, -200.0, 240.0));
			Lamp(FVector(160.0, 200.0, 240.0));
			Lamp(FVector(420.0, -200.0, 240.0));
			Lamp(FVector(780.0, -280.0, 250.0));
			L.SignAt = FVector(L.Depth - 8.0, 120.0, 250.0);
			L.SignHeight = 30.0;
			L.SignColor = FLinearColor(0.20f, 0.10f, 0.04f);
			break;
		}
		case EEatery::Bufe:
		{
			L.Depth = 800.0; L.HalfWidth = 450.0; L.Height = 360.0;
			S.bChecker = true;
			S.FloorA = FLinearColor(0.75f, 0.72f, 0.64f);
			S.FloorB = FLinearColor(0.50f, 0.18f, 0.12f);
			S.Low = FLinearColor(0.18f, 0.32f, 0.26f);
			S.High = FLinearColor(0.84f, 0.80f, 0.68f);
			S.LowHeight = 150.0;
			S.Rail = Brass();
			S.Ceiling = FLinearColor(0.88f, 0.86f, 0.80f);
			S.Door = FLinearColor(0.18f, 0.32f, 0.26f);
			Shell(B, L, S);
			// The long counter across the back, a glass case of sandwiches and the beer tap.
			Counter(B, 520.0, -400.0, 400.0, FLinearColor(0.18f, 0.32f, 0.26f), Marble());
			const TArray<FLinearColor> Bites = { FLinearColor(0.85f, 0.60f, 0.55f), FLinearColor(0.85f, 0.70f, 0.40f), FLinearColor(0.80f, 0.40f, 0.30f),
				FLinearColor(0.90f, 0.88f, 0.70f), FLinearColor(0.55f, 0.40f, 0.30f) };
			GlassCase(B, 548.0, -150.0, 420.0, Bites, Random);
			B.Add(Cylinder, Brass(), FVector(560.0, 200.0, 135.0), FVector(10.0, 10.0, 50.0));
			B.Add(Cylinder, Brass(), FVector(550.0, 200.0, 160.0), FVector(5.0, 5.0, 24.0), 90.0, 0.0, 0.0);
			B.Add(Cube, Steel(), FVector(560.0, 300.0, 140.0), FVector(50.0, 40.0, 60.0));
			BottleShelves(B, L.Depth, -400.0, 400.0, Random, 2);
			L.Staff = { FVector(640.0, 120.0, 0.0), 180.0 };
			L.bSecondStaff = true;
			L.SecondStaff = { FVector(640.0, -200.0, 0.0), 180.0 };
			L.OrderAt = FVector(460.0, 120.0, 0.0);
			// High tables, and travellers at the counter.
			HighTable(B, L, FVector2D(170.0, -250.0));
			HighTable(B, L, FVector2D(170.0, 250.0));
			HighTable(B, L, FVector2D(330.0, -100.0));
			HighTable(B, L, FVector2D(330.0, 230.0));
			L.Standers.Add({ FVector(470.0, -300.0, 0.0), 0.0 });
			L.Standers.Add({ FVector(470.0, -60.0, 0.0), 0.0 });
			L.Standers.Add({ FVector(470.0, 300.0, 0.0), 0.0 });
			// A railway timetable and a clock.
			B.Add(Cube, FLinearColor(0.92f, 0.90f, 0.80f), FVector(250.0, -L.HalfWidth + 4.0, 220.0), FVector(120.0, 3.0, 80.0));
			B.Add(Cylinder, FLinearColor(0.92f, 0.92f, 0.88f), FVector(L.Depth - 4.0, 0.0, 300.0), FVector(50.0, 50.0, 4.0), 90.0, 0.0, 0.0);
			for (int32 i = 0; i < 3; ++i)
			{
				Lamp(FVector(250.0, -280.0 + i * 280.0, 280.0));
			}
			L.SignAt = FVector(L.Depth - 8.0, 0.0, 250.0);
			L.SignHeight = 30.0;
			L.SignColor = Gilt();
			break;
		}
		case EEatery::Koser:
		{
			L.Depth = 900.0; L.HalfWidth = 450.0; L.Height = 330.0;
			S.High = FLinearColor(0.80f, 0.78f, 0.70f);
			S.Low = FLinearColor(0.22f, 0.18f, 0.24f);
			Shell(B, L, S);
			// The counter by the back wall, a dresser with the candlestick.
			Counter(B, 640.0, 100.0, L.HalfWidth - 20.0, FLinearColor(0.22f, 0.18f, 0.24f), Walnut());
			Pot(B, FVector(668.0, 200.0, 108.0), 44.0, FLinearColor(0.50f, 0.30f, 0.16f));
			B.Add(Cube, FLinearColor(0.86f, 0.70f, 0.40f), FVector(668.0, 320.0, 114.0), FVector(40.0, 60.0, 12.0));
			B.Add(Cube, Walnut(), FVector(L.Depth - 30.0, -250.0, 60.0), FVector(50.0, 200.0, 120.0), 0.0, 0.0, 0.0, true);
			B.Add(Cube, Walnut(), FVector(L.Depth - 30.0, -250.0, 122.0), FVector(56.0, 208.0, 4.0));
			B.Add(Cylinder, Brass(), FVector(L.Depth - 30.0, -250.0, 140.0), FVector(5.0, 5.0, 36.0));
			B.Add(Cube, Brass(), FVector(L.Depth - 30.0, -250.0, 158.0), FVector(3.0, 60.0, 3.0));
			for (int32 i = -3; i <= 3; ++i)
			{
				B.Add(Cylinder, Brass(), FVector(L.Depth - 30.0, -250.0 + i * 10.0, 164.0), FVector(3.0, 3.0, 12.0));
				B.Add(Cylinder, FLinearColor(0.92f, 0.90f, 0.84f), FVector(L.Depth - 30.0, -250.0 + i * 10.0, 174.0), FVector(2.0, 2.0, 10.0));
			}
			L.Staff = { FVector(720.0, 260.0, 0.0), 180.0 };
			L.OrderAt = FVector(570.0, 260.0, 0.0);
			const FVector2D Tables[5] = { FVector2D(170.0, -280.0), FVector2D(170.0, 0.0), FVector2D(170.0, 280.0), FVector2D(420.0, -280.0), FVector2D(420.0, 40.0) };
			for (const FVector2D& T : Tables)
			{
				ClothTable(B, L, T, Cloth(), FLinearColor(0.20f, 0.25f, 0.55f), false, Random);
			}
			Pictures(B, L, -1, 100.0, 600.0, 3, Walnut());
			L.bWaiter = true;
			L.WaiterFrom = FVector(600.0, 100.0, 0.0);
			Lamp(FVector(170.0, -150.0, 240.0));
			Lamp(FVector(170.0, 150.0, 240.0));
			Lamp(FVector(420.0, -120.0, 240.0));
			Lamp(FVector(680.0, 200.0, 250.0));
			L.SignAt = FVector(L.Depth - 8.0, 150.0, 250.0);
			L.SignHeight = 22.0;
			L.SignColor = FLinearColor(0.20f, 0.25f, 0.55f);
			break;
		}
		default:
		{
			// The Nemzeti's restaurant.
			L.Depth = 1600.0; L.HalfWidth = 900.0; L.Height = 600.0;
			S.FloorA = FLinearColor(0.24f, 0.13f, 0.06f);
			S.FloorB = FLinearColor(0.30f, 0.17f, 0.08f);
			S.Low = FLinearColor(0.28f, 0.14f, 0.07f);
			S.High = FLinearColor(0.88f, 0.82f, 0.66f);
			S.LowHeight = 140.0;
			S.Rail = Gilt();
			S.Ceiling = FLinearColor(0.92f, 0.88f, 0.76f);
			S.Door = FLinearColor(0.28f, 0.14f, 0.07f);
			Shell(B, L, S);
			// Gilt pilasters and mirrors along the walls.
			for (int32 i = 0; i < 6; ++i)
			{
				const double X = 150.0 + i * 260.0;
				for (int32 Side = -1; Side <= 1; Side += 2)
				{
					B.Add(Cube, FLinearColor(0.84f, 0.76f, 0.58f), FVector(X, Side * (L.HalfWidth - 15.0), L.Height * 0.5), FVector(40.0, 30.0, L.Height), 0.0, 0.0, 0.0, true);
					B.Add(Cube, Gilt(), FVector(X, Side * (L.HalfWidth - 31.0), L.Height - 40.0), FVector(50.0, 4.0, 40.0));
					if (i < 5)
					{
						Mirror(B, L, Side, X + 130.0, 300.0, 140.0, 220.0);
					}
				}
			}
			// The head waiter's desk by the door, the sideboard at the back.
			B.Add(Cube, Walnut(), FVector(150.0, 250.0, 55.0), FVector(60.0, 120.0, 110.0), 0.0, 0.0, 0.0, true);
			B.Add(Cube, FLinearColor(0.10f, 0.05f, 0.02f), FVector(150.0, 250.0, 112.0), FVector(50.0, 40.0, 4.0), 0.0, 0.0, 0.0);
			L.Staff = { FVector(190.0, 250.0, 0.0), 180.0 };
			L.OrderAt = FVector(90.0, 200.0, 0.0);
			B.Add(Cube, Walnut(), FVector(L.Depth - 40.0, 0.0, 60.0), FVector(70.0, 600.0, 120.0), 0.0, 0.0, 0.0, true);
			B.Add(Cube, Marble(), FVector(L.Depth - 40.0, 0.0, 122.0), FVector(76.0, 606.0, 4.0));
			for (int32 i = 0; i < 8; ++i)
			{
				B.Add(Cylinder, Silver(), FVector(L.Depth - 40.0, -250.0 + i * 70.0, 140.0), FVector(16.0, 16.0, 30.0));
			}
			// A champagne cooler on a stand.
			B.Add(Cylinder, Silver(), FVector(L.Depth - 140.0, 380.0, 50.0), FVector(6.0, 6.0, 100.0));
			B.Add(Cylinder, Silver(), FVector(L.Depth - 140.0, 380.0, 110.0), FVector(30.0, 30.0, 28.0));
			B.Add(Cylinder, FLinearColor(0.06f, 0.18f, 0.08f), FVector(L.Depth - 140.0, 380.0, 135.0), FVector(8.0, 8.0, 30.0), 20.0, 0.0, 0.0);
			for (int32 Row = 0; Row < 4; ++Row)
			{
				for (int32 Col = 0; Col < 4; ++Col)
				{
					if (Row == 0 && Col >= 2)
					{
						continue;
					}
					ClothTable(B, L, FVector2D(330.0 + Row * 290.0, -560.0 + Col * 320.0 + (Row % 2) * 60.0), Cloth(), Cloth() * 0.92f, false, Random, true);
				}
			}
			PottedPalm(B, FVector(L.Depth - 80.0, -L.HalfWidth + 90.0, 0.0));
			PottedPalm(B, FVector(L.Depth - 80.0, L.HalfWidth - 90.0, 0.0));
			PottedPalm(B, FVector(80.0, -L.HalfWidth + 90.0, 0.0));
			L.bWaiter = true;
			L.WaiterFrom = FVector(L.Depth - 140.0, -300.0, 0.0);
			for (int32 i = 0; i < 2; ++i)
			{
				for (int32 j = 0; j < 2; ++j)
				{
					const FVector At(480.0 + i * 600.0, -350.0 + j * 700.0, L.Height - 120.0);
					Chandelier(B, At, L.Height);
					L.Lamps.Add(At);
				}
			}
			L.SignAt = FVector(L.Depth - 8.0, 0.0, 480.0);
			L.SignHeight = 40.0;
			L.SignColor = Gilt();
			break;
		}
		}
	}

	/** The people behind the counters. */
	inline FOutfit StaffLook(EEatery Kind, bool bSecond)
	{
		using PersonKit::EHat;
		using PersonKit::ETop;
		FOutfit O;
		O.Hat = EHat::None;
		O.bApron = true;
		switch (Kind)
		{
		case EEatery::Hauer:
		case EEatery::Tejcsarnok:
			// A shop girl in black with a white apron (the milk woman older, in blue).
			O.bWoman = true;
			O.Top = ETop::Dress;
			O.Coat = Kind == EEatery::Hauer ? FLinearColor(0.05f, 0.05f, 0.06f) : FLinearColor(0.18f, 0.26f, 0.42f);
			O.Hair = Kind == EEatery::Hauer ? FLinearColor(0.35f, 0.20f, 0.08f) : FLinearColor(0.45f, 0.42f, 0.40f);
			O.bAged = Kind == EEatery::Tejcsarnok;
			O.Height = 162.f;
			O.bTie = false;
			break;
		case EEatery::Kifozde:
			// The cook woman and her help, aprons over print dresses.
			O.bWoman = true;
			O.Top = ETop::Dress;
			O.Coat = bSecond ? FLinearColor(0.40f, 0.30f, 0.32f) : FLinearColor(0.30f, 0.28f, 0.40f);
			O.Hat = bSecond ? EHat::None : EHat::Hood;
			O.HatColor = FLinearColor(0.85f, 0.85f, 0.82f);
			O.Hair = FLinearColor(0.20f, 0.12f, 0.06f);
			O.Belly = bSecond ? 0.f : 0.6f;
			O.bAged = !bSecond;
			O.Height = 160.f;
			O.bTie = false;
			break;
		case EEatery::Koser:
			// The owner: beard, black waistcoat.
			O.Top = ETop::Waistcoat;
			O.Hat = EHat::Bowler;
			O.Beard = PersonKit::EBeard::Full;
			O.bAged = true;
			O.Waistcoat = FLinearColor(0.04f, 0.04f, 0.05f);
			O.Height = 172.f;
			break;
		case EEatery::Nemzeti:
			// The head waiter in tails.
			O.bApron = false;
			O.Top = ETop::Jacket;
			O.Coat = FLinearColor(0.02f, 0.02f, 0.025f);
			O.bMoustache = true;
			O.MoustacheKind = PersonKit::EMoustache::Handlebar;
			O.Height = 182.f;
			break;
		default:
			// Innkeepers and the buffet man: shirt sleeves, waistcoat, apron, a moustache.
			O.Top = ETop::Waistcoat;
			O.Waistcoat = bSecond ? FLinearColor(0.10f, 0.12f, 0.10f) : FLinearColor(0.18f, 0.10f, 0.05f);
			O.bMoustache = true;
			O.Belly = Kind == EEatery::Bufe && bSecond ? 0.f : 0.5f;
			O.Height = 174.f;
			break;
		}
		return O;
	}

	/** The pianist at the Kispipa: slim, a dark suit, hair combed back. */
	inline FOutfit PianistLook()
	{
		FOutfit O;
		O.Hat = PersonKit::EHat::None;
		O.Top = PersonKit::ETop::Jacket;
		O.Coat = FLinearColor(0.06f, 0.06f, 0.08f);
		O.Build = 0.9f;
		O.Height = 172.f;
		O.bSeated = true;
		return O;
	}
}
