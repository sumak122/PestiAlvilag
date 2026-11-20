// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PestiCar.h"
#include "Motorbike.generated.h"

class APedestrian;
class UMaterialInterface;
class UProceduralMeshComponent;
class USpotSound;

/**
 * A Méray motorcycle with a sidecar (MotorbikeModel), as a vehicle the
 * player drives: a light, quick APestiCar that draws itself with the
 * motorcycle's looks instead of a car body (UCarVisuals::DrawNothing). Its
 * front fork turns as it steers, the wheels roll, B honks the bulb horn.
 * Parked ones stand by the kerb (AMotorbikeParking); taking one is a theft:
 * the owner comes running out and shouts.
 */
UCLASS()
class PESTIALVILAG_API AMotorbike : public APestiCar
{
	GENERATED_BODY()

public:
	AMotorbike();

	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** With or without the sidecar, and the sidecar's lacquer (MotorbikeModel::Paint). Before spawning. */
	void SetLook(bool bInSidecar, int32 InPaint);
	bool HasSidecar() const { return bSidecar; }

	/** Someone's machine parked in the street: the first ride on it is a theft. */
	void SetOwned(bool bInOwned) { bOwned = bInOwned; }
	bool IsOwned() const { return bOwned; }
	/** Has been ridden by the player at some time. */
	bool WasTaken() const { return bTaken; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UProceduralMeshComponent>> Parts;

	/** The player on the saddle while he rides. */
	UPROPERTY(Transient)
	TObjectPtr<APedestrian> Rider;

	UPROPERTY(Transient)
	TObjectPtr<USpotSound> Horn;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> BaseMaterial;

private:
	/** The owner runs out shouting, the deed is noted, the police told if they saw. */
	void Stolen();
	void UpdateHorn(float DeltaSeconds, bool bPressed);
	/** The player seated on the saddle while he rides, hidden otherwise. */
	void PlaceRider(bool bDriven);

	bool bSidecar = true;
	int32 PaintIndex = 0;
	bool bOwned = false;
	bool bTaken = false;
	/** The ground under the box's middle, in the box's frame (cm). */
	double FloorZ = -90.0;
	float WheelTurn = 0.f;
	float SteerAngle = 0.f;
	float LastYaw = 0.f;
	/** Seconds left of the honk; how many honks so far (a new one starts the sound afresh). */
	float HornTime = 0.f;
	int32 Honks = 0;
};
