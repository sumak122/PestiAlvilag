// Fill out your copyright notice in the Description page of Project Settings.

#include "Tattersall.h"

#include "CarModel.h"
#include "CarShowModel.h"
#include "CarVisuals.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "DayNight.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "PersonModel.h"
#include "ShapeSet.h"
#include "StoryPerson.h"

namespace TattersallPlace
{
	// The middle of the hall's front, on the forecourt behind the gate
	// lodges on Kerepesi út, and the way it faces (north, to the street).
	const FVector2D CityAt(117600.0, -29400.0);
	constexpr double CityYaw = -90.0;
	// How far up and down to look for the ground there (cm).
	constexpr double GroundLook = 10000.0;

	const FName HallKey(TEXT("Tattersall"));
	const FName DoorsKey(TEXT("TattersallDoors"));
	// The hall's shapes are drawn from this far off (cm).
	constexpr float DrawDistance = 30000.f;
	// Of the hall's many lamps only every so many light up for real (the
	// rest glow), each reaching this much further.
	constexpr int32 LampEvery = 4;
	constexpr float LampReachUp = 1.6f;

	// The cars on show stand on their stands; their boxes as a parked car's
	// (PestiCar's box, its bottom HoverHeight over the floor).
	const FVector CarBox(210.0, 85.0, 70.0);
	constexpr double CarUp = 90.0;

	// Daylight in the hall: lamps down the middle under the roof, as bright
	// as so many lit grey walls by day and at night (ADayNight::LampCandela),
	// reaching the side walls; on only while the player is this near (cm).
	const double HallLightX[] = { -700.0, -1750.0, -2800.0 };
	constexpr double HallLightZ = 700.0;
	constexpr float HallLightReach = 1500.f;
	constexpr float HallLightDay = 60.f;
	constexpr float HallLightNight = 16.f;
	const FLinearColor HallLightColor(1.f, 0.9f, 0.75f);
	constexpr double LightsNear = 9000.0;

	// How often doors, people and lamps follow the clock (s).
	constexpr float UpdateEvery = 0.5f;
}

ATattersall::ATattersall()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	// The cars on show (the SSK first): made here, dressed in BeginPlay.
	for (int32 i = 0; i < 5; ++i)
	{
		UBoxComponent* Box = CreateDefaultSubobject<UBoxComponent>(*FString::Printf(TEXT("CarBox%d"), i));
		Box->SetupAttachment(RootComponent);
		Box->SetBoxExtent(TattersallPlace::CarBox);
		Box->SetCollisionProfileName(TEXT("BlockAll"));
		CarBoxes.Add(Box);
		CarLooks.Add(CreateDefaultSubobject<UCarVisuals>(*FString::Printf(TEXT("CarLooks%d"), i)));
	}
}

FTransform ATattersall::Frame(const UWorld* World)
{
	if (World)
	{
		for (TActorIterator<ATattersall> It(World); It; ++It)
		{
			return It->GetActorTransform();
		}
	}
	// Not built (yet): where it would stand, on the ground there.
	double Z = 0.0;
	if (World)
	{
		FHitResult Hit;
		const FVector Top(TattersallPlace::CityAt, TattersallPlace::GroundLook);
		const FVector Bottom(TattersallPlace::CityAt, -TattersallPlace::GroundLook);
		if (World->LineTraceSingleByChannel(Hit, Top, Bottom, ECC_WorldStatic))
		{
			Z = Hit.ImpactPoint.Z;
		}
	}
	return FTransform(FRotator(0.0, TattersallPlace::CityYaw, 0.0), FVector(TattersallPlace::CityAt, Z));
}

FVector ATattersall::ToCity(const UWorld* World, const FVector& Local)
{
	return Frame(World).TransformPosition(Local);
}

bool ATattersall::IsOpen(const UWorld* World)
{
	const ADayNight* Clock = World ? ADayNight::Get(const_cast<UWorld*>(World)) : nullptr;
	if (!Clock)
	{
		return true;
	}
	const float Hour = Clock->GetHour();
	return Hour >= CarShowModel::OpenFrom && Hour < CarShowModel::OpenUntil;
}

void ATattersall::BeginPlay()
{
	Super::BeginPlay();

	PutUpHall();

	// The cars on show, on their stands.
	struct FShow
	{
		FVector At;
		double Yaw;
		CarModel::FStyle Look;
	};
	CarModel::FStyle Limousine;
	Limousine.Body = CarModel::EBody::Limousine;
	Limousine.PaintColor = FLinearColor(0.20f, 0.02f, 0.03f);
	Limousine.Wheel = CarModel::EWheel::Wire;
	CarModel::FStyle Tourer;
	Tourer.Body = CarModel::EBody::Tourer;
	Tourer.PaintColor = FLinearColor(0.42f, 0.38f, 0.30f);
	Tourer.Wheel = CarModel::EWheel::Wire;
	CarModel::FStyle Steyr;
	Steyr.Body = CarModel::EBody::Steyr;
	Steyr.PaintColor = FLinearColor(0.03f, 0.07f, 0.05f);
	CarModel::FStyle Coupe;
	Coupe.Body = CarModel::EBody::Coupe;
	Coupe.PaintColor = FLinearColor(0.50f, 0.46f, 0.38f);
	Coupe.Wheel = CarModel::EWheel::Disc;
	using namespace CarShowModel::Spots;
	const FShow Shows[] = {
		{ Ssk, SskYaw, CarModel::SskStyle() },
		{ CarFrontLeft, CarFrontLeftYaw, Limousine },
		{ CarFrontRight, CarFrontRightYaw, Tourer },
		{ CarBackLeft, CarBackLeftYaw, Steyr },
		{ CarBackRight, CarBackRightYaw, Coupe } };
	for (int32 i = 0; i < CarBoxes.Num() && i < CarLooks.Num() && i < static_cast<int32>(UE_ARRAY_COUNT(Shows)); ++i)
	{
		UBoxComponent* Box = CarBoxes[i];
		UCarVisuals* Looks = CarLooks[i];
		if (!Box || !Looks)
		{
			continue;
		}
		Box->SetRelativeLocationAndRotation(Shows[i].At + FVector(0.0, 0.0, TattersallPlace::CarUp), FRotator(0.0, Shows[i].Yaw, 0.0));
		Looks->SetEngineRunning(false);
		Looks->Show(Box, Shows[i].Look);
	}

	// Daylight in the hall, set by Update.
	for (const double X : TattersallPlace::HallLightX)
	{
		UPointLightComponent* Light = NewObject<UPointLightComponent>(this, NAME_None, RF_Transient);
		Light->SetupAttachment(RootComponent);
		Light->SetRelativeLocation(FVector(X, 0.0, TattersallPlace::HallLightZ));
		Light->SetMobility(EComponentMobility::Movable);
		Light->SetCastShadows(false);
		Light->SetIntensityUnits(ELightUnits::Candelas);
		Light->SetAttenuationRadius(TattersallPlace::HallLightReach);
		Light->SetLightColor(TattersallPlace::HallLightColor);
		Light->SetVisibility(false);
		Light->RegisterComponent();
		HallLights.Add(Light);
	}

	PlacePeople();
	Update();
}

void ATattersall::PutUpHall()
{
	const FTransform Here = GetActorTransform();
	if (!AShapeSet::Find(GetWorld(), TattersallPlace::HallKey))
	{
		TArray<MarketModel::FShape> Shapes;
		TArray<BozsiHouseModel::FLamp> Lamps;
		TArray<BozsiHouseModel::FSign> Signs;
		CarShowModel::Build(Shapes, Lamps);
		CarShowModel::Signs(Signs);
		Hall = GetWorld()->SpawnActor<AShapeSet>(AShapeSet::StaticClass(), Here.GetLocation(), Here.Rotator());
		if (Hall)
		{
			Hall->SetKey(TattersallPlace::HallKey);
			Hall->SetDrawDistance(TattersallPlace::DrawDistance);
			Hall->AddShapes(Shapes);
			for (int32 i = 0; i < Lamps.Num(); i += TattersallPlace::LampEvery)
			{
				Hall->AddLamp(Lamps[i].At, Lamps[i].Radius * TattersallPlace::LampReachUp);
			}
			for (const BozsiHouseModel::FSign& Sign : Signs)
			{
				Hall->AddSign(Sign.Text, Sign.At, Sign.Yaw, Sign.Height, Sign.Color);
			}
		}
	}
	if (!AShapeSet::Find(GetWorld(), TattersallPlace::DoorsKey))
	{
		TArray<MarketModel::FShape> Shapes;
		CarShowModel::BuildDoors(Shapes);
		Doors = GetWorld()->SpawnActor<AShapeSet>(AShapeSet::StaticClass(), Here.GetLocation(), Here.Rotator());
		if (Doors)
		{
			Doors->SetKey(TattersallPlace::DoorsKey);
			Doors->SetDrawDistance(TattersallPlace::DrawDistance);
			Doors->AddShapes(Shapes);
		}
	}
}

void ATattersall::PlacePeople()
{
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const FTransform Here = GetActorTransform();
	const double Turn = Here.Rotator().Yaw;
	const auto Place = [&](const FVector& Spot, double SpotYaw, const PersonKit::FOutfit& Outfit) -> AStoryPerson*
	{
		const FVector At = Here.TransformPosition(Spot) + FVector(0.0, 0.0, PersonModel::MiddleHeight);
		AStoryPerson* Person = GetWorld()->SpawnActor<AStoryPerson>(AStoryPerson::StaticClass(), At, FRotator(0.0, Turn + SpotYaw, 0.0), Params);
		if (Person)
		{
			Person->SetOutfit(Outfit);
		}
		return Person;
	};
	Salesman = Place(CarShowModel::SalesmanSpot, CarShowModel::SalesmanYaw, CarShowModel::Salesman());
	for (int32 i = 0; i < CarShowModel::NumVisitors; ++i)
	{
		const CarShowModel::FVisitorSpot& Spot = CarShowModel::VisitorSpots[i];
		if (AStoryPerson* Visitor = Place(Spot.At, Spot.Yaw, CarShowModel::Visitor(i)))
		{
			Visitors.Add(Visitor);
		}
	}
}

void ATattersall::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateIn -= DeltaSeconds;
	if (UpdateIn > 0.f)
	{
		return;
	}
	UpdateIn = TattersallPlace::UpdateEvery;
	Update();
}

void ATattersall::Update()
{
	// Open by day: the glass doors stand open (out of sight), the people are in.
	const int32 Open = IsOpen(GetWorld()) ? 1 : 0;
	if (Open != OpenState)
	{
		OpenState = Open;
		if (Doors)
		{
			Doors->SetActorHiddenInGame(Open == 1);
			Doors->SetActorEnableCollision(Open == 0);
		}
		if (Salesman)
		{
			Salesman->SetInWorld(Open == 1);
		}
		for (AStoryPerson* Visitor : Visitors)
		{
			if (Visitor)
			{
				Visitor->SetInWorld(Open == 1);
			}
		}
	}

	// The hall's own light, only while the player is about.
	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	const bool bNear = Player && FVector::Dist2D(Player->GetActorLocation(), GetActorLocation()) < TattersallPlace::LightsNear;
	const ADayNight* Clock = ADayNight::Get(GetWorld());
	float Candela = 0.f;
	if (bNear && Clock)
	{
		// Between day and night on a log scale, as the eye's exposure goes.
		const float Strength = FMath::Exp(FMath::Lerp(FMath::Loge(TattersallPlace::HallLightDay), FMath::Loge(TattersallPlace::HallLightNight), Clock->GetDarkness()));
		Candela = Clock->LampCandela(Strength);
	}
	for (UPointLightComponent* Light : HallLights)
	{
		if (Light)
		{
			Light->SetVisibility(bNear);
			Light->SetIntensity(Candela);
		}
	}
}
