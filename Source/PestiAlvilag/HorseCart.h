// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "HorseCartModel.h"
#include "HorseCart.generated.h"

class UBoxComponent;
class UCameraComponent;
class UHorseSound;
class UMaterialInterface;
class UProceduralMeshComponent;
class USceneComponent;
class USpringArmComponent;

/**
 * A horse and its flat market cart (HorseCartModel), driven from the bench.
 *
 * The horse has four paces, and W and S move it up and down one at a time
 * (standing, walk, trot, canter); held at a stand, S backs it slowly. A and
 * D turn it: the horse leads, swinging the shafts and the front wheels with
 * it about the turntable, and the cart follows. Space pulls it up ("Hó!"),
 * F gets down. The horse's legs step with its pace, its head nods, its
 * hooves clop on the cobbles and the iron tyres rumble.
 *
 * The cart carries up to six crates (SetCrates).
 */
UCLASS()
class PESTIALVILAG_API AHorseCart : public APawn
{
	GENERATED_BODY()

public:
	AHorseCart();

	virtual void Tick(float DeltaSeconds) override;
	virtual FVector GetVelocity() const override;

	/** Puts a person on the bench; they get down again with F. */
	void TakeDriver(APawn* InDriver);
	/** Where to climb up: by the bench (on the ground). */
	FVector GetBenchLocation() const;

	/** Shows this many crates on the bed (0 to six). */
	void SetCrates(int32 Count);
	int32 GetCrates() const { return Crates; }
	/** Where crate Index stands on the bed, in the world. */
	FVector GetCrateSpot(int32 Index) const;

	/** How fast it goes: 0 standing, 1 walk, 2 trot, 3 canter. */
	int32 GetPace() const { return Pace; }
	float GetSpeed() const { return Speed; }

protected:
	UPROPERTY(VisibleAnywhere, Category = "Cart")
	TObjectPtr<UBoxComponent> CollisionBox;

	/** The cart's frame: on the ground under the middle of the bed. */
	UPROPERTY(VisibleAnywhere, Category = "Cart")
	TObjectPtr<USceneComponent> Rig;

	/** The front axle, the shafts and the horse, turning about the turntable. */
	UPROPERTY(VisibleAnywhere, Category = "Cart")
	TObjectPtr<USceneComponent> ForecartPivot;

	/** The horse's frame: on the ground under the middle of its body. */
	UPROPERTY(VisibleAnywhere, Category = "Cart")
	TObjectPtr<USceneComponent> HorseRoot;

	UPROPERTY(VisibleAnywhere, Category = "Cart")
	TObjectPtr<UHorseSound> Sound;

	UPROPERTY(VisibleAnywhere, Category = "Cart")
	TObjectPtr<USpringArmComponent> CameraArm;

	UPROPERTY(VisibleAnywhere, Category = "Cart")
	TObjectPtr<UCameraComponent> ChaseCamera;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UProceduralMeshComponent>> HorseParts;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UProceduralMeshComponent>> CartParts;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UProceduralMeshComponent>> CrateMeshes;

	UPROPERTY(Transient)
	TObjectPtr<UProceduralMeshComponent> DriverMesh;

	UPROPERTY(Transient)
	TObjectPtr<UProceduralMeshComponent> Reins;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> BaseMaterial;

	virtual void BeginPlay() override;

private:
	UProceduralMeshComponent* MakeMesh(USceneComponent* Parent, const PersonKit::FPiece& Piece, const FVector& At);
	void Fill(UProceduralMeshComponent* Mesh, const TArray<PersonKit::FSection>& Sections);
	void Drive(APlayerController* PC, float DeltaSeconds);
	void Move(float DeltaSeconds);
	void Animate(float DeltaSeconds);
	void UpdateReins();
	void UpdateFreeLook(const APlayerController* PC, float DeltaSeconds);
	void LetDriverOut(APlayerController* PC);
	void SnapToGround();

	/** The pace asked for, and the speed (cm/s, below 0 backing). */
	int32 Pace = 0;
	float Speed = 0.f;
	/** How far the shafts are turned (degrees, right positive). */
	float SteerAngle = 0.f;
	/** How far into its stride the horse is (0..1), and which hooves were down. */
	float Stride = 0.f;
	bool WasPlanted[4] = { true, true, true, true };
	float Idle = 0.f;
	double NextSnort = 0.0;
	float WheelTurn = 0.f;
	int32 Crates = 0;

	float LookYaw = 0.f;
	float LookPitch = 0.f;
	float TimeSinceLook = 0.f;

	UPROPERTY()
	TObjectPtr<APawn> Driver;
	double DriverInTime = 0.0;
};
