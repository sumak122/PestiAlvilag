// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LorryParking.generated.h"

class ALorry;
class ATrafficManager;

/**
 * Keeps a couple of carriers' Rába lorries (ALorry), loaded, parked by
 * the kerb of the streets round the player, out of his sight. One he takes
 * is his; a new one is parked somewhere else later.
 */
UCLASS()
class PESTIALVILAG_API ALorryParking : public AActor
{
	GENERATED_BODY()

public:
	ALorryParking();

	virtual void Tick(float DeltaSeconds) override;

protected:
	UPROPERTY(Transient)
	TArray<TObjectPtr<ALorry>> Parked;

private:
	/** Parks one more at a kerb some way from Eye, out of its sight; false if no spot was found this time. */
	bool ParkOne(ATrafficManager* Traffic, const FVector& Eye);

	TWeakObjectPtr<ATrafficManager> TrafficRef;
};
