// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Mission.generated.h"

class UGoalMarkerComponent;
class UMaterialInterface;
class URouteGuideComponent;
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
};

/**
 * A story mission: a list of steps, each with a goal line on screen, a small
 * art deco marker over the target and see-through arrows over the turns on the way. A
 * mission sets itself up in SetUpMission (people, things, the steps), reacts
 * as steps begin and end, and pays its reward when the last step is done.
 *
 * A mission can fail (Fail): it says why, and offers E to try again from
 * its start (GetRetrySpot); otherwise it starts over by itself a little later.
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

protected:
	/** The mark over the target; it moves from step to step on its own. */
	UPROPERTY(VisibleAnywhere, Category = "Mission")
	TObjectPtr<UGoalMarkerComponent> Marker;

	/** Arrows over the turns on the way to the target. */
	UPROPERTY(VisibleAnywhere, Category = "Mission")
	TObjectPtr<URouteGuideComponent> Guide;

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

private:
	void StartStep(int32 Step);
	void Pass();
	/** After failing: the offer to try again, and starting over. */
	void TickFailed();
	void Retry(bool bPutBack);

	UPROPERTY(Transient)
	TArray<TObjectPtr<AActor>> MissionActors;

	int32 CurrentStep = INDEX_NONE;
	bool bFailed = false;
	double FailedAt = 0.0;
};
