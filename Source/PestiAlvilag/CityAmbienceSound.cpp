// Fill out your copyright notice in the Description page of Project Settings.

#include "CityAmbienceSound.h"

UCityAmbienceSound::UCityAmbienceSound(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bAutoActivate = false;
}

void UCityAmbienceSound::SetScene(const CitySynth::FScene& InScene)
{
	Hour.store(InScene.Hour);
	Darkness.store(InScene.Darkness);
	Park.store(InScene.Park);
	Market.store(InScene.Market);
	Busy.store(InScene.Busy);
	Station.store(InScene.Station);
	StationPan.store(InScene.StationPan);
	Church.store(InScene.Church);
	ChurchPan.store(InScene.ChurchPan);
}

bool UCityAmbienceSound::Init(int32& SampleRate)
{
	NumChannels = 2;
	SampleRateHz = static_cast<float>(FMath::Max(SampleRate, 8000));
	return true;
}

int32 UCityAmbienceSound::OnGenerateAudio(float* OutAudio, int32 NumSamples)
{
	CitySynth::FScene Now;
	Now.Hour = Hour.load();
	Now.Darkness = Darkness.load();
	Now.Park = Park.load();
	Now.Market = Market.load();
	Now.Busy = Busy.load();
	Now.Station = Station.load();
	Now.StationPan = StationPan.load();
	Now.Church = Church.load();
	Now.ChurchPan = ChurchPan.load();
	// Left and right come interleaved, so a block holds half as many frames as samples.
	Voice.Generate(OutAudio, NumSamples / 2, Now, SampleRateHz);
	return NumSamples;
}
