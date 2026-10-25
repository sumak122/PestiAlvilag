// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FoodMenus.h"
#include "GameFramework/Actor.h"
#include "FoodVendor.generated.h"

class AStoryPerson;
class UMaterialInterface;
class UStaticMesh;

/**
 * A street food seller and his cart or stand: a chestnut roaster with his
 * glowing iron stove, a sausage cart with a steaming kettle, or a pretzel
 * woman with her pole of pretzels and a basket. Placed by the street
 * furniture (food_vendors); AFoodMenus lets the player buy from them.
 *
 * Its frame: +X towards the customers (the pavement), the seller behind
 * the cart.
 */
UCLASS()
class PESTIALVILAG_API AFoodVendor : public AActor
{
	GENERATED_BODY()

public:
	AFoodVendor();

	/** Builds the cart and puts the seller behind it; call right after spawning. */
	void Setup(EFoodMenu InMenu);

	EFoodMenu GetMenu() const { return Menu; }
	/** Where a customer stands to buy (in front of the cart). */
	FVector GetOrderPoint() const;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> BaseMaterial;

	/** Cube, cylinder, sphere and cone from the engine's basic shapes. */
	UPROPERTY()
	TArray<TObjectPtr<UStaticMesh>> Shapes;

	UPROPERTY(Transient)
	TObjectPtr<AStoryPerson> Seller;

private:
	/** One shape (cm, in the vendor's frame; Size is the whole size). */
	void Add(int32 Shape, const FLinearColor& Color, const FVector& At, const FVector& Size,
		const FRotator& Turn = FRotator::ZeroRotator);
	void Sign(const FString& Words, const FVector& At);
	void BuildChestnuts();
	void BuildSausages();
	void BuildPretzels();

	EFoodMenu Menu = EFoodMenu::Chestnuts;
};
