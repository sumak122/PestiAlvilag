// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "PersonKit.h"
#include "Bicycle.generated.h"

class UBoxComponent;
class UCameraComponent;
class UMaterialInterface;
class UProceduralMeshComponent;
class USceneComponent;
class USpringArmComponent;

/**
 * A bicycle you ride (BicycleModel): the ordinary roadster with two big
 * wheels and a basket over the front one, or (bShopBike) the shop bicycle
 * with a small front wheel under its basket.
 *
 * W pedals (hold it to keep going; let go and it rolls on), S brakes and,
 * stopped, walks it back a little, Space brakes hard, A and D steer: the
 * bars turn, the bicycle leans into the turn, and it turns tighter slow
 * than fast. F gets off when it is nearly stopped. The rider is the
 * player's own dress-up figure, his feet on the turning pedals and his
 * hands on the grips. Riding hard into a wall stops it dead. Left alone it
 * leans over onto its kickstand.
 */
UCLASS()
class PESTIALVILAG_API ABicycle : public APawn
{
	GENERATED_BODY()

public:
	ABicycle();

	virtual void Tick(float DeltaSeconds) override;
	virtual FVector GetVelocity() const override;

	/** The small-wheeled shop bicycle instead of the roadster (set before it is spawned). */
	UPROPERTY(EditAnywhere, Category = "Bicycle")
	bool bShopBike = false;

	/** Puts a person on the saddle; they get off again with F. */
	void TakeRider(APawn* InRider);
	/** Where to get on: beside the saddle (on the ground). */
	FVector GetSaddleLocation() const;
	/** Shows this many parcels in the basket (0 to 3). */
	void SetParcels(int32 Count);
	int32 GetParcels() const { return Parcels; }
	float GetSpeed() const { return Speed; }

protected:
	UPROPERTY(VisibleAnywhere, Category = "Bicycle")
	TObjectPtr<UBoxComponent> CollisionBox;

	/** The bicycle's frame: on the ground under the bottom bracket. It leans into turns. */
	UPROPERTY(VisibleAnywhere, Category = "Bicycle")
	TObjectPtr<USceneComponent> Rig;

	/** At the bottom of the head tube, its Z along the steering axis. */
	UPROPERTY(VisibleAnywhere, Category = "Bicycle")
	TObjectPtr<USceneComponent> SteerPivot;

	/** Turns with the bars about the steering axis. */
	UPROPERTY(VisibleAnywhere, Category = "Bicycle")
	TObjectPtr<USceneComponent> SteerTurn;

	/** Under SteerTurn, back in the frame's own directions: the fork, bars and front wheel hang here. */
	UPROPERTY(VisibleAnywhere, Category = "Bicycle")
	TObjectPtr<USceneComponent> SteerBody;

	/** At the rider's hips, leaning him forward; his body hangs from it. */
	UPROPERTY(VisibleAnywhere, Category = "Bicycle")
	TObjectPtr<USceneComponent> RiderRoot;

	UPROPERTY(VisibleAnywhere, Category = "Bicycle")
	TObjectPtr<USceneComponent> RiderBody;

	UPROPERTY(VisibleAnywhere, Category = "Bicycle")
	TObjectPtr<USpringArmComponent> CameraArm;

	UPROPERTY(VisibleAnywhere, Category = "Bicycle")
	TObjectPtr<UCameraComponent> ChaseCamera;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UProceduralMeshComponent>> Parts;

	UPROPERTY(Transient)
	TObjectPtr<UProceduralMeshComponent> StandMesh;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UProceduralMeshComponent>> ParcelMeshes;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UProceduralMeshComponent>> RiderPieces;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> BaseMaterial;

	virtual void BeginPlay() override;

private:
	UProceduralMeshComponent* MakeMesh(USceneComponent* Parent, const PersonKit::FPiece& Piece, const FVector& At);
	void Ride(APlayerController* PC, float DeltaSeconds);
	void Move(float DeltaSeconds);
	void Animate(float DeltaSeconds);
	void PoseRider();
	void UpdateFreeLook(const APlayerController* PC, float DeltaSeconds);
	void LetRiderOff(APlayerController* PC);
	void SnapToGround();

	/** Speed (cm/s, below 0 walking it back) and how far the bars are turned (degrees, right positive). */
	float Speed = 0.f;
	float SteerAngle = 0.f;
	/** How far over it leans (degrees, right positive). */
	float Lean = 0.f;
	/** How the kickstand is turned about its hinge (degrees: down to prop it, back when ridden). */
	float StandPitch = 0.f;
	/** How far the cranks and the wheels have turned (degrees, and cm rolled). */
	float Crank = 0.f;
	float Rolled = 0.f;
	bool bPedalling = false;
	int32 Parcels = 0;

	float LookYaw = 0.f;
	float LookPitch = 0.f;
	float TimeSinceLook = 0.f;

	UPROPERTY()
	TObjectPtr<APawn> Rider;
	double RiderOnTime = 0.0;
};
