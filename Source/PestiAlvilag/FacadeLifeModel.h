// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * Life on the house fronts, drawn into the facades by AOsmGraybox::AddFacade:
 * flower boxes of geraniums on the window sills, and washing hung out of a
 * window on a line between two iron arms. Also the houses' finer details
 * (HouseDetails): balconies on consoles with iron or stone railings,
 * keystones over the windows, pilasters, consoles under the cornice, a
 * stepped attic gable over the middle; and wear: posters pasted on the
 * ground floor piers, some torn and peeling, and plaster fallen off to the
 * brick with a crack running from it.
 *
 * Which window gets what is decided by where the window is (WindowHash), not
 * by the house, so Tools/OsmGraybox/city_details.py can find the same windows
 * with the same sums and put people at windows that have nothing else
 * (FolkFrom..FolkTo, see CityLifeModel::WindowFolk).
 *
 * Header-only and free of engine objects, so the offline preview builds the
 * same boxes. A window's frame: S along the wall (from the wall's start), Z
 * up from the ground, D out of the wall face.
 */
namespace FacadeLifeModel
{
	/** The facade sections the boxes go into (after the facade's own ones). */
	enum EPart : int32
	{
		Planter,
		Leaves,
		Blooms,
		PaleBlooms,
		Linen,
		Cloth,
		Iron,
		Brick,
		Crack,
		PosterOld,
		PosterRed,
		PosterBlue,
		PosterYellow,
		NumParts
	};

	/** Boxes that go into the facade's own stone trim and dark glass, not a part of their own. */
	constexpr int32 TrimPart = -1;
	constexpr int32 GlassPart = -2;

	inline FLinearColor PartColor(int32 Part)
	{
		switch (Part)
		{
		case Planter: return FLinearColor(0.10f, 0.17f, 0.10f);
		case Leaves: return FLinearColor(0.07f, 0.16f, 0.04f);
		case Blooms: return FLinearColor(0.62f, 0.04f, 0.03f);
		case PaleBlooms: return FLinearColor(0.80f, 0.42f, 0.48f);
		case Linen: return FLinearColor(0.80f, 0.79f, 0.74f);
		case Cloth: return FLinearColor(0.24f, 0.32f, 0.45f);
		case Brick: return FLinearColor(0.34f, 0.14f, 0.08f);
		case Crack: return FLinearColor(0.07f, 0.065f, 0.06f);
		case PosterOld: return FLinearColor(0.66f, 0.62f, 0.50f);
		case PosterRed: return FLinearColor(0.55f, 0.11f, 0.07f);
		case PosterBlue: return FLinearColor(0.14f, 0.24f, 0.36f);
		case PosterYellow: return FLinearColor(0.78f, 0.60f, 0.20f);
		default: return FLinearColor(0.035f, 0.035f, 0.035f);
		}
	}

	/** A box: S0..S1 along the wall, Z0..Z1 up, D0..D1 out of the wall face. */
	struct FBox
	{
		int32 Part;
		double S0, S1, Z0, Z1, D0, D1;
	};

	// Shares of the upper floor windows (out of 1000, from WindowHash).
	constexpr uint32 FlowerShare = 90;
	constexpr uint32 WashingShare = 22;
	/** Windows with a hash in FolkFrom..FolkTo have nothing on them; the people at windows are picked from these. */
	constexpr uint32 FolkFrom = 200;
	constexpr uint32 FolkTo = 260;

	/** Where the window is decides what it has: X, Y of the window's middle on the wall, and its floor. */
	inline uint32 WindowHash(double X, double Y, int32 Floor)
	{
		const uint32 CX = static_cast<uint32>(FMath::FloorToInt(X / 50.0));
		const uint32 CY = static_cast<uint32>(FMath::FloorToInt(Y / 50.0));
		uint32 H = (CX * 73856093u) ^ (CY * 19349663u) ^ (static_cast<uint32>(Floor) * 83492791u);
		H ^= H >> 13;
		H *= 0x5bd1e995u;
		H ^= H >> 15;
		return H;
	}

	/**
	 * What one upper floor window has on it: S0..S1 the window's sides, Z0 its
	 * sill (the bottom of the glass), Z1 its top.
	 */
	inline void Window(double S0, double S1, double Z0, double Z1, uint32 Hash, TArray<FBox>& Out)
	{
		const uint32 Pick = Hash % 1000u;
		const uint32 More = Hash / 1000u;
		if (Pick < FlowerShare)
		{
			// A green box on the sill, leaves over it and a row of geraniums.
			const int32 Flower = More % 3 == 0 ? PaleBlooms : Blooms;
			Out.Add({ Planter, S0 - 6.0, S1 + 6.0, Z0 - 4.0, Z0 + 16.0, 2.0, 30.0 });
			Out.Add({ Leaves, S0 - 2.0, S1 + 2.0, Z0 + 16.0, Z0 + 30.0, 4.0, 28.0 });
			const int32 Tufts = 4;
			for (int32 i = 0; i < Tufts; ++i)
			{
				const double S = S0 + 6.0 + (S1 - S0 - 12.0) * (i + 0.5) / Tufts;
				const double Rise = 6.0 + static_cast<double>((More >> (i * 2)) % 3u) * 4.0;
				Out.Add({ Leaves, S - 11.0, S + 11.0, Z0 + 28.0, Z0 + 30.0 + Rise, 6.0, 27.0 });
				Out.Add({ Flower, S - 7.0, S + 7.0, Z0 + 29.0 + Rise, Z0 + 39.0 + Rise, 11.0, 25.0 });
			}
			return;
		}
		if (Pick < FlowerShare + WashingShare)
		{
			// Two iron arms out of the wall by the window's top, a line between
			// them and a few things hung on it.
			const double Arm = 95.0;
			const double LineZ = Z1 + 6.0;
			const double A0 = S0 - 18.0;
			const double A1 = S1 + 18.0;
			Out.Add({ Iron, A0 - 1.5, A0 + 1.5, LineZ - 1.5, LineZ + 1.5, 0.0, Arm });
			Out.Add({ Iron, A1 - 1.5, A1 + 1.5, LineZ - 1.5, LineZ + 1.5, 0.0, Arm });
			Out.Add({ Iron, A0, A1, LineZ - 0.8, LineZ + 0.8, Arm - 2.0, Arm });
			double S = A0 + 6.0;
			for (int32 i = 0; i < 4 && S < A1 - 25.0; ++i)
			{
				const uint32 Bits = More >> (i * 3);
				const double Width = FMath::Min(30.0 + static_cast<double>(Bits % 4u) * 10.0, A1 - 4.0 - S);
				const double Drop = 40.0 + static_cast<double>((Bits >> 2) % 3u) * 16.0;
				const int32 Kind = (Bits >> 1) % 3u == 0 ? Cloth : Linen;
				Out.Add({ Kind, S, S + Width, LineZ - Drop, LineZ, Arm - 4.0, Arm - 1.0 });
				S += Width + 5.0;
			}
		}
	}

	/** One street wall's layout, as AddFacade lays it out (cm). */
	struct FFront
	{
		double Length = 0.0;
		double Height = 0.0;
		double GroundTop = 0.0;
		double UpperHeight = 0.0;
		double CorniceZone = 60.0;
		double EdgeMargin = 60.0;
		double Bay = 320.0;
		double WindowWidth = 130.0;
		int32 Storeys = 1;
		int32 Bays = 1;
		/** The ground floor's openings: shop windows (wide) or windows. */
		bool bShops = false;
		/** The house's seed (the same on all its walls) and this wall's. */
		uint32 House = 0;
		uint32 Wall = 0;
	};

	inline uint32 Mix(uint32 H)
	{
		H ^= H >> 16;
		H *= 0x7feb352du;
		H ^= H >> 15;
		H *= 0x846ca68bu;
		H ^= H >> 16;
		return H;
	}

	// Shares of houses (out of 12) with each kind of detail, picked by the house.
	constexpr uint32 BalconyIronIn12 = 4;
	constexpr uint32 BalconyStoneIn12 = 2;
	constexpr uint32 KeystoneIn12 = 7;
	constexpr uint32 PilasterIn12 = 3;
	constexpr uint32 ConsoleIn12 = 4;
	constexpr uint32 GableIn12 = 4;
	// Wear, picked by the wall (out of 100): a pier with posters, a wall with fallen plaster.
	constexpr uint32 PosterPier = 18;
	constexpr uint32 FallenPlaster = 35;

	/** The middle of a bay along the wall. */
	inline double BayMiddle(const FFront& F, int32 k) { return F.EdgeMargin + F.Bay * (k + 0.5); }
	/** The floor level of an upper floor. */
	inline double FloorBase(const FFront& F, int32 Floor) { return F.GroundTop + (Floor - 1) * F.UpperHeight; }
	/** An upper floor window's sill and top, as AddFacade has them. */
	inline double SillZ(const FFront& F, int32 Floor) { return FloorBase(F, Floor) + F.UpperHeight * 0.22; }
	inline double WindowTopZ(const FFront& F, int32 Floor) { return SillZ(F, Floor) + FMath::Min(230.0, F.UpperHeight * 0.6); }

	/** A balcony in front of a window: a slab on consoles, iron or stone railing. */
	inline void Balcony(double S0, double S1, double Base, bool bStone, TArray<FBox>& Out)
	{
		constexpr double Deep = 90.0;
		const double A = S0 - 40.0;
		const double B = S1 + 40.0;
		Out.Add({ TrimPart, A, B, Base - 4.0, Base + 14.0, 0.0, Deep });
		for (const double S : { A + 18.0, B - 18.0 })
		{
			Out.Add({ TrimPart, S - 9.0, S + 9.0, Base - 30.0, Base - 4.0, 0.0, Deep - 12.0 });
			Out.Add({ TrimPart, S - 8.0, S + 8.0, Base - 58.0, Base - 30.0, 0.0, Deep * 0.45 });
		}
		if (bStone)
		{
			// Stone balustrade: posts at the corners, short balusters, a coping.
			Out.Add({ TrimPart, A + 2.0, B - 2.0, Base + 86.0, Base + 98.0, Deep - 18.0, Deep });
			Out.Add({ TrimPart, A + 2.0, A + 18.0, Base + 14.0, Base + 86.0, 0.0, Deep - 2.0 });
			Out.Add({ TrimPart, B - 18.0, B - 2.0, Base + 14.0, Base + 86.0, 0.0, Deep - 2.0 });
			for (double S = A + 34.0; S < B - 26.0; S += 24.0)
			{
				Out.Add({ TrimPart, S - 5.0, S + 5.0, Base + 14.0, Base + 86.0, Deep - 14.0, Deep - 4.0 });
			}
			for (const double D : { 2.0, 40.0 })
			{
				Out.Add({ TrimPart, A + 2.0, A + 14.0, Base + 86.0, Base + 98.0, D, D + 24.0 });
				Out.Add({ TrimPart, B - 14.0, B - 2.0, Base + 86.0, Base + 98.0, D, D + 24.0 });
			}
			return;
		}
		// Iron: a top rail round the three sides, thin bars, a band of curls near the top.
		Out.Add({ Iron, A, B, Base + 100.0, Base + 104.0, Deep - 4.0, Deep });
		Out.Add({ Iron, A, B, Base + 80.0, Base + 82.0, Deep - 3.0, Deep - 1.0 });
		Out.Add({ Iron, A, B, Base + 16.0, Base + 19.0, Deep - 3.0, Deep - 1.0 });
		for (const double S : { A, B - 4.0 })
		{
			Out.Add({ Iron, S, S + 4.0, Base + 100.0, Base + 104.0, 0.0, Deep });
			Out.Add({ Iron, S, S + 3.0, Base + 14.0, Base + 104.0, Deep - 4.0, Deep });
		}
		for (double S = A + 16.0; S < B - 10.0; S += 16.0)
		{
			Out.Add({ Iron, S - 1.0, S + 1.0, Base + 14.0, Base + 100.0, Deep - 3.0, Deep - 1.0 });
		}
		for (double S = A + 24.0; S < B - 18.0; S += 32.0)
		{
			Out.Add({ Iron, S - 6.0, S + 6.0, Base + 86.0, Base + 94.0, Deep - 2.5, Deep - 1.5 });
		}
	}

	/** Posters pasted on a ground floor pier S0..S1, the older ones under, some torn. */
	inline void Posters(double S0, double S1, uint32 Hash, TArray<FBox>& Out)
	{
		const double Room = S1 - S0;
		if (Room < 50.0)
		{
			return;
		}
		const double Mid = (S0 + S1) * 0.5;
		const double W = FMath::Min(92.0, Room - 12.0);
		const double Z0 = 110.0 + static_cast<double>(Hash % 20u);
		const double H = W * 1.4;
		// The old layer, sun-faded, peeping out round the new one.
		Out.Add({ PosterOld, Mid - W * 0.5 - 4.0, Mid + W * 0.5 + 3.0, Z0 - 6.0, Z0 + H + 4.0, 0.0, 1.0 });
		const int32 Inks[] = { PosterRed, PosterBlue, PosterYellow, PosterOld };
		const int32 Ink = Inks[(Hash >> 5) % 4u];
		const bool bTorn = (Hash >> 8) % 3u == 0u;
		const double Top = bTorn ? Z0 + H * 0.62 : Z0 + H;
		Out.Add({ Ink, Mid - W * 0.5, Mid + W * 0.5, Z0, Top, 1.0, 1.4 });
		if (bTorn)
		{
			// Torn off at the top: a ragged edge of bits, and a corner hanging loose.
			for (int32 i = 0; i < 4; ++i)
			{
				const double S = Mid - W * 0.5 + W * (i + 0.5) / 4.0;
				const double Up = static_cast<double>((Hash >> (10 + i * 2)) % 4u) * 4.0;
				Out.Add({ Ink, S - W / 8.0, S + W / 8.0, Top, Top + Up, 1.0, 1.4 });
			}
			Out.Add({ Ink, Mid + W * 0.5 - 18.0, Mid + W * 0.5, Top + 6.0, Top + 24.0, 2.0, 3.5 });
		}
		else
		{
			// The printed lines: a dark band for the title, a pale one under it.
			Out.Add({ Crack, Mid - W * 0.36, Mid + W * 0.36, Z0 + H * 0.70, Z0 + H * 0.80, 1.4, 1.6 });
			Out.Add({ PosterOld, Mid - W * 0.30, Mid + W * 0.30, Z0 + H * 0.25, Z0 + H * 0.50, 1.4, 1.6 });
			if ((Hash >> 9) % 2u == 0u)
			{
				// The bottom corner peeling off the wall.
				Out.Add({ Ink, Mid + W * 0.5 - 16.0, Mid + W * 0.5, Z0, Z0 + 16.0, 2.0, 3.6 });
			}
		}
		// A small bill stuck on beside it now and then.
		if ((Hash >> 12) % 2u == 0u && Room > W + 40.0)
		{
			Out.Add({ PosterOld, Mid + W * 0.5 + 4.0, Mid + W * 0.5 + 30.0, Z0 + 30.0, Z0 + 64.0, 0.0, 1.0 });
		}
	}

	/** A patch where the plaster has fallen off to the brick, with a crack from its corner. */
	inline void FallenPatch(double S, double Z, uint32 Hash, TArray<FBox>& Out)
	{
		const double W = 50.0 + static_cast<double>(Hash % 80u);
		const double H = 30.0 + static_cast<double>((Hash >> 6) % 50u);
		Out.Add({ Brick, S - W * 0.5, S + W * 0.5, Z - H * 0.5, Z + H * 0.5, 0.0, 0.4 });
		// Ragged edges: smaller bits over the sides.
		Out.Add({ Brick, S - W * 0.3, S + W * 0.15, Z + H * 0.5, Z + H * 0.5 + 10.0, 0.0, 0.4 });
		Out.Add({ Brick, S + W * 0.5, S + W * 0.5 + 12.0, Z - H * 0.3, Z + H * 0.2, 0.0, 0.4 });
		// A few brick joints across it.
		for (double J = Z - H * 0.5 + 8.0; J < Z + H * 0.5; J += 8.0)
		{
			Out.Add({ Crack, S - W * 0.5, S + W * 0.5, J - 0.6, J + 0.6, 0.4, 0.55 });
		}
		// The crack: steps down and to one side.
		const double Side = (Hash >> 11) % 2u == 0u ? 1.0 : -1.0;
		double CS = S + Side * W * 0.5;
		double CZ = Z - H * 0.5;
		for (int32 i = 0; i < 4; ++i)
		{
			const double Drop = 14.0 + static_cast<double>((Hash >> (13 + i)) % 3u) * 6.0;
			Out.Add({ Crack, CS - 0.8, CS + 0.8, CZ - Drop, CZ, 0.0, 0.55 });
			CZ -= Drop;
			const double Step = Side * (6.0 + static_cast<double>((Hash >> (17 + i)) % 3u) * 4.0);
			Out.Add({ Crack, FMath::Min(CS, CS + Step) - 0.8, FMath::Max(CS, CS + Step) + 0.8, CZ - 0.8, CZ + 0.8, 0.0, 0.55 });
			CS += Step;
		}
	}

	/**
	 * A wall's finer details and wear (see the top). House picks the style,
	 * so all walls of one house match; Wall picks the wear.
	 */
	inline void HouseDetails(const FFront& F, TArray<FBox>& Out)
	{
		const uint32 H = Mix(F.House ^ 0x51ed270bu);
		const uint32 BalconyPick = H % 12u;
		const bool bIronBalconies = BalconyPick < BalconyIronIn12;
		const bool bStoneBalconies = !bIronBalconies && BalconyPick < BalconyIronIn12 + BalconyStoneIn12;
		const bool bKeystones = (H >> 4) % 12u < KeystoneIn12;
		const bool bPilasters = (H >> 8) % 12u < PilasterIn12;
		const bool bConsoles = (H >> 12) % 12u < ConsoleIn12;
		const bool bGable = F.Bays >= 5 && F.Storeys >= 3 && (H >> 16) % 12u < GableIn12;
		const double Cornice = F.Height - F.CorniceZone;
		const bool bUpper = F.Storeys > 1 && F.UpperHeight > 250.0;

		if (bUpper && bPilasters)
		{
			for (int32 j = 0; j <= F.Bays; ++j)
			{
				const double S = F.EdgeMargin + F.Bay * j;
				Out.Add({ TrimPart, S - 18.0, S + 18.0, F.GroundTop, Cornice, 0.0, 5.0 });
				Out.Add({ TrimPart, S - 24.0, S + 24.0, Cornice - 22.0, Cornice, 0.0, 9.0 });
			}
		}
		if (bConsoles)
		{
			for (int32 j = 0; j <= F.Bays * 2; ++j)
			{
				const double S = F.EdgeMargin + F.Bay * j * 0.5;
				Out.Add({ TrimPart, S - 7.0, S + 7.0, Cornice - 30.0, Cornice, 0.0, 20.0 });
			}
		}
		if (bUpper)
		{
			for (int32 k = 0; k < F.Bays; ++k)
			{
				const double Mid = BayMiddle(F, k);
				const double S0 = Mid - F.WindowWidth * 0.5;
				const double S1 = Mid + F.WindowWidth * 0.5;
				if (bKeystones)
				{
					for (int32 Floor = 1; Floor < FMath::Min(F.Storeys, 3); ++Floor)
					{
						const double Top = WindowTopZ(F, Floor);
						Out.Add({ TrimPart, Mid - 11.0, Mid + 11.0, Top - 16.0, Top + (Floor == 1 ? 8.0 : 14.0), 0.0, 12.0 });
					}
				}
				// Balconies on the first floor: the middle window, and on long fronts two more.
				const bool bMiddle = k == F.Bays / 2 && F.Bays >= 3;
				const bool bSide = F.Bays >= 7 && (k == 1 || k == F.Bays - 2);
				if ((bIronBalconies || bStoneBalconies) && (bMiddle || bSide))
				{
					Balcony(S0, S1, FloorBase(F, 1), bStoneBalconies, Out);
					// A tall house has one on the second floor over the middle too.
					if (bMiddle && F.Storeys >= 4)
					{
						Balcony(S0, S1, FloorBase(F, 2), bStoneBalconies, Out);
					}
				}
			}
		}
		if (bGable)
		{
			// A stepped attic gable over the middle bays, standing on the cornice,
			// with a round window and two urns.
			const double C = F.Length * 0.5;
			const double W = F.Bay * (F.Bays >= 7 ? 3.0 : 2.0);
			const double Z = F.Height;
			Out.Add({ TrimPart, C - W * 0.5, C + W * 0.5, Z, Z + 110.0, -40.0, 18.0 });
			Out.Add({ TrimPart, C - W * 0.5 + 50.0, C + W * 0.5 - 50.0, Z + 110.0, Z + 170.0, -40.0, 18.0 });
			Out.Add({ TrimPart, C - W * 0.5 - 6.0, C + W * 0.5 + 6.0, Z + 104.0, Z + 114.0, -44.0, 26.0 });
			Out.Add({ TrimPart, C - W * 0.5 + 44.0, C + W * 0.5 - 44.0, Z + 170.0, Z + 180.0, -44.0, 26.0 });
			Out.Add({ TrimPart, C - 34.0, C + 34.0, Z + 180.0, Z + 230.0, -30.0, 14.0 });
			Out.Add({ TrimPart, C - 44.0, C + 44.0, Z + 228.0, Z + 236.0, -34.0, 20.0 });
			Out.Add({ GlassPart, C - 28.0, C + 28.0, Z + 32.0, Z + 88.0, 18.0, 19.0 });
			Out.Add({ TrimPart, C - 36.0, C + 36.0, Z + 24.0, Z + 32.0, 16.0, 24.0 });
			Out.Add({ TrimPart, C - 36.0, C + 36.0, Z + 88.0, Z + 96.0, 16.0, 24.0 });
			for (const double S : { C - W * 0.5 - 30.0, C + W * 0.5 + 30.0 })
			{
				Out.Add({ TrimPart, S - 14.0, S + 14.0, Z, Z + 12.0, 6.0, 34.0 });
				Out.Add({ TrimPart, S - 10.0, S + 10.0, Z + 12.0, Z + 50.0, 10.0, 30.0 });
				Out.Add({ TrimPart, S - 5.0, S + 5.0, Z + 50.0, Z + 64.0, 15.0, 25.0 });
			}
		}

		// Wear: posters on the ground floor piers.
		const uint32 Wear = Mix(F.Wall ^ 0x2545f491u);
		const double Opening = F.bShops ? FMath::Min(240.0, F.Bay - 40.0) + 20.0 : F.WindowWidth;
		for (int32 j = 1; j < F.Bays; ++j)
		{
			const uint32 Pier = Mix(Wear + static_cast<uint32>(j) * 0x9e3779b9u);
			if (Pier % 100u >= PosterPier)
			{
				continue;
			}
			const double S = F.EdgeMargin + F.Bay * j;
			const double Half = (F.Bay - Opening) * 0.5 - 6.0;
			Posters(S - Half, S + Half, Pier >> 7, Out);
		}
		// Plaster fallen off: at the wall's foot above the plinth, or between upper windows.
		if (Wear % 100u < FallenPlaster)
		{
			const int32 Patches = (Wear >> 8) % 3u == 0u ? 2 : 1;
			for (int32 i = 0; i < Patches; ++i)
			{
				const uint32 P = Mix(Wear + static_cast<uint32>(i + 1) * 0x85ebca6bu);
				const int32 k = static_cast<int32>(P % static_cast<uint32>(F.Bays));
				const bool bHigh = bUpper && !bPilasters && (P >> 4) % 2u == 0u && F.Bays > 1;
				if (bHigh)
				{
					const int32 Floor = 1 + static_cast<int32>((P >> 6) % static_cast<uint32>(F.Storeys - 1));
					const double S = F.EdgeMargin + F.Bay * FMath::Clamp(k, 1, F.Bays - 1);
					FallenPatch(S, (SillZ(F, Floor) + WindowTopZ(F, Floor)) * 0.5, P >> 9, Out);
				}
				else
				{
					const double S = F.EdgeMargin + F.Bay * FMath::Clamp(k, 0, F.Bays - 1) + F.Bay * 0.5 + ((P >> 5) % 2u == 0u ? 1.0 : -1.0) * F.Bay * 0.5;
					FallenPatch(FMath::Clamp(S, 40.0, F.Length - 40.0), 95.0, P >> 9, Out);
				}
			}
		}
	}
}
