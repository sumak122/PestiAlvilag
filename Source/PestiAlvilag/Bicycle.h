// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "BicycleModel.h"
#include "PersonKit.h"
#include "Bicycle.generated.h"

class UBikeBellSound;
class UBoxComponent;
class UCameraComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UProceduralMeshComponent;
class USpotLightComponent;
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
 * leans over onto its kickstand. A dynamo on the rear tyre lights the lamp,
 * brighter the faster it goes. Shift (held while pedalling) is a sprint: he
 * stands on the pedals and it goes faster until he is out of breath, which
 * comes back when he rides easy. B rings the bell.
 *
 * Other kinds (SetKind) and enamel colours (SetPaint) are for the bicycles
 * of the city (bCityBike): parked ones and those of ATrafficBike riders.
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

	/** A bicycle of the city (parked about the streets, or ridden in the traffic), not one of the story's. */
	UPROPERTY(EditAnywhere, Category = "Bicycle")
	bool bCityBike = false;

	/** The kind (unless bShopBike) and the enamel colour; set before it is spawned. */
	void SetKind(BicycleModel::EKind InKind) { BikeKind = InKind; }
	void SetPaint(const FLinearColor& InPaint) { FramePaint = InPaint; }
	BicycleModel::EKind GetKind() const { return bShopBike ? BicycleModel::EKind::ShopBike : BikeKind; }
	/** Who sits on it when someone else than the player rides it (set before it is spawned). */
	void SetRiderLook(const PersonKit::FOutfit& Outfit) { RiderLook = Outfit; bOwnRiderLook = true; }
	/** Its pieces are not drawn beyond this (cm; 0 always drawn). Set before it is spawned. */
	void SetDrawDistance(float Distance) { DrawDistance = Distance; }

	/** Puts a person on the saddle; they get off again with F. */
	virtual void TakeRider(APawn* InRider);
	/** Where to get on: beside the saddle (on the ground). */
	FVector GetSaddleLocation() const;
	/** Shows this many parcels in the basket (0 to 3). */
	void SetParcels(int32 Count);
	int32 GetParcels() const { return Parcels; }
	float GetSpeed() const { return Speed; }
	/** How many times it has been ridden hard into something and stopped dead. */
	int32 GetHardKnocks() const { return HardKnocks; }
	/** Puts the player down beside it at once, whatever its speed (a mission that is ending). */
	void PutRiderOff();

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

	/** The lamp's glass, and its own paint and its glow (lit by the dynamo). */
	UPROPERTY(Transient)
	TObjectPtr<UProceduralMeshComponent> LensMesh;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> LensPaint;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> LensGlow;

	/** The lamp's beam on the road at night, made the first time it is needed. */
	UPROPERTY(Transient)
	TObjectPtr<USpotLightComponent> Beam;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UProceduralMeshComponent>> ParcelMeshes;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UProceduralMeshComponent>> RiderPieces;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> BaseMaterial;

	/** The bell, made the first time it is rung. */
	UPROPERTY(Transient)
	TObjectPtr<UBikeBellSound> BellSound;

	virtual void BeginPlay() override;

	UProceduralMeshComponent* MakeMesh(USceneComponent* Parent, const PersonKit::FPiece& Piece, const FVector& At);
	void Animate(float DeltaSeconds);
	void PoseRider();
	/** Dresses the figure on the saddle (hidden until ShowRider). */
	void DressRider(const PersonKit::FOutfit& Outfit);
	void ShowRider(bool bShow);
	/** "Rrring-rrring", heard near the camera. */
	void RingBell();
	/** Lets the bell fall silent a while after it was rung. */
	void UpdateBell(float DeltaSeconds);
	void SnapToGround();

	/** Speed (cm/s, below 0 walking it back) and how far the bars are turned (degrees, right positive). */
	float Speed = 0.f;
	float SteerAngle = 0.f;
	/** How far over it leans (degrees, right positive), and a lean on top of that (a wobble, or lying on its side). */
	float Lean = 0.f;
	float ExtraRoll = 0.f;
	/** How the kickstand is turned about its hinge (degrees: down to prop it, back when ridden). */
	float StandPitch = 0.f;
	/** How far the cranks have turned (degrees). */
	float Crank = 0.f;
	bool bPedalling = false;
	/** 0 riding easy, 1 standing on the pedals in a sprint. */
	float SprintBlend = 0.f;
	PersonKit::FOutfit RiderLook;
	bool bOwnRiderLook = false;
	bool bRiderShown = false;
	float BellQuiet = 0.f;

	UPROPERTY()
	TObjectPtr<APawn> Rider;

private:
	void Ride(APlayerController* PC, float DeltaSeconds);
	void Move(float DeltaSeconds);
	/** The dynamo lamp: brighter the faster it goes, a beam on the road at night. */
	void UpdateLamp();
	void UpdateFreeLook(const APlayerController* PC, float DeltaSeconds);
	/** He gets off, only when it is nearly stopped unless bForce; false when there was no room. */
	bool LetRiderOff(APlayerController* PC, bool bForce = false);

	/** How bright the lamp was last set (0 dark, 1 full), and how dark the evening was then. */
	float LampGlow = 0.f;
	float LampDarkness = 0.f;
	/** How far the wheels have turned (cm rolled). */
	float Rolled = 0.f;
	int32 Parcels = 0;
	BicycleModel::EKind BikeKind = BicycleModel::EKind::Roadster;
	FLinearColor FramePaint = BicycleModel::Enamel();
	float DrawDistance = 0.f;
	/** Sprint breath (1 full, 0 spent); once spent, no sprint until it is back to SprintAgain. */
	float Breath = 1.f;
	bool bBreathSpent = false;
	int32 HardKnocks = 0;

	float LookYaw = 0.f;
	float LookPitch = 0.f;
	float TimeSinceLook = 0.f;

	double RiderOnTime = 0.0;
};
