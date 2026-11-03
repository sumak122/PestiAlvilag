// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MarketModel.h"
#include "PersonKit.h"

/**
 * The yard at Garay utca 13 for the mission "A felmondás" (from a real 1930s
 * newspaper story): Friedrich Hermann's wholesale shop on the street, with
 * his office at the back, and the timber yard behind the gate that he lets
 * to Schwimmer Mihály: stacks of planks, beams and logs, a lean-to shed, a
 * sawhorse and Schwimmer's little office hut.
 *
 * Header-only, as basic shapes (MarketModel::FShape), so it can be built and
 * looked at outside the engine. The yard's frame: the street front runs
 * along Y at X = 0, +X goes into the yard, Z up from the pavement. The shop
 * covers Y -1300..-100, the gate Y -100..350, the yard wall Y 350..2000.
 */
namespace GarayModel
{
	using MarketModel::FShape;
	using MarketModel::FRandom;

	/** Painted lettering on a sign board: the actor makes it a text in the game. */
	struct FSign
	{
		FString Text;
		/** The middle of the lettering, on the board's face. */
		FVector At;
		/** The way the lettering faces (degrees). */
		double Yaw;
		/** Letter height (cm). */
		double Height;
		FLinearColor Color;
	};

	namespace Colors
	{
		const FLinearColor Plaster(0.55f, 0.46f, 0.32f);
		const FLinearColor PlasterDark(0.40f, 0.33f, 0.24f);
		const FLinearColor FireWall(0.46f, 0.42f, 0.36f);
		const FLinearColor InsideWall(0.74f, 0.70f, 0.60f);
		const FLinearColor Floor(0.42f, 0.30f, 0.18f);
		const FLinearColor Shade(0.10f, 0.22f, 0.14f);
		const FLinearColor Globe(0.95f, 0.88f, 0.70f);
		const FLinearColor Roof(0.30f, 0.16f, 0.10f);
		const FLinearColor RoofTar(0.20f, 0.18f, 0.16f);
		const FLinearColor Fascia(0.06f, 0.12f, 0.08f);
		const FLinearColor Frame(0.10f, 0.07f, 0.05f);
		const FLinearColor Glass(0.14f, 0.18f, 0.20f);
		const FLinearColor Earth(0.33f, 0.28f, 0.21f);
		const FLinearColor Sawdust(0.58f, 0.46f, 0.30f);
		const FLinearColor Pine(0.62f, 0.48f, 0.30f);
		const FLinearColor PineOld(0.50f, 0.40f, 0.27f);
		const FLinearColor Stick(0.25f, 0.18f, 0.11f);
		const FLinearColor Bark(0.22f, 0.15f, 0.09f);
		const FLinearColor Gate(0.20f, 0.12f, 0.06f);
		const FLinearColor Tar(0.07f, 0.06f, 0.05f);
		const FLinearColor Desk(0.20f, 0.10f, 0.05f);
		const FLinearColor Iron(0.05f, 0.05f, 0.055f);
		const FLinearColor Sack(0.55f, 0.48f, 0.34f);
		const FLinearColor Crate(0.48f, 0.36f, 0.20f);
		const FLinearColor Paper(0.75f, 0.70f, 0.58f);
		const FLinearColor Gilt(0.72f, 0.52f, 0.17f);
		const FLinearColor White(0.85f, 0.84f, 0.80f);
	}

	/** Where things are, in the yard's frame (the mission's actor uses these). */
	namespace Spots
	{
		/** On the pavement in front of the shop door, where Schwimmer and you wait for noon. */
		const FVector ShopDoorOutside(-180.0, -700.0, 0.0);
		/** Just inside the shop door. */
		const FVector ShopDoorInside(80.0, -700.0, 0.0);
		/** In front of his counter, where old Hermann stands. */
		const FVector Hermann(300.0, -1030.0, 0.0);
		/** By the office door, where Géza looks over the books. */
		const FVector Geza(470.0, -640.0, 0.0);
		/** The two clerks: one by the window, one by the sacks at the back. */
		const FVector ClerkOne(150.0, -860.0, 0.0);
		const FVector ClerkTwo(350.0, -330.0, 0.0);
		/** The shop's ceiling lamps (their globes), lit by GarayYard. */
		const FVector ShopLamps[] = { FVector(250.0, -900.0, 300.0), FVector(250.0, -420.0, 300.0), FVector(650.0, -1000.0, 290.0) };
		/** The middle of the shop floor. */
		const FVector Floor(280.0, -650.0, 0.0);
		/** In the timber yard, by Schwimmer's hut, where the mission starts. */
		const FVector Schwimmer(1150.0, 1600.0, 0.0);
	}

	/** The yard's far side along the street, and its back wall (cm, in its frame). */
	constexpr double YardEnd = 3200.0;
	constexpr double YardBack = 2400.0;

	/**
	 * Where the game puts the yard: on the north-west side of Garay utca,
	 * on an open lot (none of the map's buildings stand there); the street
	 * front stands a little behind the pavement where people walk (cm, X
	 * east, Y south), and its +X points away from the street.
	 */
	const FVector2D CityAt(74888.0, -51702.0);
	constexpr double CityYaw = -124.03;

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

	/** A plain wooden chair facing +X turned by Yaw (the one Schwimmer once swung). */
	inline void Chair(TArray<FShape>& Out, const FVector& At, double Yaw)
	{
		const FRotator R(0.0, Yaw, 0.0);
		const auto P = [&](double X, double Y, double Z) { return At + R.RotateVector(FVector(X, Y, Z)); };
		Add(Out, MarketModel::Cube, Colors::Desk, P(0, 0, 45), FVector(42, 42, 4), Yaw);
		for (const double X : { -18.0, 18.0 })
		{
			for (const double Y : { -18.0, 18.0 })
			{
				Add(Out, MarketModel::Cube, Colors::Desk, P(X, Y, 22), FVector(4, 4, 44), Yaw, false);
			}
		}
		Add(Out, MarketModel::Cube, Colors::Desk, P(-19, -18, 70), FVector(4, 4, 46), Yaw, false);
		Add(Out, MarketModel::Cube, Colors::Desk, P(-19, 18, 70), FVector(4, 4, 46), Yaw, false);
		Add(Out, MarketModel::Cube, Colors::Desk, P(-19, 0, 84), FVector(3, 40, 14), Yaw, false);
	}

	/** A stack of sawn planks with spacer sticks between the layers, long side along X. */
	inline void PlankStack(TArray<FShape>& Out, const FVector& At, double Length, double Width, int32 Layers, double Yaw, FRandom& R)
	{
		const FRotator Rot(0.0, Yaw, 0.0);
		const auto P = [&](double X, double Y, double Z) { return At + Rot.RotateVector(FVector(X, Y, Z)); };
		// Sleepers on the ground.
		for (const double X : { -0.4, 0.0, 0.4 })
		{
			Add(Out, MarketModel::Cube, Colors::Stick, P(X * Length, 0, 6), FVector(12, Width + 10, 12), Yaw);
		}
		double Z = 12.0;
		const int32 Boards = 3;
		for (int32 L = 0; L < Layers; ++L)
		{
			const double Thick = 5.0;
			for (int32 B = 0; B < Boards; ++B)
			{
				const double W = Width / Boards;
				const double Y = -Width * 0.5 + W * (B + 0.5);
				const double Short = R.Range(0.0, 40.0);
				const double Shift = R.Range(-15.0, 15.0);
				const FLinearColor& C = R.Unit() < 0.3 ? Colors::PineOld : Colors::Pine;
				Add(Out, MarketModel::Cube, C, P(Shift, Y, Z + Thick * 0.5), FVector(Length - Short, W - 1.5, Thick), Yaw, B == 1);
			}
			Z += Thick;
			if (L + 1 < Layers)
			{
				for (const double X : { -0.42, 0.0, 0.42 })
				{
					Add(Out, MarketModel::Cube, Colors::Stick, P(X * Length, 0, Z + 1.5), FVector(4, Width, 3), Yaw, false);
				}
				Z += 3.0;
			}
		}
	}

	/** A pile of round logs lying along X. */
	inline void LogPile(TArray<FShape>& Out, const FVector& At, double Length, int32 Base, double Yaw)
	{
		const FRotator Rot(0.0, Yaw, 0.0);
		const double D = 32.0;
		for (int32 Row = 0; Row < Base; ++Row)
		{
			const int32 Count = Base - Row;
			for (int32 i = 0; i < Count; ++i)
			{
				const double Y = (i - (Count - 1) * 0.5) * D;
				const double Z = D * 0.5 + Row * D * 0.87;
				Add(Out, MarketModel::Cylinder, Colors::Bark, At + Rot.RotateVector(FVector(0, Y, Z)), FVector(D, D, Length), Yaw, true, 90.0);
			}
		}
	}

	/** Friedrich Hermann's shop on the street, with his office at the back. */
	inline void BuildShop(TArray<FShape>& Out, bool bRoof)
	{
		const double Y0 = -1300.0, Y1 = -100.0, Depth = 800.0, H = 380.0, T = 25.0;
		// Floor.
		Span(Out, Colors::Floor, FVector(0, Y0, 0), FVector(Depth, Y1, 4));
		// Street front: piers between a door and two shop windows.
		const double DoorA = -780.0, DoorB = -620.0, DoorH = 235.0, SillZ = 55.0, HeadZ = 250.0;
		const double WinA[2] = { -1220.0, -560.0 }, WinB[2] = { -840.0, -180.0 };
		Span(Out, Colors::Plaster, FVector(0, Y0, 0), FVector(T, WinA[0], H));
		Span(Out, Colors::Plaster, FVector(0, WinB[0], 0), FVector(T, DoorA, H));
		Span(Out, Colors::Plaster, FVector(0, DoorB, 0), FVector(T, WinA[1], H));
		Span(Out, Colors::Plaster, FVector(0, WinB[1], 0), FVector(T, Y1, H));
		Span(Out, Colors::Plaster, FVector(0, DoorA, DoorH), FVector(T, DoorB, H));
		for (int32 i = 0; i < 2; ++i)
		{
			Span(Out, Colors::PlasterDark, FVector(-4, WinA[i], 0), FVector(T, WinB[i], SillZ));
			Span(Out, Colors::Plaster, FVector(0, WinA[i], HeadZ), FVector(T, WinB[i], H));
			Span(Out, Colors::Frame, FVector(-2, WinA[i], SillZ), FVector(T - 6, WinB[i], SillZ + 6));
			Span(Out, Colors::Frame, FVector(-2, WinA[i], HeadZ - 6), FVector(T - 6, WinB[i], HeadZ));
			const double Mid = (WinA[i] + WinB[i]) * 0.5;
			Span(Out, Colors::Frame, FVector(-2, Mid - 3, SillZ), FVector(T - 6, Mid + 3, HeadZ));
			Span(Out, Colors::Glass, FVector(10, WinA[i], SillZ + 6), FVector(12, WinB[i], HeadZ - 6), true);
		}
		// The door stands open, folded back against the wall inside.
		Span(Out, Colors::Frame, FVector(T, DoorB, 4), FVector(T + 5, DoorB + 155, DoorH - 5), false);
		// Plinth and the green fascia board (the lettering is a sign).
		Span(Out, Colors::PlasterDark, FVector(-6, Y0, 0), FVector(0, WinA[0], 40), false);
		Span(Out, Colors::Fascia, FVector(-10, Y0 + 60, 268), FVector(0, Y1 - 60, 336));
		Span(Out, Colors::Gilt, FVector(-12, Y0 + 60, 334), FVector(0, Y1 - 60, 338), false);
		Span(Out, Colors::Gilt, FVector(-12, Y0 + 60, 266), FVector(0, Y1 - 60, 270), false);
		// Side and back walls; the back has a door to the yard.
		Span(Out, Colors::FireWall, FVector(0, Y0 - T, 0), FVector(Depth, Y0, H));
		Span(Out, Colors::Plaster, FVector(0, Y1, 0), FVector(Depth, Y1 + T, H));
		Span(Out, Colors::Plaster, FVector(Depth - T, Y0, 0), FVector(Depth, -470.0, H));
		Span(Out, Colors::Plaster, FVector(Depth - T, -360.0, 0), FVector(Depth, Y1, H));
		Span(Out, Colors::Plaster, FVector(Depth - T, -470.0, 225), FVector(Depth, -360.0, H));
		// Inside: cream walls, shelves of goods along the side and a long counter in front of them.
		Span(Out, Colors::InsideWall, FVector(T, Y0, 4), FVector(Depth - T, Y0 + 3, H - 10), false);
		Span(Out, Colors::InsideWall, FVector(T, Y1 - 3, 4), FVector(Depth - T, Y1, H - 10), false);
		Span(Out, Colors::InsideWall, FVector(Depth - T - 3, Y0, 4), FVector(Depth - T, -470.0, H - 10), false);
		Span(Out, Colors::InsideWall, FVector(Depth - T - 3, -360.0, 4), FVector(Depth - T, Y1, H - 10), false);
		Span(Out, Colors::Desk, FVector(110, -1150, 4), FVector(480, -1110, 100));
		Span(Out, Colors::Desk, FVector(105, -1155, 100), FVector(485, -1105, 104));
		for (int32 Shelf = 0; Shelf < 4; ++Shelf)
		{
			const double Z = 30.0 + Shelf * 55.0;
			Span(Out, Colors::Desk, FVector(T + 10, Y0 + 3, Z), FVector(500, Y0 + 50, Z + 4));
		}
		for (int32 i = 0; i < 9; ++i)
		{
			const double X = 70.0 + i * 47.0;
			for (int32 Shelf = 0; Shelf < 4; ++Shelf)
			{
				const double Z = 34.0 + Shelf * 55.0;
				const bool bBox = ((i + Shelf) % 3) != 0;
				Add(Out, bBox ? MarketModel::Cube : MarketModel::Cylinder, bBox ? Colors::Crate : Colors::Paper,
					FVector(X, Y0 + 26, Z + 16), FVector(38, 36, 32), 0.0, false);
			}
		}
		// Sacks and crates by the yard door (a wholesaler's stock).
		for (int32 i = 0; i < 4; ++i)
		{
			Add(Out, MarketModel::Cylinder, Colors::Sack, FVector(560 + (i % 2) * 65, -260 - (i / 2) * 65, 36), FVector(52, 48, 66), i * 30.0);
		}
		Span(Out, Colors::Crate, FVector(660, -500, 4), FVector(740, -400, 70));
		Span(Out, Colors::Crate, FVector(670, -490, 70), FVector(730, -410, 120));
		// The office at the back left: a partition, wood below and glass above.
		const double OX = 520.0, OY = -700.0;
		Span(Out, Colors::Desk, FVector(OX, Y0, 4), FVector(OX + 8, -810.0, 110));
		Span(Out, Colors::Glass, FVector(OX + 2, Y0, 110), FVector(OX + 6, -810.0, 230), true);
		Span(Out, Colors::Desk, FVector(OX, Y0, 230), FVector(OX + 8, OY, 240));
		Span(Out, Colors::Desk, FVector(OX, OY - 8, 4), FVector(Depth - T, OY, 110));
		Span(Out, Colors::Glass, FVector(OX, OY - 6, 110), FVector(Depth - T, OY - 2, 230), true);
		Span(Out, Colors::Desk, FVector(OX, OY - 8, 230), FVector(Depth - T, OY, 240));
		// Desk, chairs, a safe and a stove inside the office.
		Span(Out, Colors::Desk, FVector(620, -1220, 4), FVector(740, -1080, 76));
		Span(Out, Colors::Paper, FVector(650, -1190, 76), FVector(690, -1150, 78), false);
		Chair(Out, FVector(700, -1040, 0), 180.0);
		Chair(Out, FVector(580, -1150, 0), -10.0);
		Chair(Out, FVector(600, -900, 0), 30.0);
		Span(Out, Colors::Iron, FVector(700, -820, 4), FVector(770, -740, 130));
		Add(Out, MarketModel::Cylinder, Colors::Iron, FVector(560, -760, 60), FVector(50, 50, 115));
		// A plain chair for customers by the window.
		Chair(Out, FVector(90, -1040, 0), 20.0);
		// Lamps hanging from the ceiling: a rod, a green enamel shade and a glass globe (the yard lights them).
		for (const FVector& Lamp : Spots::ShopLamps)
		{
			Add(Out, MarketModel::Cylinder, Colors::Iron, Lamp + FVector(0, 0, (H - 10 - Lamp.Z) * 0.5), FVector(2, 2, H - 10 - Lamp.Z), 0.0, false);
			Add(Out, MarketModel::Cone, Colors::Shade, Lamp + FVector(0, 0, 12), FVector(46, 46, 20), 0.0, false);
			Add(Out, MarketModel::Sphere, Colors::Globe, Lamp, FVector(22, 22, 22), 0.0, false);
		}
		if (bRoof)
		{
			Span(Out, Colors::InsideWall, FVector(0, Y0, H - 10), FVector(Depth, Y1, H), false);
			// A flat tarred roof behind a moulded cornice.
			Span(Out, Colors::RoofTar, FVector(0, Y0, H), FVector(Depth, Y1 + T, H + 12), false);
			Span(Out, Colors::PlasterDark, FVector(-25, Y0, H - 14), FVector(0, Y1 + T, H + 4), false);
			Span(Out, Colors::Plaster, FVector(-12, Y0, H + 4), FVector(10, Y1 + T, H + 40), false);
			Span(Out, Colors::PlasterDark, FVector(-18, Y0, H + 40), FVector(12, Y1 + T, H + 48), false);
		}
	}

	/** The timber yard behind the gate that Schwimmer rents. */
	inline void BuildYard(TArray<FShape>& Out)
	{
		FRandom R(1932);
		const double Y1 = YardEnd, Back = YardBack, Wall = 280.0;
		// Packed earth with sawdust, the yard of the shop as well.
		Span(Out, Colors::Earth, FVector(0, -1300, -2), FVector(Back, Y1, 1), false);
		Span(Out, Colors::Sawdust, FVector(900, 400, 1), FVector(2000, Y1 - 300, 2), false);
		// Street wall with the gate; the gate's two wings stand open against the walls.
		Span(Out, Colors::Plaster, FVector(0, 350, 0), FVector(30, Y1, Wall));
		Span(Out, Colors::PlasterDark, FVector(-4, 350, Wall - 20), FVector(34, Y1, Wall));
		Span(Out, Colors::Plaster, FVector(0, -100, 0), FVector(30, -60, Wall + 40));
		Span(Out, Colors::Plaster, FVector(0, 310, 0), FVector(30, 350, Wall + 40));
		Span(Out, Colors::Gate, FVector(30, -58, 5), FVector(220, -50, 255));
		Span(Out, Colors::Gate, FVector(30, 298, 5), FVector(220, 306, 255));
		// Schwimmer's sign board on the street wall.
		Span(Out, Colors::White, FVector(-8, 520, 150), FVector(0, 1830, 250));
		Span(Out, Colors::Tar, FVector(-10, 520, 148), FVector(0, 1830, 152), false);
		Span(Out, Colors::Tar, FVector(-10, 520, 248), FVector(0, 1830, 252), false);
		// Neighbours' bare fire walls round the yard.
		Span(Out, Colors::FireWall, FVector(800, -1330, 0), FVector(Back, -1300, 1500));
		Span(Out, Colors::FireWall, FVector(0, Y1, 0), FVector(Back, Y1 + 30, 1500));
		Span(Out, Colors::FireWall, FVector(Back, -1330, 0), FVector(Back + 30, Y1 + 30, 1500));
		// Lean-to shed along the back wall for the dry timber.
		for (int32 i = 0; i <= 8; ++i)
		{
			const double Y = -900.0 + i * 500.0;
			Add(Out, MarketModel::Cube, Colors::Stick, FVector(Back - 380, Y, 160), FVector(14, 14, 320));
		}
		Add(Out, MarketModel::Cube, Colors::Roof, FVector(Back - 200, (Y1 - 1000.0) * 0.5, 360), FVector(440, Y1 + 1000.0, 8), 0.0, false, 12.0);
		for (int32 i = 0; i < 7; ++i)
		{
			PlankStack(Out, FVector(Back - 200, -650 + i * 520, 0), 420.0, 110.0, 10 + i % 3, 90.0, R);
		}
		// Open stacks in the yard, and the round logs waiting to be sawn.
		PlankStack(Out, FVector(1200, 700, 0), 520.0, 130.0, 13, 0.0, R);
		PlankStack(Out, FVector(1200, 1050, 0), 480.0, 120.0, 9, 0.0, R);
		PlankStack(Out, FVector(1750, 650, 0), 500.0, 120.0, 11, 4.0, R);
		LogPile(Out, FVector(1750, 1500, 0), 500.0, 4, 0.0);
		PlankStack(Out, FVector(1250, 2350, 0), 500.0, 130.0, 12, 0.0, R);
		PlankStack(Out, FVector(1250, 2750, 0), 460.0, 120.0, 8, -3.0, R);
		LogPile(Out, FVector(1850, 2550, 0), 520.0, 5, 0.0);
		// Squared beams leaning on the side fire wall.
		for (int32 i = 0; i < 7; ++i)
		{
			Add(Out, MarketModel::Cube, Colors::PineOld, FVector(900 + i * 26, Y1 - 45, 210), FVector(14, 14, 440), 0.0, true, 0.0, -12.0);
		}
		// Sawhorse with a plank on it, and a handcart.
		for (const double X : { -50.0, 50.0 })
		{
			Add(Out, MarketModel::Cube, Colors::Stick, FVector(1450 + X, 1250 - 120, 40), FVector(8, 8, 90), 0.0, false, 0.0, 22.0);
			Add(Out, MarketModel::Cube, Colors::Stick, FVector(1450 + X, 1250 - 80, 40), FVector(8, 8, 90), 0.0, false, 0.0, -22.0);
		}
		Add(Out, MarketModel::Cube, Colors::Pine, FVector(1450, 1150, 82), FVector(240, 24, 4), 8.0);
		Span(Out, Colors::Gate, FVector(600, 1300, 50), FVector(780, 1420, 60));
		Add(Out, MarketModel::Cylinder, Colors::Iron, FVector(690, 1290, 40), FVector(80, 80, 6), 0.0, false, 0.0, 90.0);
		Add(Out, MarketModel::Cylinder, Colors::Iron, FVector(690, 1430, 40), FVector(80, 80, 6), 0.0, false, 0.0, 90.0);
		Add(Out, MarketModel::Cube, Colors::Gate, FVector(530, 1360, 55), FVector(160, 6, 6), 0.0, false, -12.0);
		// Schwimmer's office hut: boards, a door and a little window.
		const FVector H0(1250, 1450, 0), H1(1550, 1800, 260);
		Span(Out, Colors::PineOld, FVector(H0.X, H0.Y, 0), FVector(H1.X, H0.Y + 8, 260));
		Span(Out, Colors::PineOld, FVector(H0.X, H1.Y - 8, 0), FVector(H1.X, H1.Y, 260));
		Span(Out, Colors::PineOld, FVector(H1.X - 8, H0.Y, 0), FVector(H1.X, H1.Y, 260));
		Span(Out, Colors::PineOld, FVector(H0.X, H0.Y, 0), FVector(H0.X + 8, 1560, 260));
		Span(Out, Colors::PineOld, FVector(H0.X, 1660, 0), FVector(H0.X + 8, H1.Y, 260));
		Span(Out, Colors::PineOld, FVector(H0.X, 1560, 210), FVector(H0.X + 8, 1660, 260));
		Span(Out, Colors::Glass, FVector(H0.X - 1, 1690, 120), FVector(H0.X + 2, 1770, 190), false);
		Add(Out, MarketModel::Cube, Colors::Tar, FVector(1400, 1625, 285), FVector(360, 410, 8), 0.0, false, 0.0, -8.0);
		Span(Out, Colors::White, FVector(H0.X - 4, 1470, 205), FVector(H0.X, 1550, 245), false);
	}

	/** The whole place: shop, office, gate and timber yard. */
	inline void Build(TArray<FShape>& Out, bool bRoof = true)
	{
		BuildShop(Out, bRoof);
		BuildYard(Out);
	}

	/** The painted lettering on the shop fascia, the yard wall and the hut door. */
	inline void Signs(TArray<FSign>& Out)
	{
		Out.Add({ TEXT("FRIEDRICH HERMANN  \u00b7  NAGYKERESKED\u0150"), FVector(-11, -700, 302), 180.0, 30.0, Colors::Gilt });
		Out.Add({ TEXT("SCHWIMMER MIH\u00c1LY  FAKERESKED\u00c9S  \u00b7  GARAY U. 13."), FVector(-9, 1175, 200), 180.0, 40.0, Colors::Tar });
		Out.Add({ TEXT("IRODA"), FVector(1248, 1510, 225), 180.0, 18.0, Colors::Tar });
	}

	/** Schwimmer Mihály, the timber merchant: stout, a moustache, a cap and a dusty brown coat. */
	inline PersonKit::FOutfit Schwimmer()
	{
		PersonKit::FOutfit O;
		O.Height = 174.f; O.Build = 1.2f; O.bMoustache = true;
		O.Hat = PersonKit::EHat::FlatCap; O.HatColor = FLinearColor(0.20f, 0.16f, 0.11f);
		O.Top = PersonKit::ETop::Jacket; O.Coat = FLinearColor(0.26f, 0.17f, 0.09f);
		O.Waistcoat = FLinearColor(0.12f, 0.09f, 0.06f); O.bTie = false; O.bBoots = true;
		O.Trousers = FLinearColor(0.14f, 0.12f, 0.10f); O.Shoes = FLinearColor(0.06f, 0.04f, 0.03f);
		O.Hair = FLinearColor(0.12f, 0.08f, 0.05f); O.Skin = FLinearColor(0.70f, 0.46f, 0.34f);
		O.bApron = true; O.Apron = FLinearColor(0.32f, 0.26f, 0.18f);
		return O;
	}

	/** Friedrich Hermann, 67, the wholesale merchant: grey, a white moustache, waistcoat and tie. */
	inline PersonKit::FOutfit Hermann()
	{
		PersonKit::FOutfit O;
		O.Height = 170.f; O.Build = 1.1f; O.bMoustache = true;
		O.Hat = PersonKit::EHat::None; O.Hair = FLinearColor(0.62f, 0.60f, 0.56f);
		O.Top = PersonKit::ETop::Waistcoat; O.Waistcoat = FLinearColor(0.10f, 0.10f, 0.12f);
		O.Tie = FLinearColor(0.08f, 0.10f, 0.20f); O.Trousers = FLinearColor(0.16f, 0.16f, 0.18f);
		O.Skin = FLinearColor(0.72f, 0.52f, 0.42f);
		return O;
	}

	/** Friedrich Géza, the son, an engineer: a slim grey suit. */
	inline PersonKit::FOutfit Geza()
	{
		PersonKit::FOutfit O;
		O.Height = 183.f; O.Build = 0.95f;
		O.Hat = PersonKit::EHat::None; O.Hair = FLinearColor(0.08f, 0.06f, 0.04f);
		O.Top = PersonKit::ETop::Jacket; O.Coat = FLinearColor(0.24f, 0.24f, 0.26f);
		O.Tie = FLinearColor(0.35f, 0.07f, 0.05f); O.Trousers = FLinearColor(0.22f, 0.22f, 0.24f);
		return O;
	}

	/** One of Hermann's two clerks: shirt sleeves, a waistcoat and a grey apron (Which 0 or 1). */
	inline PersonKit::FOutfit Clerk(int32 Which)
	{
		PersonKit::FOutfit O;
		O.Hat = PersonKit::EHat::None;
		O.Top = PersonKit::ETop::Waistcoat;
		O.bApron = true;
		O.Apron = FLinearColor(0.42f, 0.42f, 0.40f);
		if (Which == 0)
		{
			O.Height = 176.f; O.Build = 1.05f; O.bMoustache = true;
			O.Waistcoat = FLinearColor(0.18f, 0.14f, 0.10f); O.Tie = FLinearColor(0.10f, 0.20f, 0.12f);
			O.Hair = FLinearColor(0.20f, 0.13f, 0.07f);
		}
		else
		{
			O.Height = 168.f; O.Build = 0.92f;
			O.Waistcoat = FLinearColor(0.10f, 0.11f, 0.16f); O.bTie = false;
			O.Hair = FLinearColor(0.05f, 0.04f, 0.03f); O.Skin = FLinearColor(0.76f, 0.56f, 0.44f);
		}
		return O;
	}
}
