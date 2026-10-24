// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Mission.generated.h"

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
};

/**
 * A story mission: a list of steps, each with a goal line on screen, a tall
 * light over the target and see-through arrows over the turns on the way. A
 * mission sets itself up in SetUpMission (people, things, the steps), reacts
 * as steps begin and end, and pays its reward when the last step is done.
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
	UPROPERTY(VisibleAnywhere, Category = "Mission")
	TObjectPtr<UStaticMeshComponent> Beacon;

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

private:
	void StartStep(int32 Step);
	void Pass();

	UPROPERTY(Transient)
	TArray<TObjectPtr<AActor>> MissionActors;

	int32 CurrentStep = INDEX_NONE;
};
