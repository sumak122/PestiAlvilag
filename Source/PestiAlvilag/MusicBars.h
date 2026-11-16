// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MusicBars.generated.h"

class ABandPlayer;
class AStoryPerson;
class UBandSound;
class UPointLightComponent;

/**
 * The two music bars of Dzsoki's side quests (MusicBarModel), and the chain
 * of his missions.
 *
 * The bars' fronts are shop fronts on the map (street_furniture.py
 * MUSIC_BARS): the Fehér Galamb tavern on Vásár utca and the Rózsa coffee
 * house on József körút. On foot by the door, E takes the player in, the
 * way GTA 3 does it: a moment of black and he stands inside the room, which
 * is built far out west of the city (closed in, with its own lights). E by
 * the door inside takes him back out to the street. While he is inside, the
 * rain is not drawn and is heard only faintly (AWeather::SetIndoors).
 *
 * Guests sit at the tables and a waiter goes round them (he takes orders
 * like the café's). From the evening (BarRooms::OpensAt in MusicBars.cpp) till the small hours
 * the band plays (ABandPlayer, UBandSound): Dzsoki at his cimbalom with
 * Rózsi singing in the tavern, Gyuszi's band in the coffee house; heard
 * faintly through the wall by the street door too. By day Dzsoki and Rózsi
 * sit at a table in the tavern.
 *
 * After mission 7 Dzsoki's missions come one by one (AMission with
 * bSideStory): each waits in a ring by Dzsoki outside the tavern door;
 * while one is about, Dzsoki is out there and not inside (SetDzsokiOut),
 * and Rózsi too while she is out in one (SetRozsiOut). The band plays on
 * without them.
 */
UCLASS()
class PESTIALVILAG_API AMusicBars : public AActor
{
	GENERATED_BODY()

public:
	AMusicBars();

	virtual void Tick(float DeltaSeconds) override;

	/** The bars in World, or null. */
	static AMusicBars* Find(const UWorld* World);

	/** 0 the tavern, 1 the coffee house. */
	static constexpr int32 BarTavern = 0;
	static constexpr int32 BarCafe = 1;

	/** On the pavement in front of a bar's street door (on the ground), and the way out to the street (yaw). */
	FVector GetStreetDoor(int32 Bar) const { return StreetDoor[Bar]; }
	double GetStreetYaw(int32 Bar) const { return StreetYaw[Bar]; }
	/** Where Dzsoki waits with a mission: on the pavement beside the tavern door, facing the street (on the ground). */
	FVector GetGiverSpot() const { return StreetDoor[BarTavern] + FRotator(0.0, StreetYaw[BarTavern], 0.0).RotateVector(FVector(0.0, 230.0, 0.0)); }

	/** True while the player is inside one of the bars. */
	bool IsPlayerInside() const { return Inside != INDEX_NONE; }

	/** A mission has Dzsoki waiting outside his tavern (or not): he is not inside meanwhile. */
	void SetDzsokiOut(bool bOut) { bDzsokiOut = bOut; }
	/** Rózsi is out in a mission: not on the stage nor at the table. */
	void SetRozsiOut(bool bOut) { bRozsiOut = bOut; }

protected:
	virtual void BeginPlay() override;

private:
	/** Builds a bar's room, its lights, guests, waiter and band, at its place out west. */
	void BuildRoom(int32 Bar);
	/** By a street door on foot: E goes in. Inside by the door: E goes out. */
	void TickDoors();
	void GoIn(int32 Bar);
	void GoOut();
	/** Band on or off by the hour, who sits where, and how loud the music is heard. */
	void TickBands();
	/** The next of Dzsoki's missions, when it is time. */
	void TickChain();

	UPROPERTY(VisibleAnywhere, Category = "Bars")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UBandSound>> Music;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UPointLightComponent>> Lights;

	/** Each bar's band, and the people who sit in the tavern by day (Dzsoki, Rózsi). */
	UPROPERTY(Transient)
	TArray<TObjectPtr<ABandPlayer>> TavernBand;
	UPROPERTY(Transient)
	TArray<TObjectPtr<ABandPlayer>> CafeBand;
	UPROPERTY(Transient)
	TArray<TObjectPtr<AStoryPerson>> DaySitters;
	/** Dzsoki and Rózsi on the stage and at their table: kept out of sight while they are out for a mission. */
	UPROPERTY(Transient)
	TObjectPtr<ABandPlayer> StageDzsoki;
	UPROPERTY(Transient)
	TObjectPtr<AStoryPerson> TableDzsoki;
	UPROPERTY(Transient)
	TObjectPtr<ABandPlayer> StageRozsi;
	UPROPERTY(Transient)
	TObjectPtr<AStoryPerson> TableRozsi;

	FVector StreetDoor[2] = { FVector::ZeroVector, FVector::ZeroVector };
	double StreetYaw[2] = { 0.0, 0.0 };
	/** Where each room stands out west (its street door in the middle of its front wall, on the floor). */
	FVector RoomAt[2] = { FVector::ZeroVector, FVector::ZeroVector };

	int32 Inside = INDEX_NONE;
	bool bBandOn = false;
	bool bDzsokiOut = false;
	bool bRozsiOut = false;
	bool bIconsSent = false;
	float ChainCheckIn = 0.f;
};
