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
	// A headlamp whose middle is pushed in this far is knocked off, and a
	// bumper pushed in this far anywhere along it falls off (cm).
	constexpr double LampOffDepth = 7.0;
	constexpr double BumperOffDepth = 13.0;
	// The headlamp (drum, lens and stalk) is everything this close to the drum's middle (cm).
	constexpr double LampReach = 16.0;
	// A door comes off when the dents push its middle in this far (cm), the
	// bonnet when they push its front or back corner in this far.
	constexpr double DoorOffDepth = 15.0;
	constexpr double BonnetOffDepth = 12.0;
	// A doorway shorter than this (cm) is no door.
	constexpr double MinDoor = 20.0;
	// A wheel comes off in a crash this fast (cm/s, straight into it) within this reach of its hub (cm, flat).
	constexpr double WheelOffSpeed = 1600.0;
	constexpr double WheelReach = 75.0;
	// ...and only this far in from the wheel's own side (cm).
	constexpr double WheelSideIn = 30.0;
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
		/** Which parts have fallen off (bits of ELoose). */
		int32 Fallen = 0;
	};

	/** Parts that can be knocked off. */
	enum ELoose : int32
	{
		LampLeft,
		LampRight,
		FrontBumper,
		RearBumper,
		DoorLeft,
		DoorRight,
		Bonnet,
		// The wheels, in CarModel::WheelHub's order: front left, front right, rear left, rear right.
		WheelFrontLeft,
		WheelFrontRight,
		WheelRearLeft,
		WheelRearRight,
		LooseCount
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

	/** The middle of each headlamp's drum (left, right). */
	inline void LampDrums(const CarModel::FBuilder& Clean, FVector Out[2])
	{
		LampMiddles(Clean.Parts[CarModel::Lamp], true, Out);
		for (int32 Side = 0; Side < 2; ++Side)
		{
			Out[Side].X -= 7.0;
		}
	}

	/** Which loose part (ELoose) this piece of the body, of this part, with its middle here, belongs to, or INDEX_NONE. */
	inline int32 LooseOf(int32 Part, const FVector& Middle, const FVector& Facing, const CarModel::FLayout& L, const FVector Drums[2])
	{
		// The bonnet's top: the painted panels facing up between the windscreen and
		// the radiator, and the chrome strip along it.
		// (Not the radiator shell or its cap, at the front: they stay on the car.)
		if (Middle.X > L.Cowl + 2.0 && Middle.X < L.Nose - 6.0 && FMath::Abs(Middle.Y) < 42.0
			&& ((Part == CarModel::Paint && Facing.Z > 0.5 && Middle.Z > -5.0)
				|| (Part == CarModel::Chrome && Middle.Z > CarModel::Waist - 6.0 && FMath::Abs(Middle.Y) < 2.0 && Middle.X < L.Nose - 16.0)))
		{
			return Bonnet;
		}
		if (Part == CarModel::Chrome || Part == CarModel::Lamp)
		{
			for (int32 Side = 0; Side < 2; ++Side)
			{
				if ((Middle - Drums[Side]).Size() < LampReach)
				{
					return Side == 0 ? LampLeft : LampRight;
				}
			}
		}
		if (Part == CarModel::Chrome && Middle.Z < BumperTop)
		{
			if (Middle.X > L.Nose + 10.0)
			{
				return FrontBumper;
			}
			if (Middle.X < L.Tail - 8.0)
			{
				return RearBumper;
			}
		}
		return INDEX_NONE;
	}

	/** Which way a triangle of a part faces (its corners' normals together). */
	inline FVector FacingOf(const CarModel::FPart& Part, int32 T)
	{
		return (Part.Normals[Part.Triangles[T]] + Part.Normals[Part.Triangles[T + 1]] + Part.Normals[Part.Triangles[T + 2]]).GetSafeNormal();
	}

	/** Whether this car has front doors, and where the middle of one (side 0 left, 1 right) is. */
	inline bool HasDoors(const CarModel::FLayout& L)
	{
		return L.DoorFront - L.DoorBack >= MinDoor;
	}
	inline FVector DoorMiddle(const CarModel::FLayout& L, int32 Side)
	{
		return FVector((L.DoorFront + L.DoorBack) * 0.5, (Side == 0 ? -1.0 : 1.0) * L.HalfWidth, -12.0);
	}

	/** The box round the bonnet's top (its loose pieces), or an empty box. */
	inline FBox BonnetBox(const CarModel::FBuilder& Clean, const CarModel::FLayout& L, const FVector Drums[2])
	{
		FBox Box(ForceInit);
		for (int32 p = 0; p < CarModel::PartCount; ++p)
		{
			const CarModel::FPart& From = Clean.Parts[p];
			for (int32 t = 0; t + 2 < From.Triangles.Num(); t += 3)
			{
				const FVector Middle = (From.Vertices[From.Triangles[t]] + From.Vertices[From.Triangles[t + 1]] + From.Vertices[From.Triangles[t + 2]]) / 3.0;
				if (LooseOf(p, Middle, FacingOf(From, t), L, Drums) == Bonnet)
				{
					for (int32 k = 0; k < 3; ++k)
					{
						Box += From.Vertices[From.Triangles[t + k]];
					}
				}
			}
		}
		return Box;
	}

	/**
	 * Which wheel (ELoose) a crash at At (car frame), this fast (cm/s), knocks
	 * off, or INDEX_NONE: only a hard one, near the wheel's hub, one still on.
	 */
	inline int32 WheelOff(const CarModel::FLayout& L, const FState& State, const FVector& At, double Speed)
	{
		// Only a crash into the car's side or corner, out where the wheels are.
		if (Speed < WheelOffSpeed || FMath::Abs(At.Y) < L.TrackY - WheelSideIn)
		{
			return INDEX_NONE;
		}
		int32 Best = INDEX_NONE;
		double BestDistance = WheelReach;
		for (int32 i = 0; i < 4; ++i)
		{
			const FVector2D Hub(i < 2 ? L.FrontAxle : L.RearAxle, (i % 2 == 0 ? -1.0 : 1.0) * L.TrackY);
			const double Distance = (FVector2D(At.X, At.Y) - Hub).Size();
			if (Distance < BestDistance && (State.Fallen & (1 << (WheelFrontLeft + i))) == 0)
			{
				Best = WheelFrontLeft + i;
				BestDistance = Distance;
			}
		}
		return Best;
	}

	/**
	 * Knocks parts off that the dents have pushed in far enough. Returns the
	 * ones that have just come off (bits of ELoose), now also in State.Fallen.
	 */
	inline int32 KnockOff(const CarModel::FBuilder& Clean, const CarModel::FLayout& L, FState& State)
	{
		FVector Drums[2];
		LampDrums(Clean, Drums);
		int32 Off = 0;
		for (int32 Side = 0; Side < 2; ++Side)
		{
			if (Offset(State, Drums[Side], false).Size() > LampOffDepth)
			{
				Off |= 1 << (LampLeft + Side);
			}
		}
		const double Ends[2] = { L.Nose + 18.0, L.Tail - 14.0 };
		for (int32 End = 0; End < 2; ++End)
		{
			for (const double Y : { -70.0, -35.0, 0.0, 35.0, 70.0 })
			{
				if (Offset(State, FVector(Ends[End], Y, -58.0), false).Size() > BumperOffDepth)
				{
					Off |= 1 << (FrontBumper + End);
				}
			}
		}
		if (HasDoors(L))
		{
			for (int32 Side = 0; Side < 2; ++Side)
			{
				if (Offset(State, DoorMiddle(L, Side), false).Size() > DoorOffDepth)
				{
					Off |= 1 << (DoorLeft + Side);
				}
			}
		}
		const FBox Lid = BonnetBox(Clean, L, Drums);
		if (Lid.IsValid)
		{
			for (const double X : { Lid.Min.X + 8.0, Lid.Max.X - 8.0 })
			{
				for (const double Y : { Lid.Min.Y + 4.0, Lid.Max.Y - 4.0 })
				{
					if (Offset(State, FVector(X, Y, Lid.Max.Z), false).Size() > BonnetOffDepth)
					{
						Off |= 1 << Bonnet;
					}
				}
			}
		}
		const int32 Fresh = Off & ~State.Fallen;
		State.Fallen |= Off;
		return Fresh;
	}

	/** The shape of one loose part (ELoose), as it was on the car (car frame), and its middle. */
	inline void BuildLoose(const CarModel::FBuilder& Clean, const CarModel::FLayout& L, int32 Which, CarModel::FBuilder& Out, FVector& OutMiddle)
	{
		if (Which == DoorLeft || Which == DoorRight)
		{
			// A door is no piece of the body's shape: a painted panel the door's size, with its handle.
			const double S = Which == DoorLeft ? -1.0 : 1.0;
			const double W = L.HalfWidth;
			Out.Box(CarModel::Paint, FVector(L.DoorBack + 1.0, FMath::Min(S * (W - 2.0), S * W), -38.0), FVector(L.DoorFront - 1.0, FMath::Max(S * (W - 2.0), S * W), CarModel::Waist - 1.0));
			Out.Box(CarModel::Chrome, FVector(L.DoorBack + 3.0, FMath::Min(S * W, S * (W + 1.4)), CarModel::Waist - 9.0), FVector(L.DoorBack + 10.0, FMath::Max(S * W, S * (W + 1.4)), CarModel::Waist - 7.0));
			OutMiddle = FVector((L.DoorFront + L.DoorBack) * 0.5, S * (W - 1.0), (CarModel::Waist - 39.0) * 0.5);
			return;
		}
		FVector Drums[2];
		LampDrums(Clean, Drums);
		FVector Sum = FVector::ZeroVector;
		int32 Count = 0;
		for (int32 p = 0; p < CarModel::PartCount; ++p)
		{
			const CarModel::FPart& From = Clean.Parts[p];
			CarModel::FPart& To = Out.Parts[p];
			for (int32 t = 0; t + 2 < From.Triangles.Num(); t += 3)
			{
				const FVector Middle = (From.Vertices[From.Triangles[t]] + From.Vertices[From.Triangles[t + 1]] + From.Vertices[From.Triangles[t + 2]]) / 3.0;
				if (LooseOf(p, Middle, FacingOf(From, t), L, Drums) != Which)
				{
					continue;
				}
				for (int32 k = 0; k < 3; ++k)
				{
					const int32 I = From.Triangles[t + k];
					To.Triangles.Add(To.Vertices.Num());
					To.Vertices.Add(From.Vertices[I]);
					To.Normals.Add(From.Normals[I]);
					Sum += From.Vertices[I];
					++Count;
				}
			}
		}
		OutMiddle = Count > 0 ? Sum / Count : FVector::ZeroVector;
	}

	/**
	 * The body Clean with State's dents in it, into Out, without the parts
	 * knocked off. Broken lamps' glass goes dark (into Rubber) and
	 * OutLampBroken / OutTailBroken say which (left, right); a knocked-off
	 * lamp counts as broken.
	 */
	inline void Apply(const CarModel::FBuilder& Clean, const CarModel::FLayout& L, const FState& State, CarModel::FBuilder& Out,
		bool OutLampBroken[2], bool OutTailBroken[2])
	{
		FVector Drums[2];
		LampDrums(Clean, Drums);
		FVector LampAt[2], TailAt[2];
		LampMiddles(Clean.Parts[CarModel::Lamp], true, LampAt);
		LampMiddles(Clean.Parts[CarModel::TailLamp], false, TailAt);
		for (int32 Side = 0; Side < 2; ++Side)
		{
			OutLampBroken[Side] = Offset(State, LampAt[Side], false).Size() > LampBreakDepth || (State.Fallen & (1 << (LampLeft + Side))) != 0;
			OutTailBroken[Side] = Offset(State, TailAt[Side], false).Size() > LampBreakDepth;
		}

		for (int32 p = 0; p < CarModel::PartCount; ++p)
		{
			Out.Parts[p] = CarModel::FPart();
		}
		// Where a door has come off, a dark doorway.
		for (int32 Side = 0; Side < 2; ++Side)
		{
			if ((State.Fallen & (1 << (DoorLeft + Side))) != 0 && HasDoors(L))
			{
				const double S = Side == 0 ? -1.0 : 1.0;
				const double Y0 = S * (L.HalfWidth + 0.1), Y1 = S * (L.HalfWidth + 0.4);
				Out.Box(CarModel::Rubber, FVector(L.DoorBack + 0.5, FMath::Min(Y0, Y1), -38.0), FVector(L.DoorFront - 0.5, FMath::Max(Y0, Y1), CarModel::Waist - 2.0));
			}
		}
		// Where the bonnet has come off, the engine under it, in a dark bay.
		if ((State.Fallen & (1 << Bonnet)) != 0)
		{
			const FBox Lid = BonnetBox(Clean, L, Drums);
			if (Lid.IsValid)
			{
				const double Top = Lid.Min.Z;
				const double HalfY = FMath::Max(FMath::Abs(Lid.Min.Y), FMath::Abs(Lid.Max.Y)) - 1.0;
				const double X0 = Lid.Min.X + 0.5, X1 = Lid.Max.X - 0.5, Z0 = Top - 34.0;
				// The bonnet sides' insides (in the paint), the bulkhead and the radiator's back (dark).
				const double WallTop = Top - 2.0;
				Out.Pane(CarModel::Paint, FVector(X0, -HalfY, Z0), FVector(X1, -HalfY, Z0), FVector(X1, -HalfY, WallTop), FVector(X0, -HalfY, WallTop));
				Out.Pane(CarModel::Paint, FVector(X0, HalfY, Z0), FVector(X1, HalfY, Z0), FVector(X1, HalfY, WallTop), FVector(X0, HalfY, WallTop));
				Out.Pane(CarModel::Guards, FVector(X0, -HalfY, Z0), FVector(X0, HalfY, Z0), FVector(X0, HalfY, WallTop), FVector(X0, -HalfY, WallTop));
				Out.Pane(CarModel::Guards, FVector(X1, -HalfY, Z0), FVector(X1, HalfY, Z0), FVector(X1, HalfY, WallTop), FVector(X1, -HalfY, WallTop));
				// The engine: a dark block, its head on top, a rusty exhaust manifold
				// down one side, the hose to the radiator and the fan in front.
				const double EngineHalf = FMath::Min(HalfY * 0.45, 16.0);
				const double Front = X1 - 10.0;
				const double Rear = FMath::Max(X0 + 6.0, Front - 70.0);
				Out.Box(CarModel::Rubber, FVector(Rear, -EngineHalf, Z0), FVector(Front, EngineHalf, Top - 14.0));
				Out.Box(CarModel::Chrome, FVector(Rear + 4.0, -EngineHalf + 3.0, Top - 14.0), FVector(Front - 4.0, EngineHalf - 3.0, Top - 9.0));
				for (int32 k = 0; k < 4; ++k)
				{
					const double X = Rear + 10.0 + (Front - Rear - 20.0) * k / 3.0;
					Out.Bar(CarModel::Rubber, FVector(X, EngineHalf - 6.0, Top - 9.0), FVector(X, EngineHalf - 6.0, Top - 6.0), 1.6);
					Out.Bar(CarModel::Leather, FVector(X, -EngineHalf, Top - 18.0), FVector(X, -EngineHalf - 6.0, Top - 26.0), 3.0);
				}
				Out.Bar(CarModel::Leather, FVector(Rear + 6.0, -EngineHalf - 6.0, Top - 26.0), FVector(Front - 6.0, -EngineHalf - 6.0, Top - 26.0), 4.0);
				Out.Bar(CarModel::Rubber, FVector(Front - 2.0, 0.0, Top - 11.0), FVector(X1 - 1.0, 0.0, Top - 5.0), 4.0);
				Out.Cylinder(CarModel::Chrome, FVector(Front + 3.0, 0.0, Top - 20.0), 11.0, 1.0, 0, 6);
			}
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

				// Knocked off: gone from the car.
				if (State.Fallen != 0)
				{
					const int32 Loose = LooseOf(p, (A + B + C) / 3.0, FacingOf(From, t), L, Drums);
					if (Loose != INDEX_NONE && (State.Fallen & (1 << Loose)) != 0)
					{
						continue;
					}
				}

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
