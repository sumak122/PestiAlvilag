// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ParkModel.h"
#include "PersonKit.h"

/**
 * The shop fronts of Népszínház utca, as they were in the early 1930s: a
 * painted wooden "portál" set against the ground floor of the tenement,
 * with pilasters at the ends, a deep fascia carrying the owner's name in
 * gilt (or the trade, or the shop's sign name), a cornice, shop windows
 * on a panelled stallriser with more gold lettering on the glass above, a recessed glazed door with a
 * stone step, and goods in the window for each trade. Some have a striped
 * canvas awning, a hanging sign or the trade's own sign (a barber's brass
 * basin, a watchmaker's clock), goods set out on the pavement, or the
 * iron shutter down. The tenement gates between the shops get a stone
 * surround, panelled double doors and the blue house number plate.
 *
 * Header-only and free of engine objects, like ParkModel, so the offline
 * preview builds the same shapes. A shop's frame: X out of the wall into
 * the street, Y along the wall, Z up from the pavement; the middle of the
 * shop's width is at Y = 0 and the wall face at X = 0. Parts are
 * ParkModel parts (engine basic shapes, sizes in cm).
 */
namespace ShopModel
{
	using ParkModel::FPart;

	/** The trades, in the order the furniture script names them (see TradeOf). */
	enum class ETrade : uint8
	{
		Grocer, Baker, Butcher, Shoes, Hats, Drapery, Watches, Tobacco, Barber,
		Tavern, Greengrocer, Hardware, Pharmacy, Dairy, Stationery, Cafe, NumTrades
	};

	/** Which words of the shop a piece of lettering shows. */
	enum class ELine : uint8 { Fascia, Glass, Blade, Number };

	/** Lettering on the shop: where its middle is, which way it faces (yaw in the shop's frame, 0 = out to the street), and its letter height. */
	struct FLettering
	{
		ELine Line;
		FVector At;
		double Yaw;
		double Size;
		/** How wide the lettering may run (cm); longer words are drawn smaller to fit. */
		double Room;
		FLinearColor Color;
	};

	// Heights of the shop front (cm from the pavement).
	constexpr double PlinthTop = 15.0;
	constexpr double SillZ = 72.0;
	constexpr double TransomZ = 262.0;
	constexpr double WindowTop = 322.0;
	constexpr double FasciaBottom = 332.0;
	constexpr double FasciaTop = 398.0;
	constexpr double CorniceTop = 414.0;
	// How far its parts stand out of the wall (cm).
	constexpr double FrontX = 34.0;
	constexpr double PilasterWidth = 32.0;
	constexpr double DoorWidth = 104.0;
	/** How far the whole front stands off the wall, clear of the tenement facade's own ground floor (cm). */
	constexpr double StandOff = 11.0;
	/** Shopkeepers stand this far out, beside the door (cm). */
	constexpr double KeeperOut = 85.0;

	namespace Colors
	{
		/** The paint of the woodwork: bottle green, oxblood, walnut brown, black, navy, grey green. */
		const FLinearColor Paints[] = {
			FLinearColor(0.035f, 0.11f, 0.06f), FLinearColor(0.20f, 0.035f, 0.025f), FLinearColor(0.14f, 0.075f, 0.035f),
			FLinearColor(0.022f, 0.022f, 0.022f), FLinearColor(0.03f, 0.05f, 0.11f), FLinearColor(0.16f, 0.21f, 0.16f) };
		const FLinearColor Cream(0.62f, 0.57f, 0.45f);
		const FLinearColor Gilt(0.72f, 0.52f, 0.17f);
		const FLinearColor Brass(0.62f, 0.45f, 0.16f);
		const FLinearColor Inside(0.06f, 0.048f, 0.036f);
		const FLinearColor Shelf(0.30f, 0.20f, 0.11f);
		const FLinearColor Step(0.50f, 0.48f, 0.44f);
		const FLinearColor Stone(0.55f, 0.51f, 0.44f);
		const FLinearColor StoneDark(0.42f, 0.39f, 0.34f);
		const FLinearColor GateWood(0.16f, 0.09f, 0.045f);
		const FLinearColor Iron(0.04f, 0.04f, 0.045f);
		const FLinearColor Shutter(0.30f, 0.31f, 0.31f);
		const FLinearColor ShutterDark(0.22f, 0.23f, 0.23f);
		const FLinearColor Enamel(0.05f, 0.14f, 0.42f);
		const FLinearColor White(0.85f, 0.84f, 0.80f);
		const FLinearColor Curtain(0.70f, 0.66f, 0.55f);
		const FLinearColor Marble(0.74f, 0.73f, 0.70f);
		/** Awning canvas: a stripe colour to go with cream. */
		const FLinearColor Stripes[] = {
			FLinearColor(0.42f, 0.07f, 0.05f), FLinearColor(0.08f, 0.22f, 0.12f), FLinearColor(0.10f, 0.14f, 0.32f),
			FLinearColor(0.50f, 0.30f, 0.08f) };
		const FLinearColor Crust(0.52f, 0.30f, 0.10f);
		const FLinearColor DarkCrust(0.36f, 0.18f, 0.06f);
		const FLinearColor Meat(0.50f, 0.13f, 0.09f);
		const FLinearColor Sausage(0.36f, 0.12f, 0.07f);
		const FLinearColor Leathers[] = { FLinearColor(0.05f, 0.03f, 0.02f), FLinearColor(0.22f, 0.10f, 0.04f), FLinearColor(0.03f, 0.03f, 0.03f), FLinearColor(0.35f, 0.22f, 0.12f) };
		const FLinearColor Felts[] = { FLinearColor(0.10f, 0.09f, 0.08f), FLinearColor(0.25f, 0.20f, 0.14f), FLinearColor(0.16f, 0.16f, 0.17f), FLinearColor(0.40f, 0.33f, 0.22f), FLinearColor(0.04f, 0.04f, 0.05f) };
		const FLinearColor Cloths[] = {
			FLinearColor(0.42f, 0.10f, 0.08f), FLinearColor(0.10f, 0.14f, 0.28f), FLinearColor(0.55f, 0.50f, 0.38f), FLinearColor(0.15f, 0.25f, 0.15f),
			FLinearColor(0.45f, 0.32f, 0.18f), FLinearColor(0.12f, 0.11f, 0.10f), FLinearColor(0.60f, 0.42f, 0.40f), FLinearColor(0.30f, 0.30f, 0.33f) };
		const FLinearColor Labels[] = {
			FLinearColor(0.65f, 0.12f, 0.08f), FLinearColor(0.75f, 0.62f, 0.25f), FLinearColor(0.12f, 0.25f, 0.50f), FLinearColor(0.70f, 0.68f, 0.60f),
			FLinearColor(0.15f, 0.35f, 0.18f), FLinearColor(0.45f, 0.25f, 0.10f) };
		const FLinearColor Bottles[] = { FLinearColor(0.30f, 0.14f, 0.03f), FLinearColor(0.05f, 0.18f, 0.08f), FLinearColor(0.06f, 0.08f, 0.30f), FLinearColor(0.55f, 0.52f, 0.45f) };
		const FLinearColor Produce[] = {
			FLinearColor(0.55f, 0.08f, 0.05f), FLinearColor(0.40f, 0.30f, 0.15f), FLinearColor(0.30f, 0.45f, 0.18f),
			FLinearColor(0.80f, 0.40f, 0.06f), FLinearColor(0.75f, 0.68f, 0.45f), FLinearColor(0.45f, 0.10f, 0.25f) };
		const FLinearColor Crate(0.48f, 0.36f, 0.20f);
		const FLinearColor Barrel(0.36f, 0.22f, 0.10f);
		const FLinearColor Tin(0.55f, 0.56f, 0.56f);
	}

	/** A repeatable random series, so a shop looks the same every time. */
	struct FDice
	{
		uint32 State;
		explicit FDice(uint32 Seed) : State(Seed * 2654435761u + 977u) {}
		double Unit()
		{
			State = State * 1664525u + 1013904223u;
			return (State >> 8) / 16777216.0;
		}
		double Range(double Min, double Max) { return Min + (Max - Min) * Unit(); }
		int32 Index(int32 Count) { return FMath::Min(Count - 1, static_cast<int32>(Unit() * Count)); }
		bool Chance(double P) { return Unit() < P; }
	};

	/** The trade the furniture script names, e.g. "grocer". */
	inline ETrade TradeOf(const TCHAR* Name)
	{
		static const TCHAR* const Names[] = {
			TEXT("grocer"), TEXT("baker"), TEXT("butcher"), TEXT("shoes"), TEXT("hats"), TEXT("drapery"), TEXT("watches"), TEXT("tobacco"),
			TEXT("barber"), TEXT("tavern"), TEXT("greengrocer"), TEXT("hardware"), TEXT("pharmacy"), TEXT("dairy"), TEXT("stationery"), TEXT("cafe") };
		for (int32 i = 0; i < static_cast<int32>(ETrade::NumTrades); ++i)
		{
			if (FCString::Strcmp(Name, Names[i]) == 0)
			{
				return static_cast<ETrade>(i);
			}
		}
		return ETrade::Grocer;
	}

	/** A box between two corners. */
	inline FPart Span(double X0, double X1, double Y0, double Y1, double Z0, double Z1, const FLinearColor& Color, bool bCollide = false)
	{
		return ParkModel::Box(FVector((X0 + X1) * 0.5, (Y0 + Y1) * 0.5, (Z0 + Z1) * 0.5), FVector(X1 - X0, Y1 - Y0, Z1 - Z0), Color, bCollide);
	}

	/** An upright cylinder standing on Z. */
	inline FPart Can(double X, double Y, double Z, double Across, double Tall, const FLinearColor& Color)
	{
		return { ParkModel::Cylinder, Color, FVector(X, Y, Z + Tall * 0.5), FVector(Across, Across, Tall) };
	}

	/** A ball or loaf resting on Z. */
	inline FPart Lump(double X, double Y, double Z, const FVector& Size, const FLinearColor& Color)
	{
		return { ParkModel::Sphere, Color, FVector(X, Y, Z + Size.Z * 0.5), Size };
	}

	/** Where the door is: its middle along the shop (Y). */
	inline double DoorMiddle(double Width, uint32 Seed)
	{
		FDice Dice(Seed ^ 0x5bd1e995u);
		const double Inner = Width * 0.5 - PilasterWidth;
		const double Side = Inner - DoorWidth * 0.5 - 6.0;
		if (Width > 520.0 && Dice.Chance(0.5))
		{
			return 0.0;
		}
		return Dice.Chance(0.5) ? -Side : Side;
	}

	/** Goods in one shop window between Y0 and Y1, on the display floor. */
	inline void WindowGoods(TArray<FPart>& P, ETrade Trade, double Y0, double Y1, FDice& Dice)
	{
		using namespace Colors;
		const double Floor = SillZ + 4.0;
		const double Wide = Y1 - Y0;
		// A low stepped stand at the back of most windows.
		auto Stand = [&](double Height, const FLinearColor& Color)
		{
			P.Add(Span(3.0, 13.0, Y0 + 4.0, Y1 - 4.0, Floor, Floor + Height, Color));
		};
		auto EachAlong = [&](double Spacing, auto&& Each)
		{
			const int32 Count = FMath::Max(1, static_cast<int32>((Wide - 16.0) / Spacing));
			const double Gap = (Wide - 16.0) / Count;
			for (int32 i = 0; i < Count; ++i)
			{
				Each(Y0 + 8.0 + Gap * (i + 0.5));
			}
		};
		switch (Trade)
		{
		case ETrade::Grocer:
		case ETrade::Tobacco:
		case ETrade::Stationery:
		{
			// Jars, tins and packets in rows on two steps; a pyramid of tins in front.
			Stand(40.0, Shelf);
			Stand(80.0, Shelf);
			for (const double Z : { Floor + 40.0, Floor + 80.0 })
			{
				EachAlong(14.0, [&](double Y)
				{
					const FLinearColor& Color = Labels[Dice.Index(static_cast<int32>(UE_ARRAY_COUNT(Labels)))];
					if (Trade == ETrade::Grocer)
					{
						P.Add(Can(8.0, Y, Z, 10.0, Dice.Range(14.0, 22.0), Color));
					}
					else
					{
						P.Add(Span(5.0, 11.0, Y - 5.0, Y + 5.0, Z, Z + Dice.Range(8.0, 16.0), Color));
					}
				});
			}
			const double Middle = (Y0 + Y1) * 0.5;
			if (Trade == ETrade::Stationery)
			{
				// A school globe on its stand, and stacks of exercise books.
				P.Add(Can(20.0, Middle, Floor, 4.0, 22.0, Gilt));
				P.Add(Lump(20.0, Middle, Floor + 20.0, FVector(30.0, 30.0, 30.0), FLinearColor(0.18f, 0.30f, 0.35f)));
				for (const double Off : { -30.0, 30.0 })
				{
					for (int32 k = 0; k < 5; ++k)
					{
						P.Add(Span(17.0, 25.0, Middle + Off - 11.0, Middle + Off + 11.0, Floor + k * 3.0, Floor + k * 3.0 + 2.6, Labels[Dice.Index(static_cast<int32>(UE_ARRAY_COUNT(Labels)))]));
					}
				}
				break;
			}
			for (int32 Row = 0; Row < 3; ++Row)
			{
				for (int32 k = 0; k < 3 - Row; ++k)
				{
					const double Y = Middle + (k - (2 - Row) * 0.5) * 11.0;
					P.Add(Can(21.0, Y, Floor + Row * 12.0, 10.0, 12.0, Trade == ETrade::Grocer ? Tin : Labels[Dice.Index(static_cast<int32>(UE_ARRAY_COUNT(Labels)))]));
				}
			}
			break;
		}
		case ETrade::Baker:
		{
			// Long loaves and round ones on two tiers, crescents in front.
			Stand(45.0, Shelf);
			EachAlong(30.0, [&](double Y)
			{
				P.Add(Lump(8.0, Y, Floor + 45.0, FVector(14.0, 28.0, 12.0), Dice.Chance(0.5) ? Crust : DarkCrust));
			});
			EachAlong(24.0, [&](double Y)
			{
				P.Add(Lump(20.0, Y, Floor, FVector(20.0, 20.0, 11.0), Dice.Chance(0.6) ? Crust : DarkCrust));
			});
			break;
		}
		case ETrade::Butcher:
		{
			// A marble slab, and sausages and hams hanging from a rail.
			P.Add(Span(3.0, 28.0, Y0 + 2.0, Y1 - 2.0, Floor, Floor + 5.0, Marble));
			P.Add(ParkModel::Bar(FVector(14.0, Y0 + 4.0, 236.0), FVector(14.0, Y1 - 4.0, 236.0), 2.0, Brass));
			EachAlong(16.0, [&](double Y)
			{
				if (Dice.Chance(0.3))
				{
					P.Add(Lump(14.0, Y, 236.0 - 50.0, FVector(18.0, 16.0, 30.0), Meat));
				}
				else
				{
					const double Long = Dice.Range(35.0, 65.0);
					P.Add(Can(14.0, Y, 236.0 - Long, 5.0, Long, Sausage));
				}
			});
			EachAlong(28.0, [&](double Y)
			{
				P.Add(Lump(16.0, Y, Floor + 5.0, FVector(18.0, 22.0, 9.0), Meat));
			});
			break;
		}
		case ETrade::Shoes:
		{
			// Pairs of shoes on three steps.
			for (int32 Tier = 0; Tier < 3; ++Tier)
			{
				const double X = 22.0 - Tier * 7.0;
				const double Z = Floor + Tier * 25.0;
				if (Tier > 0)
				{
					P.Add(Span(X - 4.0, X + 4.0, Y0 + 4.0, Y1 - 4.0, Floor, Z, Shelf));
				}
				EachAlong(30.0, [&](double Y)
				{
					const FLinearColor& Color = Leathers[Dice.Index(static_cast<int32>(UE_ARRAY_COUNT(Leathers)))];
					for (const double Off : { -5.5, 5.5 })
					{
						P.Add(Span(X - 13.0, X + 13.0, Y + Off - 4.5, Y + Off + 4.5, Z, Z + 8.0, Color));
						P.Add(Span(X - 13.0, X - 4.0, Y + Off - 4.5, Y + Off + 4.5, Z + 8.0, Z + 13.0, Color));
					}
				});
			}
			break;
		}
		case ETrade::Hats:
		{
			// Hats on stands of different heights.
			EachAlong(36.0, [&](double Y)
			{
				const double Z = Floor + Dice.Range(30.0, 90.0);
				const double X = Dice.Range(10.0, 20.0);
				const FLinearColor& Felt = Felts[Dice.Index(static_cast<int32>(UE_ARRAY_COUNT(Felts)))];
				P.Add(Can(X, Y, Floor, 2.0, Z - Floor, Brass));
				P.Add(Can(X, Y, Z, 30.0, 2.0, Felt));
				P.Add(Can(X, Y, Z + 2.0, 17.0, 11.0, Felt));
				P.Add(Can(X, Y, Z + 2.0, 17.5, 2.5, Felts[4]));
			});
			break;
		}
		case ETrade::Drapery:
		{
			// Bolts of cloth stacked on their sides and a dress on a stand.
			const double Middle = Dice.Chance(0.5) ? Y0 + 32.0 : Y1 - 32.0;
			P.Add(Can(16.0, Middle, Floor, 3.0, 40.0, Iron));
			P.Add({ ParkModel::Cylinder, Cloths[Dice.Index(static_cast<int32>(UE_ARRAY_COUNT(Cloths)))], FVector(16.0, Middle, Floor + 82.0), FVector(26.0, 34.0, 84.0) });
			P.Add({ ParkModel::Cylinder, P.Last().Color, FVector(16.0, Middle, Floor + 150.0), FVector(22.0, 32.0, 52.0) });
			P.Add(Lump(16.0, Middle, Floor + 176.0, FVector(10.0, 10.0, 12.0), Cream));
			for (int32 Row = 0; Row < 3; ++Row)
			{
				EachAlong(18.0, [&](double Y)
				{
					if (FMath::Abs(Y - Middle) < 30.0)
					{
						return;
					}
					FPart Bolt{ ParkModel::Cylinder, Cloths[Dice.Index(static_cast<int32>(UE_ARRAY_COUNT(Cloths)))], FVector(14.0, Y, Floor + 8.0 + Row * 15.0), FVector(15.0, 15.0, 22.0) };
					Bolt.Pitch = 90.0;
					P.Add(Bolt);
				});
			}
			break;
		}
		case ETrade::Watches:
		{
			// A velvet stand with watches, rings and a mantel clock.
			const FLinearColor Velvet(0.25f, 0.04f, 0.06f);
			Stand(30.0, Velvet);
			Stand(60.0, Velvet);
			P.Add(Span(14.0, 28.0, Y0 + 4.0, Y1 - 4.0, Floor, Floor + 12.0, Velvet));
			for (const double Z : { Floor + 12.0, Floor + 30.0, Floor + 60.0 })
			{
				EachAlong(12.0, [&](double Y)
				{
					FPart Watch{ ParkModel::Cylinder, Dice.Chance(0.5) ? Gilt : White, FVector(Z > Floor + 20.0 ? 8.0 : 20.0, Y, Z + 3.5), FVector(6.0, 6.0, 1.5) };
					Watch.Pitch = 90.0;
					P.Add(Watch);
				});
			}
			const double Middle = (Y0 + Y1) * 0.5;
			P.Add(Span(16.0, 26.0, Middle - 14.0, Middle + 14.0, Floor + 12.0, Floor + 40.0, Shelf));
			FPart Face{ ParkModel::Cylinder, White, FVector(26.5, Middle, Floor + 28.0), FVector(16.0, 16.0, 1.0) };
			Face.Pitch = 90.0;
			P.Add(Face);
			break;
		}
		case ETrade::Pharmacy:
		{
			// Rows of medicine bottles and the great coloured show globes.
			Stand(25.0, Shelf);
			Stand(55.0, Shelf);
			for (const double Z : { Floor + 25.0, Floor + 55.0 })
			{
				EachAlong(12.0, [&](double Y)
				{
					P.Add(Can(8.0, Y, Z, 8.0, Dice.Range(16.0, 24.0), Bottles[Dice.Index(static_cast<int32>(UE_ARRAY_COUNT(Bottles)))]));
				});
			}
			for (const double Y : { Y0 + 28.0, Y1 - 28.0 })
			{
				P.Add(Can(20.0, Y, Floor, 12.0, 30.0, Brass));
				P.Add(Lump(20.0, Y, Floor + 30.0, FVector(32.0, 32.0, 42.0), Y < 0.0 ? FLinearColor(0.10f, 0.30f, 0.12f) : FLinearColor(0.45f, 0.06f, 0.05f)));
			}
			break;
		}
		case ETrade::Dairy:
		{
			// Milk cans, cheese wheels and a row of butter on a marble slab.
			P.Add(Span(3.0, 28.0, Y0 + 2.0, Y1 - 2.0, Floor, Floor + 5.0, Marble));
			EachAlong(26.0, [&](double Y)
			{
				if (Dice.Chance(0.5))
				{
					P.Add(Can(14.0, Y, Floor + 5.0, 20.0, 40.0, Tin));
					P.Add(Can(14.0, Y, Floor + 45.0, 11.0, 8.0, Tin));
				}
				else
				{
					P.Add(Can(14.0, Y, Floor + 5.0, 24.0, 10.0, FLinearColor(0.75f, 0.60f, 0.25f)));
					P.Add(Can(14.0, Y, Floor + 15.0, 20.0, 9.0, FLinearColor(0.70f, 0.55f, 0.22f)));
				}
			});
			break;
		}
		case ETrade::Hardware:
		{
			// Enamel pots, buckets and jugs, and tools on the back board.
			P.Add(Span(1.0, 3.0, Y0 + 4.0, Y1 - 4.0, Floor + 70.0, WindowTop - 70.0, Shelf));
			EachAlong(26.0, [&](double Y)
			{
				const FLinearColor Color = Dice.Chance(0.4) ? Tin : Labels[Dice.Index(static_cast<int32>(UE_ARRAY_COUNT(Labels)))];
				P.Add(Can(15.0, Y, Floor, Dice.Range(16.0, 24.0), Dice.Range(12.0, 26.0), Color));
				P.Add(Span(3.0, 5.0, Y - 2.0, Y + 2.0, Floor + 90.0, Floor + 150.0, Iron));
			});
			break;
		}
		case ETrade::Greengrocer:
		{
			// Crates of fruit and vegetables, tilted to the glass.
			EachAlong(36.0, [&](double Y)
			{
				const FLinearColor& Fruit = Produce[Dice.Index(static_cast<int32>(UE_ARRAY_COUNT(Produce)))];
				P.Add(Span(4.0, 28.0, Y - 16.0, Y + 16.0, Floor, Floor + 16.0, Crate));
				for (int32 k = 0; k < 4; ++k)
				{
					P.Add(Lump(9.0 + (k % 2) * 12.0, Y + (k / 2 - 0.5) * 14.0, Floor + 13.0, FVector(11.0, 11.0, 10.0), Fruit));
				}
			});
			break;
		}
		case ETrade::Barber:
		case ETrade::Tavern:
		case ETrade::Cafe:
		{
			// Curtains on a brass rod over the lower glass; bottles or cups above.
			P.Add(ParkModel::Bar(FVector(25.0, Y0 + 2.0, 186.0), FVector(25.0, Y1 - 2.0, 186.0), 2.0, Brass));
			const int32 Folds = FMath::Max(2, static_cast<int32>(Wide / 14.0));
			for (int32 k = 0; k < Folds; ++k)
			{
				const double Y = Y0 + 4.0 + (Wide - 8.0) * (k + 0.5) / Folds;
				P.Add({ ParkModel::Cylinder, Curtain, FVector(k % 2 ? 23.0 : 24.5, Y, (Floor + 184.0) * 0.5), FVector(4.0, (Wide - 8.0) / Folds + 1.0, 184.0 - Floor) });
			}
			if (Trade != ETrade::Barber)
			{
				P.Add(Span(3.0, 12.0, Y0 + 4.0, Y1 - 4.0, 196.0, 199.0, Shelf));
				EachAlong(13.0, [&](double Y)
				{
					P.Add(Can(7.5, Y, 199.0, 7.0, Trade == ETrade::Tavern ? Dice.Range(22.0, 30.0) : 7.0, Trade == ETrade::Tavern ? Bottles[Dice.Index(static_cast<int32>(UE_ARRAY_COUNT(Bottles)))] : White));
				});
			}
			break;
		}
		default:
			break;
		}
	}

	/** The back of the shop, seen over the window display: shelves of the trade's stock, tiles or a mirror. */
	inline void BackOfShop(TArray<FPart>& P, ETrade Trade, double Y0, double Y1, FDice& Dice)
	{
		using namespace Colors;
		if (Trade == ETrade::Butcher || Trade == ETrade::Dairy)
		{
			// White tiles with a dark band, and hooks or a shelf of jugs.
			P.Add(Span(2.5, 3.0, Y0, Y1, SillZ, TransomZ, White));
			P.Add(Span(3.0, 3.3, Y0, Y1, 200.0, 206.0, Trade == ETrade::Butcher ? Paints[1] : Paints[4]));
			return;
		}
		if (Trade == ETrade::Barber)
		{
			// The big mirror over the basins.
			P.Add(Span(2.5, 3.5, Y0 + 10.0, Y1 - 10.0, 170.0, 250.0, Gilt));
			P.Add(Span(3.5, 4.0, Y0 + 14.0, Y1 - 14.0, 174.0, 246.0, FLinearColor(0.42f, 0.46f, 0.48f)));
			return;
		}
		for (const double Z : { 186.0, 226.0 })
		{
			P.Add(Span(2.5, 8.0, Y0, Y1, Z - 2.0, Z, Shelf));
			for (double Y = Y0 + 3.0; Y < Y1 - 8.0;)
			{
				switch (Trade)
				{
				case ETrade::Grocer:
				case ETrade::Pharmacy:
				case ETrade::Tavern:
				case ETrade::Cafe:
				{
					const FLinearColor& Color = Trade == ETrade::Grocer ? Labels[Dice.Index(static_cast<int32>(UE_ARRAY_COUNT(Labels)))] : Bottles[Dice.Index(static_cast<int32>(UE_ARRAY_COUNT(Bottles)))];
					P.Add(Can(5.0, Y + 3.0, Z, 5.5, Trade == ETrade::Grocer ? Dice.Range(10.0, 16.0) : Dice.Range(18.0, 28.0), Color));
					Y += Dice.Range(6.5, 9.0);
					break;
				}
				case ETrade::Baker:
					P.Add(Lump(5.0, Y + 8.0, Z, FVector(9.0, 16.0, 9.0), Dice.Chance(0.5) ? Crust : DarkCrust));
					Y += 17.0;
					break;
				case ETrade::Shoes:
				{
					// Stacks of shoe boxes, end on.
					const FLinearColor Card = Dice.Chance(0.5) ? FLinearColor(0.60f, 0.52f, 0.38f) : FLinearColor(0.72f, 0.70f, 0.64f);
					P.Add(Span(3.0, 7.5, Y, Y + 15.0, Z, Z + 30.0, Card));
					Y += 16.0;
					break;
				}
				case ETrade::Hats:
					P.Add(Can(5.0, Y + 9.0, Z, 17.0, Dice.Range(14.0, 22.0), Dice.Chance(0.5) ? Cream : FLinearColor(0.30f, 0.20f, 0.14f)));
					Y += 19.0;
					break;
				case ETrade::Drapery:
				{
					// Folded cloth stacked flat.
					const double Wide = Dice.Range(18.0, 26.0);
					for (double H = Z; H < Z + 30.0; H += 6.0)
					{
						P.Add(Span(3.0, 7.5, Y, FMath::Min(Y + Wide, Y1 - 3.0), H, H + 5.5, Cloths[Dice.Index(static_cast<int32>(UE_ARRAY_COUNT(Cloths)))]));
					}
					Y += Wide + 2.0;
					break;
				}
				case ETrade::Hardware:
				case ETrade::Greengrocer:
					P.Add(Can(5.0, Y + 8.0, Z, 15.0, Dice.Range(8.0, 18.0), Trade == ETrade::Hardware ? Tin : Crate));
					Y += 17.0;
					break;
				case ETrade::Watches:
				{
					// Clocks on the shelves.
					FPart Dial{ ParkModel::Cylinder, White, FVector(6.0, Y + 8.0, Z + 9.0), FVector(13.0, 13.0, 1.5) };
					Dial.Pitch = 90.0;
					P.Add(Span(3.0, 5.5, Y, Y + 16.0, Z, Z + 22.0, Shelf));
					P.Add(Dial);
					Y += 20.0;
					break;
				}
				default:
				{
					// Packets and books.
					const double Wide = Dice.Range(5.0, 9.0);
					if (!Dice.Chance(0.2))
					{
						P.Add(Span(3.0, 7.5, Y, FMath::Min(Y + Wide, Y1 - 3.0), Z, Z + Dice.Range(12.0, 30.0), Labels[Dice.Index(static_cast<int32>(UE_ARRAY_COUNT(Labels)))]));
					}
					Y += Wide + 1.0;
					break;
				}
				}
			}
		}
	}

	/** One shop window: frame, glazing bars, the dark shop behind and the goods. */
	inline void Window(TArray<FPart>& P, const FLinearColor& Paint, ETrade Trade, double Y0, double Y1, FDice& Dice)
	{
		using namespace Colors;
		// The shop behind the glass, the display floor and the stallriser under it.
		P.Add(Span(0.5, 2.5, Y0, Y1, SillZ, WindowTop, Inside));
		P.Add(Span(2.5, 30.0, Y0, Y1, SillZ, SillZ + 4.0, Shelf));
		P.Add(Span(0.0, 30.0, Y0, Y1, PlinthTop, SillZ, Paint, true));
		P.Add(Span(30.0, 32.0, Y0 + 8.0, Y1 - 8.0, PlinthTop + 9.0, SillZ - 9.0, Paint));
		P.Add(Span(0.0, 36.0, Y0 - 2.0, Y1 + 2.0, SillZ - 6.0, SillZ, Paint, true));
		// Frame and bars.
		P.Add(Span(26.0, FrontX, Y0, Y0 + 7.0, SillZ, WindowTop, Paint));
		P.Add(Span(26.0, FrontX, Y1 - 7.0, Y1, SillZ, WindowTop, Paint));
		P.Add(Span(26.0, FrontX, Y0, Y1, TransomZ, TransomZ + 7.0, Paint));
		// Big panes below the transom; the light above is left whole for the lettering.
		const int32 Lights = FMath::Max(1, FMath::RoundToInt((Y1 - Y0) / 85.0));
		for (int32 k = 1; k < Lights; ++k)
		{
			const double Y = Y0 + (Y1 - Y0) * k / Lights;
			P.Add(Span(28.0, 32.0, Y - 2.0, Y + 2.0, SillZ, TransomZ, Paint));
		}
		BackOfShop(P, Trade, Y0, Y1, Dice);
		WindowGoods(P, Trade, Y0 + 7.0, Y1 - 7.0, Dice);
	}

	/** The recessed glazed door with its transom light and the stone step. */
	inline void Door(TArray<FPart>& P, const FLinearColor& Paint, double Middle)
	{
		using namespace Colors;
		const double Y0 = Middle - DoorWidth * 0.5;
		const double Y1 = Middle + DoorWidth * 0.5;
		P.Add(Span(0.0, 44.0, Y0 - 4.0, Y1 + 4.0, 0.0, PlinthTop, Step, true));
		// Reveal at the sides and the leaf set back in it.
		P.Add(Span(14.0, FrontX, Y0, Y0 + 9.0, PlinthTop, WindowTop, Paint, true));
		P.Add(Span(14.0, FrontX, Y1 - 9.0, Y1, PlinthTop, WindowTop, Paint, true));
		P.Add(Span(14.0, FrontX, Y0, Y1, TransomZ, TransomZ + 8.0, Paint));
		P.Add(Span(0.5, 2.5, Y0, Y1, TransomZ, WindowTop, Inside));
		P.Add(Span(28.0, 31.0, Middle - 1.5, Middle + 1.5, TransomZ + 8.0, WindowTop, Paint));
		P.Add(Span(12.0, 16.0, Y0 + 9.0, Y1 - 9.0, PlinthTop, TransomZ, Paint, true));
		P.Add(Span(16.0, 18.0, Y0 + 17.0, Y1 - 17.0, PlinthTop + 10.0, 95.0, Paint));
		P.Add(Span(16.0, 16.8, Y0 + 17.0, Y1 - 17.0, 108.0, TransomZ - 10.0, Inside));
		P.Add(Span(16.0, 17.5, Middle - 1.0, Middle + 1.0, 108.0, TransomZ - 10.0, Paint));
		P.Add({ ParkModel::Sphere, Brass, FVector(19.0, Y1 - 22.0, 105.0), FVector(5.0, 5.0, 5.0) });
		P.Add(Span(16.0, 17.0, Y0 + 30.0, Y1 - 30.0, 98.0, 103.0, Brass));
	}

	/** A striped canvas awning on its roller over the whole shop front. */
	inline void Awning(TArray<FPart>& P, double Width, const FLinearColor& Stripe, const FLinearColor& Paint)
	{
		using namespace Colors;
		constexpr double Slope = 24.0;
		constexpr double Reach = 175.0;
		constexpr double Valance = 26.0;
		const double TopX = FrontX + 8.0;
		const double TopZ = FasciaBottom - 4.0;
		const double Rad = FMath::DegreesToRadians(Slope);
		const double EndX = TopX + Reach * FMath::Cos(Rad);
		const double EndZ = TopZ - Reach * FMath::Sin(Rad);
		FPart Roller{ ParkModel::Cylinder, Paint, FVector(TopX, 0.0, TopZ + 2.0), FVector(12.0, 12.0, Width - 30.0) };
		Roller.Roll = 90.0;
		P.Add(Roller);
		const double Half = (Width - 36.0) * 0.5;
		const int32 Count = FMath::Max(2, FMath::RoundToInt(Half * 2.0 / 26.0));
		const double StripeWidth = Half * 2.0 / Count;
		for (int32 k = 0; k < Count; ++k)
		{
			const double Y = -Half + StripeWidth * (k + 0.5);
			const FLinearColor& Color = k % 2 ? Cream : Stripe;
			FPart Canvas{ ParkModel::Cube, Color, FVector((TopX + EndX) * 0.5, Y, (TopZ + EndZ) * 0.5), FVector(Reach, StripeWidth + 0.2, 1.5) };
			Canvas.Pitch = -Slope;
			P.Add(Canvas);
			P.Add(Span(EndX - 0.75, EndX + 0.75, Y - StripeWidth * 0.5 - 0.1, Y + StripeWidth * 0.5 + 0.1, EndZ - Valance, EndZ, Color));
		}
		// The iron arms that hold it out.
		for (const double Side : { -1.0, 1.0 })
		{
			P.Add(ParkModel::Bar(FVector(2.0, Side * (Half + 1.0), EndZ - 60.0), FVector(EndX, Side * (Half + 1.0), EndZ), 2.5, Iron));
		}
	}

	/**
	 * Moves parts from First on out by StandOff, so the front stands clear of
	 * the facade drawn on the wall behind; parts that start at the wall are
	 * made deeper instead, so they still meet it.
	 */
	inline void StandOut(TArray<FPart>& P, int32 First, TArray<FLettering>& Text, int32 FirstText)
	{
		for (int32 i = First; i < P.Num(); ++i)
		{
			FPart& Part = P[i];
			const bool bFlat = Part.Pitch == 0.0 && Part.Roll == 0.0 && Part.Shape == ParkModel::Cube;
			if (bFlat && FMath::Abs(Part.At.X - Part.Size.X * 0.5) < 0.01)
			{
				Part.Size.X += StandOff;
				Part.At.X += StandOff * 0.5;
			}
			else
			{
				Part.At.X += StandOff;
			}
		}
		for (int32 i = FirstText; i < Text.Num(); ++i)
		{
			Text[i].At.X += StandOff;
		}
	}

	/**
	 * One shop front Width cm wide, drawn against the wall (BuildShop then
	 * stands it off). bClosed: the iron shutter is down. bOutside: a crate or two, barrels or buckets set out
	 * on the pavement.
	 */
	inline void ShopAtWall(TArray<FPart>& P, TArray<FLettering>& Text, double Width, ETrade Trade, uint32 Seed, bool bClosed, bool bOutside)
	{
		using namespace Colors;
		FDice Dice(Seed);
		FLinearColor Paint = Paints[Dice.Index(static_cast<int32>(UE_ARRAY_COUNT(Paints)))];
		if (Trade == ETrade::Dairy)
		{
			Paint = Cream;
		}
		else if (Trade == ETrade::Butcher && Dice.Chance(0.6))
		{
			Paint = Paints[1];
		}
		else if (Trade == ETrade::Pharmacy)
		{
			Paint = Paints[2];
		}
		const FLinearColor& Letters = Paint == Cream ? Paints[0] : Gilt;
		const double Half = Width * 0.5;
		const double Inner = Half - PilasterWidth;

		// Plinth, pilasters with base and capital, fascia, cornice.
		P.Add(Span(0.0, 38.0, -Half, Half, 0.0, PlinthTop, Paints[3], true));
		for (const double Side : { -1.0, 1.0 })
		{
			const double Y0 = Side < 0.0 ? -Half : Inner;
			const double Y1 = Side < 0.0 ? -Inner : Half;
			P.Add(Span(0.0, 40.0, Y0 - 2.0, Y1 + 2.0, 0.0, 30.0, Paint, true));
			P.Add(Span(0.0, 36.0, Y0, Y1, 30.0, FasciaBottom, Paint, true));
			P.Add(Span(36.0, 38.0, Y0 + 7.0, Y1 - 7.0, 50.0, FasciaBottom - 30.0, Paint));
			P.Add(Span(0.0, 41.0, Y0 - 3.0, Y1 + 3.0, FasciaBottom - 16.0, FasciaBottom, Paint));
		}
		P.Add(Span(0.0, FrontX, -Half - 4.0, Half + 4.0, FasciaBottom, FasciaTop, Paint, true));
		P.Add(Span(FrontX, FrontX + 1.0, -Inner + 8.0, Inner - 8.0, FasciaBottom + 8.0, FasciaBottom + 10.0, Gilt));
		P.Add(Span(FrontX, FrontX + 1.0, -Inner + 8.0, Inner - 8.0, FasciaTop - 10.0, FasciaTop - 8.0, Gilt));
		P.Add(Span(0.0, 40.0, -Half - 7.0, Half + 7.0, FasciaTop, FasciaTop + 9.0, Paint));
		P.Add(Span(0.0, 45.0, -Half - 10.0, Half + 10.0, FasciaTop + 9.0, CorniceTop, Paint));
		Text.Add({ ELine::Fascia, FVector(FrontX + 1.5, 0.0, (FasciaBottom + FasciaTop) * 0.5), 0.0, 30.0, Inner * 2.0 - 24.0, Letters });

		// Windows either side of the door.
		const double DoorAt = DoorMiddle(Width, Seed);
		const double DoorY0 = DoorAt - DoorWidth * 0.5;
		const double DoorY1 = DoorAt + DoorWidth * 0.5;
		if (bClosed)
		{
			// The corrugated iron shutter rolled down over windows and door.
			for (double Z = PlinthTop; Z < WindowTop; Z += 8.0)
			{
				P.Add(Span(FrontX - 1.0, FrontX + 0.5, -Inner, Inner, Z, FMath::Min(Z + 8.0, WindowTop), static_cast<int32>(Z / 8.0) % 2 ? Shutter : ShutterDark, true));
			}
			P.Add(Span(FrontX + 0.5, FrontX + 3.0, -Inner, Inner, PlinthTop, PlinthTop + 6.0, Iron));
			P.Add({ ParkModel::Cube, Brass, FVector(FrontX + 4.0, DoorAt, PlinthTop + 4.0), FVector(4.0, 7.0, 9.0) });
		}
		else
		{
			if (DoorY0 - -Inner > 40.0)
			{
				Window(P, Paint, Trade, -Inner, DoorY0, Dice);
			}
			if (Inner - DoorY1 > 40.0)
			{
				Window(P, Paint, Trade, DoorY1, Inner, Dice);
			}
			Door(P, Paint, DoorAt);
			// More gold lettering on the glass over the widest window.
			const double Left = DoorY0 - -Inner;
			const double Right = Inner - DoorY1;
			const double Y = Left > Right ? (-Inner + DoorY0) * 0.5 : (DoorY1 + Inner) * 0.5;
			Text.Add({ ELine::Glass, FVector(31.5, Y, (TransomZ + 7.0 + WindowTop) * 0.5), 0.0, 17.0, FMath::Max(Left, Right) - 24.0, Letters });
		}
		// Short pieces of window or wall between the door and pilasters too narrow for a window.
		if (DoorY0 - -Inner <= 40.0)
		{
			P.Add(Span(0.0, FrontX, -Inner, DoorY0, PlinthTop, WindowTop, Paint, true));
		}
		if (Inner - DoorY1 <= 40.0)
		{
			P.Add(Span(0.0, FrontX, DoorY1, Inner, PlinthTop, WindowTop, Paint, true));
		}
		// The top rail over the windows, under the fascia.
		P.Add(Span(0.0, FrontX, -Inner, Inner, WindowTop, FasciaBottom, Paint, true));

		if (bClosed)
		{
			return;
		}
		const bool bAwning = Trade == ETrade::Greengrocer || ((Trade == ETrade::Grocer || Trade == ETrade::Baker || Trade == ETrade::Drapery
			|| Trade == ETrade::Butcher || Trade == ETrade::Dairy || Trade == ETrade::Cafe) && Dice.Chance(0.55)) || Dice.Chance(0.12);
		if (bAwning)
		{
			Awning(P, Width, Stripes[Dice.Index(static_cast<int32>(UE_ARRAY_COUNT(Stripes)))], Paint);
		}

		// A sign hung out from the wall at one end, seen along the street.
		const double SignY = DoorAt > 0.0 ? -Half - 14.0 : Half + 14.0;
		auto Bracket = [&](double Reach)
		{
			P.Add(ParkModel::Bar(FVector(0.0, SignY, FasciaTop + 30.0), FVector(Reach, SignY, FasciaTop + 30.0), 3.0, Iron));
			P.Add(ParkModel::Bar(FVector(0.0, SignY, FasciaTop - 5.0), FVector(Reach * 0.6, SignY, FasciaTop + 30.0), 2.0, Iron));
		};
		if (Trade == ETrade::Barber)
		{
			// The barber's brass basin.
			Bracket(80.0);
			P.Add(ParkModel::Bar(FVector(65.0, SignY, FasciaTop + 30.0), FVector(65.0, SignY, FasciaTop + 8.0), 1.0, Iron));
			FPart Basin{ ParkModel::Cylinder, Brass, FVector(65.0, SignY, FasciaTop - 15.0), FVector(48.0, 48.0, 3.0) };
			Basin.Roll = 90.0;
			P.Add(Basin);
			FPart Boss = Basin;
			Boss.Size = FVector(22.0, 22.0, 8.0);
			P.Add(Boss);
		}
		else if (Trade == ETrade::Watches)
		{
			// The big double-faced clock.
			Bracket(85.0);
			FPart Rim{ ParkModel::Cylinder, Paints[3], FVector(62.0, SignY, FasciaTop - 18.0), FVector(56.0, 56.0, 12.0) };
			Rim.Roll = 90.0;
			P.Add(Rim);
			for (const double Face : { -1.0, 1.0 })
			{
				FPart Dial{ ParkModel::Cylinder, White, FVector(62.0, SignY + Face * 6.2, FasciaTop - 18.0), FVector(48.0, 48.0, 0.6) };
				Dial.Roll = 90.0;
				P.Add(Dial);
				P.Add(Span(61.0, 63.0, SignY + Face * 6.6 - 0.3, SignY + Face * 6.6 + 0.3, FasciaTop - 18.0, FasciaTop + 0.0, Paints[3]));
				P.Add(Span(62.0, 76.0, SignY + Face * 6.6 - 0.3, SignY + Face * 6.6 + 0.3, FasciaTop - 19.0, FasciaTop - 17.0, Paints[3]));
			}
		}
		else if (Trade == ETrade::Tobacco || Trade == ETrade::Tavern || Trade == ETrade::Pharmacy || Trade == ETrade::Cafe || Dice.Chance(0.15))
		{
			// A hanging board, lettered on both faces.
			Bracket(95.0);
			const FLinearColor Board = Trade == ETrade::Tobacco ? FLinearColor(0.55f, 0.10f, 0.06f) : Paint;
			for (const double X : { 30.0, 80.0 })
			{
				P.Add(ParkModel::Bar(FVector(X, SignY, FasciaTop + 30.0), FVector(X, SignY, FasciaTop + 18.0), 1.0, Iron));
			}
			P.Add(Span(22.0, 92.0, SignY - 2.5, SignY + 2.5, FasciaTop - 26.0, FasciaTop + 18.0, Board));
			P.Add(Span(21.0, 93.0, SignY - 3.0, SignY + 3.0, FasciaTop + 16.0, FasciaTop + 19.0, Gilt));
			P.Add(Span(21.0, 93.0, SignY - 3.0, SignY + 3.0, FasciaTop - 27.0, FasciaTop - 24.0, Gilt));
			for (const double Face : { -1.0, 1.0 })
			{
				Text.Add({ ELine::Blade, FVector(57.0, SignY + Face * 3.2, FasciaTop - 4.0), Face * 90.0, 13.0, 62.0, Trade == ETrade::Tobacco ? White : Letters });
			}
		}

		// Goods set out on the pavement by the door.
		if (!bOutside)
		{
			return;
		}
		const double Side = DoorAt > 0.0 ? -1.0 : 1.0;
		const double Y = DoorAt + Side * (DoorWidth * 0.5 + 45.0);
		switch (Trade)
		{
		case ETrade::Greengrocer:
		case ETrade::Grocer:
		{
			// A trestle table with tilted crates.
			const double Span0 = FMath::Max(-Half + 20.0, Y - 90.0);
			const double Span1 = FMath::Min(Half - 20.0, Y + 90.0);
			if (Span1 - Span0 < 80.0)
			{
				break;
			}
			for (const double Leg : { Span0 + 6.0, Span1 - 6.0 })
			{
				P.Add(Span(50.0, 54.0, Leg - 2.0, Leg + 2.0, 0.0, 68.0, Crate, true));
				P.Add(Span(96.0, 100.0, Leg - 2.0, Leg + 2.0, 0.0, 58.0, Crate, true));
			}
			for (double C = Span0 + 4.0; C + 40.0 <= Span1; C += 42.0)
			{
				const FLinearColor& Fruit = Produce[Dice.Index(static_cast<int32>(UE_ARRAY_COUNT(Produce)))];
				FPart Tray{ ParkModel::Cube, Crate, FVector(75.0, C + 20.0, 70.0), FVector(54.0, 40.0, 14.0) };
				Tray.Pitch = 10.0;
				P.Add(Tray);
				for (int32 k = 0; k < 6; ++k)
				{
					P.Add(Lump(62.0 + (k % 3) * 13.0, C + 10.0 + (k / 3) * 20.0, 75.0 - (k % 3) * 2.3, FVector(12.0, 12.0, 11.0), Fruit));
				}
			}
			break;
		}
		case ETrade::Tavern:
		{
			// Barrels by the door.
			for (int32 k = 0; k < 2; ++k)
			{
				const double BY = Y + Side * k * 58.0;
				if (FMath::Abs(BY) > Half - 25.0)
				{
					continue;
				}
				P.Add(Can(62.0, BY, 0.0, 55.0, 80.0, Barrel));
				P.Add(Can(62.0, BY, 12.0, 57.0, 4.0, Iron));
				P.Add(Can(62.0, BY, 64.0, 57.0, 4.0, Iron));
			}
			break;
		}
		case ETrade::Hardware:
		{
			// Buckets and tubs stacked by the door, a broom against the wall.
			P.Add(Can(60.0, Y, 0.0, 34.0, 30.0, Tin));
			P.Add(Can(60.0, Y, 30.0, 30.0, 28.0, Tin));
			P.Add(Can(60.0, Y + Side * 42.0, 0.0, 44.0, 22.0, Tin));
			P.Add(ParkModel::Bar(FVector(40.0, Y + Side * 80.0, 0.0), FVector(37.0, Y + Side * 86.0, 150.0), 3.0, Shelf));
			break;
		}
		case ETrade::Baker:
		case ETrade::Dairy:
		{
			// A wicker basket on a stool.
			P.Add(Span(58.0, 92.0, Y - 17.0, Y + 17.0, 0.0, 45.0, Shelf, true));
			P.Add(Can(75.0, Y, 45.0, 44.0, 20.0, FLinearColor(0.52f, 0.38f, 0.18f)));
			for (int32 k = 0; k < 4; ++k)
			{
				P.Add(Lump(68.0 + (k % 2) * 14.0, Y - 7.0 + (k / 2) * 14.0, 60.0, FVector(16.0, 16.0, 9.0), Trade == ETrade::Baker ? Crust : Colors::White));
			}
			break;
		}
		default:
			break;
		}
	}

	/** One shop front Width cm wide: its parts go into P and its lettering into Text (see ShopAtWall). */
	inline void BuildShop(TArray<FPart>& P, TArray<FLettering>& Text, double Width, ETrade Trade, uint32 Seed, bool bClosed, bool bOutside)
	{
		const int32 FirstPart = P.Num();
		const int32 FirstText = Text.Num();
		ShopAtWall(P, Text, Width, Trade, Seed, bClosed, bOutside);
		StandOut(P, FirstPart, Text, FirstText);
	}

	/** A hat shop keeper in the best of the stock: a fedora or bowler for a man, a cloche for a woman; Pick in [0, 1) chooses. */
	inline void DressHatter(PersonKit::FOutfit& Outfit, double Pick)
	{
		static const FLinearColor HatFelts[] = {
			FLinearColor(0.30f, 0.27f, 0.24f), FLinearColor(0.22f, 0.13f, 0.07f), FLinearColor(0.05f, 0.05f, 0.06f), FLinearColor(0.38f, 0.30f, 0.20f) };
		static const FLinearColor HatBands[] = {
			FLinearColor(0.03f, 0.03f, 0.03f), FLinearColor(0.35f, 0.05f, 0.04f), FLinearColor(0.55f, 0.50f, 0.40f), FLinearColor(0.05f, 0.08f, 0.20f) };
		const int32 Count = static_cast<int32>(UE_ARRAY_COUNT(HatFelts));
		const int32 Which = FMath::Min(Count - 1, static_cast<int32>(Pick * Count));
		Outfit.bApron = false;
		Outfit.Hat = Outfit.bWoman ? PersonKit::EHat::Cloche : (Pick < 0.6 ? PersonKit::EHat::Fedora : PersonKit::EHat::Bowler);
		Outfit.HatColor = Outfit.bWoman ? Colors::Cloths[(Which * 2) % static_cast<int32>(UE_ARRAY_COUNT(Colors::Cloths))] : HatFelts[Which];
		Outfit.HatBand = HatBands[Which];
	}

	/** Width of the stone surround of a tenement gate (cm, along the wall). */
	constexpr double GateSurround = 34.0;

	/** A tenement gate Width cm wide: stone surround, panelled double doors, fanlight and the blue number plate. */
	inline void BuildGate(TArray<FPart>& P, TArray<FLettering>& Text, double Width)
	{
		using namespace Colors;
		const int32 FirstPart = P.Num();
		const int32 FirstText = Text.Num();
		const double Half = Width * 0.5;
		const double Open = Half - GateSurround;
		constexpr double Spring = 340.0;
		constexpr double Top = 418.0;
		for (const double Side : { -1.0, 1.0 })
		{
			const double Y0 = Side < 0.0 ? -Half : Open;
			const double Y1 = Side < 0.0 ? -Open : Half;
			P.Add(Span(0.0, 26.0, Y0, Y1, 0.0, 40.0, StoneDark, true));
			P.Add(Span(0.0, 22.0, Y0, Y1, 40.0, Spring, Stone, true));
			P.Add(Span(0.0, 26.0, Y0 - 2.0, Y1 + 2.0, Spring - 12.0, Spring, Stone));
			// Wheel guards at the foot of each jamb.
			P.Add(Can(30.0, Side * (Open - 8.0), 0.0, 16.0, 50.0, StoneDark));
		}
		P.Add(Span(0.0, 22.0, -Half, Half, Spring, Top, Stone, true));
		P.Add(Span(0.0, 30.0, -18.0, 18.0, Spring - 6.0, Top - 4.0, StoneDark));
		// Fanlight with bars, and the doors set back in the opening.
		P.Add(Span(0.5, 2.5, -Open, Open, Spring - 70.0, Spring, Inside));
		for (int32 k = 1; k < 4; ++k)
		{
			const double Y = -Open + Open * 2.0 * k / 4.0;
			P.Add(Span(6.0, 9.0, Y - 1.5, Y + 1.5, Spring - 70.0, Spring, GateWood));
		}
		P.Add(Span(6.0, 10.0, -Open, Open, Spring - 78.0, Spring - 70.0, GateWood));
		for (const double Side : { -1.0, 1.0 })
		{
			const double Y0 = Side < 0.0 ? -Open : 0.5;
			const double Y1 = Side < 0.0 ? -0.5 : Open;
			P.Add(Span(4.0, 10.0, Y0, Y1, 0.0, Spring - 78.0, GateWood, true));
			for (const double Z0 : { 25.0, 130.0 })
			{
				P.Add(Span(10.0, 13.0, Y0 + 14.0, Y1 - 14.0, Z0, Z0 + (Z0 < 100.0 ? 85.0 : 115.0), GateWood));
			}
			P.Add({ ParkModel::Sphere, Brass, FVector(14.0, Side * 9.0, 120.0), FVector(6.0, 6.0, 6.0) });
		}
		// The blue enamel house number plate on the right jamb.
		P.Add(Span(22.0, 23.5, Open + 3.0, Half - 3.0, 252.0, 276.0, White));
		P.Add(Span(23.5, 24.0, Open + 4.5, Half - 4.5, 253.5, 274.5, Enamel));
		Text.Add({ ELine::Number, FVector(24.6, (Open + Half) * 0.5, 264.0), 0.0, 15.0, GateSurround - 8.0, White });
		StandOut(P, FirstPart, Text, FirstText);
	}
}
