// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Tattersall.generated.h"

class AShapeSet;
class AStoryPerson;
class UBoxComponent;
class UCarVisuals;
class UPointLightComponent;

/**
 * The Tattersall on Kerepesi út, behind its gate lodges, with the 1932
 * international motor show in it (CarShowModel.h): the hall, its glass
 * doors (open by day, shut at night), the black Mercedes SSK on its
 * turntable and four more cars on stands (looks only, behind blocking
 * boxes), lamps that light the hall while the player is near, and by day
 * (CarShowModel::OpenFrom to OpenUntil) Mr. Weisz the salesman by the SSK,
 * visitors round the cars and a doorman at the door, who lets the player in
 * only in a proper suit (game mode HasSuit, ATailor). The car show missions (AMissionHirdetes,
 * AMissionKulcs) come here.
 *
 * The actor stands at the middle of the hall's front on the ground, turned
 * as the hall's frame (+X out to the street).
 */
UCLASS()
class PESTIALVILAG_API ATattersall : public AActor
{
	GENERATED_BODY()

public:
	ATattersall();

	virtual void Tick(float DeltaSeconds) override;

	/** Where the hall stands in World: its front's middle on the ground, facing the street. */
	static FTransform Frame(const UWorld* World);

	/** A point in the hall's frame (CarShowModel) in the city. */
	static FVector ToCity(const UWorld* World, const FVector& Local);

	/** True while the show is open (CarShowModel::OpenFrom to OpenUntil). */
	static bool IsOpen(const UWorld* World);

	/** Mr. Weisz the salesman: in the hall while it is open, hidden otherwise. */
	AStoryPerson* GetSalesman() const { return Salesman.Get(); }

protected:
	virtual void BeginPlay() override;

private:
	/** The hall's shapes, lettering and (a few of its) lamps, and its glass doors as a set of their own. */
	void PutUpHall();
	/** The visitors and the salesman, in the world while it is open. */
	void PlacePeople();
	/** Doors, people and the inside lamps for the time of day and how near the player is. */
	void Update();

	/** The five cars on show: a blocking box each with the car's looks on it (the SSK first). */
	UPROPERTY(VisibleAnywhere, Category = "Show")
	TArray<TObjectPtr<UBoxComponent>> CarBoxes;

	UPROPERTY(VisibleAnywhere, Category = "Show")
	TArray<TObjectPtr<UCarVisuals>> CarLooks;

	/** Daylight in the hall, on while the player is near. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UPointLightComponent>> HallLights;

	UPROPERTY(Transient)
	TObjectPtr<AShapeSet> Hall;

	UPROPERTY(Transient)
	TObjectPtr<AShapeSet> Doors;

	UPROPERTY(Transient)
	TObjectPtr<AStoryPerson> Salesman;

	/** The doorman by the glass doors while the show is open. */
	UPROPERTY(Transient)
	TObjectPtr<AStoryPerson> Doorman;

	/** Across the open doorway while the player has no suit: the doorman lets nobody in in street clothes. */
	UPROPERTY(VisibleAnywhere, Category = "Show")
	TObjectPtr<UBoxComponent> DoorBar;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AStoryPerson>> Visitors;

	/** 1 open, 0 shut, -1 not yet known. */
	int32 OpenState = -1;
	float UpdateIn = 0.f;
	/** The doorway is barred (open, and the player has no suit). */
	bool bBarred = false;
	/** When the doorman last turned the player away (s). */
	double LastTurnedAway = -100.0;
};
