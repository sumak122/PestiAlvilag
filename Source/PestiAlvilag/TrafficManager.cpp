// Fill out your copyright notice in the Description page of Project Settings.

#include "TrafficManager.h"

#include "Algo/BinarySearch.h"
#include "Camera/PlayerCameraManager.h"
#include "CarjackedDriver.h"
#include "Dom/JsonObject.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "PestiAlvilagGameModeBase.h"
#include "PestiCar.h"
#include "PestiTaxi.h"
#include "PersonModel.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "TrafficCar.h"
#include "PoliceCar.h"
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

	// Police cars are dark blue.
	const FLinearColor PolicePaint(0.01f, 0.015f, 0.05f);
	const FLinearColor TaxiPaint(0.01f, 0.01f, 0.01f);

	// Half the width of the corridor a car watches for others (cm).
	constexpr float CorridorHalfWidth = 220.f;
	// Free track a tram needs around it to be placed at the start (cm).
	constexpr float TramSpawnRoom = 2000.f;
	// Height of the car's centre above the road surface (hover + half height).
	constexpr float RideHeight = 90.f;

	// Stealing a car: the player's car is set down a little above the AI
	// car's place and settles, the driver gets out this far to the side, and
	// a police car this close sees it (cm).
	constexpr float StolenLift = 60.f;
	constexpr double DriverOut = 170.0;
	constexpr double PoliceSeeTheft = 3000.0;
	// A car to take the stolen one's place appears this far from the camera,
	// out of its view (cm).
	constexpr int32 ReplaceTries = 8;
	constexpr double ReplaceNear = 6000.0;
	constexpr double ReplaceFar = 25000.0;
	constexpr double ReplaceOutOfViewDot = 0.17;
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

	const TArray<TSharedPtr<FJsonValue>>* Streets = nullptr;
	if (Root->TryGetArrayField(TEXT("centerlines"), Streets))
	{
		for (const TSharedPtr<FJsonValue>& StreetValue : *Streets)
		{
			const TSharedPtr<FJsonObject>& Street = StreetValue->AsObject();
			FString Name;
			if (Street.IsValid() && Street->TryGetStringField(TEXT("name"), Name) && !Name.IsEmpty())
			{
				StreetNames.Add(Street->GetStringField(TEXT("id")), Name);
			}
		}
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
	if (CarsToReplace + TaxisToReplace > 0)
	{
		ReplaceCars();
	}
}

APestiCar* ATrafficManager::StealCar(ATrafficCar* Car)
{
	if (!IsValid(Car) || Cast<APoliceCar>(Car))
	{
		return nullptr;
	}
	UWorld* World = GetWorld();
	const FVector Location = Car->GetActorLocation();
	const FRotator Facing(0.f, Car->GetActorRotation().Yaw, 0.f);
	const FLinearColor Paint = Car->GetPaint();
	const bool bTaxi = Cast<ATrafficTaxi>(Car) != nullptr;
	Car->LeaveTraffic();
	Cars.Remove(Car);
	Car->Destroy();

	// The same car, now one the player drives (it settles onto the ground itself).
	const FTransform Place(Facing, Location + FVector(0.f, 0.f, Traffic::StolenLift));
	// A taxi stays a taxi, so the player can work in it.
	APestiCar* Stolen = World->SpawnActorDeferred<APestiCar>(bTaxi ? APestiTaxi::StaticClass() : APestiCar::StaticClass(), Place, nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (!Stolen)
	{
		return nullptr;
	}
	Stolen->BodyColor = Paint;
	Stolen->FinishSpawning(Place);
	++(bTaxi ? TaxisToReplace : CarsToReplace);

	// The driver gets out on the side away from the player and runs off.
	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	const FVector Danger = Player ? Player->GetActorLocation() : Location;
	const FVector Right = FRotationMatrix(Facing).GetUnitAxis(EAxis::Y);
	const double Side = FVector::DotProduct(Danger - Location, Right) > 0.0 ? -1.0 : 1.0;
	FActorSpawnParameters DriverParams;
	DriverParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (ACarjackedDriver* Driver = World->SpawnActor<ACarjackedDriver>(ACarjackedDriver::StaticClass(),
		Location + Right * (Side * Traffic::DriverOut), Facing, DriverParams))
	{
		FRandomStream Random(FMath::Rand());
		PersonKit::FOutfit Clothes = PersonModel::RandomOutfit(Random);
		Clothes.Held = PersonKit::EHeld::None;
		Driver->SetOutfit(Clothes);
		Driver->RunFrom(Danger);
	}

	// A policeman close by sees it.
	for (const ATrafficCar* Other : Cars)
	{
		if (Cast<APoliceCar>(Other) && FVector::Dist2D(Other->GetActorLocation(), Location) < Traffic::PoliceSeeTheft)
		{
			if (APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this))
			{
				Game->ReportCrime(1);
			}
			break;
		}
	}
	return Stolen;
}

void ATrafficManager::ReplaceCars()
{
	const APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC || !PC->PlayerCameraManager || Lanes.IsEmpty())
	{
		return;
	}
	const FVector Camera = PC->PlayerCameraManager->GetCameraLocation();
	const FVector2D Looking = FVector2D(PC->PlayerCameraManager->GetCameraRotation().Vector()).GetSafeNormal();
	for (int32 Try = 0; Try < Traffic::ReplaceTries; ++Try)
	{
		const int32 LaneIdx = FMath::RandRange(0, Lanes.Num() - 1);
		const FTrafficLane& Lane = Lanes[LaneIdx];
		if (Lane.Points.Num() < 2)
		{
			continue;
		}
		const int32 PointIdx = FMath::RandRange(0, Lane.Points.Num() - 2);
		FVector Location = Lane.Points[PointIdx];
		const FVector2D Away = FVector2D(Location - Camera);
		const double Distance = Away.Size();
		// Close enough to matter, but where the camera is not looking.
		if (Distance < Traffic::ReplaceNear || Distance > Traffic::ReplaceFar
			|| FVector2D::DotProduct(Away / Distance, Looking) > Traffic::ReplaceOutOfViewDot
			|| !IsAreaClear(Location, MinSpawnSpacing, nullptr))
		{
			continue;
		}
		FHitResult Hit;
		if (GetWorld()->LineTraceSingleByChannel(Hit, Location + FVector(0.f, 0.f, 200.f), Location - FVector(0.f, 0.f, 1000.f), ECC_Visibility))
		{
			Location.Z = Hit.ImpactPoint.Z;
		}
		Location.Z += Traffic::RideHeight;
		const FRotator Facing = (Lane.Points[PointIdx + 1] - Lane.Points[PointIdx]).GetSafeNormal2D().Rotation();
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		const bool bTaxi = TaxisToReplace > 0;
		ATrafficCar* Car = GetWorld()->SpawnActor<ATrafficCar>(bTaxi ? ATrafficTaxi::StaticClass() : ATrafficCar::StaticClass(), Location, Facing, SpawnParams);
		if (!Car)
		{
			continue;
		}
		Car->StartOnLane(this, LaneIdx, PointIdx + 1, bTaxi ? Traffic::TaxiPaint
			: Traffic::Paints[FMath::RandRange(0, static_cast<int32>(UE_ARRAY_COUNT(Traffic::Paints)) - 1)]);
		Cars.Add(Car);
		--(bTaxi ? TaxisToReplace : CarsToReplace);
		return;
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
		// A chasing police car shows its way to the player in red.
		if (const APoliceCar* Police = Cast<APoliceCar>(Car); Police && Police->IsChasing())
		{
			FVector From = Police->GetActorLocation();
			for (const FVector& Point : Police->GetChasePath())
			{
				DrawDebugLine(World, From, Point + Lift, FColor::Red, false, -1.f, SDPG_Foreground, 8.f);
				From = Point + Lift;
			}
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

	StreetLinks.SetNum(MaxNode + 1);
	for (int32 i = 0; i < Lanes.Num(); ++i)
	{
		const FTrafficLane& Lane = Lanes[i];
		if (Lane.FromNode < 0 || Lane.ToNode < 0)
		{
			continue;
		}
		float Length = 0.f;
		for (int32 p = 1; p < Lane.Points.Num(); ++p)
		{
			Length += static_cast<float>(FVector::Dist2D(Lane.Points[p - 1], Lane.Points[p]));
		}
		StreetLinks[Lane.FromNode].Add({ Lane.ToNode, i, false, Length });
		StreetLinks[Lane.ToNode].Add({ Lane.FromNode, i, true, Length });
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
			// Lines share tracks: don't start a tram on top of another line's.
			if (!IsAreaClear(Location, Traffic::TramSpawnRoom, nullptr))
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
		const bool bPolice = Cars.Num() < NumPolice;
		const bool bTaxi = !bPolice && Cars.Num() < NumPolice + NumTaxis;
		UClass* CarClass = bPolice ? APoliceCar::StaticClass() : bTaxi ? ATrafficTaxi::StaticClass() : ATrafficCar::StaticClass();
		ATrafficCar* Car = World->SpawnActor<ATrafficCar>(CarClass, Location, Facing, SpawnParams);
		if (!Car)
		{
			continue;
		}
		const FLinearColor& Paint = bPolice ? Traffic::PolicePaint
			: bTaxi ? Traffic::TaxiPaint
			: Traffic::Paints[FMath::RandRange(0, static_cast<int32>(UE_ARRAY_COUNT(Traffic::Paints)) - 1)];
		Car->StartOnLane(this, LaneIdx, PointIdx + 1, Paint);
		Cars.Add(Car);
		Taken.Add(Location);
	}

	UE_LOG(LogTraffic, Log, TEXT("Spawned %d traffic cars"), Cars.Num());
}

int32 ATrafficManager::NearestNode(const FVector& Location) const
{
	int32 Best = INDEX_NONE;
	double BestDistance = TNumericLimits<double>::Max();
	for (int32 Node = 0; Node < StreetLinks.Num(); ++Node)
	{
		if (StreetLinks[Node].Num() == 0)
		{
			continue;
		}
		const double Distance = FVector::DistSquared2D(NodeLocations[Node], Location);
		if (Distance < BestDistance)
		{
			BestDistance = Distance;
			Best = Node;
		}
	}
	return Best;
}

bool ATrafficManager::FindPath(const FVector& From, const FVector& To, TArray<FVector>& OutPoints) const
{
	OutPoints.Reset();
	const int32 Start = NearestNode(From);
	const int32 Goal = NearestNode(To);
	if (Start == INDEX_NONE || Goal == INDEX_NONE)
	{
		return false;
	}

	// A* over street corners, measuring along the streets.
	const int32 NumNodes = StreetLinks.Num();
	TArray<float> Cost;
	Cost.Init(TNumericLimits<float>::Max(), NumNodes);
	TArray<int32> CameBy;
	CameBy.Init(INDEX_NONE, NumNodes);
	TArray<int32> CameFrom;
	CameFrom.Init(INDEX_NONE, NumNodes);
	using FOpen = TPair<float, int32>;
	auto Cheaper = [](const FOpen& A, const FOpen& B) { return A.Key < B.Key; };
	TArray<FOpen> Open;
	Cost[Start] = 0.f;
	Open.HeapPush(FOpen(0.f, Start), Cheaper);
	while (Open.Num() > 0)
	{
		FOpen Top;
		Open.HeapPop(Top, Cheaper);
		const int32 Node = Top.Value;
		if (Node == Goal)
		{
			break;
		}
		for (int32 l = 0; l < StreetLinks[Node].Num(); ++l)
		{
			const FStreetLink& Link = StreetLinks[Node][l];
			const float NewCost = Cost[Node] + Link.Length;
			if (NewCost < Cost[Link.Node])
			{
				Cost[Link.Node] = NewCost;
				CameFrom[Link.Node] = Node;
				CameBy[Link.Node] = l;
				const float Guess = static_cast<float>(FVector::Dist2D(NodeLocations[Link.Node], NodeLocations[Goal]));
				Open.HeapPush(FOpen(NewCost + Guess, Link.Node), Cheaper);
			}
		}
	}
	if (Start != Goal && CameFrom[Goal] == INDEX_NONE)
	{
		return false;
	}

	TArray<const FStreetLink*> Steps;
	for (int32 Node = Goal; Node != Start; Node = CameFrom[Node])
	{
		Steps.Add(&StreetLinks[CameFrom[Node]][CameBy[Node]]);
	}
	OutPoints.Add(NodeLocations[Start]);
	for (int32 s = Steps.Num() - 1; s >= 0; --s)
	{
		const FTrafficLane& Lane = Lanes[Steps[s]->Lane];
		const int32 Num = Lane.Points.Num();
		for (int32 p = 1; p < Num; ++p)
		{
			OutPoints.Add(Lane.Points[Steps[s]->bReverse ? Num - 1 - p : p]);
		}
	}
	return true;
}

bool ATrafficManager::FindLanePointNear(const FVector& Location, const FVector& Forward, int32& OutLane, int32& OutPoint) const
{
	double BestDistance = TNumericLimits<double>::Max();
	OutLane = INDEX_NONE;
	for (int32 l = 0; l < Lanes.Num(); ++l)
	{
		const TArray<FVector>& Points = Lanes[l].Points;
		for (int32 p = 1; p < Points.Num(); ++p)
		{
			const FVector Heading = (Points[p] - Points[p - 1]).GetSafeNormal2D();
			const double Distance = FVector::DistSquared2D(Points[p], Location);
			if (Distance < BestDistance && FVector::DotProduct(Heading, Forward) > 0.3)
			{
				BestDistance = Distance;
				OutLane = l;
				OutPoint = p;
			}
		}
	}
	return OutLane != INDEX_NONE;
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
	// Two trams that meet where lines join must not both wait for the other:
	// a tram only waits for trams whose middle is ahead of its own.
	const bool bAskerIsTram = Car->IsA<ATram>();
	for (const ATram* Other : Trams)
	{
		if (!Other || Other == Car || Other->IsHidden() || IsOncoming(Other))
		{
			continue;
		}
		if (bAskerIsTram && FVector::DotProduct(Other->GetActorLocation() - Origin, Forward) <= 0.f)
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
	// The player's car, parked while they are on foot.
	for (TActorIterator<APestiCar> It(GetWorld()); It; ++It)
	{
		if (*It != Car)
		{
			ConsiderPoint(It->GetActorLocation());
		}
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
	for (TActorIterator<APestiCar> It(GetWorld()); It; ++It)
	{
		if (IsNear(*It))
		{
			return false;
		}
	}
	return !IsNear(UGameplayStatics::GetPlayerPawn(this, 0));
}
