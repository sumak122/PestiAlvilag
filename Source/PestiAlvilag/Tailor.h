// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Tailor.generated.h"

class AShapeSet;
class AStoryPerson;

/**
 * Kertész és Fia, the gentlemen's tailor on Bérkocsis utca (TailorModel.h):
 * the shop the player walks into, and old Kertész behind the counter. At
 * the counter E buys the suit (TailorModel::SuitPrice), which the player
 * wears from then on (game mode HasSuit, saved); the Tattersall's doorman
 * lets nobody in without one.
 *
 * The actor stands at the middle of the shop's door on the house front, on
 * the ground, turned as the shop's frame (+X out to the street).
 */
UCLASS()
class PESTIALVILAG_API ATailor : public AActor
{
	GENERATED_BODY()

public:
	ATailor();

	virtual void Tick(float DeltaSeconds) override;

	/** Where the player stands to buy, in the city (on the floor), or zero when there is no tailor. */
	static FVector BuySpot(const UWorld* World);

protected:
	virtual void BeginPlay() override;

private:
	void KerteszSays(const FString& Hu, const FString& En) const;

	UPROPERTY(Transient)
	TObjectPtr<AShapeSet> Shop;

	UPROPERTY(Transient)
	TObjectPtr<AStoryPerson> Kertesz;

	/** The player was at the counter last frame (he greets him once each time). */
	bool bAtCounter = false;
	/** When the "the price" note was last shown (s). */
	double LastNote = -100.0;
};
