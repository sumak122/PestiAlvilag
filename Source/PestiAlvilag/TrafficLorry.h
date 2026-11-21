// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TrafficCar.h"
#include "LorryModel.h"
#include "TrafficLorry.generated.h"

class UProceduralMeshComponent;
class USpotSound;

/**
 * A Rába lorry in the traffic (LorryModel): an ATrafficCar drawn with the
 * lorry's looks, slower to pull away and keeping a longer gap. Its paint
 * and load are picked when it is spawned; taken by the player it becomes
 * an ALorry with the same looks (ATrafficManager::StealCar).
 */
UCLASS()
class PESTIALVILAG_API ATrafficLorry : public ATrafficCar
{
	GENERATED_BODY()

public:
	ATrafficLorry();

	virtual void Tick(float DeltaSeconds) override;

	int32 GetPaintIndex() const { return PaintIndex; }
	LorryModel::ELoad GetLoad() const { return Load; }
	/** Makes it one of the Royal Post's vans. Right after spawning. */
	void MakePost() { Load = LorryModel::ELoad::Post; }
	/** Makes it the ambulance society's ambulance, now and then rushing with its bell ringing. Right after spawning. */
	void MakeAmbulance() { Load = LorryModel::ELoad::Ambulance; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UProceduralMeshComponent>> Parts;

	/** The ambulance's bell, made the first time it rings. */
	UPROPERTY(Transient)
	TObjectPtr<USpotSound> Bell;

private:
	/** Swaps the car's looks for the lorry's, once it is on the road. */
	void Dress();
	/** The ambulance's bell: rings for a spell now and then, heard round it. */
	void TickBell(float DeltaSeconds);

	int32 PaintIndex = 0;
	LorryModel::ELoad Load = LorryModel::ELoad::Crates;
	bool bDressed = false;
	float WheelTurn = 0.f;
	float SteerAngle = 0.f;
	float LastYaw = 0.f;
	bool bRinging = false;
	/** Seconds till the bell starts or stops. */
	float BellLeft = 0.f;
};
