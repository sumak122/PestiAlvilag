// Fill out your copyright notice in the Description page of Project Settings.

#include "TrafficManager.h"

#include "Algo/BinarySearch.h"
#include "Dom/JsonObject.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "TrafficCar.h"
#include "Tram.h"

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
	PrimaryActorTick.bCanEverTick = true;
}

void ATrafficManager::BeginPlay()
{
	Super::BeginPlay();

	const FString Path = FPaths::Combine(FPaths::ProjectDir(), DataFile);
	FString Text;
	TSharedPtr<FJsonObject> Root;
	if (!FFileHelper::LoadFileToString(Text, *Path)
		|| !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) || !Root.IsValid())
	{
		UE_LOG(LogTraffic, Error, TEXT("Could not read %s"), *Path);
		return;
	}

	if (LoadLanes(*Root))
	{
		SpawnCars();
	}
	LoadTramRoutes(*Root);
	SpawnTrams();
}

void ATrafficManager::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (PC && PC->WasInputKeyJustPressed(EKeys::N))
	{
		bShowDebug = !bShowDebug;
	}
	if (bShowDebug)
	{
		DrawDebugView();
	}
}

void ATrafficManager::DrawDebugView() const
{
#if ENABLE_DRAW_DEBUG
	// Like the 2D game's node view: blue lanes, junctions green when free and
	// red when a car holds them, and each car's route ahead in yellow.
	UWorld* World = GetWorld();
	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	const FVector Center = Player ? Player->GetActorLocation() : FVector::ZeroVector;
	const double RangeSquared = FMath::Square(static_cast<double>(DebugRange));
	const FVector Lift(0.f, 0.f, 40.f);

	for (const FTrafficLane& Lane : Lanes)
	{
		if (FVector::DistSquared2D(Lane.Points[0], Center) > RangeSquared)
		{
			continue;
		}
		for (int32 i = 0; i + 1 < Lane.Points.Num(); ++i)
		{
			DrawDebugLine(World, Lane.Points[i] + Lift, Lane.Points[i + 1] + Lift, FColor(60, 120, 255), false, -1.f, 0, 4.f);
		}
	}

	for (int32 Node = 0; Node < NodeLocations.Num(); ++Node)
	{
		if (!IsJunction(Node) || FVector::DistSquared2D(NodeLocations[Node], Center) > RangeSquared)
		{
			continue;
		}
		const bool bTaken = NodeClaims[Node].Holder.IsValid();
		DrawDebugPoint(World, NodeLocations[Node] + Lift * 2.f, 20.f, bTaken ? FColor::Red : FColor::Green, false, -1.f, SDPG_Foreground);
	}

	// Tram routes in magenta, their stops as cyan points.
	for (const FTramRoute& Route : TramRoutes)
	{
		for (int32 i = 0; i + 1 < Route.Points.Num(); ++i)
		{
			if (FVector::DistSquared2D(Route.Points[i], Center) < RangeSquared)
			{
				DrawDebugLine(World, Route.Points[i] + Lift, Route.Points[i + 1] + Lift, FColor::Magenta, false, -1.f, 0, 6.f);
			}
		}
		for (const float Stop : Route.Stops)
		{
			const FVector StopLocation = Route.LocationAt(Stop);
			if (FVector::DistSquared2D(StopLocation, Center) < RangeSquared)
			{
				DrawDebugPoint(World, StopLocation + Lift * 2.f, 16.f, FColor::Cyan, false, -1.f, SDPG_Foreground);
			}
		}
	}

	for (const ATrafficCar* Car : Cars)
	{
		if (!Car || FVector::DistSquared2D(Car->GetActorLocation(), Center) > RangeSquared || !Lanes.IsValidIndex(Car->GetLaneIndex()))
		{
			continue;
		}
		FVector Previous = Car->GetActorLocation();
		const FTrafficLane& Current = Lanes[Car->GetLaneIndex()];
		for (int32 i = Car->GetPointIndex(); i < Current.Points.Num(); ++i)
		{
			DrawDebugLine(World, Previous, Current.Points[i] + Lift, FColor::Yellow, false, -1.f, SDPG_Foreground, 8.f);
			Previous = Current.Points[i] + Lift;
		}
		if (Lanes.IsValidIndex(Car->GetNextLaneIndex()))
		{
			for (const FVector& Point : Lanes[Car->GetNextLaneIndex()].Points)
			{
				DrawDebugLine(World, Previous, Point + Lift, FColor::Yellow, false, -1.f, SDPG_Foreground, 8.f);
				Previous = Point + Lift;
			}
		}
	}
#endif
}

bool ATrafficManager::LoadLanes(const FJsonObject& Root)
{
	const TArray<TSharedPtr<FJsonValue>>* LaneValues = nullptr;
	if (!Root.TryGetArrayField(TEXT("lanes"), LaneValues))
	{
		UE_LOG(LogTraffic, Error, TEXT("%s has no lanes; rerun osm_to_graybox.py"), *DataFile);
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

FVector FTramRoute::LocationAt(float Distance) const
{
	if (Points.Num() == 0)
	{
		return FVector::ZeroVector;
	}
	if (Distance <= 0.f)
	{
		return Points[0];
	}
	if (Distance >= Length)
	{
		return Points.Last();
	}
	const int32 Upper = Algo::UpperBound(PointDistances, Distance);
	const int32 i = FMath::Clamp(Upper - 1, 0, Points.Num() - 2);
	const float SegmentLength = PointDistances[i + 1] - PointDistances[i];
	const float Alpha = SegmentLength > 0.f ? (Distance - PointDistances[i]) / SegmentLength : 0.f;
	return FMath::Lerp(Points[i], Points[i + 1], Alpha);
}

void ATrafficManager::LoadTramRoutes(const FJsonObject& Root)
{
	const TArray<TSharedPtr<FJsonValue>>* RouteValues = nullptr;
	if (!Root.TryGetArrayField(TEXT("trams"), RouteValues))
	{
		UE_LOG(LogTraffic, Warning, TEXT("%s has no tram routes; rerun osm_to_graybox.py"), *DataFile);
		return;
	}

	for (const TSharedPtr<FJsonValue>& RouteValue : *RouteValues)
	{
		const TSharedPtr<FJsonObject>& RouteObject = RouteValue->AsObject();
		const TArray<TSharedPtr<FJsonValue>>* Coords = nullptr;
		if (!RouteObject.IsValid() || !RouteObject->TryGetArrayField(TEXT("points"), Coords) || Coords->Num() < 4)
		{
			continue;
		}

		FTramRoute& Route = TramRoutes.AddDefaulted_GetRef();
		Route.Ref = RouteObject->GetStringField(TEXT("ref"));
		for (int32 i = 0; i + 1 < Coords->Num(); i += 2)
		{
			const FVector Point((*Coords)[i]->AsNumber(), (*Coords)[i + 1]->AsNumber(), 0.0);
			if (Route.Points.Num() > 0)
			{
				Route.Length += static_cast<float>(FVector::Dist2D(Route.Points.Last(), Point));
			}
			Route.Points.Add(Point);
			Route.PointDistances.Add(Route.Length);
		}

		const TArray<TSharedPtr<FJsonValue>>* StopValues = nullptr;
		if (RouteObject->TryGetArrayField(TEXT("stops"), StopValues))
		{
			for (const TSharedPtr<FJsonValue>& StopValue : *StopValues)
			{
				const TSharedPtr<FJsonObject>& Stop = StopValue->AsObject();
				if (Stop.IsValid())
				{
					Route.Stops.Add(static_cast<float>(Stop->GetNumberField(TEXT("at"))));
				}
			}
			Route.Stops.Sort();
		}
	}
	UE_LOG(LogTraffic, Log, TEXT("Loaded %d tram routes"), TramRoutes.Num());
}

void ATrafficManager::SpawnTrams()
{
	UWorld* World = GetWorld();
	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	for (int32 RouteIdx = 0; RouteIdx < TramRoutes.Num(); ++RouteIdx)
	{
		const FTramRoute& Route = TramRoutes[RouteIdx];
		const int32 Count = FMath::Max(1, FMath::RoundToInt(Route.Length / TramSpacing));
		const float Spacing = Route.Length / Count;
		const float Offset = FMath::FRandRange(0.f, Spacing);
		for (int32 n = 0; n < Count; ++n)
		{
			const float Distance = Offset + n * Spacing;
			const FVector Location = Route.LocationAt(Distance);
			if (Player && FVector::Dist2D(Location, Player->GetActorLocation()) < 3000.f)
			{
				continue;
			}
			ATram* NewTram = World->SpawnActor<ATram>(ATram::StaticClass(), Location, FRotator::ZeroRotator, SpawnParams);
			if (NewTram)
			{
				NewTram->StartOnRoute(this, RouteIdx, Distance);
				Trams.Add(NewTram);
			}
		}
	}
	UE_LOG(LogTraffic, Log, TEXT("Spawned %d trams"), Trams.Num());
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

		// Find the road surface under the spawn point. Start low, so a bridge
		// over the street isn't mistaken for the road.
		FHitResult Hit;
		const FVector TraceStart = Location + FVector(0.f, 0.f, 200.f);
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
	if (!IsJunction(Node))
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

bool ATrafficManager::IsJunction(int32 Node) const
{
	// Two lanes out means a street simply carries on.
	return LanesFromNode.IsValidIndex(Node) && LanesFromNode[Node].Num() > 2;
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

bool ATrafficManager::HasRoomOnLane(int32 LaneIndex, float Room, const AActor* Asker) const
{
	if (!Lanes.IsValidIndex(LaneIndex))
	{
		return true;
	}
	const FVector Start = Lanes[LaneIndex].Points[0];
	for (const ATrafficCar* Other : Cars)
	{
		if (Other && Other != Asker && Other->GetLaneIndex() == LaneIndex
			&& FVector::Dist2D(Other->GetActorLocation(), Start) < Room)
		{
			return false;
		}
	}
	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	return !Player || FVector::Dist2D(Player->GetActorLocation(), Start) >= Room;
}

float ATrafficManager::ClearDistanceAhead(const AActor* Car, float LookAhead, bool bIgnoreAICars) const
{
	const FVector Origin = Car->GetActorLocation();
	const FVector Forward = Car->GetActorForwardVector().GetSafeNormal2D();
	const FVector Right = FVector(-Forward.Y, Forward.X, 0.f);

	float Clear = LookAhead;
	auto ConsiderPoint = [&](const FVector& Point)
	{
		const FVector Offset = Point - Origin;
		const float Ahead = static_cast<float>(FVector::DotProduct(Offset, Forward));
		const float Side = static_cast<float>(FMath::Abs(FVector::DotProduct(Offset, Right)));
		if (Ahead > 0.f && Ahead < Clear && Side < Traffic::CorridorHalfWidth)
		{
			Clear = Ahead;
		}
	};
	// Oncoming cars and trams in the other lane are no reason to stop. The
	// player always counts, whichever way they face.
	auto IsOncoming = [&](const AActor* Other)
	{
		return FVector::DotProduct(Other->GetActorForwardVector().GetSafeNormal2D(), Forward) < 0.f;
	};

	if (!bIgnoreAICars)
	{
		for (const ATrafficCar* Other : Cars)
		{
			if (Other && Other != Car && !IsOncoming(Other))
			{
				ConsiderPoint(Other->GetActorLocation());
			}
		}
	}
	for (const ATram* Other : Trams)
	{
		if (!Other || Other == Car || Other->IsHidden() || IsOncoming(Other))
		{
			continue;
		}
		// A tram is long: check its ends as well as its middle, so a tram
		// crossing in front is seen.
		const FVector Along = Other->GetActorForwardVector().GetSafeNormal2D() * 450.f;
		ConsiderPoint(Other->GetActorLocation());
		ConsiderPoint(Other->GetActorLocation() + Along);
		ConsiderPoint(Other->GetActorLocation() - Along);
	}
	if (const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0); Player && Player != Car)
	{
		ConsiderPoint(Player->GetActorLocation());
	}
	return Clear;
}

bool ATrafficManager::IsAreaClear(const FVector& Location, float Radius, const AActor* Asker) const
{
	auto IsNear = [&](const AActor* Other)
	{
		return Other && Other != Asker && !Other->IsHidden() && FVector::Dist2D(Other->GetActorLocation(), Location) < Radius;
	};
	for (const ATrafficCar* Other : Cars)
	{
		if (IsNear(Other))
		{
			return false;
		}
	}
	for (const ATram* Other : Trams)
	{
		if (IsNear(Other))
		{
			return false;
		}
	}
	return !IsNear(UGameplayStatics::GetPlayerPawn(this, 0));
}
