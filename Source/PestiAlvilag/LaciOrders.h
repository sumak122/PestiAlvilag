// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CarModel.h"
#include "LaciYardModel.h"
#include "LaciOrders.generated.h"

class ALaciYard;
class APestiCar;
class UGoalMarkerComponent;
class URouteGuideComponent;

enum class ELaciOrder : uint8
{
	None,
	Steal,      // the car stands parked somewhere; go and take it
	Deliver,    // the player drives it; bring it to Laci's pit
	Handover,   // the screen is black while Laci takes it in
};

/**
 * Laci's car orders, a side job after mission 7. Press E by Laci and he
 * names a car he wants (a kind and a colour) and the street where one is
 * parked; a marker shows it. Take it, lose the police if anyone saw, and
 * stop it over his pit: the car is gone, he pays (less for a battered
 * one), and its leftovers turn up in his yard (LaciYardModel::BuildLeftovers).
 * A car is kept as Body * 100 + paint (the game mode's LaciCars, saved).
 */
UCLASS()
class PESTIALVILAG_API ALaciOrders : public AActor
{
	GENERATED_BODY()

public:
	ALaciOrders();

	virtual void Tick(float DeltaSeconds) override;

	/** The look of an ordered car, from its saved number. */
	static CarModel::FStyle OrderStyle(int32 Packed);
	/** What is left of it in Laci's yard. */
	static LaciYardModel::FStrippedCar Leftovers(int32 Packed);

	/** The car of the order going on (Laci's garage leaves it alone), or null. */
	APestiCar* GetOrderCar() const;

protected:
	UPROPERTY(VisibleAnywhere, Category = "Laci")
	TObjectPtr<UGoalMarkerComponent> Marker;

	UPROPERTY(VisibleAnywhere, Category = "Laci")
	TObjectPtr<URouteGuideComponent> Guide;

private:
	ALaciYard* GetYard() const;
	bool IsMissionUnderway() const;
	void StartOrder();
	void SayOrder() const;
	void TickSteal(APawn* Player);
	void TickDeliver(APawn* Player, float DeltaSeconds);
	void TickHandover();
	/** The order is off (the car wrecked or lost): says why. */
	void DropOrder(const FString& Line);
	void Note(const FString& Line, float Seconds) const;
	void LaciSays(const FString& Hu, const FString& En) const;

	ELaciOrder State = ELaciOrder::None;

	UPROPERTY(Transient)
	TObjectPtr<APestiCar> OrderCar;

	int32 OrderPacked = 0;
	FString OrderStreet;
	/** How long the car has stood still over the pit (s). */
	float StoodFor = 0.f;
	/** Whether the theft has been noticed yet (once per order). */
	bool bTaken = false;
	double LastHint = -100.0;
	/** The handover: when the car goes and the money is paid, and when the screen clears. */
	double SwapAt = -1.0;
	double ClearAt = -1.0;
	int32 Pay = 0;
};
