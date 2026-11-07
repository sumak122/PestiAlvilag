// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Mission.generated.h"

class UGoalMarkerComponent;
class UMaterialInterface;
class URouteGuideComponent;
class UStartRingComponent;
class UStaticMeshComponent;

/** One step of a mission: get somewhere, perhaps on foot, perhaps pressing E there. */
struct FMissionStep
{
	/** What to do, in Hungarian and in English (shown together). */
	FString GoalHu;
	FString GoalEn;
	FVector Target = FVector::ZeroVector;
	/** How close to Target counts as there (cm). */
	float Radius = 300.f;
	bool bOnFoot = true;
	/** The step only ends when the player presses E within Radius. */
	bool bPressE = false;
	/** Arrows over the street turns on the way while the target is far off. */
	bool bGuide = true;
	/**
	 * The mission itself says when the step is done (FinishStep), not
	 * getting there; its target may move (SetStepTarget), say a man running.
	 */
	bool bManual = false;
	/** What people in the street call the place, when the player asks them the way (a street, a square); may be empty. */
	FString Place;
	/**
	 * Finishing the step plays a short scene (ACutscene) this long (s): the
	 * player and SceneWith (or Target, when that is empty) talking. 0 for none.
	 */
	float SceneSeconds = 0.f;
	TWeakObjectPtr<AActor> SceneWith;
};

/**
 * A story mission: a list of steps, each with a goal line on screen, a small
 * art deco marker over the target, and on the minimap its diamond and the way there. A
 * mission sets itself up in SetUpMission (people, things, the steps), reacts
 * as steps begin and end, and pays its reward when the last step is done.
 *
 * A mission can fail (Fail): it says why, and offers E to try again from
 * its start (GetRetrySpot); otherwise it starts over by itself a little later.
 *
 * A mission that starts at a ring (bStartsAtRing) waits quietly in a gold
 * ring by whoever gives it (the first step's SceneWith) until the player
 * walks in on foot; then its first step is done at once and its scene plays,
 * ending with the mission's number and name. Steps with SceneSeconds play
 * a short scene when they are done (a mission's last one: being paid).
 *
 * On-screen lines: 7 the goal, 8 what people say, 9 big announcements.
 */
UCLASS(Abstract)
class PESTIALVILAG_API AMission : public AActor
{
	GENERATED_BODY()

public:
	AMission();

	virtual void Tick(float DeltaSeconds) override;

	/** The name the save game remembers the mission by. */
	FName MissionId;

	/** Paid when the mission is passed, in fillér. */
	int32 Reward = 0;

	FString TitleHu;
	FString TitleEn;
	/** Its place in act 1 (1 the prologue), shown with its name when it starts; 0 for none. */
	int32 Number = 0;

	/**
	 * Where the player should head now, and what people call that place
	 * (may be empty); false while there is nowhere to go (the mission is
	 * over or has failed). Used by passers-by who are asked the way.
	 */
	bool GetWayTo(FVector& OutTarget, FString& OutPlace) const;

	/**
	 * While the mission waits in its start ring: where the ring is, and the
	 * flat way out of it away from whoever gives the mission. False otherwise.
	 */
	bool GetStartSpot(FVector& OutAt, FVector& OutAway) const;

protected:
	/** The mark over the target; it moves from step to step on its own. */
	UPROPERTY(VisibleAnywhere, Category = "Mission")
	TObjectPtr<UGoalMarkerComponent> Marker;

	/** Arrows over the turns on the way to the target. */
	UPROPERTY(VisibleAnywhere, Category = "Mission")
	TObjectPtr<URouteGuideComponent> Guide;

	/** Where the mission waits to begin, when it starts at a ring. */
	UPROPERTY(VisibleAnywhere, Category = "Mission")
	TObjectPtr<UStartRingComponent> Ring;

	/**
	 * Set in SetUpMission: the mission waits in a ring by its first step's
	 * SceneWith (or at its Target) and starts when the player walks in;
	 * StartLetter (the giver's first letter) shows over the ring and on the maps.
	 */
	bool bStartsAtRing = false;
	FString StartLetter;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> BaseMaterial;

	TArray<FMissionStep> Steps;

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Spawn what the mission needs and fill Steps. */
	virtual void SetUpMission() {}
	virtual void OnStepStarted(int32 Step) {}
	virtual void OnStepDone(int32 Step) {}
	/** Called each frame while a step runs, before checking whether it is done. */
	virtual void TickStep(int32 Step, float DeltaSeconds) {}

	/** Someone speaks: Hungarian with the English after it. */
	void Say(const FString& Who, const FString& LineHu, const FString& LineEn, float Seconds = 6.f) const;
	/** A big line across the screen. */
	void Announce(const FString& LineHu, const FString& LineEn, float Seconds = 5.f) const;

	/** The player's pawn (on foot or in a car), or null. */
	APawn* GetPlayer() const;
	/** The ground under a map point, or Fallback when nothing is there. */
	double GroundZ(const FVector2D& At, double Fallback = 0.0) const;
	/** Spawn an actor for this mission; it goes when the mission ends. */
	template <class T>
	T* SpawnForMission(const FVector& Location, double Yaw)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		T* Spawned = GetWorld()->SpawnActor<T>(T::StaticClass(), Location, FRotator(0.0, Yaw, 0.0), Params);
		if (Spawned)
		{
			MissionActors.Add(Spawned);
		}
		return Spawned;
	}
	/** An actor spawned some other way (say deferred, to set it up first) that goes when the mission ends. */
	void AddMissionActor(AActor* Actor);
	/** Keep an actor the mission spawned after it ends (for example a car). */
	void KeepAfterMission(AActor* Actor);

	/** Ends the current step, for one the mission decides (bManual). */
	void FinishStep();
	/** Moves the current step's target and its marker. */
	void SetStepTarget(const FVector& At);
	/** The mission is lost: says why, then offers to try again. */
	void Fail(const FString& WhyHu, const FString& WhyEn);
	bool HasFailed() const { return bFailed; }
	/** Where trying again puts the player (on foot), facing Yaw; false to leave him where he is. */
	virtual bool GetRetrySpot(FVector& OutAt, double& OutYaw) const { return false; }
	/** The mission has just failed: stop what it set going. */
	virtual void OnFailed() {}
	/**
	 * A letter the player carries for this mission and can read with R:
	 * handwritten Hungarian (lines split by \n, an empty line between
	 * paragraphs, a line starting with > goes to the right) and the English.
	 * False when there is none.
	 */
	virtual bool GetLetter(FString& OutHu, FString& OutEn) const { return false; }

private:
	void StartStep(int32 Step);
	/** Waiting in the ring: starts the mission when the player walks in. */
	void TickRing();
	/** The scene of a step just done, if it has one. */
	void PlayStepScene(int32 Step);
	void Pass();
	/** After failing: the offer to try again, and starting over. */
	void TickFailed();
	void Retry(bool bPutBack);
	/** R takes the mission's letter out to read, R or Backspace puts it away. */
	void TickLetter();
	void CloseLetter();

	UPROPERTY(Transient)
	TArray<TObjectPtr<AActor>> MissionActors;

	int32 CurrentStep = INDEX_NONE;
	bool bAtRing = false;
	/** The player has been outside the ring since it showed (so standing in it from the start doesn't count). */
	bool bRingArmed = false;
	bool bFailed = false;
	double FailedAt = 0.0;
	bool bLetterOpen = false;
};
