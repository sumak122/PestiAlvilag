// Fill out your copyright notice in the Description page of Project Settings.

#include "TestGrounds.h"

#include "Brawl.h"
#include "CarModel.h"
#include "ChaseThief.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/TextRenderComponent.h"
#include "DayNight.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Bicycle.h"
#include "HorseCart.h"
#include "InputCoreTypes.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PersonModel.h"
#include "PestiAlvilagGameModeBase.h"
#include "PestiCar.h"
#include "PestiHud.h"
#include "PestiPerson.h"
#include "PestiTaxi.h"
#include "Ruffian.h"
#include "TestGroundsModel.h"
#include "UObject/ConstructorHelpers.h"

namespace TestGroundsPlace
{
	// Things on the grounds are not drawn from further off than this (cm).
	constexpr float DrawDistance = 30000.f;

	// The gas lamps: as bright as a street lamp at night (see ADayNight::LampCandela), off by day.
	constexpr float LampNight = 10.f;
	constexpr float LampReach = 1600.f;
	const FLinearColor LampColor(1.f, 0.85f, 0.62f);
	constexpr float LampUpdate = 1.f;

	// The big line across the screen (see APestiHUD::Show).
	constexpr int32 AnnounceLine = 9;
	// The key help under the goal panel while he is out here.
	constexpr int32 HintLine = 50;
	const FColor HintColor(235, 215, 160);

	// Group fights as in mission 4: two go for him, the rest wait round him.
	constexpr int32 AtOnce = 2;
	constexpr float FirstEager = 0.8f;
	constexpr float SecondEager = 0.45f;
	constexpr double RingRadius = 380.0;
	const double RingAngles[] = { 75.0, -75.0, 125.0, -125.0 };
	// He turns to another attacker who is this much nearer than the one he faces.
	constexpr double TurnShare = 0.6;

	// How high over the ground a car's and a horse cart's middle is put down (they settle themselves).
	constexpr double CarLift = 90.0;
	constexpr double CartLift = 150.0;

	// The fighters of each number key.
	const ERuffian OneTough[] = { ERuffian::Bika };
	const ERuffian ThreeToughs[] = { ERuffian::Bika, ERuffian::Svihak, ERuffian::Csontos };
	const ERuffian ShopPeople[] = { ERuffian::Geza, ERuffian::ClerkOne, ERuffian::ClerkTwo, ERuffian::Hermann };
}

ATestGrounds::ATestGrounds()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicShapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	BaseMaterial = BasicShapeMaterial.Object;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMesh(TEXT("/Engine/BasicShapes/Cone.Cone"));
	Shapes = { CubeMesh.Object, CylinderMesh.Object, SphereMesh.Object, ConeMesh.Object };
}

ATestGrounds* ATestGrounds::Find(const UWorld* World)
{
	if (World)
	{
		for (TActorIterator<ATestGrounds> It(World); It; ++It)
		{
			return *It;
		}
	}
	return nullptr;
}

void ATestGrounds::Go(UWorld* World, ETestGround Where)
{
	APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	APestiPerson* Person = PC ? Cast<APestiPerson>(PC->GetPawn()) : nullptr;
	if (!Person || Person->IsRiding())
	{
		return;
	}
	ATestGrounds* Grounds = Find(World);
	if (!Grounds)
	{
		Grounds = World->SpawnActor<ATestGrounds>(ATestGrounds::StaticClass(), TestGroundsModel::Place, FRotator::ZeroRotator);
	}
	if (Grounds)
	{
		Grounds->Arrive(Person, PC, Where);
	}
}

void ATestGrounds::GoBack(UWorld* World)
{
	APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	APestiPerson* Person = PC ? Cast<APestiPerson>(PC->GetPawn()) : nullptr;
	ATestGrounds* Grounds = Find(World);
	if (!Person || Person->IsRiding() || !Grounds || !Grounds->bHasCitySpot)
	{
		return;
	}
	Grounds->ClearFoes(Person);
	Person->SetActorLocationAndRotation(Grounds->CityAt, FRotator(0.0, Grounds->CityYaw, 0.0), false, nullptr, ETeleportType::TeleportPhysics);
	Person->GetCharacterMovement()->StopMovementImmediately();
	PC->SetControlRotation(FRotator(0.0, Grounds->CityYaw, 0.0));
	Grounds->bHasCitySpot = false;
}

bool ATestGrounds::IsPlayerHere(const UWorld* World)
{
	const ATestGrounds* Grounds = Find(World);
	const APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	const APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	return Grounds && Pawn && Grounds->IsOnGrounds(Pawn->GetActorLocation());
}

FVector ATestGrounds::ToWorld(const FVector& Local) const
{
	return GetActorTransform().TransformPosition(Local);
}

FVector ATestGrounds::ToLocal(const FVector& World) const
{
	return GetActorTransform().InverseTransformPosition(World);
}

bool ATestGrounds::IsOnGrounds(const FVector& World) const
{
	const FVector Local = ToLocal(World);
	return Local.X > TestGroundsModel::SlabWest && Local.X < TestGroundsModel::SlabEast
		&& Local.Y > TestGroundsModel::SlabNorth && Local.Y < TestGroundsModel::SlabSouth
		&& FMath::Abs(Local.Z) < 5000.0;
}

bool ATestGrounds::IsInFightYard(const FVector& World) const
{
	const FVector Local = ToLocal(World);
	return FMath::Abs(Local.X) < TestGroundsModel::RingHalf && FMath::Abs(Local.Y) < TestGroundsModel::RingHalf + 100.0
		&& FMath::Abs(Local.Z) < 1000.0;
}

void ATestGrounds::BeginPlay()
{
	Super::BeginPlay();

	TArray<MarketModel::FShape> Pieces;
	TestGroundsModel::Build(Pieces);
	for (const MarketModel::FShape& Piece : Pieces)
	{
		AddShape(Piece.Shape, Piece.Color, Piece.At, Piece.Size, FRotator(Piece.Pitch, Piece.Yaw, Piece.Roll), Piece.bCollide);
	}

	TArray<TestGroundsModel::FSign> Signs;
	TestGroundsModel::Signs(Signs);
	for (const TestGroundsModel::FSign& Sign : Signs)
	{
		UTextRenderComponent* Text = NewObject<UTextRenderComponent>(this, NAME_None, RF_Transient);
		Text->SetupAttachment(RootComponent);
		Text->SetRelativeLocationAndRotation(Sign.At, FRotator(0.0, Sign.Yaw, 0.0));
		Text->SetText(FText::FromString(Sign.Text));
		Text->SetHorizontalAlignment(EHTA_Center);
		Text->SetVerticalAlignment(EVRTA_TextCenter);
		Text->SetWorldSize(Sign.Height);
		Text->SetTextRenderColor(Sign.Color.ToFColor(true));
		Text->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Text->RegisterComponent();
	}

	TArray<FVector> LampSpots;
	for (const FVector& Top : TestGroundsModel::LampTops)
	{
		LampSpots.Add(Top);
	}
	LampSpots.Add(TestGroundsModel::IslandLampTop);
	for (const FVector& Top : LampSpots)
	{
		UPointLightComponent* Light = NewObject<UPointLightComponent>(this, NAME_None, RF_Transient);
		Light->SetupAttachment(RootComponent);
		Light->SetRelativeLocation(Top - FVector(0.0, 0.0, 10.0));
		Light->SetMobility(EComponentMobility::Movable);
		Light->SetCastShadows(false);
		Light->SetIntensityUnits(ELightUnits::Candelas);
		Light->SetAttenuationRadius(TestGroundsPlace::LampReach);
		Light->SetLightColor(TestGroundsPlace::LampColor);
		Light->RegisterComponent();
		Lamps.Add(Light);
	}
	UpdateLamps();
}

void ATestGrounds::Arrive(APestiPerson* Person, APlayerController* PC, ETestGround Where)
{
	// Coming out from the city: remember where, to go back there.
	if (!IsOnGrounds(Person->GetActorLocation()))
	{
		CityAt = Person->GetActorLocation();
		CityYaw = Person->GetActorRotation().Yaw;
		bHasCitySpot = true;
	}
	if (Person->GetAct() != Brawl::EAct::None)
	{
		Person->EndFight();
	}
	const bool bFight = Where == ETestGround::Fight;
	const FVector Spot = bFight ? TestGroundsModel::RingArrive : TestGroundsModel::YardArrive;
	const double Yaw = bFight ? TestGroundsModel::RingArriveYaw : TestGroundsModel::YardArriveYaw;
	const FVector At = ToWorld(Spot) + FVector(0.0, 0.0, PersonModel::MiddleHeight + 10.0);
	Person->SetActorLocationAndRotation(At, FRotator(0.0, Yaw, 0.0), false, nullptr, ETeleportType::TeleportPhysics);
	Person->GetCharacterMovement()->StopMovementImmediately();
	PC->SetControlRotation(FRotator(0.0, Yaw, 0.0));
	APestiHUD::Show(this, TestGroundsPlace::AnnounceLine, 3.f, FColor::White,
		bFight ? TEXT("Pr\u00f3baterep: vereked\u00e9s  (Test ground: fighting)") : TEXT("Pr\u00f3baterep: j\u00e1rm\u0171vek  (Test ground: vehicles)"));
}

void ATestGrounds::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bVehiclesDue)
	{
		bVehiclesDue = false;
		SpawnVehicles();
	}
	const double Now = GetWorld()->GetTimeSeconds();
	if (Now >= NextLampUpdate)
	{
		NextLampUpdate = Now + TestGroundsPlace::LampUpdate;
		UpdateLamps();
	}

	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	APestiPerson* Person = PC ? Cast<APestiPerson>(PC->GetPawn()) : nullptr;
	if (Person && !Person->IsRiding())
	{
		ReadKeys(Person, PC);
		TickFight(Person);
	}
	else if (PC && PC->GetPawn() && IsOnGrounds(PC->GetPawn()->GetActorLocation()))
	{
		// Driving out here.
		if (PC->WasInputKeyJustPressed(EKeys::Zero))
		{
			SpawnVehicles();
		}
		APestiHUD::Show(this, TestGroundsPlace::HintLine, 0.f, TestGroundsPlace::HintColor,
			TEXT("Pr\u00f3baterep \u00b7 0: friss j\u00e1rm\u0171vek  (Test ground \u00b7 0: fresh vehicles)"));
	}
}

void ATestGrounds::ReadKeys(APestiPerson* Person, APlayerController* PC)
{
	const FVector At = Person->GetActorLocation();
	if (IsInFightYard(At))
	{
		APestiHUD::Show(this, TestGroundsPlace::HintLine, 0.f, TestGroundsPlace::HintColor,
			TEXT("1 tolvaj \u00b7 2 Bika \u00b7 3 h\u00e1rom jassz \u00b7 4 Hermann boltja \u00b7 0 takar\u00edt\u00e1s, gy\u00f3gyul\u00e1s  (1 thief \u00b7 2 Bika \u00b7 3 three ruffians \u00b7 4 Hermann's shop \u00b7 0 clear and heal)"));
		const FKey Keys[] = { EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four };
		for (int32 k = 0; k < static_cast<int32>(UE_ARRAY_COUNT(Keys)); ++k)
		{
			if (PC->WasInputKeyJustPressed(Keys[k]))
			{
				SpawnFoes(Person, k + 1);
				return;
			}
		}
		if (PC->WasInputKeyJustPressed(EKeys::Zero))
		{
			ClearFoes(Person);
		}
	}
	else if (IsOnGrounds(At))
	{
		APestiHUD::Show(this, TestGroundsPlace::HintLine, 0.f, TestGroundsPlace::HintColor,
			TEXT("Pr\u00f3baterep \u00b7 F: besz\u00e1ll\u00e1s \u00b7 0: friss j\u00e1rm\u0171vek  (Test ground \u00b7 F: get in \u00b7 0: fresh vehicles)"));
		if (PC->WasInputKeyJustPressed(EKeys::Zero))
		{
			SpawnVehicles();
		}
	}
}

void ATestGrounds::ClearFoes(APestiPerson* Person)
{
	if (Person && Person->GetAct() != Brawl::EAct::None)
	{
		Person->EndFight();
	}
	for (ARuffian* Tough : Foes)
	{
		if (IsValid(Tough))
		{
			Tough->Destroy();
		}
	}
	Foes.Reset();
	if (IsValid(Thief))
	{
		Thief->Destroy();
	}
	Thief = nullptr;
	WonAt = -1.0;
	if (APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this))
	{
		Game->HealPlayer(Game->GetMaxHealth());
	}
}

void ATestGrounds::SpawnFoes(APestiPerson* Person, int32 Set)
{
	ClearFoes(Person);

	TArray<ERuffian> Kinds;
	if (Set == 2)
	{
		Kinds.Append(TestGroundsPlace::OneTough, static_cast<int32>(UE_ARRAY_COUNT(TestGroundsPlace::OneTough)));
	}
	else if (Set == 3)
	{
		Kinds.Append(TestGroundsPlace::ThreeToughs, static_cast<int32>(UE_ARRAY_COUNT(TestGroundsPlace::ThreeToughs)));
	}
	else if (Set == 4)
	{
		Kinds.Append(TestGroundsPlace::ShopPeople, static_cast<int32>(UE_ARRAY_COUNT(TestGroundsPlace::ShopPeople)));
	}

	const FVector Middle = Person->GetActorLocation();
	auto PlaceAt = [this, &Middle](int32 Index)
	{
		const int32 Spots = static_cast<int32>(UE_ARRAY_COUNT(TestGroundsModel::FoeSpots));
		const FVector Spot = ToWorld(TestGroundsModel::FoeSpots[Index % Spots]) + FVector(0.0, 0.0, PersonModel::MiddleHeight + 10.0);
		const FVector Look = Middle - Spot;
		return FTransform(FRotator(0.0, FMath::RadiansToDegrees(FMath::Atan2(Look.Y, Look.X)), 0.0), Spot);
	};

	if (Set == 1)
	{
		const FTransform Place = PlaceAt(0);
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		Thief = GetWorld()->SpawnActor<AChaseThief>(AChaseThief::StaticClass(), Place.GetLocation(), Place.Rotator(), Params);
		if (Thief)
		{
			Thief->FightBack(Person);
			Person->StartFight(Thief.Get());
		}
		return;
	}

	for (int32 i = 0; i < Kinds.Num(); ++i)
	{
		const FTransform Place = PlaceAt(i);
		ARuffian* Tough = GetWorld()->SpawnActorDeferred<ARuffian>(ARuffian::StaticClass(), Place, this, nullptr,
			ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
		if (!Tough)
		{
			continue;
		}
		Tough->Kind = Kinds[i];
		Tough->FinishSpawning(Place);
		Foes.Add(Tough);
	}
}

void ATestGrounds::TickFight(APestiPerson* Person)
{
	const double Now = GetWorld()->GetTimeSeconds();
	auto Won = [this, Person, Now]()
	{
		if (WonAt >= 0.0)
		{
			return;
		}
		WonAt = Now;
		if (Person->GetAct() != Brawl::EAct::None)
		{
			Person->EndFight();
		}
		APestiHUD::Show(this, TestGroundsPlace::AnnounceLine, 3.f, FColor::White, TEXT("Nyert\u00e9l!  (You won!)"));
	};

	// The thief: one on one.
	if (IsValid(Thief))
	{
		if (Thief->IsBeaten())
		{
			Won();
		}
		else if (!Person->IsFighting() && IsInFightYard(Person->GetActorLocation()))
		{
			Person->StartFight(Thief.Get());
		}
		return;
	}
	if (Foes.Num() == 0)
	{
		return;
	}

	TArray<ARuffian*> Standing;
	for (ARuffian* Tough : Foes)
	{
		if (IsValid(Tough) && !Tough->IsBeaten())
		{
			Standing.Add(Tough);
		}
	}
	if (Standing.Num() == 0)
	{
		Won();
		return;
	}
	// Walked out of the yard: they wait where they are, fists up.
	if (!IsInFightYard(Person->GetActorLocation()))
	{
		for (ARuffian* Tough : Standing)
		{
			Tough->Circle(Person, Tough->GetActorLocation());
		}
		return;
	}

	// Two go for him; the rest stand round him with their fists up, waiting.
	const double PersonYaw = Person->GetActorRotation().Yaw;
	const FVector Middle = Person->GetActorLocation();
	const int32 Angles = static_cast<int32>(UE_ARRAY_COUNT(TestGroundsPlace::RingAngles));
	for (int32 k = TestGroundsPlace::AtOnce; k < Standing.Num(); ++k)
	{
		const double Angle = PersonYaw + TestGroundsPlace::RingAngles[(k - TestGroundsPlace::AtOnce) % Angles];
		Standing[k]->Circle(Person, Middle + FRotator(0.0, Angle, 0.0).RotateVector(FVector::ForwardVector) * TestGroundsPlace::RingRadius);
	}
	const int32 Coming = FMath::Min(TestGroundsPlace::AtOnce, Standing.Num());
	for (int32 k = 0; k < Coming; ++k)
	{
		Standing[k]->Attack(Person, k == 0 ? TestGroundsPlace::FirstEager : TestGroundsPlace::SecondEager);
	}

	// He faces the nearest man coming at him, and turns to another who gets much closer.
	ARuffian* Nearest = nullptr;
	double NearestApart = TNumericLimits<double>::Max();
	for (int32 k = 0; k < Coming; ++k)
	{
		const double Apart = FVector::Dist2D(Standing[k]->GetActorLocation(), Middle);
		if (Apart < NearestApart)
		{
			NearestApart = Apart;
			Nearest = Standing[k];
		}
	}
	ARuffian* Facing = Cast<ARuffian>(Person->GetFoe());
	const bool bFacingAttacker = Facing && Facing->IsAttacking() && !Facing->IsBeaten();
	if (!Person->IsFighting())
	{
		Person->StartFight(Nearest);
	}
	else if (!bFacingAttacker
		|| (Nearest != Facing && NearestApart < FVector::Dist2D(Facing->GetActorLocation(), Middle) * TestGroundsPlace::TurnShare))
	{
		const Brawl::EAct Doing = Person->GetAct();
		// Not in the middle of holding someone.
		if (Doing != Brawl::EAct::Holding && Doing != Brawl::EAct::Knee && Doing != Brawl::EAct::Held)
		{
			Person->SetFoe(Nearest);
		}
	}
}

void ATestGrounds::SpawnVehicles()
{
	const APlayerController* PC = GetWorld()->GetFirstPlayerController();
	const APawn* Driven = PC ? PC->GetPawn() : nullptr;
	for (APawn* Old : Vehicles)
	{
		if (IsValid(Old) && Old != Driven)
		{
			Old->Destroy();
		}
	}
	Vehicles.Reset();

	TArray<TestGroundsModel::FVehicleSpot> Spots;
	TestGroundsModel::Vehicles(Spots);
	for (const TestGroundsModel::FVehicleSpot& Spot : Spots)
	{
		using TestGroundsModel::EVehicle;
		const bool bCart = Spot.Kind == EVehicle::HorseCart;
		const FVector At = ToWorld(Spot.At) + FVector(0.0, 0.0, bCart ? TestGroundsPlace::CartLift : TestGroundsPlace::CarLift);
		// Something already standing there (the one he drove off in and left): leave the spot empty.
		if (Driven && FVector::Dist2D(Driven->GetActorLocation(), At) < 400.0)
		{
			continue;
		}
		const FTransform Place(FRotator(0.0, Spot.Yaw, 0.0), At);
		if (bCart)
		{
			AHorseCart* Cart = GetWorld()->SpawnActorDeferred<AHorseCart>(AHorseCart::StaticClass(), Place, nullptr, nullptr,
				ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
			if (Cart)
			{
				Cart->FinishSpawning(Place);
				Vehicles.Add(Cart);
			}
			continue;
		}
		if (Spot.Kind == EVehicle::Bicycle || Spot.Kind == EVehicle::ShopBicycle)
		{
			ABicycle* Bike = GetWorld()->SpawnActorDeferred<ABicycle>(ABicycle::StaticClass(), Place, nullptr, nullptr,
				ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
			if (Bike)
			{
				Bike->bShopBike = Spot.Kind == EVehicle::ShopBicycle;
				Bike->FinishSpawning(Place);
				Vehicles.Add(Bike);
			}
			continue;
		}

		CarModel::FStyle Look;
		UClass* Kind = APestiCar::StaticClass();
		if (Spot.Kind == EVehicle::Taxi || Spot.Kind == EVehicle::DeluxeTaxi)
		{
			// The special taxi as an ordinary company taxi in its looks, so the kiosk's own stays in town.
			Kind = APestiTaxi::StaticClass();
			Look = Spot.Kind == EVehicle::Taxi ? CarModel::TaxiStyle() : CarModel::DeluxeTaxiStyle();
		}
		else if (Spot.Body == CarModel::EBody::Steyr)
		{
			// Dr. Halász's Steyr (mission 7) in its own colours.
			Look = CarModel::SteyrStyle();
		}
		else if (Spot.Body == CarModel::EBody::Ssk)
		{
			// The car show's black Mercedes SSK in its own colours.
			Look = CarModel::SskStyle();
		}
		else
		{
			Look.Body = Spot.Body;
			Look.Wheel = Spot.Wheel;
			Look.PaintColor = Spot.Paint;
			if (Spot.Body == CarModel::EBody::TownCar || Spot.Body == CarModel::EBody::FordT)
			{
				Look.AccentColor = CarModel::OldBrass();
			}
			if (Spot.Body == CarModel::EBody::FordT)
			{
				Look.SpokeColor = CarModel::OldWood();
			}
			if (Spot.Kind == EVehicle::Police)
			{
				// As the force's own: dark blue on steel discs, the blue lamp by the windscreen.
				Look.SpokeColor = Spot.Paint;
				Look.bPolice = true;
				Look.AccentColor = FLinearColor(0.05f, 0.25f, 1.0f);
			}
		}
		APestiCar* Car = GetWorld()->SpawnActorDeferred<APestiCar>(Kind, Place, nullptr, nullptr,
			ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
		if (!Car)
		{
			continue;
		}
		Car->BodyColor = Look.PaintColor;
		Car->SetStyle(Look);
		if (Spot.Kind == EVehicle::DeluxeTaxi)
		{
			Car->MaxSpeed = 3300.f;
			Car->Acceleration = 1150.f;
		}
		Car->FinishSpawning(Place);
		Vehicles.Add(Car);
	}
}

void ATestGrounds::UpdateLamps()
{
	const ADayNight* Clock = ADayNight::Get(GetWorld());
	if (!Clock)
	{
		return;
	}
	const float Candela = Clock->LampCandela(TestGroundsPlace::LampNight) * Clock->GetDarkness();
	for (UPointLightComponent* Light : Lamps)
	{
		if (Light)
		{
			Light->SetIntensity(Candela);
		}
	}
}

void ATestGrounds::AddShape(int32 Shape, const FLinearColor& Color, const FVector& At, const FVector& Size, const FRotator& Turn, bool bCollide)
{
	if (!Shapes.IsValidIndex(Shape) || !Shapes[Shape])
	{
		return;
	}
	// One batch per shape, colour and collision; the instances are placed in the grounds' frame.
	const FString Key = FString::Printf(TEXT("%d %s %d"), Shape, *Color.ToString(), bCollide ? 1 : 0);
	UInstancedStaticMeshComponent* Instances = nullptr;
	if (const int32* Found = BatchIndex.Find(Key))
	{
		Instances = Batches[*Found];
	}
	else
	{
		Instances = NewObject<UInstancedStaticMeshComponent>(this, NAME_None, RF_Transient);
		Instances->SetupAttachment(RootComponent);
		Instances->SetStaticMesh(Shapes[Shape]);
		if (BaseMaterial)
		{
			UMaterialInstanceDynamic* Paint = UMaterialInstanceDynamic::Create(BaseMaterial, this);
			Paint->SetVectorParameterValue(TEXT("Color"), Color);
			Instances->SetMaterial(0, Paint);
		}
		Instances->SetCollisionProfileName(bCollide ? TEXT("BlockAll") : TEXT("NoCollision"));
		Instances->SetCullDistances(0, TestGroundsPlace::DrawDistance);
		Instances->SetCanEverAffectNavigation(false);
		Instances->RegisterComponent();
		BatchIndex.Add(Key, Batches.Add(Instances));
	}
	Instances->AddInstance(FTransform(Turn, At, Size / 100.0), false);
}
