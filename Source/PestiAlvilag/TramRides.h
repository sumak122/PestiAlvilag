// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TramRides.generated.h"

class ATrafficManager;
class ATram;

/**
 * Riding the tram as a passenger. On foot near a tram standing at a stop,
 * T gets you on (you pay the conductor the fare) and you sit down on a
 * free seat on the benches inside; T rings the bell to get off at the next stop, or gets you
 * off at once while the tram stands at one. You step off onto the
 * pavement by the stop sign. Before a tram leaves the map at the end of
 * its route, everybody gets off at its last stop. Not while the police
 * are after you.
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

	TArray<FStopPlace> Stops;
	TWeakObjectPtr<ATrafficManager> Manager;
	TWeakObjectPtr<ATram> Riding;
	/** Rang the bell: off at the next stop. */
	bool bWantOff = false;
	/** The stop the tram stood at when we last looked, so it is called out once. */
	int32 CalledStop = INDEX_NONE;
};
