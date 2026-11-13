// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PersonKit.h"
#include "StreetFights.generated.h"

class APestiPerson;
class APoliceman;
class ARuffian;

/** What became of a constable handed over to a fight (AStreetFights::ConstableState). */
enum class EConstableFight : uint8
{
	None,
	// Still at it: the fighter in his uniform stands in for him.
	Fighting,
	// Just beaten down: he lies there, then gets up.
	Down,
	// Just out of it, on his feet (the player ran off, was taken, fainted): after him again.
	Back
};

/**
 * Fights the player picks with people in the street, GTA style: on foot,
 * a click at someone next to him (a passer-by, or anyone else who hands
 * himself over with Begin) throws the first punch. The man swapped in for
 * him (ARuffian, Kind Passerby, in the same clothes) either puts his fists
 * up and fights back by the brawl's rules, or runs off; women always run.
 *
 * A constable or police car close by sees the first punch: the player is
 * wanted.
 *
 * Constables can be fought too (BeginConstable): a click at one throws the
 * first punch, and one who catches up with the player while he is in a
 * fist fight wades in, waiting his turn. The constable is hidden while a
 * fighter in his uniform, truncheon in hand, stands in; his owner (the foot
 * patrols, a police car) asks ConstableState what became of him. Hitting a
 * constable gives two stars, beating one down three, and a little street
 * name; standing by him does not get the player arrested while he fights,
 * but going down under the truncheon does. Beating someone down costs a little street name. When the
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

	/**
	 * A constable steps into a fight with the player, standing on the ground
	 * at Ground facing FaceYaw: hidden while a fighter in his uniform stands
	 * in. bPlayerStarts: the player clicked at him and throws the first
	 * punch; else he wades in, waiting his turn while the player is busy with
	 * someone else. False (nothing done) when no fight can start now.
	 */
	bool BeginConstable(APestiPerson* Player, APoliceman* Constable, const FVector& Ground, double FaceYaw, bool bPlayerStarts);

	/**
	 * What became of a constable handed over to a fight. Fighting while he
	 * is at it; Down or Back once, as it ends (he is then shown again). OutAt:
	 * where on the map he is or ended up.
	 */
	static EConstableFight ConstableState(const UObject* WorldContextObject, const APoliceman* Constable, FVector2D& OutAt);

	/** True when the player is in a fist fight a constable can wade into. */
	static bool CanJoinIn(const APestiPerson* Player);

	/** True when a click now at something at At is a punch at it: within reach, in front, about level. */
	static bool InPunchReach(const APestiPerson* Player, const FVector& At);

	/** True while a constable is fighting the player (going down then means arrest). */
	static bool IsPoliceFighting(const UObject* WorldContextObject);

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
		// A constable's fight: the man he really is (hidden meanwhile), and his fighter's last spot on the map.
		TWeakObjectPtr<APoliceman> Constable;
		bool bConstable = false;
		bool bConstableHidCollision = false;
		FVector2D LastAt = FVector2D::ZeroVector;
	};

	/** A constable's fight that has ended, waiting for his owner to ask. */
	struct FConstableEnd
	{
		TWeakObjectPtr<APoliceman> Constable;
		EConstableFight State = EConstableFight::Back;
		FVector2D At = FVector2D::ZeroVector;
	};
	TArray<FConstableEnd> Ended;

	TArray<FFight> Fights;
	FRandomStream Random;
	int32 ArrestsSeen = 0;
	double LastShout = -10.0;

	/** A line from the street (a passer-by, unless Speaker says who). */
	void Say(const TCHAR* Hu, const TCHAR* En, const TCHAR* Speaker = nullptr);
	/** He gives up: off and away (if he still can), and the player's fists down if it was him. */
	void SendOff(FFight& Fight, APestiPerson* Player);
	/** Puts a fighter in Dress in the street, at Ground facing FaceYaw: a passer-by, or a constable (bPolice). */
	ARuffian* SpawnFighter(bool bPolice, const PersonKit::FOutfit& Dress, const FVector& Ground, double FaceYaw);
	/** A constable's fight is over: the fighter goes, the constable is shown again (bDown: lying beaten). */
	void HandBack(FFight& Fight, APestiPerson* Player, bool bDown);
	/** The fights' men take turns: the one the player faces comes in, the rest wait round him. */
	void TakeTurns(FFight& Fight, ARuffian* Man, APestiPerson* Player);
};
