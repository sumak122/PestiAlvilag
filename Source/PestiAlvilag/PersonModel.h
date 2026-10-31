// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PersonKit.h"

class AActor;
class UMaterialInterface;
class UProceduralMeshComponent;
class USceneComponent;

/**
 * Puts a person from the dress-up kit (PersonKit) into the game: eleven mesh
 * pieces made while the game runs, each painted in its outfit's colours.
 */
namespace PersonModel
{
	/** The actor's origin is this far above the feet, as for the old box people (cm). */
	constexpr double MiddleHeight = 90.0;

	/** A seated person's middle (PersonKit's seated build) above the floor, sitting on a 48 cm seat. */
	constexpr double SeatedMiddle = MiddleHeight - PersonKit::SeatDrop + 2.0;

	/**
	 * Dresses a person under Parent, whose origin is MiddleHeight above the
	 * feet: the thighs, upper arms, body and the halves of the skirts, each placed
	 * at its hip or shoulder so it swings from there (pitch forward), and the
	 * shins under the thighs at the knees and the forearms under the upper
	 * arms at the elbows. Any pieces already in InOutPieces
	 * are removed first. The pieces come in PersonKit::EPiece order: left
	 * thigh, right thigh, left arm, right arm, body, left shin, right shin,
	 * left skirt, right skirt, left forearm, right forearm.
	 */
	void Dress(AActor* Owner, USceneComponent* Parent, UMaterialInterface* BaseMaterial, const PersonKit::FOutfit& Outfit,
		TArray<TObjectPtr<UProceduralMeshComponent>>& InOutPieces);

	/**
	 * Swings the legs of a dressed person: each thigh to its pitch (forward
	 * is positive), each knee bent by its own (negative takes the foot back),
	 * and the skirts after the thighs (PersonKit::SkirtFollow).
	 */
	void PoseLegs(const TArray<TObjectPtr<UProceduralMeshComponent>>& Pieces, float Left, float Right, float LeftKnee = 0.f, float RightKnee = 0.f);

	/**
	 * Walking legs at this point of the stride: the left thigh at
	 * Sin(Phase) * Swing, the right against it, and the knees bending with
	 * the stride by KneeAmount (0 standing, 1 walking, more running).
	 */
	void StrideLegs(const TArray<TObjectPtr<UProceduralMeshComponent>>& Pieces, float Phase, float Swing, float KneeAmount);

	/** Bends the elbows of a dressed person (degrees, positive brings the forearm forward and up). */
	void PoseElbows(const TArray<TObjectPtr<UProceduralMeshComponent>>& Pieces, float Left, float Right);

	/** Someone out on the streets of Pest in the autumn of 1932. */
	PersonKit::FOutfit RandomOutfit(FRandomStream& Random);

	/** The player: a dark overcoat, a grey fedora and a red scarf. */
	PersonKit::FOutfit PlayerOutfit();
}
