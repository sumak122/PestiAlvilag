// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AnimalModel.h"
#include "Animals.generated.h"

class UAnimalSound;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UProceduralMeshComponent;
class USceneComponent;

/**
 * The cats and dogs of the streets and yards (AnimalModel.h): Lafi and
 * Bütyök in Laci's yard, stray cats in the yards and markets, a rough mutt
 * or two and a black puli on the squares. Each keeps near its own spot,
 * sits, lies about, gets up and wanders a little. Cats run from a stranger
 * who comes too close (Laci's two are used to people); dogs watch him, bark
 * and growl. Nothing ever hurts them: from a car coming at them they leap
 * clear in time and run off, and they have no collision for anything to hit.
 */
UCLASS()
class PESTIALVILAG_API AAnimals : public AActor
{
	GENERATED_BODY()

public:
	AAnimals();

	virtual void Tick(float DeltaSeconds) override;

	/**
	 * Milk spilt on the road at At (a churn off a bolting cart): a stray cat
	 * or two come out from somewhere out of sight and lap it up for about
	 * Seconds, then wander off again.
	 */
	static void CallToMilk(const UObject* WorldContextObject, const FVector& At, float Seconds);

	/** Animals further than this from the camera are hidden and left still (cm). */
	UPROPERTY(EditAnywhere, Category = "Animals")
	float ShowWithin = 7000.f;

protected:
	virtual void BeginPlay() override;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> BaseMaterial;

	/** Their voices: moved to whichever animal barks or meows. */
	UPROPERTY(Transient)
	TObjectPtr<UAnimalSound> Voice;

private:
	enum class EMood : uint8 { Rest, Walk, Watch, Leap, Flee, Lap };

	struct FPet
	{
		AnimalModel::EKind Kind = AnimalModel::EKind::TabbyCat;
		bool bDog = false;
		/** Lafi and Bütyök: used to people, they don't run from the player. */
		bool bTame = false;
		/** Bütyök: lies about nearly all day. */
		bool bLazy = false;
		USceneComponent* Root = nullptr;
		USceneComponent* Hips = nullptr;
		UProceduralMeshComponent* Parts[AnimalModel::NumParts] = {};
		FVector Home = FVector::ZeroVector;
		double Roam = 300.0;
		FVector At = FVector::ZeroVector;
		float Yaw = 0.f;
		EMood Mood = EMood::Rest;
		AnimalModel::EPose Resting = AnimalModel::EPose::Sit;
		/** Where it walks or runs to, which way, and how fast. */
		FVector Target = FVector::ZeroVector;
		FVector2D Heading = FVector2D(1.0, 0.0);
		float Speed = 0.f;
		/** Seconds left in this mood. */
		float Busy = 0.f;
		/** The walk or run cycle (0..1), and the slow idle sway. */
		float Phase = 0.f;
		float Idle = 0.f;
		/** The leap's height arc: seconds since it jumped. */
		float Hop = -1.f;
		/** The head turned towards the player (degrees, relative to the body). */
		float Look = 0.f;
		/** Seconds till it may bark or meow again. */
		float Quiet = 0.f;
		/** The pose it is melting out of, and how far into the new one (0..1). */
		AnimalModel::FPose From;
		AnimalModel::FPose Last;
		float Blend = 1.f;
		AnimalModel::EPose Showing = AnimalModel::EPose::Sit;
		bool bShown = true;
		/** A stray come for spilt milk: laps it while it lasts, then goes; put away (hidden) once out of sight. */
		bool bVisitor = false;
		bool bAway = false;
		bool bLeaving = false;
		FVector MilkAt = FVector::ZeroVector;
		double MilkUntil = 0.0;
		int32 MilkTries = 0;
		float AwayClock = 0.f;
	};

	/** Spilt milk the strays come to, till that time (world seconds). */
	struct FMilk
	{
		FVector At = FVector::ZeroVector;
		double Until = 0.0;
		int32 Called = 0;
	};

	/** Something that moves and can frighten them: the player (on foot or driving) or a car in the traffic. */
	struct FMover
	{
		TWeakObjectPtr<AActor> Actor;
		FVector At = FVector::ZeroVector;
		FVector2D Velocity = FVector2D::ZeroVector;
		bool bVehicle = false;
		bool bPlayer = false;
	};

	void SetUp();
	void AddPet(AnimalModel::EKind Kind, const FVector& Near, double Roam, bool bTame, bool bLazy);
	bool GroundAt(const FVector& Near, double Below, FVector& OutGround) const;
	bool PathClear(const FVector& From, const FVector& To) const;
	void GatherMovers(float DeltaSeconds);
	void React(FPet& Pet);
	void Think(FPet& Pet);
	void Move(FPet& Pet, float DeltaSeconds);
	void PoseIt(FPet& Pet, float DeltaSeconds);
	void SetMood(FPet& Pet, EMood Mood, float Seconds);
	void RunFrom(FPet& Pet, const FVector2D& Away, bool bLeap);
	void Speak(const FPet& Pet, uint8 Kind, float Strength, float Delay = 0.f);
	/** Brings a stray to a milk spill from out of sight; false when no spot was found. */
	bool SendVisitor(FMilk& Milk);
	/** A visitor's next step: to the milk, lapping, or off and away. */
	void ThinkVisitor(FPet& Pet);
	/** The cat comes back for the milk at this spot (cm), a little way from its middle. */
	FVector LapSpot(const FPet& Pet) const;
	UMaterialInstanceDynamic* Paint(const FLinearColor& Color);

	TMap<FString, UMaterialInstanceDynamic*> Paints;
	TArray<AnimalModel::FAnimal> Models;
	TArray<FPet> Pets;
	TArray<FMover> Movers;
	TMap<TWeakObjectPtr<AActor>, FVector> LastSeen;
	TArray<const AActor*> Ignore;
	TArray<FMilk> Milks;
	float VisitorClock = 0.f;
	FVector Camera = FVector::ZeroVector;
	FVector CameraAhead = FVector::ForwardVector;
	bool bSetUp = false;
	float SetUpWait = 1.f;
	FRandomStream Random;
};
