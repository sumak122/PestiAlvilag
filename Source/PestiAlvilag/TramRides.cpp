// Fill out your copyright notice in the Description page of Project Settings.

#include "TramRides.h"

#include "Dom/JsonObject.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Camera/PlayerCameraManager.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "PestiAlvilagGameModeBase.h"
#include "PersonModel.h"
#include "PestiPerson.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "TrafficManager.h"
#include "Tram.h"
#include "WalkingPerson.h"

DEFINE_LOG_CATEGORY_STATIC(LogTramRides, Log, All);

namespace TramRides
{
	// On-screen lines: the hint to get on, the ride, and what people say.
	constexpr int32 HintLine = 30;
	constexpr int32 RideLine = 1;
	constexpr int32 SpeechLine = 6;
	// A stop place belongs to a stop this close to it (cm).
	constexpr double SamePlace = 500.0;
	// Without a stop place, step off this far to the side of the tram (cm).
	constexpr double SideStep = 300.0;
	// The doors stay open at least this long after someone gets on (s).
	constexpr float BoardingHold = 2.f;
	// People come and go at stops this close to the camera (cm)...
	constexpr double CrowdRange = 6000.0;
	// ...and start waiting once the tram is this far off along its route (cm).
	constexpr float WaitAhead = 20000.f;
	// Up to this many wait at a stop, and get off at one.
	constexpr int32 MaxWaiting = 2;
	constexpr int32 MaxGettingOff = 2;
	// Where they stand, along the stop and back from the kerb (cm).
	constexpr float WaitSpread = 200.f;
	constexpr float WaitBack = 60.f;
	// Walking to the steps and away (cm/s), and how far away they go (cm).
	constexpr float BoardSpeed = 150.f;
	constexpr float LeaveSpeed = 130.f;
	constexpr double LeaveDistance = 900.0;
	// The actor's location is a person's middle, this far above the ground (cm).
	constexpr double Middle = 90.0;
	const FColor Yellow(255, 210, 60);
}

ATramRides::ATramRides()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ATramRides::BeginPlay()
{
	Super::BeginPlay();
	Random.Initialize(FMath::Rand());
	LoadStops();
}

bool ATramRides::LoadStops()
{
	const FString Path = FPaths::Combine(FPaths::ProjectDir(), DataFile);
	FString Text;
	TSharedPtr<FJsonObject> Root;
	const TArray<TSharedPtr<FJsonValue>>* Rows = nullptr;
	if (!FFileHelper::LoadFileToString(Text, *Path)
		|| !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) || !Root.IsValid()
		|| !Root->TryGetArrayField(TEXT("tram_stops"), Rows))
	{
		UE_LOG(LogTramRides, Warning, TEXT("No tram stops in %s; rerun street_furniture.py"), *Path);
		return false;
	}
	for (const TSharedPtr<FJsonValue>& Row : *Rows)
	{
		const TSharedPtr<FJsonObject>* Entry = nullptr;
		const TArray<TSharedPtr<FJsonValue>>* At = nullptr;
		const TArray<TSharedPtr<FJsonValue>>* Wait = nullptr;
		if (Row->TryGetObject(Entry) && (*Entry)->TryGetArrayField(TEXT("at"), At) && At->Num() >= 3
			&& (*Entry)->TryGetArrayField(TEXT("wait"), Wait) && Wait->Num() >= 2)
		{
			FStopPlace Place;
			Place.Name = (*Entry)->GetStringField(TEXT("name"));
			Place.At = FVector2D((*At)[0]->AsNumber(), (*At)[1]->AsNumber());
			Place.Yaw = (*At)[2]->AsNumber();
			Place.Wait = FVector2D((*Wait)[0]->AsNumber(), (*Wait)[1]->AsNumber());
			Stops.Add(Place);
		}
	}
	return Stops.Num() > 0;
}

ATrafficManager* ATramRides::GetManager()
{
	if (!Manager.IsValid())
	{
		for (TActorIterator<ATrafficManager> It(GetWorld()); It; ++It)
		{
			Manager = *It;
			break;
		}
	}
	return Manager.Get();
}

FString ATramRides::StopName(const ATram* Tram, int32 Stop) const
{
	const FTramRoute& Route = Manager->GetTramRoute(Tram->GetRouteIndex());
	return Route.StopNames.IsValidIndex(Stop) && !Route.StopNames[Stop].IsEmpty() ? Route.StopNames[Stop] : FString(TEXT("?"));
}

FString ATramRides::LastStopName(const ATram* Tram) const
{
	return StopName(Tram, Manager->GetTramRoute(Tram->GetRouteIndex()).Stops.Num() - 1);
}

bool ATramRides::IsLastStop(const ATram* Tram) const
{
	return Tram->GetNextStop() >= Manager->GetTramRoute(Tram->GetRouteIndex()).Stops.Num() - 1;
}

ATram* ATramRides::TramToBoard(const APawn* Player) const
{
	ATram* Best = nullptr;
	double BestDistance = BoardRange;
	for (TActorIterator<ATram> It(GetWorld()); It; ++It)
	{
		if (!It->IsAtStop() || IsLastStop(*It))
		{
			continue;
		}
		const double Distance = FVector::Dist2D(It->GetActorLocation(), Player->GetActorLocation());
		if (Distance < BestDistance)
		{
			Best = *It;
			BestDistance = Distance;
		}
	}
	return Best;
}

const ATramRides::FStopPlace* ATramRides::PlaceFor(const ATram* Tram, int32 Stop) const
{
	const FTramRoute& Route = Manager->GetTramRoute(Tram->GetRouteIndex());
	if (!Route.Stops.IsValidIndex(Stop))
	{
		return nullptr;
	}
	const FVector2D StopAt(Route.LocationAt(Route.Stops[Stop]));
	const FStopPlace* Place = nullptr;
	double PlaceDistance = TramRides::SamePlace;
	for (const FStopPlace& Candidate : Stops)
	{
		const double Distance = FVector2D::Distance(Candidate.At, StopAt);
		if (Distance < PlaceDistance)
		{
			Place = &Candidate;
			PlaceDistance = Distance;
		}
	}
	return Place;
}

bool ATramRides::GetOff(APestiPerson* Player, ATram* Tram)
{
	// The pavement by the stop the tram stands at, facing the way it runs.
	const FStopPlace* Place = PlaceFor(Tram, Tram->GetNextStop());
	const double Yaw = Tram->GetActorRotation().Yaw;
	const FVector2D At = Place ? Place->Wait : FVector2D(Tram->GetActorLocation() - Tram->GetActorRightVector() * TramRides::SideStep);
	if (!Player->GetOffAt(FVector(At, Tram->GetActorLocation().Z), Yaw))
	{
		return false;
	}
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		PC->SetControlRotation(FRotator(-10.0, Yaw, 0.0));
	}
	Tram->ReserveSeat(INDEX_NONE);
	Riding.Reset();
	bWantOff = false;
	CalledStop = INDEX_NONE;
	return true;
}

AWalkingPerson* ATramRides::SpawnWalker(const FVector& At, double Yaw, const PersonKit::FOutfit& Outfit)
{
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AWalkingPerson* Walker = GetWorld()->SpawnActor<AWalkingPerson>(AWalkingPerson::StaticClass(), At, FRotator(0.0, Yaw, 0.0), Params);
	if (Walker)
	{
		PersonKit::FOutfit Clothes = Outfit;
		Clothes.bSeated = false;
		Walker->SetOutfit(Clothes);
	}
	return Walker;
}

void ATramRides::SendAway(FStopCrowd& Crowd, const FStopPlace* Place)
{
	for (const TWeakObjectPtr<AWalkingPerson>& Each : Crowd.Boarding)
	{
		if (AWalkingPerson* Walker = Each.Get())
		{
			const FVector2D From(Walker->GetActorLocation());
			const FVector2D Along = Place ? FVector2D(FMath::Cos(FMath::DegreesToRadians(Place->Yaw)), FMath::Sin(FMath::DegreesToRadians(Place->Yaw))) : FVector2D(1.0, 0.0);
			Walker->SetPath({ From + Along * (Random.FRand() < 0.5f ? 1.0 : -1.0) * TramRides::LeaveDistance }, TramRides::LeaveSpeed, true);
		}
	}
	Crowd.Boarding.Reset();
}

void ATramRides::UpdateStopCrowds()
{
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC || !PC->PlayerCameraManager)
	{
		return;
	}
	const FVector Camera = PC->PlayerCameraManager->GetCameraLocation();

	// Forget trams that are gone.
	for (auto It = Crowds.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid())
		{
			SendAway(It.Value(), nullptr);
			It.RemoveCurrent();
		}
	}

	for (TActorIterator<ATram> It(GetWorld()); It; ++It)
	{
		ATram* Tram = *It;
		if (!Tram->IsInService() || Tram->GetRouteIndex() == INDEX_NONE)
		{
			continue;
		}
		const int32 Stop = Tram->GetNextStop();
		FStopCrowd& Crowd = Crowds.FindOrAdd(TWeakObjectPtr<ATram>(Tram));
		const FStopPlace* Place = PlaceFor(Tram, Stop);
		if (Crowd.Stop != Stop)
		{
			// The tram has left the last stop: anyone still waiting there walks off.
			SendAway(Crowd, PlaceFor(Tram, Crowd.Stop));
			Crowd = FStopCrowd();
			Crowd.Stop = Stop;
		}
		if (!Place)
		{
			continue;
		}
		const FTramRoute& Route = Manager->GetTramRoute(Tram->GetRouteIndex());
		const bool bLast = IsLastStop(Tram);
		const FVector2D Along(FMath::Cos(FMath::DegreesToRadians(Place->Yaw)), FMath::Sin(FMath::DegreesToRadians(Place->Yaw)));
		const FVector2D ToTrack = (Place->At - Place->Wait).GetSafeNormal();
		const double Z = Tram->GetActorLocation().Z;

		// A few people wait at the stop for the tram coming.
		if (!Crowd.bWaitingPlaced && !Tram->IsAtStop() && !bLast
			&& Route.Stops[Stop] - Tram->GetDistance() < TramRides::WaitAhead
			&& FVector2D::Distance(FVector2D(Camera), Place->Wait) < TramRides::CrowdRange)
		{
			Crowd.bWaitingPlaced = true;
			const int32 Count = Random.RandRange(0, TramRides::MaxWaiting);
			for (int32 i = 0; i < Count; ++i)
			{
				const FVector2D At = Place->Wait + Along * Random.FRandRange(-TramRides::WaitSpread, TramRides::WaitSpread) - ToTrack * Random.FRandRange(0.f, TramRides::WaitBack);
				const double Facing = FMath::RadiansToDegrees(FMath::Atan2(ToTrack.Y, ToTrack.X));
				if (AWalkingPerson* Walker = SpawnWalker(FVector(At, Z), Facing, PersonModel::RandomOutfit(Random)))
				{
					Walker->StandFacing(Facing);
					Crowd.Boarding.Add(Walker);
				}
			}
		}

		if (!Tram->IsAtStop())
		{
			continue;
		}
		const FVector Door = Tram->GetDoorNear(FVector(Place->Wait, Z));

		// The doors open: some get off and walk away, the waiting ones go to the steps.
		if (!Crowd.bExchanged)
		{
			Crowd.bExchanged = true;
			if (FVector::Dist2D(Camera, Tram->GetActorLocation()) < TramRides::CrowdRange)
			{
				const int32 Off = bLast ? Tram->NumPassengers() : Random.RandRange(0, FMath::Min(TramRides::MaxGettingOff, Tram->NumPassengers()));
				for (int32 i = 0; i < Off; ++i)
				{
					PersonKit::FOutfit Outfit;
					if (!Tram->TakeOffPassenger(Outfit))
					{
						break;
					}
					const FVector2D Kerb = Place->Wait + Along * Random.FRandRange(-TramRides::WaitSpread, TramRides::WaitSpread);
					const FVector2D Away = Kerb + Along * ((Random.FRand() < 0.5f ? 1.0 : -1.0) * TramRides::LeaveDistance);
					const FVector2D Out = Kerb - FVector2D(Door);
					if (AWalkingPerson* Walker = SpawnWalker(Door + FVector(0.0, 0.0, TramRides::Middle), FMath::RadiansToDegrees(FMath::Atan2(Out.Y, Out.X)), Outfit))
					{
						Walker->SetPath({ Kerb, Away }, TramRides::LeaveSpeed, true);
					}
				}
			}
			for (const TWeakObjectPtr<AWalkingPerson>& Each : Crowd.Boarding)
			{
				if (AWalkingPerson* Walker = Each.Get())
				{
					Walker->SetPath({ FVector2D(Door) }, TramRides::BoardSpeed, false);
				}
			}
		}

		// Those who reach the steps get on and sit down; the tram waits for the rest.
		for (int32 i = Crowd.Boarding.Num() - 1; i >= 0; --i)
		{
			AWalkingPerson* Walker = Crowd.Boarding[i].Get();
			if (!Walker)
			{
				Crowd.Boarding.RemoveAt(i);
				continue;
			}
			Walker->SetGoal(FVector2D(Door));
			if (Walker->HasArrived())
			{
				Tram->SeatPassenger(Walker->GetOutfit());
				Walker->Destroy();
				Crowd.Boarding.RemoveAt(i);
			}
		}
		if (Crowd.Boarding.Num() > 0)
		{
			Tram->HoldAtStop(1.f);
		}
	}
}

void ATramRides::Say(const FString& Text, float Seconds) const
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(TramRides::SpeechLine, Seconds, TramRides::Yellow, Text);
	}
}

void ATramRides::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC || !GetManager())
	{
		return;
	}
	UpdateStopCrowds();
	APestiPerson* Player = Cast<APestiPerson>(PC->GetPawn());
	if (!Player)
	{
		return;
	}
	const bool bT = PC->WasInputKeyJustPressed(EKeys::T);

	// On board.
	if (Player->IsRiding())
	{
		ATram* Tram = Riding.Get();
		if (!Tram || !Tram->IsInService())
		{
			// Should not happen (everybody gets off at the last stop), but never ride off the map.
			const FVector At = Player->GetActorLocation();
			Player->GetOffAt(FVector(At.X, At.Y, At.Z), Player->GetActorRotation().Yaw);
			Riding.Reset();
			return;
		}
		const int32 Next = Tram->GetNextStop();
		const FString Line = Manager->GetTramRoute(Tram->GetRouteIndex()).Name;
		if (Tram->IsAtStop())
		{
			if (CalledStop != Next)
			{
				CalledStop = Next;
				Say(FString::Printf(TEXT("\u201e%s!\u201d"), *StopName(Tram, Next)), 4.f);
			}
			if (IsLastStop(Tram))
			{
				if (GetOff(Player, Tram))
				{
					Say(TEXT("\u201eV\u00e9g\u00e1llom\u00e1s, k\u00e9rem lesz\u00e1llni!\u201d  (Last stop, everybody off!)"));
				}
				else
				{
					Tram->HoldAtStop(1.f);
				}
				return;
			}
			if (bWantOff || bT)
			{
				if (!GetOff(Player, Tram))
				{
					Tram->HoldAtStop(1.f);
				}
				return;
			}
		}
		else if (bT)
		{
			bWantOff = !bWantOff;
			if (bWantOff)
			{
				Say(TEXT("*csiling-csiling*  (You ring the bell: off at the next stop.)"), 3.f);
			}
		}
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(TramRides::RideLine, 0.f, FColor::White,
				FString::Printf(TEXT("On the tram (%s)   Next stop: %s   %s   C: change view"), *Line,
					*StopName(Tram, Tram->IsAtStop() ? Next + 1 : Next),
					Tram->IsAtStop() ? TEXT("T: get off here") : bWantOff ? TEXT("Getting off there (T: stay on)") : TEXT("T: ring the bell to get off there")));
		}
		return;
	}

	// On foot, beside a tram at a stop.
	if (Player->IsCarrying() || Player->IsHidden())
	{
		return;
	}
	ATram* Tram = TramToBoard(Player);
	if (!Tram)
	{
		return;
	}
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(TramRides::HintLine, 0.f, TramRides::Yellow,
			FString::Printf(TEXT("T: get on the tram towards %s   (fare %s)"), *LastStopName(Tram), *APestiAlvilagGameModeBase::FormatMoney(Fare)));
	}
	if (!bT)
	{
		return;
	}
	APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this);
	if (Game && Game->GetWantedLevel() > 0)
	{
		Say(TEXT("\u201eMaga nem sz\u00e1ll fel, keresi a rend\u0151rs\u00e9g!\u201d  (The conductor won't let you on with the police after you!)"));
		return;
	}
	if (!Game || Game->GetMoney() < Fare)
	{
		Say(FString::Printf(TEXT("\u201eJegy n\u00e9lk\u00fcl nem lehet.\u201d  (No ticket, no ride.)   The fare is %s"), *APestiAlvilagGameModeBase::FormatMoney(Fare)));
		return;
	}
	const int32 SeatIndex = Tram->GetFreeSeat();
	if (SeatIndex == INDEX_NONE)
	{
		Say(TEXT("\u201eTele vagyunk!\u201d  (The tram is full.)"));
		return;
	}
	Game->AddMoney(-Fare);
	const TramModel::FSeat Place = TramModel::Seat(SeatIndex, PersonModel::SeatedMiddle);
	Player->RideOn(Tram, Place.At, Place.Yaw);
	Tram->ReserveSeat(SeatIndex);
	Riding = Tram;
	bWantOff = false;
	CalledStop = Tram->GetNextStop();
	Tram->HoldAtStop(TramRides::BoardingHold);
	Say(FString::Printf(TEXT("\u201eJegyeket k\u00e9rem!\u201d  (Tickets, please!)   You pay %s"), *APestiAlvilagGameModeBase::FormatMoney(Fare)));
}
