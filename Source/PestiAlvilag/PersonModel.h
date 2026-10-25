// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PersonKit.h"

class AActor;
class UMaterialInterface;
class UProceduralMeshComponent;
class USceneComponent;

/**
 * Puts a person from the dress-up kit (PersonKit) into the game: five mesh
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
	 * feet: the legs, arms and body, each placed at its hip or shoulder so it
	 * swings from there (pitch forward). Any pieces already in InOutPieces are
	 * removed first. The pieces come in PersonKit::EPiece order: left leg,
	 * right leg, left arm, right arm, body.
	 */
	void Dress(AActor* Owner, USceneComponent* Parent, UMaterialInterface* BaseMaterial, const PersonKit::FOutfit& Outfit,
		TArray<TObjectPtr<UProceduralMeshComponent>>& InOutPieces);

	/** Someone out on the streets of Pest in the autumn of 1932. */
	PersonKit::FOutfit RandomOutfit(FRandomStream& Random);

	/** The player: a dark overcoat, a grey fedora and a red scarf. */
	PersonKit::FOutfit PlayerOutfit();
}
