// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PersonKit.h"

/**
 * The city street sweeper (AKnockables sends him to tidy up things knocked
 * over): his clothes, a grey-blue work jacket, cap and canvas apron, and his
 * long birch twig broom. The broom is built round the grip (where his hands
 * hold it), the handle up along +Z, the twigs down along -Z.
 */
namespace SweeperModel
{
	/** Below the grip to where the twigs start, and how long they are (cm). */
	constexpr double HandleBelow = 45.0;
	constexpr double HandleAbove = 65.0;
	constexpr double TwigLength = 55.0;
	constexpr double TwigSpread = 16.0;
	constexpr int32 Twigs = 9;

	inline PersonKit::FOutfit Outfit(FRandomStream& Random)
	{
		PersonKit::FOutfit O;
		O.Height = Random.FRandRange(166.f, 180.f);
		O.Coat = FLinearColor(0.18f, 0.22f, 0.27f);
		O.Trousers = FLinearColor(0.10f, 0.11f, 0.13f);
		O.Top = PersonKit::ETop::Jacket;
		O.Hat = PersonKit::EHat::FlatCap;
		O.HatColor = FLinearColor(0.14f, 0.16f, 0.19f);
		O.bTie = false;
		O.bApron = true;
		O.Apron = FLinearColor(0.42f, 0.38f, 0.30f);
		O.bMoustache = Random.FRand() < 0.6f;
		O.bAged = Random.FRand() < 0.4f;
		return O;
	}

	inline void Broom(PersonKit::FPiece& Out)
	{
		const FLinearColor Handle(0.42f, 0.30f, 0.17f);
		const FLinearColor Twig(0.33f, 0.24f, 0.13f);
		const FLinearColor Band(0.16f, 0.14f, 0.12f);
		PersonKit::Rod(Out, Handle, FVector(0.0, 0.0, HandleAbove), FVector(0.0, 0.0, -HandleBelow - 10.0), 1.6);
		// The twigs bound round the end of the handle, fanning out to the tips.
		PersonKit::Block(Out, Band, FVector(0.0, 0.0, -HandleBelow - 4.0), FVector(3.5, 3.5, 3.0));
		for (int32 i = 0; i < Twigs; ++i)
		{
			const double Angle = 2.0 * PI * i / Twigs;
			const double Ring = (i % 3 == 0) ? 0.4 : 1.0;
			const FVector Root(2.5 * FMath::Cos(Angle), 2.5 * FMath::Sin(Angle), -HandleBelow - 2.0);
			const FVector Tip(TwigSpread * Ring * FMath::Cos(Angle), TwigSpread * 0.6 * Ring * FMath::Sin(Angle), -HandleBelow - TwigLength);
			PersonKit::Rod(Out, Twig, Root, Tip, 1.4, 4);
		}
	}
}
