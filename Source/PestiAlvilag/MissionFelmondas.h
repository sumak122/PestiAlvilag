// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Mission.h"
#include "Ruffian.h"
#include "MissionFelmondas.generated.h"

class APestiPerson;
class AStoryPerson;

/**
 * Act 1, mission 5, "A felmondás" (The notice), after a real newspaper
 * story of the 1930s ("Kereskedők véres verekedése a Garay-utcában"):
 * Pista bácsi sends the player to his old friend Schwimmer Mihály, a
 * timber merchant on Garay utca. His landlord, the wholesaler Friedrich
 * Hermann, has given him notice to leave the yard. Standing in for
 * Schwimmer's brother-in-law Erdős, the player waits with him outside
 * Hermann's shop until noon, goes in and hits the old man, fights his son
 * Géza and the two clerks, gets away from the police and meets Schwimmer
 * again at the Teleki tér market. The next day's headline closes it.
 *
 * Failing (fainting, being caught, walking out of the fight) tries again
 * from the pavement outside the shop, not from Pista bácsi.
 */
UCLASS()
class PESTIALVILAG_API AMissionFelmondas : public AMission
{
	GENERATED_BODY()

public:
	AMissionFelmondas();

protected:
	virtual void SetUpMission() override;
	virtual void OnStepStarted(int32 Step) override;
	virtual void OnStepDone(int32 Step) override;
	virtual void TickStep(int32 Step, float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual bool GetRetrySpot(FVector& OutAt, double& OutYaw) const override;
	virtual void OnFailed() override;

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
	/** A point of the yard (GarayModel's frame) in the city, at its ground. */
	FVector YardPoint(const FVector& Local) const;
	/** Hermann, Géza and the clerks, at their places in the shop. */
	void SpawnShopPeople();
	/** The next day's paper over the screen, and putting it away. */
	void ShowClipping();
	void HideClipping();
	ARuffian* GetShopPerson(ERuffian Kind) const;
	/** The old man squares up to him once he walks up to the counter. */
	void TickHermann();
	/** Géza and the clerks go for him, two at a time. */
	void TickFight();

	UPROPERTY(Transient)
	TObjectPtr<AStoryPerson> Pista;

	UPROPERTY(Transient)
	TObjectPtr<AStoryPerson> Schwimmer;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ARuffian>> ShopPeople;

	TArray<FLater> Lines;
	FTransform YardFrame;
	FVector PistaAt = FVector::ZeroVector;
	FVector DoorOutside = FVector::ZeroVector;
	FVector MeetAt = FVector::ZeroVector;
	/** Trying again after failing in the shop: the first two steps go by at once. */
	bool bResuming = false;
	/** The furthest step begun, for where trying again starts. */
	int32 Reached = 0;
	/** When he got to the shop door, waiting for noon (s), or below 0. */
	double WaitingSince = -1.0;
	bool bHermannUp = false;
	/** When the last of them went down (s), or below 0. */
	double WonAt = -1.0;
	/** The police's arrests when the getaway began. */
	int32 ArrestsBefore = 0;
	/** When the headline went up (s), or below 0. */
	double HeadlineAt = -1.0;
	double ThanksAt = -1.0;
};
