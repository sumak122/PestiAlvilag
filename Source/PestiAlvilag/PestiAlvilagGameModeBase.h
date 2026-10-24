// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "PestiAlvilagGameModeBase.generated.h"

/**
 * Puts up the Népszínház on Blaha Lujza tér, starts the player on foot in
 * front of it with their car parked nearby, starts the traffic, and keeps the player's wanted level (on foot
 * or driving): ramming traffic makes the police come after you; staying out
 * of their sight long enough loses them, and staying slow next to one gets
 * you arrested. P toggles the police for testing.
 */
UCLASS()
class PESTIALVILAG_API APestiAlvilagGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	APestiAlvilagGameModeBase();

	virtual void StartPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual APawn* SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot) override;

	/** The running game's mode, or null (for example in the editor). */
	static APestiAlvilagGameModeBase* Get(const UObject* WorldContextObject);

	/** Seconds out of police sight that lose them. */
	UPROPERTY(EditAnywhere, Category = "Police")
	float EscapeTime = 20.f;

	/** Seconds slow beside a police car before being arrested. */
	UPROPERTY(EditAnywhere, Category = "Police")
	float ArrestTime = 3.f;

	/** 0 = not wanted; higher means more trouble. */
	int32 GetWantedLevel() const { return WantedLevel; }
	void ReportCrime(int32 CrimeLevel);
	/** Police call these every frame they see the player or are right beside them. */
	void NotifySeenByPolice();
	void NotifyPoliceNear();

private:
	void UpdateWanted(float DeltaSeconds);
	void ClearWanted(const TCHAR* Message);
	/** Ground height under a map point, or 0 when nothing is there. */
	double GroundZ(const FVector2D& At) const;

	int32 WantedLevel = 0;
	float TimeUnseen = 0.f;
	float ArrestProgress = 0.f;
	double LastPoliceNearTime = -1.0;
};
