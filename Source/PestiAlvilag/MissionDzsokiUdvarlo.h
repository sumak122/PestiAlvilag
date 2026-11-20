// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Mission.h"
#include "MissionDzsokiUdvarlo.generated.h"

class APestiPerson;
class ARuffian;
class AStoryPerson;

/**
 * Dzsoki's second side mission, "Az udvarló" (The suitor), in the evening.
 * Rózsi has gone out walking with Berki Gyuszi, the young violinist of the
 * Rózsa kávéház, and Dzsoki sends the player after them: follow them, but
 * not so close that they see him. Halfway to the körút three hired toughs
 * stop the pair; Gyuszi stands in front of Rózsi and the player fights them.
 * Beaten, their leader lets slip that a man with a gold tooth paid them, the
 * same who paid for the hammers. The pair walk on to the Rózsa, and back at
 * the tavern Dzsoki thinks again about the boy.
 *
 * A side story (bSideStory): AMusicBars starts it after "Dzsoki ütői".
 */
UCLASS()
class PESTIALVILAG_API AMissionDzsokiUdvarlo : public AMission
{
	GENERATED_BODY()

public:
	AMissionDzsokiUdvarlo();

	/** The name the save game remembers it by. */
	static constexpr const TCHAR* Id = TEXT("DzsokiUdvarlo");

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
	/** A point this far along the pair's way (cm), and which way the way runs there. */
	FVector2D PointAlong(double Distance, FVector2D& OutAhead) const;
	/** The pair walk on along their way (their pacers go ahead of them). */
	void WalkPair(float DeltaSeconds);
	/** Following: lost them, or too close and seen. */
	void TickFollow();
	void SpawnToughs();
	void TickFight();

	UPROPERTY(Transient)
	TObjectPtr<AStoryPerson> Dzsoki;
	UPROPERTY(Transient)
	TObjectPtr<AStoryPerson> Gyuszi;
	UPROPERTY(Transient)
	TObjectPtr<AStoryPerson> Rozsi;
	/** Unseen walkers a step ahead of each of them, for them to follow. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<AStoryPerson>> Pacers;
	UPROPERTY(Transient)
	TArray<TObjectPtr<ARuffian>> Toughs;

	TArray<FVector2D> Way;
	double WayLength = 0.0;
	/** How far along their way the pair are (cm). */
	double Along = 0.0;
	bool bPairGone = false;

	FVector GiverAt = FVector::ZeroVector;
	double GiverYaw = 0.0;

	double FarSince = -1.0;
	double NearSince = -1.0;
	double WarnedAt = -100.0;
	bool bFightOn = false;
	double WonAt = -1.0;
};
