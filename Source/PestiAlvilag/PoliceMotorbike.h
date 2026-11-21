// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PoliceCar.h"
#include "PoliceMotorbike.generated.h"

class APoliceman;
class UProceduralMeshComponent;

/**
 * A motorbike constable (MotorbikeModel, police paint, the rider in a
 * leather crash helmet): a police patrol like APoliceCar, but on a solo
 * Méray. It joins a chase only from two stars, is quicker and turns
 * sharper than the cars, and has one constable, who gets off to lay hands
 * on the player. Knocked by a crash, he is thrown off and comes on foot.
 */
UCLASS()
class PESTIALVILAG_API APoliceMotorbike : public APoliceCar
{
	GENERATED_BODY()

public:
	APoliceMotorbike();

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual PersonKit::FOutfit OfficerOutfit(FRandomStream& Random) const override;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UProceduralMeshComponent>> Parts;

	/** The constable in the saddle while he rides. */
	UPROPERTY(Transient)
	TObjectPtr<APoliceman> Rider;

private:
	/** Swaps the car's looks for the motorbike's, once it is on the road. */
	void Dress();
	void PlaceRider(bool bRiding);

	bool bDressed = false;
	float WheelTurn = 0.f;
	float SteerAngle = 0.f;
	float LastYaw = 0.f;
};
