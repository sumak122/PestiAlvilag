// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Garage.generated.h"

class AShapeSet;
class APestiCar;

/**
 * The player's lock-up garage on Bérkocsis utca, by his attic (GarageModel.h),
 * his from the end of act 1. Its two green doors swing open when he comes
 * near, on foot or driving, unless the police are after him. A car left
 * inside is kept when the doors shut behind him (its looks in the game
 * mode, saved with the game at home) and stands there again the next time
 * they open; one car at a time. A kept car driven out and left elsewhere is
 * no longer kept.
 *
 * The actor stands at the middle of the door on the house front, on the
 * ground, turned as the garage's frame (+X out to the street).
 */
UCLASS()
class PESTIALVILAG_API AGarage : public AActor
{
	GENERATED_BODY()

public:
	AGarage();

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

private:
	/** Turns the doors to Amount open (0 shut, 1 wide open); shut doors block the way. */
	void SetDoors(float Amount);
	/** True when Point (in the city) is inside the room, out of the doors' way. */
	bool IsInRoom(const FVector& Point) const;
	/** True when Point is inside the room or in the doorway in front of it. */
	bool IsInDoorway(const FVector& Point) const;
	/** A car standing in the room with nobody driving it, or null. */
	APestiCar* FindCarInRoom() const;
	/** The doors have just shut: the car in the room is kept (and put away), or the garage is empty. */
	void OnShut();
	/** The doors are opening: the kept car stands in the room again. */
	void BringOutKeptCar();
	void Note(const FString& Text) const;

	UPROPERTY(Transient)
	TObjectPtr<AShapeSet> Room;

	/** The left and right door, each turning about its hinge. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<AShapeSet>> Leaves;

	/** The kept car while it stands in the world (the doors open). */
	UPROPERTY(Transient)
	TObjectPtr<APestiCar> KeptCar;

	float Opened = 0.f;
	bool bDoorsBlock = true;
	/** When the "police" note was last shown (s). */
	double LastWantedNote = -100.0;
};
