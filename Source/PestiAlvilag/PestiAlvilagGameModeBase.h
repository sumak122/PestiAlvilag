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

	/**
	 * Seconds out of police sight that lose them, at two stars (less at one,
	 * more at three; half as long hidden in a courtyard gateway).
	 */
	UPROPERTY(EditAnywhere, Category = "Police")
	float EscapeTime = 20.f;

	/** Seconds slow beside a police car before being arrested. */
	UPROPERTY(EditAnywhere, Category = "Police")
	float ArrestTime = 3.f;

	/** 0 = not wanted; higher means more trouble. */
	int32 GetWantedLevel() const { return WantedLevel; }
	/** The police lose the player (Laci's garage gave the car new paint); Message goes on the screen. */
	void LoseThePolice(const TCHAR* Message) { ClearWanted(Message); }
	/** How many times the police have caught him (missions tell a getaway from an arrest). */
	int32 GetArrests() const { return Arrests; }
	void ReportCrime(int32 CrimeLevel);
	/** Seconds still to stay out of police sight to lose them. */
	float GetEscapeSecondsLeft() const { return FMath::Max(0.f, EscapeTimeNow() - TimeUnseen); }
	/** Seconds out of sight that lose them at the stars he has now. */
	float EscapeTimeNow() const;
	/**
	 * Out of their sight a moment, the police search round where they last
	 * saw him (the red circle on the maps): leaving the circle loses them
	 * soon, hiding in a courtyard gateway loses them faster.
	 */
	bool IsPoliceSearching() const;
	FVector GetLastSeenAt() const { return LastSeenAt; }
	float GetSearchRadius() const;
	/** True while he hides in a courtyard gateway from the police. */
	bool IsHiding() const { return bHiding; }
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
	void AddTaxiJob();

	/** Cars brought to Laci on his orders, each as Body * 100 + paint (ALaciOrders), oldest first. */
	const TArray<int32>& GetLaciCars() const { return LaciCars; }
	void AddLaciCar(int32 Car) { LaciCars.Add(Car); }

	/** Pista bácsi's cart runs done (APistaRuns). */
	int32 GetCartRuns() const { return CartRuns; }
	void AddCartRun() { ++CartRuns; }

	/** Varga's delivery rounds (AVargaRuns): the best time in tenths of a second (0 for none) and the best medal (0 none, 1 bronze, 2 silver, 3 gold). */
	int32 GetBestRoundTenths() const { return BestRoundTenths; }
	int32 GetBestMedal() const { return BestMedal; }
	void NoteRound(int32 Tenths, int32 Medal)
	{
		BestRoundTenths = BestRoundTenths <= 0 ? Tenths : FMath::Min(BestRoundTenths, Tenths);
		BestMedal = FMath::Max(BestMedal, Medal);
	}

	/** Laci's night races won (bits, ANightRaces order). */
	bool HasWonRace(int32 Index) const { return Index >= 0 && Index < 31 && (RacesWon & (1 << Index)) != 0; }
	void MarkRaceWon(int32 Index)
	{
		if (Index >= 0 && Index < 31)
		{
			RacesWon |= 1 << Index;
		}
	}
	int32 GetRacesWon() const { return RacesWon; }

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
	void AddFoundLighter(int32 Index);

	void MarkMissionDone(FName MissionId);

	/**
	 * Fight points (bunyópont): every blow the player lands earns some
	 * (see Brawl::PointsHit), and a note shows the points of the fight so
	 * far. Bandi (ABoxingYard) teaches moves for them.
	 */
	int32 GetFightPoints() const { return FightPoints; }
	void AddFightPoints(int32 Points);
	/** Bandi's lessons (Brawl::ELesson, as a number): learnt or not, and learning one for its points; false if short of them. */
	bool KnowsLesson(int32 Lesson) const { return Lesson >= 0 && Lesson < 32 && (Lessons & (1 << Lesson)) != 0; }
	bool LearnLesson(int32 Lesson, int32 Cost);
	/** The men beaten in Bandi's ring so far, in order (0 to 5). */
	int32 GetBoxingWins() const { return BoxingWins; }
	void AddBoxingWin();

	/**
	 * Street name (hírnév, StreetName.h): points from 0 to 100. Missions,
	 * boxing wins, lighters found and taxi fares earn them; running people
	 * down costs them. Each rank has passers-by greet him, and the higher
	 * ones make food and taxi rides cheaper (PriceFor). A note shows each
	 * change, and a big line a new rank.
	 */
	int32 GetRenown() const { return RenownPoints; }
	int32 GetRenownRank() const;
	void AddRenown(int32 Points, const FString& WhyHu, const FString& WhyEn);
	/** A price in fillér after what his street name takes off (to whole 10 fillér). */
	static int32 PriceFor(const UObject* WorldContextObject, int32 Filler);
	/** Whether act 1 mission Index (0 for the prologue to 7 for the fur coat) is done. */
	bool IsActOneDone(int32 Index) const;
	/** The employer's letter waiting at the home door (StreetName::Letters), or INDEX_NONE. */
	int32 GetWaitingLetter() const;
	bool HasReadLetter(int32 Index) const { return Index >= 0 && Index < 32 && (LettersRead & (1 << Index)) != 0; }
	int32 GetLettersRead() const;
	/** He opens the employer's letter Index: the money in it goes in his pocket. */
	void ReadLetter(int32 Index);

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
	/** Caught: outside a mission the picture goes dark and he comes out of the police station, poorer. */
	void Busted();
	void UpdateBusted();
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
	/** Passers-by greet him by his street name, and Bözsi néni has a word at her stall. */
	void TickGreetings();

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
	TArray<int32> LaciCars;
	int32 CartRuns = 0;
	int32 BestRoundTenths = 0;
	int32 BestMedal = 0;
	int32 RacesWon = 0;
	TArray<FName> TriedDishes;
	TArray<int32> FoundLighters;
	bool bHasBaton = false;
	int32 FightPoints = 0;
	int32 Lessons = 0;
	int32 BoxingWins = 0;
	int32 RenownPoints = 0;
	/** The employer's letters opened (bits, StreetName::Letters order). */
	int32 LettersRead = 0;
	/** When a passer-by may next greet him, and Bözsi néni next speak. */
	double NextGreetAt = 0.0;
	double NextBozsiAt = 0.0;
	/** When to announce the rank he has just reached, or below 0. */
	double RankAnnounceAt = -1.0;
	/** The points of the fight going on, and when the last came (for the note). */
	int32 RecentPoints = 0;
	double RecentPointsAt = -100.0;
	bool bSaveRead = false;
	/** A jump to a mission is waiting to put the player by its ring, until this time. */
	bool bPlaceAtRing = false;
	double PlaceAtRingUntil = 0.0;
	float TimeUnseen = 0.f;
	float ArrestProgress = 0.f;
	/** Where the police last had him in sight, and whether he is hiding now. */
	FVector LastSeenAt = FVector::ZeroVector;
	bool bHiding = false;
	/** After an arrest: when he is let out of the station, or below 0; and what it cost him. */
	double BustedUntil = -1.0;
	int32 BustedFine = 0;
	int32 BustedStars = 0;
	double LastPoliceNearTime = -1.0;
	/** Until when the cars called by telephone are out, or below 0. */
	double PoliceCalledUntil = -1.0;
};
