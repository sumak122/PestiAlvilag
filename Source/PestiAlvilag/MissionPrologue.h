// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Mission.h"
#include "MissionPrologue.generated.h"

class AStoryPerson;
class APestiTaxi;

/**
 * Act 1, mission 1, "Megérkezés" (Arrival). The player steps out of Keleti
 * station in the autumn of 1932, carries a gentleman's suitcase to his taxi
 * on Baross tér for a 50 fillér tip, then walks through the park on Tisza
 * Kálmán tér to Bözsi néni's tenement on Népszínház utca, which becomes
 * home. Teaches walking, following the goal light and saving at home.
 */
UCLASS()
class PESTIALVILAG_API AMissionPrologue : public AMission
{
	GENERATED_BODY()

public:
	AMissionPrologue();

	/** Where a new game starts: in front of Keleti's main doors. */
	static FVector2D StartAt();
	static double StartYaw();

protected:
	UPROPERTY(VisibleAnywhere, Category = "Mission")
	TObjectPtr<UStaticMeshComponent> Suitcase;

	virtual void SetUpMission() override;
	virtual void OnStepStarted(int32 Step) override;
	virtual void OnStepDone(int32 Step) override;

private:
	UPROPERTY(Transient)
	TObjectPtr<AStoryPerson> Gentleman;

	UPROPERTY(Transient)
	TObjectPtr<AStoryPerson> Driver;

	UPROPERTY(Transient)
	TObjectPtr<APestiTaxi> Taxi;
};
