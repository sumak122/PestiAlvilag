// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * Life on the house fronts, drawn into the facades by AOsmGraybox::AddFacade:
 * flower boxes of geraniums on the window sills, and washing hung out of a
 * window on a line between two iron arms.
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
		NumParts
	};

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
}
