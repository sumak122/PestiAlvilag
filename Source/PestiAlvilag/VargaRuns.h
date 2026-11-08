// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VargaRuns.generated.h"

class AStoryPerson;
class UGoalMarkerComponent;
class URouteGuideComponent;

/**
 * Varga's delivery rounds, a side job after mission 6. Varga Lajos stands
 * at his grocery on Népszínház utca again; press E by him for a round of
 * three parcels to three addresses (by the kerb of named streets), then
 * back to the shop. The clock runs from the start: a fast round earns a
 * medal (bronze, silver, gold) and a bonus, and the best time and medal
 * are kept (the game mode, saved). Any way of getting there will do, but
 * Varga's old bicycle is the quick one.
 */
UCLASS()
class PESTIALVILAG_API AVargaRuns : public AActor
{
	GENERATED_BODY()

public:
	AVargaRuns();

	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	bool IsRunning() const { return bRunning; }

	/** A time in tenths of a second as m:ss.t. */
	static FString FormatTenths(int32 Tenths);

protected:
	UPROPERTY(VisibleAnywhere, Category = "Varga")
	TObjectPtr<UGoalMarkerComponent> Marker;

	UPROPERTY(VisibleAnywhere, Category = "Varga")
	TObjectPtr<URouteGuideComponent> Guide;

private:
	bool IsMissionUnderway() const;
	void PlaceVarga();
	bool StartRound();
	void FinishRound();
	void StopRound();
	void Note(const FString& Line, float Seconds) const;
	void VargaSays(const FString& Hu, const FString& En) const;

	UPROPERTY(Transient)
	TObjectPtr<AStoryPerson> Varga;

	bool bRunning = false;
	/** The three addresses and their streets; Next is the one the player is on (3 = back to the shop). */
	TArray<FVector> Drops;
	TArray<FString> Streets;
	int32 Next = 0;
	FVector ShopAt = FVector::ZeroVector;
	float Clock = 0.f;
	/** What the round should take (s), from its length: the gold time. */
	float Par = 0.f;
};
