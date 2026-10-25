// Fill out your copyright notice in the Description page of Project Settings.

#include "Pedestrians.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "OsmGraybox.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

DEFINE_LOG_CATEGORY_STATIC(LogPedestrians, Log, All);

namespace Pedestrians
{
	// Autumn coats of 1932: browns, greys, navy, black, loden and a few light ones.
	const FLinearColor Coats[] = {
		FLinearColor(0.10f, 0.07f, 0.05f),
		FLinearColor(0.16f, 0.12f, 0.08f),
		FLinearColor(0.12f, 0.12f, 0.12f),
		FLinearColor(0.05f, 0.06f, 0.10f),
		FLinearColor(0.03f, 0.03f, 0.03f),
		FLinearColor(0.10f, 0.12f, 0.07f),
		FLinearColor(0.35f, 0.30f, 0.22f),
		FLinearColor(0.22f, 0.10f, 0.08f),
	};

	constexpr float MinSpeed = 105.f;
	constexpr float MaxSpeed = 160.f;
	// Each keeps to the right of the walking line by this much.
	constexpr float MinLane = 40.f;
	constexpr float MaxLane = 100.f;
	constexpr float MinSize = 0.93f;
	constexpr float MaxSize = 1.06f;
	constexpr float Middle = 90.f;

	constexpr double EndCell = 1000.0;
	constexpr double JoinDistance = 700.0;
	constexpr double RefindLines = 2000.0;
	constexpr float GroundEvery = 0.3f;
	constexpr int32 SpawnPerFrame = 4;
	constexpr int32 PickTries = 24;
	constexpr double FillNear = 500.0;

	constexpr float LegSwing = 25.f;
	constexpr float ArmSwing = 18.f;
	constexpr float StrideLength = 110.f;

	FIntPoint CellOf(const FVector2D& At)
	{
		return FIntPoint(FMath::FloorToInt(At.X / EndCell), FMath::FloorToInt(At.Y / EndCell));
	}
}

APedestrian::APedestrian()
{
	// APedestrians moves him; he is no obstacle for cars or the player.
	PrimaryActorTick.bCanEverTick = false;
	Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void APedestrian::WalkTo(const FVector& Location, double Yaw, float Speed, float DeltaSeconds)
{
	SetActorLocationAndRotation(Location, FRotator(0.0, Yaw, 0.0));

	Stride += DeltaSeconds * Speed / Pedestrians::StrideLength * PI;
	const float Swing = FMath::Sin(Stride);
	const float Pitches[] = { Swing * Pedestrians::LegSwing, -Swing * Pedestrians::LegSwing, -Swing * Pedestrians::ArmSwing, Swing * Pedestrians::ArmSwing };
	for (int32 i = 0; i < static_cast<int32>(UE_ARRAY_COUNT(Pitches)) && i < BodyParts.Num(); ++i)
	{
		if (BodyParts[i])
		{
			BodyParts[i]->SetRelativeRotation(FRotator(Pitches[i], 0.f, 0.f));
		}
	}
}

APedestrians::APedestrians()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void APedestrians::BeginPlay()
{
	Super::BeginPlay();

	Random.Initialize(1932);
	bReady = LoadLines();
}

bool APedestrians::LoadLines()
{
	const FString Path = FPaths::Combine(FPaths::ProjectDir(), DataFile);
	FString Text;
	TSharedPtr<FJsonObject> Root;
	if (!FFileHelper::LoadFileToString(Text, *Path)
		|| !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) || !Root.IsValid())
	{
		UE_LOG(LogPedestrians, Error, TEXT("Could not read %s"), *Path);
		return false;
	}
	const TArray<TSharedPtr<FJsonValue>>* Rows = nullptr;
	if (!Root->TryGetArrayField(TEXT("sidewalks"), Rows))
	{
		UE_LOG(LogPedestrians, Warning, TEXT("No sidewalks in %s"), *Path);
		return false;
	}

	for (const TSharedPtr<FJsonValue>& Row : *Rows)
	{
		const TArray<TSharedPtr<FJsonValue>>& Numbers = Row->AsArray();
		FLine Line;
		for (int32 i = 0; i + 1 < Numbers.Num(); i += 2)
		{
			const FVector2D Point(Numbers[i]->AsNumber(), Numbers[i + 1]->AsNumber());
			Line.Length += Line.Points.Num() > 0 ? FVector2D::Distance(Line.Points.Last(), Point) : 0.0;
			Line.Points.Add(Point);
			Line.Along.Add(Line.Length);
		}
		if (Line.Points.Num() < 2 || Line.Length < 1.0)
		{
			continue;
		}
		const int32 Index = Lines.Add(MoveTemp(Line));
		Ends.Add(Pedestrians::CellOf(Lines[Index].Points[0]), FIntPoint(Index, 0));
		Ends.Add(Pedestrians::CellOf(Lines[Index].Points.Last()), FIntPoint(Index, 1));
	}
	UE_LOG(LogPedestrians, Log, TEXT("Pedestrians: %d walking lines"), Lines.Num());
	return Lines.Num() > 0;
}

bool APedestrians::CameraLocation(FVector& OutLocation) const
{
	const APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC || !PC->PlayerCameraManager)
	{
		return false;
	}
	OutLocation = PC->PlayerCameraManager->GetCameraLocation();
	return true;
}

FVector2D APedestrians::PointOn(const FLine& Line, double Along, FVector2D& OutTangent) const
{
	int32 i = 0;
	while (i + 2 < Line.Points.Num() && Line.Along[i + 1] < Along)
	{
		++i;
	}
	const FVector2D A = Line.Points[i];
	const FVector2D B = Line.Points[i + 1];
	const double Span = FMath::Max(Line.Along[i + 1] - Line.Along[i], 1.0);
	OutTangent = (B - A).GetSafeNormal();
	return A + (B - A) * FMath::Clamp((Along - Line.Along[i]) / Span, 0.0, 1.0);
}

bool APedestrians::PickSpot(const FVector& Camera, double MinDistance, int32& OutLine, double& OutAlong)
{
	if (FVector::Dist2D(Camera, NearLinesFrom) > Pedestrians::RefindLines)
	{
		NearLinesFrom = Camera;
		NearLines.Reset();
		for (int32 i = 0; i < Lines.Num(); ++i)
		{
			for (const FVector2D& Point : Lines[i].Points)
			{
				if (FVector2D::Distance(Point, FVector2D(Camera)) < Range + Lines[i].Length)
				{
					NearLines.Add(i);
					break;
				}
			}
		}
	}
	if (NearLines.IsEmpty())
	{
		return false;
	}
	for (int32 Try = 0; Try < Pedestrians::PickTries; ++Try)
	{
		const int32 Index = NearLines[Random.RandRange(0, NearLines.Num() - 1)];
		const double Along = Random.FRandRange(0.f, 1.f) * Lines[Index].Length;
		FVector2D Tangent;
		const double Distance = FVector2D::Distance(PointOn(Lines[Index], Along, Tangent), FVector2D(Camera));
		if (Distance >= MinDistance && Distance < Range)
		{
			OutLine = Index;
			OutAlong = Along;
			return true;
		}
	}
	return false;
}

void APedestrians::TurnAtEnd(FWalker& Walker)
{
	const FLine& Line = Lines[Walker.Line];
	const FVector2D End = Walker.Dir > 0 ? Line.Points.Last() : Line.Points[0];
	const FIntPoint Cell = Pedestrians::CellOf(End);

	TArray<FIntPoint> Joins;
	for (int32 dx = -1; dx <= 1; ++dx)
	{
		for (int32 dy = -1; dy <= 1; ++dy)
		{
			TArray<FIntPoint> Found;
			Ends.MultiFind(Cell + FIntPoint(dx, dy), Found);
			for (const FIntPoint& Other : Found)
			{
				const FLine& Next = Lines[Other.X];
				const FVector2D Start = Other.Y == 0 ? Next.Points[0] : Next.Points.Last();
				if (Other.X != Walker.Line && FVector2D::Distance(Start, End) < Pedestrians::JoinDistance)
				{
					Joins.Add(Other);
				}
			}
		}
	}

	if (Joins.IsEmpty())
	{
		// A dead end: turn round.
		Walker.Dir = -Walker.Dir;
		return;
	}
	const FIntPoint Join = Joins[Random.RandRange(0, Joins.Num() - 1)];
	Walker.Line = Join.X;
	Walker.Dir = Join.Y == 0 ? 1 : -1;
	Walker.Along = Join.Y == 0 ? 0.0 : Lines[Join.X].Length;
}

bool APedestrians::GroundUnder(const FVector2D& At, double Near, double& OutZ) const
{
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(PedestrianGround), false, this);
	const double Top = Near + 300.0;
	if (!GetWorld()->LineTraceSingleByChannel(Hit, FVector(At, Top), FVector(At, Near - 2000.0), ECC_Visibility, Params))
	{
		return false;
	}
	// Only the open ground: not a lamp, a bench or a car in the way.
	if (!Cast<AOsmGraybox>(Hit.GetActor()))
	{
		return false;
	}
	OutZ = Hit.ImpactPoint.Z;
	return true;
}

void APedestrians::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	FVector Camera;
	if (!bReady || !CameraLocation(Camera))
	{
		return;
	}

	// Bring the crowd up to strength: close by at the start, out of the way later.
	const bool bFilling = GetWorld()->GetTimeSeconds() < 2.0;
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	for (int32 n = 0; n < Pedestrians::SpawnPerFrame && Walkers.Num() < Crowd; ++n)
	{
		FWalker Walker;
		if (!PickSpot(Camera, bFilling ? Pedestrians::FillNear : AppearBeyond, Walker.Line, Walker.Along))
		{
			break;
		}
		Walker.Dir = Random.FRand() < 0.5f ? 1 : -1;
		Walker.Speed = Random.FRandRange(Pedestrians::MinSpeed, Pedestrians::MaxSpeed);
		Walker.Lane = Random.FRandRange(Pedestrians::MinLane, Pedestrians::MaxLane);
		FVector2D Tangent;
		const FVector2D At = PointOn(Lines[Walker.Line], Walker.Along, Tangent);
		Walker.Person = GetWorld()->SpawnActor<APedestrian>(APedestrian::StaticClass(), FVector(At, Camera.Z), FRotator::ZeroRotator, Params);
		if (!Walker.Person)
		{
			break;
		}
		const int32 Coat = Random.RandRange(0, static_cast<int32>(UE_ARRAY_COUNT(Pedestrians::Coats)) - 1);
		Walker.Person->SetCoatColor(Pedestrians::Coats[Coat]);
		Walker.Person->SetActorScale3D(FVector(Random.FRandRange(Pedestrians::MinSize, Pedestrians::MaxSize)));
		Walkers.Add(Walker);
	}

	for (FWalker& Walker : Walkers)
	{
		if (!IsValid(Walker.Person))
		{
			continue;
		}

		// Fallen far behind: start again somewhere out of the way nearer the camera.
		if (FVector::Dist2D(Walker.Person->GetActorLocation(), Camera) > Range * 1.2)
		{
			if (PickSpot(Camera, AppearBeyond, Walker.Line, Walker.Along))
			{
				Walker.bGroundKnown = false;
			}
		}

		Walker.Along += Walker.Dir * Walker.Speed * DeltaSeconds;
		const FLine& Line = Lines[Walker.Line];
		if (Walker.Along < 0.0 || Walker.Along > Line.Length)
		{
			Walker.Along = FMath::Clamp(Walker.Along, 0.0, Line.Length);
			TurnAtEnd(Walker);
		}

		FVector2D Tangent;
		const FVector2D OnLine = PointOn(Lines[Walker.Line], Walker.Along, Tangent);
		const FVector2D Heading = Tangent * Walker.Dir;
		// Keep to the right, so people going opposite ways pass each other.
		const FVector2D At = OnLine + FVector2D(-Heading.Y, Heading.X) * Walker.Lane;

		Walker.GroundClock -= DeltaSeconds;
		if (!Walker.bGroundKnown || Walker.GroundClock <= 0.f)
		{
			Walker.GroundClock = Pedestrians::GroundEvery;
			double Z;
			if (GroundUnder(At, Walker.bGroundKnown ? Walker.GroundZ : Camera.Z, Z))
			{
				Walker.GroundZ = Z;
				Walker.bGroundKnown = true;
			}
		}

		const double Size = Walker.Person->GetActorScale3D().Z;
		const double Yaw = FMath::RadiansToDegrees(FMath::Atan2(Heading.Y, Heading.X));
		if (Walker.Person->IsHidden() == Walker.bGroundKnown)
		{
			Walker.Person->SetActorHiddenInGame(!Walker.bGroundKnown);
		}
		Walker.Person->WalkTo(FVector(At, Walker.GroundZ + Pedestrians::Middle * Size), Yaw, Walker.Speed, DeltaSeconds);
	}
}
