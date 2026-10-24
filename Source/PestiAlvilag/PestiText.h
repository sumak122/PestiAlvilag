// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/** Small helpers for the on-screen text. */
namespace PestiText
{
	/** Which way to go, as a compass word (X is east, Y is south). */
	inline const TCHAR* Compass(const FVector& From, const FVector& To)
	{
		static const TCHAR* const Words[] = {
			TEXT("north"), TEXT("north-east"), TEXT("east"), TEXT("south-east"),
			TEXT("south"), TEXT("south-west"), TEXT("west"), TEXT("north-west") };
		const FVector Delta = To - From;
		const double Bearing = FMath::RadiansToDegrees(FMath::Atan2(Delta.X, -Delta.Y));
		const int32 Sector = static_cast<int32>(FMath::RoundToDouble(FRotator::ClampAxis(Bearing) / 45.0)) % 8;
		return Words[Sector];
	}

	/** A distance in metres, rounded to tens. */
	inline FString Metres(double Centimetres)
	{
		return FString::Printf(TEXT("%d m"), FMath::RoundToInt(Centimetres / 100.0 / 10.0) * 10);
	}
}
