// Fill out your copyright notice in the Description page of Project Settings.


#include "PestiAlvilagGameModeBase.h"

#include "EngineUtils.h"
#include "PestiCar.h"
#include "TrafficManager.h"

APestiAlvilagGameModeBase::APestiAlvilagGameModeBase()
{
	DefaultPawnClass = APestiCar::StaticClass();
}

void APestiAlvilagGameModeBase::StartPlay()
{
	Super::StartPlay();

	// Start traffic unless the level already has its own manager.
	if (!TActorIterator<ATrafficManager>(GetWorld()))
	{
		GetWorld()->SpawnActor<ATrafficManager>();
	}
}
