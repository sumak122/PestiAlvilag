// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FiakerStand.generated.h"

class ATrafficCart;
class ATrafficManager;

/**
 * The cab stand in front of Keleti: two fiakers (ATrafficCart::MakeFiaker)
 * waiting for fares beside the taxis. T beside one hires it (ATaxiRides), F
 * pulls the cabman off and takes it. A taken one is replaced while the
 * player is away.
 */
UCLASS()
class PESTIALVILAG_API AFiakerStand : public AActor
{
	GENERATED_BODY()

public:
	AFiakerStand();

	virtual void Tick(float DeltaSeconds) override;

protected:
	/** The cab waiting on each spot, or null. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<ATrafficCart>> Waiting;

private:
	/** Puts a fresh fiaker on spot Index; false if the ground is not there yet. */
	bool PutCab(int32 Index, ATrafficManager* Traffic);

	TWeakObjectPtr<ATrafficManager> TrafficRef;
};
