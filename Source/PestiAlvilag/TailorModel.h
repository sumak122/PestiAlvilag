// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BozsiHouseModel.h"
#include "MarketModel.h"
#include "PersonKit.h"

/**
 * Kertész és Fia, a gentlemen's tailor on Bérkocsis utca across the street
 * from the player's garage: a shop cut into the house front
 * (Tools/OsmGraybox/city_details.py tailor_cut makes the hole; keep CityAt,
 * CityYaw, HalfWidth, Depth and Top the same there) that the player walks
 * into. A shop front with two display windows, a suit on a dummy in each,
 * and the open door between them; inside panelled walls, shelves of cloth,
 * the counter, a long mirror, a fitting curtain and two lamps; old Kertész
 * behind the counter. The suit bought here gets the player into the
 * Tattersall show (CarShowModel::Doorman).
 *
 * Header-only, as basic shapes (MarketModel::FShape). The shop's frame:
 * X = 0 on the house front, +X out to the street, the room back to -X,
 * Y across (the door's middle at Y = 0), Z up from the pavement.
 */
namespace TailorModel
{
	using MarketModel::FShape;
	using BozsiHouseModel::FSign;
	using BozsiHouseModel::FLamp;
	using BozsiHouseModel::Add;
	using BozsiHouseModel::Span;

	/** The middle of the shop's door on the house front, and the way out to the street. */
	const FVector2D CityAt(19022.0, 29251.0);
	constexpr double CityYaw = 76.2;

	// Sizes (cm).
	constexpr double HalfWidth = 330.0;    // the room, Y -330..330
	constexpr double Depth = 650.0;        // from the front back to X = -650
	constexpr double Top = 340.0;          // the house goes on over the shop from here
	constexpr double DoorHalf = 70.0;      // the open door between the windows
	constexpr double FrameThick = 16.0;

	/** The suit's price (fillér). */
	constexpr int32 SuitPrice = 800;

	/** Where old Kertész stands behind the counter, and where the player stands to buy (on the floor). */
	const FVector TailorSpot(-470.0, -150.0, 0.0);
	constexpr double TailorYaw = 0.0;
	const FVector BuySpot(-300.0, -150.0, 0.0);

	namespace Colors
	{
		const FLinearColor Floor(0.30f, 0.19f, 0.10f);
		const FLinearColor FloorDark(0.22f, 0.13f, 0.07f);
		const FLinearColor Paper(0.16f, 0.24f, 0.19f);
		const FLinearColor Panel(0.26f, 0.15f, 0.08f);
		const FLinearColor Ceiling(0.70f, 0.67f, 0.58f);
		const FLinearColor Front(0.12f, 0.10f, 0.08f);
		const FLinearColor Gilt(0.78f, 0.62f, 0.26f);
		const FLinearColor Glass(0.42f, 0.50f, 0.52f);
		const FLinearColor Brass(0.72f, 0.56f, 0.24f);
		const FLinearColor Iron(0.07f, 0.07f, 0.075f);
		const FLinearColor Counter(0.32f, 0.19f, 0.09f);
		const FLinearColor Baize(0.10f, 0.22f, 0.14f);
		const FLinearColor Mirror(0.60f, 0.66f, 0.68f);
		const FLinearColor Curtain(0.38f, 0.08f, 0.08f);
		const FLinearColor Linen(0.78f, 0.70f, 0.56f);
		const FLinearColor Shade(0.55f, 0.45f, 0.25f);
		const FLinearColor Bulb(0.98f, 0.92f, 0.72f);
		/** Bolts of cloth on the shelves. */
		const FLinearColor Cloths[] = {
			FLinearColor(0.08f, 0.08f, 0.10f), FLinearColor(0.20f, 0.20f, 0.22f), FLinearColor(0.30f, 0.24f, 0.17f),
			FLinearColor(0.10f, 0.12f, 0.20f), FLinearColor(0.36f, 0.33f, 0.28f), FLinearColor(0.16f, 0.11f, 0.08f),
			FLinearColor(0.24f, 0.22f, 0.25f), FLinearColor(0.42f, 0.38f, 0.30f),
		};
	}

	/** The suit the player buys: charcoal three-piece, white shirt, burgundy tie, a grey fedora. */
	inline PersonKit::FOutfit Suit(PersonKit::FOutfit O)
	{
		O.Hat = PersonKit::EHat::Fedora;
		O.HatColor = FLinearColor(0.16f, 0.16f, 0.17f);
		O.HatBand = FLinearColor(0.03f, 0.03f, 0.03f);
		O.Top = PersonKit::ETop::Jacket;
		O.Coat = FLinearColor(0.09f, 0.09f, 0.10f);
		O.Waistcoat = FLinearColor(0.09f, 0.09f, 0.10f);
		O.Trousers = FLinearColor(0.09f, 0.09f, 0.10f);
		O.Shirt = FLinearColor(0.92f, 0.91f, 0.88f);
		O.bTie = true;
		O.Tie = FLinearColor(0.38f, 0.06f, 0.09f);
		O.bScarf = false;
		O.Shoes = FLinearColor(0.02f, 0.015f, 0.01f);
		return O;
	}

	/** Old Kertész: shirt sleeves, waistcoat, a tape measure round his neck, spectacles' grey hair. */
	inline PersonKit::FOutfit Tailor()
	{
		PersonKit::FOutfit O;
		O.Height = 168.f;
		O.Hat = PersonKit::EHat::None;
		O.Top = PersonKit::ETop::Waistcoat;
		O.Waistcoat = FLinearColor(0.20f, 0.17f, 0.13f);
		O.Shirt = FLinearColor(0.88f, 0.86f, 0.80f);
		O.Trousers = FLinearColor(0.14f, 0.13f, 0.12f);
		O.bTie = false;
		O.bScarf = true;
		O.Scarf = FLinearColor(0.85f, 0.78f, 0.40f);
		O.Hair = FLinearColor(0.62f, 0.60f, 0.56f);
		O.bAged = true;
		O.bMoustache = true;
		return O;
	}

	/** A headless tailor's dummy in a suit at At (its foot), turned by Yaw. */
	inline void Dummy(TArray<FShape>& Out, const FVector& At, double Yaw, const FLinearColor& Cloth)
	{
		using namespace Colors;
		const FRotator Turn(0.0, Yaw, 0.0);
		Add(Out, MarketModel::Cylinder, Iron, At + FVector(0, 0, 2), FVector(44, 44, 4), 0.0, false);
		Add(Out, MarketModel::Cylinder, Iron, At + FVector(0, 0, 50), FVector(4, 4, 96), 0.0, false);
		// The jacket over the chest and shoulders, a white collar and a dark tie.
		Add(Out, MarketModel::Cylinder, Cloth, At + FVector(0, 0, 128), FVector(30, 46, 64), Yaw, false);
		Add(Out, MarketModel::Cube, Cloth, At + FVector(0, 0, 158), FVector(26, 52, 8), Yaw, false);
		Add(Out, MarketModel::Cylinder, Linen, At + FVector(0, 0, 168), FVector(13, 13, 14), 0.0, false);
		Add(Out, MarketModel::Sphere, Linen, At + FVector(0, 0, 176), FVector(10, 10, 6), 0.0, false);
		const FVector Chest = At + FVector(0, 0, 140) + Turn.RotateVector(FVector(14.5, 0, 0));
		Add(Out, MarketModel::Cube, FLinearColor(0.92f, 0.91f, 0.88f), Chest, FVector(1, 10, 22), Yaw, false);
		Add(Out, MarketModel::Cube, FLinearColor(0.38f, 0.06f, 0.09f), Chest + Turn.RotateVector(FVector(0.6, 0, -2)), FVector(1, 4, 18), Yaw, false);
	}

	/** The room's lining, its things and the shop front (the door is an open gap). */
	inline void Build(TArray<FShape>& Out, TArray<FLamp>& Lamps)
	{
		using namespace Colors;
		const double W = HalfWidth;
		const double D = Depth;

		// Floorboards, panelled dado, papered walls, a pale ceiling.
		Span(Out, Floor, FVector(-D, -W, -4), FVector(FrameThick, W, 2), false);
		for (double X = -D + 30.0; X < 0.0; X += 60.0)
		{
			Span(Out, FloorDark, FVector(X - 1, -W, 2), FVector(X + 1, W, 2.3), false);
		}
		Span(Out, Paper, FVector(-D, -W, 2), FVector(-D + 4, W, Top), false);
		Span(Out, Panel, FVector(-D + 4, -W, 2), FVector(-D + 6, W, 100), false);
		for (const double Side : { -1.0, 1.0 })
		{
			Span(Out, Paper, FVector(-D, Side * (W - 4), 2), FVector(0, Side * W, Top), false);
			Span(Out, Panel, FVector(-D, Side * (W - 6), 2), FVector(0, Side * (W - 4), 100), false);
		}
		Span(Out, Ceiling, FVector(-D, -W, Top - 4), FVector(0, W, Top), false);

		// Shelves of cloth bolts on the back wall, floor to near the ceiling.
		int32 k = 0;
		for (int32 Row = 0; Row < 5; ++Row)
		{
			const double Z = 30.0 + Row * 55.0;
			Span(Out, Panel, FVector(-D + 4, -W + 20, Z - 3), FVector(-D + 48, W - 20, Z), false);
			for (double Y = -W + 34.0; Y < W - 30.0; Y += 30.0)
			{
				const FLinearColor& C = Cloths[(k++ * 5 + Row) % UE_ARRAY_COUNT(Cloths)];
				Span(Out, C, FVector(-D + 8, Y - 13, Z), FVector(-D + 44, Y + 13, Z + 22 + (k % 3) * 4), false);
			}
		}
		Span(Out, Panel, FVector(-D + 4, -W + 20, 0), FVector(-D + 48, -W + 26, 300));
		Span(Out, Panel, FVector(-D + 4, W - 26, 0), FVector(-D + 48, W - 20, 300));

		// The counter across the left, with green baize, a bolt laid out, shears and a tape.
		Span(Out, Counter, FVector(-400, -W + 30, 0), FVector(-340, 60, 92));
		Span(Out, Baize, FVector(-402, -W + 28, 92), FVector(-338, 62, 95), false);
		Span(Out, Cloths[0], FVector(-392, -150, 95), FVector(-350, -40, 99), false);
		Span(Out, Iron, FVector(-372, 10, 95), FVector(-366, 34, 97), false);
		Add(Out, MarketModel::Cylinder, Gilt, FVector(-375, -200, 97), FVector(12, 12, 4), 0.0, false);

		// A long mirror and the fitting corner with its curtain on the right.
		Span(Out, Gilt, FVector(-260, W - 8, 20), FVector(-140, W - 4, 230), false);
		Span(Out, Mirror, FVector(-252, W - 9, 28), FVector(-148, W - 7, 222), false);
		Span(Out, Iron, FVector(-D + 60, W - 140, 228), FVector(-D + 200, W - 136, 232), false);
		Span(Out, Curtain, FVector(-D + 64, W - 140, 20), FVector(-D + 196, W - 135, 226), false);
		// A chair for the customer to wait on, a hat stand by the door.
		Span(Out, Panel, FVector(-120, -W + 30, 44), FVector(-74, -W + 76, 48), false);
		Span(Out, Panel, FVector(-120, -W + 30, 48), FVector(-116, -W + 76, 98), false);
		for (const FVector2D Leg : { FVector2D(-118, -W + 32), FVector2D(-76, -W + 32), FVector2D(-118, -W + 74), FVector2D(-76, -W + 74) })
		{
			Span(Out, Panel, FVector(Leg.X - 2, Leg.Y - 2, 0), FVector(Leg.X + 2, Leg.Y + 2, 44), false);
		}
		Add(Out, MarketModel::Cylinder, Panel, FVector(-60, W - 40, 90), FVector(5, 5, 180), 0.0, false);
		Add(Out, MarketModel::Cylinder, Panel, FVector(-60, W - 40, 2), FVector(40, 40, 4), 0.0, false);
		Add(Out, MarketModel::Cylinder, FLinearColor(0.14f, 0.13f, 0.12f), FVector(-60, W - 40, 186), FVector(26, 26, 8), 0.0, false);
		// A dummy inside in a brown suit half made, chalk marks on it.
		Dummy(Out, FVector(-200, 60, 0), 200.0, FLinearColor(0.26f, 0.19f, 0.12f));

		// Two lamps in brass shades.
		for (const double X : { -480.0, -200.0 })
		{
			Add(Out, MarketModel::Cylinder, Brass, FVector(X, 0, Top - 30), FVector(2, 2, 60), 0.0, false);
			Add(Out, MarketModel::Cone, Shade, FVector(X, 0, Top - 66), FVector(40, 40, 18), 0.0, false);
			Add(Out, MarketModel::Sphere, Bulb, FVector(X, 0, Top - 78), FVector(12, 12, 12), 0.0, false);
			Lamps.Add({ FVector(X, 0, Top - 90), 650.f });
		}

		// The shop front: dark timber, two display windows and the door between them.
		for (const double Side : { -1.0, 1.0 })
		{
			const double In = Side * DoorHalf;
			const double OutY = Side * (W + 20);
			// Posts at the door and at the ends, the stall riser under the window.
			Span(Out, Front, FVector(0, In, 0), FVector(FrameThick, In + Side * 14, Top + 10));
			Span(Out, Front, FVector(0, OutY - Side * 20, 0), FVector(FrameThick, OutY, Top + 10));
			Span(Out, Front, FVector(0, In + Side * 14, 0), FVector(FrameThick, OutY - Side * 20, 60));
			Span(Out, Gilt, FVector(FrameThick, In + Side * 14, 56), FVector(FrameThick + 1, OutY - Side * 20, 60), false);
			// The window (left open to the eye: only its glazing bars are drawn) with a bar across its top.
			const double Mid = (In + Side * 14 + OutY - Side * 20) * 0.5;
			Span(Out, Front, FVector(6, Mid - 2, 60), FVector(10, Mid + 2, 212), false);
			// The transom over it, black glass with the trade in gilt.
			Span(Out, Front, FVector(6, In + Side * 14, 212), FVector(8, OutY - Side * 20, 270), false);
			Span(Out, Front, FVector(0, In + Side * 14, 270), FVector(FrameThick, OutY - Side * 20, 282), false);
			// A suit on a dummy in each window, a bolt of cloth draped at its foot.
			Dummy(Out, FVector(-50, Side * 200, 60), 0.0, Side < 0 ? FLinearColor(0.09f, 0.09f, 0.10f) : FLinearColor(0.22f, 0.21f, 0.23f));
			Span(Out, Front, FVector(-110, In + Side * 14, 58), FVector(0, OutY - Side * 20, 60), false);
			Span(Out, Cloths[Side < 0 ? 2 : 3], FVector(-60, Side * 120, 60), FVector(-20, Side * 160, 66), false);
		}
		// The door's lintel and the fascia over the whole front.
		Span(Out, Front, FVector(0, -DoorHalf, 270), FVector(FrameThick, DoorHalf, 282), false);
		Span(Out, Front, FVector(0, -HalfWidth - 20, 282), FVector(FrameThick + 6, HalfWidth + 20, Top + 10));
		Span(Out, Gilt, FVector(FrameThick + 6, -HalfWidth - 20, 282), FVector(FrameThick + 7, HalfWidth + 20, 286), false);
		Span(Out, Gilt, FVector(FrameThick + 6, -HalfWidth - 20, Top + 6), FVector(FrameThick + 7, HalfWidth + 20, Top + 10), false);
		// The door leaf stands open inside, against the wall by the left window.
		Span(Out, Front, FVector(-128, -DoorHalf - 4, 2), FVector(-4, -DoorHalf, 266), false);
		Span(Out, Glass, FVector(-118, -DoorHalf - 5, 120), FVector(-14, -DoorHalf - 4, 250), false);
		Span(Out, Brass, FVector(-120, -DoorHalf - 8, 110), FVector(-112, -DoorHalf - 5, 118), false);
	}

	/** The name on the fascia and on the windows. */
	inline void Signs(TArray<FSign>& Out)
	{
		Out.Add({ TEXT("KERT\u00c9SZ \u00c9S FIA"), FVector(FrameThick + 8, 0, 296), 0.0, 34.0, Colors::Gilt });
		Out.Add({ TEXT("\u00daRI SZAB\u00d3"), FVector(9, -200, 232), 0.0, 22.0, Colors::Gilt });
		Out.Add({ TEXT("\u00daRI SZAB\u00d3"), FVector(9, 200, 232), 0.0, 22.0, Colors::Gilt });
	}
}
