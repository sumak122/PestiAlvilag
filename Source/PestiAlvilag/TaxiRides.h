// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TaxiRides.generated.h"

class APlayerController;

/**
 * Riding in a taxi as a paying passenger. On foot beside a taxi (one parked
 * at a stand or one going by in the traffic), T calls up the places it can
 * take you, with the fare to each; a number key picks one, T again or
 * Escape lets it go. The screen fades out, you pay, and you step out on the
 * pavement there. After 20 taxi jobs of your own the drivers know you and
 * charge half. Not while the police are after you.
 *
 * Reads the places from the street furniture file (taxi_destinations).
 */
UCLASS()
class PESTIALVILAG_API ATaxiRides : public AActor
{
	GENERATED_BODY()

public:
	ATaxiRides();

	virtual void Tick(float DeltaSeconds) override;

	/** The furniture file with the places, relative to the project folder. */
	UPROPERTY(EditAnywhere, Category = "Taxi")
	FString DataFile = TEXT("Data/OsmGraybox/street_furniture.json");

	/** How close a taxi must be to hail it (cm). */
	UPROPERTY(EditAnywhere, Category = "Taxi")
	float HailRange = 600.f;

	/** The meter: flag fall and per kilometre along the streets (fillér). */
	UPROPERTY(EditAnywhere, Category = "Taxi")
	int32 FlagFall = 80;

	UPROPERTY(EditAnywhere, Category = "Taxi")
	int32 PerKilometre = 60;

	/** Taxi jobs the player must have done to ride at half fare. */
	UPROPERTY(EditAnywhere, Category = "Taxi")
	int32 RegularAfterJobs = 20;

protected:
	virtual void BeginPlay() override;

private:
	struct FPlace
	{
		FString Name;
		FVector2D At;
		double Yaw = 0.0;
	};

	bool LoadPlaces();
	/** The taxi next to the player on foot, or null. */
	AActor* TaxiNearby(const APawn* Player) const;
	int32 FareTo(const FVector& From, const FPlace& Place) const;
	void ShowChoices(const FVector& From) const;
	void Say(const FString& Text, float Seconds = 5.f) const;
	void SetChoosing(APlayerController* PC, bool bInChoosing);

	TArray<FPlace> Places;
	bool bChoosing = false;
	/** During the ride: the place, the fare, and the time the screen went dark. */
	int32 Riding = INDEX_NONE;
	int32 RideFare = 0;
	double RideStart = 0.0;
	bool bArrived = false;
};
