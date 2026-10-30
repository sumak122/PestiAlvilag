// Fill out your copyright notice in the Description page of Project Settings.

#include "ExhaustSmoke.h"

#include "DayNight.h"
#include "Engine/Texture.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

namespace ExhaustSmoke
{
	// How many puffs can be in the air at once, light and dark.
	constexpr int32 PoolSizes[2] = { 220, 30 };
	// Seconds a puff lasts.
	constexpr float MinLife = 0.9f;
	constexpr float MaxLife = 1.6f;
	// Puffs swell to this many times their size before they fade.
	constexpr float Swell = 3.f;
	// Rising (cm/s each second) and slowing in the air (share lost each second).
	constexpr float Lift = 40.f;
	constexpr float Drag = 1.8f;
	// See-through grey: colour and how much it hides (alpha).
	const FLinearColor Colors[2] = {
		FLinearColor(0.62f, 0.62f, 0.60f, 0.12f),
		FLinearColor(0.10f, 0.10f, 0.10f, 0.28f) };

	/** A rough ball of 12 corners, 1 across. */
	const TArray<FVector>& Ball()
	{
		static const TArray<FVector> Corners = []()
		{
			const double T = (1.0 + FMath::Sqrt(5.0)) * 0.5;
			TArray<FVector> C = {
				FVector(-1.0, T, 0.0), FVector(1.0, T, 0.0), FVector(-1.0, -T, 0.0), FVector(1.0, -T, 0.0),
				FVector(0.0, -1.0, T), FVector(0.0, 1.0, T), FVector(0.0, -1.0, -T), FVector(0.0, 1.0, -T),
				FVector(T, 0.0, -1.0), FVector(T, 0.0, 1.0), FVector(-T, 0.0, -1.0), FVector(-T, 0.0, 1.0) };
			for (FVector& V : C)
			{
				V = V.GetSafeNormal() * 0.5;
			}
			return C;
		}();
		return Corners;
	}

	const int32 BallTriangles[] = {
		0, 11, 5, 0, 5, 1, 0, 1, 7, 0, 7, 10, 0, 10, 11,
		1, 5, 9, 5, 11, 4, 11, 10, 2, 10, 7, 6, 7, 1, 8,
		3, 9, 4, 3, 4, 2, 3, 2, 6, 3, 6, 8, 3, 8, 9,
		4, 9, 5, 2, 4, 11, 6, 2, 10, 8, 6, 7, 9, 8, 1 };
}

AExhaustSmoke::AExhaustSmoke()
{
	PrimaryActorTick.bCanEverTick = true;

	Mesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCanEverAffectNavigation(false);
	Mesh->SetCastShadow(false);

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Translucent(TEXT("/Engine/EngineMaterials/Widget3DPassThrough_Translucent.Widget3DPassThrough_Translucent"));
	SeeThroughMaterial = Translucent.Object;
	static ConstructorHelpers::FObjectFinder<UTexture> White(TEXT("/Engine/EngineResources/WhiteSquareTexture.WhiteSquareTexture"));
	WhiteTexture = White.Object;
}

AExhaustSmoke* AExhaustSmoke::Get(UWorld* World)
{
	static TWeakObjectPtr<AExhaustSmoke> Found;
	if (!World)
	{
		return nullptr;
	}
	if (Found.IsValid() && Found->GetWorld() == World)
	{
		return Found.Get();
	}
	for (TActorIterator<AExhaustSmoke> It(World); It; ++It)
	{
		Found = *It;
		return *It;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AExhaustSmoke* Smoke = World->SpawnActor<AExhaustSmoke>(AExhaustSmoke::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Params);
	Found = Smoke;
	return Smoke;
}

void AExhaustSmoke::BeginPlay()
{
	Super::BeginPlay();

	// The mesh is in world space; the actor stays at the origin.
	SetActorLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);
	const TArray<FVector>& Ball = ExhaustSmoke::Ball();
	for (int32 Section = 0; Section < 2; ++Section)
	{
		FPool& Pool = Pools[Section];
		Pool.Puffs.SetNum(ExhaustSmoke::PoolSizes[Section]);
		TArray<int32> Triangles;
		Vertices.Reset();
		Normals.Reset();
		for (int32 p = 0; p < Pool.Puffs.Num(); ++p)
		{
			const int32 First = p * Ball.Num();
			for (const FVector& Corner : Ball)
			{
				Vertices.Add(FVector::ZeroVector);
				Normals.Add(Corner.GetSafeNormal());
			}
			for (const int32 Index : ExhaustSmoke::BallTriangles)
			{
				Triangles.Add(First + Index);
			}
		}
		Mesh->CreateMeshSection_LinearColor(Section, Vertices, Triangles, Normals,
			TArray<FVector2D>(), TArray<FLinearColor>(), TArray<FProcMeshTangent>(), false);
		if (SeeThroughMaterial)
		{
			UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(SeeThroughMaterial, this);
			if (WhiteTexture)
			{
				Material->SetTextureParameterValue(TEXT("SlateUI"), WhiteTexture);
			}
			Material->SetScalarParameterValue(TEXT("OpacityFromTexture"), 0.f);
			Material->SetVectorParameterValue(TEXT("TintColorAndOpacity"), ExhaustSmoke::Colors[Section]);
			// Unlit, so it is darkened with the night instead of shining.
			if (ADayNight* Clock = ADayNight::Get(GetWorld()))
			{
				Clock->AddSeeThrough(Material, ExhaustSmoke::Colors[Section]);
			}
			Mesh->SetMaterial(Section, Material);
		}
		else
		{
			Mesh->SetMeshSectionVisible(Section, false);
		}
	}
}

void AExhaustSmoke::Puff(const FVector& At, const FVector& Velocity, float Size, bool bDark)
{
	FPool& Pool = Pools[bDark ? 1 : 0];
	if (Pool.Puffs.Num() == 0)
	{
		return;
	}
	// The oldest puff makes way for the new one.
	FSmokePuff& New = Pool.Puffs[Pool.Next];
	Pool.Next = (Pool.Next + 1) % Pool.Puffs.Num();
	New.At = At;
	New.Velocity = Velocity;
	New.Size = Size;
	New.Age = 0.f;
	New.Life = FMath::FRandRange(ExhaustSmoke::MinLife, ExhaustSmoke::MaxLife) * (bDark ? 1.5f : 1.f);
	Pool.bAny = true;
}

void AExhaustSmoke::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	for (int32 Section = 0; Section < 2; ++Section)
	{
		UpdatePool(Section, Pools[Section], DeltaSeconds);
	}
}

void AExhaustSmoke::UpdatePool(int32 Section, FPool& Pool, float DeltaSeconds)
{
	// Nothing in the air, and nothing was last frame: the mesh is already empty.
	if (!Pool.bAny)
	{
		return;
	}
	const TArray<FVector>& Ball = ExhaustSmoke::Ball();
	Vertices.Reset();
	Normals.Reset();
	bool bAny = false;
	const float Slowing = FMath::Exp(-ExhaustSmoke::Drag * DeltaSeconds);
	for (FSmokePuff& Each : Pool.Puffs)
	{
		float Scale = 0.f;
		if (Each.Age < Each.Life)
		{
			Each.Age += DeltaSeconds;
			Each.Velocity *= Slowing;
			Each.Velocity.Z += ExhaustSmoke::Lift * DeltaSeconds;
			Each.At += Each.Velocity * DeltaSeconds;
			// Swell as it spreads, then shrink away to nothing at the end.
			const float T = FMath::Clamp(Each.Age / Each.Life, 0.f, 1.f);
			const float Fade = T > 0.65f ? (1.f - T) / 0.35f : 1.f;
			Scale = Each.Size * FMath::Lerp(1.f, ExhaustSmoke::Swell, FMath::Sqrt(T)) * Fade;
			bAny = true;
		}
		for (const FVector& Corner : Ball)
		{
			Vertices.Add(Each.At + Corner * Scale);
			Normals.Add(Corner.GetSafeNormal());
		}
	}
	Mesh->UpdateMeshSection_LinearColor(Section, Vertices, Normals,
		TArray<FVector2D>(), TArray<FLinearColor>(), TArray<FProcMeshTangent>(), false);
	// One more update after the last puff dies, to clear it away.
	Pool.bAny = bAny;
}
