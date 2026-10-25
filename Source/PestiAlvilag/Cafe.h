// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StoryPerson.h"
#include "Cafe.generated.h"

class UInstancedStaticMeshComponent;
class UMaterialInterface;
class UStaticMesh;
class UTextRenderComponent;

/**
 * The little coffee house on the ground floor of Bözsi néni's tenement on
 * Népszínház utca, beside her street door: a dark green shop front with lit
 * windows and a gilt sign, a red and cream striped awning, and a terrace of
 * round marble-topped tables and bentwood chairs under parasols on the
 * pavement, with a few guests and a waiter going between the tables.
 *
 * It stands where the home door stands (same place and facing: +X out to
 * the street) and runs along the wall to the door's right (+Y).
 */
UCLASS()
class PESTIALVILAG_API ACafe : public AActor
{
	GENERATED_BODY()

public:
	ACafe();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Cafe")
	TObjectPtr<UTextRenderComponent> SignText;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> BaseMaterial;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMesh>> ShapeMeshes;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UInstancedStaticMeshComponent>> Batches;

private:
	/** One shape of the café (cm, in the café's own frame). */
	void Add(int32 Shape, const FLinearColor& Color, const FVector& At, const FVector& Scale,
		const FRotator& Turn = FRotator::ZeroRotator, bool bCollide = false);
	void BuildFront();
	void BuildTerrace();
	void SeatGuests();

	TMap<FString, int32> BatchIndex;
};

/** A guest at a café table, sitting with a cup and now and then taking a sip. */
UCLASS()
class PESTIALVILAG_API ACafeGuest : public AStoryPerson
{
	GENERATED_BODY()

public:
	ACafeGuest();

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

private:
	void PoseArms();

	float SipClock = 0.f;
};

/**
 * The waiter: white jacket, black trousers, a tray in hand. He comes out of
 * the café door, goes to a table, waits there a moment and goes back in,
 * one table after another. Called over to take an order, he stops and
 * stands facing the customer until they have chosen.
 */
UCLASS()
class PESTIALVILAG_API ACafeWaiter : public AStoryPerson
{
	GENERATED_BODY()

public:
	ACafeWaiter();

	virtual void Tick(float DeltaSeconds) override;

	/** The café door first, then the tables (world points on the ground). */
	void SetRounds(const FVector& InDoor, const TArray<FVector>& InTables);

	/** Stops his rounds to take Customer's order; null sends him back to them. */
	void Attend(const AActor* InCustomer) { Customer = InCustomer; }

protected:
	UPROPERTY(VisibleAnywhere, Category = "Waiter")
	TObjectPtr<UStaticMeshComponent> Tray;

	virtual void BeginPlay() override;

private:
	FVector Door = FVector::ZeroVector;
	TArray<FVector> Tables;
	int32 Table = 0;
	bool bGoingToTable = true;
	float Wait = 0.f;
	float Phase = 0.f;
	TWeakObjectPtr<const AActor> Customer;
};
