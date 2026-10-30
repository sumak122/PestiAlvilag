// Fill out your copyright notice in the Description page of Project Settings.

#include "DayNight.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/MeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/SkyLight.h"
#include "Engine/Texture.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace DayNight
{
	// A whole day lasts this many minutes of play.
	constexpr float DayMinutes = 48.f;
	constexpr float TimeScale = 24.f * 60.f / DayMinutes;
	// The game starts in the late afternoon, so the first evening comes soon.
	constexpr float StartHour = 16.f;

	// An October day in Pest: the sun rises at half past six in the east,
	// is 35 degrees up at noon in the south and sets at half past five in
	// the west. Elevation = SunHeight * (cos(hour angle) - SunOffset).
	constexpr float SunRise = 6.5f;
	constexpr float NoonHeight = 35.f;
	const float SunOffset = FMath::Cos(UE_PI * (12.f - SunRise) / 12.f);
	const float SunHeight = NoonHeight / (1.f - SunOffset);

	// The moon stands high in the south-west all night (its light shines north-east).
	const FRotator MoonRotation(-38.f, -45.f, 0.f);
	const FLinearColor MoonColor(0.55f, 0.65f, 1.f);
	constexpr float MoonShare = 0.012f;

	// The sun is low and warm at dawn and dusk.
	const FLinearColor LowSunColor(1.f, 0.62f, 0.38f);

	// Darkness rises as the sun sinks from 3 degrees above the horizon to 6
	// below; the lamps are lit from 0.3 and put out again below 0.22.
	constexpr float DarkFrom = 3.f;
	constexpr float DarkTo = -6.f;
	constexpr float LampsOn = 0.3f;
	constexpr float LampsOff = 0.22f;

	// At night the eye (the camera's exposure) settles this many stops
	// below the day, so the night stays dark instead of being brightened up.
	constexpr float NightStopsMin = 4.5f;
	constexpr float NightStopsMax = 2.5f;

	// How bright a lit grey wall looks, as a share of the sun (lux): by day,
	// and at night once the exposure has settled (see NightStops).
	constexpr float DayGrey = 0.12f;
	const float NightGrey = 0.125f * FMath::Pow(2.f, -NightStopsMin);

	// The sky light and fog by night, as a share of the day.
	constexpr float NightSky = 0.5f;
	constexpr float NightFog = 0.04f;

	// Street lamps: the nearest few light the ground; each makes the pavement
	// under it about twice as bright as a lit grey wall.
	constexpr int32 LampLightCount = 16;
	constexpr float LampReach = 1800.f;
	constexpr float LampStrength = 56.f;
	constexpr double LampRange = 5000.0;
	constexpr double LampFade = 3500.0;
	const FLinearColor GasLight(1.f, 0.72f, 0.42f);

	// The sun is only turned when it has moved this far (degrees), as
	// turning it redraws all the shadows.
	constexpr float SunStep = 0.2f;
}

ADayNight::ADayNight()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	// An unlit material whose tint is its colour: brighter than white glows.
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Unlit(TEXT("/Engine/EngineMaterials/Widget3DPassThrough_Opaque.Widget3DPassThrough_Opaque"));
	GlowBase = Unlit.Object;
	static ConstructorHelpers::FObjectFinder<UTexture> White(TEXT("/Engine/EngineResources/WhiteSquareTexture.WhiteSquareTexture"));
	WhiteTexture = White.Object;
}

ADayNight* ADayNight::Get(UWorld* World)
{
	static TWeakObjectPtr<ADayNight> Found;
	if (!World)
	{
		return nullptr;
	}
	if (Found.IsValid() && Found->GetWorld() == World)
	{
		return Found.Get();
	}
	for (TActorIterator<ADayNight> It(World); It; ++It)
	{
		Found = *It;
		return *It;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ADayNight* Made = World->SpawnActor<ADayNight>(ADayNight::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Params);
	Found = Made;
	return Made;
}

void ADayNight::BeginPlay()
{
	Super::BeginPlay();
	Hour = DayNight::StartHour;
	FindSky();

	for (int32 i = 0; i < DayNight::LampLightCount; ++i)
	{
		UPointLightComponent* Light = NewObject<UPointLightComponent>(this);
		Light->SetupAttachment(RootComponent);
		Light->SetMobility(EComponentMobility::Movable);
		Light->SetCastShadows(false);
		Light->SetIntensityUnits(ELightUnits::Candelas);
		Light->SetAttenuationRadius(DayNight::LampReach);
		Light->SetLightColor(DayNight::GasLight);
		Light->SetVisibility(false);
		Light->RegisterComponent();
		LampLights.Add(Light);
	}

	UpdateSun(true);
	UpdateGlows(true);
}

void ADayNight::FindSky()
{
	UWorld* World = GetWorld();
	for (TActorIterator<ADirectionalLight> It(World); It; ++It)
	{
		Sun = *It;
		break;
	}
	if (!Sun)
	{
		Sun = World->SpawnActor<ADirectionalLight>(ADirectionalLight::StaticClass(), FVector(0.0, 0.0, 10000.0), FRotator(-40.f, 0.f, 0.f));
	}
	if (Sun)
	{
		UDirectionalLightComponent* Light = Cast<UDirectionalLightComponent>(Sun->GetLightComponent());
		if (Light)
		{
			Light->SetMobility(EComponentMobility::Movable);
			Light->SetAtmosphereSunLight(true);
			DaySunIntensity = FMath::Max(Light->Intensity, 0.01f);
			DaySunColor = Light->GetLightColor();
		}
	}

	// A sky that changes colour with the sun.
	if (!TActorIterator<ASkyAtmosphere>(World))
	{
		World->SpawnActor<ASkyAtmosphere>(ASkyAtmosphere::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
	}

	for (TActorIterator<ASkyLight> It(World); It; ++It)
	{
		Sky = *It;
		break;
	}
	if (!Sky)
	{
		Sky = World->SpawnActor<ASkyLight>(ASkyLight::StaticClass(), FVector(0.0, 0.0, 5000.0), FRotator::ZeroRotator);
	}
	if (Sky && Sky->GetLightComponent())
	{
		USkyLightComponent* Light = Sky->GetLightComponent();
		Light->SetMobility(EComponentMobility::Movable);
		// It follows the sky as it darkens.
		Light->bRealTimeCapture = true;
		Light->MarkRenderStateDirty();
		DaySkyIntensity = Light->Intensity;
	}

	for (TActorIterator<AExponentialHeightFog> It(World); It; ++It)
	{
		Fog = *It;
		break;
	}
	if (Fog && Fog->GetComponent())
	{
		DayFogColor = Fog->GetComponent()->FogInscatteringLuminance;
	}

	// The night's exposure: blended in with the darkness, so the day is untouched.
	NightExposure = World->SpawnActor<APostProcessVolume>(APostProcessVolume::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
	if (NightExposure)
	{
		const float DayStops = FMath::Log2(DaySunIntensity);
		NightExposure->bUnbound = true;
		NightExposure->Priority = 10.f;
		NightExposure->BlendWeight = 0.f;
		FPostProcessSettings& Settings = NightExposure->Settings;
		Settings.bOverride_AutoExposureMinBrightness = true;
		Settings.AutoExposureMinBrightness = DayStops - DayNight::NightStopsMin;
		Settings.bOverride_AutoExposureMaxBrightness = true;
		Settings.AutoExposureMaxBrightness = DayStops - DayNight::NightStopsMax;
	}
}

float ADayNight::SunElevation(float AtHour) const
{
	return DayNight::SunHeight * (FMath::Cos(UE_PI * (AtHour - 12.f) / 12.f) - DayNight::SunOffset);
}

float ADayNight::GreyLuminance() const
{
	// Between the day and the night on a log scale, as the exposure changes.
	const float Day = DayNight::DayGrey * DaySunIntensity;
	const float Night = DayNight::NightGrey * DaySunIntensity;
	return FMath::Exp(FMath::Lerp(FMath::Loge(Day), FMath::Loge(Night), Darkness));
}

float ADayNight::LampCandela(float Strength) const
{
	// A grey wall (30% of the light back) lit by E lux looks 0.3 E / pi bright.
	return Strength * DayNight::NightGrey * DaySunIntensity * UE_PI / 0.3f;
}

FString ADayNight::GetClockText() const
{
	const int32 Minutes = FMath::FloorToInt(Hour * 60.f);
	return FString::Printf(TEXT("%d:%02d"), (Minutes / 60) % 24, Minutes % 60);
}

void ADayNight::SkipHour()
{
	Hour = FMath::Fmod(Hour + 1.f, 24.f);
	UpdateSun(true);
	UpdateGlows(true);
}

void ADayNight::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Hour = FMath::Fmod(Hour + DeltaSeconds * DayNight::TimeScale / 3600.f, 24.f);
	UpdateSun(false);

	GlowTimer -= DeltaSeconds;
	if (GlowTimer <= 0.f)
	{
		GlowTimer = 0.5f;
		UpdateGlows(false);
	}
	LampTimer -= DeltaSeconds;
	if (LampTimer <= 0.f)
	{
		LampTimer = 0.3f;
		UpdateLampLights();
	}
}

void ADayNight::UpdateSun(bool bForce)
{
	using namespace DayNight;
	const float Elevation = SunElevation(Hour);
	Darkness = FMath::Clamp((DarkFrom - Elevation) / (DarkFrom - DarkTo), 0.f, 1.f);

	if (Sun)
	{
		UDirectionalLightComponent* Light = Cast<UDirectionalLightComponent>(Sun->GetLightComponent());
		// From the east at sunrise (yaw 0, X is east), through the south
		// (+Y) at noon, to the west. The light shines away from the sun.
		const float Azimuth = 90.f + (Hour - 12.f) * 15.f;
		const bool bMoon = Elevation < -8.f;
		const FRotator Wanted = bMoon ? MoonRotation : FRotator(-Elevation, Azimuth + 180.f, 0.f);
		const bool bTurn = bForce || !Wanted.Equals(LastSunRotation, SunStep);
		if (bTurn)
		{
			Sun->SetActorRotation(Wanted);
			LastSunRotation = Wanted;
		}
		if (Light)
		{
			if (bMoon)
			{
				const float Rise = FMath::Clamp((-10.f - Elevation) / 4.f, 0.f, 1.f);
				Light->SetIntensity(DaySunIntensity * MoonShare * Rise);
				Light->SetLightColor(MoonColor);
			}
			else
			{
				Light->SetIntensity(DaySunIntensity * FMath::Clamp((Elevation + 6.f) / 8.f, 0.f, 1.f));
				Light->SetLightColor(FMath::Lerp(LowSunColor, DaySunColor, FMath::Clamp(Elevation / 20.f, 0.f, 1.f)));
			}
		}
	}

	if (NightExposure)
	{
		NightExposure->BlendWeight = Darkness;
	}
	if (Sky && Sky->GetLightComponent())
	{
		Sky->GetLightComponent()->SetIntensity(DaySkyIntensity * FMath::Lerp(1.f, NightSky, Darkness));
	}
	if (Fog && Fog->GetComponent())
	{
		Fog->GetComponent()->SetFogInscatteringColor(DayFogColor * FMath::Lerp(1.f, NightFog, Darkness));
	}
}

UMaterialInstanceDynamic* ADayNight::MakeGlowMaterial(UObject* Outer) const
{
	if (!GlowBase)
	{
		return nullptr;
	}
	UMaterialInstanceDynamic* Glow = UMaterialInstanceDynamic::Create(GlowBase, Outer);
	if (WhiteTexture)
	{
		Glow->SetTextureParameterValue(TEXT("SlateUI"), WhiteTexture);
	}
	Glow->SetScalarParameterValue(TEXT("OpacityFromTexture"), 0.f);
	return Glow;
}

void ADayNight::SetGlow(UMaterialInstanceDynamic* Glow, const FLinearColor& Color, float Strength) const
{
	if (Glow)
	{
		FLinearColor Tint = Color * (Strength * GreyLuminance());
		Tint.A = 1.f;
		Glow->SetVectorParameterValue(TEXT("TintColorAndOpacity"), Tint);
	}
}

void ADayNight::AddNightGlow(UMeshComponent* Mesh, int32 Slot, const FLinearColor& Color, float Strength)
{
	if (!Mesh || !GlowBase)
	{
		return;
	}
	FNightGlow& Glow = Glows.AddDefaulted_GetRef();
	Glow.Mesh = Mesh;
	Glow.Slot = Slot;
	Glow.Day = Mesh->GetMaterial(Slot);
	Glow.Glow = MakeGlowMaterial(this);
	Glow.Color = Color;
	Glow.Strength = Strength;
	if (bLampsLit)
	{
		SetGlow(Glow.Glow, Color, Strength);
		Mesh->SetMaterial(Slot, Glow.Glow);
	}
}

void ADayNight::UpdateGlows(bool bForce)
{
	const bool bWasLit = bLampsLit;
	bLampsLit = Darkness > (bLampsLit ? DayNight::LampsOff : DayNight::LampsOn);
	const bool bSwitch = bForce || bLampsLit != bWasLit;
	if (!bSwitch && (!bLampsLit || FMath::Abs(Darkness - LastGlowDarkness) < 0.01f))
	{
		return;
	}
	LastGlowDarkness = Darkness;

	for (int32 i = Glows.Num() - 1; i >= 0; --i)
	{
		FNightGlow& Glow = Glows[i];
		UMeshComponent* Mesh = Glow.Mesh.Get();
		if (!Mesh)
		{
			Glows.RemoveAtSwap(i);
			continue;
		}
		if (bLampsLit)
		{
			SetGlow(Glow.Glow, Glow.Color, Glow.Strength);
		}
		if (bSwitch)
		{
			Mesh->SetMaterial(Glow.Slot, bLampsLit ? Glow.Glow.Get() : Glow.Day.Get());
		}
	}
}

void ADayNight::UpdateLampLights()
{
	using namespace DayNight;
	const APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0);
	if (!bLampsLit || !Camera || Lamps.Num() == 0)
	{
		for (UPointLightComponent* Light : LampLights)
		{
			Light->SetVisibility(false);
		}
		return;
	}

	// The lamps nearest the camera, closest first.
	const FVector Eye = Camera->GetCameraLocation();
	TArray<TPair<double, int32>> Near;
	for (int32 i = 0; i < Lamps.Num(); ++i)
	{
		const double Distance = FVector::Dist2D(Lamps[i], Eye);
		if (Distance < LampRange)
		{
			Near.Emplace(Distance, i);
		}
	}
	Near.Sort([](const TPair<double, int32>& A, const TPair<double, int32>& B) { return A.Key < B.Key; });

	const float Candela = LampCandela(LampStrength) * FMath::Clamp((Darkness - LampsOff) / (1.f - LampsOff), 0.2f, 1.f);
	for (int32 i = 0; i < LampLights.Num(); ++i)
	{
		UPointLightComponent* Light = LampLights[i];
		if (!Near.IsValidIndex(i))
		{
			Light->SetVisibility(false);
			continue;
		}
		// Far ones fade out, so a lamp doesn't pop on as another drops off the list.
		const float Fade = 1.f - FMath::Clamp(static_cast<float>((Near[i].Key - LampFade) / (LampRange - LampFade)), 0.f, 1.f);
		Light->SetWorldLocation(Lamps[Near[i].Value] - FVector(0.0, 0.0, 15.0));
		Light->SetIntensity(Candela * Fade);
		Light->SetVisibility(true);
	}
}
