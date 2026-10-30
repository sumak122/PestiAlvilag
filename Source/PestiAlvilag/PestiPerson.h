// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Brawl.h"
#include "FreeRun.h"
#include "PersonKit.h"
#include "PestiPerson.generated.h"

class UCameraComponent;
class UMaterialInterface;
class UProceduralMeshComponent;
class USceneComponent;
class USpringArmComponent;

/**
 * The player on foot, from the dress-up kit (PersonKit): a man in a long
 * coat, fedora and red scarf, with a GTA style camera behind him.
 *
 * Keys: W/A/S/D or left stick to jog, Shift or left stick click to sprint,
 * Space or gamepad A to jump, or running at something low or something to
 * get under, to vault over it or slide under it (FreeRun), mouse or right
 * stick to look around,
 * F or gamepad B to get into the car next to you, or to pull the driver out
 * of a car in the traffic and take it.
 *
 * In a fist fight (Brawl, StartFight) he faces the other man and the keys
 * change: click or gamepad X to jab, held for a heavy swing; right click or
 * the right shoulder button held to guard, or pressed when the gold sign
 * lights up to counter; E or gamepad B to grab him by the collar, then click
 * to knee him and E to shove him off, into whatever is behind him. The
 * other man (AChaseThief) fights by the same rules from his own head.
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

	virtual void NotifyHit(UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp, bool bSelfMoved,
		FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit) override;

	/** Squares up to InFoe: fists up and facing him (see Brawl). */
	void StartFight(APestiPerson* InFoe);
	/** Fists down: back to walking about. */
	void EndFight();
	bool IsFighting() const { return Act != Brawl::EAct::None && Act != Brawl::EAct::Beaten; }
	APestiPerson* GetFoe() const { return Foe.Get(); }
	Brawl::EAct GetAct() const { return Act; }
	float GetStamina() const { return Stamina; }
	/** In the middle of a vault or a slide. */
	bool IsFreeRunning() const { return FreeMove != FreeRun::EMove::None; }

	/**
	 * The seconds until his heavy swing or his grab lands, while one is
	 * coming (the moment for the other man to counter); below 0 otherwise.
	 */
	float GetWindUpLeft() const;

	// The moves, for the player's keys and for the other man's head alike:
	// false when he can't now (busy, or out of breath).
	bool Jab();
	bool HeavySwing();
	/** Under his swing and back with a hook, if it is coming; false otherwise. */
	bool Counter();
	/** Reaching for his collar; slowly, it can be seen coming. */
	bool Grab(bool bSlowly = false);
	/** A knee, while holding him. */
	bool Knee();
	/** Throws him off along Direction, while holding him. */
	bool Shove(const FVector& Direction);
	void SetGuard(bool bUp);
	/** Held by the collar: a tug to get free. */
	void TugFree();

	/** Brings Runner down from behind with a dive; both get up to fight. */
	void TackleDown(APestiPerson* Runner);
	/** A blow from From lands on him. */
	void TakeBlow(Brawl::EBlow Blow, APestiPerson* From);
	/** Sits down beaten, both hands up: his fight is over. */
	void GiveIn();

	/** What the fight shows over his head and his health (the foe's bar on the screen). */
	virtual FString GetFightName() const { return FString(); }
	virtual float GetHealthShare() const { return 1.f; }

protected:
	/** Legs, arms and body (PersonKit::EPiece order), made when play begins. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UProceduralMeshComponent>> Pieces;

	/** What the pieces hang from: tilted and lowered as a whole to vault and slide. */
	UPROPERTY(VisibleAnywhere, Category = "Person")
	TObjectPtr<USceneComponent> PoseRoot;

	UPROPERTY(VisibleAnywhere, Category = "Person")
	TObjectPtr<USpringArmComponent> CameraArm;

	UPROPERTY(VisibleAnywhere, Category = "Person")
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> BaseMaterial;

	virtual void BeginPlay() override;

	/** The clothes he wears. */
	virtual PersonKit::FOutfit GetDress() const;
	/** Health lost in a fight: the player's from the game's, anyone else's from their own. */
	virtual void LoseHealth(float Amount);

	/** The player's keys in a fight, and what the screen shows of it. */
	void PlayerFight(APlayerController* PC);
	/** The way the movement keys point, flat (zero when none are held). */
	FVector InputDirection(const APlayerController* PC) const;
	/** The blow lands if the foe is within reach in front. */
	void LandBlow(Brawl::EBlow Blow);
	void FaceFoe(float DeltaSeconds);
	/** Takes Cost from his breath; false when he has none left. */
	bool SpendStamina(float Cost);
	/** Held by Grabber, or thrown off by him. */
	void BeHeld(APestiPerson* Grabber);
	void BeThrown(const FVector& Direction, APestiPerson* Thrower);

	/** Into a new act of the fight, for Seconds (0 for one that lasts). */
	void BeginAct(Brawl::EAct NewAct, float Seconds);
	void TickBrawl(float DeltaSeconds);
	/** Lets go of whoever he holds, or gets let go of. */
	void LetGo();

	/** The vault or slide under way, and how far into it (s). */
	FreeRun::EMove FreeMove = FreeRun::EMove::None;
	double BoostUntil = -1.0;
	double StumbleUntil = -1.0;

	/** The fight: what he does, how far into it (s), and how long it lasts. */
	Brawl::EAct Act = Brawl::EAct::None;
	float ActClock = 0.f;
	float ActLength = 0.f;
	TWeakObjectPtr<APestiPerson> Foe;
	float Stamina = Brawl::MaxStamina;

private:
	void Look(APlayerController* PC, float DeltaSeconds);
	void Walk(const APlayerController* PC);
	void SwingLimbs(float DeltaSeconds);
	void TryEnterCar(APlayerController* PC);
	void SetInWorld(bool bInWorld);
	void SetRideView(bool bOutside, APlayerController* PC);
	/** Over or under the thing ahead, as FreeRun found it. */
	void StartMove(const FreeRun::FChance& Chance);
	void TickMove(float DeltaSeconds);
	void FinishMove();
	void ApplyPose(const FreeRun::FPose& Pose);

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

	FreeRun::FChance MoveChance;
	FVector MoveFrom = FVector::ZeroVector;
	FVector MoveDir = FVector::ForwardVector;
	float MoveClock = 0.f;
	float MoveLength = 0.f;
	/** Pressed at the right moment: out of it faster. Pressed nearly on it: a stumble after. */
	bool bMoveClean = false;
	bool bMoveLate = false;

	/** The blow of the act under way has landed (or missed). */
	bool bBlowDone = false;
	/** When his breath starts coming back. */
	double RestFrom = 0.0;
	/** While holding someone: until when, and the knees given. */
	double HoldUntil = 0.0;
	int32 KneesGiven = 0;
	/** Held: who holds him, and the tugs to get free. */
	TWeakObjectPtr<APestiPerson> HeldBy;
	TWeakObjectPtr<APestiPerson> Holding;
	int32 Tugs = 0;
	/** Thrown off: by whom, and which way his head snaps when hit. */
	TWeakObjectPtr<APestiPerson> ThrownBy;
	float HurtSide = 1.f;
	/** The punch key: when it went down, and whether holding it made a heavy swing. */
	double PunchDownAt = -1.0;
	bool bPunchHeld = false;
};
