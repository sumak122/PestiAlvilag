// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NightRaces.generated.h"

class ALaciYard;
class APestiCar;
class ARaceRival;
class AShapeSet;
class UGoalMarkerComponent;
class URouteGuideComponent;
class UStartRingComponent;

enum class ENightRace : uint8
{
	None,      // the ring waits in front of Laci's gate (at night)
	Countdown, // the cars lined up, 3, 2, 1
	Racing,
};

/**
 * Laci's night races, a side job after mission 7. After dark (21:00 till
 * half past four) a gold ring stands in the street in front of his yard's
 * gate; stop a car in it and Laci's pals line up two more beside you. Four
 * races, each a round through Józsefváros and back to the gate through
 * gates of lanterns his pals have hung across the streets, one just past
 * each corner where the way turns. Coming first
 * pays a prize (the first win of each also street name, kept in the game
 * mode, saved); later wins of the same race pay half. When all four are
 * won Laci has a word about a black Mercedes at the Tattersall show.
 */
UCLASS()
class PESTIALVILAG_API ANightRaces : public AActor
{
	GENERATED_BODY()

public:
	ANightRaces();

	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	bool IsRunning() const { return State != ENightRace::None; }

protected:
	UPROPERTY(VisibleAnywhere, Category = "Race")
	TObjectPtr<UStartRingComponent> StartRing;

	UPROPERTY(VisibleAnywhere, Category = "Race")
	TObjectPtr<UGoalMarkerComponent> Marker;

	UPROPERTY(VisibleAnywhere, Category = "Race")
	TObjectPtr<URouteGuideComponent> Guide;

private:
	bool IsMissionUnderway() const;
	bool IsNight() const;
	ALaciYard* GetYard() const;
	/** Where the ring stands: on the street's lane in front of the gate, and the way that lane heads. */
	bool FindStartSpot(FVector& OutAt, FVector& OutDir) const;
	/** The race the next start gives: the first not yet won, then each in turn. */
	int32 PickRace() const;
	/** Plans the line and its gates, lines up the rivals, starts the count. */
	bool StartRace(APestiCar* Car);
	void TickCountdown(APestiCar* Car, float DeltaSeconds);
	void TickRacing(APawn* Player, float DeltaSeconds);
	void Finish(int32 Place);
	void StopRace(const FString& Why);
	/** Puts up the gates from First on (two at a time), taking down the ones before. */
	void ShowGates(int32 First);
	void ClearGates();
	/** How far along the line the player is (cm). */
	double PlayerAlong(const FVector& At) const;
	double GroundAt(const FVector& At) const;
	void Note(const FString& Line, float Seconds) const;
	void LaciSays(const FString& Hu, const FString& En) const;

	ENightRace State = ENightRace::None;
	int32 Race = 0;

	/** The race's line through the streets, and how far along it each point is. */
	TArray<FVector> RaceLine;
	TArray<double> RaceAlong;

	/** The gates: where they stand, which way the cars come, half their width, how far along the line. The last is the finish. */
	TArray<FVector> GateAt;
	TArray<FVector> GateDir;
	TArray<double> GateHalf;
	TArray<double> GateAlong;
	/** The place each gate stands at, shown on screen as the player heads for it. */
	TArray<FString> GateName;
	int32 NextGate = 0;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AShapeSet>> GateSets;
	/** Which gate each of GateSets shows. */
	TArray<int32> GateShown;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ARaceRival>> Rivals;

	FVector StartAt = FVector::ZeroVector;
	FVector StartDir = FVector::ForwardVector;
	float Clock = 0.f;
	float OutOfCar = 0.f;
	float NightHintIn = 0.f;
	/** How long the car has stood still in the ring (s). */
	float StoodFor = 0.f;
	/** Laci's word about the Mercedes, this many seconds after the last first win (0: none due). */
	float MercedesLineIn = 0.f;
	/** The ring starts a race only once the player has been away from it (not straight after one). */
	bool bRingArmed = true;
};
