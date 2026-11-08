// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BozsiHouseModel.h"
#include "MarketModel.h"
#include "PersonKit.h"

/**
 * The car show in the Tattersall hall on Kerepesi út (the SSK chain):
 * - the hall: a stone front with tall shop windows and glass doors in the
 *   middle (the doors are their own list, BuildDoors, so the SSK can smash
 *   out through them), the show's banner, a glass roof on iron trusses;
 * - inside: the SSK on a round turntable behind brass posts and red ropes,
 *   two more cars on low stands, palms, show boards, hanging lamps;
 * - at the back the importer's glazed office with the key cabinet, and the
 *   side door with the night watchman's table and lantern by it;
 * - the people: Ottó bácsi the night watchman, the importer's chauffeur,
 *   and a show mechanic in overalls.
 *
 * Header-only, as basic shapes (MarketModel::FShape). The hall's frame:
 * its front along Y at X = 0, +X out across the pavement to the street,
 * the hall running back to -X, Z up from the pavement.
 */
namespace CarShowModel
{
	using MarketModel::FShape;
	using BozsiHouseModel::FSign;
	using BozsiHouseModel::FLamp;
	using BozsiHouseModel::Add;
	using BozsiHouseModel::Span;

	namespace Colors
	{
		const FLinearColor Stone(0.62f, 0.56f, 0.45f);
		const FLinearColor StoneDark(0.46f, 0.41f, 0.33f);
		const FLinearColor Plaster(0.70f, 0.66f, 0.56f);
		const FLinearColor Wainscot(0.20f, 0.13f, 0.08f);
		const FLinearColor Floor(0.42f, 0.38f, 0.32f);
		const FLinearColor FloorDark(0.22f, 0.18f, 0.15f);
		const FLinearColor Glass(0.12f, 0.15f, 0.17f);
		const FLinearColor Iron(0.06f, 0.065f, 0.07f);
		const FLinearColor Bronze(0.36f, 0.24f, 0.10f);
		const FLinearColor Brass(0.66f, 0.48f, 0.16f);
		const FLinearColor Velvet(0.38f, 0.04f, 0.05f);
		const FLinearColor Stand(0.12f, 0.10f, 0.09f);
		const FLinearColor StandTop(0.55f, 0.08f, 0.08f);
		const FLinearColor Banner(0.80f, 0.74f, 0.58f);
		const FLinearColor BannerText(0.40f, 0.05f, 0.04f);
		const FLinearColor Lettering(0.80f, 0.64f, 0.28f);
		const FLinearColor Pot(0.40f, 0.22f, 0.12f);
		const FLinearColor Palm(0.10f, 0.22f, 0.08f);
		const FLinearColor Board(0.08f, 0.07f, 0.06f);
		const FLinearColor Paper(0.86f, 0.82f, 0.70f);
		const FLinearColor Wood(0.30f, 0.17f, 0.08f);
		const FLinearColor Desk(0.22f, 0.12f, 0.06f);
		const FLinearColor Safe(0.12f, 0.16f, 0.12f);
		const FLinearColor Globe(0.95f, 0.88f, 0.70f);
		const FLinearColor Door(0.16f, 0.10f, 0.06f);
		const FLinearColor Flag(0.62f, 0.08f, 0.06f);
		const FLinearColor FlagWhite(0.85f, 0.83f, 0.78f);
		const FLinearColor FlagGreen(0.08f, 0.32f, 0.12f);
	}

	// Sizes (cm).
	constexpr double HalfWidth = 900.0;    // the hall, Y -900..900
	constexpr double Depth = 2400.0;       // from the front back to X = -2400
	constexpr double WallTop = 760.0;
	constexpr double RidgeTop = 1120.0;
	constexpr double DoorHalf = 240.0;     // the glass doors, Y -240..240
	constexpr double DoorTop = 400.0;
	constexpr double WallThick = 40.0;
	constexpr double OfficeX = -1950.0;    // the office's glass front
	constexpr double OfficeY = 380.0;      // the office runs from here to the right wall
	constexpr double SideDoorX = -1700.0;  // the side door in the left wall (Y = -HalfWidth)

	/** Where things are, in the hall's frame. */
	namespace Spots
	{
		/** The SSK on its turntable, facing the doors. */
		const FVector Ssk(-1050.0, 0.0, 30.0);
		constexpr double SskYaw = 0.0;
		/** The two other cars on their stands, half turned to the door. */
		const FVector CarLeft(-650.0, -560.0, 12.0);
		constexpr double CarLeftYaw = 35.0;
		const FVector CarRight(-650.0, 560.0, 12.0);
		constexpr double CarRightYaw = -35.0;
		/** The key cabinet on the office's back wall, and where the player stands to take the key. */
		const FVector KeyCabinet(-2370.0, 700.0, 150.0);
		const FVector TakeKey(-2280.0, 700.0, 0.0);
		/** The side door (its middle, on the wall's outer face) and the watchman's chair by it. */
		const FVector SideDoor(SideDoorX, -HalfWidth - WallThick * 0.5, 0.0);
		const FVector Watchman(-1580.0, -760.0, 0.0);
		constexpr double WatchmanYaw = 120.0;
		/** Just outside the glass doors, where the SSK lands on the pavement. */
		const FVector OutOfDoors(450.0, 0.0, 0.0);
	}

	/** Ottó bácsi, the night watchman: old, heavy, grey moustache, a peaked cap and a long dark coat, a stick. */
	inline PersonKit::FOutfit Watchman()
	{
		PersonKit::FOutfit O;
		O.Height = 172.f;
		O.Build = 1.2f;
		O.Hat = PersonKit::EHat::FlatCap;
		O.HatColor = FLinearColor(0.08f, 0.09f, 0.12f);
		O.Top = PersonKit::ETop::Overcoat;
		O.Coat = FLinearColor(0.10f, 0.11f, 0.14f);
		O.bMoustache = true;
		O.Hair = FLinearColor(0.55f, 0.54f, 0.52f);
		O.Held = PersonKit::EHeld::Baton;
		O.HeldColor = FLinearColor(0.25f, 0.15f, 0.07f);
		O.bBoots = true;
		return O;
	}

	/** The importer's chauffeur: young, a grey uniform coat buttoned up, peaked cap, leather boots, gloves. */
	inline PersonKit::FOutfit Chauffeur()
	{
		PersonKit::FOutfit O;
		O.Height = 180.f;
		O.Hat = PersonKit::EHat::FlatCap;
		O.HatColor = FLinearColor(0.20f, 0.21f, 0.22f);
		O.Top = PersonKit::ETop::Overcoat;
		O.Coat = FLinearColor(0.24f, 0.25f, 0.26f);
		O.bTie = false;
		O.bBoots = true;
		O.Shoes = FLinearColor(0.05f, 0.03f, 0.02f);
		O.Hair = FLinearColor(0.18f, 0.12f, 0.06f);
		return O;
	}

	/** A show mechanic in blue overalls and a cap (the player wears the same once he has stolen a pair). */
	inline PersonKit::FOutfit Mechanic()
	{
		PersonKit::FOutfit O;
		O.Height = 176.f;
		O.Hat = PersonKit::EHat::FlatCap;
		O.HatColor = FLinearColor(0.10f, 0.14f, 0.24f);
		O.Top = PersonKit::ETop::Jacket;
		O.Coat = FLinearColor(0.12f, 0.18f, 0.32f);
		O.Trousers = FLinearColor(0.12f, 0.18f, 0.32f);
		O.bTie = false;
		O.bBoots = true;
		return O;
	}

	/** A potted palm at At (the pot's foot). */
	inline void PottedPalm(TArray<FShape>& Out, const FVector& At)
	{
		Add(Out, MarketModel::Cylinder, Colors::Pot, At + FVector(0, 0, 30), FVector(56, 56, 60));
		Add(Out, MarketModel::Cylinder, Colors::Wood, At + FVector(0, 0, 110), FVector(8, 8, 100), 0.0, false);
		for (int32 i = 0; i < 7; ++i)
		{
			const double Yaw = i * 360.0 / 7.0;
			const FVector Out2 = FRotator(0.0, Yaw, 0.0).RotateVector(FVector(55, 0, 0));
			Add(Out, MarketModel::Cube, Colors::Palm, At + FVector(0, 0, 160) + Out2, FVector(120, 26, 3), Yaw, false, -25.0);
		}
	}

	/** A brass post for the ropes, at At (its foot). */
	inline void RopePost(TArray<FShape>& Out, const FVector& At)
	{
		Add(Out, MarketModel::Cylinder, Colors::Brass, At + FVector(0, 0, 2), FVector(28, 28, 4), 0.0, false);
		Add(Out, MarketModel::Cylinder, Colors::Brass, At + FVector(0, 0, 50), FVector(6, 6, 96), 0.0, false);
		Add(Out, MarketModel::Sphere, Colors::Brass, At + FVector(0, 0, 100), FVector(10, 10, 10), 0.0, false);
	}

	/** A sagging red rope between two posts' tops. */
	inline void Rope(TArray<FShape>& Out, const FVector& A, const FVector& B)
	{
		const FVector Mid = (A + B) * 0.5 - FVector(0, 0, 12);
		for (const FVector& End : { A, B })
		{
			const FVector D = Mid - End;
			const double Yaw = FMath::RadiansToDegrees(FMath::Atan2(D.Y, D.X));
			const double Pitch = FMath::RadiansToDegrees(FMath::Atan2(D.Z, FVector2D(D.X, D.Y).Size()));
			Add(Out, MarketModel::Cylinder, Colors::Velvet, (End + Mid) * 0.5, FVector(4, 4, D.Size()), Yaw, false, Pitch - 90.0);
		}
	}

	/** A low display stand for a car at At (its top middle), turned Yaw. */
	inline void CarStand(TArray<FShape>& Out, const FVector& At, double Yaw)
	{
		Add(Out, MarketModel::Cube, Colors::Stand, At - FVector(0, 0, At.Z * 0.5), FVector(560, 260, At.Z), Yaw);
		Add(Out, MarketModel::Cube, Colors::StandTop, At - FVector(0, 0, 0.5), FVector(540, 240, 1), Yaw, false);
	}

	/** A show board on an easel at At (its foot), facing Yaw. */
	inline void ShowBoard(TArray<FShape>& Out, const FVector& At, double Yaw)
	{
		const FRotator Turn(0.0, Yaw, 0.0);
		for (const double Side : { -1.0, 1.0 })
		{
			Add(Out, MarketModel::Cube, Colors::Wood, At + Turn.RotateVector(FVector(0, Side * 34, 80)), FVector(4, 4, 160), Yaw, false, 8.0);
		}
		Add(Out, MarketModel::Cube, Colors::Board, At + Turn.RotateVector(FVector(4, 0, 120)), FVector(3, 80, 64), Yaw, false, 8.0);
		Add(Out, MarketModel::Cube, Colors::Paper, At + Turn.RotateVector(FVector(6, 0, 120)), FVector(1, 70, 54), Yaw, false, 8.0);
	}

	/** The hall's shell: front, walls, the glass roof on its trusses, the floor. */
	inline void BuildHall(TArray<FShape>& Out, TArray<FLamp>& Lamps)
	{
		using namespace Colors;
		const double W = HalfWidth;
		const double T = WallThick;
		// The floor, in big squares.
		Span(Out, Floor, FVector(-Depth, -W, -1), FVector(0, W, 1), false);
		for (double X = -Depth + 100.0; X < -40.0; X += 200.0)
		{
			for (double Y = -W + 100.0; Y < W - 40.0; Y += 200.0)
			{
				if ((static_cast<int32>((X + Depth) / 200.0) + static_cast<int32>((Y + W) / 200.0)) % 2 == 0)
				{
					Span(Out, FloorDark, FVector(X - 100, Y - 100, 1), FVector(X + 100, Y + 100, 1.5), false);
				}
			}
		}
		// The side and back walls, with a dark wainscot inside.
		Span(Out, Plaster, FVector(-Depth, -W - T, 0), FVector(0, -W, WallTop));
		Span(Out, Plaster, FVector(-Depth, W, 0), FVector(0, W + T, WallTop));
		Span(Out, Plaster, FVector(-Depth - T, -W - T, 0), FVector(-Depth, W + T, WallTop));
		Span(Out, Wainscot, FVector(-Depth, -W, 0), FVector(-40, -W + 2, 120), false);
		Span(Out, Wainscot, FVector(-Depth, W - 2, 0), FVector(-40, W, 120), false);
		// The front: stone piers, the shop windows between them, the glass doors' frame, the fascia and cornice.
		for (const double Y : { -W, -560.0, -DoorHalf, DoorHalf, 560.0, W })
		{
			const double Half = (Y == -W || Y == W) ? 60.0 : 40.0;
			Span(Out, Stone, FVector(-T, Y - Half, 0), FVector(20, Y + Half, WallTop));
			Span(Out, StoneDark, FVector(20, Y - Half - 4, 0), FVector(26, Y + Half + 4, 40), false);
		}
		for (const double Side : { -1.0, 1.0 })
		{
			for (const double Y0 : { DoorHalf + 40.0, 600.0 })
			{
				const double Y1 = Y0 == 600.0 ? W - 60.0 : 520.0;
				const double A = Side * Y0;
				const double B = Side * Y1;
				Span(Out, StoneDark, FVector(-T, A, 0), FVector(10, B, 70));
				Span(Out, Glass, FVector(-12, A, 70), FVector(-8, B, 470));
				Span(Out, Iron, FVector(-8, A, 466), FVector(-4, B, 474), false);
				Span(Out, Iron, FVector(-8, (A + B) * 0.5 - 3, 70), FVector(-4, (A + B) * 0.5 + 3, 470), false);
				Span(Out, Stone, FVector(-T, A, 470), FVector(10, B, 500));
			}
		}
		// Over the doors: a glazed fanlight in iron bars.
		Span(Out, Iron, FVector(-10, -DoorHalf, DoorTop), FVector(-2, DoorHalf, DoorTop + 10), false);
		Span(Out, Glass, FVector(-12, -DoorHalf, DoorTop + 10), FVector(-8, DoorHalf, 500));
		for (double Y = -DoorHalf + 60.0; Y < DoorHalf; Y += 60.0)
		{
			Span(Out, Iron, FVector(-8, Y - 2, DoorTop + 10), FVector(-4, Y + 2, 500), false);
		}
		// The fascia for the lettering, the cornice and a parapet with the hall's name.
		Span(Out, Stone, FVector(-T, -W - 60, 500), FVector(14, W + 60, 640));
		Span(Out, StoneDark, FVector(14, -W - 64, 632), FVector(30, W + 64, 650), false);
		Span(Out, Stone, FVector(-T, -W - 60, 650), FVector(10, W + 60, WallTop));
		Span(Out, Stone, FVector(-T, -340, WallTop), FVector(10, 340, 900));
		Span(Out, StoneDark, FVector(10, -350, 890), FVector(22, 350, 910), false);
		// The show's banner, hung under the cornice on two iron arms.
		Span(Out, Banner, FVector(30, -620, 520), FVector(32, 620, 620), false);
		for (const double Y : { -600.0, 600.0 })
		{
			Span(Out, Iron, FVector(10, Y - 2, 616), FVector(34, Y + 2, 620), false);
		}
		// Flags on the parapet: the tricolour either side of the name.
		for (const double Y : { -300.0, 300.0 })
		{
			Add(Out, MarketModel::Cylinder, Iron, FVector(20, Y, 960), FVector(5, 5, 300), 0.0, false, 0.0, 0.0);
			Span(Out, Flag, FVector(19, Y + 4, 1050), FVector(21, Y + 120, 1074), false);
			Span(Out, FlagWhite, FVector(19, Y + 4, 1026), FVector(21, Y + 120, 1050), false);
			Span(Out, FlagGreen, FVector(19, Y + 4, 1002), FVector(21, Y + 120, 1026), false);
		}
		// The roof: iron trusses across the hall, glass slopes, a lantern ridge.
		for (double X = -Depth + 200.0; X < -100.0; X += 400.0)
		{
			for (const double Side : { -1.0, 1.0 })
			{
				const double Run = W + T;
				const double Len = FMath::Sqrt(Run * Run + (RidgeTop - WallTop) * (RidgeTop - WallTop));
				const double Slope = FMath::RadiansToDegrees(FMath::Atan2(RidgeTop - WallTop, Run));
				Add(Out, MarketModel::Cube, Iron, FVector(X, Side * Run * 0.5, (WallTop + RidgeTop) * 0.5), FVector(12, Len, 18), 0.0, false, 0.0, Side * Slope);
				Add(Out, MarketModel::Cube, Iron, FVector(X, Side * Run * 0.5, WallTop + 20.0), FVector(8, Run, 6), 0.0, false);
				for (double F = 0.2; F < 0.95; F += 0.2)
				{
					const double Y = Side * Run * F;
					const double Z = WallTop + (RidgeTop - WallTop) * (1.0 - F);
					Add(Out, MarketModel::Cube, Iron, FVector(X, Y, (Z + WallTop + 20.0) * 0.5), FVector(5, 5, Z - WallTop - 20.0), 0.0, false);
				}
			}
		}
		for (const double Side : { -1.0, 1.0 })
		{
			const double Run = W + T;
			const double Len = FMath::Sqrt(Run * Run + (RidgeTop - WallTop) * (RidgeTop - WallTop));
			const double Slope = FMath::RadiansToDegrees(FMath::Atan2(RidgeTop - WallTop, Run));
			Add(Out, MarketModel::Cube, Glass, FVector(-Depth * 0.5, Side * Run * 0.5, (WallTop + RidgeTop) * 0.5 + 10.0), FVector(Depth, Len, 3), 0.0, false, 0.0, Side * Slope);
		}
		Span(Out, Iron, FVector(-Depth, -20, RidgeTop - 4), FVector(0, 20, RidgeTop + 30), false);
		// The hanging lamps down the middle and over the cars.
		for (const FVector& At : { FVector(-300, 0, 520), FVector(-650, -560, 520), FVector(-650, 560, 520), FVector(-1050, 0, 560), FVector(-1500, 0, 520), FVector(-1500, -560, 520) })
		{
			Add(Out, MarketModel::Cylinder, Iron, At + FVector(0, 0, (WallTop - At.Z) * 0.5 + 20.0), FVector(2, 2, WallTop - At.Z + 40.0), 0.0, false);
			Add(Out, MarketModel::Cone, Iron, At + FVector(0, 0, 14), FVector(70, 70, 26), 0.0, false);
			Add(Out, MarketModel::Sphere, Globe, At, FVector(26, 26, 26), 0.0, false);
			Lamps.Add({ At, 900.f });
		}
	}

	/** The glass doors, in their own list so they can be smashed: two tall leaves of plate glass in bronze frames. */
	inline void BuildDoors(TArray<FShape>& Out)
	{
		using namespace Colors;
		for (const double Side : { -1.0, 1.0 })
		{
			const double A = Side * 4.0;
			const double B = Side * DoorHalf;
			Span(Out, Glass, FVector(-12, A, 20), FVector(-8, B, DoorTop - 10));
			Span(Out, Bronze, FVector(-14, A, 0), FVector(-6, B, 24), false);
			Span(Out, Bronze, FVector(-14, A, DoorTop - 12), FVector(-6, B, DoorTop), false);
			for (const double Y : { A, B })
			{
				Span(Out, Bronze, FVector(-14, Y - 5, 0), FVector(-6, Y + 5, DoorTop), false);
			}
			// The long brass push bar across each leaf.
			Span(Out, Brass, FVector(-4, Side * 30.0, 100), FVector(0, Side * (DoorHalf - 30.0), 106), false);
		}
	}

	/** The show inside: the SSK's turntable and ropes, the other cars' stands, palms and boards. */
	inline void BuildShow(TArray<FShape>& Out)
	{
		using namespace Colors;
		const FVector Ssk = Spots::Ssk;
		Add(Out, MarketModel::Cylinder, Stand, FVector(Ssk.X, Ssk.Y, Ssk.Z * 0.5), FVector(620, 620, Ssk.Z));
		Add(Out, MarketModel::Cylinder, StandTop, FVector(Ssk.X, Ssk.Y, Ssk.Z - 0.5), FVector(600, 600, 1), 0.0, false);
		Add(Out, MarketModel::Cylinder, Brass, FVector(Ssk.X, Ssk.Y, Ssk.Z - 6.0), FVector(624, 624, 4), 0.0, false);
		// Ropes round it on brass posts, open at the back where the mechanics come in.
		TArray<FVector> Posts;
		for (int32 i = 0; i < 10; ++i)
		{
			const double A = FMath::DegreesToRadians(-150.0 + i * 300.0 / 9.0);
			Posts.Add(FVector(Ssk.X + FMath::Cos(A) * 400.0, Ssk.Y + FMath::Sin(A) * 400.0, 0.0));
		}
		for (int32 i = 0; i < Posts.Num(); ++i)
		{
			RopePost(Out, Posts[i]);
			if (i + 1 < Posts.Num())
			{
				Rope(Out, Posts[i] + FVector(0, 0, 92), Posts[i + 1] + FVector(0, 0, 92));
			}
		}
		CarStand(Out, Spots::CarLeft, Spots::CarLeftYaw);
		CarStand(Out, Spots::CarRight, Spots::CarRightYaw);
		ShowBoard(Out, Ssk + FVector(460, 160, -Ssk.Z), 20.0);
		ShowBoard(Out, Spots::CarLeft + FVector(260, -200, -Spots::CarLeft.Z), 10.0);
		ShowBoard(Out, Spots::CarRight + FVector(260, 200, -Spots::CarRight.Z), -10.0);
		for (const FVector& At : { FVector(-150, -760, 0), FVector(-150, 760, 0), FVector(-1500, -760, 0), FVector(-1500, 300, 0), FVector(-1050, -560, 0) })
		{
			PottedPalm(Out, At);
		}
	}

	/** The importer's office in the back right corner: a glazed partition, a door, desk, safe and the key cabinet. */
	inline void BuildOffice(TArray<FShape>& Out, TArray<FLamp>& Lamps)
	{
		using namespace Colors;
		const double W = HalfWidth;
		const double X0 = OfficeX;
		// The partition: a wooden dado with glass above, along the front and the open side.
		Span(Out, Wood, FVector(X0 - 6, OfficeY, 0), FVector(X0, W, 110));
		Span(Out, Glass, FVector(X0 - 4, OfficeY, 110), FVector(X0 - 2, W, 300));
		Span(Out, Wood, FVector(X0 - 6, OfficeY, 296), FVector(X0, W, 310));
		Span(Out, Wood, FVector(-Depth, OfficeY, 0), FVector(X0, OfficeY + 6, 110));
		Span(Out, Glass, FVector(-Depth, OfficeY + 2, 110), FVector(X0, OfficeY + 4, 300));
		Span(Out, Wood, FVector(-Depth, OfficeY, 296), FVector(X0, OfficeY + 6, 310));
		for (double Y = OfficeY + 100.0; Y < W; Y += 100.0)
		{
			Span(Out, Wood, FVector(X0 - 6, Y - 3, 110), FVector(X0, Y + 3, 300), false);
		}
		// Its door, at the open side, near the front.
		Span(Out, Door, FVector(X0 - 110, OfficeY - 1, 0), FVector(X0 - 20, OfficeY + 7, 220), false);
		Add(Out, MarketModel::Sphere, Brass, FVector(X0 - 95, OfficeY - 4, 105), FVector(6, 6, 6), 0.0, false);
		// The desk with a lamp and ledgers, the chair, the safe.
		Span(Out, Desk, FVector(-2200, 520, 0), FVector(-2100, 760, 78));
		Span(Out, Paper, FVector(-2190, 560, 78), FVector(-2150, 600, 82), false);
		Span(Out, Wainscot, FVector(-2180, 640, 78), FVector(-2140, 680, 86), false);
		Add(Out, MarketModel::Cylinder, Brass, FVector(-2170, 720, 100), FVector(4, 4, 44), 0.0, false);
		Add(Out, MarketModel::Cone, FlagGreen, FVector(-2170, 720, 124), FVector(30, 30, 14), 0.0, false);
		Span(Out, Wood, FVector(-2270, 600, 0), FVector(-2230, 660, 46));
		Span(Out, Safe, FVector(-2400, 420, 0), FVector(-2320, 500, 130));
		Add(Out, MarketModel::Cylinder, Brass, FVector(-2318, 460, 80), FVector(14, 14, 3), 0.0, false, 90.0);
		// The key cabinet on the back wall: a glass-fronted box with keys on brass hooks, the SSK's on its own red tag.
		const FVector K = Spots::KeyCabinet;
		Span(Out, Wood, K + FVector(-30, -50, -40), K + FVector(-18, 50, 40), false);
		for (int32 Row = 0; Row < 3; ++Row)
		{
			for (int32 Col = 0; Col < 5; ++Col)
			{
				const FVector Hook = K + FVector(-17, -36 + Col * 18, 24 - Row * 24);
				Add(Out, MarketModel::Cylinder, Brass, Hook, FVector(2, 2, 6), 0.0, false, 90.0);
				Span(Out, Row == 1 && Col == 2 ? Flag : Paper, Hook + FVector(1, -3, -14), Hook + FVector(2, 3, -6), false);
			}
		}
		Span(Out, Glass, K + FVector(-18, -50, -40), K + FVector(-16, 50, 40), false);
		Lamps.Add({ FVector(-2170, 720, 130), 300.f });
	}

	/** The side door in the left wall and the watchman's corner by it: a table, a chair, his lantern, a coat on a peg. */
	inline void BuildWatchCorner(TArray<FShape>& Out, TArray<FLamp>& Lamps)
	{
		using namespace Colors;
		const double W = HalfWidth;
		const double T = WallThick;
		Span(Out, Door, FVector(SideDoorX - 55, -W - T - 2, 0), FVector(SideDoorX + 55, -W - T + 4, 230), false);
		Span(Out, Door, FVector(SideDoorX - 55, -W - 4, 0), FVector(SideDoorX + 55, -W + 2, 230), false);
		Span(Out, StoneDark, FVector(SideDoorX - 70, -W - T - 6, 230), FVector(SideDoorX + 70, -W - T, 250), false);
		Add(Out, MarketModel::Cylinder, Iron, FVector(SideDoorX, -W - T - 10, 290), FVector(4, 4, 30), 0.0, false, 90.0);
		Add(Out, MarketModel::Sphere, Globe, FVector(SideDoorX, -W - T - 24, 280), FVector(20, 20, 20), 0.0, false);
		Lamps.Add({ FVector(SideDoorX, -W - T - 24, 280), 500.f });
		// The table and chair, his lantern and a mug, a newspaper.
		Span(Out, Wood, FVector(-1560, -880, 0), FVector(-1460, -800, 74));
		Span(Out, Wood, FVector(-1610, -800, 0), FVector(-1560, -740, 44));
		Span(Out, Wood, FVector(-1614, -800, 44), FVector(-1606, -740, 100), false);
		Add(Out, MarketModel::Cylinder, Iron, FVector(-1490, -850, 90), FVector(16, 16, 30), 0.0, false);
		Add(Out, MarketModel::Sphere, Globe, FVector(-1490, -850, 92), FVector(12, 12, 16), 0.0, false);
		Add(Out, MarketModel::Cylinder, Paper, FVector(-1530, -820, 80), FVector(8, 8, 12), 0.0, false);
		Span(Out, Paper, FVector(-1555, -875, 74), FVector(-1515, -840, 75), false);
		Lamps.Add({ FVector(-1490, -850, 92), 400.f });
	}

	/** Everything of the hall but the glass doors. */
	inline void Build(TArray<FShape>& Out, TArray<FLamp>& Lamps)
	{
		BuildHall(Out, Lamps);
		BuildShow(Out);
		BuildOffice(Out, Lamps);
		BuildWatchCorner(Out, Lamps);
	}

	/** The lettering: the hall's name, the show banner, the boards, the office door. */
	inline void Signs(TArray<FSign>& Out)
	{
		using namespace Colors;
		Out.Add({ TEXT("TATTERSALL"), FVector(11, 0, 825), 0.0, 70.0, Lettering });
		Out.Add({ TEXT("AUT\u00D3SZALON"), FVector(11, 0, 705), 0.0, 44.0, Lettering });
		Out.Add({ TEXT("NEMZETK\u00D6ZI AUT\u00D3KI\u00C1LL\u00CDT\u00C1S \u00B7 1932"), FVector(33, 0, 570), 0.0, 44.0, BannerText });
		Out.Add({ TEXT("MERCEDES-BENZ SSK"), Spots::Ssk + FVector(467, 158, 98), 20.0, 8.0, Board });
		Out.Add({ TEXT("IRODA"), FVector(OfficeX - 65, OfficeY - 2, 190), -90.0, 14.0, Lettering });
	}
}
