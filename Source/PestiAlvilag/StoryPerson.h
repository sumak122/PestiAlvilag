// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CarBody.h"
#include "GameFramework/Actor.h"
#include "StoryPerson.generated.h"

class UCapsuleComponent;
class UMaterialInterface;
class UStaticMeshComponent;

/**
 * Someone a mission needs: a man in a coat and hat who can stand, turn to
 * face someone and walk after them. The actor's location is his middle,
 * 90 cm above the ground.
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
	/** Walk after Target (null to stand still). */
	void Follow(AActor* Target);
	/** Turn to look at a point. */
	void FaceTowards(const FVector& Point);
	/** Hides him, for example while he sits in a car. */
	void SetInWorld(bool bInWorld);

protected:
	UPROPERTY(VisibleAnywhere, Category = "Person")
	TObjectPtr<UCapsuleComponent> Capsule;

	UPROPERTY(VisibleAnywhere, Category = "Person")
	TArray<TObjectPtr<UStaticMeshComponent>> BodyParts;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> BaseMaterial;

	virtual void BeginPlay() override;

private:
	void SwingLimbs(float Speed, float DeltaSeconds);

	UPROPERTY(Transient)
	TObjectPtr<AActor> FollowTarget;

	TArray<CarBody::EPaint> BodyPaints;
	float StridePhase = 0.f;
};
