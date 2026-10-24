// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "PestiAlvilagGameModeBase.generated.h"

/**
 * 
 */
UCLASS()
class PESTIALVILAG_API APestiAlvilagGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	APestiAlvilagGameModeBase();

	virtual void StartPlay() override;
};
