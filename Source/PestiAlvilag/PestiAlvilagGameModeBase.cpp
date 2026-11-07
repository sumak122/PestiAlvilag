// Fill out your copyright notice in the Description page of Project Settings.


#include "PestiAlvilagGameModeBase.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "StartRingComponent.h"
#include "Cafe.h"
#include "SoundSpots.h"
#include "GarayModel.h"
#include "GarayYard.h"
#include "StableYard.h"
#include "StableYardModel.h"
#include "BoxingYard.h"
#include "BoxingYardModel.h"
#include "LaciYard.h"
#include "LaciYardModel.h"
#include "DayNight.h"
#include "HomeDoor.h"
#include "Bicycle.h"
#include "HorseCart.h"
#include "KeletiEagle.h"
#include "KeletiStation.h"
#include "Knockables.h"
#include "Lighters.h"
#include "MarketHall.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "MissionPrologue.h"
#include "MissionUzenet.h"
#include "MissionZsebtolvaj.h"
#include "MissionZsibvasar.h"
#include "MissionFelmondas.h"
#include "MissionKifutofiu.h"
#include "MissionElsoAuto.h"
#include "MissionBunda.h"
#include "BozsiHouseModel.h"
#include "FleaMarket.h"
#include "FurrierModel.h"
#include "ShapeSet.h"
#include "StoryPerson.h"
#include "Nepszinhaz.h"
#include "Pedestrians.h"
#include "Pigeons.h"
#include "Animals.h"
#include "ChimneySmoke.h"
#include "Weather.h"
#include "PolicePatrols.h"
#include "PestiCar.h"
#include "PestiHud.h"
#include "PestiMenu.h"
#include "PestiPerson.h"
#include "PestiSaveGame.h"
#include "PestiTaxi.h"
#include "StreetFurniture.h"
#include "TaxiCompany.h"
#include "TaxiRides.h"
#include "TramRides.h"
#include "FoodMenus.h"
#include "TrafficManager.h"

namespace PestiStart
{
	// The Népszínház stood on the Corvin side of Blaha Lujza tér, its front
	// towards the west (cm, X east, Y south).
	const FVector2D TheatreAt(-3600.f, 200.f);
	constexpr float TheatreYaw = 180.f;

	// The player's car is parked on the square in front of it.
	const FVector2D CarAt(-8600.f, 2600.f);
	constexpr float Yaw = -23.f;

	// Bözsi néni's street door on the north side of Népszínház utca, between
	// Kun utca and Teleki tér, facing the street.
	const FVector2D HomeDoorAt(58392.f, 21268.f);
	constexpr float HomeDoorYaw = 108.6f;
	// A bicycle stands this far out from the door and along the wall to its right (cm).
	constexpr double BikeOut = 90.0;
	constexpr double BikeAlong = 260.0;
	const FName Prologue(TEXT("Prologue"));
	const FName Uzenet(TEXT("Uzenet"));
	const FName Zsebtolvaj(TEXT("Zsebtolvaj"));
	// "Zsibvasar2": the first test build paid mission 4 at the crates, before the
	// ruffians fight was in it; a new name has those saves play it again whole.
	const FName Zsibvasar(TEXT("Zsibvasar2"));
	const FName Felmondas(TEXT("Felmondas"));
	const FName Kifutofiu(TEXT("Kifutofiu"));
	const FName ElsoAuto(TEXT("ElsoAuto"));
	const FName Bunda(TEXT("Bunda"));

	// Act 1 in order, for the menu's mission list (JumpToMission).
	const FName* const ActOne[] = { &Prologue, &Uzenet, &Zsebtolvaj, &Zsibvasar, &Felmondas, &Kifutofiu, &ElsoAuto, &Bunda };
	// The mission the next level start jumps to (1 to 8), or 0 for none.
	int32 PendingJump = 0;
	// After a jump: how long to wait for the mission's ring (s), and how far
	// out from the ring the player stands (cm), so it counts when he walks in.
	constexpr double RingWait = 10.0;
	constexpr double RingStandOff = 260.0;
	// The fight points note (AddFightPoints): its key on the screen, how long
	// it stays, and the gap (s) after which points start a new count.
	constexpr int32 PointsNoteKey = 61;
	constexpr float PointsNoteSeconds = 4.f;
	constexpr double PointsNoteGap = 6.0;

	// The built houses and shops made of basic shapes, found again by these names.
	const FName BozsiHouseKey(TEXT("BozsiHouse"));
	const FName FurrierKey(TEXT("Furrier"));
	// The street door the raid's escape comes out of, on Tisza Kálmán tér.
	const FName ExitDoorKey(TEXT("ExitDoor"));
	// A plain door where the home door is not: the attic before the end of act 1, Bözsi's street door after.
	const FName SpareDoorKey(TEXT("SpareDoor"));
	// Bözsi néni at her flea market stall (gone after the raid).
	const TCHAR* const BozsiName = TEXT("B\u00f6zsi n\u00e9ni");

	/** Puts up a set of shapes, its lamps and its lettering at At, turned by SetYaw. */
	inline void PutUpShapeSet(UWorld* World, FName Key, const FVector& At, double SetYaw, const TArray<MarketModel::FShape>& Shapes,
		const TArray<BozsiHouseModel::FLamp>& Lamps, const TArray<BozsiHouseModel::FSign>& Signs)
	{
		AShapeSet* Set = World->SpawnActor<AShapeSet>(AShapeSet::StaticClass(), At, FRotator(0.0, SetYaw, 0.0));
		if (!Set)
		{
			return;
		}
		Set->SetKey(Key);
		Set->AddShapes(Shapes);
		for (const BozsiHouseModel::FLamp& Lamp : Lamps)
		{
			Set->AddLamp(Lamp.At, Lamp.Radius);
		}
		for (const BozsiHouseModel::FSign& Sign : Signs)
		{
			Set->AddSign(Sign.Text, Sign.At, Sign.Yaw, Sign.Height, Sign.Color);
		}
	}

	// The next story mission starts a little after the last one is passed.
	constexpr float NextMissionDelay = 8.f;

	// Fainting: the picture goes dark, and he comes round at home (or back
	// at the station before he has a home), a pengő poorer for the doctor.
	constexpr float FaintFade = 1.5f;
	constexpr float FaintDark = 2.5f;
	constexpr int32 DoctorFee = 100;

	// The taxi company's kiosk west of the square, two of its taxis waiting
	// on the open ground beside it.
	const FVector2D TaxiCompanyAt(-12000.f, 3100.f);
	const FVector2D TaxiAt[] = { FVector2D(-10400.f, 2500.f), FVector2D(-11100.f, 2500.f) };
	// The special taxi for regulars waits closest to the kiosk.
	const FVector2D DeluxeTaxiAt(-11800.f, 2500.f);

	// Below this speed next to a police car counts as being caught (cm/s).
	constexpr float CaughtSpeed = 300.f;
	constexpr int32 MaxWantedLevel = 3;
}

APestiAlvilagGameModeBase::APestiAlvilagGameModeBase()
{
	PrimaryActorTick.bCanEverTick = true;
	DefaultPawnClass = APestiPerson::StaticClass();
	HUDClass = APestiHUD::StaticClass();
}

void APestiAlvilagGameModeBase::SpawnDeluxeTaxi()
{
	if (TActorIterator<APestiDeluxeTaxi>(GetWorld()))
	{
		return;
	}
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	const FVector Location(PestiStart::DeluxeTaxiAt, GroundZ(PestiStart::DeluxeTaxiAt) + 150.0);
	GetWorld()->SpawnActor<APestiDeluxeTaxi>(APestiDeluxeTaxi::StaticClass(), Location, FRotator::ZeroRotator, Params);
}

APestiAlvilagGameModeBase* APestiAlvilagGameModeBase::Get(const UObject* WorldContextObject)
{
	return Cast<APestiAlvilagGameModeBase>(UGameplayStatics::GetGameMode(WorldContextObject));
}

void APestiAlvilagGameModeBase::StartPlay()
{
	Super::StartPlay();

	// The clock, the sun and the moon, before anything that lights up at night.
	ADayNight::Get(GetWorld());

	SpawnLandmarks();

	// Put up the taxi company and its taxis unless the level already has one.
	if (!TActorIterator<ATaxiCompany>(GetWorld()))
	{
		const FVector Location(PestiStart::TaxiCompanyAt, GroundZ(PestiStart::TaxiCompanyAt));
		GetWorld()->SpawnActor<ATaxiCompany>(ATaxiCompany::StaticClass(), Location, FRotator::ZeroRotator);

		FActorSpawnParameters TaxiParams;
		TaxiParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		for (const FVector2D& At : PestiStart::TaxiAt)
		{
			const FVector TaxiLocation(At, GroundZ(At) + 150.0);
			GetWorld()->SpawnActor<APestiTaxi>(APestiTaxi::StaticClass(), TaxiLocation, FRotator::ZeroRotator, TaxiParams);
		}
	}

	// The player's home door: Bözsi néni's, and after act 1 the attic room's.
	if (!TActorIterator<AHomeDoor>(GetWorld()))
	{
		const FVector2D DoorAt = HomeDoorSpot();
		const FVector Location(DoorAt, GroundZ(DoorAt));
		GetWorld()->SpawnActor<AHomeDoor>(AHomeDoor::StaticClass(), Location, FRotator(0.0, HomeDoorFacing(), 0.0));
	}

	// The coffee house beside it, with its terrace on the pavement.
	if (!TActorIterator<ACafe>(GetWorld()))
	{
		const FVector Location(PestiStart::HomeDoorAt, GroundZ(PestiStart::HomeDoorAt));
		GetWorld()->SpawnActor<ACafe>(ACafe::StaticClass(), Location, FRotator(0.f, PestiStart::HomeDoorYaw, 0.f));
	}

	// Lamps, benches, advertising columns and the park on Tisza Kálmán tér.
	if (!TActorIterator<AStreetFurniture>(GetWorld()))
	{
		GetWorld()->SpawnActor<AStreetFurniture>();
	}

	// People walking the pavements.
	if (!TActorIterator<APedestrians>(GetWorld()))
	{
		GetWorld()->SpawnActor<APedestrians>();
	}

	// Crates, barrels, churns and dustbins to knock over.
	if (!TActorIterator<AKnockables>(GetWorld()))
	{
		GetWorld()->SpawnActor<AKnockables>();
	}

	// Pigeons on the squares.
	if (!TActorIterator<APigeons>(GetWorld()))
	{
		GetWorld()->SpawnActor<APigeons>();
	}

	// Sounds placed about the city: the church bell, Keleti, markets, pigeons, the park, the café, courtyards.
	if (!TActorIterator<ASoundSpots>(GetWorld()))
	{
		GetWorld()->SpawnActor<ASoundSpots>();
	}

	// Taxis to ride in as a paying passenger.
	if (!TActorIterator<ATaxiRides>(GetWorld()))
	{
		GetWorld()->SpawnActor<ATaxiRides>();
	}

	// Food to buy at the café, the station restaurant and the street sellers.
	if (!TActorIterator<AFoodMenus>(GetWorld()))
	{
		GetWorld()->SpawnActor<AFoodMenus>();
	}

	// The thirty lighters hidden about the district (after the furniture they lie on).
	if (!TActorIterator<ALighters>(GetWorld()))
	{
		GetWorld()->SpawnActor<ALighters>();
	}

	// Trams to ride as a passenger.
	if (!TActorIterator<ATramRides>(GetWorld()))
	{
		GetWorld()->SpawnActor<ATramRides>();
	}

	// Start traffic unless the level already has its own manager.
	if (!TActorIterator<ATrafficManager>(GetWorld()))
	{
		GetWorld()->SpawnActor<ATrafficManager>();
	}

	// Constables on the beat round their telephone boxes (after the traffic: they find their way along its streets).
	if (!TActorIterator<APolicePatrols>(GetWorld()))
	{
		GetWorld()->SpawnActor<APolicePatrols>();
	}

	// A new game begins with the prologue, a saved one with the first
	// mission not yet passed (traffic first: the prologue finds its route on the lanes).
	LoadSaveOnce();
	PlaceHomeDoor();
	SpawnNextMission();
}

void APestiAlvilagGameModeBase::StartNextMission(float Delay)
{
	NextMissionAt = GetWorld()->GetTimeSeconds() + (Delay >= 0.f ? Delay : PestiStart::NextMissionDelay);
}

void APestiAlvilagGameModeBase::SpawnNextMission()
{
	if (!IsMissionDone(PestiStart::Prologue))
	{
		GetWorld()->SpawnActor<AMissionPrologue>();
	}
	else if (!IsMissionDone(PestiStart::Uzenet))
	{
		GetWorld()->SpawnActor<AMissionUzenet>();
	}
	else if (!IsMissionDone(PestiStart::Zsebtolvaj))
	{
		GetWorld()->SpawnActor<AMissionZsebtolvaj>();
	}
	else if (!IsMissionDone(PestiStart::Zsibvasar))
	{
		GetWorld()->SpawnActor<AMissionZsibvasar>();
	}
	else if (!IsMissionDone(PestiStart::Felmondas))
	{
		GetWorld()->SpawnActor<AMissionFelmondas>();
	}
	else if (!IsMissionDone(PestiStart::Kifutofiu))
	{
		GetWorld()->SpawnActor<AMissionKifutofiu>();
	}
	else if (!IsMissionDone(PestiStart::ElsoAuto))
	{
		GetWorld()->SpawnActor<AMissionElsoAuto>();
	}
	else if (!IsMissionDone(PestiStart::Bunda))
	{
		GetWorld()->SpawnActor<AMissionBunda>();
	}
	else if (AStoryPerson* BozsiAtStall = AFleaMarket::FindKeeper(GetWorld(), PestiStart::BozsiName))
	{
		// Act 1 is over: Bözsi néni was taken away in the raid.
		BozsiAtStall->SetInWorld(false);
	}
}

APawn* APestiAlvilagGameModeBase::SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot)
{
	// A new game starts on the platform at Keleti station for the
	// prologue, a saved one at the home door, whatever PlayerStart the
	// level has. The station must stand first: the player starts inside it.
	SpawnLandmarks();
	LoadSaveOnce();
	PlaceHomeDoor();
	FVector2D StartAt = AMissionPrologue::StartAt();
	double StartYaw = AMissionPrologue::StartYaw();
	if (IsMissionDone(PestiStart::Prologue))
	{
		StartAt = HomeStartAt();
		StartYaw = HomeDoorFacing();
	}
	const FVector PersonLocation(StartAt, GroundZ(StartAt) + 100.0);
	APawn* Pawn = SpawnDefaultPawnAtTransform(NewPlayer, FTransform(FRotator(0.0, StartYaw, 0.0), PersonLocation));

	// The player's car, parked in front of the Népszínház (it settles onto the ground itself).
	const FRotator Facing(0.f, PestiStart::Yaw, 0.f);
	FActorSpawnParameters CarParams;
	CarParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	const FVector CarLocation(PestiStart::CarAt, GroundZ(PestiStart::CarAt) + 150.0);
	GetWorld()->SpawnActor<APestiCar>(APestiCar::StaticClass(), CarLocation, Facing, CarParams);

	if (!TActorIterator<AHorseCart>(GetWorld()))
	{
		// Pista bácsi's horse and cart wait in the carters' yard on Auróra utca.
		const FVector2D CartAt(AStableYard::ToCity(StableYardModel::Spots::Cart, 0.0));
		const FVector CartLocation(CartAt, GroundZ(CartAt) + 150.0);
		const double CartYaw = StableYardModel::CityYaw + StableYardModel::Spots::CartYaw;
		GetWorld()->SpawnActor<AHorseCart>(AHorseCart::StaticClass(), CartLocation, FRotator(0.0, CartYaw, 0.0), CarParams);
	}

	// (Only a roadster counts: the shop bicycles belong to the errand-boy mission, and the
	// city's bicycles, parked and ridden, to the streets.)
	bool bHasRoadster = false;
	for (TActorIterator<ABicycle> It(GetWorld()); It; ++It)
	{
		bHasRoadster = bHasRoadster || (!It->bShopBike && !It->bCityBike);
	}
	if (!bHasRoadster)
	{
		// A bicycle against the house wall a few steps along from Bözsi néni's door.
		const FRotator DoorFacing(0.0, PestiStart::HomeDoorYaw, 0.0);
		const FVector2D Out(DoorFacing.Vector());
		const FVector2D Along(FRotator(0.0, PestiStart::HomeDoorYaw + 90.0, 0.0).Vector());
		const FVector2D BikeAt = PestiStart::HomeDoorAt + Out * PestiStart::BikeOut + Along * PestiStart::BikeAlong;
		const FVector BikeLocation(BikeAt, GroundZ(BikeAt) + 80.0);
		GetWorld()->SpawnActor<ABicycle>(ABicycle::StaticClass(), BikeLocation, FRotator(0.0, PestiStart::HomeDoorYaw + 90.0, 0.0), CarParams);
	}

	return Pawn;
}

void APestiAlvilagGameModeBase::SpawnLandmarks()
{
	// Put up the Népszínház unless the level already has one.
	if (!TActorIterator<ANepszinhaz>(GetWorld()))
	{
		const FVector Location(PestiStart::TheatreAt, GroundZ(PestiStart::TheatreAt));
		GetWorld()->SpawnActor<ANepszinhaz>(ANepszinhaz::StaticClass(), Location, FRotator(0.f, PestiStart::TheatreYaw, 0.f));
	}

	// And Keleti station, its front on Baross tér.
	if (!TActorIterator<AKeletiStation>(GetWorld()))
	{
		// Its ground height is taken on the square just in front of it.
		const FVector2D At = AKeletiStation::FrontAt();
		const double Z = GroundZ(At - FVector2D(800.0, 0.0));
		GetWorld()->SpawnActor<AKeletiStation>(AKeletiStation::StaticClass(), FVector(At, Z), FRotator::ZeroRotator);
	}

	// And the market hall on Rákóczi tér, its gateway facing the square.
	if (!TActorIterator<AMarketHall>(GetWorld()))
	{
		const double Z = GroundZ(AMarketHall::ToCity(FVector2D(-800.0, 0.0)));
		GetWorld()->SpawnActor<AMarketHall>(AMarketHall::StaticClass(), FVector(AMarketHall::FrontAt(), Z), FRotator(0.0, AMarketHall::Yaw(), 0.0));
	}

	// And Garay utca 13: Hermann's shop and Schwimmer's timber yard (mission 5).
	if (!TActorIterator<AGarayYard>(GetWorld()))
	{
		const double Z = GroundZ(GarayModel::CityAt);
		GetWorld()->SpawnActor<AGarayYard>(AGarayYard::StaticClass(), FVector(GarayModel::CityAt, Z), FRotator(0.0, GarayModel::CityYaw, 0.0));
	}

	// And the carters' yard on Auróra utca, where Pista bácsi keeps his horse
	// (its ground height taken in the middle of the yard).
	if (!TActorIterator<AStableYard>(GetWorld()))
	{
		const double Z = GroundZ(FVector2D(AStableYard::ToCity(FVector(1500.0, 0.0, 0.0), 0.0)));
		GetWorld()->SpawnActor<AStableYard>(AStableYard::StaticClass(), FVector(StableYardModel::CityAt, Z), FRotator(0.0, StableYardModel::CityYaw, 0.0));
	}

	// And Laci's motor workshop on Szerdahelyi utca (mission 7), its ground
	// height taken in the middle of the yard.
	if (!TActorIterator<ALaciYard>(GetWorld()))
	{
		const double Z = GroundZ(FVector2D(ALaciYard::ToCity(LaciYardModel::Spots::Park, 0.0)));
		GetWorld()->SpawnActor<ALaciYard>(ALaciYard::StaticClass(), FVector(LaciYardModel::CityAt, Z), FRotator(0.0, LaciYardModel::CityYaw, 0.0));
	}

	// And Bandi's boxing yard, the grund on Kis Fuvaros utca (open after mission 3).
	if (!TActorIterator<ABoxingYard>(GetWorld()))
	{
		const double Z = GroundZ(FVector2D(ABoxingYard::ToCity(BoxingYardModel::RingAt, 0.0)));
		GetWorld()->SpawnActor<ABoxingYard>(ABoxingYard::StaticClass(), FVector(BoxingYardModel::CityAt, Z), FRotator(0.0, BoxingYardModel::CityYaw, 0.0));
	}

	// And Bözsi néni's house, Népszínház utca 29, built in place of the plain
	// map block so the player can go into its courtyard (mission 8); its
	// ground height taken on the pavement in front of the gateway.
	if (!AShapeSet::Find(GetWorld(), PestiStart::BozsiHouseKey))
	{
		const FRotator Turn(0.0, BozsiHouseModel::CityYaw, 0.0);
		const FVector2D Pavement = BozsiHouseModel::CityAt + FVector2D(Turn.RotateVector(FVector(-150.0, 0.0, 0.0)));
		TArray<MarketModel::FShape> Shapes;
		TArray<BozsiHouseModel::FLamp> Lamps;
		TArray<BozsiHouseModel::FSign> Signs;
		BozsiHouseModel::Build(Shapes, Lamps);
		BozsiHouseModel::Signs(Signs);
		PestiStart::PutUpShapeSet(GetWorld(), PestiStart::BozsiHouseKey, FVector(BozsiHouseModel::CityAt, GroundZ(Pavement)), BozsiHouseModel::CityYaw, Shapes, Lamps, Signs);
	}

	// And Friedmann's furrier's shop on Rákóczi út (mission 8), its ground height taken out on the pavement.
	if (!AShapeSet::Find(GetWorld(), PestiStart::FurrierKey))
	{
		const FRotator Turn(0.0, FurrierModel::CityYaw, 0.0);
		const FVector2D Pavement = FurrierModel::CityAt + FVector2D(Turn.RotateVector(FVector(400.0, 0.0, 0.0)));
		TArray<MarketModel::FShape> Shapes;
		TArray<BozsiHouseModel::FLamp> Lamps;
		TArray<BozsiHouseModel::FSign> Signs;
		FurrierModel::Build(Shapes, Lamps);
		FurrierModel::Signs(Signs);
		PestiStart::PutUpShapeSet(GetWorld(), PestiStart::FurrierKey, FVector(FurrierModel::CityAt, GroundZ(Pavement)), FurrierModel::CityYaw, Shapes, Lamps, Signs);
	}

	// And the street door on Tisza Kálmán tér that the raid's escape comes out of, its ground height taken in front of it.
	if (!AShapeSet::Find(GetWorld(), PestiStart::ExitDoorKey))
	{
		TArray<MarketModel::FShape> Shapes;
		TArray<BozsiHouseModel::FLamp> Lamps;
		BozsiHouseModel::StreetDoor(Shapes, Lamps, true);
		PestiStart::PutUpShapeSet(GetWorld(), PestiStart::ExitDoorKey, FVector(BozsiHouseModel::ExitDoorAt, GroundZ(BozsiHouseModel::ExitAt)),
			BozsiHouseModel::ExitDoorYaw, Shapes, Lamps, TArray<BozsiHouseModel::FSign>());
	}

	// And the eagle that circles over Keleti.
	if (!TActorIterator<AKeletiEagle>(GetWorld()))
	{
		const FVector2D At = AKeletiStation::FrontAt();
		const double Z = GroundZ(At - FVector2D(800.0, 0.0));
		GetWorld()->SpawnActor<AKeletiEagle>(AKeletiEagle::StaticClass(), FVector(At, Z), FRotator::ZeroRotator);
	}

	// The cats and dogs of the yards and squares (they settle in once the yards above are up).
	if (!TActorIterator<AAnimals>(GetWorld()))
	{
		GetWorld()->SpawnActor<AAnimals>();
	}

	// Smoke from the chimneys, steam from the engines at Keleti.
	if (!TActorIterator<AChimneySmoke>(GetWorld()))
	{
		GetWorld()->SpawnActor<AChimneySmoke>();
	}

	// The weather: clear, cloud, rain and fog by turns.
	AWeather::Get(GetWorld());
}

FVector2D APestiAlvilagGameModeBase::HomeDoorSpot() const
{
	return IsMissionDone(PestiStart::Bunda) ? BozsiHouseModel::AtticDoorAt : PestiStart::HomeDoorAt;
}

double APestiAlvilagGameModeBase::HomeDoorFacing() const
{
	return IsMissionDone(PestiStart::Bunda) ? BozsiHouseModel::AtticDoorYaw : static_cast<double>(PestiStart::HomeDoorYaw);
}

FVector2D APestiAlvilagGameModeBase::HomeStartAt() const
{
	const FVector2D Out = FVector2D(FRotator(0.0, HomeDoorFacing(), 0.0).Vector()) * 150.0;
	return HomeDoorSpot() + Out;
}

void APestiAlvilagGameModeBase::PlaceHomeDoor()
{
	const FVector2D DoorAt = HomeDoorSpot();
	for (TActorIterator<AHomeDoor> It(GetWorld()); It; ++It)
	{
		It->SetActorLocationAndRotation(FVector(DoorAt, GroundZ(DoorAt)), FRotator(0.0, HomeDoorFacing(), 0.0));
	}

	// A plain door stands at the other one, so the attic has its door before
	// the end of act 1 and Bözsi's street door is not left a bare wall after.
	if (AShapeSet* OldSpare = AShapeSet::Find(GetWorld(), PestiStart::SpareDoorKey))
	{
		OldSpare->Destroy();
	}
	const bool bInAttic = IsMissionDone(PestiStart::Bunda);
	const FVector2D SpareAt = bInAttic ? PestiStart::HomeDoorAt : BozsiHouseModel::AtticDoorAt;
	const double SpareYaw = bInAttic ? static_cast<double>(PestiStart::HomeDoorYaw) : BozsiHouseModel::AtticDoorYaw;
	TArray<MarketModel::FShape> SpareShapes;
	TArray<BozsiHouseModel::FLamp> SpareLamps;
	BozsiHouseModel::StreetDoor(SpareShapes, SpareLamps, false);
	PestiStart::PutUpShapeSet(GetWorld(), PestiStart::SpareDoorKey, FVector(SpareAt, GroundZ(SpareAt)), SpareYaw,
		SpareShapes, SpareLamps, TArray<BozsiHouseModel::FSign>());
}

double APestiAlvilagGameModeBase::GroundZ(const FVector2D& At) const
{
	FHitResult Hit;
	const FVector Top(At, 10000.0);
	const FVector Bottom(At, -10000.0);
	return GetWorld()->LineTraceSingleByChannel(Hit, Top, Bottom, ECC_Visibility) ? Hit.ImpactPoint.Z : 0.0;
}

void APestiAlvilagGameModeBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (PC && PC->WasInputKeyJustPressed(EKeys::P))
	{
		if (WantedLevel > 0)
		{
			ClearWanted(TEXT("Police called off (P)"));
		}
		else
		{
			ReportCrime(1);
		}
	}

	// K moves the clock on an hour, to see the evening without waiting for it.
	if (PC && PC->WasInputKeyJustPressed(EKeys::K))
	{
		if (ADayNight* Clock = ADayNight::Get(GetWorld()))
		{
			Clock->SkipHour();
		}
	}

	if (PC && PC->WasInputKeyJustPressed(EKeys::F9))
	{
		StartNewGame();
		return;
	}

	UpdateWanted(DeltaSeconds);
	UpdateFaint();
	TickPlaceAtRing();

	if (NextMissionAt > 0.0 && GetWorld()->GetTimeSeconds() >= NextMissionAt)
	{
		NextMissionAt = -1.0;
		SpawnNextMission();
	}
}

void APestiAlvilagGameModeBase::HurtPlayer(float Amount)
{
	if (FaintUntil < 0.0)
	{
		Health = FMath::Max(0.f, Health - Amount);
	}
}

void APestiAlvilagGameModeBase::HealPlayer(float Amount)
{
	Health = FMath::Min(MaxHealth, Health + Amount);
}

void APestiAlvilagGameModeBase::UpdateFaint()
{
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	const double Now = GetWorld()->GetTimeSeconds();
	if (FaintUntil < 0.0)
	{
		if (Health > 0.f || !PC)
		{
			return;
		}
		// Down he goes: the picture darkens and the police lose interest.
		FaintUntil = Now + PestiStart::FaintDark;
		PC->SetIgnoreMoveInput(true);
		if (PC->PlayerCameraManager)
		{
			PC->PlayerCameraManager->StartCameraFade(0.f, 1.f, PestiStart::FaintFade, FLinearColor::Black, false, true);
		}
		WantedLevel = 0;
		PoliceCalledUntil = -1.0;
		TimeUnseen = 0.f;
		ArrestProgress = 0.f;
		return;
	}
	if (Now < FaintUntil)
	{
		return;
	}

	// He comes round at home, or on the platform before he has a home.
	FaintUntil = -1.0;
	Health = MaxHealth;
	const bool bHome = IsMissionDone(PestiStart::Prologue);
	const FVector2D WakeAt = bHome ? HomeStartAt() : AMissionPrologue::StartAt();
	const double WakeYaw = bHome ? HomeDoorFacing() : AMissionPrologue::StartYaw();
	if (PC)
	{
		if (APestiPerson* Person = Cast<APestiPerson>(PC->GetPawn()))
		{
			Person->SetActorLocationAndRotation(FVector(WakeAt, GroundZ(WakeAt) + 100.0), FRotator(0.0, WakeYaw, 0.0), false, nullptr, ETeleportType::TeleportPhysics);
			PC->SetControlRotation(FRotator(-10.0, WakeYaw, 0.0));
		}
		PC->SetIgnoreMoveInput(false);
		if (PC->PlayerCameraManager)
		{
			PC->PlayerCameraManager->StartCameraFade(1.f, 0.f, PestiStart::FaintFade, FLinearColor::Black);
		}
	}
	if (APestiHUD* Hud = APestiHUD::Get(this))
	{
		if (bHome)
		{
			AddMoney(-PestiStart::DoctorFee);
			Hud->Announce(TEXT("EL\u00c1JULT\u00c1L"), IsMissionDone(PestiStart::Bunda)
				? TEXT("You fainted. You came round in your room, the doctor's bill on the table: \u22121 peng\u0151")
				: TEXT("You fainted. B\u00f6zsi n\u00e9ni fetched the doctor: \u22121 peng\u0151"), 6.f);
		}
		else
		{
			Hud->Announce(TEXT("EL\u00c1JULT\u00c1L"), TEXT("You fainted. A porter helped you back to your feet."), 6.f);
		}
	}
}

void APestiAlvilagGameModeBase::ReportCrime(int32 CrimeLevel)
{
	if (WantedLevel == 0)
	{
		APestiHUD::Show(this, 3, 4.f, FColor::Red, TEXT("The police are after you!"));
	}
	WantedLevel = FMath::Clamp(FMath::Max(WantedLevel, CrimeLevel), 0, PestiStart::MaxWantedLevel);
	TimeUnseen = 0.f;
}

void APestiAlvilagGameModeBase::NotifySeenByPolice()
{
	TimeUnseen = 0.f;
}

void APestiAlvilagGameModeBase::NotifyPoliceNear()
{
	LastPoliceNearTime = GetWorld()->GetTimeSeconds();
}

void APestiAlvilagGameModeBase::CallInPolice()
{
	if (WantedLevel == 0)
	{
		return;
	}
	const bool bAlready = IsPoliceCalledIn();
	PoliceCalledUntil = GetWorld()->GetTimeSeconds() + CalledInTime;
	TimeUnseen = 0.f;
	if (!bAlready)
	{
		WantedLevel = FMath::Min(WantedLevel + 1, PestiStart::MaxWantedLevel);
		APestiHUD::Show(this, 3, 5.f, FColor::Red, TEXT("A constable telephoned the station: police cars are coming!"));
	}
}

bool APestiAlvilagGameModeBase::IsPoliceCalledIn() const
{
	return WantedLevel > 0 && PoliceCalledUntil >= 0.0 && GetWorld()->GetTimeSeconds() < PoliceCalledUntil;
}

void APestiAlvilagGameModeBase::UpdateWanted(float DeltaSeconds)
{
	if (WantedLevel == 0)
	{
		return;
	}

	TimeUnseen += DeltaSeconds;
	if (TimeUnseen > EscapeTime)
	{
		ClearWanted(TEXT("You lost the police!"));
		return;
	}

	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	const bool bPoliceNear = GetWorld()->GetTimeSeconds() - LastPoliceNearTime < 0.2;
	if (Player && bPoliceNear && Player->GetVelocity().Size2D() < PestiStart::CaughtSpeed)
	{
		ArrestProgress += DeltaSeconds;
		if (ArrestProgress > ArrestTime)
		{
			if (APestiCar* Car = Cast<APestiCar>(UGameplayStatics::GetPlayerPawn(this, 0)))
			{
				Car->ForwardSpeed = 0.f;
			}
			++Arrests;
			ClearWanted(TEXT("BUSTED! The police caught you."));
		}
	}
	else
	{
		ArrestProgress = FMath::Max(0.f, ArrestProgress - DeltaSeconds);
	}
}

void APestiAlvilagGameModeBase::ClearWanted(const TCHAR* Message)
{
	WantedLevel = 0;
	PoliceCalledUntil = -1.0;
	TimeUnseen = 0.f;
	ArrestProgress = 0.f;
	APestiHUD::Show(this, 3, 4.f, FColor::Yellow, Message);
}

void APestiAlvilagGameModeBase::AddMoney(int32 Filler)
{
	Money = FMath::Max(0, Money + Filler);
}

FString APestiAlvilagGameModeBase::FormatMoney(int32 Filler)
{
	const int32 Sign = Filler < 0 ? -1 : 1;
	const int32 Abs = FMath::Abs(Filler);
	return FString::Printf(TEXT("%s%d,%02d peng\u0151"), Sign < 0 ? TEXT("-") : TEXT(""), Abs / 100, Abs % 100);
}

void APestiAlvilagGameModeBase::LoadSaveOnce()
{
	if (bSaveRead)
	{
		return;
	}
	bSaveRead = true;
	if (const UPestiSaveGame* Save = Cast<UPestiSaveGame>(UGameplayStatics::LoadGameFromSlot(UPestiSaveGame::SlotName(), 0)))
	{
		Money = Save->Money;
		CompletedMissions = Save->CompletedMissions;
		TaxiJobs = Save->TaxiJobs;
		TriedDishes = Save->TriedDishes;
		FoundLighters = Save->FoundLighters;
		bHasBaton = Save->bHasBaton;
		FightPoints = Save->FightPoints;
		Lessons = Save->Lessons;
		BoxingWins = Save->BoxingWins;
	}

	// A jump from the menu's mission list: the missions before it count as done.
	const int32 Jump = PestiStart::PendingJump;
	PestiStart::PendingJump = 0;
	if (Jump >= 1 && Jump <= static_cast<int32>(UE_ARRAY_COUNT(PestiStart::ActOne)))
	{
		CompletedMissions.Reset();
		for (int32 i = 0; i < Jump - 1; ++i)
		{
			CompletedMissions.Add(*PestiStart::ActOne[i]);
		}
		// Schwimmer's baton comes with mission 5.
		bHasBaton = Jump > 5;
		// Mission 1 starts at Keleti like a new game; the others at their ring.
		bPlaceAtRing = Jump > 1;
		PlaceAtRingUntil = -1.0;
	}
}

void APestiAlvilagGameModeBase::JumpToMission(UWorld* World, int32 Number)
{
	if (!World)
	{
		return;
	}
	PestiStart::PendingJump = Number;
	FPestiMenu::SkipTitleOnce();
	UGameplayStatics::OpenLevel(World, FName(*UGameplayStatics::GetCurrentLevelName(World)));
}

void APestiAlvilagGameModeBase::TickPlaceAtRing()
{
	if (!bPlaceAtRing)
	{
		return;
	}
	const double Now = GetWorld()->GetTimeSeconds();
	if (PlaceAtRingUntil < 0.0)
	{
		PlaceAtRingUntil = Now + PestiStart::RingWait;
	}
	if (Now > PlaceAtRingUntil)
	{
		// No ring came up: leave him at the home door.
		bPlaceAtRing = false;
		return;
	}
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	APestiPerson* Person = PC ? Cast<APestiPerson>(PC->GetPawn()) : nullptr;
	if (!Person)
	{
		return;
	}
	for (TActorIterator<AMission> It(GetWorld()); It; ++It)
	{
		FVector RingAt;
		FVector Away;
		if (!It->GetStartSpot(RingAt, Away))
		{
			continue;
		}
		bPlaceAtRing = false;
		const FVector2D StandAt(RingAt + Away * (UStartRingComponent::Radius() + PestiStart::RingStandOff));
		const double FaceYaw = (-Away).Rotation().Yaw;
		const FVector At(StandAt, GroundZ(StandAt) + 100.0);
		Person->SetActorLocationAndRotation(At, FRotator(0.0, FaceYaw, 0.0), false, nullptr, ETeleportType::TeleportPhysics);
		Person->GetCharacterMovement()->StopMovementImmediately();
		PC->SetControlRotation(FRotator(0.0, FaceYaw, 0.0));
		return;
	}
}

bool APestiAlvilagGameModeBase::HasBaton()
{
	LoadSaveOnce();
	// Games that passed mission 5 before the baton was in it have it too.
	return bHasBaton || IsMissionDone(PestiStart::Felmondas);
}

void APestiAlvilagGameModeBase::GiveBaton()
{
	bHasBaton = true;
	if (APestiPerson* Person = Cast<APestiPerson>(UGameplayStatics::GetPlayerPawn(this, 0)))
	{
		Person->RefreshDress();
	}
}

void APestiAlvilagGameModeBase::AddFightPoints(int32 Points)
{
	if (Points <= 0)
	{
		return;
	}
	FightPoints += Points;
	// The note counts up through one fight; a new fight starts it again.
	const double Now = GetWorld()->GetTimeSeconds();
	if (Now - RecentPointsAt > PestiStart::PointsNoteGap)
	{
		RecentPoints = 0;
	}
	RecentPoints += Points;
	RecentPointsAt = Now;
	APestiHUD::Show(this, PestiStart::PointsNoteKey, PestiStart::PointsNoteSeconds, FColor(255, 210, 60),
		FString::Printf(TEXT("+%d BUNY\u00d3PONT  (+%d fight points, %d in all)"), RecentPoints, RecentPoints, FightPoints));
}

bool APestiAlvilagGameModeBase::LearnLesson(int32 Lesson, int32 Cost)
{
	if (Lesson < 0 || Lesson >= 32 || KnowsLesson(Lesson) || FightPoints < Cost)
	{
		return false;
	}
	FightPoints -= Cost;
	Lessons |= 1 << Lesson;
	return true;
}

void APestiAlvilagGameModeBase::MarkMissionDone(FName MissionId)
{
	CompletedMissions.AddUnique(MissionId);
	// After the raid the player's home is the attic room.
	if (MissionId == PestiStart::Bunda)
	{
		PlaceHomeDoor();
	}
}

bool APestiAlvilagGameModeBase::CanSaveAtHome() const
{
	return IsMissionDone(PestiStart::Prologue);
}

bool APestiAlvilagGameModeBase::SaveToDisk() const
{
	UPestiSaveGame* Save = Cast<UPestiSaveGame>(UGameplayStatics::CreateSaveGameObject(UPestiSaveGame::StaticClass()));
	if (!Save)
	{
		return false;
	}
	Save->Money = Money;
	Save->CompletedMissions = CompletedMissions;
	Save->TaxiJobs = TaxiJobs;
	Save->TriedDishes = TriedDishes;
	Save->FoundLighters = FoundLighters;
	Save->bHasBaton = bHasBaton;
	Save->FightPoints = FightPoints;
	Save->Lessons = Lessons;
	Save->BoxingWins = BoxingWins;
	return UGameplayStatics::SaveGameToSlot(Save, UPestiSaveGame::SlotName(), 0);
}

void APestiAlvilagGameModeBase::StartNewGame()
{
	// Straight into the new game, not the title screen again.
	FPestiMenu::SkipTitleOnce();
	UGameplayStatics::DeleteGameInSlot(UPestiSaveGame::SlotName(), 0);
	UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this)));
}
