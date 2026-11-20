// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Mission.h"
#include "MissionDzsokiUtok.generated.h"

class AChaseThief;
class APestiPerson;
class AStoryPerson;
class UStaticMesh;
class UStaticMeshComponent;

/**
 * Dzsoki's first side mission, "Dzsoki ütői" (Dzsoki's hammers). Dzsoki,
 * the cimbalom player of the Fehér Galamb, waits outside his tavern: the
 * hammers he has played with for twenty years were taken from the stage,
 * and the lad who took them still loiters across the street. The lad runs
 * (AChaseThief, along the street to the west and up past the körút);
 * caught, he fights; beaten, he drops the hammers wrapped in a cloth and
 * says a man with a gold tooth paid him, "from Berki Gyuszi". Back at the
 * door Dzsoki swears Gyuszi will pay for it.
 *
 * A side story (bSideStory): AMusicBars starts it, and the next one after it.
 */
UCLASS()
class PESTIALVILAG_API AMissionDzsokiUtok : public AMission
{
	GENERATED_BODY()

public:
	AMissionDzsokiUtok();

	/** The name the save game remembers it by. */
	static constexpr const TCHAR* Id = TEXT("DzsokiUtok");

protected:
	virtual void SetUpMission() override;
	virtual void OnStepStarted(int32 Step) override;
	virtual void OnStepDone(int32 Step) override;
	virtual void TickStep(int32 Step, float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual bool GetRetrySpot(FVector& OutAt, double& OutYaw) const override;
	virtual void OnFailed() override;

private:
	APestiPerson* PlayerOnFoot() const;
	/** The hammers in their cloth on the ground where he dropped them. */
	void DropBundle(const FVector& At);
	void PickUpBundle();

	UPROPERTY(Transient)
	TObjectPtr<AStoryPerson> Dzsoki;

	UPROPERTY(Transient)
	TObjectPtr<AChaseThief> Thief;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> CubeMesh;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> Bundle;

	FVector GiverAt = FVector::ZeroVector;
	double GiverYaw = 0.0;
	FVector BundleAt = FVector::ZeroVector;

	/** The chase: whether he is off yet, since when, how long the chaser has been far behind. */
	bool bRunning = false;
	double ChaseFrom = 0.0;
	double RunAt = 0.0;
	double FarSince = -1.0;
	bool bTaunted = false;
};
