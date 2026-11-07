// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MarketModel.h"
#include "PersonKit.h"

/**
 * Bandi's boxing yard: a grund, an empty lot on Kis Fuvaros utca between
 * the bare fire walls of the houses round it, where Bandi, a young boxer,
 * trains and the lads of the street fight on summer evenings. A plank fence
 * with a gate to the street, a ring chalked on the beaten earth with four
 * posts and two ropes, crates and a bench for the watchers, a timber pile
 * the boys sit on, Bandi's corner with a punching bag hung from a frame, a
 * lean-to shed against the back wall, a lamp on a pole over the ring, a
 * washing line across a corner and a slate with the names of the men to beat.
 *
 * Header-only, as basic shapes (MarketModel::FShape). The yard's frame: the
 * fence runs along Y at X = 0 (the street beyond it, at -X), +X goes into
 * the lot, Z up from the pavement. The lot is X 0..Depth, Y -Half..Half.
 */
namespace BoxingYardModel
{
	using MarketModel::FShape;
	using MarketModel::FRandom;

	/** Painted lettering on a board or a wall: the actor makes it a text in the game. */
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
		const FLinearColor Earth(0.29f, 0.24f, 0.17f);
		const FLinearColor EarthDark(0.22f, 0.18f, 0.13f);
		const FLinearColor Grass(0.13f, 0.17f, 0.07f);
		const FLinearColor Chalk(0.80f, 0.78f, 0.72f);
		const FLinearColor Plaster(0.50f, 0.45f, 0.37f);
		const FLinearColor PlasterGrey(0.40f, 0.38f, 0.34f);
		const FLinearColor PlasterDark(0.33f, 0.29f, 0.24f);
		const FLinearColor Brick(0.36f, 0.16f, 0.10f);
		const FLinearColor Planks(0.24f, 0.17f, 0.10f);
		const FLinearColor PlanksLight(0.33f, 0.24f, 0.15f);
		const FLinearColor Wood(0.30f, 0.21f, 0.12f);
		const FLinearColor Log(0.27f, 0.20f, 0.13f);
		const FLinearColor LogEnd(0.55f, 0.43f, 0.28f);
		const FLinearColor Rope(0.52f, 0.43f, 0.28f);
		const FLinearColor Leather(0.25f, 0.12f, 0.05f);
		const FLinearColor Canvas(0.55f, 0.50f, 0.40f);
		const FLinearColor Tin(0.30f, 0.31f, 0.32f);
		const FLinearColor Iron(0.05f, 0.05f, 0.055f);
		const FLinearColor Dark(0.035f, 0.03f, 0.03f);
		const FLinearColor Red(0.45f, 0.06f, 0.04f);
		const FLinearColor Blue(0.08f, 0.13f, 0.30f);
		const FLinearColor Paper(0.72f, 0.66f, 0.50f);
		const FLinearColor PaperRed(0.55f, 0.16f, 0.10f);
		const FLinearColor Slate(0.07f, 0.08f, 0.08f);
		const FLinearColor Linen(0.80f, 0.78f, 0.72f);
		const FLinearColor Water(0.10f, 0.11f, 0.12f);
		const FLinearColor Globe(0.85f, 0.80f, 0.62f);
		const FLinearColor Lettering(0.10f, 0.07f, 0.05f);
		const FLinearColor WallAd(0.62f, 0.52f, 0.34f);
		const FLinearColor WallAdRed(0.42f, 0.14f, 0.09f);
	}

	/** Where the yard stands in the city: the middle of its fence on Kis Fuvaros utca, and the way into it (degrees). */
	const FVector2D CityAt(67771.0, 32278.0);
	constexpr double CityYaw = -126.89;

	// Sizes (cm).
	constexpr double Depth = 2800.0;
	constexpr double Half = 1875.0;
	constexpr double GateHalf = 230.0;      // half the gateway's width, about Y = 0
	constexpr double FenceH = 230.0;
	constexpr double SideWallH = 1500.0;    // the fire walls of the houses either side
	constexpr double BackWallH = 1800.0;
	/** The ring: its middle and half its side (chalk line to chalk line). */
	const FVector RingAt(1450.0, 150.0, 0.0);
	constexpr double RingHalf = 320.0;
	constexpr double RopeLow = 62.0, RopeHigh = 104.0, PostH = 125.0;

	/** Where things are, in the yard's frame. */
	namespace Spots
	{
		/** In the gateway, the way in and out. */
		const FVector Gate(0.0, 0.0, 0.0);
		/** Bandi, by his corner of the ring when nobody is fighting, looking at the gate. */
		const FVector Bandi(980.0, -340.0, 0.0);
		constexpr double BandiYaw = 200.0;
		/** The two fighters' marks in the ring: the player's (near the gate) and the other man's. */
		const FVector PlayerMark(RingAt.X - 150.0, RingAt.Y - 60.0, 0.0);
		const FVector RivalMark(RingAt.X + 150.0, RingAt.Y + 60.0, 0.0);
		/** The punching bag. */
		const FVector Bag(2350.0, -1250.0, 0.0);
	}

	/** One watcher round the ring: where, which way he looks, and whether he sits. */
	struct FWatcher
	{
		FVector At;
		double Yaw;
		bool bSeated;
		bool bBoy;
	};

	inline void Add(TArray<FShape>& Out, int32 Shape, const FLinearColor& Color, const FVector& At, const FVector& Size,
		double Yaw = 0.0, bool bCollide = true, double Pitch = 0.0, double Roll = 0.0)
	{
		FShape S;
		S.Shape = Shape;
		S.Color = Color;
		S.At = At;
		S.Size = Size;
		S.Pitch = Pitch;
		S.Yaw = Yaw;
		S.Roll = Roll;
		S.bCollide = bCollide;
		Out.Add(S);
	}

	/** A box from one corner to the other (axis-aligned). */
	inline void Span(TArray<FShape>& Out, const FLinearColor& Color, const FVector& A, const FVector& B, bool bCollide = true)
	{
		Add(Out, MarketModel::Cube, Color, (A + B) * 0.5, (B - A).GetAbs(), 0.0, bCollide);
	}

	/** A rope or a beam from A to B, Thick across. */
	inline void Line(TArray<FShape>& Out, const FLinearColor& Color, const FVector& A, const FVector& B, double Thick, bool bCollide = false)
	{
		const FVector D = B - A;
		const double Flat = FMath::Sqrt(D.X * D.X + D.Y * D.Y);
		const double LineYaw = FMath::RadiansToDegrees(FMath::Atan2(D.Y, D.X));
		const double LinePitch = FMath::RadiansToDegrees(FMath::Atan2(D.Z, Flat));
		Add(Out, MarketModel::Cube, Color, (A + B) * 0.5, FVector(D.Size(), Thick, Thick), LineYaw, bCollide, LinePitch);
	}

	/** A crate at At, Size on a side, turned Yaw. */
	inline void Crate(TArray<FShape>& Out, const FVector& At, double Size, double Yaw)
	{
		Add(Out, MarketModel::Cube, Colors::PlanksLight, At + FVector(0, 0, Size * 0.5), FVector(Size, Size, Size), Yaw);
		Add(Out, MarketModel::Cube, Colors::Planks, At + FVector(0, 0, Size * 0.5), FVector(Size + 1.5, Size * 0.18, Size + 1.5), Yaw, false);
	}

	/** The fence to the street: tarred planks either side of the gateway, posters, the two leaves standing open inside. */
	inline void BuildFence(TArray<FShape>& Out, FRandom& R)
	{
		const double T = 6.0;
		for (const double Side : { -1.0, 1.0 })
		{
			const double Y0 = Side * GateHalf, Y1 = Side * Half;
			Span(Out, Colors::Planks, FVector(0, FMath::Min(Y0, Y1), 0), FVector(T, FMath::Max(Y0, Y1), FenceH));
			// Lighter boards here and there, and the rails behind.
			for (double Y = Side * (GateHalf + 40.0); Side * Y < Half - 20.0; Y += Side * R.Range(70.0, 160.0))
			{
				const double Tall = FenceH - R.Range(0.0, 14.0);
				Span(Out, Colors::PlanksLight, FVector(-1.0, Y - 9.0, 0), FVector(0.0, Y + 9.0, Tall), false);
			}
			for (const double Z : { 40.0, 190.0 })
			{
				Span(Out, Colors::Planks, FVector(T, FMath::Min(Y0, Y1), Z - 5.0), FVector(T + 6.0, FMath::Max(Y0, Y1), Z + 5.0), false);
			}
			// Posters on the street side, old and new.
			for (int32 i = 0; i < 3; ++i)
			{
				const double Y = Side * R.Range(GateHalf + 300.0, Half - 200.0);
				const double W = R.Range(50.0, 80.0);
				Span(Out, i == 1 ? Colors::PaperRed : Colors::Paper, FVector(-1.5, Y - W * 0.5, 80.0), FVector(-0.5, Y + W * 0.5, 80.0 + W * 1.35), false);
			}
			// The gate post.
			Add(Out, MarketModel::Cube, Colors::Wood, FVector(T * 0.5, Y0, 130.0), FVector(16, 16, 260));
			// The leaf, swung open into the lot.
			const double Open = 75.0 * Side;
			const FRotator Turn(0.0, Open, 0.0);
			const FVector Hinge(T + 8.0, Y0, 0.0);
			const auto P = [&](double Along, double Z) { return Hinge + Turn.RotateVector(FVector(0.0, -Side * Along, Z)); };
			Add(Out, MarketModel::Cube, Colors::Planks, P(GateHalf * 0.5, 110), FVector(5, GateHalf - 10, 200), Open);
			Add(Out, MarketModel::Cube, Colors::PlanksLight, P(GateHalf * 0.5, 110), FVector(7, GateHalf - 20, 12), Open, false, 0.0, Side * 40.0);
		}
		// The board over the gateway.
		Span(Out, Colors::Wood, FVector(0, -GateHalf, 250), FVector(T, GateHalf, 262), false);
		Span(Out, Colors::Paper, FVector(-3, -GateHalf + 25, 262), FVector(1, GateHalf - 25, 310), false);
	}

	/** The fire walls of the houses either side and behind: bare plaster, patches of brick, an old painted advertisement. */
	inline void BuildWalls(TArray<FShape>& Out, FRandom& R)
	{
		const double T = 40.0;
		Span(Out, Colors::Plaster, FVector(0, -Half - T, 0), FVector(Depth + T, -Half, SideWallH));
		Span(Out, Colors::PlasterGrey, FVector(0, Half, 0), FVector(Depth + T, Half + T, SideWallH));
		Span(Out, Colors::PlasterDark, FVector(Depth, -Half - T, 0), FVector(Depth + T, Half + T, BackWallH));
		// The side walls step up behind, where the houses are taller.
		Span(Out, Colors::Plaster, FVector(Depth - 900.0, -Half - T, SideWallH), FVector(Depth + T, -Half, BackWallH));
		// Brick showing through, and damp along the foot.
		for (int32 i = 0; i < 9; ++i)
		{
			const int32 Wall = i % 3;
			const double W = R.Range(60.0, 180.0), Hh = R.Range(40.0, 110.0), Z = R.Range(120.0, 1200.0);
			if (Wall == 2)
			{
				const double Y = R.Range(-Half + 200.0, Half - 200.0);
				Span(Out, Colors::Brick, FVector(Depth - 1.5, Y, Z), FVector(Depth, Y + W, Z + Hh), false);
			}
			else
			{
				const double X = R.Range(300.0, Depth - 300.0);
				const double Y = Wall == 0 ? -Half : Half - 1.5;
				Span(Out, Colors::Brick, FVector(X, Y, Z), FVector(X + W, Y + 1.5, Z + Hh), false);
			}
		}
		Span(Out, Colors::PlasterDark, FVector(0, -Half, 0), FVector(Depth, -Half + 1.5, 70), false);
		Span(Out, Colors::PlasterDark, FVector(0, Half - 1.5, 0), FVector(Depth, Half, 70), false);
		// The faded painted board on the back wall (its lettering is a sign).
		Span(Out, Colors::WallAdRed, FVector(Depth - 1.0, -900.0, 900.0), FVector(Depth, 900.0, 1260.0), false);
		Span(Out, Colors::WallAd, FVector(Depth - 1.6, -860.0, 930.0), FVector(Depth - 1.0, 860.0, 1230.0), false);
	}

	/** Beaten earth, worn bare in the ring, grass along the walls, a puddle, the chalked ring. */
	inline void BuildGround(TArray<FShape>& Out, FRandom& R)
	{
		Span(Out, Colors::Earth, FVector(0, -Half, 0), FVector(Depth, Half, 2), false);
		Add(Out, MarketModel::Cylinder, Colors::EarthDark, RingAt + FVector(0, 0, 2.2), FVector(RingHalf * 2.6, RingHalf * 2.4, 0.4), 20.0, false);
		Add(Out, MarketModel::Cylinder, Colors::EarthDark, FVector(80.0, 0, 2.2), FVector(600.0, 420.0, 0.4), 0.0, false);
		for (int32 i = 0; i < 26; ++i)
		{
			const bool bSide = i % 2 == 0;
			const FVector At = bSide
				? FVector(R.Range(200.0, Depth - 200.0), (i % 4 == 0 ? -1.0 : 1.0) * (Half - R.Range(20.0, 120.0)), 2.3)
				: FVector(Depth - R.Range(20.0, 120.0), R.Range(-Half + 400.0, Half - 100.0), 2.3);
			Add(Out, MarketModel::Cylinder, Colors::Grass, At, FVector(R.Range(80.0, 220.0), R.Range(50.0, 120.0), 0.6), R.Range(0.0, 180.0), false);
		}
		Add(Out, MarketModel::Cylinder, Colors::Water, FVector(600.0, 1100.0, 2.5), FVector(260.0, 150.0, 0.4), 30.0, false);
		// The chalk square, a little rubbed out.
		const double H = RingHalf, W = 7.0;
		Span(Out, Colors::Chalk, RingAt + FVector(-H, -H - W, 2.6), RingAt + FVector(H, -H + W, 2.9), false);
		Span(Out, Colors::Chalk, RingAt + FVector(-H, H - W, 2.6), RingAt + FVector(H, H + W, 2.9), false);
		Span(Out, Colors::Chalk, RingAt + FVector(-H - W, -H, 2.6), RingAt + FVector(-H + W, H, 2.9), false);
		Span(Out, Colors::Chalk, RingAt + FVector(H - W, -H, 2.6), RingAt + FVector(H + W, H, 2.9), false);
	}

	/** The ring's four posts and two ropes; Bandi's corner has a red rag, the far one a blue. */
	inline void BuildRing(TArray<FShape>& Out)
	{
		const double H = RingHalf + 15.0;
		const FVector Corners[4] = {
			RingAt + FVector(-H, -H, 0), RingAt + FVector(H, -H, 0), RingAt + FVector(H, H, 0), RingAt + FVector(-H, H, 0) };
		for (int32 i = 0; i < 4; ++i)
		{
			Add(Out, MarketModel::Cylinder, Colors::Wood, Corners[i] + FVector(0, 0, PostH * 0.5), FVector(14, 14, PostH));
			Add(Out, MarketModel::Cylinder, Colors::Dark, Corners[i] + FVector(0, 0, PostH + 2.0), FVector(16, 16, 4), 0.0, false);
			const FVector& Next = Corners[(i + 1) % 4];
			for (const double Z : { RopeLow, RopeHigh })
			{
				// The ropes sag a little: two straight halves.
				const FVector Mid = (Corners[i] + Next) * 0.5 + FVector(0, 0, Z - 5.0);
				Line(Out, Colors::Rope, Corners[i] + FVector(0, 0, Z), Mid, 3.5);
				Line(Out, Colors::Rope, Mid, Next + FVector(0, 0, Z), 3.5);
			}
		}
		Add(Out, MarketModel::Cube, Colors::Red, Corners[0] + FVector(0, 0, PostH - 18.0), FVector(17, 17, 26), 0.0, false);
		Add(Out, MarketModel::Cube, Colors::Blue, Corners[2] + FVector(0, 0, PostH - 18.0), FVector(17, 17, 26), 0.0, false);
		// A bucket and a stool in Bandi's corner, a towel over the rope.
		const FVector C0 = Corners[0] + FVector(-45, -40, 0);
		Add(Out, MarketModel::Cylinder, Colors::Tin, C0 + FVector(0, 0, 16), FVector(30, 30, 32), 0.0, false);
		Add(Out, MarketModel::Cylinder, Colors::Water, C0 + FVector(0, 0, 30), FVector(26, 26, 2), 0.0, false);
		Add(Out, MarketModel::Cylinder, Colors::Wood, C0 + FVector(30, -55, 40), FVector(36, 36, 5), 0.0, false);
		for (const FVector& L : { FVector(14, 0, 0), FVector(-8, 12, 0), FVector(-8, -12, 0) })
		{
			Add(Out, MarketModel::Cube, Colors::Wood, C0 + FVector(30, -55, 19) + L, FVector(4, 4, 38), 0.0, false);
		}
		Add(Out, MarketModel::Cube, Colors::Linen, Corners[0] + FVector(80, 0, RopeHigh - 14.0), FVector(40, 3, 32), 0.0, false);
	}

	/** Seats for the watchers: crates, a plank bench, the timber pile at the back the boys climb, an old handcart. */
	inline void BuildSeats(TArray<FShape>& Out)
	{
		const double Far = RingHalf + 260.0;
		// A bench of a plank on two crates along the street side of the ring.
		const double BX = RingAt.X - Far;
		Crate(Out, FVector(BX, RingAt.Y - 220.0, 0), 42.0, 5.0);
		Crate(Out, FVector(BX, RingAt.Y + 200.0, 0), 42.0, -4.0);
		Span(Out, Colors::PlanksLight, FVector(BX - 16, RingAt.Y - 280.0, 42), FVector(BX + 16, RingAt.Y + 260.0, 47));
		// Crates on the far sides.
		Crate(Out, FVector(RingAt.X + 160.0, RingAt.Y + Far, 0), 46.0, 12.0);
		Crate(Out, FVector(RingAt.X - 120.0, RingAt.Y + Far + 20.0, 0), 40.0, -20.0);
		Crate(Out, FVector(RingAt.X + Far, RingAt.Y + 120.0, 0), 48.0, 30.0);
		Crate(Out, FVector(RingAt.X + Far + 10.0, RingAt.Y + 120.0, 48), 38.0, 18.0);
		// The timber pile against the back wall.
		const FVector Pile(Depth - 180.0, 900.0, 0);
		for (int32 Row = 0; Row < 4; ++Row)
		{
			for (int32 i = 0; i < 7 - Row; ++i)
			{
				const FVector At = Pile + FVector((i - (6 - Row) * 0.5) * 34.0, 0, 17.0 + Row * 30.0);
				Add(Out, MarketModel::Cylinder, Colors::Log, At, FVector(32, 32, 520), 0.0, true, 0.0, 90.0);
				for (const double End : { -261.0, 261.0 })
				{
					Add(Out, MarketModel::Cylinder, Colors::LogEnd, At + FVector(0, End, 0),
						FVector(28, 28, 2), 0.0, false, 0.0, 90.0);
				}
			}
		}
		// An old handcart left by the left wall, shafts down.
		const FVector Cart(700.0, -Half + 180.0, 0);
		Add(Out, MarketModel::Cube, Colors::Planks, Cart + FVector(0, 0, 70), FVector(200, 110, 8), 10.0, true, 8.0);
		Add(Out, MarketModel::Cylinder, Colors::Wood, Cart + FVector(20, -62, 46), FVector(92, 92, 6), 10.0, false, 0.0, 90.0);
		Add(Out, MarketModel::Cylinder, Colors::Wood, Cart + FVector(0, 62, 46), FVector(92, 92, 6), 10.0, false, 0.0, 90.0);
		Line(Out, Colors::Wood, Cart + FVector(-90, -40, 60), Cart + FVector(-260, -55, 4), 6.0);
		Line(Out, Colors::Wood, Cart + FVector(-90, 40, 60), Cart + FVector(-260, 25, 4), 6.0);
	}

	/** Bandi's corner at the back: the punching bag on its frame, a heavy sack, the lean-to shed against the back wall, the slate. */
	inline void BuildCorner(TArray<FShape>& Out)
	{
		// The frame: two posts and a beam, the bag on a short chain.
		const FVector B = Spots::Bag;
		for (const double Y : { -120.0, 120.0 })
		{
			Add(Out, MarketModel::Cube, Colors::Wood, B + FVector(0, Y, 140), FVector(14, 14, 280));
			Line(Out, Colors::Wood, B + FVector(-90, Y, 0), B + FVector(0, Y, 150), 9.0);
			Line(Out, Colors::Wood, B + FVector(90, Y, 0), B + FVector(0, Y, 150), 9.0);
		}
		Span(Out, Colors::Wood, B + FVector(-8, -135, 272), B + FVector(8, 135, 288), false);
		Add(Out, MarketModel::Cylinder, Colors::Iron, B + FVector(0, 0, 252), FVector(2, 2, 40), 0.0, false);
		Add(Out, MarketModel::Cylinder, Colors::Leather, B + FVector(0, 0, 175), FVector(40, 40, 110), 0.0, false);
		Add(Out, MarketModel::Sphere, Colors::Leather, B + FVector(0, 0, 120), FVector(40, 40, 16), 0.0, false);
		Add(Out, MarketModel::Cylinder, Colors::Dark, B + FVector(0, 0, 228), FVector(42, 42, 4), 0.0, false);
		// A sack of sand on the ground, a skipping rope over a post.
		Add(Out, MarketModel::Sphere, Colors::Canvas, B + FVector(-160, 170, 22), FVector(80, 50, 44), 25.0, false);
		Line(Out, Colors::Rope, B + FVector(0, -127, 230), B + FVector(4, -128, 180), 2.0);
		Line(Out, Colors::Rope, B + FVector(0, -113, 230), B + FVector(-4, -112, 180), 2.0);
		// The lean-to: posts at the front, a tin roof sloping up to the back wall, a door of planks in the left wall.
		const double S0 = Depth - 380.0, Y0 = -Half, Y1 = -Half + 560.0, Eave = 230.0, Top = 300.0;
		for (const double Y : { Y0 + 20.0, Y1 - 10.0 })
		{
			Add(Out, MarketModel::Cube, Colors::Wood, FVector(S0, Y, Eave * 0.5), FVector(14, 14, Eave));
		}
		Span(Out, Colors::Planks, FVector(S0 - 4, Y1 - 6, 0), FVector(Depth, Y1 + 6, Eave), true);
		const double Run = Depth - S0 + 40.0, Rise = Top - Eave;
		Add(Out, MarketModel::Cube, Colors::Tin, FVector(S0 - 40.0 + Run * 0.5, (Y0 + Y1) * 0.5, (Eave + Top) * 0.5 + 4.0),
			FVector(FMath::Sqrt(Run * Run + Rise * Rise), Y1 - Y0 + 30.0, 4.0), 0.0, true, FMath::RadiansToDegrees(FMath::Atan2(Rise, Run)));
		// Inside: a bench and a chest, dark.
		Span(Out, Colors::Dark, FVector(S0 + 10, Y0, 0), FVector(Depth, Y1 - 6, 2.5), false);
		Span(Out, Colors::Wood, FVector(Depth - 70, Y0 + 60, 40), FVector(Depth, Y1 - 80, 46), false);
		Span(Out, Colors::Leather, FVector(S0 + 80, Y0 + 40, 0), FVector(S0 + 150, Y0 + 140, 50), false);
		// The slate on two legs by the ring, with the names (a sign).
		const FVector SlateAt(RingAt.X - 120.0, RingAt.Y - RingHalf - 230.0, 0);
		for (const double Y : { -70.0, 70.0 })
		{
			Add(Out, MarketModel::Cube, Colors::Wood, SlateAt + FVector(0, Y, 75), FVector(6, 6, 150), 0.0, false, -8.0);
		}
		Add(Out, MarketModel::Cube, Colors::Wood, SlateAt + FVector(-2, 0, 125), FVector(5, 162, 102), 0.0, false, -8.0);
		Add(Out, MarketModel::Cube, Colors::Slate, SlateAt + FVector(-4.5, 0, 125), FVector(2, 150, 90), 0.0, false, -8.0);
	}

	/** The lamp on a pole over the ring (the actor lights it at night), a washing line across the right-hand corner. */
	inline void BuildLampAndLine(TArray<FShape>& Out)
	{
		const FVector Pole(RingAt.X + RingHalf + 120.0, RingAt.Y - RingHalf - 120.0, 0);
		Add(Out, MarketModel::Cylinder, Colors::Wood, Pole + FVector(0, 0, 230), FVector(16, 16, 460));
		Line(Out, Colors::Wood, Pole + FVector(0, 0, 440), Pole + FVector(-260, 260, 440), 8.0);
		Add(Out, MarketModel::Cylinder, Colors::Iron, Pole + FVector(-260, 260, 425), FVector(2, 2, 30), 0.0, false);
		Add(Out, MarketModel::Cone, Colors::Iron, Pole + FVector(-260, 260, 405), FVector(46, 46, 20), 0.0, false);
		Add(Out, MarketModel::Sphere, Colors::Globe, Pole + FVector(-260, 260, 396), FVector(14, 14, 14), 0.0, false);
		// The line from the side wall to the timber pile's corner pole, sheets and a shirt on it.
		const FVector A(400.0, Half - 2.0, 260.0), Bp(1100.0, Half - 2.0, 260.0);
		const FVector C(800.0, Half - 520.0, 0.0);
		Add(Out, MarketModel::Cylinder, Colors::Wood, C + FVector(0, 0, 135), FVector(9, 9, 270));
		for (const FVector& End : { A, Bp })
		{
			const FVector Top = C + FVector(0, 0, 262);
			const FVector Mid = (End + Top) * 0.5 - FVector(0, 0, 18);
			Line(Out, Colors::Rope, End, Mid, 1.5);
			Line(Out, Colors::Rope, Mid, Top, 1.5);
		}
		const FVector Top = C + FVector(0, 0, 262);
		const FVector Dir = (A - Top).GetSafeNormal();
		const double Along = FMath::RadiansToDegrees(FMath::Atan2(Dir.Y, Dir.X));
		const FVector Sheets[3] = { Top + (A - Top) * 0.28, Top + (A - Top) * 0.55, Top + (Bp - Top) * 0.45 };
		for (int32 i = 0; i < 3; ++i)
		{
			const FVector Dn = i == 2 ? (Bp - Top).GetSafeNormal() : Dir;
			const double Y = i == 2 ? FMath::RadiansToDegrees(FMath::Atan2(Dn.Y, Dn.X)) : Along;
			const double Long = i == 1 ? 60.0 : 120.0;
			Add(Out, MarketModel::Cube, i == 1 ? FLinearColor(0.35f, 0.40f, 0.48f) : Colors::Linen, Sheets[i] - FVector(0, 0, Long * 0.5 + 14.0), FVector(i == 1 ? 50.0 : 110.0, 1.5, Long), Y, false);
		}
	}

	inline void Build(TArray<FShape>& Out)
	{
		FRandom R(1932);
		BuildGround(Out, R);
		BuildFence(Out, R);
		BuildWalls(Out, R);
		BuildRing(Out);
		BuildSeats(Out);
		BuildCorner(Out);
		BuildLampAndLine(Out);
	}

	inline void Signs(TArray<FSign>& Out)
	{
		// Over the gate, on the street side.
		Out.Add({ TEXT("ÖKÖLVÍVÁS"), FVector(-4.0, 0.0, 290.0), 180.0, 26.0, Colors::Lettering });
		// The slate by the ring: the men to beat, in chalk.
		const FVector SlateAt(RingAt.X - 120.0 - 11.0, RingAt.Y - RingHalf - 230.0, 0.0);
		const TCHAR* Names[] = { TEXT("1. BANDI"), TEXT("2. FÜRGE JENŐ"), TEXT("3. MÁZSÁS"), TEXT("4. SZŐKE GYURI"), TEXT("5. VASÖKLŰ") };
		for (int32 i = 0; i < 5; ++i)
		{
			Out.Add({ Names[i], SlateAt + FVector(0.0, -25.0, 155.0 - i * 13.0), 180.0, 7.5, Colors::Chalk });
		}
		// The old painted board on the back wall.
		Out.Add({ TEXT("KŐBÁNYAI SÖR"), FVector(Depth - 2.0, 0.0, 1110.0), 180.0, 120.0, Colors::WallAdRed });
		Out.Add({ TEXT("A LEGJOBB"), FVector(Depth - 2.0, 0.0, 990.0), 180.0, 60.0, Colors::Lettering });
	}

	/** The watchers round the ring (the actor dresses them from Watcher()). */
	inline void Watchers(TArray<FWatcher>& Out)
	{
		const FVector C = RingAt;
		const double Far = RingHalf + 70.0;
		const auto Look = [&C](const FVector& At) { return FMath::RadiansToDegrees(FMath::Atan2(C.Y - At.Y, C.X - At.X)); };
		const FVector Standing[] = {
			C + FVector(-Far - 20.0, -150.0, 0), C + FVector(-Far, 60.0, 0), C + FVector(-Far - 60.0, 280.0, 0),
			C + FVector(Far, -200.0, 0), C + FVector(Far + 30.0, 40.0, 0), C + FVector(Far - 20.0, 300.0, 0),
			C + FVector(-160.0, Far + 40.0, 0), C + FVector(40.0, Far + 10.0, 0), C + FVector(260.0, Far + 60.0, 0),
			C + FVector(230.0, -Far - 30.0, 0), C + FVector(60.0, -Far - 120.0, 0) };
		for (const FVector& At : Standing)
		{
			Out.Add({ At, Look(At), false, false });
		}
		// On the bench and the crates.
		const double BX = C.X - (RingHalf + 260.0);
		for (const double Y : { -200.0, -40.0, 140.0 })
		{
			const FVector At(BX, C.Y + Y, 0);
			Out.Add({ At, 0.0, true, false });
		}
		Out.Add({ C + FVector(160.0, RingHalf + 260.0, 0), -90.0, true, false });
		// Boys up on the timber pile.
		for (const double Y : { 820.0, 960.0 })
		{
			const FVector At(Depth - 180.0, Y, 123.0);
			Out.Add({ At, Look(At), true, true });
		}
	}

	/** A watcher's clothes: lads and men of the street, in shirtsleeves and caps, a few in jackets. */
	inline PersonKit::FOutfit Watcher(int32 Index, bool bBoy)
	{
		FRandom R(static_cast<uint32>(Index * 7919 + 17));
		PersonKit::FOutfit O;
		O.Height = bBoy ? static_cast<float>(R.Range(140.0, 152.0)) : static_cast<float>(R.Range(166.0, 184.0));
		O.Build = bBoy ? 0.8f : static_cast<float>(R.Range(0.9, 1.15));
		const PersonKit::EHat Hats[] = { PersonKit::EHat::FlatCap, PersonKit::EHat::FlatCap, PersonKit::EHat::Fedora, PersonKit::EHat::None, PersonKit::EHat::Bowler };
		O.Hat = Hats[R.Index(5)];
		const PersonKit::ETop Tops[] = { PersonKit::ETop::Shirt, PersonKit::ETop::Waistcoat, PersonKit::ETop::Jacket };
		O.Top = Tops[R.Index(3)];
		O.bTie = R.Unit() < 0.25;
		O.bMoustache = !bBoy && R.Unit() < 0.4;
		const FLinearColor Cloth[] = {
			FLinearColor(0.12f, 0.11f, 0.10f), FLinearColor(0.20f, 0.17f, 0.13f), FLinearColor(0.10f, 0.12f, 0.16f),
			FLinearColor(0.22f, 0.21f, 0.19f), FLinearColor(0.16f, 0.12f, 0.08f) };
		O.Coat = Cloth[R.Index(5)];
		O.Waistcoat = Cloth[R.Index(5)];
		O.Trousers = Cloth[R.Index(5)];
		const FLinearColor Shirts[] = { FLinearColor(0.80f, 0.78f, 0.72f), FLinearColor(0.55f, 0.58f, 0.62f), FLinearColor(0.62f, 0.56f, 0.46f) };
		O.Shirt = Shirts[R.Index(3)];
		O.HatColor = Cloth[R.Index(5)];
		O.Hair = R.Unit() < 0.2 ? FLinearColor(0.40f, 0.28f, 0.12f) : FLinearColor(0.08f, 0.06f, 0.04f);
		O.bSeated = false;
		return O;
	}

	/** Bandi: nineteen, wiry, stripped to the waist, a red belt, short dark hair, no hat. */
	inline PersonKit::FOutfit Bandi()
	{
		PersonKit::FOutfit O;
		O.Height = 177.f;
		O.Build = 0.98f;
		O.Hat = PersonKit::EHat::None;
		O.Top = PersonKit::ETop::Shirt;
		O.bTie = false;
		O.Shirt = O.Skin;
		O.bSash = true;
		O.Sash = FLinearColor(0.50f, 0.06f, 0.04f);
		O.Trousers = FLinearColor(0.07f, 0.07f, 0.08f);
		O.Shoes = FLinearColor(0.05f, 0.04f, 0.03f);
		O.Hair = FLinearColor(0.07f, 0.05f, 0.03f);
		return O;
	}

	/** The men to beat after Bandi, 1..4: Fürge Jenő (small, quick), Mázsás (a big porter), Szőke Gyuri (fair, guarded), Vasöklű (the champion). */
	inline PersonKit::FOutfit Rival(int32 Which)
	{
		PersonKit::FOutfit O;
		O.Hat = PersonKit::EHat::None;
		O.Top = PersonKit::ETop::Shirt;
		O.bTie = false;
		O.bSash = true;
		switch (Which)
		{
		case 1:
			O.Height = 166.f;
			O.Build = 0.84f;
			O.Shirt = FLinearColor(0.80f, 0.78f, 0.72f);
			O.Sash = FLinearColor(0.10f, 0.20f, 0.12f);
			O.Trousers = FLinearColor(0.20f, 0.18f, 0.15f);
			O.Hair = FLinearColor(0.12f, 0.08f, 0.04f);
			break;
		case 2:
			O.Height = 189.f;
			O.Build = 1.3f;
			O.Shirt = FLinearColor(0.42f, 0.40f, 0.37f);
			O.Sash = FLinearColor(0.20f, 0.12f, 0.06f);
			O.Trousers = FLinearColor(0.12f, 0.11f, 0.10f);
			O.bMoustache = true;
			break;
		case 3:
			O.Height = 181.f;
			O.Build = 1.05f;
			O.Shirt = FLinearColor(0.50f, 0.58f, 0.68f);
			O.Sash = FLinearColor(0.08f, 0.13f, 0.30f);
			O.Hair = FLinearColor(0.55f, 0.42f, 0.20f);
			O.Trousers = FLinearColor(0.16f, 0.16f, 0.17f);
			break;
		default:
			O.Height = 185.f;
			O.Build = 1.2f;
			O.Shirt = O.Skin;
			O.Sash = FLinearColor(0.03f, 0.03f, 0.03f);
			O.Trousers = FLinearColor(0.22f, 0.20f, 0.17f);
			O.bMoustache = true;
			O.bBoots = true;
			break;
		}
		return O;
	}
}
