// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Tram.h"
#include "Bus.generated.h"

class UEngineSound;

/**
 * A city bus (BusModel: the MÁVAG "Harcsa" in dark blue and white) on one
 * of the 1932 bus lines. It drives its route exactly as a tram does (ATram):
 * along the traffic lanes, keeping left, stopping at its stops and behind
 * whatever is in front of it, then off the map and back in later. People
 * wait at its stops and get on and off by the door on the left, and the
 * player can ride it like a tram (ATramRides).
 *
 * Its own: the body with the line's number on the roof, turning wheels, the
 * seats in pairs facing forward, and an engine (UEngineSound) instead of
 * the tram's motors and bell.
 */
UCLASS()
class PESTIALVILAG_API ABus : public ATram
{
	GENERATED_BODY()

public:
	ABus();

	virtual void Tick(float DeltaSeconds) override;
	virtual void StartOnRoute(ATrafficManager* InManager, int32 InRouteIndex, float InDistance) override;
	virtual int32 GetFreeSeat() const override;
	virtual void GetSeat(int32 Index, FVector& OutAt, double& OutYaw) const override;
	virtual double GetGroundBelow() const override;
	virtual bool IsBus() const override { return true; }
	virtual FVector GetDoorNear(const FVector& Point) const override;

protected:
	virtual void BuildBody() override;
	virtual int32 GetNumSeats() const override;
	virtual void UpdateSound() override;
	virtual void StopSound() override;
	virtual bool RingBell() override { return false; }
	virtual void LeaveStop() override {}

	/** The four wheels, turned as the bus goes (BusModel::Hub order). */
	UPROPERTY(VisibleAnywhere, Category = "Bus")
	TArray<TObjectPtr<UProceduralMeshComponent>> Wheels;

	/** Made when the camera first comes near. */
	UPROPERTY(Transient)
	TObjectPtr<UEngineSound> Engine;

private:
	/** The line's number, shown on the roof box (0 until the route is known). */
	int32 RouteNumber = 0;
	/** The number the body was last built with, and whether its colours are set. */
	int32 BuiltNumber = INDEX_NONE;
	bool bPainted = false;
	/** How far the wheels have turned (degrees), and the route distance it was worked out at. */
	double WheelTurn = 0.0;
	float LastDistance = 0.f;
};
