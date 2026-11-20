// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Eateries.generated.h"

class UPointLightComponent;

/**
 * The nine places to eat (EateryModel): the Hauer pastry shop, a cook-shop,
 * the milk bar, the carters' inn, a wine cellar, the Kispipa, the standing
 * buffet by Keleti, a kosher eating house and the Hotel Nemzeti's restaurant.
 *
 * Their fronts are shop fronts on the map (street_furniture.py EATERIES; the
 * Nemzeti's is the hotel door). On foot by the door, E takes the player in,
 * the way the music bars do: a moment of black and he stands inside the room,
 * built far out west of the city; E by the door inside takes him out. Each
 * keeps its own hours (shut, the door says when it opens), nobody goes in
 * while the police are after him, and the Nemzeti's doorman wants the suit.
 *
 * Inside: guests at the tables, staff behind the counter, a waiter going
 * round where there is one, and the Kispipa's pianist. The player orders at
 * the counter (GetCounters, read by AFoodMenus) or from the waiter, each
 * place with its own menu (EFoodMenu::Hauer...). The places show on the maps.
 */
UCLASS()
class PESTIALVILAG_API AEateries : public AActor
{
	GENERATED_BODY()

public:
	AEateries();

	virtual void Tick(float DeltaSeconds) override;

	/** True while the player is inside one of the places. */
	bool IsPlayerInside() const { return Inside != INDEX_NONE; }

	/** Where to order at the counters (world, on the floor) and each one's menu (an EFoodMenu). */
	void GetCounters(TArray<FVector>& OutAt, TArray<uint8>& OutMenu) const;

protected:
	virtual void BeginPlay() override;

private:
	void BuildRoom(int32 Place);
	void TickDoors();
	void GoIn(int32 Place);
	void GoOut();
	bool IsOpen(int32 Place) const;
	/** Draws and runs only the people of this room (INDEX_NONE: none). */
	void SetRoomShown(int32 Place);

	UPROPERTY(VisibleAnywhere, Category = "Eateries")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UPointLightComponent>> Lights;

	/** Everyone in the rooms, and which room each is in. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<AActor>> People;
	TArray<int32> PeopleRoom;

	TArray<FVector> StreetDoor;
	TArray<double> StreetYaw;
	TArray<FVector> RoomAt;
	TArray<FVector> CounterAt;

	int32 Inside = INDEX_NONE;
	bool bIconsSent = false;
};
