// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PersonKit.h"
#include "StreetFights.generated.h"

class APestiPerson;
class ARuffian;

/**
 * Fights the player picks with people in the street, GTA style: on foot,
 * a click at someone next to him (a passer-by, or anyone else who hands
 * himself over with Begin) throws the first punch. The man swapped in for
 * him (ARuffian, Kind Passerby, in the same clothes) either puts his fists
 * up and fights back by the brawl's rules, or runs off; women always run.
 *
 * A constable or police car close by sees the first punch: the player is
 * wanted. Beating someone down costs a little street name. When the
 * player faints, is arrested or goes off, the other man walks away; the
 * beaten lie there until nobody sees them and are then cleared away.
 */
UCLASS()
class PESTIALVILAG_API AStreetFights : public AActor
{
	GENERATED_BODY()

public:
	AStreetFights();

	virtual void Tick(float DeltaSeconds) override;

	/** The one in this world (made the first time it is asked for). */
	static AStreetFights* Get(const UObject* WorldContextObject);

	/**
	 * The player picks a fight with someone dressed in Dress, standing on
	 * the ground at Ground facing FaceYaw: a fighter is put in his place (the
	 * caller then removes his own person) and the player throws the first
	 * punch. False (nothing done) when no fight can start now.
	 */
	bool Begin(APestiPerson* Player, const PersonKit::FOutfit& Dress, const FVector& Ground, double FaceYaw);

	/** True when the player is on foot, free, and has just clicked to punch. */
	static bool WantsToPunch(const APestiPerson* Player);

	/** The fight just begun is with a pickpocket carrying Filler pengő of the player's: he fights back, and gives it up when beaten. */
	void SetLoot(int32 Filler);

	/** True when a fight is going on (not beaten, not run off) within Radius of At. */
	static bool IsFightNear(const UObject* WorldContextObject, const FVector& At, double Radius);

private:
	struct FFight
	{
		TWeakObjectPtr<ARuffian> Man;
		double Since = 0.0;
		double BeatenAt = -1.0;
		bool bFightsBack = false;
		bool bRanOff = false;
		// Money the man took off the player (a pickpocket), handed back when he is beaten.
		int32 Loot = 0;
		// When the street crowd round this fight is next called (s).
		double NextGather = 0.0;
	};

	TArray<FFight> Fights;
	FRandomStream Random;
	int32 ArrestsSeen = 0;
	double LastShout = -10.0;

	void Say(const TCHAR* Hu, const TCHAR* En);
	/** He gives up: off and away (if he still can), and the player's fists down if it was him. */
	void SendOff(FFight& Fight, APestiPerson* Player);
};
