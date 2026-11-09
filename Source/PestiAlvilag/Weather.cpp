// Fill out your copyright notice in the Description page of Project Settings.

#include "Weather.h"

#include "Camera/PlayerCameraManager.h"
#include "DayNight.h"
#include "Engine/Texture.h"
#include "Engine/World.h"
#include "GroundSurfaces.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"
#include "RainSound.h"
#include "UObject/ConstructorHelpers.h"

namespace Weather
{
	// The first spell after the game starts is always clear, this long (s).
	constexpr float FirstClear = 300.f;
	// Each spell lasts this long (s).
	constexpr float MinSpell = 180.f;
	constexpr float MaxSpell = 420.f;
	// How fast the sky turns (share a second): slowly by itself, quickly when set from the test page.
	constexpr float Turning = 1.f / 45.f;
	constexpr float TurningForced = 1.f / 3.f;
	// Rain only starts once the cloud is this thick.
	constexpr float RainNeedsCloud = 0.6f;

	// Raindrops: how many at full rain, the box round the camera they fall through (cm),
	// how fast they fall, how long and wide a streak is.
	constexpr int32 DropCount = 1600;
	constexpr double BoxHalf = 2200.0;
	constexpr double BoxBelow = 700.0;
	constexpr double BoxAbove = 1700.0;
	constexpr double FallSpeed = 1100.0;
	const FVector Wind(140.0, 60.0, 0.0);
	constexpr double StreakLength = 75.0;
	constexpr double StreakWidth = 1.4;
	// Pale see-through grey: colour and how much it hides.
	const FLinearColor DropTint(0.72f, 0.75f, 0.80f, 0.35f);
	constexpr float RainVolume = 0.8f;

	// The streets: in rain they get as wet as WetFloor plus the rest by how
	// hard it rains, soaking up in about SoakSeconds and drying in about
	// DrySeconds after it stops (quicker when set from the test page); the
	// ground is told again once it changed by WetStep.
	constexpr float WetFloor = 0.55f;
	constexpr float SoakSeconds = 40.f;
	constexpr float DrySeconds = 240.f;
	constexpr float ForcedSoakSeconds = 6.f;
	constexpr float ForcedDrySeconds = 30.f;
	constexpr float WetStep = 0.01f;
}

AWeather::AWeather()
{
	PrimaryActorTick.bCanEverTick = true;

	Mesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCanEverAffectNavigation(false);
	// Left out of the ray-traced lighting, the distance fields and the shadows:
	// it changes every frame, and rebuilding it there each time costs the
	// graphics card far more than it is worth.
	Mesh->SetCastShadow(false);
	Mesh->bVisibleInRayTracing = false;
	Mesh->bAffectDistanceFieldLighting = false;
	Mesh->bAffectDynamicIndirectLighting = false;

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Translucent(TEXT("/Engine/EngineMaterials/Widget3DPassThrough_Translucent.Widget3DPassThrough_Translucent"));
	SeeThroughMaterial = Translucent.Object;
	static ConstructorHelpers::FObjectFinder<UTexture> White(TEXT("/Engine/EngineResources/WhiteSquareTexture.WhiteSquareTexture"));
	WhiteTexture = White.Object;
}

AWeather* AWeather::Get(UWorld* World)
{
	static TWeakObjectPtr<AWeather> Found;
	if (!World)
	{
		return nullptr;
	}
	if (Found.IsValid() && Found->GetWorld() == World)
	{
		return Found.Get();
	}
	for (TActorIterator<AWeather> It(World); It; ++It)
	{
		Found = *It;
		return *It;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AWeather* Made = World->SpawnActor<AWeather>(AWeather::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Params);
	Found = Made;
	return Made;
}

void AWeather::BeginPlay()
{
	Super::BeginPlay();

	// The mesh is in world space; the actor stays at the origin.
	SetActorLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);
	Random.Initialize(FMath::Rand());
	SpellLeft = Weather::FirstClear;

	Drops.SetNum(Weather::DropCount);
	TArray<int32> Triangles;
	Vertices.Reset();
	Normals.Reset();
	for (int32 d = 0; d < Drops.Num(); ++d)
	{
		const int32 First = d * 4;
		for (int32 c = 0; c < 4; ++c)
		{
			Vertices.Add(FVector::ZeroVector);
			Normals.Add(FVector::UpVector);
		}
		// Both faces, so a streak shows from either side.
		Triangles.Append({ First, First + 1, First + 2, First, First + 2, First + 3,
			First, First + 2, First + 1, First, First + 3, First + 2 });
	}
	Mesh->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals,
		TArray<FVector2D>(), TArray<FLinearColor>(), TArray<FProcMeshTangent>(), false);
	if (SeeThroughMaterial)
	{
		UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(SeeThroughMaterial, this);
		if (WhiteTexture)
		{
			Material->SetTextureParameterValue(TEXT("SlateUI"), WhiteTexture);
		}
		Material->SetScalarParameterValue(TEXT("OpacityFromTexture"), 0.f);
		Material->SetVectorParameterValue(TEXT("TintColorAndOpacity"), Weather::DropTint);
		// Unlit, so it is darkened with the night instead of shining.
		if (ADayNight* Clock = ADayNight::Get(GetWorld()))
		{
			Clock->AddSeeThrough(Material, Weather::DropTint, false);
		}
		Mesh->SetMaterial(0, Material);
	}
	Mesh->SetMeshSectionVisible(0, false);
}

void AWeather::SetTestWeather(ETestWeather To)
{
	Forced = To;
	switch (Forced)
	{
	case ETestWeather::Clear:
		Aim(ESpell::Clear);
		break;
	case ETestWeather::Rain:
		Aim(ESpell::Rain);
		break;
	case ETestWeather::Fog:
		Aim(ESpell::Fog);
		break;
	default:
		NextSpell();
		break;
	}
}

void AWeather::Aim(ESpell Spell)
{
	switch (Spell)
	{
	case ESpell::Cloudy:
		AimOvercast = Random.FRandRange(0.6f, 0.85f);
		AimMist = Random.FRandRange(0.f, 0.15f);
		AimRain = 0.f;
		break;
	case ESpell::Rain:
		AimOvercast = 1.f;
		AimMist = Random.FRandRange(0.15f, 0.35f);
		AimRain = Forced == ETestWeather::Rain ? 1.f : Random.FRandRange(0.45f, 1.f);
		break;
	case ESpell::Fog:
		AimOvercast = Random.FRandRange(0.4f, 0.6f);
		AimMist = Forced == ETestWeather::Fog ? 1.f : Random.FRandRange(0.6f, 1.f);
		AimRain = 0.f;
		break;
	case ESpell::Clear:
	default:
		AimOvercast = Random.FRandRange(0.f, 0.2f);
		AimMist = 0.f;
		AimRain = 0.f;
		break;
	}
}

void AWeather::NextSpell()
{
	SpellLeft = Random.FRandRange(Weather::MinSpell, Weather::MaxSpell);
	// Mist mostly in the early morning and the evening.
	const ADayNight* Clock = ADayNight::Get(GetWorld());
	const float Hour = Clock ? Clock->GetHour() : 12.f;
	const bool bMisty = Hour < 9.f || Hour > 19.f;
	const float FogShare = bMisty ? 0.3f : 0.07f;
	const float Pick = Random.FRand();
	Aim(Pick < FogShare ? ESpell::Fog : Pick < FogShare + 0.25f ? ESpell::Rain : Pick < FogShare + 0.45f ? ESpell::Cloudy : ESpell::Clear);
}

void AWeather::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (DeltaSeconds <= 0.f)
	{
		return;
	}

	if (Forced == ETestWeather::Natural)
	{
		SpellLeft -= DeltaSeconds;
		if (SpellLeft <= 0.f)
		{
			NextSpell();
		}
	}

	// The sky turns slowly towards the spell; the rain waits for the cloud, and stops first.
	const float Rate = Forced == ETestWeather::Natural ? Weather::Turning : Weather::TurningForced;
	Overcast = FMath::FInterpConstantTo(Overcast, AimOvercast, DeltaSeconds, Rate);
	Mist = FMath::FInterpConstantTo(Mist, AimMist, DeltaSeconds, Rate);
	const float RainAim = Overcast >= Weather::RainNeedsCloud ? AimRain : 0.f;
	Rain = FMath::FInterpConstantTo(Rain, RainAim, DeltaSeconds, 2.f * Rate);

	if (ADayNight* Clock = ADayNight::Get(GetWorld()))
	{
		Clock->SetWeather(Overcast, Mist);
	}

	// Wet streets: soaking in the rain, drying slowly after.
	const bool bForced = Forced != ETestWeather::Natural;
	const float WetAim = Rain > 0.05f ? FMath::Min(1.f, Weather::WetFloor + (1.f - Weather::WetFloor) * Rain) : 0.f;
	const float WetSeconds = WetAim > Wet
		? (bForced ? Weather::ForcedSoakSeconds : Weather::SoakSeconds)
		: (bForced ? Weather::ForcedDrySeconds : Weather::DrySeconds);
	Wet = FMath::FInterpConstantTo(Wet, WetAim, DeltaSeconds, 1.f / WetSeconds);
	if (WetShown < 0.f || FMath::Abs(Wet - WetShown) >= Weather::WetStep || (Wet == 0.f && WetShown > 0.f))
	{
		WetShown = Wet;
		GroundSurfaces::SetWet(Wet);
	}

	if (Rain > 0.005f && !RainNoise)
	{
		RainNoise = NewObject<URainSound>(this);
		RainNoise->SetupAttachment(RootComponent);
		RainNoise->bAllowSpatialization = false;
		RainNoise->RegisterComponent();
		AddInstanceComponent(RainNoise);
		RainNoise->SetVolumeMultiplier(Weather::RainVolume);
		RainNoise->Start();
	}
	if (RainNoise)
	{
		RainNoise->SetRain(Rain);
	}

	const APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (PC && PC->PlayerCameraManager)
	{
		UpdateDrops(DeltaSeconds, PC->PlayerCameraManager->GetCameraLocation());
	}
}

void AWeather::UpdateDrops(float DeltaSeconds, const FVector& Eye)
{
	using namespace Weather;
	const int32 Showing = FMath::RoundToInt(Rain * Drops.Num());
	if (Showing <= 0)
	{
		if (bDropsShown)
		{
			Mesh->SetMeshSectionVisible(0, false);
			bDropsShown = false;
		}
		return;
	}
	if (!bDropsShown)
	{
		// Scatter them through the box afresh.
		for (FVector& Drop : Drops)
		{
			Drop = Eye + FVector(Random.FRandRange(-BoxHalf, BoxHalf), Random.FRandRange(-BoxHalf, BoxHalf), Random.FRandRange(-BoxBelow, BoxAbove));
		}
		Mesh->SetMeshSectionVisible(0, true);
		bDropsShown = true;
	}

	const FVector Fall = Wind + FVector(0.0, 0.0, -FallSpeed);
	const FVector Along = Fall.GetSafeNormal();
	Vertices.Reset();
	Normals.Reset();
	for (int32 d = 0; d < Drops.Num(); ++d)
	{
		FVector& Drop = Drops[d];
		Drop += Fall * DeltaSeconds;
		// Keep each drop in the box round the camera: one that falls out the bottom comes in again at the top.
		FVector Rel = Drop - Eye;
		if (Rel.Z < -BoxBelow)
		{
			Rel = FVector(Random.FRandRange(-BoxHalf, BoxHalf), Random.FRandRange(-BoxHalf, BoxHalf), BoxAbove - Random.FRandRange(0.0, 200.0));
		}
		else if (Rel.Z > BoxAbove)
		{
			Rel.Z -= BoxAbove + BoxBelow;
		}
		if (Rel.X > BoxHalf) { Rel.X -= 2.0 * BoxHalf; }
		if (Rel.X < -BoxHalf) { Rel.X += 2.0 * BoxHalf; }
		if (Rel.Y > BoxHalf) { Rel.Y -= 2.0 * BoxHalf; }
		if (Rel.Y < -BoxHalf) { Rel.Y += 2.0 * BoxHalf; }
		Drop = Eye + Rel;

		if (d >= Showing)
		{
			for (int32 c = 0; c < 4; ++c)
			{
				Vertices.Add(Drop);
				Normals.Add(FVector::UpVector);
			}
			continue;
		}
		// A thin streak along the fall, turned to face the camera.
		const FVector Side = FVector::CrossProduct(Along, Rel).GetSafeNormal() * (0.5 * StreakWidth * (1.0 + Rel.Size() / 1500.0));
		const FVector Tail = Drop - Along * StreakLength;
		Vertices.Add(Drop - Side);
		Vertices.Add(Drop + Side);
		Vertices.Add(Tail + Side);
		Vertices.Add(Tail - Side);
		for (int32 c = 0; c < 4; ++c)
		{
			Normals.Add(FVector::UpVector);
		}
	}
	Mesh->UpdateMeshSection_LinearColor(0, Vertices, Normals,
		TArray<FVector2D>(), TArray<FLinearColor>(), TArray<FProcMeshTangent>(), false);
}
