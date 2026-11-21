// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HandCartModel.h"
#include "PestiCar.h"
#include "HandCart.generated.h"

class APedestrian;
class ATrafficManager;
class UMaterialInterface;
class UProceduralMeshComponent;

/**
 * A porter's hand cart (HandCartModel): a red-capped porter pushes it along
 * the side of the road at a walk, with a traveller's luggage, a family's
 * moving or sacks on it (AHandCarts sends one now and then, rarely). Cars
 * go past it; one that hits it hard knocks the load off onto the road and
 * the porter shouts and shakes his fist. F takes it: the porter runs after
 * the player shouting, and it is an APestiCar pushed by hand (no engine,
 * slow), the player walking behind it at the handles.
 */
UCLASS()
class PESTIALVILAG_API AHandCart : public APestiCar
{
	GENERATED_BODY()

public:
	AHandCart();

	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** What it carries. Before spawning. */
	void SetLoad(HandCartModel::ELoad InLoad) { Load = InLoad; }

	/** Called right after spawning: the porter pushes it along this lane at the kerb side, from this point. */
	void StartInTraffic(ATrafficManager* InManager, int32 InLaneIndex, int32 InPointIndex);

	/** Still pushed by its porter (not taken by the player). */
	bool IsInTraffic() const { return Manager != nullptr; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UProceduralMeshComponent>> Parts;

	/** The porter behind it in the traffic; the player's figure at the handles while he pushes it. */
	UPROPERTY(Transient)
	TObjectPtr<APedestrian> Porter;
	UPROPERTY(Transient)
	TObjectPtr<APedestrian> Pusher;

	UPROPERTY(Transient)
	TObjectPtr<ATrafficManager> Manager;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> BaseMaterial;

private:
	/** Along the lane at the kerb side, at a walk, stopping for what is in front. */
	void FollowLane(float DeltaSeconds);
	/** Where to walk for a point of the current lane: moved off it towards the kerb, short of any wall. */
	FVector KerbSpot(int32 Point) const;
	/** The player bumping into it: a hard knock tips the load off. */
	void CheckBumps();
	void KnockLoadOff(const FVector& From);
	/** Taken by the player: out of the traffic, the porter runs after him shouting. */
	void LeaveTraffic();
	/** Far off and out of sight: gone. */
	bool TickAway(float DeltaSeconds);
	void SnapDown();
	/** Rolls the wheels by how far it went, and puts the porter or the player at the handles. */
	void Animate(float DeltaSeconds, float Moved);
	/** The porter's (or the player's) middle at the handles, and the pose of his arms pushing. */
	FVector HandlesSpot() const;
	/** A line from the porter, if the player is near. */
	void CallOut(const FString& Text) const;

	HandCartModel::ELoad Load = HandCartModel::ELoad::Luggage;
	int32 PorterPick = 0;
	int32 LaneIndex = INDEX_NONE;
	int32 NextLaneIndex = INDEX_NONE;
	int32 PointIndex = 0;
	FVector Target = FVector::ZeroVector;
	/** The porter's walking pace now and at his ease (cm/s). */
	float WalkSpeed = 0.f;
	float Cruise = 120.f;
	/** The ground under the box's middle, in the box's frame (cm). */
	double FloorZ = -90.0;
	float WheelTurn = 0.f;
	float StuckTime = 0.f;
	float SqueezeTime = 0.f;
	float BumpCooldown = 0.f;
	/** Seconds left of standing shaking his fist after the load went over. */
	float AngryTime = 0.f;
	bool bLoadOff = false;
	float AwayClock = 0.f;
	FName DrivenProfile;
};
