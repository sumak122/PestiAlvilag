// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FireStation.generated.h"

class AFireTruck;
class APedestrian;
class AShapeSet;

/** What the station's two firemen are doing: on duty, running after a stolen engine, shaking their fists, walking back. */
enum class EFireDuty : uint8 { OnDuty, Chasing, Shaking, Back };

/**
 * The VIII. district fire station on Dologház utca (FireStationModel): its
 * dressed front and drill tower over the plain house front, two firemen on
 * duty, and the brigade's engine (AFireTruck) waiting on the apron. Taking
 * it is a crime: one fireman shouts, both run after it a few steps and
 * shake their fists, and the police are called (two stars). A new engine
 * stands there again the next morning, once the player is away.
 */
UCLASS()
class PESTIALVILAG_API AFireStation : public AActor
{
	GENERATED_BODY()

public:
	AFireStation();

	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	UPROPERTY(Transient)
	TObjectPtr<AShapeSet> Front;

	/** The engine waiting on the apron, while it waits. */
	UPROPERTY(Transient)
	TObjectPtr<AFireTruck> Parked;

	UPROPERTY(Transient)
	TArray<TObjectPtr<APedestrian>> Firemen;

private:
	/** The front, its lamps and the firemen, once the city is there. */
	bool Build();
	bool GroundAt(const FVector2D& Near, double& OutZ) const;
	void SpawnEngine();
	/** The engine has gone: the shout, the police, the firemen after it. */
	void Stolen();
	void TickFiremen(float DeltaSeconds, bool bNear);

	FTransform Frame;
	double Ground = 0.0;
	bool bBuilt = false;
	/** Waiting for the next morning before a new engine stands there. */
	bool bWaitForMorning = false;
	float LastHour = -1.f;
	EFireDuty Duty = EFireDuty::OnDuty;
	float DutyTime = 0.f;
	float Clock = 0.f;
	/** The stolen engine, for the firemen to run after. */
	TWeakObjectPtr<AFireTruck> Thief;
};
