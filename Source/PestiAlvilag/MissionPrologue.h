// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Mission.h"
#include "MissionPrologue.generated.h"

class AStoryPerson;

/**
 * Act 1, mission 1, "Megérkezés" (Arrival). The player steps off the train
 * at Keleti station in the autumn of 1932 and makes their own way to Bözsi
 * néni's tenement on Népszínház utca, which becomes home. An old gentleman
 * waits on the platform with his suitcase, only as part of the scene.
 * Teaches walking, following the goal marker and saving at home.
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
	virtual void OnStepDone(int32 Step) override;

private:
	UPROPERTY(Transient)
	TObjectPtr<AStoryPerson> Gentleman;
};
