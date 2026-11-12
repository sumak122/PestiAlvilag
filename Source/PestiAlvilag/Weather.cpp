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

	// No rain under roofs: the map round the camera in squares this wide (cm),
	// out to this many squares each way; a few of them each frame are looked
	// down on from this far above the drops' box, and what is found is
	// forgotten once this many squares are remembered. Roofs without
	// collision (AddShelter) are drawn onto squares this wide instead.
	constexpr double CoverSquare = 250.0;
	constexpr int32 CoverReach = 9;
	constexpr int32 CoverPerFrame = 24;
	constexpr double CoverFromAbove = 4000.0;
	constexpr int32 CoverForget = 6000;
	constexpr double ShelterSquare = 200.0;
	constexpr float NoCover = -1.0e9f;

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

	LookForCover(Eye);

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

		// Not shown: more drops than the rain needs, or under a roof.
		if (d >= Showing || Drop.Z < CoverOver(Drop))
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

void AWeather::AddShelter(UProceduralMeshComponent* Roof)
{
	if (Roof)
	{
		FShelter& Shelter = Shelters.AddDefaulted_GetRef();
		Shelter.Roof = Roof;
	}
}

void AWeather::DrawShelter(FShelter& Shelter) const
{
	using namespace Weather;
	UProceduralMeshComponent* Roof = Shelter.Roof.Get();
	if (!Roof || Roof->GetNumSections() == 0)
	{
		return;
	}
	Shelter.bDrawn = true;

	const FBox Bounds = Roof->Bounds.GetBox();
	Shelter.Corner = FVector2D(Bounds.Min.X, Bounds.Min.Y);
	Shelter.Across = FMath::Clamp(FMath::CeilToInt32((Bounds.Max.X - Bounds.Min.X) / ShelterSquare) + 1, 1, 1024);
	Shelter.Down = FMath::Clamp(FMath::CeilToInt32((Bounds.Max.Y - Bounds.Min.Y) / ShelterSquare) + 1, 1, 1024);
	Shelter.Tops.Init(NoCover, Shelter.Across * Shelter.Down);

	// Every triangle of the roof, sampled finely enough to touch each square under it.
	const FTransform& ToWorld = Roof->GetComponentTransform();
	for (int32 s = 0; s < Roof->GetNumSections(); ++s)
	{
		const FProcMeshSection* Section = Roof->GetProcMeshSection(s);
		if (!Section)
		{
			continue;
		}
		const TArray<FProcMeshVertex>& Points = Section->ProcVertexBuffer;
		const TArray<uint32>& Corners = Section->ProcIndexBuffer;
		for (int32 t = 0; t + 2 < Corners.Num(); t += 3)
		{
			const int32 First = static_cast<int32>(Corners[t]);
			const int32 Second = static_cast<int32>(Corners[t + 1]);
			const int32 Third = static_cast<int32>(Corners[t + 2]);
			if (!Points.IsValidIndex(First) || !Points.IsValidIndex(Second) || !Points.IsValidIndex(Third))
			{
				continue;
			}
			const FVector A = ToWorld.TransformPosition(Points[First].Position);
			const FVector B = ToWorld.TransformPosition(Points[Second].Position);
			const FVector C = ToWorld.TransformPosition(Points[Third].Position);
			const double Longest = FMath::Max3(FVector::Dist2D(A, B), FVector::Dist2D(B, C), FVector::Dist2D(C, A));
			const int32 Steps = FMath::Clamp(FMath::CeilToInt32(Longest / (ShelterSquare * 0.5)), 1, 64);
			for (int32 i = 0; i <= Steps; ++i)
			{
				for (int32 j = 0; i + j <= Steps; ++j)
				{
					const double U = static_cast<double>(i) / Steps;
					const double V = static_cast<double>(j) / Steps;
					const FVector On = A + (B - A) * U + (C - A) * V;
					const int32 Column = FMath::FloorToInt32((On.X - Shelter.Corner.X) / ShelterSquare);
					const int32 Row = FMath::FloorToInt32((On.Y - Shelter.Corner.Y) / ShelterSquare);
					if (Column >= 0 && Column < Shelter.Across && Row >= 0 && Row < Shelter.Down)
					{
						float& Top = Shelter.Tops[Row * Shelter.Across + Column];
						Top = FMath::Max(Top, static_cast<float>(On.Z));
					}
				}
			}
		}
	}
}

void AWeather::LookForCover(const FVector& Eye)
{
	using namespace Weather;
	for (FShelter& Shelter : Shelters)
	{
		if (!Shelter.bDrawn)
		{
			DrawShelter(Shelter);
		}
	}

	if (CoverSquares.Num() > CoverForget)
	{
		CoverSquares.Reset();
	}
	const int32 Side = 2 * CoverReach + 1;
	const int32 EyeColumn = FMath::FloorToInt32(Eye.X / CoverSquare);
	const int32 EyeRow = FMath::FloorToInt32(Eye.Y / CoverSquare);
	const double From = Eye.Z + BoxAbove + CoverFromAbove;
	const double To = Eye.Z - BoxBelow - 100.0;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(RainCover), false);
	for (int32 n = 0; n < CoverPerFrame; ++n)
	{
		CoverTurn = (CoverTurn + 1) % (Side * Side);
		const FIntPoint Square(EyeColumn - CoverReach + CoverTurn % Side, EyeRow - CoverReach + CoverTurn / Side);
		const double X = (Square.X + 0.5) * CoverSquare;
		const double Y = (Square.Y + 0.5) * CoverSquare;
		FHitResult Hit;
		const bool bFound = GetWorld()->LineTraceSingleByChannel(Hit, FVector(X, Y, From), FVector(X, Y, To), ECC_Visibility, Params);
		CoverSquares.Add(Square, bFound ? static_cast<float>(Hit.ImpactPoint.Z) : NoCover);
	}
}

float AWeather::CoverOver(const FVector& At) const
{
	using namespace Weather;
	float Highest = NoCover;
	if (const float* Found = CoverSquares.Find(FIntPoint(FMath::FloorToInt32(At.X / CoverSquare), FMath::FloorToInt32(At.Y / CoverSquare))))
	{
		Highest = *Found;
	}
	for (const FShelter& Shelter : Shelters)
	{
		if (!Shelter.bDrawn)
		{
			continue;
		}
		const int32 Column = FMath::FloorToInt32((At.X - Shelter.Corner.X) / ShelterSquare);
		const int32 Row = FMath::FloorToInt32((At.Y - Shelter.Corner.Y) / ShelterSquare);
		if (Column >= 0 && Column < Shelter.Across && Row >= 0 && Row < Shelter.Down)
		{
			Highest = FMath::Max(Highest, Shelter.Tops[Row * Shelter.Across + Column]);
		}
	}
	return Highest;
}
