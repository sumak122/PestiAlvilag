// Fill out your copyright notice in the Description page of Project Settings.

#include "TaxiRides.h"

#include "Camera/PlayerCameraManager.h"
#include "Dom/JsonObject.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "PestiAlvilagGameModeBase.h"
#include "PestiPerson.h"
#include "PestiTaxi.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "TrafficCar.h"

DEFINE_LOG_CATEGORY_STATIC(LogTaxiRides, Log, All);

namespace TaxiRides
{
	// On-screen lines: the hint or the heading, and one per place under it.
	constexpr int32 HeadLine = 20;
	constexpr int32 FirstPlaceLine = 21;
	constexpr int32 SpeechLine = 6;
	constexpr int32 MaxPlaces = 9;

	/** The number key for the place at Index (0 is the 1 key). */
	FKey PlaceKey(int32 Index)
	{
		switch (Index)
		{
		case 0: return EKeys::One;
		case 1: return EKeys::Two;
		case 2: return EKeys::Three;
		case 3: return EKeys::Four;
		case 4: return EKeys::Five;
		case 5: return EKeys::Six;
		case 6: return EKeys::Seven;
		case 7: return EKeys::Eight;
		default: return EKeys::Nine;
		}
	}

	// Streets wind: the ride is this much longer than the straight line.
	constexpr double RoadFactor = 1.3;
	// No ride to a place this close (cm).
	constexpr double TooClose = 3000.0;
	// The fade to black, the dark while riding, and the fade back (s).
	constexpr float FadeOut = 0.6f;
	constexpr float RideDark = 1.2f;
	constexpr float FadeIn = 0.8f;
	constexpr double StandHeight = 100.0;
	// A taxi from the traffic waits this long at a time while you choose (s).
	constexpr float HoldFor = 0.5f;
}

ATaxiRides::ATaxiRides()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ATaxiRides::BeginPlay()
{
	Super::BeginPlay();
	LoadPlaces();
}

bool ATaxiRides::LoadPlaces()
{
	const FString Path = FPaths::Combine(FPaths::ProjectDir(), DataFile);
	FString Text;
	TSharedPtr<FJsonObject> Root;
	const TArray<TSharedPtr<FJsonValue>>* Rows = nullptr;
	if (!FFileHelper::LoadFileToString(Text, *Path)
		|| !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) || !Root.IsValid()
		|| !Root->TryGetArrayField(TEXT("taxi_destinations"), Rows))
	{
		UE_LOG(LogTaxiRides, Warning, TEXT("No taxi destinations in %s"), *Path);
		return false;
	}
	for (const TSharedPtr<FJsonValue>& Row : *Rows)
	{
		const TSharedPtr<FJsonObject>* Entry = nullptr;
		const TArray<TSharedPtr<FJsonValue>>* At = nullptr;
		if (Row->TryGetObject(Entry) && (*Entry)->TryGetArrayField(TEXT("at"), At) && At->Num() >= 3)
		{
			FPlace Place;
			Place.Name = (*Entry)->GetStringField(TEXT("name"));
			Place.At = FVector2D((*At)[0]->AsNumber(), (*At)[1]->AsNumber());
			Place.Yaw = (*At)[2]->AsNumber();
			Places.Add(Place);
		}
	}
	return Places.Num() > 0;
}

AActor* ATaxiRides::TaxiNearby(const APawn* Player) const
{
	AActor* Best = nullptr;
	double BestDistance = HailRange;
	auto Consider = [&](AActor* Taxi)
	{
		const double Distance = FVector::Dist2D(Taxi->GetActorLocation(), Player->GetActorLocation());
		if (Distance < BestDistance)
		{
			Best = Taxi;
			BestDistance = Distance;
		}
	};
	for (TActorIterator<APestiTaxi> It(GetWorld()); It; ++It)
	{
		if (!It->GetController())
		{
			Consider(*It);
		}
	}
	for (TActorIterator<ATrafficTaxi> It(GetWorld()); It; ++It)
	{
		Consider(*It);
	}
	return Best;
}

int32 ATaxiRides::FareTo(const FVector& From, const FPlace& Place) const
{
	const double Kilometres = FVector2D::Distance(FVector2D(From), Place.At) * TaxiRides::RoadFactor / 100000.0;
	int32 Fare = FMath::RoundToInt((FlagFall + PerKilometre * Kilometres) / 10.0) * 10;
	const APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this);
	if (Game && Game->GetTaxiJobs() >= RegularAfterJobs)
	{
		Fare = FMath::RoundToInt(Fare * 0.5f / 10.f) * 10;
	}
	return Fare;
}

void ATaxiRides::Say(const FString& Text, float Seconds) const
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(TaxiRides::SpeechLine, Seconds, FColor(255, 210, 60), Text);
	}
}

void ATaxiRides::ShowChoices(const FVector& From) const
{
	if (!GEngine)
	{
		return;
	}
	// Newer lines go on top, so the last place first and the heading last.
	const int32 Count = FMath::Min(Places.Num(), TaxiRides::MaxPlaces);
	for (int32 i = Count - 1; i >= 0; --i)
	{
		const bool bHere = FVector2D::Distance(FVector2D(From), Places[i].At) < TaxiRides::TooClose;
		GEngine->AddOnScreenDebugMessage(TaxiRides::FirstPlaceLine + i, 0.f, bHere ? FColor(120, 120, 120) : FColor(255, 210, 60),
			bHere ? FString::Printf(TEXT("   %d  %s   (you are here)"), i + 1, *Places[i].Name)
				: FString::Printf(TEXT("   %d  %s   %s"), i + 1, *Places[i].Name, *APestiAlvilagGameModeBase::FormatMoney(FareTo(From, Places[i]))));
	}
	const APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this);
	const bool bRegular = Game && Game->GetTaxiJobs() >= RegularAfterJobs;
	GEngine->AddOnScreenDebugMessage(TaxiRides::HeadLine, 0.f, FColor::White,
		FString::Printf(TEXT("\u201eHov\u00e1 lesz?\u201d  (Where to?)   Number key: go   T: never mind%s"),
			bRegular ? TEXT("   [regular: half fare]") : TEXT("")));
}

void ATaxiRides::SetChoosing(APlayerController* PC, bool bInChoosing)
{
	bChoosing = bInChoosing;
	PC->SetIgnoreMoveInput(bInChoosing);
}

void ATaxiRides::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC || Places.IsEmpty())
	{
		return;
	}
	APestiPerson* Player = Cast<APestiPerson>(PC->GetPawn());
	APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this);

	// The ride: dark, then there, then the screen comes back.
	if (Riding != INDEX_NONE)
	{
		const double Elapsed = GetWorld()->GetTimeSeconds() - RideStart;
		if (!bArrived && Elapsed > TaxiRides::FadeOut + TaxiRides::RideDark * 0.5)
		{
			bArrived = true;
			const FPlace& Place = Places[Riding];
			if (Player)
			{
				FHitResult Hit;
				FCollisionQueryParams Params(SCENE_QUERY_STAT(TaxiRideDrop), false, Player);
				double Z = Player->GetActorLocation().Z;
				if (GetWorld()->LineTraceSingleByChannel(Hit, FVector(Place.At, 20000.0), FVector(Place.At, -20000.0), ECC_Visibility, Params))
				{
					Z = Hit.ImpactPoint.Z;
				}
				Player->TeleportTo(FVector(Place.At, Z + TaxiRides::StandHeight), FRotator(0.0, Place.Yaw, 0.0));
				PC->SetControlRotation(FRotator(-10.0, Place.Yaw, 0.0));
			}
			if (Game)
			{
				Game->AddMoney(-RideFare);
			}
		}
		if (Elapsed > TaxiRides::FadeOut + TaxiRides::RideDark)
		{
			if (PC->PlayerCameraManager)
			{
				PC->PlayerCameraManager->StartCameraFade(1.f, 0.f, TaxiRides::FadeIn, FLinearColor::Black, false, false);
			}
			Say(FString::Printf(TEXT("\u201eMeg\u00e9rkezt\u00fcnk.\u201d  (Here we are.)   %s, fare %s"), *Places[Riding].Name,
				*APestiAlvilagGameModeBase::FormatMoney(RideFare)));
			Riding = INDEX_NONE;
			PC->SetIgnoreMoveInput(false);
		}
		return;
	}

	// Only on foot, with nothing in hand.
	if (!Player || Player->IsCarrying())
	{
		if (bChoosing)
		{
			SetChoosing(PC, false);
		}
		return;
	}
	AActor* Taxi = TaxiNearby(Player);
	if (!bChoosing)
	{
		if (!Taxi)
		{
			return;
		}
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(TaxiRides::HeadLine, 0.f, FColor(255, 210, 60), TEXT("T: ride in the taxi"));
		}
		if (PC->WasInputKeyJustPressed(EKeys::T))
		{
			if (Game && Game->GetWantedLevel() > 0)
			{
				Say(TEXT("\u201eNem viszem, a rend\u0151rs\u00e9g keresi!\u201d  (Not with the police after you!)"));
				return;
			}
			SetChoosing(PC, true);
		}
		return;
	}

	// Choosing where to go; a taxi from the traffic pulls up and waits.
	if (!Taxi || PC->WasInputKeyJustPressed(EKeys::T) || PC->WasInputKeyJustPressed(EKeys::Escape))
	{
		SetChoosing(PC, false);
		return;
	}
	if (ATrafficTaxi* Passing = Cast<ATrafficTaxi>(Taxi))
	{
		Passing->HoldStill(TaxiRides::HoldFor);
	}
	const FVector From = Player->GetActorLocation();
	ShowChoices(From);
	const int32 Count = FMath::Min(Places.Num(), TaxiRides::MaxPlaces);
	for (int32 i = 0; i < Count; ++i)
	{
		if (!PC->WasInputKeyJustPressed(TaxiRides::PlaceKey(i)))
		{
			continue;
		}
		if (FVector2D::Distance(FVector2D(From), Places[i].At) < TaxiRides::TooClose)
		{
			Say(TEXT("\u201eDe hiszen itt vagyunk!\u201d  (But we're already here!)"));
			break;
		}
		const int32 Fare = FareTo(From, Places[i]);
		if (!Game || Game->GetMoney() < Fare)
		{
			Say(FString::Printf(TEXT("\u201eP\u00e9nz n\u00e9lk\u00fcl nem megy.\u201d  (No money, no ride.)   The fare is %s"), *APestiAlvilagGameModeBase::FormatMoney(Fare)));
			break;
		}
		bChoosing = false;
		Riding = i;
		RideFare = Fare;
		RideStart = GetWorld()->GetTimeSeconds();
		bArrived = false;
		if (PC->PlayerCameraManager)
		{
			PC->PlayerCameraManager->StartCameraFade(0.f, 1.f, TaxiRides::FadeOut, FLinearColor::Black, false, true);
		}
		break;
	}
}
