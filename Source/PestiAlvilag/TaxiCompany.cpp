// Fill out your copyright notice in the Description page of Project Settings.

#include "TaxiCompany.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PestiAlvilagGameModeBase.h"
#include "PestiPerson.h"
#include "PestiText.h"
#include "PestiTaxi.h"
#include "TaxiPassenger.h"
#include "TrafficManager.h"
#include "UObject/ConstructorHelpers.h"

namespace TaxiCompany
{
	// On-screen message slots (the car and person use 1, the police 2 and 3).
	constexpr int32 StatusLine = 4;
	constexpr int32 SpeechLine = 6;

	// Fares stand this far to the left of their lane, on the pavement (cm).
	constexpr float KerbOffsets[] = { 350.f, 220.f };
	constexpr float PersonHalfHeight = 92.f;
	constexpr double FirstFareDelay = 2.0;
	constexpr double NextFareDelay = 5.0;
	constexpr double RouteRefresh = 0.5;
	// A quick ride earns this share of the meter as a tip, less a share per bump.
	constexpr float BestTip = 0.25f;
	constexpr float TipLostPerBump = 0.08f;

	// The kiosk: a green wooden booth with a roof and a sign (cm, X out the front).
	struct FKioskPart
	{
		const TCHAR* Name;
		FVector Location;
		FVector Scale;
		FLinearColor Color;
	};
	const FLinearColor Wood(0.04f, 0.10f, 0.05f);
	const FLinearColor Roof(0.08f, 0.08f, 0.09f);
	const FLinearColor Glass(0.03f, 0.04f, 0.05f);
	const FLinearColor Yellow(0.85f, 0.62f, 0.05f);
	const FKioskPart KioskSpecs[] = {
		{ TEXT("Booth"), FVector(0.f, 0.f, 130.f), FVector(2.2f, 2.6f, 2.6f), Wood },
		{ TEXT("Window"), FVector(111.f, 0.f, 160.f), FVector(0.02f, 1.8f, 0.9f), Glass },
		{ TEXT("Roof"), FVector(0.f, 0.f, 270.f), FVector(2.8f, 3.2f, 0.2f), Roof },
		{ TEXT("SignBoard"), FVector(0.f, 0.f, 320.f), FVector(0.1f, 3.0f, 0.8f), Yellow },
		{ TEXT("PolePost"), FVector(250.f, 220.f, 175.f), FVector(0.08f, 0.08f, 3.5f), Roof },
		{ TEXT("PoleSign"), FVector(250.f, 220.f, 330.f), FVector(0.1f, 0.9f, 0.4f), Yellow },
	};

	// Streets the passengers name, as they were called in 1932. Today's name
	// first; streets renamed since are given their old name, and streets
	// not on this list (newer, or renamed in a way not yet checked) are
	// never chosen.
	const TCHAR* const Renamed[][2] = {
		{ TEXT("Dobozi utca"), TEXT("Karpfenstein utca") },
		{ TEXT("II. J\u00e1nos P\u00e1l p\u00e1pa t\u00e9r"), TEXT("Tisza K\u00e1lm\u00e1n t\u00e9r") },
		{ TEXT("Teleki L\u00e1szl\u00f3 t\u00e9r"), TEXT("Teleki t\u00e9r") },
	};
	const TCHAR* const PeriodStreets[] = {
		TEXT("R\u00e1k\u00f3czi \u00fat"), TEXT("N\u00e9psz\u00ednh\u00e1z utca"), TEXT("J\u00f3zsef k\u00f6r\u00fat"),
		TEXT("Erzs\u00e9bet k\u00f6r\u00fat"), TEXT("Baross t\u00e9r"), TEXT("Kerepesi \u00fat"), TEXT("Fiumei \u00fat"),
		TEXT("Bezer\u00e9dj utca"), TEXT("Csokonai utca"), TEXT("Dob utca"), TEXT("Doh\u00e1ny utca"),
		TEXT("Wessel\u00e9nyi utca"), TEXT("Ak\u00e1cfa utca"), TEXT("Kert\u00e9sz utca"), TEXT("Ny\u00e1r utca"),
		TEXT("Klauz\u00e1l utca"), TEXT("Rottenbiller utca"), TEXT("Istv\u00e1n utca"), TEXT("Garay utca"),
		TEXT("Sz\u00f6vets\u00e9g utca"), TEXT("Nefelejcs utca"), TEXT("Th\u00f6k\u00f6ly \u00fat"), TEXT("M\u00e1ty\u00e1s t\u00e9r"),
		TEXT("Kun utca"), TEXT("Szerdahelyi utca"), TEXT("B\u00e9rkocsis utca"), TEXT("Luther utca"), TEXT("Vas utca"),
		TEXT("Szentkir\u00e1lyi utca"), TEXT("M\u00fazeum utca"), TEXT("K\u0151farag\u00f3 utca"), TEXT("R\u00f6kk Szil\u00e1rd utca"),
		TEXT("K\u00f6lcsey utca"), TEXT("Hor\u00e1nszky utca"), TEXT("Husz\u00e1r utca"), TEXT("Als\u00f3 erd\u0151sor"),
		TEXT("Csengery utca"), TEXT("Lujza utca"), TEXT("Magdolna utca"), TEXT("N\u00e9met utca"),
		TEXT("Nagy Fuvaros utca"), TEXT("Kis Fuvaros utca"), TEXT("Koszor\u00fa utca"), TEXT("Tavaszmez\u0151 utca"),
		TEXT("Sal\u00e9trom utca"), TEXT("Kis Sal\u00e9trom utca"), TEXT("Sz\u0171z utca"), TEXT("Rig\u00f3 utca"),
		TEXT("Dologh\u00e1z utca"), TEXT("Fecske utca"), TEXT("V\u00edg utca"), TEXT("Bethlen G\u00e1bor utca"),
		TEXT("Mur\u00e1nyi utca"), TEXT("R\u00f3zsa utca"), TEXT("Sz\u00e1zh\u00e1z utca"), TEXT("Gyulai P\u00e1l utca"),
		TEXT("Karpfenstein utca"), TEXT("Tisza K\u00e1lm\u00e1n t\u00e9r"), TEXT("Teleki t\u00e9r"),
	};

	FString PeriodName(const FString* TodaysName)
	{
		if (!TodaysName)
		{
			return FString();
		}
		FString Name = *TodaysName;
		for (const auto& Pair : Renamed)
		{
			if (Name == Pair[0])
			{
				Name = Pair[1];
			}
		}
		for (const TCHAR* Street : PeriodStreets)
		{
			if (Name == Street)
			{
				return Name;
			}
		}
		return FString();
	}
}

ATaxiCompany::ATaxiCompany()
{
	PrimaryActorTick.bCanEverTick = true;

	BaseMaterial = CarBody::LoadPaintMaterial();

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	for (const TaxiCompany::FKioskPart& Spec : TaxiCompany::KioskSpecs)
	{
		UStaticMeshComponent* Part = CreateDefaultSubobject<UStaticMeshComponent>(Spec.Name);
		Part->SetupAttachment(Root);
		Part->SetStaticMesh(CubeMesh.Object);
		Part->SetRelativeLocation(Spec.Location);
		Part->SetRelativeScale3D(Spec.Scale);
		Part->SetCollisionProfileName(TEXT("BlockAll"));
		KioskParts.Add(Part);
	}

	// The company's name on both faces of the roof sign.
	auto MakeSign = [this, Root](const TCHAR* Name, double Side, double Yaw)
	{
		UTextRenderComponent* Sign = CreateDefaultSubobject<UTextRenderComponent>(Name);
		Sign->SetupAttachment(Root);
		Sign->SetRelativeLocationAndRotation(FVector(Side * 6.0, 0.0, 320.0), FRotator(0.0, Yaw, 0.0));
		Sign->SetHorizontalAlignment(EHTA_Center);
		Sign->SetVerticalAlignment(EVRTA_TextCenter);
		Sign->SetWorldSize(42.f);
		Sign->SetTextRenderColor(FColor(20, 20, 20));
		Sign->SetText(FText::FromString(TEXT("AUT\u00d3TAXI")));
		return Sign;
	};
	SignFront = MakeSign(TEXT("SignFront"), 1.0, 0.0);
	SignBack = MakeSign(TEXT("SignBack"), -1.0, 180.0);

	// The destination light; it lives in the world, not on the kiosk.
	Beacon = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Beacon"));
	Beacon->SetupAttachment(Root);
	Beacon->SetUsingAbsoluteLocation(true);
	Beacon->SetUsingAbsoluteRotation(true);
	Beacon->SetUsingAbsoluteScale(true);
	Beacon->SetStaticMesh(CylinderMesh.Object);
	Beacon->SetWorldScale3D(FVector(0.6f, 0.6f, 30.f));
	Beacon->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Beacon->SetCastShadow(false);
	Beacon->SetVisibility(false);
}

void ATaxiCompany::BeginPlay()
{
	Super::BeginPlay();

	if (!BaseMaterial)
	{
		return;
	}
	for (int32 i = 0; i < KioskParts.Num() && i < static_cast<int32>(UE_ARRAY_COUNT(TaxiCompany::KioskSpecs)); ++i)
	{
		UMaterialInstanceDynamic* Paintwork = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		Paintwork->SetVectorParameterValue(TEXT("Color"), TaxiCompany::KioskSpecs[i].Color);
		KioskParts[i]->SetMaterial(0, Paintwork);
	}
	UMaterialInstanceDynamic* Light = UMaterialInstanceDynamic::Create(BaseMaterial, this);
	Light->SetVectorParameterValue(TEXT("Color"), FLinearColor(1.f, 0.75f, 0.1f));
	Beacon->SetMaterial(0, Light);
}

ATrafficManager* ATaxiCompany::GetTraffic() const
{
	TActorIterator<ATrafficManager> It(GetWorld());
	return It ? *It : nullptr;
}

void ATaxiCompany::Say(const FString& Line) const
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(TaxiCompany::SpeechLine, 6.f, FColor(255, 220, 120), Line);
	}
}

void ATaxiCompany::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	APawn* Player = PC ? PC->GetPawn() : nullptr;
	if (!Player)
	{
		return;
	}

	// Starting and ending a shift at the kiosk, on foot.
	const bool bAtKiosk = Player->IsA<APestiPerson>() && FVector::Dist2D(Player->GetActorLocation(), GetActorLocation()) < SignOnRange;
	if (bAtKiosk && PC->WasInputKeyJustPressed(EKeys::E))
	{
		ToggleShift();
	}

	if (Shift == ETaxiShift::Off)
	{
		if (bAtKiosk && GEngine)
		{
			GEngine->AddOnScreenDebugMessage(TaxiCompany::StatusLine, 0.f, FColor(255, 200, 60),
				TEXT("Budapesti Aut\u00f3taxi   E: start a taxi shift"));
		}
		return;
	}

	APestiTaxi* Taxi = Cast<APestiTaxi>(Player);
	if (Taxi)
	{
		UpdateFare(Taxi, DeltaSeconds);
	}
	else
	{
		// Leaving the taxi with a fare aboard loses them.
		if (Shift == ETaxiShift::ToDropoff)
		{
			Say(TEXT("\u201eHova megy? Na j\u00f3, majd gyalog.\u201d  (Where are you going? Fine, I'll walk.)"));
			EndFare();
		}
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(TaxiCompany::StatusLine, 0.f, FColor(255, 200, 60), bAtKiosk
				? FString(TEXT("Taxi shift   E: end the shift"))
				: FString(TEXT("Taxi shift: get into one of the company's black and yellow taxis")));
		}
	}
}

void ATaxiCompany::ToggleShift()
{
	if (Shift == ETaxiShift::Off)
	{
		Shift = ETaxiShift::Waiting;
		Fares = 0;
		NextFareTime = GetWorld()->GetTimeSeconds() + TaxiCompany::FirstFareDelay;
		Say(TEXT("\u201eIndulhat, a kocsik ott \u00e1llnak.\u201d  (Off you go, the cars are over there.)"));
	}
	else
	{
		EndFare();
		Shift = ETaxiShift::Off;
		Say(FString::Printf(TEXT("Shift over: %d fares."), Fares));
	}
}

void ATaxiCompany::UpdateFare(APestiTaxi* Taxi, float DeltaSeconds)
{
	const double Now = GetWorld()->GetTimeSeconds();
	const FVector At = Taxi->GetActorLocation();
	const bool bStopped = Taxi->GetVelocity().Size2D() < StopSpeed;
	FString Status;

	switch (Shift)
	{
	case ETaxiShift::Waiting:
		Status = TEXT("Taxi shift: waiting for a fare...");
		if (Now >= NextFareTime && !StartFare(At))
		{
			NextFareTime = Now + 1.0;
		}
		break;

	case ETaxiShift::ToPickup:
	{
		const double Distance = FVector::Dist2D(At, PickupSpot);
		if (Distance < PickupRange && bStopped)
		{
			PickUp(Taxi);
			break;
		}
		if (Now - FareTime > PatienceSeconds)
		{
			Say(TEXT("The fare got tired of waiting and walked off."));
			EndFare();
			break;
		}
		UpdateRoute(At, PickupSpot);
		Status = FString::Printf(TEXT("Fare waiting on %s: %s %s  (stop beside them)"),
			*PickupStreet, *PestiText::Metres(Distance), PestiText::Compass(At, PickupSpot));
		break;
	}

	case ETaxiShift::ToDropoff:
	{
		APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this);
		if (Game && Game->GetWantedLevel() > 0 && !bPassengerScared)
		{
			bPassengerScared = true;
			Say(TEXT("\u201eEngedjen ki! Azonnal!\u201d  (Let me out! Right now!)"));
		}
		if (bPassengerScared && bStopped)
		{
			Say(TEXT("The passenger jumped out without paying."));
			DropOff(Taxi, false);
			break;
		}
		const int32 Bumps = Taxi->GetBumpCount() - BumpsAtPickup;
		if (Bumps > BumpsSaid)
		{
			BumpsSaid = Bumps;
			Say(TEXT("\u201eVigy\u00e1zzon, k\u00e9rem!\u201d  (Careful, please!)"));
		}
		const double Distance = FVector::Dist2D(At, DropoffRoad);
		if (Distance < DropoffRange && bStopped)
		{
			DropOff(Taxi, true);
			break;
		}
		UpdateRoute(At, DropoffRoad);
		const double TimeLeft = RideLength / ExpectedSpeed - (Now - FareTime);
		Status = FString::Printf(TEXT("To %s: %s %s   %s"), *DropoffStreet, *PestiText::Metres(Distance),
			PestiText::Compass(At, DropoffRoad),
			TimeLeft > 0.0 ? *FString::Printf(TEXT("tip if there in %d s"), FMath::CeilToInt(TimeLeft)) : TEXT("too slow for a tip"));
		break;
	}

	default:
		break;
	}

	if (Shift == ETaxiShift::ToPickup || Shift == ETaxiShift::ToDropoff)
	{
		DrawRoute();
	}
	if (GEngine && !Status.IsEmpty())
	{
		GEngine->AddOnScreenDebugMessage(TaxiCompany::StatusLine, 0.f, FColor(255, 200, 60), Status);
	}
}

bool ATaxiCompany::StartFare(const FVector& From)
{
	FVector Road;
	if (!PickStreetSpot(From, MinPickupDistance, MaxPickupDistance, PickupSpot, Road, PickupStreet))
	{
		return false;
	}

	// They stand facing the street.
	const FRotator Facing = (Road - PickupSpot).GetSafeNormal2D().Rotation();
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Passenger = GetWorld()->SpawnActor<ATaxiPassenger>(ATaxiPassenger::StaticClass(), PickupSpot, FRotator(0.0, Facing.Yaw, 0.0), Params);
	if (!Passenger)
	{
		return false;
	}
	Shift = ETaxiShift::ToPickup;
	FareTime = GetWorld()->GetTimeSeconds();
	NextRouteTime = 0.0;
	Say(FString::Printf(TEXT("Someone is waving for a taxi on %s."), *PickupStreet));
	return true;
}

void ATaxiCompany::PickUp(APestiTaxi* Taxi)
{
	FString Street;
	if (!PickStreetSpot(PickupSpot, MinRideDistance, MaxRideDistance, DropoffSpot, DropoffRoad, Street))
	{
		return;
	}
	DropoffStreet = Street;

	// The meter runs along the streets, not as the crow flies.
	TArray<FVector> Way;
	RideLength = FVector::Dist2D(PickupSpot, DropoffRoad) * 1.3f;
	if (ATrafficManager* Traffic = GetTraffic())
	{
		if (Traffic->FindPath(PickupSpot, DropoffRoad, Way) && Way.Num() > 1)
		{
			RideLength = 0.f;
			for (int32 i = 1; i < Way.Num(); ++i)
			{
				RideLength += static_cast<float>(FVector::Dist2D(Way[i - 1], Way[i]));
			}
		}
	}

	if (Passenger)
	{
		Passenger->SetInWorld(false);
	}
	Shift = ETaxiShift::ToDropoff;
	FareTime = GetWorld()->GetTimeSeconds();
	BumpsAtPickup = Taxi->GetBumpCount();
	BumpsSaid = 0;
	bPassengerScared = false;
	NextRouteTime = 0.0;
	Beacon->SetWorldLocation(DropoffSpot + FVector(0.0, 0.0, 1500.0));
	Beacon->SetVisibility(true);
	Say(FString::Printf(TEXT("\u201e%s, legyen sz\u00edves!\u201d  (%s, please!)"), *DropoffStreet, *DropoffStreet));
}

void ATaxiCompany::DropOff(APestiTaxi* Taxi, bool bPaid)
{
	if (Passenger)
	{
		// Out on the kerb side.
		const FVector Left = -Taxi->GetActorRightVector().GetSafeNormal2D();
		Passenger->SetActorLocation(Taxi->GetActorLocation() + Left * 200.0 + FVector(0.0, 0.0, 10.0));
		Passenger->SetInWorld(true);
		Passenger->SetWaiting(false);
		Passenger->SetLifeSpan(8.f);
		Passenger = nullptr;
	}

	if (bPaid)
	{
		const float Kilometres = RideLength / 100000.f;
		const int32 Meter = FMath::RoundToInt((FlagFall + PerKilometre * Kilometres) / 10.f) * 10;
		const double Took = GetWorld()->GetTimeSeconds() - FareTime;
		const int32 Bumps = Taxi->GetBumpCount() - BumpsAtPickup;
		const bool bQuick = Took <= RideLength / ExpectedSpeed;
		const float TipShare = bQuick ? FMath::Max(0.f, TaxiCompany::BestTip - TaxiCompany::TipLostPerBump * Bumps) : 0.f;
		const int32 Tip = FMath::RoundToInt(Meter * TipShare / 10.f) * 10;
		const int32 Earned = FMath::RoundToInt(Meter * DriverShare) + Tip;
		++Fares;

		if (APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this))
		{
			Game->AddMoney(Earned);
		}
		const FString Thanks = Tip > 0
			? FString(TEXT("\u201eK\u00f6sz\u00f6n\u00f6m, a t\u00f6bbi a mag\u00e1\u00e9.\u201d  (Thank you, keep the change.)"))
			: bQuick ? FString(TEXT("\u201eLegk\u00f6zelebb finomabban!\u201d  (Gentler next time!)"))
			: FString(TEXT("\u201eH\u00e1t, ez eltartott egy darabig.\u201d  (Well, that took a while.)"));
		Say(FString::Printf(TEXT("%s   Meter %s, your share %s, tip %s"), *Thanks,
			*APestiAlvilagGameModeBase::FormatMoney(Meter),
			*APestiAlvilagGameModeBase::FormatMoney(FMath::RoundToInt(Meter * DriverShare)),
			*APestiAlvilagGameModeBase::FormatMoney(Tip)));
	}
	EndFare();
}

void ATaxiCompany::EndFare()
{
	if (Passenger)
	{
		Passenger->Destroy();
		Passenger = nullptr;
	}
	Beacon->SetVisibility(false);
	Route.Reset();
	if (Shift != ETaxiShift::Off)
	{
		Shift = ETaxiShift::Waiting;
		NextFareTime = GetWorld()->GetTimeSeconds() + TaxiCompany::NextFareDelay;
	}
}

bool ATaxiCompany::PickStreetSpot(const FVector& From, float MinDistance, float MaxDistance, FVector& OutKerb, FVector& OutRoad, FString& OutStreet) const
{
	const ATrafficManager* Traffic = GetTraffic();
	if (!Traffic || Traffic->NumLanes() == 0)
	{
		return false;
	}

	FCollisionQueryParams Params(SCENE_QUERY_STAT(TaxiSpot), false, this);
	const FCollisionShape Body = FCollisionShape::MakeCapsule(40.f, TaxiCompany::PersonHalfHeight);
	for (int32 Try = 0; Try < 200; ++Try)
	{
		const int32 LaneIndex = FMath::RandRange(0, Traffic->NumLanes() - 1);
		const FString Street = TaxiCompany::PeriodName(Traffic->FindStreetName(LaneIndex));
		const TArray<FVector>& Points = Traffic->GetLane(LaneIndex).Points;
		if (Street.IsEmpty() || Points.Num() < 2)
		{
			continue;
		}
		const int32 p = FMath::RandRange(1, Points.Num() - 1);
		const FVector Road = Points[p];
		const double Distance = FVector::Dist2D(Road, From);
		if (Distance < MinDistance || Distance > MaxDistance)
		{
			continue;
		}

		// The kerb is on the left when keeping left.
		const FVector Ahead = (Points[p] - Points[p - 1]).GetSafeNormal2D();
		const FVector Left(Ahead.Y, -Ahead.X, 0.0);
		for (const float Offset : TaxiCompany::KerbOffsets)
		{
			const FVector Spot = Road + Left * Offset;
			FHitResult Ground;
			if (!GetWorld()->LineTraceSingleByChannel(Ground, Spot + FVector(0.0, 0.0, 5000.0), Spot - FVector(0.0, 0.0, 5000.0), ECC_Visibility, Params))
			{
				continue;
			}
			const FVector Standing = Ground.ImpactPoint + FVector(0.0, 0.0, TaxiCompany::PersonHalfHeight + 5.0);
			if (GetWorld()->OverlapAnyTestByChannel(Standing, FQuat::Identity, ECC_WorldStatic, Body, Params))
			{
				continue;
			}
			OutKerb = Standing - FVector(0.0, 0.0, TaxiCompany::PersonHalfHeight + 5.0) + FVector(0.0, 0.0, 90.0);
			OutRoad = FVector(Road.X, Road.Y, Ground.ImpactPoint.Z);
			OutStreet = Street;
			return true;
		}
	}
	return false;
}

void ATaxiCompany::UpdateRoute(const FVector& From, const FVector& To)
{
	const double Now = GetWorld()->GetTimeSeconds();
	if (Now < NextRouteTime)
	{
		return;
	}
	NextRouteTime = Now + TaxiCompany::RouteRefresh;
	Route.Reset();
	if (const ATrafficManager* Traffic = GetTraffic())
	{
		if (Traffic->FindPath(From, To, Route))
		{
			Route.Insert(From, 0);
			Route.Add(To);
		}
	}
}

void ATaxiCompany::DrawRoute() const
{
#if ENABLE_DRAW_DEBUG
	// A yellow line along the streets to follow, like a satnav the 1930s did not have.
	const FVector Lift(0.0, 0.0, 60.0);
	for (int32 i = 0; i + 1 < Route.Num(); ++i)
	{
		DrawDebugLine(GetWorld(), Route[i] + Lift, Route[i + 1] + Lift, FColor(255, 190, 40), false, -1.f, 0, 25.f);
	}
#endif
}
