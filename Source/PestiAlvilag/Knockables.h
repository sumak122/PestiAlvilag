// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KnockModel.h"
#include "Knockables.generated.h"

class APedestrian;
class FJsonObject;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UProceduralMeshComponent;
class UStaticMesh;
class UStaticMeshComponent;

/**
 * Loose things set out on the pavements and at the Teleki tér market that
 * the player can knock over on foot or with the car (KnockModel.h): crates
 * of apples, potatoes and cabbages by the greengrocers and grocers, beer
 * barrels by the taverns, milk churns by the dairies, bread baskets by the
 * bakers, tin dustbins by the house gates, and crates and baskets beside
 * the market stalls.
 *
 * Each is a small physics body. People and cars do not bump into them in
 * the physics sense (so the player is never stopped dead by a crate): each
 * frame they are pushed out of the way of the player or the player's car,
 * harder the faster it goes, and fly, roll and tumble from there. A crate
 * that goes over spills its goods as loose lumps. Things that have been
 * moved go back where they stood once the camera is far away and they are
 * out of sight. Nearer by, now and then a street sweeper comes along with
 * his broom (TickSweeper): sweeps up the spilt goods, carries the thing back
 * to its place and walks off again.
 */
UCLASS()
class PESTIALVILAG_API AKnockables : public AActor
{
	GENERATED_BODY()

public:
	AKnockables();

	virtual void Tick(float DeltaSeconds) override;

	/** The furniture file with the shops, gates and market stalls, relative to the project folder. */
	UPROPERTY(EditAnywhere, Category = "Knockables")
	FString DataFile = TEXT("Data/OsmGraybox/street_furniture.json");

	/** Things moved from their places go back once the camera is this far away (cm)... */
	UPROPERTY(EditAnywhere, Category = "Knockables")
	float ResetDistance = 15000.f;

	/** ...and at least this long after they were last moved (s). */
	UPROPERTY(EditAnywhere, Category = "Knockables")
	float ResetAfter = 60.f;

	/** They are not drawn beyond this distance (cm). */
	UPROPERTY(EditAnywhere, Category = "Knockables")
	float CullDistance = 9000.f;

protected:
	virtual void BeginPlay() override;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> BaseMaterial;

	UPROPERTY()
	TObjectPtr<UStaticMesh> SphereMesh;

private:
	struct FItem
	{
		UProceduralMeshComponent* Mesh = nullptr;
		KnockModel::EKind Kind = KnockModel::EKind::Crate;
		FTransform Home;
		/** The mesh sections that are the goods on top (hidden once spilled). */
		int32 FirstLoad = INDEX_NONE;
		int32 Loads = 0;
		bool bSpilled = false;
		bool bMoved = false;
		/** When it was last knocked (game seconds). */
		double MovedAt = 0.0;
		/** The car slows down once for each thing it hits, not every frame it touches it. */
		float LastHit = -10.f;
		TArray<UStaticMeshComponent*> Lumps;
		/** The sweeper has it in his arms: nobody else pushes it. */
		bool bCarried = false;
	};

	/** What the street sweeper is doing. */
	enum class ESweep : uint8 { None, Coming, Sweeping, Carrying, Leaving };
	/** Sends a sweeper to a thing knocked over near the player, and walks him through the job. */
	void TickSweeper(float DeltaSeconds, const FVector& PlayerAt, const FVector& Camera);
	/** A clear spot on the pavement out of the way to come from (and go back to), or false. */
	bool FindSweeperStart(const FItem& Item, const FVector& Camera, FVector& OutStart) const;
	/** Walks him a step towards Target (ground height kept); true once he is there. */
	bool SweeperStep(APedestrian* Man, const FVector& Target, float DeltaSeconds);
	void SendSweeperOff();

	bool LoadPlaces(TArray<TPair<FTransform, KnockModel::EKind>>& OutPlaces) const;
	/** Adds one thing standing at At (its bottom), turned to Yaw, unless something solid is in the way. */
	void AddItem(KnockModel::EKind Kind, const FVector& At, double Yaw);
	void Push(FItem& Item, const FVector& Direction, float Speed, float Lift, float Spin);
	void Spill(FItem& Item);
	void PutBack(FItem& Item);
	UMaterialInstanceDynamic* Paint(const FLinearColor& Color);
	bool GroundAt(const FVector2D& At, double Near, FVector& OutGround) const;

	/** One built model per kind (every crate of apples looks the same). */
	TMap<int32, KnockModel::FThing> Models;
	TMap<FString, UMaterialInstanceDynamic*> Paints;
	TArray<FItem> Items;

	/** Where the player (or his car) was last frame, to know how fast it goes. */
	FVector LastPusherAt = FVector::ZeroVector;
	TWeakObjectPtr<AActor> LastPusher;
	FVector PusherVelocity = FVector::ZeroVector;
	float ResetClock = 0.f;
	int32 ResetNext = 0;

	UPROPERTY(Transient)
	TObjectPtr<APedestrian> Sweeper;
	ESweep SweepState = ESweep::None;
	int32 SweepItem = INDEX_NONE;
	/** Seconds in this step, and in the whole job. */
	float SweepClock = 0.f;
	float SweepJob = 0.f;
	/** Seconds before looking for the next job. */
	float SweepWait = 20.f;
	FVector SweepFrom = FVector::ZeroVector;
	float SweeperGroundZ = 0.f;
	FRandomStream Random;
};
