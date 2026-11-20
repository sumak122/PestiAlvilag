// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PestiCar.h"
#include "FireTruck.generated.h"

class AFireEngine;
class APedestrian;
class ATrafficManager;
class UMaterialInterface;
class UProceduralMeshComponent;
class USpotSound;

/**
 * The fire brigade's motor engine as a vehicle the player drives: a heavy,
 * slow APestiCar that draws itself with the brigade engine's look
 * (DailyLifeModel::FireEngine) instead of a car body. B (or the pad's
 * D-pad down) rings its bell while he drives, and the traffic ahead stops
 * for it as for the brigade's own engine. It waits in front of the fire
 * station (AFireStation), or takes the place of the brigade's engine
 * standing at a fire (TakeFrom).
 */
UCLASS()
class PESTIALVILAG_API AFireTruck : public APestiCar
{
	GENERATED_BODY()

public:
	AFireTruck();

	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Its bell is ringing (B while driving). */
	bool IsBellRinging() const { return bBellOn; }

	/**
	 * The brigade's engine standing at a fire, its crew off it: a drivable
	 * one in its place (the crew left standing, shaking their fists, and the
	 * police called), or null.
	 */
	static AFireTruck* TakeFrom(AFireEngine* Engine);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UProceduralMeshComponent>> Parts;

	/** The player on the driver's bench while he drives. */
	UPROPERTY(Transient)
	TObjectPtr<APedestrian> WheelMan;

	UPROPERTY(Transient)
	TObjectPtr<USpotSound> Bell;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> BaseMaterial;

private:
	/** The bell's sound, its swing and the traffic stopping ahead. */
	void UpdateBell(float DeltaSeconds);
	/** The player seated at the wheel while he drives, hidden otherwise. */
	void PlaceWheelMan(bool bDriven);
	ATrafficManager* FindTraffic();

	TWeakObjectPtr<ATrafficManager> Traffic;
	/** The ground under the box's middle, in the box's frame (cm). */
	double FloorZ = -90.0;
	bool bBellOn = false;
	bool bBlocking = false;
	float WheelTurn = 0.f;
	float BellClock = 0.f;
};
