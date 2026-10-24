// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

class AActor;
class UMaterialInterface;
class USceneComponent;
class UStaticMeshComponent;

/**
 * The shared look of the prototype's cars: an early 1930s saloon built from
 * engine shapes. Long bonnet with grille and round headlamps at the front,
 * cabin with dark windows at the back, spare wheel and red tail lamps on the
 * rear, so you can tell which way a car faces. Fits a 420 x 170 x 140 cm box
 * centred on the parent, with the ground 90 cm below the centre.
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

	/** Call from an actor constructor. Adds the parts under Parent. */
	void Build(AActor* Owner, USceneComponent* Parent, TArray<TObjectPtr<UStaticMeshComponent>>& OutParts, TArray<EPaint>& OutPaints);

	/**
	 * Call from an actor constructor, after Build: taxi markings on the
	 * saloon. A yellow and black chequered band round the cabin, a yellow
	 * roof lamp and the meter's red "free" flag by the driver's door.
	 */
	void BuildTaxi(AActor* Owner, USceneComponent* Parent, TArray<TObjectPtr<UStaticMeshComponent>>& OutParts, TArray<EPaint>& OutPaints);

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
