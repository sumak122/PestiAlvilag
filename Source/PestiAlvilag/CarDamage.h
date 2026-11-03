// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CarModel.h"

/**
 * Crash damage on a CarModel body. Each hard knock leaves a dent: the body
 * is pushed in around where it was hit, crumpled unevenly, the chrome bent
 * and sagging, glass nearby shattered out, and a lamp in the way broken
 * (dark, and no longer lit). Knocks close together deepen one dent.
 *
 * Header-only (like CarModel) so the offline preview builds the same shapes.
 * Everything is in the car's frame (CarModel: X forward, Y right, Z up).
 */
namespace CarDamage
{
	// Knocks slower than this leave no mark (cm/s); faster ones dent this deep
	// per cm/s over it, up to the most one knock can do, and a dent knocked
	// again deepens up to the second (cm).
	constexpr double MinSpeed = 250.0;
	constexpr double DepthPerSpeed = 20.0 / 2000.0;
	constexpr double MaxDepth = 20.0;
	constexpr double MaxDentDepth = 30.0;
	// A dent reaches this far round where it was hit, more the deeper it is (cm).
	constexpr double BaseRadius = 45.0;
	constexpr double RadiusPerDepth = 2.0;
	constexpr double MaxRadius = 90.0;
	// Knocks this close to a dent (share of its reach) deepen it instead of making a new one.
	constexpr double MergeShare = 0.6;
	// A car keeps at most this many dents (the shallowest goes first).
	constexpr int32 MaxDents = 16;
	// The bent metal is cut into pieces about this big (cm), but at most 4^MaxLevel per piece of the body.
	constexpr double PieceSize = 14.0;
	constexpr int32 MaxLevel = 3;
	// Glass pushed in this far shatters out (cm, a little more or less here and there).
	constexpr double ShatterDepth = 4.0;
	// A lamp whose middle is pushed in this far breaks (cm).
	constexpr double LampBreakDepth = 2.5;
	// Bent bumpers (chrome below this height, cm) also sag this much for each cm they are pushed in.
	constexpr double BumperTop = -45.0;
	constexpr double ChromeSag = 0.5;
	// How worn out the car gets for each cm/s of a knock over MinSpeed (1 is done for).
	constexpr double WearPerSpeed = 1.0 / 7000.0;

	struct FDent
	{
		/** Where it was hit, and the way it was pushed (into the car). */
		FVector At = FVector::ZeroVector;
		FVector Into = FVector::ZeroVector;
		double Depth = 0.0;
		double Radius = 0.0;
		/** Each dent crumples its own way. */
		int32 Seed = 0;
	};

	struct FState
	{
		TArray<FDent> Dents;
		/** 0 like new, 1 worn out: the engine steams, smokes and at last stops. */
		double Wear = 0.0;
	};

	/** 0 to 1, the same for the same whole numbers. */
	inline double Hash(int32 X, int32 Y, int32 Z)
	{
		uint32 H = static_cast<uint32>(X) * 73856093u ^ static_cast<uint32>(Y) * 19349663u ^ static_cast<uint32>(Z) * 83492791u;
		H ^= H >> 13;
		H *= 0x5bd1e995u;
		H ^= H >> 15;
		return static_cast<double>(H & 0xFFFFu) / 65535.0;
	}

	/** Smooth noise, 0 to 1, changing about once per unit. */
	inline double Noise(const FVector& P)
	{
		const int32 X0 = FMath::FloorToInt(P.X), Y0 = FMath::FloorToInt(P.Y), Z0 = FMath::FloorToInt(P.Z);
		auto Ease = [](double T) { return T * T * (3.0 - 2.0 * T); };
		const double TX = Ease(P.X - X0), TY = Ease(P.Y - Y0), TZ = Ease(P.Z - Z0);
		double Sum = 0.0;
		for (int32 Corner = 0; Corner < 8; ++Corner)
		{
			const int32 DX = Corner & 1, DY = (Corner >> 1) & 1, DZ = (Corner >> 2) & 1;
			const double Weight = (DX ? TX : 1.0 - TX) * (DY ? TY : 1.0 - TY) * (DZ ? TZ : 1.0 - TZ);
			Sum += Weight * Hash(X0 + DX, Y0 + DY, Z0 + DZ);
		}
		return Sum;
	}

	/**
	 * A knock at At (on the body, car frame), pushing Into (towards the
	 * inside of the car), at Speed (cm/s). Returns whether it left a mark.
	 */
	inline bool Knock(FState& State, const FVector& At, const FVector& Into, double Speed)
	{
		const double Depth = FMath::Min((Speed - MinSpeed) * DepthPerSpeed, MaxDepth);
		if (Depth < 0.5)
		{
			return false;
		}
		State.Wear = FMath::Min(1.0, State.Wear + (Speed - MinSpeed) * WearPerSpeed);
		const FVector Push = Into.GetSafeNormal();
		for (FDent& Dent : State.Dents)
		{
			if ((Dent.At - At).Size() < Dent.Radius * MergeShare)
			{
				Dent.Depth = FMath::Min(Dent.Depth + Depth * 0.7, MaxDentDepth);
				Dent.Radius = FMath::Min(FMath::Max(Dent.Radius, BaseRadius + RadiusPerDepth * Dent.Depth), MaxRadius);
				Dent.Into = (Dent.Into + Push).GetSafeNormal();
				return true;
			}
		}
		FDent Made;
		Made.At = At;
		Made.Into = Push;
		Made.Depth = Depth;
		Made.Radius = FMath::Min(BaseRadius + RadiusPerDepth * Depth, MaxRadius);
		Made.Seed = State.Dents.Num() * 31 + FMath::FloorToInt(Speed);
		if (State.Dents.Num() < MaxDents)
		{
			State.Dents.Add(Made);
			return true;
		}
		int32 Shallowest = 0;
		for (int32 i = 1; i < State.Dents.Num(); ++i)
		{
			if (State.Dents[i].Depth < State.Dents[Shallowest].Depth)
			{
				Shallowest = i;
			}
		}
		State.Dents[Shallowest] = Made;
		return true;
	}

	/** How far the dents move this point of the body (cm). */
	inline FVector Offset(const FState& State, const FVector& P, bool bChrome)
	{
		FVector Out = FVector::ZeroVector;
		for (const FDent& Dent : State.Dents)
		{
			const double Near = (P - Dent.At).SizeSquared() / (Dent.Radius * Dent.Radius);
			if (Near >= 1.0)
			{
				continue;
			}
			const double Falloff = (1.0 - Near) * (1.0 - Near);
			// Crumpled, not smooth: some places pushed in more, some less.
			const FVector Shift(Dent.Seed * 7.3, Dent.Seed * 3.1, Dent.Seed * 5.7);
			const double Crumple = 0.65 + 0.7 * Noise(P / 15.0 + Shift);
			Out += Dent.Into * (Dent.Depth * Falloff * Crumple);
			if (bChrome && P.Z < BumperTop)
			{
				Out.Z -= ChromeSag * Dent.Depth * Falloff;
			}
		}
		return Out;
	}

	/** The middle of each side's lamps of this part (side 0 left, 1 right), at the front or the back. */
	inline void LampMiddles(const CarModel::FPart& Part, bool bFront, FVector Out[2])
	{
		FVector Sum[2] = { FVector::ZeroVector, FVector::ZeroVector };
		int32 Count[2] = { 0, 0 };
		for (const FVector& V : Part.Vertices)
		{
			if ((V.X > 0.0) != bFront)
			{
				continue;
			}
			const int32 Side = V.Y < 0.0 ? 0 : 1;
			Sum[Side] += V;
			++Count[Side];
		}
		for (int32 Side = 0; Side < 2; ++Side)
		{
			Out[Side] = Count[Side] > 0 ? Sum[Side] / Count[Side] : FVector(0.0, 0.0, 10000.0);
		}
	}

	/**
	 * The body Clean with State's dents in it, into Out. Broken lamps' glass
	 * goes dark (into Rubber) and OutLampBroken / OutTailBroken say which
	 * (left, right).
	 */
	inline void Apply(const CarModel::FBuilder& Clean, const FState& State, CarModel::FBuilder& Out,
		bool OutLampBroken[2], bool OutTailBroken[2])
	{
		FVector LampAt[2], TailAt[2];
		LampMiddles(Clean.Parts[CarModel::Lamp], true, LampAt);
		LampMiddles(Clean.Parts[CarModel::TailLamp], false, TailAt);
		for (int32 Side = 0; Side < 2; ++Side)
		{
			OutLampBroken[Side] = Offset(State, LampAt[Side], false).Size() > LampBreakDepth;
			OutTailBroken[Side] = Offset(State, TailAt[Side], false).Size() > LampBreakDepth;
		}

		for (int32 p = 0; p < CarModel::PartCount; ++p)
		{
			Out.Parts[p] = CarModel::FPart();
		}
		for (int32 p = 0; p < CarModel::PartCount; ++p)
		{
			const CarModel::FPart& From = Clean.Parts[p];
			const bool bChrome = p == CarModel::Chrome;
			const bool bGlass = p == CarModel::Glass;
			TArray<int32> Kept;
			for (int32 i = 0; i < From.Vertices.Num(); ++i)
			{
				Kept.Add(INDEX_NONE);
			}
			for (int32 t = 0; t + 2 < From.Triangles.Num(); t += 3)
			{
				const int32 I0 = From.Triangles[t], I1 = From.Triangles[t + 1], I2 = From.Triangles[t + 2];
				const FVector A = From.Vertices[I0], B = From.Vertices[I1], C = From.Vertices[I2];

				// A broken lamp's glass goes dark.
				int32 Target = p;
				if (p == CarModel::Lamp || p == CarModel::TailLamp)
				{
					const int32 Side = (A.Y + B.Y + C.Y) < 0.0 ? 0 : 1;
					const bool bFront = A.X + B.X + C.X > 0.0;
					const bool bBroken = p == CarModel::Lamp ? (bFront && OutLampBroken[Side]) : (!bFront && OutTailBroken[Side]);
					Target = bBroken ? CarModel::Rubber : p;
				}
				CarModel::FPart& To = Out.Parts[Target];

				// Out of every dent's reach: as it was.
				const FVector Middle = (A + B + C) / 3.0;
				const double Size = FMath::Max((A - Middle).Size(), FMath::Max((B - Middle).Size(), (C - Middle).Size()));
				bool bReached = false;
				for (const FDent& Dent : State.Dents)
				{
					bReached = bReached || (Middle - Dent.At).Size() < Dent.Radius + Size;
				}
				if (!bReached)
				{
					for (const int32 I : { I0, I1, I2 })
					{
						if (Kept[I] == INDEX_NONE || Target != p)
						{
							Kept[I] = To.Vertices.Num();
							To.Vertices.Add(From.Vertices[I]);
							To.Normals.Add(From.Normals[I]);
						}
						To.Triangles.Add(Kept[I]);
					}
					if (Target != p)
					{
						Kept[I0] = Kept[I1] = Kept[I2] = INDEX_NONE;
					}
					continue;
				}

				// In reach: cut it into small pieces, each pushed in and turned its own way.
				const double Longest = FMath::Max((B - A).Size(), FMath::Max((C - B).Size(), (A - C).Size()));
				int32 Level = 0;
				while (Level < MaxLevel && Longest / (1 << Level) > PieceSize)
				{
					++Level;
				}
				const int32 Cuts = 1 << Level;
				const FVector Facing = (From.Normals[I0] + From.Normals[I1] + From.Normals[I2]).GetSafeNormal();
				auto Point = [&](int32 U, int32 W)
				{
					const FVector Flat = A + (B - A) * (static_cast<double>(U) / Cuts) + (C - A) * (static_cast<double>(W) / Cuts);
					return Flat + Offset(State, Flat, bChrome);
				};
				auto Piece = [&](const FVector& P0, const FVector& P1, const FVector& P2, const FVector& FlatMiddle)
				{
					if (bGlass)
					{
						const double Pushed = Offset(State, FlatMiddle, false).Size();
						if (Pushed > ShatterDepth * (0.6 + 0.8 * Noise(FlatMiddle / 6.0)))
						{
							return;
						}
					}
					FVector Normal = FVector::CrossProduct(P1 - P0, P2 - P0).GetSafeNormal();
					if (Normal.IsNearlyZero())
					{
						Normal = Facing;
					}
					else if (FVector::DotProduct(Normal, Facing) < 0.0)
					{
						Normal = -Normal;
					}
					const int32 First = To.Vertices.Num();
					To.Vertices.Add(P0);
					To.Vertices.Add(P1);
					To.Vertices.Add(P2);
					for (int32 k = 0; k < 3; ++k)
					{
						To.Normals.Add(Normal);
						To.Triangles.Add(First + k);
					}
				};
				for (int32 U = 0; U < Cuts; ++U)
				{
					for (int32 W = 0; U + W < Cuts; ++W)
					{
						const FVector FlatA = A + (B - A) * ((U + 0.33) / Cuts) + (C - A) * ((W + 0.33) / Cuts);
						Piece(Point(U, W), Point(U + 1, W), Point(U, W + 1), FlatA);
						if (U + W + 1 < Cuts)
						{
							const FVector FlatB = A + (B - A) * ((U + 0.67) / Cuts) + (C - A) * ((W + 0.67) / Cuts);
							Piece(Point(U + 1, W), Point(U + 1, W + 1), Point(U, W + 1), FlatB);
						}
					}
				}
			}
		}
	}

	/** Where steam and smoke come out when the car is worn: the radiator cap. */
	inline FVector RadiatorCap(const CarModel::FLayout& L)
	{
		return FVector(L.Nose - 5.0, 0.0, CarModel::Waist + 10.0);
	}
}
