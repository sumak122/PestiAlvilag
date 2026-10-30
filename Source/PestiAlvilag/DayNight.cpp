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
#include "GroundSurfaces.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "OsmGraybox.h"
#include "ProceduralMeshComponent.h"
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

	// Street lamps: the nearest light the ground, out to 110 m round the
	// camera, fading out over the last 30 m; each makes the pavement under it
	// about twice as bright as a lit grey wall. Beyond that a glow on the
	// ground under each lamp fades in over the same 30 m, so no lamp ever
	// pops on, however far away.
	constexpr int32 LampLightCount = 56;
	constexpr float LampReach = 1800.f;
	constexpr float LampStrength = 56.f;
	constexpr double LampRange = 11000.0;
	constexpr double LampFade = 8000.0;
	// The glow sits this far above the ground (over the kerbs and rails)...
	constexpr double LightPoolLift = 20.0;
	// ...under lanterns about this high (m)...
	constexpr float LanternHeight = 4.f;
	// ...on ground that sends back this much of the light.
	constexpr float LitGroundShare = 0.25f;
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
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Translucent(TEXT("/Engine/EngineMaterials/Widget3DPassThrough_Translucent.Widget3DPassThrough_Translucent"));
	SeeThroughBase = Translucent.Object;
}

ADayNight* ADayNight::Get(UWorld* World)
{
	static TWeakObjectPtr<ADayNight> Found;
	// Only in the game: not for things built in the editor.
	if (!World || !World->IsGameWorld())
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

	FindSeeThroughs();
	UpdateSun(true);
	UpdateGlows(true);
	UpdateSeeThrough(true);
}

void ADayNight::FindSeeThroughs()
{
	// Things built in the editor (a station placed in the level) made their
	// glass before there was a clock to tell, so look for it now.
	if (!SeeThroughBase)
	{
		return;
	}
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		TInlineComponentArray<UProceduralMeshComponent*> Meshes(*It);
		for (UProceduralMeshComponent* Mesh : Meshes)
		{
			for (int32 Slot = 0; Slot < Mesh->GetNumMaterials(); ++Slot)
			{
				UMaterialInstanceDynamic* Material = Cast<UMaterialInstanceDynamic>(Mesh->GetMaterial(Slot));
				FLinearColor Tint;
				if (Material && Material->Parent == SeeThroughBase
					&& Material->GetVectorParameterValue(FMaterialParameterInfo(FName(TEXT("TintColorAndOpacity"))), Tint))
				{
					AddSeeThrough(Material, Tint);
				}
			}
		}
	}
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
	UpdateSeeThrough(true);
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
		UpdateSeeThrough(false);
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

void ADayNight::AddSeeThrough(UMaterialInstanceDynamic* Material, const FLinearColor& Tint)
{
	if (!Material)
	{
		return;
	}
	for (const FSeeThrough& Known : SeeThroughs)
	{
		if (Known.Material == Material)
		{
			return;
		}
	}
	FSeeThrough& Glass = SeeThroughs.AddDefaulted_GetRef();
	Glass.Material = Material;
	Glass.Tint = Tint;
	ShadeSeeThrough(Glass);
}

void ADayNight::ShadeSeeThrough(const FSeeThrough& Glass) const
{
	if (UMaterialInstanceDynamic* Material = Glass.Material.Get())
	{
		// As much darker than by day as a lit grey wall looks now.
		const float Shade = GreyLuminance() / (DayNight::DayGrey * DaySunIntensity);
		FLinearColor Tint = Glass.Tint * Shade;
		Tint.A = Glass.Tint.A;
		Material->SetVectorParameterValue(TEXT("TintColorAndOpacity"), Tint);
	}
}

void ADayNight::UpdateSeeThrough(bool bForce)
{
	if (!bForce && FMath::Abs(Darkness - LastShadeDarkness) < 0.01f)
	{
		return;
	}
	LastShadeDarkness = Darkness;
	for (int32 i = SeeThroughs.Num() - 1; i >= 0; --i)
	{
		if (!SeeThroughs[i].Material.IsValid())
		{
			SeeThroughs.RemoveAtSwap(i);
			continue;
		}
		ShadeSeeThrough(SeeThroughs[i]);
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
		if (LightPools)
		{
			LightPools->SetVisibility(false);
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

	// The far lamps' glow: as bright in the middle as a real lamp makes the
	// ground straight under it.
	BuildLightPools();
	if (LightPools && LightPoolMaterial)
	{
		const float Middle = Candela / FMath::Square(LanternHeight) * LitGroundShare / UE_PI;
		LightPoolMaterial->SetVectorParameterValue(TEXT("Light"), GasLight * Middle);
		LightPools->SetVisibility(true);
	}
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

void ADayNight::BuildLightPools()
{
	using namespace DayNight;
	if (bLightPoolsBuilt)
	{
		return;
	}
	bLightPoolsBuilt = true;
	LightPoolMaterial = GroundSurfaces::MakeLightPool(this);
	if (!LightPoolMaterial)
	{
		return;
	}
	LightPoolMaterial->SetScalarParameterValue(TEXT("FadeFrom"), static_cast<float>(LampFade));
	LightPoolMaterial->SetScalarParameterValue(TEXT("FadeLength"), static_cast<float>(LampRange - LampFade));

	// The ground under each lamp, from the city's own ground (a plain trace
	// would hit the lamp post).
	TArray<UPrimitiveComponent*> Grounds;
	for (TActorIterator<AOsmGraybox> It(GetWorld()); It; ++It)
	{
		TInlineComponentArray<UPrimitiveComponent*> Parts(*It);
		Grounds.Append(Parts);
	}

	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> Corners;
	const double R = GroundSurfaces::LightPoolRadius;
	const FCollisionQueryParams Params(SCENE_QUERY_STAT(LightPoolGround), false, this);
	for (const FVector& Lamp : Lamps)
	{
		double GroundZ = Lamp.Z - LanternHeight * 100.0;
		for (UPrimitiveComponent* Ground : Grounds)
		{
			FHitResult Hit;
			if (Ground->LineTraceComponent(Hit, Lamp, Lamp - FVector(0.0, 0.0, 2000.0), Params))
			{
				GroundZ = Hit.ImpactPoint.Z;
				break;
			}
		}
		const FVector Middle(Lamp.X, Lamp.Y, GroundZ + LightPoolLift);
		const int32 First = Vertices.Num();
		Vertices.Add(Middle + FVector(-R, -R, 0.0));
		Vertices.Add(Middle + FVector(R, -R, 0.0));
		Vertices.Add(Middle + FVector(R, R, 0.0));
		Vertices.Add(Middle + FVector(-R, R, 0.0));
		Corners.Add(FVector2D(0.0, 0.0));
		Corners.Add(FVector2D(1.0, 0.0));
		Corners.Add(FVector2D(1.0, 1.0));
		Corners.Add(FVector2D(0.0, 1.0));
		for (int32 i = 0; i < 4; ++i)
		{
			Normals.Add(FVector::UpVector);
		}
		// Facing up: Unreal faces a triangle along (C - A) x (B - A).
		Triangles.Append({ First, First + 2, First + 1, First, First + 3, First + 2 });
	}

	LightPools = NewObject<UProceduralMeshComponent>(this);
	LightPools->SetupAttachment(RootComponent);
	LightPools->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	LightPools->SetCastShadow(false);
	LightPools->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, Corners, TArray<FLinearColor>(), TArray<FProcMeshTangent>(), false);
	LightPools->SetMaterial(0, LightPoolMaterial);
	LightPools->SetVisibility(false);
	LightPools->RegisterComponent();
}
