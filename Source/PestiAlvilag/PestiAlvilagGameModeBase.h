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
	void ReportCrime(int32 CrimeLevel);
	/** Seconds still to stay out of police sight to lose them. */
	float GetEscapeSecondsLeft() const { return FMath::Max(0.f, EscapeTime - TimeUnseen); }
	/** How far an arrest has got, from 0 to 1. */
	float GetArrestFraction() const { return ArrestTime > 0.f ? FMath::Clamp(ArrestProgress / ArrestTime, 0.f, 1.f) : 0.f; }
	/** Police call these every frame they see the player or are right beside them. */
	void NotifySeenByPolice();
	void NotifyPoliceNear();

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
	/** Starts the first story mission not yet passed, after a short pause. */
	void StartNextMission();

	/**
	 * The player's health. Cars and trams hurt him when they hit him, and so
	 * does a hard fall; food heals him and sleeping at home makes him well
	 * again. At nothing he faints and comes round at home.
	 */
	float GetHealth() const { return Health; }
	float GetMaxHealth() const { return MaxHealth; }
	void HurtPlayer(float Amount);
	void HealPlayer(float Amount);
	bool IsMissionDone(FName MissionId) const { return CompletedMissions.Contains(MissionId); }

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
	static FVector2D HomeStartAt();
	void ClearWanted(const TCHAR* Message);
	/** Puts up the hero buildings the level doesn't have yet. */
	void SpawnLandmarks();
	/** Ground height under a map point, or 0 when nothing is there. */
	double GroundZ(const FVector2D& At) const;
	/** Reads the save slot the first time it is needed. */
	void LoadSaveOnce();
	void StartNewGame();

	int32 WantedLevel = 0;
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
	bool bSaveRead = false;
	float TimeUnseen = 0.f;
	float ArrestProgress = 0.f;
	double LastPoliceNearTime = -1.0;
};
