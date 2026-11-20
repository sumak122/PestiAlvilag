// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Mission.h"
#include "MissionHirdetes.generated.h"

class AStoryPerson;

/**
 * The car show chain, mission 1, "A hirdetés" (The advert): with all four
 * of Laci's night races won, Laci shows the player the paper: the black
 * Mercedes SSK is on show in the Tattersall on Kerepesi út (ATattersall).
 * Its doorman lets nobody in in street clothes, so first a suit from
 * Kertész the tailor across from the garage (ATailor; skipped when he has
 * one). By day (CarShowModel::OpenFrom to OpenUntil) he asks Mr. Weisz the
 * salesman about the car; Kálmán, the importer's chauffeur, comes in for
 * the spare key while they talk, and the player overhears where he drinks
 * (Bauer's tavern on Baross tér). Then a look at the side door the night
 * watchman keeps, and back to Laci. Nothing to fail.
 */
UCLASS()
class PESTIALVILAG_API AMissionHirdetes : public AMission
{
	GENERATED_BODY()

public:
	AMissionHirdetes();

protected:
	virtual void SetUpMission() override;
	virtual void OnStepStarted(int32 Step) override;
	virtual void OnStepDone(int32 Step) override;
	virtual void TickStep(int32 Step, float DeltaSeconds) override;
	virtual bool GetRetrySpot(FVector& OutAt, double& OutYaw) const override;

private:
	/** A line someone says a few seconds from now. */
	struct FLater
	{
		double At;
		FString Who;
		FString Hu;
		FString En;
	};

	void SayLater(float Seconds, const FString& Who, const FString& LineHu, const FString& LineEn);
	/** A short note under the goal. */
	void Hint(const FString& Text) const;
	/** "The show is shut" every so often, while it is. */
	void HintShut();
	/** Mr. Weisz in the hall, or null. */
	AStoryPerson* FindWeisz() const;
	/** Kálmán walks in to Weisz, gets the key, says where he drinks and goes. */
	void TickChauffeur();

	UPROPERTY(Transient)
	TObjectPtr<AStoryPerson> Laci;

	UPROPERTY(Transient)
	TObjectPtr<AStoryPerson> Kalman;

	/** Where Kálmán walks off to when he has the key. */
	UPROPERTY(Transient)
	TObjectPtr<AActor> WayOut;

	TArray<FLater> Lines;
	/** When the "shut" note was last shown (s). */
	double LastShutHint = -100.0;
	/** When Kálmán came in (s); when his talk with Weisz is over (s, below 0 before it starts). */
	double KalmanSince = 0.0;
	double TalkOverAt = -1.0;
};
