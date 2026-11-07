// Fill out your copyright notice in the Description page of Project Settings.

#include "CrashBits.h"

#include "Camera/PlayerCameraManager.h"
#include "DayNight.h"
#include "Engine/Texture.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "ExhaustSmoke.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

namespace CrashBits
{
	enum ESection : int32 { Sparks, Glass, Grit, SectionCount };

	// How many of each can be about at once.
	constexpr int32 PoolSizes[SectionCount] = { 260, 140, 90 };
	// Falling (cm/s each second).
	constexpr float Gravity = 980.f;
	// Sparks: how many a full-strength crash throws, how fast (cm/s), how long they glow (s).
	constexpr int32 MostSparks = 70;
	constexpr int32 FewestSparks = 12;
	constexpr float SparkSpeed = 650.f;
	constexpr float SparkMinLife = 0.25f;
	constexpr float SparkMaxLife = 0.65f;
	// A spark is drawn as a streak this many seconds of its flight long, and this wide (cm).
	constexpr float StreakTime = 0.03f;
	constexpr float StreakMin = 3.f;
	constexpr float StreakMax = 45.f;
	constexpr float SparkWidth = 1.3f;
	// How bright the sparks glow, as many times a lit grey wall.
	constexpr float SparkBright = 7.f;
	const FLinearColor SparkColor(1.f, 0.55f, 0.16f);
	// Glass and grit: how many a full-strength crash throws, how big they are (cm), how long they lie (s).
	constexpr int32 MostGlass = 34;
	constexpr int32 MostGrit = 16;
	constexpr float GlassSize = 5.f;
	constexpr float GritSize = 4.f;
	constexpr float PieceLife = 9.f;
	constexpr float PieceShrink = 1.2f;
	// Pieces fly off this fast (cm/s), tumbling this fast (degrees a second).
	constexpr float PieceSpeed = 380.f;
	constexpr float PieceSpin = 720.f;
	// Dust puffs: how many, and how big when they start (cm).
	constexpr int32 MostDust = 7;
	constexpr float DustSize = 34.f;
	// A scrape throws this many sparks each time it is called.
	constexpr int32 ScrapeSparks = 3;
	// Glass is pale and see-through, grit dark.
	const FLinearColor GlassColor(0.78f, 0.88f, 0.92f, 0.55f);
	const FLinearColor GritColor(0.07f, 0.065f, 0.06f);
}

ACrashBits::ACrashBits()
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
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicShapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	BaseMaterial = BasicShapeMaterial.Object;
	static ConstructorHelpers::FObjectFinder<UTexture> White(TEXT("/Engine/EngineResources/WhiteSquareTexture.WhiteSquareTexture"));
	WhiteTexture = White.Object;
}

ACrashBits* ACrashBits::Get(UWorld* World)
{
	static TWeakObjectPtr<ACrashBits> Found;
	if (!World)
	{
		return nullptr;
	}
	if (Found.IsValid() && Found->GetWorld() == World)
	{
		return Found.Get();
	}
	for (TActorIterator<ACrashBits> It(World); It; ++It)
	{
		Found = *It;
		return *It;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ACrashBits* Made = World->SpawnActor<ACrashBits>(ACrashBits::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Params);
	Found = Made;
	return Made;
}

void ACrashBits::BeginPlay()
{
	Super::BeginPlay();

	// The mesh is in world space; the actor stays at the origin.
	SetActorLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);
	for (int32 Section = 0; Section < CrashBits::SectionCount; ++Section)
	{
		FPool& Pool = Pools[Section];
		Pool.Bits.SetNum(CrashBits::PoolSizes[Section]);
		// Each bit is a little square, both faces: 8 corners, 4 triangles.
		TArray<int32> Triangles;
		Vertices.Reset();
		Normals.Reset();
		for (int32 b = 0; b < Pool.Bits.Num(); ++b)
		{
			const int32 First = b * 8;
			for (int32 c = 0; c < 8; ++c)
			{
				Vertices.Add(FVector::ZeroVector);
				Normals.Add(FVector::UpVector);
			}
			Triangles.Append({ First, First + 1, First + 2, First, First + 2, First + 3 });
			Triangles.Append({ First + 4, First + 6, First + 5, First + 4, First + 7, First + 6 });
		}
		Mesh->CreateMeshSection_LinearColor(Section, Vertices, Triangles, Normals,
			TArray<FVector2D>(), TArray<FLinearColor>(), TArray<FProcMeshTangent>(), false);
	}

	ADayNight* Clock = ADayNight::Get(GetWorld());
	// The sparks glow (unlit, brighter than white); without that material, plain orange.
	SparkGlow = Clock ? Clock->MakeGlowMaterial(this) : nullptr;
	if (SparkGlow)
	{
		Clock->SetGlow(SparkGlow, CrashBits::SparkColor, CrashBits::SparkBright);
		Mesh->SetMaterial(CrashBits::Sparks, SparkGlow);
	}
	else if (BaseMaterial)
	{
		UMaterialInstanceDynamic* Plain = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		Plain->SetVectorParameterValue(TEXT("Color"), CrashBits::SparkColor);
		Mesh->SetMaterial(CrashBits::Sparks, Plain);
	}

	if (SeeThroughMaterial)
	{
		UMaterialInstanceDynamic* GlassPaint = UMaterialInstanceDynamic::Create(SeeThroughMaterial, this);
		if (WhiteTexture)
		{
			GlassPaint->SetTextureParameterValue(TEXT("SlateUI"), WhiteTexture);
		}
		GlassPaint->SetScalarParameterValue(TEXT("OpacityFromTexture"), 0.f);
		GlassPaint->SetVectorParameterValue(TEXT("TintColorAndOpacity"), CrashBits::GlassColor);
		// Unlit, so it is darkened with the night instead of shining.
		if (Clock)
		{
			Clock->AddSeeThrough(GlassPaint, CrashBits::GlassColor, false);
		}
		Mesh->SetMaterial(CrashBits::Glass, GlassPaint);
	}
	if (BaseMaterial)
	{
		UMaterialInstanceDynamic* GritPaint = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		GritPaint->SetVectorParameterValue(TEXT("Color"), CrashBits::GritColor);
		Mesh->SetMaterial(CrashBits::Grit, GritPaint);
	}
}

ACrashBits::FBit& ACrashBits::Take(FPool& Pool)
{
	// The oldest bit makes way for the new one.
	FBit& Fresh = Pool.Bits[Pool.Next];
	Pool.Next = (Pool.Next + 1) % Pool.Bits.Num();
	Fresh = FBit();
	Pool.bAny = true;
	return Fresh;
}

void ACrashBits::AddSparks(const FVector& At, const FVector& Away, const FVector& Along, int32 Count, float Speed, double Ground)
{
	FPool& Pool = Pools[CrashBits::Sparks];
	if (Pool.Bits.Num() == 0)
	{
		return;
	}
	for (int32 i = 0; i < Count; ++i)
	{
		FBit& Spark = Take(Pool);
		// Mostly along the wall and up, a little out from it.
		const float Spread = FMath::FRandRange(-1.f, 1.f);
		const FVector Way = (Along * Spread + Away * FMath::FRandRange(0.15f, 0.7f) + FVector::UpVector * FMath::FRandRange(0.1f, 0.9f)).GetSafeNormal();
		Spark.At = At + FVector(FMath::FRandRange(-6.f, 6.f), FMath::FRandRange(-6.f, 6.f), FMath::FRandRange(-6.f, 6.f));
		Spark.Velocity = Way * Speed * FMath::FRandRange(0.4f, 1.2f);
		Spark.Life = FMath::FRandRange(CrashBits::SparkMinLife, CrashBits::SparkMaxLife);
		Spark.Ground = Ground;
	}
}

void ACrashBits::Burst(const FVector& At, const FVector& Away, float Strength, bool bGlass, double Ground)
{
	const float S = FMath::Clamp(Strength, 0.f, 1.f);
	const FVector Out = FVector(Away.X, Away.Y, 0.0).GetSafeNormal();
	const FVector Along = FVector::CrossProduct(Out, FVector::UpVector);

	AddSparks(At, Out, Along, FMath::RoundToInt(FMath::Lerp(static_cast<float>(CrashBits::FewestSparks), static_cast<float>(CrashBits::MostSparks), S)),
		CrashBits::SparkSpeed * (0.6f + 0.6f * S), Ground);

	// Glass (when a pane or lamp broke) and grit fly out, tumbling.
	const auto Throw = [&](int32 Section, int32 Count, float Size)
	{
		FPool& Pool = Pools[Section];
		if (Pool.Bits.Num() == 0)
		{
			return;
		}
		for (int32 i = 0; i < Count; ++i)
		{
			FBit& Piece = Take(Pool);
			const FVector Way = (Out * FMath::FRandRange(0.3f, 1.f) + Along * FMath::FRandRange(-1.f, 1.f) + FVector::UpVector * FMath::FRandRange(0.2f, 1.f)).GetSafeNormal();
			Piece.At = At + FVector(FMath::FRandRange(-15.f, 15.f), FMath::FRandRange(-15.f, 15.f), FMath::FRandRange(-5.f, 15.f));
			Piece.Velocity = Way * CrashBits::PieceSpeed * FMath::FRandRange(0.3f, 1.1f) * (0.6f + 0.6f * S);
			Piece.Axis = FVector(FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-1.f, 1.f)).GetSafeNormal();
			if (Piece.Axis.IsNearlyZero())
			{
				Piece.Axis = FVector::ForwardVector;
			}
			Piece.Angle = FMath::FRandRange(0.f, 360.f);
			Piece.Spin = FMath::FRandRange(-1.f, 1.f) * CrashBits::PieceSpin;
			Piece.Size = Size * FMath::FRandRange(0.5f, 1.4f);
			Piece.Life = CrashBits::PieceLife * FMath::FRandRange(0.8f, 1.2f);
			Piece.Ground = Ground + FMath::FRandRange(0.4f, 0.9f);
		}
	};
	if (bGlass)
	{
		Throw(CrashBits::Glass, FMath::RoundToInt(CrashBits::MostGlass * (0.4f + 0.6f * S)), CrashBits::GlassSize);
	}
	Throw(CrashBits::Grit, FMath::RoundToInt(CrashBits::MostGrit * S), CrashBits::GritSize);

	// A puff of road dust and flaked paint.
	if (AExhaustSmoke* Smoke = AExhaustSmoke::Get(GetWorld()))
	{
		const int32 Puffs = 2 + FMath::RoundToInt((CrashBits::MostDust - 2) * S);
		for (int32 i = 0; i < Puffs; ++i)
		{
			const FVector From = At + Out * FMath::FRandRange(10.f, 60.f) + Along * FMath::FRandRange(-90.f, 90.f)
				+ FVector(0.0, 0.0, FMath::FRandRange(-30.f, 20.f));
			const FVector Drift = Out * FMath::FRandRange(40.f, 140.f) + Along * FMath::FRandRange(-60.f, 60.f) + FVector(0.0, 0.0, FMath::FRandRange(10.f, 60.f));
			Smoke->Puff(From, Drift, CrashBits::DustSize * FMath::FRandRange(0.7f, 1.3f), false);
		}
	}
}

void ACrashBits::Scrape(const FVector& At, const FVector& Away, const FVector& Along, double Ground)
{
	const FVector Out = FVector(Away.X, Away.Y, 0.0).GetSafeNormal();
	// Thrown back the way the car came from, along the wall.
	AddSparks(At, Out * 0.3, -Along.GetSafeNormal(), CrashBits::ScrapeSparks, CrashBits::SparkSpeed * 0.7f, Ground);
}

void ACrashBits::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	FVector Eye = GetActorLocation();
	if (const APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		if (PC->PlayerCameraManager)
		{
			Eye = PC->PlayerCameraManager->GetCameraLocation();
		}
	}
	UpdateSparks(DeltaSeconds, Eye);
	UpdatePieces(CrashBits::Glass, DeltaSeconds);
	UpdatePieces(CrashBits::Grit, DeltaSeconds);
}

void ACrashBits::UpdateSparks(float DeltaSeconds, const FVector& Eye)
{
	FPool& Pool = Pools[CrashBits::Sparks];
	if (!Pool.bAny)
	{
		return;
	}
	// Keep the glow as bright as the daylight needs.
	if (SparkGlow)
	{
		if (const ADayNight* Clock = ADayNight::Get(GetWorld()))
		{
			Clock->SetGlow(SparkGlow, CrashBits::SparkColor, CrashBits::SparkBright);
		}
	}
	Vertices.Reset();
	Normals.Reset();
	bool bAny = false;
	for (FBit& Spark : Pool.Bits)
	{
		FVector Head = Spark.At;
		FVector Side = FVector::ZeroVector;
		FVector Tail = Spark.At;
		if (Spark.Age < Spark.Life)
		{
			Spark.Age += DeltaSeconds;
			Spark.Velocity.Z -= CrashBits::Gravity * DeltaSeconds;
			Spark.At += Spark.Velocity * DeltaSeconds;
			// A spark that reaches the road skips along it, slower.
			if (Spark.At.Z < Spark.Ground)
			{
				Spark.At.Z = Spark.Ground;
				Spark.Velocity.Z = FMath::Abs(Spark.Velocity.Z) * 0.3f;
				Spark.Velocity.X *= 0.6f;
				Spark.Velocity.Y *= 0.6f;
			}
			// A streak behind it as long as its last moment of flight, thinning as it dies.
			const float Speed = static_cast<float>(Spark.Velocity.Size());
			const float Long = FMath::Clamp(Speed * CrashBits::StreakTime, CrashBits::StreakMin, CrashBits::StreakMax);
			const FVector Dir = Speed > KINDA_SMALL_NUMBER ? Spark.Velocity / Speed : FVector::UpVector;
			Head = Spark.At;
			Tail = Spark.At - Dir * Long;
			const float T = FMath::Clamp(Spark.Age / Spark.Life, 0.f, 1.f);
			const FVector Facing = FVector::CrossProduct(Dir, Eye - Spark.At).GetSafeNormal();
			Side = Facing * (CrashBits::SparkWidth * 0.5f * (1.f - 0.7f * T));
			bAny = true;
		}
		const FVector Corners[4] = { Tail - Side, Head - Side, Head + Side, Tail + Side };
		for (int32 Face = 0; Face < 2; ++Face)
		{
			for (const FVector& Corner : Corners)
			{
				Vertices.Add(Corner);
				Normals.Add(FVector::UpVector);
			}
		}
	}
	Mesh->UpdateMeshSection_LinearColor(CrashBits::Sparks, Vertices, Normals,
		TArray<FVector2D>(), TArray<FLinearColor>(), TArray<FProcMeshTangent>(), false);
	// One more update after the last spark dies, to clear it away.
	Pool.bAny = bAny;
}

void ACrashBits::UpdatePieces(int32 Section, float DeltaSeconds)
{
	FPool& Pool = Pools[Section];
	if (!Pool.bAny)
	{
		return;
	}
	Vertices.Reset();
	Normals.Reset();
	bool bAny = false;
	for (FBit& Piece : Pool.Bits)
	{
		float Half = 0.f;
		FQuat Turn = FQuat::Identity;
		if (Piece.Age < Piece.Life)
		{
			Piece.Age += DeltaSeconds;
			const bool bLying = Piece.Velocity.IsNearlyZero() && Piece.At.Z <= Piece.Ground + 0.01;
			if (!bLying)
			{
				Piece.Velocity.Z -= CrashBits::Gravity * DeltaSeconds;
				Piece.At += Piece.Velocity * DeltaSeconds;
				Piece.Angle += Piece.Spin * DeltaSeconds;
				// On the road it lies flat and still.
				if (Piece.At.Z <= Piece.Ground)
				{
					Piece.At.Z = Piece.Ground;
					Piece.Velocity = FVector::ZeroVector;
					Piece.Spin = 0.f;
					Piece.Angle = 0.f;
				}
			}
			// Shrinks away at the end, when nobody is looking closely any more.
			const float Left = Piece.Life - Piece.Age;
			Half = Piece.Size * 0.5f * FMath::Clamp(Left / CrashBits::PieceShrink, 0.f, 1.f);
			Turn = FQuat(Piece.Axis, FMath::DegreesToRadians(Piece.Angle));
			bAny = true;
		}
		const FVector X = Turn.RotateVector(FVector(Half, 0.0, 0.0));
		const FVector Y = Turn.RotateVector(FVector(0.0, Half * 0.7, 0.0));
		const FVector Up = Turn.RotateVector(FVector::UpVector);
		const FVector Corners[4] = { Piece.At - X - Y, Piece.At + X - Y, Piece.At + X + Y, Piece.At - X + Y };
		for (int32 Face = 0; Face < 2; ++Face)
		{
			for (const FVector& Corner : Corners)
			{
				Vertices.Add(Corner);
				Normals.Add(Face == 0 ? Up : -Up);
			}
		}
	}
	Mesh->UpdateMeshSection_LinearColor(Section, Vertices, Normals,
		TArray<FVector2D>(), TArray<FLinearColor>(), TArray<FProcMeshTangent>(), false);
	Pool.bAny = bAny;
}
