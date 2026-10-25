// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * The Teleki tér market's stalls, sheds and goods as a list of basic shapes
 * (the engine's cube, cylinder, sphere and cone, each 100 cm across):
 * trestle tables and blankets, canvas awnings, clothes rails, shoes, pots,
 * old iron, books, furniture, vegetables and crockery. AFleaMarket puts the
 * shapes in the game.
 *
 * Header-only (like PersonKit and TramModel) so it can be built and looked
 * at outside the engine. A stall's frame: +X towards its aisle, Z up from
 * the ground.
 */
namespace MarketModel
{
	enum EShape : int32 { Cube, Cylinder, Sphere, Cone };
	enum class EBuild : uint8 { Table, Shed, Blanket };
	enum class EGoods : uint8 { Clothes, Shoes, Pots, Junk, Books, Furniture, Produce, Crockery };

	/** One shape: where its middle is, its whole size, and its turn (degrees, as FRotator). */
	struct FShape
	{
		int32 Shape;
		FLinearColor Color;
		FVector At;
		FVector Size;
		double Pitch;
		double Yaw;
		double Roll;
		bool bCollide;
	};

	/** A repeatable random series (the same stall looks the same every time). */
	struct FRandom
	{
		uint32 State;
		explicit FRandom(uint32 Seed) : State(Seed * 2654435761u + 12345u) {}
		double Unit()
		{
			State = State * 1664525u + 1013904223u;
			return (State >> 8) / 16777216.0;
		}
		double Range(double Min, double Max) { return Min + (Max - Min) * Unit(); }
		int32 Index(int32 Count) { return FMath::Min(Count - 1, static_cast<int32>(Unit() * Count)); }
	};

	inline const FLinearColor& Earth() { static const FLinearColor C(0.30f, 0.26f, 0.20f); return C; }
	inline const FLinearColor& ShedRoof() { static const FLinearColor C(0.55f, 0.55f, 0.52f); return C; }

	namespace Colors
	{
		const FLinearColor Post(0.24f, 0.17f, 0.10f);
		const FLinearColor Board(0.38f, 0.27f, 0.15f);
		const FLinearColor OldBoard(0.30f, 0.24f, 0.17f);
		const FLinearColor Iron(0.06f, 0.06f, 0.065f);
		const FLinearColor Rust(0.30f, 0.14f, 0.07f);
		const FLinearColor Copper(0.55f, 0.27f, 0.12f);
		const FLinearColor Enamel(0.80f, 0.80f, 0.74f);
		const FLinearColor Leather(0.12f, 0.07f, 0.04f);
		const FLinearColor Paper(0.72f, 0.66f, 0.52f);
		const FLinearColor Crate(0.48f, 0.36f, 0.20f);
		const FLinearColor Wicker(0.52f, 0.38f, 0.18f);
		// Faded cloth for awnings, blankets and clothes.
		const FLinearColor Cloths[] = {
			FLinearColor(0.62f, 0.58f, 0.48f), FLinearColor(0.35f, 0.12f, 0.10f), FLinearColor(0.16f, 0.20f, 0.28f),
			FLinearColor(0.22f, 0.26f, 0.16f), FLinearColor(0.45f, 0.38f, 0.26f), FLinearColor(0.12f, 0.11f, 0.10f),
			FLinearColor(0.50f, 0.30f, 0.22f), FLinearColor(0.28f, 0.28f, 0.30f) };
		const FLinearColor Produce[] = {
			FLinearColor(0.55f, 0.08f, 0.05f),   // apples
			FLinearColor(0.40f, 0.30f, 0.15f),   // potatoes
			FLinearColor(0.30f, 0.45f, 0.18f),   // cabbages
			FLinearColor(0.80f, 0.40f, 0.06f),   // pumpkins and carrots
			FLinearColor(0.75f, 0.68f, 0.45f) }; // onions
		const FLinearColor BookCovers[] = {
			FLinearColor(0.30f, 0.08f, 0.06f), FLinearColor(0.08f, 0.12f, 0.20f), FLinearColor(0.10f, 0.16f, 0.08f),
			FLinearColor(0.42f, 0.32f, 0.18f), FLinearColor(0.08f, 0.07f, 0.06f) };
		const FLinearColor Metals[] = { Iron, Copper, Enamel, Rust };
		const FLinearColor Glazes[] = {
			Enamel, FLinearColor(0.70f, 0.72f, 0.78f), FLinearColor(0.55f, 0.35f, 0.20f), FLinearColor(0.20f, 0.30f, 0.45f) };
	}

	// A stall's table: depth (X), width (Y), top height (cm).
	constexpr double TableDepth = 110.0;
	constexpr double TableWidth = 280.0;
	constexpr double TableTop = 80.0;
	constexpr double AwningChance = 0.5;
	// A shed: eaves and apex height, overhang (cm).
	constexpr double ShedEaves = 300.0;
	constexpr double ShedApex = 560.0;
	constexpr double ShedOverhang = 40.0;
	// The seller stands this far behind the stall's middle.
	constexpr double SellerBack = 100.0;

	/** Shapes in a frame on the ground at Origin turned by Yaw: adds them to Out in world space. */
	struct FBuilder
	{
		TArray<FShape>& Out;
		FVector Origin;
		double FrameYaw;

		void Add(int32 Shape, const FLinearColor& Color, const FVector& At, const FVector& Size,
			double Pitch = 0.0, double Yaw = 0.0, double Roll = 0.0, bool bCollide = false)
		{
			const double A = FMath::DegreesToRadians(FrameYaw);
			const FVector World(Origin.X + At.X * FMath::Cos(A) - At.Y * FMath::Sin(A),
				Origin.Y + At.X * FMath::Sin(A) + At.Y * FMath::Cos(A), Origin.Z + At.Z);
			// The frame only turns about Z, so its yaw adds to the shape's own.
			Out.Add({ Shape, Color, World, Size, Pitch, Yaw + FrameYaw, Roll, bCollide });
		}
	};

	template <typename T, int32 N>
	const T& Pick(const T (&Items)[N], FRandom& Random)
	{
		return Items[Random.Index(N)];
	}

	/** A shed's posts and eaves beams (the roof is a pyramid AFleaMarket makes itself). */
	inline void BuildShed(TArray<FShape>& Out, const FVector& Middle, double Yaw, double Width, double Depth)
	{
		using namespace Colors;
		FBuilder B{ Out, Middle, Yaw };
		const double HX = Depth * 0.5;
		const double HY = Width * 0.5;
		for (const double X : { -HX, 0.0, HX })
		{
			for (const double Y : { -HY, 0.0, HY })
			{
				if (X != 0.0 || Y != 0.0)
				{
					B.Add(Cube, Post, FVector(X, Y, ShedEaves * 0.5), FVector(16.0, 16.0, ShedEaves), 0.0, 0.0, 0.0, true);
				}
			}
		}
		for (const double Y : { -HY, HY })
		{
			B.Add(Cube, Post, FVector(0.0, Y, ShedEaves - 8.0), FVector(Depth, 14.0, 16.0));
		}
		for (const double X : { -HX, HX })
		{
			B.Add(Cube, Post, FVector(X, 0.0, ShedEaves - 8.0), FVector(14.0, Width, 16.0));
		}
	}

	/** A stall facing its +X with its goods; returns how far behind it the seller stands. */
	inline double BuildStall(TArray<FShape>& Out, const FVector& Middle, double Yaw, EBuild Build, EGoods Goods, FRandom& Random)
	{
		using namespace Colors;
		FBuilder B{ Out, Middle, Yaw };
		const bool bBlanket = Build == EBuild::Blanket;
		double Top = 2.0;
		if (bBlanket)
		{
			// Goods spread on a blanket on the ground.
			B.Add(Cube, Pick(Cloths, Random), FVector(0.0, 0.0, 1.0), FVector(150.0, 230.0, 2.0), 0.0, Random.Range(-6.0, 6.0));
		}
		else
		{
			// A trestle table of old boards.
			Top = TableTop;
			B.Add(Cube, Random.Unit() < 0.5 ? Board : OldBoard, FVector(0.0, 0.0, TableTop - 2.0), FVector(TableDepth, TableWidth, 4.0),
				0.0, 0.0, 0.0, true);
			for (const double X : { -TableDepth * 0.5 + 8.0, TableDepth * 0.5 - 8.0 })
			{
				for (const double Y : { -TableWidth * 0.5 + 15.0, TableWidth * 0.5 - 15.0 })
				{
					B.Add(Cube, Post, FVector(X, Y, (TableTop - 4.0) * 0.5), FVector(6.0, 6.0, TableTop - 4.0));
				}
			}
			// Some in the open have a canvas awning on two poles behind.
			if (Build == EBuild::Table && Random.Unit() < AwningChance)
			{
				for (const double Y : { -TableWidth * 0.5, TableWidth * 0.5 })
				{
					B.Add(Cylinder, Post, FVector(-TableDepth * 0.5 - 50.0, Y, 115.0), FVector(6.0, 6.0, 230.0));
				}
				B.Add(Cube, Pick(Cloths, Random), FVector(-TableDepth * 0.5 + 20.0, 0.0, 212.0), FVector(170.0, TableWidth + 30.0, 2.0), -14.0);
			}
		}

		const double HalfDepth = bBlanket ? 60.0 : TableDepth * 0.5 - 12.0;
		const double HalfWidth = bBlanket ? 100.0 : TableWidth * 0.5 - 15.0;
		auto Spot = [&](double Lift)
		{
			const double X = Random.Range(-HalfDepth, HalfDepth);
			const double Y = Random.Range(-HalfWidth, HalfWidth);
			return FVector(X, Y, Top + Lift);
		};
		auto Spin = [&]() { return Random.Range(-40.0, 40.0); };

		switch (Goods)
		{
		case EGoods::Clothes:
		{
			// Folded stacks, and coats hanging on a rail behind.
			for (int32 i = 0; i < 6; ++i)
			{
				const double Height = Random.Range(6.0, 16.0);
				const FVector At = Spot(Height * 0.5);
				B.Add(Cube, Pick(Cloths, Random), At, FVector(35.0, 30.0, Height), 0.0, Spin());
			}
			// The rail stands at the stall's side end, the coats hanging along it.
			const double RailY = bBlanket ? 145.0 : TableWidth * 0.5 + 40.0;
			for (const double X : { -60.0, 60.0 })
			{
				B.Add(Cylinder, Post, FVector(X, RailY, 85.0), FVector(4.0, 4.0, 170.0));
			}
			B.Add(Cube, Iron, FVector(0.0, RailY, 168.0), FVector(124.0, 3.0, 3.0));
			const int32 Coats = 3 + Random.Index(3);
			for (int32 i = 0; i < Coats; ++i)
			{
				const double X = -45.0 + 90.0 * i / FMath::Max(1, Coats - 1);
				B.Add(Cube, Pick(Cloths, Random), FVector(X, RailY, 118.0), FVector(6.0, 45.0, 95.0));
			}
			break;
		}
		case EGoods::Shoes:
			// Pairs in rows.
			for (int32 Row = 0; Row < 2; ++Row)
			{
				for (int32 k = 0; k < 6; ++k)
				{
					const FLinearColor Color = Random.Unit() < 0.6 ? Leather : Pick(Cloths, Random);
					const double X = -HalfDepth * 0.5 + Row * HalfDepth;
					const double Y = -HalfWidth + 15.0 + k * (2.0 * HalfWidth - 30.0) / 5.0;
					for (const double Side : { -6.0, 6.0 })
					{
						B.Add(Cube, Color, FVector(X, Y + Side, Top + 5.0), FVector(26.0, 9.0, 10.0));
					}
				}
			}
			break;
		case EGoods::Pots:
		{
			for (int32 i = 0; i < 7; ++i)
			{
				const double Size = Random.Range(18.0, 34.0);
				const double Height = Size * Random.Range(0.5, 0.9);
				const FVector At = Spot(Height * 0.5);
				B.Add(Cylinder, Pick(Metals, Random), At, FVector(Size, Size, Height));
			}
			// A lid and a kettle.
			B.Add(Cylinder, Iron, Spot(1.0), FVector(30.0, 30.0, 2.0));
			const FVector Kettle = Spot(10.0);
			B.Add(Sphere, Enamel, Kettle, FVector(24.0, 24.0, 20.0));
			B.Add(Cone, Enamel, Kettle + FVector(0.0, 0.0, 12.0), FVector(8.0, 8.0, 8.0));
			break;
		}
		case EGoods::Junk:
		{
			// Old iron: gear wheels on edge, tools, a lamp, a box of odds and ends.
			for (int32 i = 0; i < 4; ++i)
			{
				const double Size = Random.Range(14.0, 30.0);
				const FVector At = Spot(Size * 0.5);
				B.Add(Cylinder, Rust, At, FVector(Size, Size, 4.0), 90.0, Random.Range(0.0, 180.0));
			}
			for (int32 i = 0; i < 5; ++i)
			{
				const FLinearColor& Color = Random.Unit() < 0.5 ? Iron : Rust;
				const double Length = Random.Range(25.0, 45.0);
				const FVector At = Spot(2.0);
				B.Add(Cube, Color, At, FVector(Length, 4.0, 3.0), 0.0, Spin());
			}
			const FVector Lamp = Spot(12.0);
			B.Add(Cylinder, Copper, Lamp, FVector(12.0, 12.0, 24.0));
			B.Add(Cone, Iron, Lamp + FVector(0.0, 0.0, 16.0), FVector(14.0, 14.0, 10.0));
			const FVector Box = Spot(12.0);
			B.Add(Cube, Crate, Box, FVector(40.0, 30.0, 24.0), 0.0, Spin());
			break;
		}
		case EGoods::Books:
			for (int32 i = 0; i < 7; ++i)
			{
				const FVector At = Spot(0.0);
				const int32 Count = 2 + Random.Index(5);
				for (int32 k = 0; k < Count; ++k)
				{
					B.Add(Cube, Pick(BookCovers, Random), At + FVector(0.0, 0.0, 2.5 + k * 4.5), FVector(22.0, 16.0, 4.0), 0.0, Random.Range(-10.0, 10.0));
				}
			}
			B.Add(Cube, Paper, Spot(1.0), FVector(30.0, 40.0, 2.0), 0.0, Spin());
			break;
		case EGoods::Furniture:
			// Chairs and a little cupboard in front of the table, a mirror on it.
			for (int32 i = 0; i < 2; ++i)
			{
				const double Turn = Random.Range(-30.0, 30.0);
				FBuilder Chair{ Out, Middle, Yaw };
				const double A = FMath::DegreesToRadians(Yaw);
				const FVector Seat(TableDepth * 0.5 + 40.0, -90.0 + i * 70.0, 0.0);
				Chair.Origin = Middle + FVector(Seat.X * FMath::Cos(A) - Seat.Y * FMath::Sin(A), Seat.X * FMath::Sin(A) + Seat.Y * FMath::Cos(A), 0.0);
				Chair.FrameYaw = Yaw + Turn;
				Chair.Add(Cube, Board, FVector(0.0, 0.0, 45.0), FVector(40.0, 40.0, 4.0));
				for (const double X : { -16.0, 16.0 })
				{
					for (const double Y : { -16.0, 16.0 })
					{
						Chair.Add(Cube, Post, FVector(X, Y, 22.0), FVector(4.0, 4.0, 44.0));
					}
				}
				Chair.Add(Cube, Board, FVector(-18.0, 0.0, 70.0), FVector(4.0, 38.0, 46.0));
			}
			B.Add(Cube, OldBoard, FVector(TableDepth * 0.5 + 40.0, 80.0, 60.0), FVector(45.0, 70.0, 120.0), 0.0, 10.0, 0.0, true);
			B.Add(Cube, Enamel, FVector(-20.0, 0.0, Top + 30.0), FVector(3.0, 40.0, 55.0));
			B.Add(Cube, Post, FVector(-21.0, 0.0, Top + 30.0), FVector(2.0, 46.0, 61.0));
			break;
		case EGoods::Produce:
			// Crates and baskets heaped with vegetables and apples.
			for (int32 i = 0; i < 4; ++i)
			{
				const FVector At(-HalfDepth * 0.4 + (i % 2) * HalfDepth * 0.8, -HalfWidth * 0.75 + (i / 2) * HalfWidth * 1.5, Top);
				if (Random.Unit() < 0.35)
				{
					B.Add(Cylinder, Wicker, At + FVector(0.0, 0.0, 12.0), FVector(45.0, 45.0, 24.0));
				}
				else
				{
					B.Add(Cube, Crate, At + FVector(0.0, 0.0, 12.0), FVector(45.0, 60.0, 24.0));
				}
				const int32 Kind = Random.Index(static_cast<int32>(UE_ARRAY_COUNT(Produce)));
				const double Size = Kind == 2 ? 20.0 : 9.0;
				const int32 Count = Kind == 2 ? 4 : 14;
				for (int32 k = 0; k < Count; ++k)
				{
					const double X = Random.Range(-14.0, 14.0);
					const double Y = Random.Range(-20.0, 20.0);
					const double Z = 24.0 + Random.Range(0.0, 8.0);
					B.Add(Sphere, Produce[Kind], At + FVector(X, Y, Z), FVector(Size, Size, Size * 0.9));
				}
			}
			break;
		case EGoods::Crockery:
		default:
			// Stacks of plates, jugs and bowls.
			for (int32 i = 0; i < 5; ++i)
			{
				const int32 Plates = 3 + Random.Index(6);
				const FVector At = Spot(Plates * 1.0);
				B.Add(Cylinder, Pick(Glazes, Random), At, FVector(24.0, 24.0, Plates * 2.0));
			}
			for (int32 i = 0; i < 4; ++i)
			{
				const FVector Jug = Spot(12.0);
				const FLinearColor Glaze = Pick(Glazes, Random);
				B.Add(Cylinder, Glaze, Jug, FVector(14.0, 14.0, 24.0));
				B.Add(Cone, Glaze, Jug + FVector(0.0, 0.0, 15.0), FVector(10.0, 10.0, 8.0), 180.0);
			}
			break;
		}
		return bBlanket ? SellerBack + 30.0 : SellerBack;
	}
}
