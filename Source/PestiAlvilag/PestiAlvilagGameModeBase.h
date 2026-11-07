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
 * you arrested. P toggles the police for testing. Also puts up the taxi
 * company with two of its taxis, and keeps the player's money.
 *
 * Saving: the game is saved by sleeping at home (Bözsi néni's door on
 * Népszínház utca). A new game starts with the prologue at Keleti station;
 * a saved game starts at the home door. F9 throws the save away and starts
 * a new game.
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
	/** How many times the police have caught him (missions tell a getaway from an arrest). */
	int32 GetArrests() const { return Arrests; }
	void ReportCrime(int32 CrimeLevel);
	/** Seconds still to stay out of police sight to lose them. */
	float GetEscapeSecondsLeft() const { return FMath::Max(0.f, EscapeTime - TimeUnseen); }
	/** How far an arrest has got, from 0 to 1. */
	float GetArrestFraction() const { return ArrestTime > 0.f ? FMath::Clamp(ArrestProgress / ArrestTime, 0.f, 1.f) : 0.f; }
	/** Police call these every frame they see the player or are right beside them. */
	void NotifySeenByPolice();
	void NotifyPoliceNear();

	/**
	 * A constable has telephoned the station from a police box: for a while
	 * the police cars of the whole district come after the player, not just
	 * the ones nearby, and he is wanted a step more.
	 */
	void CallInPolice();
	/** True while the cars called by telephone are out. */
	bool IsPoliceCalledIn() const;

	/** How long the cars called by telephone keep coming (s). */
	UPROPERTY(EditAnywhere, Category = "Police")
	float CalledInTime = 60.f;

	/** The player's money in fillér (100 fillér = 1 pengő). */
	int32 GetMoney() const { return Money; }
	void AddMoney(int32 Filler);
	/** Money as the period wrote it, for example "1,20 pengő". */
	static FString FormatMoney(int32 Filler);

	/** Taxi fares driven for the company, over all shifts. */
	int32 GetTaxiJobs() const { return TaxiJobs; }
	void AddTaxiJob() { ++TaxiJobs; }

	/** Dishes eaten at least once; true the first time. */
	bool TryDish(FName DishId)
	{
		if (TriedDishes.Contains(DishId))
		{
			return false;
		}
		TriedDishes.Add(DishId);
		return true;
	}
	bool HasTriedDish(FName DishId) const { return TriedDishes.Contains(DishId); }
	int32 GetDishesTried() const { return TriedDishes.Num(); }

	/** The hidden lighters found so far, by number. */
	bool HasFoundLighter(int32 Index) const { return FoundLighters.Contains(Index); }
	int32 GetLightersFound() const { return FoundLighters.Num(); }
	void AddFoundLighter(int32 Index) { FoundLighters.AddUnique(Index); }

	void MarkMissionDone(FName MissionId);

	/** Whether he has Schwimmer's bamboo baton (it makes his blows harder). */
	bool HasBaton();
	/** Schwimmer hands it over: he carries it from now on. */
	void GiveBaton();
	/** Starts the first story mission not yet passed, after a short pause (Delay seconds, or the usual below 0). */
	void StartNextMission(float Delay = -1.f);

	/**
	 * The player's health. Cars and trams hurt him when they hit him, and so
	 * does a hard fall; food heals him and sleeping at home makes him well
	 * again. At nothing he faints and comes round at home.
	 */
	float GetHealth() const { return Health; }
	/** True from fainting until he comes round. */
	bool IsFainted() const { return FaintUntil >= 0.0; }
	float GetMaxHealth() const { return MaxHealth; }
	void HurtPlayer(float Amount);
	void HealPlayer(float Amount);
	bool IsMissionDone(FName MissionId) const { return CompletedMissions.Contains(MissionId); }
	int32 GetMissionsDone() const { return CompletedMissions.Num(); }
	/** Throws the save away and starts the city again from the prologue (F9, the menu's New game). */
	void StartNewGame();
	/**
	 * For testing (the menu's mission list): starts the city again with the
	 * act 1 missions before Number (1 to 8) counted as done, and puts the
	 * player by the gold ring of mission Number. The save on disk is left
	 * as it is, until the player saves at home.
	 */
	static void JumpToMission(UWorld* World, int32 Number);

	/** Home can be slept at once the prologue has led the player there. */
	bool CanSaveAtHome() const;
	/** Parks the company's special taxi beside the kiosk, unless it is already in town. */
	void SpawnDeluxeTaxi();

	/** Writes money and missions done to the save slot. */
	bool SaveToDisk() const;

private:
	void UpdateWanted(float DeltaSeconds);
	void UpdateFaint();
	void SpawnNextMission();
	/** Where he stands on the pavement outside the home door. */
	FVector2D HomeStartAt() const;
	/** The home door: Bözsi néni's, or after the raid (mission 8) the attic room's on Bérkocsis utca; and the way it faces. */
	FVector2D HomeDoorSpot() const;
	double HomeDoorFacing() const;
	/** Moves the home door to where home is now. */
	void PlaceHomeDoor();
	void ClearWanted(const TCHAR* Message);
	/** Puts up the hero buildings the level doesn't have yet. */
	void SpawnLandmarks();
	/** Ground height under a map point, or 0 when nothing is there. */
	double GroundZ(const FVector2D& At) const;
	/** Reads the save slot the first time it is needed. */
	void LoadSaveOnce();
	/** After a jump to a mission: puts the player by its ring once it shows. */
	void TickPlaceAtRing();

	int32 WantedLevel = 0;
	int32 Arrests = 0;
	float Health = 100.f;
	float MaxHealth = 100.f;
	/** While fainted: when he comes round; otherwise below 0. */
	double FaintUntil = -1.0;
	double NextMissionAt = -1.0;
	int32 Money = 0;
	TArray<FName> CompletedMissions;
	int32 TaxiJobs = 0;
	TArray<FName> TriedDishes;
	TArray<int32> FoundLighters;
	bool bHasBaton = false;
	bool bSaveRead = false;
	/** A jump to a mission is waiting to put the player by its ring, until this time. */
	bool bPlaceAtRing = false;
	double PlaceAtRingUntil = 0.0;
	float TimeUnseen = 0.f;
	float ArrestProgress = 0.f;
	double LastPoliceNearTime = -1.0;
	/** Until when the cars called by telephone are out, or below 0. */
	double PoliceCalledUntil = -1.0;
};
