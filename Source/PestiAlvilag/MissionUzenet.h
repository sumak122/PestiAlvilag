// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Mission.h"
#include "MissionUzenet.generated.h"

class AStoryPerson;
class APestiPerson;
class UStaticMesh;

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
	/** The letter goes in his coat's breast pocket, its top showing. */
	void PocketLetter();
	/** The parcel, tied with string and sealed, held out in front in both hands. */
	void HoldParcel();
	/** Whatever he has goes (to Pista bácsi or Bözsi néni). */
	void PutAway();
	/** A box shape of what he has, on the holder (cm, in its frame). */
	void AddPart(const FLinearColor& Color, const FVector& At, const FVector& Size, const FRotator& Turn = FRotator::ZeroRotator);
	/** Where the holder goes on him: the parcel sits on his lap when he rides. */
	void PlaceHolder();

	UPROPERTY(Transient)
	TObjectPtr<AStoryPerson> Bozsi;

	UPROPERTY(Transient)
	TObjectPtr<AStoryPerson> Pista;

	/** Bözsi néni at her market stall, away while she is at home. */
	UPROPERTY(Transient)
	TObjectPtr<AStoryPerson> BozsiAtStall;

	UPROPERTY(Transient)
	TObjectPtr<APestiPerson> Carrier;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> CubeMesh;

	/** What he has, on him: the holder and its parts. */
	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> Holder;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> Parts;

	bool bParcel = false;

	TArray<FLater> Lines;
};
