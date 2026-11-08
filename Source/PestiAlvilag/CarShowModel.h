// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BozsiHouseModel.h"
#include "MarketModel.h"
#include "PersonKit.h"

/**
 * The car show in the Tattersall hall on Kerepesi ut (the SSK chain):
 * - the hall: a two-storey stone front with tall shop windows, glass doors
 *   in the middle (their own list, BuildDoors, so the SSK can smash out
 *   through them), the show's banner, a clock and flags on the gable,
 *   lanterns on the piers, two candelabra lamps on the pavement; a glass
 *   roof on iron trusses carried by iron columns down both sides;
 * - inside: the SSK on a round turntable behind brass posts and red ropes
 *   under its own lamps, four more cars on stands, an engine on a stand,
 *   a tyre and parts counter, bunting, posters, palms and benches, a
 *   gallery across the back with a stair, hanging lamps and wall lamps;
 * - under the gallery the importer's glazed office with the key cabinet;
 *   the side door in the left wall with the night watchman's corner;
 * - the people: Otto bacsi the night watchman, the importer's chauffeur,
 *   and a show mechanic in blue overalls.
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
		const FLinearColor StoneLight(0.72f, 0.66f, 0.54f);
		const FLinearColor Plaster(0.70f, 0.66f, 0.56f);
		const FLinearColor Wainscot(0.20f, 0.13f, 0.08f);
		const FLinearColor Floor(0.42f, 0.38f, 0.32f);
		const FLinearColor FloorDark(0.22f, 0.18f, 0.15f);
		const FLinearColor Glass(0.12f, 0.15f, 0.17f);
		const FLinearColor Iron(0.06f, 0.065f, 0.07f);
		const FLinearColor IronGreen(0.08f, 0.14f, 0.11f);
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
		const FLinearColor Poster(0.72f, 0.36f, 0.12f);
		const FLinearColor PosterBlue(0.14f, 0.24f, 0.42f);
		const FLinearColor Wood(0.30f, 0.17f, 0.08f);
		const FLinearColor Desk(0.22f, 0.12f, 0.06f);
		const FLinearColor Safe(0.12f, 0.16f, 0.12f);
		const FLinearColor Globe(0.95f, 0.88f, 0.70f);
		const FLinearColor Door(0.16f, 0.10f, 0.06f);
		const FLinearColor Flag(0.62f, 0.08f, 0.06f);
		const FLinearColor FlagWhite(0.85f, 0.83f, 0.78f);
		const FLinearColor FlagGreen(0.08f, 0.32f, 0.12f);
		const FLinearColor Tyre(0.04f, 0.04f, 0.04f);
		const FLinearColor Engine(0.20f, 0.21f, 0.22f);
		const FLinearColor ClockFace(0.92f, 0.90f, 0.82f);
	}

	// Sizes (cm).
	constexpr double HalfWidth = 1300.0;   // the hall, Y -1300..1300
	constexpr double Depth = 3600.0;       // from the front back to X = -3600
	constexpr double WallTop = 900.0;
	constexpr double RidgeTop = 1450.0;
	constexpr double DoorHalf = 280.0;     // the glass doors, Y -280..280
	constexpr double DoorTop = 440.0;
	constexpr double WallThick = 40.0;
	constexpr double TrussGap = 450.0;     // between the roof trusses (and the columns under them)
	constexpr double ColumnIn = 160.0;     // the columns stand this far in from the side walls
	constexpr double GalleryDepth = 420.0; // the gallery across the back
	constexpr double GalleryZ = 400.0;
	constexpr double OfficeY = 500.0;      // the office, under the gallery, from here to the right wall
	constexpr double SideDoorX = -2700.0;  // the side door in the left wall (Y = -HalfWidth)

	/** Where things are, in the hall's frame. */
	namespace Spots
	{
		/** The SSK on its turntable in the middle, facing the doors. */
		const FVector Ssk(-1750.0, 0.0, 30.0);
		constexpr double SskYaw = 0.0;
		/** The four other cars on their stands, half turned to the middle. */
		const FVector CarFrontLeft(-900.0, -820.0, 12.0);
		constexpr double CarFrontLeftYaw = 35.0;
		const FVector CarFrontRight(-900.0, 820.0, 12.0);
		constexpr double CarFrontRightYaw = -35.0;
		const FVector CarBackLeft(-2350.0, -820.0, 12.0);
		constexpr double CarBackLeftYaw = 145.0;
		const FVector CarBackRight(-2350.0, 820.0, 12.0);
		constexpr double CarBackRightYaw = -145.0;
		/** The key cabinet on the back wall in the office, and where the player stands to take the key. */
		const FVector KeyCabinet(-Depth + 20.0, 1000.0, 150.0);
		const FVector TakeKey(-Depth + 110.0, 1000.0, 0.0);
		/** The side door (its middle, on the wall's outer face) and the watchman's chair by it. */
		const FVector SideDoor(SideDoorX, -HalfWidth - WallThick * 0.5, 0.0);
		const FVector Watchman(SideDoorX + 120.0, -HalfWidth + 140.0, 0.0);
		constexpr double WatchmanYaw = 120.0;
		/** Just outside the glass doors, where the SSK lands on the pavement. */
		const FVector OutOfDoors(450.0, 0.0, 0.0);
	}

	/** Otto bacsi, the night watchman: old, heavy, grey moustache, a peaked cap and a long dark coat, a stick. */
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

	/** The importer's chauffeur: young, a grey uniform coat buttoned up, peaked cap, leather boots. */
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
			const FVector Reach = FRotator(0.0, Yaw, 0.0).RotateVector(FVector(55, 0, 0));
			Add(Out, MarketModel::Cube, Colors::Palm, At + FVector(0, 0, 160) + Reach, FVector(120, 26, 3), Yaw, false, -25.0);
		}
	}

	/** A brass post for the ropes, at At (its foot). */
	inline void RopePost(TArray<FShape>& Out, const FVector& At)
	{
		Add(Out, MarketModel::Cylinder, Colors::Brass, At + FVector(0, 0, 2), FVector(28, 28, 4), 0.0, false);
		Add(Out, MarketModel::Cylinder, Colors::Brass, At + FVector(0, 0, 50), FVector(6, 6, 96), 0.0, false);
		Add(Out, MarketModel::Sphere, Colors::Brass, At + FVector(0, 0, 100), FVector(10, 10, 10), 0.0, false);
	}

	/** A straight bar of Color from A to B, Thick across. */
	inline void Rod(TArray<FShape>& Out, const FLinearColor& Color, const FVector& A, const FVector& B, double Thick)
	{
		const FVector D = B - A;
		const double Yaw = FMath::RadiansToDegrees(FMath::Atan2(D.Y, D.X));
		const double Pitch = FMath::RadiansToDegrees(FMath::Atan2(D.Z, FVector2D(D.X, D.Y).Size()));
		Add(Out, MarketModel::Cube, Color, (A + B) * 0.5, FVector(D.Size(), Thick, Thick), Yaw, false, Pitch);
	}

	/** A sagging red rope between two posts' tops. */
	inline void Rope(TArray<FShape>& Out, const FVector& A, const FVector& B)
	{
		const FVector Mid = (A + B) * 0.5 - FVector(0, 0, 12);
		Rod(Out, Colors::Velvet, A, Mid, 4.0);
		Rod(Out, Colors::Velvet, Mid, B, 4.0);
	}

	/** A string of little flag pennants from A to B, sagging, in the national colours. */
	inline void Bunting(TArray<FShape>& Out, const FVector& A, const FVector& B)
	{
		const FLinearColor Cols[3] = { Colors::Flag, Colors::FlagWhite, Colors::FlagGreen };
		const int32 Count = FMath::Max(4, static_cast<int32>((B - A).Size() / 60.0));
		FVector Last = A;
		for (int32 i = 1; i <= Count; ++i)
		{
			const double F = static_cast<double>(i) / Count;
			const FVector P = A + (B - A) * F - FVector(0, 0, 120.0 * 4.0 * F * (1.0 - F));
			Rod(Out, Colors::Iron, Last, P, 1.2);
			const FVector D = P - Last;
			const double Yaw = FMath::RadiansToDegrees(FMath::Atan2(D.Y, D.X));
			Add(Out, MarketModel::Cone, Cols[i % 3], (Last + P) * 0.5 - FVector(0, 0, 14), FVector(24, 2, 26), Yaw, false, 0.0, 180.0);
			Last = P;
		}
	}

	/** A hanging lamp at At: a rod up to Top, an enamel shade, a glowing globe. */
	inline void HangingLamp(TArray<FShape>& Out, TArray<FLamp>& Lamps, const FVector& At, double Top, float Radius)
	{
		Add(Out, MarketModel::Cylinder, Colors::Iron, At + FVector(0, 0, (Top - At.Z) * 0.5), FVector(2, 2, Top - At.Z), 0.0, false);
		Add(Out, MarketModel::Cone, Colors::IronGreen, At + FVector(0, 0, 16), FVector(80, 80, 28), 0.0, false);
		Add(Out, MarketModel::Sphere, Colors::Globe, At, FVector(28, 28, 28), 0.0, false);
		Lamps.Add({ At - FVector(0, 0, 20), Radius });
	}

	/** A wall lamp on a bracket, the wall's face at At, sticking out along Away (unit, flat). */
	inline void WallLamp(TArray<FShape>& Out, TArray<FLamp>& Lamps, const FVector& At, const FVector& Away, float Radius)
	{
		const FVector Tip = At + Away * 40.0;
		Rod(Out, Colors::Iron, At, Tip + FVector(0, 0, 10), 3.0);
		Add(Out, MarketModel::Cylinder, Colors::Iron, Tip + FVector(0, 0, 26), FVector(18, 18, 4), 0.0, false);
		Add(Out, MarketModel::Sphere, Colors::Globe, Tip + FVector(0, 0, 10), FVector(22, 22, 26), 0.0, false);
		Lamps.Add({ Tip, Radius });
	}

	/** A low display stand for a car at At (its top middle), turned Yaw, with a brass edge and a show board in front. */
	inline void CarStand(TArray<FShape>& Out, const FVector& At, double Yaw)
	{
		const FRotator Turn(0.0, Yaw, 0.0);
		Add(Out, MarketModel::Cube, Colors::Stand, At - FVector(0, 0, At.Z * 0.5), FVector(580, 280, At.Z), Yaw);
		Add(Out, MarketModel::Cube, Colors::StandTop, At - FVector(0, 0, 0.5), FVector(560, 260, 1), Yaw, false);
		Add(Out, MarketModel::Cube, Colors::Brass, At - FVector(0, 0, 2.0), FVector(584, 284, 2), Yaw, false);
		const FVector BoardAt = At + Turn.RotateVector(FVector(340, 120, 0)) - FVector(0, 0, At.Z);
		for (const double Side : { -1.0, 1.0 })
		{
			Add(Out, MarketModel::Cube, Colors::Wood, BoardAt + Turn.RotateVector(FVector(0, Side * 34, 80)), FVector(4, 4, 160), Yaw, false, 8.0);
		}
		Add(Out, MarketModel::Cube, Colors::Board, BoardAt + Turn.RotateVector(FVector(4, 0, 120)), FVector(3, 80, 64), Yaw, false, 8.0);
		Add(Out, MarketModel::Cube, Colors::Paper, BoardAt + Turn.RotateVector(FVector(6, 0, 120)), FVector(1, 70, 54), Yaw, false, 8.0);
	}

	/** A framed poster on a wall whose face is at At, facing Facing (degrees). */
	inline void WallPoster(TArray<FShape>& Out, const FVector& At, double Facing, const FLinearColor& Color)
	{
		const FVector N = FRotator(0.0, Facing, 0.0).RotateVector(FVector(1, 0, 0));
		Add(Out, MarketModel::Cube, Colors::Wood, At + N * 1.5, FVector(3, 130, 180), Facing, false);
		Add(Out, MarketModel::Cube, Color, At + N * 3.2, FVector(1, 116, 166), Facing, false);
		Add(Out, MarketModel::Cube, Colors::Paper, At + N * 3.8 + FVector(0, 0, 40), FVector(1, 90, 40), Facing, false);
		Add(Out, MarketModel::Cylinder, Colors::Paper, At + N * 3.8 - FVector(0, 0, 30), FVector(70, 1, 70), Facing, false, 0.0, 90.0);
	}

	/** The hall's front: two storeys of stone with piers, windows, cornices, the gable, flags, clock and lamps. */
	inline void BuildFront(TArray<FShape>& Out, TArray<FLamp>& Lamps)
	{
		using namespace Colors;
		const double W = HalfWidth;
		const double T = WallThick;
		// The piers: the corners, between the windows, either side of the doors; a plinth and capitals on each.
		for (const double Y : { -W, -800.0, -DoorHalf, DoorHalf, 800.0, W })
		{
			const double Half = (Y == -W || Y == W) ? 80.0 : 46.0;
			Span(Out, Stone, FVector(-T, Y - Half, 0), FVector(22, Y + Half, WallTop));
			Span(Out, StoneDark, FVector(22, Y - Half - 6, 0), FVector(30, Y + Half + 6, 60), false);
			Span(Out, StoneLight, FVector(22, Y - Half - 6, 530), FVector(32, Y + Half + 6, 560), false);
			Span(Out, StoneLight, FVector(22, Y - Half - 4, WallTop - 120), FVector(30, Y + Half + 4, WallTop - 100), false);
			// Rusticated joints up the lower storey.
			for (double Z = 100.0; Z < 520.0; Z += 60.0)
			{
				Span(Out, StoneDark, FVector(22, Y - Half, Z), FVector(23, Y + Half, Z + 3), false);
			}
		}
		// The shop windows: a low wall, plate glass in three lights, bronze rails, a fanlight row.
		for (const double Side : { -1.0, 1.0 })
		{
			for (const double Y0 : { DoorHalf + 46.0, 846.0 })
			{
				const double Y1 = Y0 > 800.0 ? W - 80.0 : 754.0;
				const double A = Side * Y0;
				const double B = Side * Y1;
				Span(Out, StoneDark, FVector(-T, A, 0), FVector(12, B, 80));
				Span(Out, Glass, FVector(-12, A, 80), FVector(-8, B, 420));
				Span(Out, Bronze, FVector(-8, A, 76), FVector(-2, B, 84), false);
				Span(Out, Bronze, FVector(-8, A, 416), FVector(-2, B, 424), false);
				for (const double F : { 1.0 / 3.0, 2.0 / 3.0 })
				{
					const double Y = FMath::Lerp(A, B, F);
					Span(Out, Bronze, FVector(-8, Y - 3, 80), FVector(-2, Y + 3, 520), false);
				}
				Span(Out, Glass, FVector(-12, A, 424), FVector(-8, B, 520));
				Span(Out, Stone, FVector(-T, A, 520), FVector(12, B, 560));
				// What shows through: a poster on an easel in each window.
				Add(Out, MarketModel::Cube, Side > 0 ? Poster : PosterBlue, FVector(-60, (A + B) * 0.5, 230), FVector(2, 120, 160), 0.0, false, 6.0);
			}
		}
		// Over the doors: the fanlight in bronze bars, and a stone hood on brackets.
		Span(Out, Bronze, FVector(-10, -DoorHalf, DoorTop), FVector(-2, DoorHalf, DoorTop + 10), false);
		Span(Out, Glass, FVector(-12, -DoorHalf, DoorTop + 10), FVector(-8, DoorHalf, 520));
		for (double Y = -DoorHalf + 56.0; Y < DoorHalf; Y += 56.0)
		{
			Span(Out, Bronze, FVector(-8, Y - 2, DoorTop + 10), FVector(-4, Y + 2, 520), false);
		}
		Span(Out, Stone, FVector(-T, -DoorHalf, 520), FVector(12, DoorHalf, 560));
		Span(Out, StoneLight, FVector(12, -DoorHalf - 60, 548), FVector(90, DoorHalf + 60, 566));
		for (const double Y : { -DoorHalf - 30.0, DoorHalf + 30.0 })
		{
			Span(Out, StoneLight, FVector(22, Y - 12, 490), FVector(80, Y + 12, 548), false);
		}
		// The first cornice, the upper storey with its row of small arched windows, the main cornice with dentils.
		Span(Out, StoneLight, FVector(-T, -W - 80, 560), FVector(40, W + 80, 590));
		Span(Out, Stone, FVector(-T, -W + 80, 590), FVector(14, W - 80, WallTop));
		for (double Y = -W + 170.0; Y < W - 120.0; Y += 140.0)
		{
			if (FMath::Abs(Y) < 360.0)
			{
				continue;
			}
			Span(Out, Glass, FVector(14, Y - 40, 640), FVector(16, Y + 40, 780), false);
			Add(Out, MarketModel::Cylinder, Glass, FVector(15, Y, 780), FVector(80, 2, 80), 0.0, false, 0.0, 90.0);
			Span(Out, StoneLight, FVector(14, Y - 50, 630), FVector(20, Y + 50, 640), false);
		}
		Span(Out, StoneLight, FVector(-T, -W - 90, WallTop), FVector(46, W + 90, WallTop + 30));
		for (double Y = -W - 80.0; Y < W + 80.0; Y += 40.0)
		{
			Span(Out, StoneLight, FVector(40, Y, WallTop - 14), FVector(52, Y + 18, WallTop), false);
		}
		// A balustrade along the top, the gable over the middle with the name and a clock.
		for (const double Side : { -1.0, 1.0 })
		{
			Span(Out, Stone, FVector(-T, Side * 420.0, WallTop + 30), FVector(20, Side * (W + 80), WallTop + 50));
			Span(Out, Stone, FVector(-T, Side * 420.0, WallTop + 130), FVector(26, Side * (W + 80), WallTop + 150));
		}
		for (double Y = -W - 60.0; Y < W + 70.0; Y += 36.0)
		{
			if (FMath::Abs(Y) > 420.0)
			{
				Add(Out, MarketModel::Cylinder, StoneLight, FVector(4, Y, WallTop + 90), FVector(16, 16, 80), 0.0, false);
			}
		}
		Span(Out, Stone, FVector(-T, -420, WallTop), FVector(14, 420, 1220));
		Span(Out, StoneLight, FVector(14, -440, 1220), FVector(40, 440, 1250));
		Span(Out, Stone, FVector(-T, -200, 1250), FVector(14, 200, 1380));
		Span(Out, StoneLight, FVector(14, -220, 1380), FVector(36, 220, 1400));
		const FVector Clock(16, 0, 1310);
		Add(Out, MarketModel::Cylinder, StoneLight, Clock, FVector(110, 6, 110), 0.0, false, 0.0, 90.0);
		Add(Out, MarketModel::Cylinder, ClockFace, Clock + FVector(3, 0, 0), FVector(90, 2, 90), 0.0, false, 0.0, 90.0);
		Span(Out, Iron, Clock + FVector(4.5, -2, 0), Clock + FVector(5.5, 2, 34), false);
		Span(Out, Iron, Clock + FVector(4.5, 0, -2), Clock + FVector(5.5, 26, 2), false);
		// Flags on poles either side of the gable.
		for (const double Y : { -560.0, 560.0 })
		{
			Add(Out, MarketModel::Cylinder, Iron, FVector(10, Y, WallTop + 300), FVector(6, 6, 450), 0.0, false);
			Add(Out, MarketModel::Sphere, Brass, FVector(10, Y, WallTop + 528), FVector(12, 12, 12), 0.0, false);
			Span(Out, Flag, FVector(9, Y + 4, WallTop + 450), FVector(11, Y + 150, WallTop + 480), false);
			Span(Out, FlagWhite, FVector(9, Y + 4, WallTop + 420), FVector(11, Y + 150, WallTop + 450), false);
			Span(Out, FlagGreen, FVector(9, Y + 4, WallTop + 390), FVector(11, Y + 150, WallTop + 420), false);
		}
		// The show's banner across the upper storey, with three lamps on arms lighting it at night.
		Span(Out, Banner, FVector(34, -760, 600), FVector(36, 760, 720), false);
		for (const double Y : { -740.0, 740.0 })
		{
			Span(Out, Iron, FVector(14, Y - 2, 716), FVector(38, Y + 2, 720), false);
		}
		for (const double Y : { -500.0, 0.0, 500.0 })
		{
			Rod(Out, Iron, FVector(14, Y, 760), FVector(120, Y, 790), 3.0);
			Add(Out, MarketModel::Cone, IronGreen, FVector(120, Y, 776), FVector(36, 36, 22), 0.0, false, -30.0);
			Add(Out, MarketModel::Sphere, Globe, FVector(118, Y, 768), FVector(12, 12, 12), 0.0, false);
			Lamps.Add({ FVector(100, Y, 740), 500.f });
		}
		// Lanterns on the piers by the doors and windows.
		for (const double Y : { -800.0, -DoorHalf, DoorHalf, 800.0 })
		{
			WallLamp(Out, Lamps, FVector(22, Y, 420), FVector(1, 0, 0), 450.f);
		}
		// Two candelabra lamps on the pavement, three globes each.
		for (const double Y : { -700.0, 700.0 })
		{
			const FVector Foot(330, Y, 0);
			Add(Out, MarketModel::Cylinder, IronGreen, Foot + FVector(0, 0, 30), FVector(44, 44, 60));
			Add(Out, MarketModel::Cylinder, IronGreen, Foot + FVector(0, 0, 230), FVector(14, 14, 400));
			for (const double S : { -1.0, 0.0, 1.0 })
			{
				const FVector Bulb = Foot + FVector(0, S * 50.0, S == 0.0 ? 470.0 : 420.0);
				if (S != 0.0)
				{
					Rod(Out, IronGreen, Foot + FVector(0, 0, 400), Bulb - FVector(0, 0, 20), 4.0);
				}
				Add(Out, MarketModel::Sphere, Globe, Bulb, FVector(30, 30, 34), 0.0, false);
			}
			Lamps.Add({ Foot + FVector(0, 0, 440), 900.f });
		}
	}

	/** The hall's shell: floor, walls, the iron columns, the glass roof on its trusses, lamps, bunting, posters. */
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
		// The side and back walls, with a dark wainscot and rails inside.
		Span(Out, Plaster, FVector(-Depth, -W - T, 0), FVector(0, -W, WallTop));
		Span(Out, Plaster, FVector(-Depth, W, 0), FVector(0, W + T, WallTop));
		Span(Out, Plaster, FVector(-Depth - T, -W - T, 0), FVector(-Depth, W + T, WallTop));
		for (const double Side : { -1.0, 1.0 })
		{
			const double Face = Side * W;
			Span(Out, Wainscot, FVector(-Depth, Face - Side * 2.0, 0), FVector(-40, Face, 130), false);
			Span(Out, Wood, FVector(-Depth, Face - Side * 4.0, 128), FVector(-40, Face, 136), false);
			Span(Out, Wood, FVector(-Depth, Face - Side * 4.0, 560), FVector(-40, Face, 566), false);
		}
		Span(Out, Wainscot, FVector(-Depth, -W, 0), FVector(-Depth + 2, W, 130), false);
		// Columns down both sides under each truss, with brackets up to it; wall lamps and posters between them.
		for (double X = -TrussGap; X > -Depth + 100.0; X -= TrussGap)
		{
			for (const double Side : { -1.0, 1.0 })
			{
				const double Y = Side * (W - ColumnIn);
				Add(Out, MarketModel::Cylinder, IronGreen, FVector(X, Y, 20), FVector(46, 46, 40));
				Add(Out, MarketModel::Cylinder, IronGreen, FVector(X, Y, WallTop * 0.5), FVector(24, 24, WallTop));
				Add(Out, MarketModel::Cylinder, IronGreen, FVector(X, Y, WallTop - 20), FVector(40, 40, 30), 0.0, false);
				Rod(Out, IronGreen, FVector(X, Y, WallTop - 140), FVector(X, Y - Side * 200.0, WallTop + 30), 8.0);
				Rod(Out, IronGreen, FVector(X, Y, WallTop - 140), FVector(X, Side * W, WallTop - 20), 8.0);
				if (X > -Depth + GalleryDepth + 100.0)
				{
					WallLamp(Out, Lamps, FVector(X + TrussGap * 0.5, Side * W, 330), FVector(0, -Side, 0), 450.f);
					WallPoster(Out, FVector(X, Side * W, 300), -Side * 90.0, static_cast<int32>(-X / TrussGap) % 2 ? Poster : PosterBlue);
				}
			}
		}
		// Long iron beams along the column heads.
		for (const double Side : { -1.0, 1.0 })
		{
			Span(Out, IronGreen, FVector(-Depth, Side * (W - ColumnIn) - 10, WallTop - 6), FVector(0, Side * (W - ColumnIn) + 10, WallTop + 16), false);
		}
		// The roof: iron trusses across, glass slopes with glazing bars, a lantern ridge.
		const double Run = W + T;
		const double Rise = RidgeTop - WallTop;
		const double Len = FMath::Sqrt(Run * Run + Rise * Rise);
		const double Slope = FMath::RadiansToDegrees(FMath::Atan2(Rise, Run));
		for (double X = -TrussGap; X > -Depth + 100.0; X -= TrussGap)
		{
			for (const double Side : { -1.0, 1.0 })
			{
				Add(Out, MarketModel::Cube, Iron, FVector(X, Side * Run * 0.5, WallTop + Rise * 0.5), FVector(12, Len, 20), 0.0, false, 0.0, Side * Slope);
				Add(Out, MarketModel::Cube, Iron, FVector(X, Side * Run * 0.5, WallTop + 30.0), FVector(8, Run, 8), 0.0, false);
				for (double F = 0.15; F < 0.95; F += 0.15)
				{
					const double Y = Side * Run * F;
					const double Z = WallTop + Rise * (1.0 - F);
					Add(Out, MarketModel::Cube, Iron, FVector(X, Y, (Z + WallTop + 30.0) * 0.5), FVector(5, 5, Z - WallTop - 30.0), 0.0, false);
					Rod(Out, Iron, FVector(X, Y, WallTop + 30.0), FVector(X, Side * Run * (F + 0.15), WallTop + Rise * (1.0 - F - 0.15)), 4.0);
				}
			}
		}
		for (const double Side : { -1.0, 1.0 })
		{
			Add(Out, MarketModel::Cube, Glass, FVector(-Depth * 0.5, Side * Run * 0.5, WallTop + Rise * 0.5 + 12.0), FVector(Depth, Len, 3), 0.0, false, 0.0, Side * Slope);
			for (double X = -Depth + 150.0; X < 0.0; X += 150.0)
			{
				Add(Out, MarketModel::Cube, Iron, FVector(X, Side * Run * 0.5, WallTop + Rise * 0.5 + 14.0), FVector(4, Len, 4), 0.0, false, 0.0, Side * Slope);
			}
		}
		Span(Out, Iron, FVector(-Depth, -30, RidgeTop - 4), FVector(0, 30, RidgeTop + 60), false);
		Span(Out, Glass, FVector(-Depth, -26, RidgeTop + 4), FVector(0, 26, RidgeTop + 56), false);
		// Bunting from column to column across the hall.
		for (double X = -TrussGap; X > -Depth + GalleryDepth + 100.0; X -= TrussGap * 2.0)
		{
			Bunting(Out, FVector(X, -(W - ColumnIn), WallTop - 60), FVector(X, W - ColumnIn, WallTop - 60));
		}
		// Rows of hanging lamps, and two bright ones over the turntable.
		for (double X = -500.0; X > -Depth + GalleryDepth; X -= 600.0)
		{
			for (const double Y : { -700.0, 0.0, 700.0 })
			{
				if (Y == 0.0 && FMath::Abs(X - Spots::Ssk.X) < 400.0)
				{
					continue;
				}
				HangingLamp(Out, Lamps, FVector(X, Y, 600), WallTop + 30.0, 900.f);
			}
		}
		for (const double Y : { -220.0, 220.0 })
		{
			HangingLamp(Out, Lamps, FVector(Spots::Ssk.X, Y, 480), WallTop + 30.0, 700.f);
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

	/** The show inside: the SSK's turntable and ropes, the other cars' stands, the engine, the parts counter, palms and benches. */
	inline void BuildShow(TArray<FShape>& Out)
	{
		using namespace Colors;
		const FVector Ssk = Spots::Ssk;
		Add(Out, MarketModel::Cylinder, Stand, FVector(Ssk.X, Ssk.Y, Ssk.Z * 0.5), FVector(640, 640, Ssk.Z));
		Add(Out, MarketModel::Cylinder, StandTop, FVector(Ssk.X, Ssk.Y, Ssk.Z - 0.5), FVector(620, 620, 1), 0.0, false);
		Add(Out, MarketModel::Cylinder, Brass, FVector(Ssk.X, Ssk.Y, Ssk.Z - 6.0), FVector(644, 644, 4), 0.0, false);
		// Ropes round it on brass posts, open at the back where the mechanics come in.
		TArray<FVector> Posts;
		for (int32 i = 0; i < 12; ++i)
		{
			const double A = FMath::DegreesToRadians(-150.0 + i * 300.0 / 11.0);
			Posts.Add(FVector(Ssk.X + FMath::Cos(A) * 430.0, Ssk.Y + FMath::Sin(A) * 430.0, 0.0));
		}
		for (int32 i = 0; i < Posts.Num(); ++i)
		{
			RopePost(Out, Posts[i]);
			if (i + 1 < Posts.Num())
			{
				Rope(Out, Posts[i] + FVector(0, 0, 92), Posts[i + 1] + FVector(0, 0, 92));
			}
		}
		// The SSK's own board out in front of the ropes, and the four other stands.
		const FVector Easel = Ssk + FVector(500, 180, -Ssk.Z);
		for (const double Side : { -1.0, 1.0 })
		{
			Add(Out, MarketModel::Cube, Wood, Easel + FVector(0, Side * 34, 80), FVector(4, 4, 160), 0.0, false, 8.0);
		}
		Add(Out, MarketModel::Cube, Board, Easel + FVector(4, 0, 120), FVector(3, 80, 64), 0.0, false, 8.0);
		Add(Out, MarketModel::Cube, Paper, Easel + FVector(6, 0, 120), FVector(1, 70, 54), 0.0, false, 8.0);
		CarStand(Out, Spots::CarFrontLeft, Spots::CarFrontLeftYaw);
		CarStand(Out, Spots::CarFrontRight, Spots::CarFrontRightYaw);
		CarStand(Out, Spots::CarBackLeft, Spots::CarBackLeftYaw);
		CarStand(Out, Spots::CarBackRight, Spots::CarBackRightYaw);
		// A bare engine on a stand, and a counter of tyres and parts across from it.
		const FVector Motor(-450.0, 320.0, 0.0);
		Span(Out, Wood, Motor + FVector(-60, -40, 0), Motor + FVector(60, 40, 70));
		Span(Out, Engine, Motor + FVector(-50, -22, 70), Motor + FVector(50, 22, 130));
		Span(Out, Engine, Motor + FVector(-46, -16, 130), Motor + FVector(46, 16, 150), false);
		for (int32 i = 0; i < 6; ++i)
		{
			Add(Out, MarketModel::Cylinder, Brass, Motor + FVector(-38 + i * 15, 0, 156), FVector(6, 6, 12), 0.0, false);
		}
		Add(Out, MarketModel::Cylinder, Engine, Motor + FVector(62, 0, 100), FVector(40, 40, 6), 0.0, false, 90.0);
		const FVector Counter(-450.0, -320.0, 0.0);
		Span(Out, Wood, Counter + FVector(-60, -160, 0), Counter + FVector(60, 160, 95));
		Span(Out, Glass, Counter + FVector(60, -150, 20), Counter + FVector(62, 150, 85), false);
		for (int32 i = 0; i < 5; ++i)
		{
			for (int32 Up = 0; Up < 3; ++Up)
			{
				Add(Out, MarketModel::Cylinder, Tyre, Counter + FVector(-110, -120 + i * 60, 9 + Up * 17), FVector(70, 70, 16), 0.0, false);
			}
		}
		for (int32 i = 0; i < 4; ++i)
		{
			Add(Out, MarketModel::Cylinder, Brass, Counter + FVector(0, -110 + i * 70, 105), FVector(20, 20, 20), 0.0, false, 0.0, 90.0);
		}
		// Palms along the walls and by the turntable, benches for the visitors.
		for (const FVector& At : { FVector(-200, -1050, 0), FVector(-200, 1050, 0), FVector(-1750, -650, 0), FVector(-1750, 650, 0),
			FVector(-1350, -1050, 0), FVector(-1350, 1050, 0), FVector(-2200, -1050, 0), FVector(-2200, 1050, 0) })
		{
			PottedPalm(Out, At);
		}
		for (const double X : { -1250.0, -2250.0 })
		{
			for (const double Y : { -360.0, 360.0 })
			{
				Span(Out, Wood, FVector(X - 90, Y - 22, 40), FVector(X + 90, Y + 22, 46));
				for (const double Leg : { -80.0, 80.0 })
				{
					Span(Out, Iron, FVector(X + Leg - 3, Y - 20, 0), FVector(X + Leg + 3, Y + 20, 40), false);
				}
			}
		}
	}

	/** The gallery across the back on iron posts, with its railing and a stair up the left side. */
	inline void BuildGallery(TArray<FShape>& Out, TArray<FLamp>& Lamps)
	{
		using namespace Colors;
		const double W = HalfWidth;
		const double Edge = -Depth + GalleryDepth;
		Span(Out, Wood, FVector(-Depth, -W, GalleryZ - 20), FVector(Edge, W, GalleryZ));
		Span(Out, IronGreen, FVector(Edge - 10, -W, GalleryZ - 40), FVector(Edge, W, GalleryZ - 20), false);
		for (double Y = -W + 300.0; Y < W; Y += 400.0)
		{
			Add(Out, MarketModel::Cylinder, IronGreen, FVector(Edge - 20, Y, (GalleryZ - 40) * 0.5), FVector(18, 18, GalleryZ - 40));
		}
		// The railing: a top rail, bars, a brass handrail.
		Span(Out, IronGreen, FVector(Edge - 6, -W + 140, GalleryZ + 96), FVector(Edge, W, GalleryZ + 102), false);
		Span(Out, Brass, FVector(Edge - 8, -W + 140, GalleryZ + 102), FVector(Edge + 2, W, GalleryZ + 106), false);
		for (double Y = -W + 140.0; Y < W; Y += 18.0)
		{
			Span(Out, IronGreen, FVector(Edge - 4, Y - 1, GalleryZ), FVector(Edge - 2, Y + 1, GalleryZ + 96), false);
		}
		// The stair: steps rising along the left wall from the hall floor up to the gallery.
		const int32 Steps = 20;
		const double StairTop = Edge;
		const double StairFoot = Edge + 560.0;
		for (int32 i = 0; i < Steps; ++i)
		{
			const double X0 = FMath::Lerp(StairFoot, StairTop, static_cast<double>(i) / Steps);
			const double X1 = FMath::Lerp(StairFoot, StairTop, static_cast<double>(i + 1) / Steps);
			Span(Out, Wood, FVector(X1, -W, 0), FVector(X0, -W + 140, GalleryZ * (i + 1) / Steps));
		}
		Rod(Out, Brass, FVector(StairFoot, -W + 140, 100), FVector(StairTop, -W + 140, GalleryZ + 100), 4.0);
		for (int32 i = 0; i <= 4; ++i)
		{
			const double F = i / 4.0;
			const FVector Foot(FMath::Lerp(StairFoot, StairTop, F), -W + 140, GalleryZ * F);
			Span(Out, IronGreen, Foot - FVector(2, 2, 0), Foot + FVector(2, 2, 100), false);
		}
		// Up there: little tables for the dealers, two wall lamps.
		for (const double Y : { -500.0, 0.0, 500.0 })
		{
			Add(Out, MarketModel::Cylinder, Wood, FVector(-Depth + 200, Y, GalleryZ + 74), FVector(70, 70, 4), 0.0, false);
			Add(Out, MarketModel::Cylinder, Iron, FVector(-Depth + 200, Y, GalleryZ + 37), FVector(6, 6, 74), 0.0, false);
		}
		for (const double Y : { -600.0, 600.0 })
		{
			WallLamp(Out, Lamps, FVector(-Depth, Y, GalleryZ + 200), FVector(1, 0, 0), 500.f);
		}
	}

	/** The importer's office under the right of the gallery: glazed partitions, a door, desk, safe and the key cabinet. */
	inline void BuildOffice(TArray<FShape>& Out, TArray<FLamp>& Lamps)
	{
		using namespace Colors;
		const double W = HalfWidth;
		const double X0 = -Depth + GalleryDepth - 30.0;
		const double Roof = GalleryZ - 40.0;
		Span(Out, Wood, FVector(X0 - 6, OfficeY, 0), FVector(X0, W, 110));
		Span(Out, Glass, FVector(X0 - 4, OfficeY, 110), FVector(X0 - 2, W, Roof - 20));
		Span(Out, Wood, FVector(X0 - 6, OfficeY, Roof - 20), FVector(X0, W, Roof));
		Span(Out, Wood, FVector(-Depth, OfficeY, 0), FVector(X0, OfficeY + 6, 110));
		Span(Out, Glass, FVector(-Depth, OfficeY + 2, 110), FVector(X0, OfficeY + 4, Roof - 20));
		Span(Out, Wood, FVector(-Depth, OfficeY, Roof - 20), FVector(X0, OfficeY + 6, Roof));
		for (double Y = OfficeY + 100.0; Y < W; Y += 100.0)
		{
			Span(Out, Wood, FVector(X0 - 6, Y - 3, 110), FVector(X0, Y + 3, Roof - 20), false);
		}
		// Its door, in the glass front near the open side.
		Span(Out, Door, FVector(X0 - 7, OfficeY + 20, 0), FVector(X0 + 1, OfficeY + 110, 230), false);
		Add(Out, MarketModel::Sphere, Brass, FVector(X0 + 3, OfficeY + 95, 105), FVector(6, 6, 6), 0.0, false);
		// The desk with a green lamp and ledgers, the chair, the safe, a filing cupboard.
		const double DX = -Depth + 200.0;
		Span(Out, Desk, FVector(DX - 50, 760, 0), FVector(DX + 50, 1060, 78));
		Span(Out, Paper, FVector(DX - 40, 800, 78), FVector(DX, 840, 82), false);
		Span(Out, Wainscot, FVector(DX - 30, 880, 78), FVector(DX + 10, 920, 86), false);
		Add(Out, MarketModel::Cylinder, Brass, FVector(DX - 20, 1000, 100), FVector(4, 4, 44), 0.0, false);
		Add(Out, MarketModel::Cone, FlagGreen, FVector(DX - 20, 1000, 124), FVector(30, 30, 14), 0.0, false);
		Span(Out, Wood, FVector(DX + 70, 860, 0), FVector(DX + 110, 920, 46));
		Span(Out, Safe, FVector(-Depth, 560, 0), FVector(-Depth + 80, 640, 130));
		Add(Out, MarketModel::Cylinder, Brass, FVector(-Depth + 82, 600, 80), FVector(14, 14, 3), 0.0, false, 90.0);
		Span(Out, Wood, FVector(X0 - 60, W - 80, 0), FVector(X0 - 6, W, 200));
		// The key cabinet on the back wall: a glass-fronted box, keys on brass hooks, the SSK's on its red tag.
		const FVector K = Spots::KeyCabinet;
		Span(Out, Wood, K + FVector(-20, -50, -40), K + FVector(-8, 50, 40), false);
		for (int32 Row = 0; Row < 3; ++Row)
		{
			for (int32 Col = 0; Col < 5; ++Col)
			{
				const FVector Hook = K + FVector(-7, -36 + Col * 18, 24 - Row * 24);
				Add(Out, MarketModel::Cylinder, Brass, Hook, FVector(2, 2, 6), 0.0, false, 90.0);
				Span(Out, Row == 1 && Col == 2 ? Flag : Paper, Hook + FVector(1, -3, -14), Hook + FVector(2, 3, -6), false);
			}
		}
		Span(Out, Glass, K + FVector(-8, -50, -40), K + FVector(-6, 50, 40), false);
		Lamps.Add({ FVector(DX - 20, 1000, 130), 350.f });
		HangingLamp(Out, Lamps, FVector(-Depth + 200, 900, 300), Roof, 500.f);
	}

	/** The side door in the left wall and the watchman's corner by it: a table, a chair, his lantern, a mug, the paper. */
	inline void BuildWatchCorner(TArray<FShape>& Out, TArray<FLamp>& Lamps)
	{
		using namespace Colors;
		const double W = HalfWidth;
		const double T = WallThick;
		Span(Out, Door, FVector(SideDoorX - 55, -W - T - 2, 0), FVector(SideDoorX + 55, -W - T + 4, 230), false);
		Span(Out, Door, FVector(SideDoorX - 55, -W - 4, 0), FVector(SideDoorX + 55, -W + 2, 230), false);
		Span(Out, StoneDark, FVector(SideDoorX - 70, -W - T - 6, 230), FVector(SideDoorX + 70, -W - T, 250), false);
		WallLamp(Out, Lamps, FVector(SideDoorX, -W - T, 290), FVector(0, -1, 0), 500.f);
		const FVector C = Spots::Watchman;
		Span(Out, Wood, C + FVector(40, -120, 0), C + FVector(140, -40, 74));
		Span(Out, Wood, C + FVector(-10, -40, 0), C + FVector(40, 20, 44));
		Span(Out, Wood, C + FVector(-14, -40, 44), C + FVector(-6, 20, 100), false);
		Add(Out, MarketModel::Cylinder, Iron, C + FVector(110, -90, 90), FVector(16, 16, 30), 0.0, false);
		Add(Out, MarketModel::Sphere, Globe, C + FVector(110, -90, 92), FVector(12, 12, 16), 0.0, false);
		Add(Out, MarketModel::Cylinder, Paper, C + FVector(70, -60, 80), FVector(8, 8, 12), 0.0, false);
		Span(Out, Paper, C + FVector(45, -115, 74), C + FVector(85, -80, 75), false);
		Lamps.Add({ C + FVector(110, -90, 92), 400.f });
	}

	/** Everything of the hall but the glass doors. */
	inline void Build(TArray<FShape>& Out, TArray<FLamp>& Lamps)
	{
		BuildFront(Out, Lamps);
		BuildHall(Out, Lamps);
		BuildShow(Out);
		BuildGallery(Out, Lamps);
		BuildOffice(Out, Lamps);
		BuildWatchCorner(Out, Lamps);
	}

	/** The lettering: the hall's name, the show banner, the SSK's board, the office door. */
	inline void Signs(TArray<FSign>& Out)
	{
		using namespace Colors;
		Out.Add({ TEXT("TATTERSALL"), FVector(15, 0, 1140), 0.0, 80.0, Lettering });
		Out.Add({ TEXT("AUT\u00D3SZALON"), FVector(15, 0, 1040), 0.0, 50.0, Lettering });
		Out.Add({ TEXT("NEMZETK\u00D6ZI AUT\u00D3KI\u00C1LL\u00CDT\u00C1S \u00B7 1932"), FVector(37, 0, 660), 0.0, 52.0, BannerText });
		Out.Add({ TEXT("MERCEDES-BENZ SSK"), Spots::Ssk + FVector(507, 180, 98), 0.0, 8.0, Board });
		Out.Add({ TEXT("IRODA"), FVector(-Depth + GalleryDepth - 28, OfficeY + 160, 250), 0.0, 16.0, Lettering });
	}
}
