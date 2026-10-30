// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Mission.h"
#include "MissionZsebtolvaj.generated.h"

class AChaseThief;
class APestiPerson;
class AStoryPerson;
class UStaticMesh;
class UStaticMeshComponent;

/**
 * Act 1, mission 3, "Zsebtolvaj" (The pickpocket). Bözsi néni is shopping
 * in the Rákóczi tér market hall. While she talks to the player a young
 * man in a flat cap sidles up, snatches her purse and runs: down the nave,
 * out of the side door, along Vásár utca and round the front, over the
 * deliveries and through the garden past the fountain (ChaseModel's route,
 * over and under the things in it). Caught, he is brought down and fights
 * back with his fists (Brawl); beaten, he drops the purse and slinks off,
 * and the player takes it back to her. If he gets away, or the player is
 * knocked out, the mission fails and can be tried again from the hall.
 * Teaches the chase moves and the fist fight.
 *
 * While the mission runs Bözsi néni is at the hall, not at her flea market stall.
 */
UCLASS()
class PESTIALVILAG_API AMissionZsebtolvaj : public AMission
{
	GENERATED_BODY()

public:
	AMissionZsebtolvaj();

protected:
	virtual void SetUpMission() override;
	virtual void OnStepStarted(int32 Step) override;
	virtual void OnStepDone(int32 Step) override;
	virtual void TickStep(int32 Step, float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual bool GetRetrySpot(FVector& OutAt, double& OutYaw) const override;
	virtual void OnFailed() override;

private:
	/** A point in the market hall's frame (cm), on the ground in the city. */
	FVector HallPoint(const FVector2D& Local) const;
	/** The player on foot, or null. */
	APestiPerson* PlayerOnFoot() const;
	/** The purse on the ground where he dropped it. */
	void DropPurse(const FVector& At);
	void PickUpPurse();

	UPROPERTY(Transient)
	TObjectPtr<AStoryPerson> Bozsi;

	/** Bözsi néni at her flea market stall, away while she shops at the hall. */
	UPROPERTY(Transient)
	TObjectPtr<AStoryPerson> BozsiAtStall;

	UPROPERTY(Transient)
	TObjectPtr<AChaseThief> Thief;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> CubeMesh;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> Purse;

	FVector PurseAt = FVector::ZeroVector;

	/** The chase: he sneaks up first, then runs; when he started, and how long the chaser has been far behind. */
	bool bSnatched = false;
	double SnatchAt = 0.0;
	double SneakFrom = 0.0;
	double FarSince = -1.0;
	bool bTaunted = false;
};
