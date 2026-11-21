// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HandCarts.generated.h"

class AHandCart;

/**
 * Now and then, rarely, by day and when it is not raining, sends a porter
 * pushing his hand cart (AHandCart) along the side of a street some way from
 * the player, out of his sight; one at a time. The cart goes when it is far
 * off and unseen, and the next one comes a few minutes later.
 */
UCLASS()
class PESTIALVILAG_API AHandCarts : public AActor
{
	GENERATED_BODY()

public:
	AHandCarts();

	virtual void Tick(float DeltaSeconds) override;

	/** Sends one onto the street near Near now (the test spawn menu); false if no street was found. */
	static bool SpawnNear(UWorld* World, const FVector& Near, bool bInView);

protected:
	UPROPERTY(Transient)
	TArray<TObjectPtr<AHandCart>> Out;

private:
	/** Seconds till the next one may come. */
	float Wait = 0.f;
};
