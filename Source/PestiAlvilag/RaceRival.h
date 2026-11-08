// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CarModel.h"
#include "GameFramework/Actor.h"
#include "RaceRival.generated.h"

class UBoxComponent;
class UCarVisuals;

/**
 * One of the other cars in Laci's night races (ANightRaces). It follows the
 * race's line through the streets (a list of points), a little to one side,
 * slowing for the corners it sees coming and speeding up again after them;
 * it holds back a little when far ahead of the player and pushes on when
 * far behind. It bumps into things (the same box as a traffic car); stuck
 * on one for a while, it drives on through (never through the player).
 */
UCLASS()
class PESTIALVILAG_API ARaceRival : public AActor
{
	GENERATED_BODY()

public:
	ARaceRival();

	virtual void Tick(float DeltaSeconds) override;

	/** The line to follow (city points, in order), its cumulative lengths, the top speed (cm/s), the side offset (cm, + right) and the look. */
	void Setup(const TArray<FVector>& InLine, const TArray<double>& InAlong, float InTopSpeed, double InSide, const CarModel::FStyle& InStyle, double StartAlong);

	/** Off it goes (until then it stands still with its engine running). */
	void Go() { bGoing = true; }

	/** How far along the line it is (cm), and the player's, for holding back or pushing on. */
	double GetAlong() const { return Along; }
	void SetPlayerAlong(double InAlong) { PlayerAlong = InAlong; }
	bool HasFinished() const { return Along >= Total; }

	/** The point a distance S along the line (no side offset), and the way the line heads there. */
	static FVector PointAt(const TArray<FVector>& Points, const TArray<double>& Lengths, double S, FVector* OutDir = nullptr);

protected:
	UPROPERTY(VisibleAnywhere, Category = "Race")
	TObjectPtr<UBoxComponent> CollisionBox;

	UPROPERTY(VisibleAnywhere, Category = "Race")
	TObjectPtr<UCarVisuals> Visuals;

private:
	/** The fastest it may go now, for the corners ahead. */
	float CornerLimit() const;
	double GroundAt(const FVector& At) const;

	TArray<FVector> Line;
	TArray<double> Cumulative;
	double Total = 0.0;
	double Along = 0.0;
	double PlayerAlong = 0.0;
	double Side = 0.0;
	float TopSpeed = 1800.f;
	float Speed = 0.f;
	float BlockedTime = 0.f;
	bool bGoing = false;
};
