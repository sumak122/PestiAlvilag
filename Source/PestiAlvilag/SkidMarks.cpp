// Fill out your copyright notice in the Description page of Project Settings.

#include "SkidMarks.h"

#include "Engine/Texture.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

namespace SkidMarks
{
	// Marks are kept in this many batches; a batch takes new marks for this
	// long (s) or this many strips, then the next (oldest) one is wiped for them.
	constexpr int32 NumBatches = 4;
	constexpr double BatchTime = 15.0;
	constexpr int32 MaxStrips = 600;
	// A tyre draws a new strip once it has gone this far (cm); a gap longer
	// than StripBreak cm or MarkBreak s starts a new line.
	constexpr double StripLength = 25.0;
	constexpr double StripBreak = 250.0;
	constexpr double MarkBreak = 0.2;
	// The marks are this wide (cm) faint and strong, and lie this far above the ground (cm).
	constexpr double ThinWidth = 10.0;
	constexpr double FullWidth = 18.0;
	constexpr double Lift = 3.0;
	// Rubber black, this see-through when new; each older batch is fainter.
	const FLinearColor MarkColor(0.012f, 0.011f, 0.010f, 0.f);
	constexpr float NewAlpha = 0.75f;
	// Lines no tyre has added to for this long (s) are forgotten.
	constexpr double ForgetTrail = 3.0;
}

ASkidMarks::ASkidMarks()
{
	PrimaryActorTick.bCanEverTick = true;

	Mesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Mesh"));
	RootComponent = Mesh;
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCanEverAffectNavigation(false);
	// Changed often: kept out of the ray-traced lighting, the distance fields
	// and the shadows, which would cost far more than the marks are worth.
	Mesh->SetCastShadow(false);
	Mesh->bVisibleInRayTracing = false;
	Mesh->bAffectDistanceFieldLighting = false;
	Mesh->bAffectDynamicIndirectLighting = false;

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Translucent(TEXT("/Engine/EngineMaterials/Widget3DPassThrough_Translucent.Widget3DPassThrough_Translucent"));
	SeeThroughMaterial = Translucent.Object;
	static ConstructorHelpers::FObjectFinder<UTexture> White(TEXT("/Engine/EngineResources/WhiteSquareTexture.WhiteSquareTexture"));
	WhiteTexture = White.Object;
}

ASkidMarks* ASkidMarks::Get(UWorld* World)
{
	static TWeakObjectPtr<ASkidMarks> Found;
	if (!World)
	{
		return nullptr;
	}
	if (Found.IsValid() && Found->GetWorld() == World)
	{
		return Found.Get();
	}
	for (TActorIterator<ASkidMarks> It(World); It; ++It)
	{
		Found = *It;
		return *It;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ASkidMarks* Marks = World->SpawnActor<ASkidMarks>(ASkidMarks::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Params);
	Found = Marks;
	return Marks;
}

void ASkidMarks::BeginPlay()
{
	Super::BeginPlay();

	// The mesh is in world space; the actor stays at the origin.
	SetActorLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);
	Batches.SetNum(SkidMarks::NumBatches);
	BatchMaterials.Reset();
	for (int32 i = 0; i < SkidMarks::NumBatches; ++i)
	{
		UMaterialInstanceDynamic* Material = nullptr;
		if (SeeThroughMaterial)
		{
			Material = UMaterialInstanceDynamic::Create(SeeThroughMaterial, this);
			if (WhiteTexture)
			{
				Material->SetTextureParameterValue(TEXT("SlateUI"), WhiteTexture);
			}
			Material->SetScalarParameterValue(TEXT("OpacityFromTexture"), 0.f);
		}
		BatchMaterials.Add(Material);
	}
	Newest = 0;
	NewestSince = GetWorld()->GetTimeSeconds();
	FadeBatches();
}

void ASkidMarks::FadeBatches()
{
	for (int32 i = 0; i < BatchMaterials.Num(); ++i)
	{
		if (!BatchMaterials[i])
		{
			continue;
		}
		// 0 is the newest batch, NumBatches - 1 the oldest.
		const int32 Age = (Newest - i + SkidMarks::NumBatches) % SkidMarks::NumBatches;
		FLinearColor Tint = SkidMarks::MarkColor;
		Tint.A = SkidMarks::NewAlpha * (1.f - static_cast<float>(Age) / SkidMarks::NumBatches);
		BatchMaterials[i]->SetVectorParameterValue(TEXT("TintColorAndOpacity"), Tint);
	}
}

void ASkidMarks::NextBatch(double Now)
{
	Newest = (Newest + 1) % SkidMarks::NumBatches;
	NewestSince = Now;
	FBatch& Wiped = Batches[Newest];
	Wiped.Vertices.Reset();
	Wiped.Triangles.Reset();
	Wiped.Normals.Reset();
	Wiped.Strips = 0;
	Wiped.bDirty = true;
	FadeBatches();
}

void ASkidMarks::Mark(int32 Tyre, const FVector& At, float Strength)
{
	if (Batches.Num() != SkidMarks::NumBatches || !GetWorld())
	{
		return;
	}
	const double Now = GetWorld()->GetTimeSeconds();
	FTrail& Trail = Trails.FindOrAdd(Tyre);
	const double Gone = FVector::Dist2D(Trail.Last, At);
	if (Now - Trail.Time > SkidMarks::MarkBreak || Gone > SkidMarks::StripBreak)
	{
		// A new line starts here.
		Trail.Last = At;
		Trail.Time = Now;
		return;
	}
	Trail.Time = Now;
	if (Gone < SkidMarks::StripLength)
	{
		return;
	}

	if (Batches[Newest].Strips >= SkidMarks::MaxStrips)
	{
		NextBatch(Now);
	}
	FBatch& Batch = Batches[Newest];
	const FVector Along = FVector(At.X - Trail.Last.X, At.Y - Trail.Last.Y, 0.0).GetSafeNormal();
	const double Width = FMath::Lerp(SkidMarks::ThinWidth, SkidMarks::FullWidth, static_cast<double>(FMath::Clamp(Strength, 0.f, 1.f)));
	const FVector Across = FVector(-Along.Y, Along.X, 0.0) * (Width * 0.5);
	const FVector Up(0.0, 0.0, SkidMarks::Lift);
	const int32 First = Batch.Vertices.Num();
	Batch.Vertices.Add(Trail.Last - Across + Up);
	Batch.Vertices.Add(Trail.Last + Across + Up);
	Batch.Vertices.Add(At + Across + Up);
	Batch.Vertices.Add(At - Across + Up);
	for (int32 i = 0; i < 4; ++i)
	{
		Batch.Normals.Add(FVector::UpVector);
	}
	// Both ways round, so it shows whichever way the faces are taken to point.
	const int32 Faces[] = { 0, 1, 2, 0, 2, 3, 0, 2, 1, 0, 3, 2 };
	for (const int32 Corner : Faces)
	{
		Batch.Triangles.Add(First + Corner);
	}
	++Batch.Strips;
	Batch.bDirty = true;
	if (!bLoggedFirst)
	{
		// Once, so a test can tell from the log that marks are being laid.
		bLoggedFirst = true;
		UE_LOG(LogTemp, Log, TEXT("SkidMarks: first tyre mark at %s, strength %.2f"), *At.ToString(), Strength);
	}
	Trail.Last = At;
}

void ASkidMarks::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const double Now = GetWorld()->GetTimeSeconds();
	if (Batches.Num() != SkidMarks::NumBatches)
	{
		return;
	}
	if (Batches[Newest].Strips > 0 && Now - NewestSince > SkidMarks::BatchTime)
	{
		NextBatch(Now);
	}
	for (int32 i = 0; i < Batches.Num(); ++i)
	{
		FBatch& Batch = Batches[i];
		if (!Batch.bDirty)
		{
			continue;
		}
		Batch.bDirty = false;
		if (Batch.Strips == 0)
		{
			Mesh->ClearMeshSection(i);
			continue;
		}
		Mesh->CreateMeshSection_LinearColor(i, Batch.Vertices, Batch.Triangles, Batch.Normals,
			TArray<FVector2D>(), TArray<FLinearColor>(), TArray<FProcMeshTangent>(), false);
		if (BatchMaterials.IsValidIndex(i) && BatchMaterials[i])
		{
			Mesh->SetMaterial(i, BatchMaterials[i]);
		}
	}

	// Lines no tyre is drawing any more are forgotten.
	for (auto It = Trails.CreateIterator(); It; ++It)
	{
		if (Now - It.Value().Time > SkidMarks::ForgetTrail)
		{
			It.RemoveCurrent();
		}
	}
}
