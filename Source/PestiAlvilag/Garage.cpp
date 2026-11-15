// Fill out your copyright notice in the Description page of Project Settings.

#include "Garage.h"

#include "CarModel.h"
#include "CarVisuals.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GarageModel.h"
#include "Kismet/GameplayStatics.h"
#include "PestiAlvilagGameModeBase.h"
#include "PestiCar.h"
#include "PestiHud.h"
#include "PestiTaxi.h"
#include "ShapeSet.h"

namespace GaragePlace
{
	// The garage is the player's from the end of act 1 (mission 8 moves him into the attic).
	const FName OpenedBy(TEXT("Bunda"));
	// The doors open when he comes this near the door (cm), and shut once he is this far.
	constexpr double OpenNear = 1200.0;
	constexpr double ShutFar = 1600.0;
	// How long the doors take to swing open or shut (s), and how far they swing (degrees).
	constexpr float SwingSeconds = 1.4f;
	constexpr double SwingAngle = 84.0;
	// A car this slow (cm/s) in the room counts as left there.
	constexpr double StillSpeed = 60.0;
	// The doorway reaches this far out in front of the doors (cm), and this far either side.
	constexpr double DoorwayOut = 450.0;
	constexpr double DoorwaySide = 60.0;
	// A kept car is put down this high over the floor and settles by itself (cm).
	constexpr double CarLift = 120.0;
	// The garage is drawn from this far off (cm).
	constexpr float DrawDistance = 15000.f;

	constexpr int32 NoteKey = 81;
	constexpr float NoteSeconds = 6.f;
	constexpr double WantedNoteEvery = 10.0;
}

AGarage::AGarage()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void AGarage::BeginPlay()
{
	Super::BeginPlay();

	const FTransform Here = GetActorTransform();
	TArray<MarketModel::FShape> Shapes;
	TArray<BozsiHouseModel::FLamp> Lamps;
	TArray<BozsiHouseModel::FSign> Signs;
	GarageModel::Build(Shapes, Lamps);
	GarageModel::Signs(Signs);
	Room = GetWorld()->SpawnActor<AShapeSet>(AShapeSet::StaticClass(), Here.GetLocation(), Here.Rotator());
	if (Room)
	{
		Room->SetDrawDistance(GaragePlace::DrawDistance);
		Room->AddShapes(Shapes);
		for (const BozsiHouseModel::FLamp& Lamp : Lamps)
		{
			Room->AddLamp(Lamp.At, Lamp.Radius);
		}
		for (const BozsiHouseModel::FSign& Sign : Signs)
		{
			Room->AddSign(Sign.Text, Sign.At, Sign.Yaw, Sign.Height, Sign.Color);
		}
	}

	// The two doors, each in its hinge's frame.
	for (const double Side : { -1.0, 1.0 })
	{
		TArray<MarketModel::FShape> Leaf;
		GarageModel::BuildLeaf(Leaf, Side);
		AShapeSet* Door = GetWorld()->SpawnActor<AShapeSet>(AShapeSet::StaticClass(), Here.TransformPosition(GarageModel::Hinge(Side)), Here.Rotator());
		if (Door)
		{
			Door->SetDrawDistance(GaragePlace::DrawDistance);
			Door->AddShapes(Leaf);
		}
		Leaves.Add(Door);
	}
	SetDoors(0.f);
}

void AGarage::SetDoors(float Amount)
{
	Opened = FMath::Clamp(Amount, 0.f, 1.f);
	const FTransform Here = GetActorTransform();
	for (int32 i = 0; i < Leaves.Num(); ++i)
	{
		AShapeSet* Door = Leaves[i];
		if (!Door)
		{
			continue;
		}
		const double Side = i == 0 ? -1.0 : 1.0;
		// Out to the street: the left door turns one way, the right the other.
		const double Swing = Side * GaragePlace::SwingAngle * FMath::SmoothStep(0.f, 1.f, Opened);
		Door->SetActorRotation(FRotator(0.0, Here.Rotator().Yaw + Swing, 0.0));
	}
	// Shut doors block the way; once they move they let cars and people by.
	const bool bBlock = Opened <= 0.f;
	if (bBlock != bDoorsBlock)
	{
		bDoorsBlock = bBlock;
		for (AShapeSet* Door : Leaves)
		{
			if (Door)
			{
				Door->SetActorEnableCollision(bBlock);
			}
		}
	}
}

bool AGarage::IsInRoom(const FVector& Point) const
{
	const FVector Local = GetActorTransform().InverseTransformPosition(Point);
	return Local.X < GarageModel::InFrom && Local.X > -GarageModel::Depth && FMath::Abs(Local.Y) < GarageModel::HalfWidth;
}

bool AGarage::IsInDoorway(const FVector& Point) const
{
	const FVector Local = GetActorTransform().InverseTransformPosition(Point);
	return Local.X < GaragePlace::DoorwayOut && Local.X > -GarageModel::Depth
		&& FMath::Abs(Local.Y) < GarageModel::HalfWidth + GaragePlace::DoorwaySide;
}

APestiCar* AGarage::FindCarInRoom() const
{
	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	for (TActorIterator<APestiCar> It(GetWorld()); It; ++It)
	{
		APestiCar* Car = *It;
		if (Car && Car != Player && IsInRoom(Car->GetActorLocation()) && Car->GetVelocity().Size() < GaragePlace::StillSpeed)
		{
			return Car;
		}
	}
	return nullptr;
}

void AGarage::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this);
	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Game || !Player || !Game->IsMissionDone(GaragePlace::OpenedBy))
	{
		return;
	}

	const FVector Front = GetActorTransform().TransformPosition(FVector(GaragePlace::DoorwayOut * 0.5, 0.0, 0.0));
	const double Distance = FVector::Dist2D(Player->GetActorLocation(), Front);
	const bool bWanted = Game->GetWantedLevel() > 0;
	bool bWantOpen = Opened > 0.f ? Distance < GaragePlace::ShutFar : Distance < GaragePlace::OpenNear;
	if (bWanted && Opened <= 0.f && bWantOpen)
	{
		// Not with the police on his tail: they would see where he keeps his cars.
		bWantOpen = false;
		const double Now = GetWorld()->GetTimeSeconds();
		if (Now - LastWantedNote > GaragePlace::WantedNoteEvery)
		{
			LastWantedNote = Now;
			Note(TEXT("A garázs zárva marad, amíg a rendőrök a nyomodban vannak  (The garage stays shut while the police are on your tail)"));
		}
	}
	// Never shut on him, or on a car still rolling in or out.
	if (!bWantOpen && Opened > 0.f)
	{
		if (IsInDoorway(Player->GetActorLocation()))
		{
			bWantOpen = true;
		}
		for (TActorIterator<APestiCar> It(GetWorld()); It && !bWantOpen; ++It)
		{
			const APestiCar* Car = *It;
			if (Car && IsInDoorway(Car->GetActorLocation()) && !IsInRoom(Car->GetActorLocation()))
			{
				bWantOpen = true;
			}
		}
	}

	const float Was = Opened;
	const float Step = DeltaSeconds / GaragePlace::SwingSeconds;
	if (bWantOpen && Opened < 1.f)
	{
		if (Was <= 0.f)
		{
			BringOutKeptCar();
		}
		SetDoors(Opened + Step);
	}
	else if (!bWantOpen && Opened > 0.f)
	{
		SetDoors(Opened - Step);
		if (Opened <= 0.f)
		{
			OnShut();
		}
	}
}

void AGarage::BringOutKeptCar()
{
	APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this);
	if (!Game || IsValid(KeptCar.Get()) || FindCarInRoom())
	{
		return;
	}
	const FGarageCar& Kept = Game->GetGarageCar();
	if (Kept.Body < 0 || Kept.Body >= static_cast<int32>(CarModel::EBody::Count))
	{
		return;
	}
	CarModel::FStyle Look;
	Look.Body = static_cast<CarModel::EBody>(Kept.Body);
	Look.Wheel = static_cast<CarModel::EWheel>(Kept.Wheel);
	Look.PaintColor = Kept.Paint;
	Look.GuardColor = Kept.Guard;
	Look.SpokeColor = Kept.Spoke;
	Look.AccentColor = Kept.Accent;
	Look.bTaxi = Kept.bTaxi;
	Look.bPolice = Kept.bPolice;

	const FTransform Here = GetActorTransform();
	const FTransform Place(FRotator(0.0, Here.Rotator().Yaw + GarageModel::CarYaw, 0.0),
		Here.TransformPosition(GarageModel::CarAt) + FVector(0.0, 0.0, GaragePlace::CarLift));
	UClass* Kind = Look.bTaxi ? APestiTaxi::StaticClass() : APestiCar::StaticClass();
	APestiCar* Car = GetWorld()->SpawnActorDeferred<APestiCar>(Kind, Place, nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (!Car)
	{
		return;
	}
	Car->BodyColor = Look.PaintColor;
	Car->SetStyle(Look);
	Car->FinishSpawning(Place);
	KeptCar = Car;
}

void AGarage::OnShut()
{
	APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this);
	if (!Game)
	{
		return;
	}
	const bool bHadCar = Game->GetGarageCar().Body >= 0;
	APestiCar* Car = FindCarInRoom();
	if (!Car)
	{
		// Nothing in the room: whatever was kept has been driven off.
		if (bHadCar)
		{
			Game->SetGarageCar(FGarageCar());
		}
		KeptCar = nullptr;
		return;
	}
	const UCarVisuals* Looks = Car->FindComponentByClass<UCarVisuals>();
	if (!Looks)
	{
		return;
	}
	const CarModel::FStyle& Look = Looks->GetStyle();
	FGarageCar Kept;
	Kept.Body = static_cast<int32>(Look.Body);
	Kept.Wheel = static_cast<int32>(Look.Wheel);
	Kept.Paint = Look.PaintColor;
	Kept.Guard = Look.GuardColor;
	Kept.Spoke = Look.SpokeColor;
	Kept.Accent = Look.AccentColor;
	Kept.bTaxi = Look.bTaxi || Car->IsA<APestiTaxi>();
	Kept.bPolice = Look.bPolice;
	Game->SetGarageCar(Kept);
	// Put away behind the doors; it stands there again when they next open.
	const bool bNew = Car != KeptCar.Get();
	KeptCar = nullptr;
	Car->Destroy();
	if (bNew)
	{
		Note(TEXT("Az autó a garázsban marad. Otthon ments, hogy meg is maradjon!  (The car stays in the garage. Save at home to keep it!)"));
	}
}

void AGarage::Note(const FString& Text) const
{
	APestiHUD::Show(this, GaragePlace::NoteKey, GaragePlace::NoteSeconds, FColor(255, 210, 60), Text);
}
