// Fill out your copyright notice in the Description page of Project Settings.

#include "TrafficManager.h"

#include "Dom/JsonObject.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "TrafficCar.h"

DEFINE_LOG_CATEGORY_STATIC(LogTraffic, Log, All);

namespace Traffic
{
	// Car paint of the early 1930s: mostly black, some dark colours, a few light ones.
	const FLinearColor Paints[] = {
		FLinearColor(0.01f, 0.01f, 0.01f),
		FLinearColor(0.01f, 0.01f, 0.01f),
		FLinearColor(0.01f, 0.01f, 0.01f),
		FLinearColor(0.02f, 0.05f, 0.03f),
		FLinearColor(0.08f, 0.01f, 0.01f),
		FLinearColor(0.01f, 0.02f, 0.07f),
		FLinearColor(0.45f, 0.40f, 0.30f),
	};

	// Half the width of the corridor a car watches for others (cm).
	constexpr float CorridorHalfWidth = 220.f;
	// Height of the car's centre above the road surface (hover + half height).
	constexpr float RideHeight = 90.f;
}

ATrafficManager::ATrafficManager()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ATrafficManager::BeginPlay()
{
	Super::BeginPlay();

	if (LoadLanes())
	{
		SpawnCars();
	}
}

bool ATrafficManager::LoadLanes()
{
	const FString Path = FPaths::Combine(FPaths::ProjectDir(), DataFile);
	FString Text;
	TSharedPtr<FJsonObject> Root;
	if (!FFileHelper::LoadFileToString(Text, *Path)
		|| !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) || !Root.IsValid())
	{
		UE_LOG(LogTraffic, Error, TEXT("Could not read lanes from %s"), *Path);
		return false;
	}

	const TArray<TSharedPtr<FJsonValue>>* LaneValues = nullptr;
	if (!Root->TryGetArrayField(TEXT("lanes"), LaneValues))
	{
		UE_LOG(LogTraffic, Error, TEXT("%s has no lanes; rerun osm_to_graybox.py"), *Path);
		return false;
	}

	int32 MaxNode = INDEX_NONE;
	for (const TSharedPtr<FJsonValue>& LaneValue : *LaneValues)
	{
		const TSharedPtr<FJsonObject>& LaneObject = LaneValue->AsObject();
		const TArray<TSharedPtr<FJsonValue>>* Coords = nullptr;
		if (!LaneObject.IsValid() || !LaneObject->TryGetArrayField(TEXT("points"), Coords) || Coords->Num() < 4)
		{
			continue;
		}

		FTrafficLane& Lane = Lanes.AddDefaulted_GetRef();
		for (int32 i = 0; i + 1 < Coords->Num(); i += 2)
		{
			Lane.Points.Emplace((*Coords)[i]->AsNumber(), (*Coords)[i + 1]->AsNumber(), 0.0);
		}
		Lane.FromNode = static_cast<int32>(LaneObject->GetNumberField(TEXT("from")));
		Lane.ToNode = static_cast<int32>(LaneObject->GetNumberField(TEXT("to")));
		Lane.Speed = static_cast<float>(LaneObject->GetNumberField(TEXT("speed")));
		Lane.Way = LaneObject->GetStringField(TEXT("way"));
		MaxNode = FMath::Max3(MaxNode, Lane.FromNode, Lane.ToNode);
	}

	LanesFromNode.SetNum(MaxNode + 1);
	NodeLocations.SetNum(MaxNode + 1);
	NodeClaims.SetNum(MaxNode + 1);
	for (int32 i = 0; i < Lanes.Num(); ++i)
	{
		const FTrafficLane& Lane = Lanes[i];
		if (Lane.FromNode >= 0)
		{
			LanesFromNode[Lane.FromNode].Add(i);
			NodeLocations[Lane.FromNode] = Lane.Points[0];
		}
		if (Lane.ToNode >= 0)
		{
			NodeLocations[Lane.ToNode] = Lane.Points.Last();
		}
	}

	UE_LOG(LogTraffic, Log, TEXT("Loaded %d traffic lanes"), Lanes.Num());
	return Lanes.Num() > 0;
}

void ATrafficManager::SpawnCars()
{
	UWorld* World = GetWorld();
	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	const FVector PlayerLocation = Player ? Player->GetActorLocation() : FVector::ZeroVector;

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	TArray<FVector> Taken;
	const int32 MaxAttempts = NumCars * 20;
	for (int32 Attempt = 0; Attempt < MaxAttempts && Cars.Num() < NumCars; ++Attempt)
	{
		const int32 LaneIdx = FMath::RandRange(0, Lanes.Num() - 1);
		const FTrafficLane& Lane = Lanes[LaneIdx];
		const int32 PointIdx = FMath::RandRange(0, Lane.Points.Num() - 2);
		FVector Location = Lane.Points[PointIdx];

		if (FVector::Dist2D(Location, PlayerLocation) < 2500.f)
		{
			continue;
		}
		bool bTooClose = false;
		for (const FVector& Other : Taken)
		{
			if (FVector::Dist2D(Location, Other) < MinSpawnSpacing)
			{
				bTooClose = true;
				break;
			}
		}
		if (bTooClose)
		{
			continue;
		}

		// Find the road surface under the spawn point.
		FHitResult Hit;
		const FVector TraceStart = Location + FVector(0.f, 0.f, 1000.f);
		const FVector TraceEnd = Location - FVector(0.f, 0.f, 1000.f);
		if (World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility))
		{
			Location.Z = Hit.ImpactPoint.Z;
		}
		Location.Z += Traffic::RideHeight;

		const FRotator Facing = (Lane.Points[PointIdx + 1] - Lane.Points[PointIdx]).GetSafeNormal2D().Rotation();
		ATrafficCar* Car = World->SpawnActor<ATrafficCar>(ATrafficCar::StaticClass(), Location, Facing, SpawnParams);
		if (!Car)
		{
			continue;
		}
		const FLinearColor& Paint = Traffic::Paints[FMath::RandRange(0, static_cast<int32>(UE_ARRAY_COUNT(Traffic::Paints)) - 1)];
		Car->StartOnLane(this, LaneIdx, PointIdx + 1, Paint);
		Cars.Add(Car);
		Taken.Add(Location);
	}

	UE_LOG(LogTraffic, Log, TEXT("Spawned %d traffic cars"), Cars.Num());
}

int32 ATrafficManager::PickNextLane(int32 LaneIndex) const
{
	const FTrafficLane& Current = Lanes[LaneIndex];
	if (!LanesFromNode.IsValidIndex(Current.ToNode))
	{
		return LaneIndex;
	}

	TArray<int32> Choices;
	int32 UTurn = INDEX_NONE;
	for (const int32 Next : LanesFromNode[Current.ToNode])
	{
		const FTrafficLane& Candidate = Lanes[Next];
		if (Candidate.Way == Current.Way && Candidate.ToNode == Current.FromNode)
		{
			UTurn = Next;
		}
		else
		{
			Choices.Add(Next);
		}
	}
	if (Choices.Num() > 0)
	{
		return Choices[FMath::RandRange(0, Choices.Num() - 1)];
	}
	return UTurn != INDEX_NONE ? UTurn : LaneIndex;
}

bool ATrafficManager::TryClaimNode(int32 Node, const ATrafficCar* Car, bool bForce)
{
	// Two lanes out means a street simply carries on; only real junctions need turns.
	if (!NodeClaims.IsValidIndex(Node) || LanesFromNode[Node].Num() <= 2)
	{
		return true;
	}

	// A claim older than this is stale (its car got stuck or was destroyed).
	constexpr double ClaimTimeout = 8.0;
	const double Now = GetWorld()->GetTimeSeconds();
	FNodeClaim& Claim = NodeClaims[Node];
	if (bForce || !Claim.Holder.IsValid() || Claim.Holder.Get() == Car || Now - Claim.Time > ClaimTimeout)
	{
		Claim.Holder = Car;
		Claim.Time = Now;
		return true;
	}
	return false;
}

void ATrafficManager::ReleaseNode(int32 Node, const ATrafficCar* Car)
{
	if (NodeClaims.IsValidIndex(Node) && NodeClaims[Node].Holder.Get() == Car)
	{
		NodeClaims[Node].Holder.Reset();
	}
}

FVector ATrafficManager::GetNodeLocation(int32 Node) const
{
	return NodeLocations.IsValidIndex(Node) ? NodeLocations[Node] : FVector::ZeroVector;
}

float ATrafficManager::ClearDistanceAhead(const AActor* Car, float LookAhead) const
{
	const FVector Origin = Car->GetActorLocation();
	const FVector Forward = Car->GetActorForwardVector().GetSafeNormal2D();
	const FVector Right = FVector(-Forward.Y, Forward.X, 0.f);

	float Clear = LookAhead;
	auto Consider = [&](const AActor* Other)
	{
		if (!Other || Other == Car)
		{
			return;
		}
		const FVector Offset = Other->GetActorLocation() - Origin;
		const float Ahead = static_cast<float>(FVector::DotProduct(Offset, Forward));
		const float Side = static_cast<float>(FMath::Abs(FVector::DotProduct(Offset, Right)));
		if (Ahead > 0.f && Ahead < Clear && Side < Traffic::CorridorHalfWidth)
		{
			Clear = Ahead;
		}
	};

	for (const ATrafficCar* Other : Cars)
	{
		Consider(Other);
	}
	Consider(UGameplayStatics::GetPlayerPawn(this, 0));
	return Clear;
}
