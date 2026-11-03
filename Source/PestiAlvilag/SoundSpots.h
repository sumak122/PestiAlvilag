// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CitySynth.h"
#include "SoundSpots.generated.h"

class USpotSound;

/**
 * Sounds placed about Józsefváros: the church bell in its tower, the engines
 * at Keleti, the crowds of the Teleki market and the market hall, the pigeons
 * on the squares, sparrows and crickets in the Tisza Kálmán tér park, a
 * gramophone at the café, and in the courtyards dogs, carpets being beaten,
 * smiths and gramophones at open windows.
 *
 * Each is a sound spot with its own reach and waking hours. Only the few the
 * listener is nearest to are heard, played by a small set of USpotSound
 * placed at the spots, so they grow louder walking up and fade walking away.
 */
UCLASS()
class PESTIALVILAG_API ASoundSpots : public AActor
{
	GENERATED_BODY()

public:
	ASoundSpots();

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

private:
	struct FSpot
	{
		CitySynth::ESpot Kind = CitySynth::ESpot::Dog;
		FVector2D At = FVector2D::ZeroVector;
		/** How high the sound comes from, above the street (cm). */
		double Height = 150.0;
		/** How far it is heard (cm). */
		double Reach = 3000.0;
		/** How loud beside it, 0..1. */
		float Loud = 1.f;
	};

	/** A playing sound: which spot it is at, and how long it still fades out before it is free again. */
	struct FSlot
	{
		int32 Spot = INDEX_NONE;
		float Fading = 0.f;
	};

	void AddSpot(CitySynth::ESpot Kind, const FVector2D& At, double Reach, float Loud, double Height = 150.0);
	static bool IsAwake(CitySynth::ESpot Kind, float Hour, float Darkness);

	UPROPERTY()
	TArray<TObjectPtr<USpotSound>> Players;

	TArray<FSpot> Spots;
	TArray<FSlot> Slots;
	int32 NextGeneration = 1;
};
