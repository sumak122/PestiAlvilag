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
 * The player goes there by day (CarShowModel::OpenFrom to OpenUntil), asks
 * Mr. Weisz the salesman about the car (two keys: one in the office
 * cabinet, one with the importer's chauffeur, Kálmán, who drinks at the
 * Bauer tavern on Baross tér), looks at the side door the night watchman
 * keeps, and tells Laci. Nothing to fail.
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

	UPROPERTY(Transient)
	TObjectPtr<AStoryPerson> Laci;

	TArray<FLater> Lines;
	/** When the "shut" note was last shown (s). */
	double LastShutHint = -100.0;
};
