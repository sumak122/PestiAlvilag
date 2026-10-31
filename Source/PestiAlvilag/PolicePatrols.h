// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Pedestrians.h"
#include "Components/SynthComponent.h"
#include <atomic>
#include "PolicePatrols.generated.h"

class ATrafficManager;
class UBoxComponent;
class UMaterialInterface;
class UProceduralMeshComponent;
class UTextRenderComponent;
class UTexture;

/**
 * A constable of the state police (PoliceModel): a walker in uniform whom
 * APolicePatrols moves, with a truncheon he draws in a chase.
 */
UCLASS()
class PESTIALVILAG_API APoliceman : public APedestrian
{
	GENERATED_BODY()

public:
	APoliceman();

	/** Dresses him in the uniform, with the shako, brass and sabre. */
	void PutOnUniform(const PersonKit::FOutfit& Uniform);

	void ShowTruncheon(bool bShow);

	/** Walking: the stride of WalkTo, the elbows a little bent. */
	void WalkOn(const FVector& Location, double Yaw, float Speed, float DeltaSeconds);

	/** Running after someone: the stride of WalkTo, the truncheon raised. */
	void RunTo(const FVector& Location, double Yaw, float Speed, float DeltaSeconds);

	/** At the telephone: the right hand on the crank (turning it while bCranking), the earpiece at his left ear. */
	void PoseAtPhone(const FVector& Location, double Yaw, bool bCranking, float Seconds);

	/** Standing still, facing Yaw. */
	void Stand(const FVector& Location, double Yaw);

private:
	UPROPERTY(Transient)
	TObjectPtr<UProceduralMeshComponent> Kit;

	UPROPERTY(Transient)
	TObjectPtr<UProceduralMeshComponent> Truncheon;

	UPROPERTY(Transient)
	TObjectPtr<UProceduralMeshComponent> Earpiece;
};

/**
 * A police telephone box (PoliceModel::BuildBox): an iron and glass kiosk
 * with RENDŐRSÉG under the eaves, from which the constables ring the
 * station. Its door faces the actor's forward direction.
 */
UCLASS()
class PESTIALVILAG_API APoliceBox : public AActor
{
	GENERATED_BODY()

public:
	APoliceBox();

	/** Where a constable stands to use the telephone, on the map, and the way he faces. */
	FVector2D PhoneSpot() const;
	double PhoneYaw() const;
	/** On the pavement just outside the door. */
	FVector2D DoorStep() const;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Police")
	TObjectPtr<UBoxComponent> Collision;

	UPROPERTY(VisibleAnywhere, Category = "Police")
	TObjectPtr<UProceduralMeshComponent> Body;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> BaseMaterial;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> SeeThroughMaterial;

	UPROPERTY()
	TObjectPtr<UTexture> WhiteTexture;
};

/** A pea whistle's two shrill blasts, made as it plays. */
UCLASS()
class PESTIALVILAG_API UPoliceWhistle : public USynthComponent
{
	GENERATED_BODY()

public:
	UPoliceWhistle(const FObjectInitializer& ObjectInitializer);

	void Blow() { Blows.fetch_add(1); }

protected:
	virtual bool Init(int32& SampleRate) override;
	virtual int32 OnGenerateAudio(float* OutAudio, int32 NumSamples) override;

private:
	std::atomic<int32> Blows { 0 };

	// Sound thread only.
	float SampleRateHz = 48000.f;
	float BlastTime = -1.f;
	float WavePhase = 0.f;
	uint32 NoiseSeed = 12345u;
};

/**
 * The state police on foot. Puts up the police telephone boxes from the
 * street furniture file and gives each a pair of constables who walk the
 * pavements round it, side by side, and now and then one of them steps
 * into the box and rings the station while the other waits.
 *
 * They watch the street: a constable who sees the player run someone down,
 * take a car or wreck a stall makes him wanted (see Witnessed). Once he is
 * wanted, constables near him blow the whistle, shout and run after him,
 * along the streets until they have him in sight and then straight at him,
 * and staying slow beside one gets him arrested, as beside a police car.
 * Meanwhile the second of the pair runs to the box and telephones for help,
 * and then the police cars of the whole district come (see
 * APestiAlvilagGameModeBase::CallInPolice). Knocking one down with the car
 * makes it worse. When he is no longer wanted they walk back to their beat.
 */
UCLASS()
class PESTIALVILAG_API APolicePatrols : public AActor
{
	GENERATED_BODY()

public:
	APolicePatrols();

	virtual void Tick(float DeltaSeconds) override;

	/**
	 * Did the police see something happen here? True when a police car is
	 * within CarRange or a constable has it in sight; the constables who saw
	 * it set off after the player.
	 */
	static bool Witnessed(const UObject* WorldContextObject, const FVector& At, double CarRange);

	/** The furniture file with the boxes and the walking lines, relative to the project folder. */
	UPROPERTY(EditAnywhere, Category = "Police")
	FString DataFile = TEXT("Data/OsmGraybox/street_furniture.json");

	/** How far from its box a pair's beat reaches (cm). */
	UPROPERTY(EditAnywhere, Category = "Police")
	float BeatRadius = 9000.f;

	/** A constable sees a crime this far off, if nothing is in the way (cm). */
	UPROPERTY(EditAnywhere, Category = "Police")
	float SightRange = 4500.f;

	/** Constables this close when the player is wanted go after him (cm). */
	UPROPERTY(EditAnywhere, Category = "Police")
	float ResponseRange = 6000.f;

	/** A chase is given up when the player gets this far away (cm). */
	UPROPERTY(EditAnywhere, Category = "Police")
	float GiveUpRange = 11000.f;

	UPROPERTY(EditAnywhere, Category = "Police")
	float WalkSpeed = 115.f;

	/** Running after the player (cm/s): faster than his jog, slower than his run. */
	UPROPERTY(EditAnywhere, Category = "Police")
	float RunSpeed = 560.f;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(Transient)
	TObjectPtr<UPoliceWhistle> Whistle;

private:
	enum class EDoing : uint8
	{
		Beat,       // walking the beat with his mate
		Waiting,    // standing by while his mate telephones
		ToPhone,    // walking (or running) to the box
		Phoning,    // ringing the station from the box
		Chasing,    // after the player
		Down,       // knocked down by the player's car
		Returning,  // back to the beat
	};

	struct FMan
	{
		TObjectPtr<APoliceman> Person = nullptr;
		EDoing Doing = EDoing::Beat;
		/** Where he is drawn, on the map, and which way he faces. */
		FVector2D At = FVector2D::ZeroVector;
		float Yaw = 0.f;
		double GroundZ = 0.0;
		float GroundClock = 0.f;
		/** The way he is going (when not on the beat line), and the next point on it. */
		TArray<FVector2D> Path;
		int32 Next = 0;
		float Clock = 0.f;
		float SightClock = 0.f;
		float ReplanClock = 0.f;
		bool bSeesPlayer = false;
		/** Telephoning for help (not the routine call). */
		bool bForHelp = false;
	};

	struct FPatrol
	{
		FMan Men[2];
		int32 Box = 0;
		/** Their place on the walking lines. */
		int32 Line = 0;
		double Along = 0.0;
		int32 Dir = 1;
		/** Until the next routine call to the station (s). */
		float CallClock = 0.f;
		/** Whistled and shouted at the player, and telephoned for help, this time he is wanted. */
		bool bAlarmed = false;
		bool bPhonedForHelp = false;
	};

	struct FLine
	{
		TArray<FVector2D> Points;
		TArray<double> Along;
		double Length = 0.0;
	};

	bool LoadData();
	void SpawnPatrols();
	FVector2D PointOn(const FLine& Line, double Along, FVector2D& OutTangent) const;
	/** The nearest point on any walking line to At. */
	bool NearestOnLines(const FVector2D& At, int32& OutLine, double& OutAlong) const;
	/** At a line's end: on to a line that joins there and stays on the beat, or back. */
	void TurnAtEnd(FPatrol& Patrol);
	/** Where each man walks on the beat now. */
	FVector2D BeatSpot(const FPatrol& Patrol, int32 Which, FVector2D& OutTangent) const;

	void TickPatrol(FPatrol& Patrol, float DeltaSeconds);
	void TickMan(FPatrol& Patrol, int32 Which, float DeltaSeconds);
	/** Moves him towards Goal at Speed; true once he is there. */
	bool StepTowards(FMan& Man, const FVector2D& Goal, float Speed, float DeltaSeconds);
	/** Along Path, then; true at its end. */
	bool FollowPath(FMan& Man, float Speed, float DeltaSeconds);
	/** Works out a way along the streets from where he is to Goal. */
	void PlanPath(FMan& Man, const FVector2D& Goal);
	void UpdateGround(FMan& Man, float DeltaSeconds);
	FVector MiddleOf(const FMan& Man) const;
	bool CanSee(const FMan& Man, const FVector& Target) const;

	void StartChase(FPatrol& Patrol, bool bSawIt);
	void SendToPhone(FPatrol& Patrol, int32 Which, bool bForHelp);
	void BackToBeat(FPatrol& Patrol, int32 Which);
	void Shout(const FMan& Man, const TCHAR* Hu, const TCHAR* En);

	TArray<FLine> Lines;
	/** Line ends on a coarse grid: cell -> (line, end: 0 start, 1 end). */
	TMultiMap<FIntPoint, FIntPoint> Ends;
	/** The boxes from the file: where they stand and their names. */
	TArray<FVector> BoxPlaces;
	TArray<FString> BoxNames;
	UPROPERTY(Transient)
	TArray<TObjectPtr<APoliceBox>> Boxes;
	TArray<FPatrol> Patrols;
	TWeakObjectPtr<ATrafficManager> Traffic;
	FRandomStream Random;
	bool bReady = false;
	bool bWasWanted = false;
	double LastShout = -10.0;

	/** The player (or the car he drives), and how fast it goes. */
	TWeakObjectPtr<AActor> LastPlayer;
	FVector PlayerAt = FVector::ZeroVector;
	FVector LastPlayerAt = FVector::ZeroVector;
	FVector PlayerVelocity = FVector::ZeroVector;
	bool bPlayerDriving = false;
};
