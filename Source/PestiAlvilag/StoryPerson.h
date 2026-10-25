// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PersonKit.h"
#include "StoryPerson.generated.h"

class UCapsuleComponent;
class UMaterialInterface;
class UProceduralMeshComponent;
class USceneComponent;

/**
 * Someone a mission needs: a person from the dress-up kit (a man in a coat
 * and hat unless given another outfit) who can stand, turn to face someone
 * and walk after them. The actor's location is his middle, 90 cm above the
 * ground.
 */
UCLASS()
class PESTIALVILAG_API AStoryPerson : public AActor
{
	GENERATED_BODY()

public:
	AStoryPerson();

	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Person")
	FLinearColor CoatColor = FLinearColor(0.10f, 0.10f, 0.10f);

	UPROPERTY(EditAnywhere, Category = "Person")
	float WalkSpeed = 160.f;

	/** How close he keeps to the one he follows (cm). */
	UPROPERTY(EditAnywhere, Category = "Person")
	float FollowDistance = 250.f;

	/** Repaint the coat (after spawning). */
	void SetCoatColor(const FLinearColor& Color);

	/** Dress him (or her) differently; before or after spawning. */
	void SetOutfit(const PersonKit::FOutfit& InOutfit);
	const PersonKit::FOutfit& GetOutfit() const { return Outfit; }
	/** Walk after Target (null to stand still). */
	void Follow(AActor* Target);
	/** Turn to look at a point. */
	void FaceTowards(const FVector& Point);
	/** Hides him, for example while he sits in a car. */
	void SetInWorld(bool bInWorld);

protected:
	UPROPERTY(VisibleAnywhere, Category = "Person")
	TObjectPtr<UCapsuleComponent> Capsule;

	/** The dressed person: legs, arms and body (PersonKit::EPiece order). */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UProceduralMeshComponent>> Pieces;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> BaseMaterial;

	/** What he wears; subclasses set it in their constructor. */
	PersonKit::FOutfit Outfit;

	virtual void BeginPlay() override;

	/** A limb to swing from its hip or shoulder: 0 left leg, 1 right leg, 2 left arm, 3 right arm. */
	USceneComponent* Limb(int32 Index) const;

private:
	void SwingLimbs(float Speed, float DeltaSeconds);

	UPROPERTY(Transient)
	TObjectPtr<AActor> FollowTarget;

	float StridePhase = 0.f;
};
