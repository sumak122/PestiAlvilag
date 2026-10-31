// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ChaseModel.h"
#include "MarketHallModel.h"
#include "MarketModel.h"

/**
 * The market inside the Rákóczi tér market hall, in the hall's own frame
 * (see MarketHallModel.h): the stalls in the booths, more stalls standing
 * free in the naves (back to back down the middle where nobody needs the
 * way, along the sides where the pickpocket runs, so that he runs through
 * a lane of them), who sells and who shops where, and the lamps hanging
 * from the roof. AMarketHall hands the stalls and people to an AFleaMarket,
 * which brings them to life like the Teleki tér market.
 */
namespace HallMarket
{
	struct FStall
	{
		FVector At;
		double Yaw;
		MarketModel::EBuild Build;
		MarketModel::EGoods Goods;
		bool bSeller;
	};

	struct FShopper
	{
		FVector At;
		double Yaw;
	};

	/** A lamp hanging on a rod from Top down to At (the middle of its globe). */
	struct FLamp
	{
		FVector At;
		double Top;
	};

	// Free stalls keep this far from the pickpocket's way and the things in it (cm).
	constexpr double RouteClear = 330.0;
	constexpr double SpotClear = 300.0;
	constexpr double MissionClear = 380.0;
	// And this far from the crossing's middle, which stays open.
	constexpr double CrossingClear = MarketHall::InnerHalf + 150.0;
	constexpr double FreePitch = 400.0;
	constexpr double FreeKeep = 0.8;
	// Along the naves' sides, the free stalls stand this far out from the middle.
	constexpr double SideRow = MarketHall::NaveHalf - 135.0;
	// Down the middle, back to back, this far either side of it.
	constexpr double MiddleRow = 110.0;
	constexpr double BoothSeller = 0.75;
	constexpr double FreeSeller = 0.7;
	constexpr double ShopperChance = 0.32;
	constexpr double ShopperOut = 150.0;
	constexpr int32 Strollers = 28;
	constexpr double LampStep = 1200.0;
	constexpr double LampHeight = 520.0;

	inline double ToSegment(const FVector2D& P, const FVector2D& A, const FVector2D& B)
	{
		const FVector2D AB = B - A;
		const double T = FMath::Clamp(FVector2D::DotProduct(P - A, AB) / FMath::Max(AB.SizeSquared(), 1.0), 0.0, 1.0);
		return FVector2D::Distance(P, A + AB * T);
	}

	/** True if P is clear of the pickpocket's way and everything standing in it. */
	inline bool OffRoute(const FVector2D& P)
	{
		const TArray<FVector2D> Route = ChaseModel::ThiefRoute();
		for (int32 i = 0; i + 1 < Route.Num(); ++i)
		{
			if (ToSegment(P, Route[i], Route[i + 1]) < RouteClear)
			{
				return false;
			}
		}
		for (const ChaseModel::FSpot& Spot : ChaseModel::RouteSpots())
		{
			if (FVector2D::Distance(P, FVector2D(Spot.At)) < SpotClear)
			{
				return false;
			}
		}
		// Where the mission puts Bözsi néni, the pickpocket and the player
		// for a retry (as in MissionZsebtolvaj.cpp).
		for (const FVector2D& Spot : { FVector2D(1300.0, 760.0), FVector2D(2350.0, 420.0), FVector2D(800.0, 350.0) })
		{
			if (FVector2D::Distance(P, Spot) < MissionClear)
			{
				return false;
			}
		}
		return true;
	}

	inline MarketModel::EGoods FreeGoods(MarketModel::FRandom& Random)
	{
		const double Kind = Random.Unit();
		return Kind < 0.55 ? MarketModel::EGoods::Produce : Kind < 0.68 ? MarketModel::EGoods::Crockery
			: Kind < 0.80 ? MarketModel::EGoods::Pots : Kind < 0.92 ? MarketModel::EGoods::Clothes : MarketModel::EGoods::Shoes;
	}

	inline void Layout(TArray<FStall>& Stalls, TArray<FShopper>& Shoppers, TArray<FLamp>& Lamps)
	{
		using namespace MarketHall;

		// The stalls in the booths: mostly vegetables and fruit, here and
		// there pots and pans or crockery (as the hall always had them).
		TArray<MarketHall::FStall> Booths;
		StallSpots(Booths);
		MarketModel::FRandom Sell(1897);
		for (int32 k = 0; k < Booths.Num(); ++k)
		{
			MarketModel::FRandom Random(static_cast<uint32>(k * 7 + 3));
			const double Kind = Random.Unit();
			const MarketModel::EGoods Goods = Kind < 0.75 ? MarketModel::EGoods::Produce : Kind < 0.88 ? MarketModel::EGoods::Crockery : MarketModel::EGoods::Pots;
			Stalls.Add({ Booths[k].At, Booths[k].Yaw, MarketModel::EBuild::Table, Goods, Sell.Unit() < BoothSeller });
		}
		const int32 FirstFree = Stalls.Num();

		// Free stalls: tables and goods spread on blankets.
		MarketModel::FRandom Random(1932);
		auto AddFree = [&](const FVector2D& At, double Yaw)
		{
			if (Random.Unit() > FreeKeep || !OffRoute(At) || FVector2D::Distance(At, FVector2D(CrossX, 0.0)) < CrossingClear)
			{
				return;
			}
			const FVector2D Jog(Random.Range(-40.0, 40.0), Random.Range(-25.0, 25.0));
			const MarketModel::EGoods Goods = FreeGoods(Random);
			const MarketModel::EBuild Build = Random.Unit() < 0.3 && Goods != MarketModel::EGoods::Clothes ? MarketModel::EBuild::Blanket : MarketModel::EBuild::Table;
			Stalls.Add({ FVector(At + Jog, 0.0), Yaw + Random.Range(-4.0, 4.0), Build, Goods, Random.Unit() < FreeSeller });
		};
		// The long nave, front to back: along both sides, facing in, and back
		// to back down the middle wherever the pickpocket does not run.
		for (double X = 650.0; X < BackFront - 650.0; X += FreePitch)
		{
			AddFree(FVector2D(X, -SideRow), 90.0);
			AddFree(FVector2D(X, SideRow), -90.0);
			AddFree(FVector2D(X, -MiddleRow), -90.0);
			AddFree(FVector2D(X, MiddleRow), 90.0);
		}
		// The nave across, likewise.
		for (double Y = InnerHalf + 350.0; Y < SideFront - 450.0; Y += FreePitch)
		{
			for (const double Sgn : { -1.0, 1.0 })
			{
				AddFree(FVector2D(CrossX - SideRow, Sgn * Y), 0.0);
				AddFree(FVector2D(CrossX + SideRow, Sgn * Y), 180.0);
				AddFree(FVector2D(CrossX - MiddleRow, Sgn * Y), 180.0);
				AddFree(FVector2D(CrossX + MiddleRow, Sgn * Y), 0.0);
			}
		}

		// Shoppers at the stalls, looking at the goods.
		MarketModel::FRandom Shop(1933);
		for (const FStall& Stall : Stalls)
		{
			if (Shop.Unit() > ShopperChance)
			{
				continue;
			}
			const double A = FMath::DegreesToRadians(Stall.Yaw);
			const FVector Forward(FMath::Cos(A), FMath::Sin(A), 0.0);
			const FVector Side(-Forward.Y, Forward.X, 0.0);
			Shoppers.Add({ Stall.At + Forward * ShopperOut + Side * Shop.Range(-80.0, 80.0), Stall.Yaw + 180.0 + Shop.Range(-25.0, 25.0) });
		}
		// And more walking the naves between the stalls, pickpocket's way or not.
		auto Free = [&](const FVector2D& P)
		{
			for (int32 i = FirstFree; i < Stalls.Num(); ++i)
			{
				if (FVector2D::Distance(P, FVector2D(Stalls[i].At)) < 190.0)
				{
					return false;
				}
			}
			for (const FShopper& Other : Shoppers)
			{
				if (FVector2D::Distance(P, FVector2D(Other.At)) < 110.0)
				{
					return false;
				}
			}
			return FVector2D::Distance(P, FVector2D(CrossX, 0.0)) > 300.0;
		};
		for (int32 Tries = 0, Made = 0; Made < Strollers && Tries < Strollers * 8; ++Tries)
		{
			// Half down the long nave, half across.
			const bool bLong = Shop.Unit() < 0.55;
			const double Along = bLong ? Shop.Range(400.0, BackFront - 400.0) : Shop.Range(-SideFront + 400.0, SideFront - 400.0);
			const double Across = Shop.Range(-SideRow + 150.0, SideRow - 150.0);
			const FVector2D P = bLong ? FVector2D(Along, Across) : FVector2D(CrossX + Across, Along);
			if (Free(P))
			{
				Shoppers.Add({ FVector(P, 0.0), Shop.Range(-180.0, 180.0) });
				++Made;
			}
		}

		// Lamps hanging from the roof: down the middle of each nave and the
		// inner aisles, a ring of four round the crossing.
		const double Aisle = (NaveHalf + InnerHalf) / 2.0;
		for (double X = 600.0; X < BackFront - 500.0; X += LampStep)
		{
			if (FMath::Abs(X - CrossX) > InnerHalf)
			{
				Lamps.Add({ FVector(X, 0.0, LampHeight), NaveEaves - 100.0 });
			}
			Lamps.Add({ FVector(X, -Aisle, LampHeight), InnerTop - 150.0 });
			Lamps.Add({ FVector(X, Aisle, LampHeight), InnerTop - 150.0 });
		}
		for (double Y = InnerHalf + 600.0; Y < SideFront - 300.0; Y += LampStep)
		{
			for (const double Sgn : { -1.0, 1.0 })
			{
				Lamps.Add({ FVector(CrossX, Sgn * Y, LampHeight), NaveEaves - 100.0 });
				Lamps.Add({ FVector(CrossX - Aisle, Sgn * Y, LampHeight), InnerTop - 150.0 });
				Lamps.Add({ FVector(CrossX + Aisle, Sgn * Y, LampHeight), InnerTop - 150.0 });
			}
		}
		for (const FVector2D& Corner : { FVector2D(-1.0, -1.0), FVector2D(-1.0, 1.0), FVector2D(1.0, -1.0), FVector2D(1.0, 1.0) })
		{
			Lamps.Add({ FVector(FVector2D(CrossX, 0.0) + Corner * 450.0, LampHeight + 80.0), CentreBand - 200.0 });
		}
	}
}
