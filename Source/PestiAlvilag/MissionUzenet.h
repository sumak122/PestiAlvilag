// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Mission.h"
#include "MissionUzenet.generated.h"

class AStoryPerson;
class APestiPerson;

/**
 * Act 1, mission 2, "Üzenet" (The message). Bözsi néni waits on the pavement
 * beside her door with a sealed letter for Pista bácsi, the junk dealer at
 * the Teleki tér flea market. He reads it and hands over a wrapped parcel
 * that is not to be looked into, and the player brings it back to her. The
 * tram stop is right outside the house, so the tram is the quick way.
 * Teaches talking to people and riding the tram to get somewhere.
 *
 * While the mission runs Bözsi néni is at home, not at her market stall.
 */
UCLASS()
class PESTIALVILAG_API AMissionUzenet : public AMission
{
	GENERATED_BODY()

public:
	AMissionUzenet();

protected:
	/** The letter, then the parcel, in the player's hand. */
	UPROPERTY(VisibleAnywhere, Category = "Mission")
	TObjectPtr<UStaticMeshComponent> InHand;

	virtual void SetUpMission() override;
	virtual void OnStepStarted(int32 Step) override;
	virtual void OnStepDone(int32 Step) override;
	virtual void TickStep(int32 Step, float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

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
	/** Puts the letter or the parcel in his hand. */
	void Hold(const FVector& Size, const FLinearColor& Color);

	UPROPERTY(Transient)
	TObjectPtr<AStoryPerson> Bozsi;

	UPROPERTY(Transient)
	TObjectPtr<AStoryPerson> Pista;

	/** Bözsi néni at her market stall, away while she is at home. */
	UPROPERTY(Transient)
	TObjectPtr<AStoryPerson> BozsiAtStall;

	UPROPERTY(Transient)
	TObjectPtr<APestiPerson> Carrier;

	TArray<FLater> Lines;
};
