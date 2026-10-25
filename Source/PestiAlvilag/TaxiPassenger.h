// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TaxiPassenger.generated.h"

class UMaterialInterface;
class UProceduralMeshComponent;
class UStaticMeshComponent;

/**
 * Someone who wants a taxi: stands at the kerb waving, with a tall yellow
 * light above them so they can be found from a distance.
 */
UCLASS()
class PESTIALVILAG_API ATaxiPassenger : public AActor
{
	GENERATED_BODY()

public:
	ATaxiPassenger();

	virtual void Tick(float DeltaSeconds) override;

	/** Waving with the light on, or just standing. */
	void SetWaiting(bool bWaiting);
	/** Hides them while they ride in the taxi. */
	void SetInWorld(bool bInWorld);

protected:
	/** Legs, arms and body (PersonKit::EPiece order), dressed when play begins. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UProceduralMeshComponent>> Pieces;

	UPROPERTY(VisibleAnywhere, Category = "Passenger")
	TObjectPtr<UStaticMeshComponent> Beacon;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> BaseMaterial;

	virtual void BeginPlay() override;

private:
	bool bIsWaiting = true;
	float WavePhase = 0.f;
};
