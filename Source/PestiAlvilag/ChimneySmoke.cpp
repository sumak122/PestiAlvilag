// Fill out your copyright notice in the Description page of Project Settings.

#include "ChimneySmoke.h"

#include "Camera/PlayerCameraManager.h"
#include "CarBody.h"
#include "DayNight.h"
#include "Engine/Texture.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "KeletiStation.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "OsmGraybox.h"
#include "ProceduralMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

namespace ChimneySmoke
{
	// The roofs are looked over in squares this wide (cm), out to this far round the camera, a few squares at a time.
	constexpr double Cell = 2500.0;
	constexpr double ScanReach = 10000.0;
	constexpr int32 CellsPerScan = 2;
	constexpr float ScanEvery = 0.15f;
	constexpr int32 TriesPerCell = 3;
	// A roof is at least this high over the street (cm), flat, and flat this far round the stack too.
	constexpr double RoofAbove = 900.0;
	constexpr double RoofRoom = 120.0;
	// The stacks: brick, this big (cm), with a darker cap.
	const FVector StackSize(70.0, 45.0, 130.0);
	const FLinearColor Brick(0.30f, 0.15f, 0.10f);
	const FLinearColor Cap(0.12f, 0.11f, 0.10f);
	// How many smoke: this share, more in the morning and evening.
	constexpr float SmokingShare = 0.55f;
	// Smoke only drifts from stacks this near the camera (cm).
	constexpr double SmokeReach = 7000.0;

	// Puffs in the air at once: coal smoke, steam.
	constexpr int32 PoolSizes[2] = { 360, 160 };
	const FLinearColor Colors[2] = {
		FLinearColor(0.35f, 0.34f, 0.33f, 0.10f),
		FLinearColor(0.85f, 0.85f, 0.85f, 0.16f) };
	// The wind that carries it off (cm/s), the rise, and slowing in the air.
	const FVector Wind(55.0, 22.0, 0.0);
	constexpr float Lift = 25.f;
	constexpr float Drag = 0.6f;

	/** A rough ball of 12 corners, 1 across (as in AExhaustSmoke). */
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

	/** Adds a box from Low to High (world, square to the axes) to a mesh section's lists. */
	void AddBox(TArray<FVector>& V, TArray<int32>& T, TArray<FVector>& N, const FVector& Low, const FVector& High)
	{
		const FVector Corner[8] = {
			FVector(Low.X, Low.Y, Low.Z), FVector(High.X, Low.Y, Low.Z), FVector(High.X, High.Y, Low.Z), FVector(Low.X, High.Y, Low.Z),
			FVector(Low.X, Low.Y, High.Z), FVector(High.X, Low.Y, High.Z), FVector(High.X, High.Y, High.Z), FVector(Low.X, High.Y, High.Z) };
		// Each face: its four corners (counter-clockwise seen from outside) and its normal.
		const int32 Faces[6][4] = { { 4, 5, 6, 7 }, { 0, 3, 2, 1 }, { 0, 1, 5, 4 }, { 2, 3, 7, 6 }, { 1, 2, 6, 5 }, { 3, 0, 4, 7 } };
		const FVector Facing[6] = { FVector::UpVector, FVector::DownVector, FVector(0, -1, 0), FVector(0, 1, 0), FVector(1, 0, 0), FVector(-1, 0, 0) };
		for (int32 f = 0; f < 6; ++f)
		{
			const int32 First = V.Num();
			for (int32 c = 0; c < 4; ++c)
			{
				V.Add(Corner[Faces[f][c]]);
				N.Add(Facing[f]);
			}
			// Both windings, so the face shows whichever way round the renderer counts its front.
			T.Append({ First, First + 1, First + 2, First, First + 2, First + 3,
				First, First + 2, First + 1, First, First + 3, First + 2 });
		}
	}
}

AChimneySmoke::AChimneySmoke()
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

	Stacks = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Stacks"));
	Stacks->SetupAttachment(Mesh);
	Stacks->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Stacks->SetCanEverAffectNavigation(false);
	// Out of the ray tracing and the shadows too: it spans the whole city and
	// is rebuilt as stacks are added, which would redo the shadows everywhere.
	Stacks->SetCastShadow(false);
	Stacks->bVisibleInRayTracing = false;
	Stacks->bAffectDistanceFieldLighting = false;
	Stacks->bAffectDynamicIndirectLighting = false;

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Translucent(TEXT("/Engine/EngineMaterials/Widget3DPassThrough_Translucent.Widget3DPassThrough_Translucent"));
	SeeThroughMaterial = Translucent.Object;
	static ConstructorHelpers::FObjectFinder<UTexture> White(TEXT("/Engine/EngineResources/WhiteSquareTexture.WhiteSquareTexture"));
	WhiteTexture = White.Object;
	BaseMaterial = CarBody::LoadPaintMaterial();
}

UMaterialInterface* AChimneySmoke::SeeThrough(const FLinearColor& Tint)
{
	if (!SeeThroughMaterial)
	{
		return nullptr;
	}
	UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(SeeThroughMaterial, this);
	if (WhiteTexture)
	{
		Material->SetTextureParameterValue(TEXT("SlateUI"), WhiteTexture);
	}
	Material->SetScalarParameterValue(TEXT("OpacityFromTexture"), 0.f);
	Material->SetVectorParameterValue(TEXT("TintColorAndOpacity"), Tint);
	// Unlit, so it is darkened with the night instead of shining.
	if (ADayNight* Clock = ADayNight::Get(GetWorld()))
	{
		Clock->AddSeeThrough(Material, Tint, false);
	}
	return Material;
}

void AChimneySmoke::BeginPlay()
{
	Super::BeginPlay();

	// Everything is in world space; the actor stays at the origin.
	SetActorLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);
	Random.Initialize(1932);

	const TArray<FVector>& Corners = ChimneySmoke::Ball();
	for (int32 Section = 0; Section < 2; ++Section)
	{
		FPool& Pool = Pools[Section];
		Pool.Puffs.SetNum(ChimneySmoke::PoolSizes[Section]);
		TArray<int32> Triangles;
		Vertices.Reset();
		Normals.Reset();
		for (int32 p = 0; p < Pool.Puffs.Num(); ++p)
		{
			const int32 First = p * Corners.Num();
			for (const FVector& Corner : Corners)
			{
				Vertices.Add(FVector::ZeroVector);
				Normals.Add(Corner.GetSafeNormal());
			}
			for (const int32 Index : ChimneySmoke::BallTriangles)
			{
				Triangles.Add(First + Index);
			}
		}
		Mesh->CreateMeshSection_LinearColor(Section, Vertices, Triangles, Normals,
			TArray<FVector2D>(), TArray<FLinearColor>(), TArray<FProcMeshTangent>(), false);
		if (UMaterialInterface* Material = SeeThrough(ChimneySmoke::Colors[Section]))
		{
			Mesh->SetMaterial(Section, Material);
		}
		else
		{
			Mesh->SetMeshSectionVisible(Section, false);
		}
	}
}

bool AChimneySmoke::IsRoof(const FVector2D& At, double Street, double& OutZ) const
{
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ChimneyRoof), false, this);
	if (!GetWorld()->LineTraceSingleByChannel(Hit, FVector(At, Street + 30000.0), FVector(At, Street + ChimneySmoke::RoofAbove), ECC_Visibility, Params))
	{
		return false;
	}
	if (!Cast<AOsmGraybox>(Hit.GetActor()) || Hit.ImpactNormal.Z < 0.95)
	{
		return false;
	}
	OutZ = Hit.ImpactPoint.Z;
	return true;
}

void AChimneySmoke::Scan(const FVector& Eye, double Street)
{
	const int32 Reach = FMath::CeilToInt(ChimneySmoke::ScanReach / ChimneySmoke::Cell);
	const FIntPoint Here(FMath::FloorToInt(Eye.X / ChimneySmoke::Cell), FMath::FloorToInt(Eye.Y / ChimneySmoke::Cell));
	int32 Done = 0;
	// Nearest squares first: ring by ring out from the camera.
	for (int32 Ring = 0; Ring <= Reach && Done < ChimneySmoke::CellsPerScan; ++Ring)
	{
		for (int32 dx = -Ring; dx <= Ring && Done < ChimneySmoke::CellsPerScan; ++dx)
		{
			for (int32 dy = -Ring; dy <= Ring && Done < ChimneySmoke::CellsPerScan; ++dy)
			{
				if (FMath::Max(FMath::Abs(dx), FMath::Abs(dy)) != Ring)
				{
					continue;
				}
				const FIntPoint Square = Here + FIntPoint(dx, dy);
				if (Scanned.Contains(Square))
				{
					continue;
				}
				Scanned.Add(Square);
				++Done;
				for (int32 Try = 0; Try < ChimneySmoke::TriesPerCell; ++Try)
				{
					const FVector2D At((Square.X + Random.FRand()) * ChimneySmoke::Cell, (Square.Y + Random.FRand()) * ChimneySmoke::Cell);
					double Z = 0.0;
					if (!IsRoof(At, Street, Z))
					{
						continue;
					}
					// Flat all round, at the same height (not a ledge or the edge of a roof).
					bool bFlat = true;
					for (const FVector2D Offset : { FVector2D(1.0, 0.0), FVector2D(-1.0, 0.0), FVector2D(0.0, 1.0), FVector2D(0.0, -1.0) })
					{
						double Other = 0.0;
						bFlat = bFlat && IsRoof(At + Offset * ChimneySmoke::RoofRoom, Street, Other) && FMath::Abs(Other - Z) < 5.0;
					}
					if (!bFlat)
					{
						continue;
					}
					FStack Stack;
					Stack.Top = FVector(At, Z + ChimneySmoke::StackSize.Z);
					Stack.Rate = Random.FRand() < ChimneySmoke::SmokingShare ? Random.FRandRange(1.2f, 2.5f) : 0.f;
					Stack.Timer = Random.FRand();
					Chimneys.Add(Stack);
					bStacksDirty = true;
				}
			}
		}
	}
}

void AChimneySmoke::BuildStacks()
{
	bStacksDirty = false;
	TArray<FVector> V[2];
	TArray<int32> T[2];
	TArray<FVector> N[2];
	const FVector Half(ChimneySmoke::StackSize.X * 0.5, ChimneySmoke::StackSize.Y * 0.5, 0.0);
	for (const FStack& Stack : Chimneys)
	{
		const FVector Foot = Stack.Top - FVector(0.0, 0.0, ChimneySmoke::StackSize.Z);
		ChimneySmoke::AddBox(V[0], T[0], N[0], Foot - Half, Stack.Top - Half + FVector(Half.X * 2.0, Half.Y * 2.0, -12.0));
		ChimneySmoke::AddBox(V[1], T[1], N[1], Stack.Top - Half - FVector(6.0, 6.0, 12.0), Stack.Top + Half + FVector(6.0, 6.0, 0.0));
	}
	const FLinearColor Paints[2] = { ChimneySmoke::Brick, ChimneySmoke::Cap };
	for (int32 s = 0; s < 2; ++s)
	{
		Stacks->CreateMeshSection_LinearColor(s, V[s], T[s], N[s],
			TArray<FVector2D>(), TArray<FLinearColor>(), TArray<FProcMeshTangent>(), false);
		if (BaseMaterial && !Stacks->GetMaterial(s))
		{
			UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(BaseMaterial, this);
			Material->SetVectorParameterValue(TEXT("Color"), Paints[s]);
			Stacks->SetMaterial(s, Material);
		}
	}
}

void AChimneySmoke::FindSteam()
{
	for (TActorIterator<AKeletiStation> It(GetWorld()); It; ++It)
	{
		bSteamFound = true;
		TArray<FVector> Chimney;
		TArray<FVector> Cylinder;
		AKeletiStation::SteamSpots(Chimney, Cylinder);
		const FTransform Frame = It->GetActorTransform();
		for (const FVector& Each : Chimney)
		{
			SteamChimneys.Add(Frame.TransformPosition(Each));
		}
		for (const FVector& Each : Cylinder)
		{
			SteamCylinders.Add(Frame.TransformPosition(Each));
			CylinderTimers.Add(Random.FRandRange(1.f, 6.f));
		}
		break;
	}
}

void AChimneySmoke::Emit(FPool& Pool, const FVector& At, const FVector& Velocity, float Size, float Grow, float Life)
{
	if (Pool.Puffs.Num() == 0)
	{
		return;
	}
	// The oldest puff makes way for the new one.
	FPuff& New = Pool.Puffs[Pool.Next];
	Pool.Next = (Pool.Next + 1) % Pool.Puffs.Num();
	New.At = At;
	New.Velocity = Velocity;
	New.Size = Size;
	New.Grow = Grow;
	New.Age = 0.f;
	New.Life = Life;
	Pool.bAny = true;
}

void AChimneySmoke::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (DeltaSeconds <= 0.f)
	{
		return;
	}
	const APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC || !PC->PlayerCameraManager)
	{
		return;
	}
	const FVector Eye = PC->PlayerCameraManager->GetCameraLocation();
	// The street's height: under the player's feet (the city is flat).
	const APawn* Player = PC->GetPawn();
	const double Street = Player ? Player->GetActorLocation().Z - 100.0 : Eye.Z - 250.0;

	ScanTimer -= DeltaSeconds;
	if (ScanTimer <= 0.f)
	{
		ScanTimer = ChimneySmoke::ScanEvery;
		Scan(Eye, Street);
	}
	// New stacks are drawn a few times a second, not every time one is found.
	StackTimer -= DeltaSeconds;
	if (bStacksDirty && StackTimer <= 0.f)
	{
		StackTimer = 0.5f;
		BuildStacks();
	}

	// More fires lit in the morning and the evening.
	const ADayNight* Clock = ADayNight::Get(GetWorld());
	const float Hour = Clock ? Clock->GetHour() : 12.f;
	const float Fires = (Hour > 5.f && Hour < 10.f) || (Hour > 16.f && Hour < 23.f) ? 1.f : 0.6f;
	for (FStack& Stack : Chimneys)
	{
		if (Stack.Rate <= 0.f || FVector::DistSquared2D(Stack.Top, Eye) > FMath::Square(ChimneySmoke::SmokeReach))
		{
			continue;
		}
		Stack.Timer -= DeltaSeconds * Stack.Rate * Fires;
		if (Stack.Timer <= 0.f)
		{
			Stack.Timer += 1.f;
			const FVector Up(Random.FRandRange(-10.f, 10.f), Random.FRandRange(-10.f, 10.f), Random.FRandRange(45.f, 70.f));
			Emit(Pools[0], Stack.Top + FVector(0.0, 0.0, 10.0), Up + ChimneySmoke::Wind * 0.3, Random.FRandRange(35.f, 50.f), Random.FRandRange(4.f, 6.f), Random.FRandRange(7.f, 10.f));
		}
	}

	// The engines at Keleti: soft steam from the chimney, now and then a white blast from a cylinder.
	if (!bSteamFound)
	{
		FindSteam();
	}
	SteamTimer -= DeltaSeconds;
	const bool bSteamNear = SteamChimneys.Num() > 0 && FVector::DistSquared2D(SteamChimneys[0], Eye) < FMath::Square(25000.0);
	if (bSteamNear && SteamTimer <= 0.f)
	{
		SteamTimer = 0.3f;
		for (const FVector& Top : SteamChimneys)
		{
			Emit(Pools[1], Top, FVector(Random.FRandRange(-15.f, 15.f), Random.FRandRange(-15.f, 15.f), Random.FRandRange(70.f, 110.f)),
				Random.FRandRange(30.f, 45.f), Random.FRandRange(3.f, 4.5f), Random.FRandRange(3.f, 4.5f));
		}
	}
	for (int32 c = 0; bSteamNear && c < SteamCylinders.Num(); ++c)
	{
		CylinderTimers[c] -= DeltaSeconds;
		if (CylinderTimers[c] <= 0.f)
		{
			CylinderTimers[c] = Random.FRandRange(4.f, 9.f);
			// Out sideways from the engine, low over the platform, rolling up.
			const FVector Out = SteamCylinders[c] - SteamChimneys[FMath::Min(c / 2, SteamChimneys.Num() - 1)];
			const FVector Side = FVector(0.0, FMath::Sign(Out.Y), 0.0);
			for (int32 k = 0; k < 8; ++k)
			{
				Emit(Pools[1], SteamCylinders[c], Side * Random.FRandRange(150.f, 260.f) + FVector(Random.FRandRange(-60.f, 60.f), 0.0, Random.FRandRange(10.f, 60.f)),
					Random.FRandRange(40.f, 60.f), Random.FRandRange(3.f, 5.f), Random.FRandRange(2.f, 3.5f));
			}
		}
	}

	for (int32 Section = 0; Section < 2; ++Section)
	{
		UpdatePool(Section, Pools[Section], DeltaSeconds);
	}
}

void AChimneySmoke::UpdatePool(int32 Section, FPool& Pool, float DeltaSeconds)
{
	// Nothing in the air, and nothing was last frame: the mesh is already empty.
	if (!Pool.bAny)
	{
		return;
	}
	const TArray<FVector>& Corners = ChimneySmoke::Ball();
	Vertices.Reset();
	Normals.Reset();
	bool bAny = false;
	const float Slowing = FMath::Exp(-ChimneySmoke::Drag * DeltaSeconds);
	for (FPuff& Each : Pool.Puffs)
	{
		float Scale = 0.f;
		if (Each.Age < Each.Life)
		{
			Each.Age += DeltaSeconds;
			// Slowed by the air, lifted, and carried off with the wind.
			Each.Velocity = (Each.Velocity - ChimneySmoke::Wind) * Slowing + ChimneySmoke::Wind;
			Each.Velocity.Z += ChimneySmoke::Lift * DeltaSeconds;
			Each.At += Each.Velocity * DeltaSeconds;
			// Swell as it spreads, then thin away to nothing at the end.
			const float T = FMath::Clamp(Each.Age / Each.Life, 0.f, 1.f);
			const float Fade = T < 0.1f ? T / 0.1f : T > 0.6f ? (1.f - T) / 0.4f : 1.f;
			Scale = Each.Size * FMath::Lerp(1.f, Each.Grow, FMath::Sqrt(T)) * Fade;
			bAny = true;
		}
		for (const FVector& Corner : Corners)
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
