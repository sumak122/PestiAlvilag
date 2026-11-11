// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PoliceChase.generated.h"

class AShapeSet;
class APoliceman;
class APestiCar;
class ATrafficManager;

/**
 * The police's own places in a chase (PoliceStationModel.h):
 * - the VIII. district police station's front door on Víg utca, with two
 *   constables on guard. After an arrest the player walks out of it (see
 *   APestiAlvilagGameModeBase::Busted);
 * - road blocks at three stars, while the player drives: wooden barriers
 *   across a street ahead of him, out of his sight, with two constables.
 *   Driving into a barrier knocks it flying (and dents the car); traffic
 *   stops in front of one (BlockPoints). The constables see him and arrest
 *   him if he stops there. Blocks are taken away once he is far off or no
 *   longer wanted.
 */
UCLASS()
class PESTIALVILAG_API APoliceChase : public AActor
{
	GENERATED_BODY()

public:
	APoliceChase();

	virtual void Tick(float DeltaSeconds) override;

	static APoliceChase* Get(const UWorld* World);

	/** Where the player comes out after a night in the cells (on the ground), and the way he faces. */
	bool GetLetOut(FVector& OutAt, double& OutYaw) const;

	/** The barriers of the road blocks still standing (traffic stops in front of them). */
	static const TArray<FVector>& BlockPoints();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	struct FBarrier
	{
		TWeakObjectPtr<AShapeSet> Set;
		double GroundZ = 0.0;
		bool bKnocked = false;
		bool bLanded = false;
		FVector Velocity = FVector::ZeroVector;
		FRotator Spin = FRotator::ZeroRotator;
	};

	struct FBlock
	{
		TArray<FBarrier> Barriers;
		TArray<TWeakObjectPtr<APoliceman>> Men;
		TArray<FVector> MenAt;
		/** The road's middle at the block, and the way the traffic it stops comes from. */
		FVector Middle = FVector::ZeroVector;
		FVector Facing = FVector::ForwardVector;
		double Born = 0.0;
	};

	void BuildStation();
	void TickBlocks(float DeltaSeconds);
	bool TryPlaceBlock(const APawn* Player);
	void KnockBarriers(FBlock& Block, APestiCar* Car);
	void FlyBarriers(FBlock& Block, float DeltaSeconds);
	void ClearBlock(FBlock& Block);
	bool GroundAt(const FVector& Near, double& OutZ) const;
	ATrafficManager* FindTraffic();

	TArray<FBlock> Blocks;
	TWeakObjectPtr<ATrafficManager> Traffic;
	double NextBlockAt = 0.0;
	float SightClock = 0.f;
	FRandomStream Looks;

	/** The station's frame: the door's foot on the pavement, and the way out to the street. */
	FVector StationAt = FVector::ZeroVector;
	double StationYaw = 0.0;
	bool bStationBuilt = false;
	TArray<TWeakObjectPtr<APoliceman>> Sentries;
	TArray<FVector> SentryAt;
};
