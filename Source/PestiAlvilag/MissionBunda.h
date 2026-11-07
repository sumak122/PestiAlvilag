// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Mission.h"
#include "MissionBunda.generated.h"

class APestiPerson;
class APoliceman;
class AShapeSet;
class AStoryPerson;
class UPoliceWhistle;

/**
 * Act 1, mission 8, "A bunda" (The fur coat), and the end of act 1.
 *
 * Bozsi neni has a fine lady who wants a fur coat cheap. The player takes
 * one off the table outside Friedmann's furrier's shop on Rakoczi ut (one
 * star), shakes off the police and finds a Frommer pistol in its pocket,
 * and brings it to Bozsi, who sells it at her door. The lady is Kardosne,
 * the police inspector's wife, and the coat is her own, left with the
 * furrier for the summer.
 *
 * That night, up in Bozsi's flat, the police raid the house: the player
 * runs along the courtyard balcony, over the wash-house roof, down into
 * the back yard and out through the small back door onto Tisza Kalman ter
 * (two stars), and shakes them off. Next morning the paper has Bozsi's
 * arrest; the telephone in the kiosk on the square rings, and the
 * mysterious employer sends him to an attic room on Berkocsis utca, his
 * home from now on.
 *
 * Caught in the raid, or arrested after it, he tries again from the raid:
 * the mission then waits in a ring at the house's gateway.
 */
UCLASS()
class PESTIALVILAG_API AMissionBunda : public AMission
{
	GENERATED_BODY()

public:
	AMissionBunda();

protected:
	virtual void SetUpMission() override;
	virtual void OnStepStarted(int32 Step) override;
	virtual void OnStepDone(int32 Step) override;
	virtual void TickStep(int32 Step, float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual bool GetRetrySpot(FVector& OutAt, double& OutYaw) const override;
	virtual void OnFailed() override;

private:
	/** What each step is (the steps differ when trying the raid again). */
	enum class EKind : uint8 { Talk, Grab, LoseCoat, Deliver, Night, Escape, LoseRaid, Phone, Room };

	/** A line someone says a few seconds from now. */
	struct FLater
	{
		double At;
		FString Who;
		FString Hu;
		FString En;
	};

	/** A constable in the raid: where he is on his way (a point of the house's frame), and from when he runs. */
	struct FRaider
	{
		TWeakObjectPtr<APoliceman> Man;
		int32 Next = 0;
		double StartAt = 0.0;
		bool bGuard = false;
	};

	void SayLater(float Seconds, const FString& Who, const FString& LineHu, const FString& LineEn);
	void Hint(const FString& Text) const;
	/** A point of Bozsi's house in the city (its own frame, Z from the pavement). */
	FVector HouseAt(const FVector& Local) const;
	/** A point of the furrier's shop in the city. */
	FVector ShopAt(const FVector& Local) const;
	/** Puts the player there on foot, the picture coming up out of black. */
	void PutPlayer(const FVector& At, double Yaw);
	/** The coat over the player's arms, or gone. */
	void HoldCoat(bool bHold);
	void PlaceCoat();
	/** The raid begins: night, Bozsi at her door, the police in the gateway. */
	void StartRaid();
	/** The constables after the player; true if one has caught him. */
	bool TickRaiders(float DeltaSeconds);
	void EndRaid();
	/** True once he has been arrested since the police came after him. */
	bool WasArrested() const;
	void ShowPaper(bool bShow);
	EKind KindOf(int32 Step) const { return Kinds.IsValidIndex(Step) ? Kinds[Step] : EKind::Room; }

	UPROPERTY(Transient)
	TObjectPtr<AStoryPerson> Bozsi;

	/** Bozsi neni at her flea market stall, hidden while she is at home. */
	UPROPERTY(Transient)
	TObjectPtr<AStoryPerson> BozsiAtStall;

	UPROPERTY(Transient)
	TObjectPtr<AStoryPerson> Furrier;

	UPROPERTY(Transient)
	TObjectPtr<AStoryPerson> Lady;

	/** The coat folded on the furrier's table, and the one in the player's arms. */
	UPROPERTY(Transient)
	TObjectPtr<AShapeSet> TableCoat;

	UPROPERTY(Transient)
	TObjectPtr<AShapeSet> CarriedCoat;

	UPROPERTY(Transient)
	TObjectPtr<APestiPerson> Carrier;

	UPROPERTY(Transient)
	TObjectPtr<AShapeSet> Kiosk;

	UPROPERTY(Transient)
	TObjectPtr<UPoliceWhistle> Whistle;

	TArray<EKind> Kinds;
	TArray<FRaider> Raiders;
	TArray<FLater> Lines;
	/** The furthest step kind reached. */
	EKind Reached = EKind::Talk;
	bool bResuming = false;
	/** The pavement's height at the house and at the shop. */
	double HouseGround = 0.0;
	double ShopGround = 0.0;
	/** Where Bozsi stands at her street door, and which way she faces. */
	FVector BozsiStreet = FVector::ZeroVector;
	double BozsiYaw = 0.0;
	/** The chase in the courtyard waits for the scene to end. */
	bool bChaseOn = false;
	int32 ArrestsAtStart = 0;
	/** When the newspaper goes away, and when the lady and Bozsi leave the street door. */
	double PaperUntil = -1.0;
	double LadyGoesAt = -1.0;
};
