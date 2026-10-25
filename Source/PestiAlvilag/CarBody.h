// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

class AActor;
class UMaterialInterface;
class USceneComponent;
class UStaticMeshComponent;

/**
 * Simple shapes built from engine parts (an old tram and a box man), and
 * the paint material every shape in the game is coloured from. The cars
 * are made by CarModel and CarVisuals.
 */
namespace CarBody
{
	enum class EPaint : uint8
	{
		Body,
		Tyre,
		Chrome,
		Glass,
		Headlamp,
		TailLamp,
		Skin,
		Trousers,
		Accent,
	};

	/** Call from an actor constructor. A 10 x 2.2 x 3.5 m tram, ground 165 cm below the parent's centre. */
	void BuildTram(AActor* Owner, USceneComponent* Parent, TArray<TObjectPtr<UStaticMeshComponent>>& OutParts, TArray<EPaint>& OutPaints);

	/**
	 * Call from an actor constructor. A man in a long coat and hat, 1.85 m
	 * tall, ground 90 cm below the parent's centre, facing +X. Limb parts
	 * come first, in the order left leg, right leg, left arm, right arm.
	 */
	void BuildPerson(AActor* Owner, USceneComponent* Parent, TArray<TObjectPtr<UStaticMeshComponent>>& OutParts, TArray<EPaint>& OutPaints);

	/** Call from an actor constructor: the material every part is painted from. */
	UMaterialInterface* LoadPaintMaterial();

	/** Call at runtime (BeginPlay or later). */
	void Paint(AActor* Owner, UMaterialInterface* BaseMaterial, const TArray<TObjectPtr<UStaticMeshComponent>>& Parts,
		const TArray<EPaint>& Paints, const FLinearColor& BodyColor);
}
