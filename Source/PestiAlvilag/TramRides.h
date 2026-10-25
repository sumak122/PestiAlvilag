// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PersonKit.h"
#include "TramRides.generated.h"

class ATrafficManager;
class ATram;
class AWalkingPerson;

/**
 * Riding the tram as a passenger. On foot near a tram standing at a stop,
 * T gets you on (you pay the conductor the fare) and you sit down on a
 * free seat on the benches inside; T rings the bell to get off at the next stop, or gets you
 * off at once while the tram stands at one. You step off onto the
 * pavement by the stop sign. Before a tram leaves the map at the end of
 * its route, everybody gets off at its last stop. Not while the police
 * are after you.
 *
 * Near the camera, people wait at the stops; when the tram comes in, a
 * few riders get off and walk away, and the waiting people walk to the
 * platform steps and take a seat (the tram waits for them).
 *
 * Reads where the stops' pavements are from the street furniture file
 * (tram_stops); the stops themselves come with the tram routes.
 */
UCLASS()
class PESTIALVILAG_API ATramRides : public AActor
{
	GENERATED_BODY()

public:
	ATramRides();

	virtual void Tick(float DeltaSeconds) override;

	/** The furniture file with the stops, relative to the project folder. */
	UPROPERTY(EditAnywhere, Category = "Tram")
	FString DataFile = TEXT("Data/OsmGraybox/street_furniture.json");

	/** How close to a tram at a stop you must be to get on (cm). */
	UPROPERTY(EditAnywhere, Category = "Tram")
	float BoardRange = 1500.f;

	/** One ride, any distance (fillér). */
	UPROPERTY(EditAnywhere, Category = "Tram")
	int32 Fare = 20;

protected:
	virtual void BeginPlay() override;

private:
	struct FStopPlace
	{
		FString Name;
		/** On the track, and the way the tram runs there. */
		FVector2D At;
		double Yaw = 0.0;
		/** On the pavement beside it. */
		FVector2D Wait;
	};

	bool LoadStops();
	ATrafficManager* GetManager();
	/** The tram standing at a stop nearest the player within reach, or null. */
	ATram* TramToBoard(const APawn* Player) const;
	FString StopName(const ATram* Tram, int32 Stop) const;
	FString LastStopName(const ATram* Tram) const;
	bool IsLastStop(const ATram* Tram) const;
	/** Gets the player off the tram by the stop it stands at; false if there is no room. */
	bool GetOff(class APestiPerson* Player, ATram* Tram);
	void Say(const FString& Text, float Seconds = 5.f) const;
	/** The pavement place of a tram's stop, or null. */
	const FStopPlace* PlaceFor(const ATram* Tram, int32 Stop) const;

	/** The people at one tram's next stop: waiting, then walking to the steps. */
	struct FStopCrowd
	{
		int32 Stop = INDEX_NONE;
		bool bWaitingPlaced = false;
		bool bExchanged = false;
		TArray<TWeakObjectPtr<AWalkingPerson>> Boarding;
	};
	void UpdateStopCrowds();
	/** The tram left without them: they walk off. */
	void SendAway(FStopCrowd& Crowd, const FStopPlace* Place);
	AWalkingPerson* SpawnWalker(const FVector& At, double Yaw, const PersonKit::FOutfit& Outfit);

	TArray<FStopPlace> Stops;
	TWeakObjectPtr<ATrafficManager> Manager;
	TWeakObjectPtr<ATram> Riding;
	/** Rang the bell: off at the next stop. */
	bool bWantOff = false;
	/** The stop the tram stood at when we last looked, so it is called out once. */
	int32 CalledStop = INDEX_NONE;

	TMap<TWeakObjectPtr<ATram>, FStopCrowd> Crowds;
	FRandomStream Random;
};
