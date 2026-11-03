// Fill out your copyright notice in the Description page of Project Settings.

#include "CityAmbience.h"

#include "Camera/PlayerCameraManager.h"
#include "CityAmbienceSound.h"
#include "Components/SceneComponent.h"
#include "DayNight.h"
#include "Engine/World.h"
#include "KeletiStation.h"
#include "Kismet/GameplayStatics.h"
#include "MarketHall.h"

namespace CityAmbience
{
	// Places that sound different (cm, X east, Y south).
	// Tisza Kálmán tér, the park: birds by day, crickets at night, leaves in the wind.
	const FVector2D ParkAt(60000.0, 500.0);
	constexpr double ParkNear = 4500.0;
	constexpr double ParkFade = 7000.0;

	// The Teleki tér market; the market hall is found from AMarketHall.
	const FVector2D TelekiAt(99969.0, 28889.0);
	constexpr double MarketNear = 3000.0;
	constexpr double MarketFade = 6000.0;

	// Rákóczi út, from Blaha Lujza tér to Baross tér: the busiest street, murmur and horns.
	const FVector2D BlahaAt(-6500.0, 1500.0);
	const FVector2D BarossAt(90531.0, -35706.0);
	constexpr double BusyNear = 1800.0;
	constexpr double BusyFade = 6000.0;

	// The tracks behind Keleti's front: whistles and engines heard from far off.
	const FVector2D TracksBehindFront(0.0, -12000.0);
	constexpr double StationHeard = 45000.0;

	// The church on Horváth Mihály tér, striking the hours.
	const FVector2D ChurchAt(43800.0, 72700.0);
	constexpr double ChurchHeard = 70000.0;

	double Nearness(double Distance, double Near, double Fade)
	{
		return FMath::Clamp(1.0 - (Distance - Near) / Fade, 0.0, 1.0);
	}

	/** -1 when the place lies to the listener's left, 1 to the right. */
	float SideOf(const FVector2D& From, const FVector2D& Right, const FVector2D& Place)
	{
		const FVector2D Toward = (Place - From).GetSafeNormal();
		return static_cast<float>(FVector2D::DotProduct(Toward, Right));
	}
}

ACityAmbience::ACityAmbience()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	Sound = CreateDefaultSubobject<UCityAmbienceSound>(TEXT("Sound"));
	Sound->SetupAttachment(RootComponent);
	// Heard the same wherever it is: the synth pans its own far-off sounds.
	Sound->bAllowSpatialization = false;
}

void ACityAmbience::BeginPlay()
{
	Super::BeginPlay();
}

void ACityAmbience::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0);
	if (!Camera)
	{
		return;
	}
	const FVector Ear = Camera->GetCameraLocation();
	const FVector2D From(Ear.X, Ear.Y);
	const double Yaw = FMath::DegreesToRadians(static_cast<double>(Camera->GetCameraRotation().Yaw));
	const FVector2D Right(-FMath::Sin(Yaw), FMath::Cos(Yaw));

	CitySynth::FScene Scene;
	if (const ADayNight* Clock = ADayNight::Get(GetWorld()))
	{
		Scene.Hour = Clock->GetHour();
		Scene.Darkness = FMath::Clamp(Clock->GetDarkness(), 0.f, 1.f);
	}

	Scene.Park = static_cast<float>(CityAmbience::Nearness(FVector2D::Distance(From, CityAmbience::ParkAt), CityAmbience::ParkNear, CityAmbience::ParkFade));

	const double ToTeleki = FVector2D::Distance(From, CityAmbience::TelekiAt);
	const double ToHall = FVector2D::Distance(From, AMarketHall::FrontAt());
	Scene.Market = static_cast<float>(CityAmbience::Nearness(FMath::Min(ToTeleki, ToHall), CityAmbience::MarketNear, CityAmbience::MarketFade));

	const FVector2D OnRakoczi = FMath::ClosestPointOnSegment2D(From, CityAmbience::BlahaAt, CityAmbience::BarossAt);
	Scene.Busy = static_cast<float>(CityAmbience::Nearness(FVector2D::Distance(From, OnRakoczi), CityAmbience::BusyNear, CityAmbience::BusyFade));

	const FVector2D Tracks = AKeletiStation::FrontAt() + CityAmbience::TracksBehindFront;
	Scene.Station = static_cast<float>(CityAmbience::Nearness(FVector2D::Distance(From, Tracks), 0.0, CityAmbience::StationHeard));
	Scene.StationPan = CityAmbience::SideOf(From, Right, Tracks);

	Scene.Church = static_cast<float>(CityAmbience::Nearness(FVector2D::Distance(From, CityAmbience::ChurchAt), 0.0, CityAmbience::ChurchHeard));
	Scene.ChurchPan = CityAmbience::SideOf(From, Right, CityAmbience::ChurchAt);

	Sound->SetScene(Scene);
	// Start only once the sound knows the hour, so it does not strike a wrong one at first.
	if (!bStarted)
	{
		bStarted = true;
		Sound->Start();
	}
}
