// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "PestiPerson.generated.h"

class UCameraComponent;
class UMaterialInterface;
class UProceduralMeshComponent;
class USpringArmComponent;

/**
 * The player on foot, from the dress-up kit (PersonKit): a man in a long
 * coat, fedora and red scarf, with a GTA style camera behind him.
 *
 * Keys: W/A/S/D or left stick to jog, Shift or left stick click to sprint,
 * Space or gamepad A to jump, mouse or right stick to look around,
 * F or gamepad B to get into the car next to you, or to pull the driver out
 * of a car in the traffic and take it.
 */
UCLASS()
class PESTIALVILAG_API APestiPerson : public ACharacter
{
	GENERATED_BODY()

public:
	APestiPerson();

	virtual void Tick(float DeltaSeconds) override;

	/** The everyday pace: a light jog (cm/s). */
	UPROPERTY(EditAnywhere, Category = "Person")
	float JogSpeed = 330.f;

	/** Holding Shift (cm/s). */
	UPROPERTY(EditAnywhere, Category = "Person")
	float RunSpeed = 750.f;

	/** Walking speed with something heavy in hand; no running then (cm/s). */
	UPROPERTY(EditAnywhere, Category = "Person")
	float CarrySpeed = 260.f;

	/** How close a car must be to get in (cm, centre to centre). */
	UPROPERTY(EditAnywhere, Category = "Person")
	float EnterCarRange = 400.f;

	/** Degrees the camera turns per unit of mouse movement. */
	UPROPERTY(EditAnywhere, Category = "Camera")
	float MouseLookScale = 2.5f;

	/** Degrees per second the camera turns with the gamepad's right stick. */
	UPROPERTY(EditAnywhere, Category = "Camera")
	float StickLookRate = 150.f;

	/** Called by a car the player leaves: appear at At if there is room. */
	bool TryStepOutAt(const FVector& At, double Yaw);

	/**
	 * Something heavy in the right hand (a mission attaches the thing itself):
	 * a slow walk, no running, jumping or getting into cars.
	 */
	void SetCarrying(bool bInCarrying);
	bool IsCarrying() const { return bCarrying; }

	/**
	 * Something held out in front in both hands (a parcel; a mission
	 * attaches the thing itself): both arms reach forward and don't swing.
	 */
	void SetHoldingInFront(bool bInHolding) { bHoldingInFront = bInHolding; }
	bool IsHoldingInFront() const { return bHoldingInFront; }

	/**
	 * Riding along on something (a tram): he sits at SeatAt (where his
	 * middle goes, on the vehicle, facing SeatYaw) and goes where it goes;
	 * only looking around works, and C swaps the camera between close
	 * behind him and out where the whole vehicle shows.
	 */
	void RideOn(AActor* Vehicle, const FVector& SeatAt, double SeatYaw);
	/** Gets off where there is room near At; false while riding on if not. */
	bool GetOffAt(const FVector& At, double Yaw);
	bool IsRiding() const { return bRiding; }

	/**
	 * A car or tram has hit him at Speed (cm/s), going along Push: it hurts
	 * more the faster it was going, and knocks him aside.
	 */
	void HitByVehicle(float Speed, const FVector& Push);

	virtual void Landed(const FHitResult& Hit) override;

protected:
	/** Legs, arms and body (PersonKit::EPiece order), made when play begins. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UProceduralMeshComponent>> Pieces;

	UPROPERTY(VisibleAnywhere, Category = "Person")
	TObjectPtr<USpringArmComponent> CameraArm;

	UPROPERTY(VisibleAnywhere, Category = "Person")
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> BaseMaterial;

	virtual void BeginPlay() override;

private:
	void Look(APlayerController* PC, float DeltaSeconds);
	void Walk(const APlayerController* PC);
	void SwingLimbs(float DeltaSeconds);
	void TryEnterCar(APlayerController* PC);
	void SetInWorld(bool bInWorld);
	void SetRideView(bool bOutside, APlayerController* PC);

	float StridePhase = 0.f;
	bool bCameraAimed = false;
	bool bCarrying = false;
	bool bHoldingInFront = false;
	bool bRiding = false;
	/** Riding with the camera out where the whole vehicle shows. */
	bool bRideOutside = false;
	/** Where he sits on the vehicle he rides. */
	FTransform RideSeat;
	double OutOfCarTime = -1.0;
	/** The last time a vehicle hurt him, so one bump hurts only once. */
	double LastHitTime = -100.0;
	/** The fastest he has been falling since he left the ground (cm/s). */
	float FallSpeed = 0.f;
};
