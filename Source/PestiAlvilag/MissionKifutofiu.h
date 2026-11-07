// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DeliveryModel.h"
#include "Mission.h"
#include "MissionKifutofiu.generated.h"

class ABicycle;
class AStoryPerson;
class UProceduralMeshComponent;

/**
 * Act 1, mission 6, "Kifutófiú" (The errand boy): Varga Lajos, the grocer
 * on Népszínház utca, needs an errand boy. He puts three parcels in the
 * basket of his shop bicycle, and the player rides them against the clock
 * to the Oppenheim café, a lady traveller at the Keleti taxi stand and
 * Dr. Halász's maid at Népszínház utca 7, then comes back for his pay.
 * Mazsola, the errand boys' leader, waits at the shop with his own bicycle.
 *
 * Each parcel is handed over on foot (E at the door) with the bicycle
 * close by. Riding hard into something throws a parcel out of the basket:
 * it lies in the road until he picks it up again (E). Out of time, he tries
 * again from the shop. Varga lets him keep the old bicycle.
 */
UCLASS()
class PESTIALVILAG_API AMissionKifutofiu : public AMission
{
	GENERATED_BODY()

public:
	AMissionKifutofiu();

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
	/** A point of a place's frame in the city, on that place's ground. */
	FVector SpotAt(DeliveryModel::EPlace Spot, const FVector& Local) const;
	double SpotYaw(DeliveryModel::EPlace Spot, double LocalYaw) const;
	AStoryPerson* SpawnPerson(DeliveryModel::EPlace Spot, const FVector& Local, double LocalYaw, const PersonKit::FOutfit& Outfit);
	ABicycle* SpawnBike(DeliveryModel::EPlace Spot, const FVector& Local, double LocalYaw);
	/** A shape of the mission's own, standing at At turned by Yaw. */
	UProceduralMeshComponent* MakeMesh(const PersonKit::FPiece& Piece, const FVector& At, double Yaw);
	/** The clock while the parcels are out; out of time fails the mission. */
	void TickClock(float DeltaSeconds);
	/** A hard knock throws a parcel out; E beside it puts it back in the basket. True when E was used here. */
	bool TickFallenParcel();
	/** E at the door of a delivery step, with the bicycle near: one parcel handed over. */
	void TickDelivery(int32 Step);
	/** A short note under the goal. */
	void Hint(const FString& Text) const;

	UPROPERTY(Transient)
	TObjectPtr<AStoryPerson> Varga;

	UPROPERTY(Transient)
	TObjectPtr<AStoryPerson> Mazsola;

	/** Who takes each parcel: the waiter, the lady, the maid. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<AStoryPerson>> Receivers;

	/** Varga's shop bicycle the player rides, and Mazsola's own. */
	UPROPERTY(Transient)
	TObjectPtr<ABicycle> Bike;

	UPROPERTY(Transient)
	TObjectPtr<ABicycle> MazsolaBike;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UProceduralMeshComponent>> Scenery;

	/** A parcel thrown out of the basket, lying in the road (hidden when there is none). */
	UPROPERTY(Transient)
	TObjectPtr<UProceduralMeshComponent> FallenParcel;

	TArray<FLater> Lines;
	/** Each place's ground height (by DeliveryModel::EPlace). */
	TArray<double> SpotGround;
	FVector VargaAt = FVector::ZeroVector;
	/** Seconds left on the clock, or below 0 while it is not running. */
	float TimeLeft = -1.f;
	/** The bicycle's hard knocks already seen. */
	int32 KnocksSeen = 0;
	bool bParcelDown = false;
	/** Varga gave him the bicycle: it stays when the mission ends. */
	bool bKeepBike = false;
};
