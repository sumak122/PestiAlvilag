// Fill out your copyright notice in the Description page of Project Settings.

#include "Eateries.h"

#include "Cafe.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "DayNight.h"
#include "EateryModel.h"
#include "Engine/World.h"
#include "FoodMenus.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "PersonModel.h"
#include "PestiAlvilagGameModeBase.h"
#include "PestiHud.h"
#include "PestiPerson.h"
#include "ShapeSet.h"
#include "ShopModel.h"
#include "StoryPerson.h"
#include "Weather.h"

namespace EateryRooms
{
	using EateryModel::EEatery;

	/**
	 * Each place: its shop front (street_furniture.json, named by
	 * street_furniture.py EATERIES: where, which way, how wide, its seed;
	 * Width 0 for the Nemzeti, whose At is the middle of the hotel's main front),
	 * its hours, and its name for the prompts.
	 */
	struct FPlace
	{
		FVector2D At;
		double Yaw;
		double Width;
		uint32 Seed;
		float Opens;
		float Closes;
		const TCHAR* Name;
		const TCHAR* English;
	};
	const FPlace Places[EateryModel::NumEateries] = {
		{ FVector2D(6223.0, -10608.0), 68.5, 975.0, 41595195u, 9.f, 21.f, TEXT("Hauer cukrászda"), TEXT("Hauer pastry shop") },
		{ FVector2D(23441.0, 12109.0), -71.2, 980.0, 422841658u, 7.f, 21.f, TEXT("Kifőzde"), TEXT("Cook-shop") },
		{ FVector2D(20151.0, 62677.0), -105.0, 649.0, 139531542u, 6.f, 20.f, TEXT("Központi Tejcsarnok"), TEXT("Milk bar") },
		{ FVector2D(53960.0, 28792.0), 152.7, 743.0, 1482426672u, 6.f, 24.f, TEXT("Fuvaros vendéglő"), TEXT("Carters' inn") },
		{ FVector2D(95498.0, 39753.0), 145.7, 616.0, 366836713u, 14.f, 2.f, TEXT("Borozó"), TEXT("Wine cellar") },
		{ FVector2D(-9179.0, -8782.0), -42.5, 643.0, 1852685215u, 12.f, 3.f, TEXT("Kispipa vendéglő"), TEXT("Kispipa inn") },
		{ FVector2D(77823.0, -31762.0), -89.5, 734.0, 1664692439u, 5.f, 1.f, TEXT("Állóbüfé"), TEXT("Standing buffet") },
		{ FVector2D(3389.0, 32870.0), -3.5, 320.0, 1243200910u, 8.f, 21.f, TEXT("Kóser étkezde"), TEXT("Kosher eating house") },
		// The Nemzeti's main front: from [1136, 4405], yaw -3.9, 3876 long (city_details.json "hotels", HotelModel::MainFront).
		{ FVector2D(1136.0, 4405.0) + FVector2D(FRotator(0.0, -3.9, 0.0).RotateVector(FVector(0.0, 1938.0, 0.0))), -3.9, 0.0, 0u, 12.f, 1.f, TEXT("Nemzeti étterem"), TEXT("Hotel Nemzeti restaurant") },
	};
	/** How far out from the hotel wall the Nemzeti's door spot is (cm). */
	constexpr double HotelOut = 190.0;
	/** Where the rooms are built, far out west of the city (beyond the bars and the test grounds), one every this far. */
	const FVector FirstRoom(-380000.0, -40000.0, 0.0);
	constexpr double RoomStep = 10000.0;
	constexpr double OutFront = 70.0;
	constexpr double InFront = 150.0;
	constexpr double DoorReach = 190.0;
	constexpr float Fade = 0.8f;
	constexpr double LostRoom = 4000.0;
	/** Share of the seats and standing places taken. */
	constexpr float Taken = 0.55f;
	const FLinearColor LampLight(1.f, 0.80f, 0.56f);
	constexpr float LampCandela = 70.f;
	constexpr float LampRadius = 900.f;
	constexpr float FillCandela = 30.f;
	constexpr int32 NoteKey = 62;

	EEatery KindOf(int32 Place) { return static_cast<EEatery>(Place); }
	uint8 MenuOf(int32 Place) { return static_cast<uint8>(static_cast<int32>(EFoodMenu::Hauer) + Place); }
}

AEateries::AEateries()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;
}

void AEateries::BeginPlay()
{
	Super::BeginPlay();

	for (int32 Place = 0; Place < EateryModel::NumEateries; ++Place)
	{
		const EateryRooms::FPlace& Front = EateryRooms::Places[Place];
		const FRotator Facing(0.0, Front.Yaw, 0.0);
		const FVector Flat = Front.Width > 0.0
			? FVector(Front.At, 0.0) + Facing.RotateVector(FVector(ShopModel::FrontX + ShopModel::StandOff + EateryRooms::OutFront, ShopModel::DoorMiddle(Front.Width, Front.Seed), 0.0))
			: FVector(Front.At, 0.0) + Facing.RotateVector(FVector(EateryRooms::HotelOut, 0.0, 0.0));
		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(EateryDoor), false, this);
		double Z = 0.0;
		if (GetWorld()->LineTraceSingleByChannel(Hit, Flat + FVector(0.0, 0.0, 3000.0), Flat - FVector(0.0, 0.0, 3000.0), ECC_Visibility, Params))
		{
			Z = Hit.ImpactPoint.Z;
		}
		StreetDoor.Add(FVector(Flat.X, Flat.Y, Z));
		StreetYaw.Add(Front.Yaw);
		RoomAt.Add(EateryRooms::FirstRoom + FVector(0.0, Place * EateryRooms::RoomStep, 0.0));
		CounterAt.Add(FVector::ZeroVector);
		BuildRoom(Place);
	}
	SetRoomShown(INDEX_NONE);
}

void AEateries::BuildRoom(int32 Place)
{
	using namespace EateryModel;
	const EEatery Kind = EateryRooms::KindOf(Place);
	const FVector At = RoomAt[Place];
	TArray<FShape> Shapes;
	FLayout L;
	Build(Kind, Shapes, FVector::ZeroVector, 0.0, L);
	if (AShapeSet* Set = GetWorld()->SpawnActor<AShapeSet>(AShapeSet::StaticClass(), At, FRotator::ZeroRotator))
	{
		Set->AddShapes(Shapes);
		Set->AddSign(SignOf(Kind), L.SignAt, 180.0, L.SignHeight, L.SignColor);
	}
	CounterAt[Place] = At + L.OrderAt;

	// The lamps burn day and night in here, and a soft light fills the room.
	L.Lamps.Add(FVector(L.Depth * 0.5, 0.0, L.Height - 60.0));
	for (int32 i = 0; i < L.Lamps.Num(); ++i)
	{
		UPointLightComponent* Light = NewObject<UPointLightComponent>(this);
		Light->SetupAttachment(Root);
		Light->SetMobility(EComponentMobility::Movable);
		Light->SetWorldLocation(At + L.Lamps[i] - FVector(0.0, 0.0, 15.0));
		Light->SetCastShadows(false);
		Light->SetIntensityUnits(ELightUnits::Candelas);
		const bool bFill = i == L.Lamps.Num() - 1;
		Light->SetIntensity(bFill ? EateryRooms::FillCandela : EateryRooms::LampCandela);
		Light->SetAttenuationRadius(bFill ? EateryRooms::LampRadius * 2.f : EateryRooms::LampRadius);
		Light->SetLightColor(EateryRooms::LampLight);
		Light->RegisterComponent();
		AddInstanceComponent(Light);
		Lights.Add(Light);
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	FRandomStream Random(1932 + Place * 31);
	auto Keep = [this, Place](AActor* Who)
	{
		if (Who)
		{
			People.Add(Who);
			PeopleRoom.Add(Place);
		}
	};

	// Guests on the chairs and benches, facing their tables; hats off indoors, for the men.
	for (const FSpot& Seat : L.Seats)
	{
		if (Random.FRand() > EateryRooms::Taken)
		{
			continue;
		}
		ACafeGuest* Guest = GetWorld()->SpawnActor<ACafeGuest>(ACafeGuest::StaticClass(), At + Seat.At + FVector(0.0, 0.0, PersonModel::SeatedMiddle), FRotator(0.0, Seat.Yaw, 0.0), Params);
		if (Guest)
		{
			PersonKit::FOutfit Clothes = PersonModel::RandomOutfit(Random);
			Clothes.bSeated = true;
			Clothes.Held = PersonKit::EHeld::None;
			if (!Clothes.bWoman)
			{
				Clothes.Hat = PersonKit::EHat::None;
			}
			Guest->SetOutfit(Clothes);
			Keep(Guest);
		}
	}
	// People standing at the high tables, the shelf or the counter (hats on: they won't stay long).
	for (const FSpot& Spot : L.Standers)
	{
		if (Random.FRand() > EateryRooms::Taken + 0.15f)
		{
			continue;
		}
		AStoryPerson* Stander = GetWorld()->SpawnActor<AStoryPerson>(AStoryPerson::StaticClass(), At + Spot.At + FVector(0.0, 0.0, PersonModel::MiddleHeight), FRotator(0.0, Spot.Yaw, 0.0), Params);
		if (Stander)
		{
			PersonKit::FOutfit Clothes = PersonModel::RandomOutfit(Random);
			Clothes.Held = PersonKit::EHeld::None;
			Stander->SetOutfit(Clothes);
			Keep(Stander);
		}
	}
	// The staff behind the counter.
	for (int32 k = 0; k < 2; ++k)
	{
		if (k == 1 && !L.bSecondStaff)
		{
			break;
		}
		const FSpot& Spot = k == 0 ? L.Staff : L.SecondStaff;
		AStoryPerson* Staff = GetWorld()->SpawnActor<AStoryPerson>(AStoryPerson::StaticClass(), At + Spot.At + FVector(0.0, 0.0, PersonModel::MiddleHeight), FRotator(0.0, Spot.Yaw, 0.0), Params);
		if (Staff)
		{
			Staff->SetOutfit(StaffLook(Kind, k == 1));
			Keep(Staff);
		}
	}
	// The waiter round the tables, bringing this place's menu.
	if (L.bWaiter)
	{
		const FVector From = At + L.WaiterFrom;
		TArray<FVector> Stops;
		for (const FVector& Stop : L.TableStops)
		{
			Stops.Add(At + Stop);
		}
		if (ACafeWaiter* Waiter = GetWorld()->SpawnActor<ACafeWaiter>(ACafeWaiter::StaticClass(), From + FVector(0.0, 0.0, PersonModel::MiddleHeight), FRotator::ZeroRotator, Params))
		{
			Waiter->SetRounds(From, Stops);
			Waiter->SetMenu(EateryRooms::MenuOf(Place));
			if (Kind == EEatery::Nemzeti)
			{
				// In tails at the Nemzeti.
				Waiter->SetCoatColor(FLinearColor(0.02f, 0.02f, 0.025f));
			}
			Keep(Waiter);
		}
	}
	// The pianist at the Kispipa, hands on the keys.
	if (L.bPiano)
	{
		if (ACafeGuest* Pianist = GetWorld()->SpawnActor<ACafeGuest>(ACafeGuest::StaticClass(), At + L.Pianist.At + FVector(0.0, 0.0, PersonModel::SeatedMiddle), FRotator(0.0, L.Pianist.Yaw, 0.0), Params))
		{
			Pianist->SetOutfit(PianistLook());
			Keep(Pianist);
		}
	}
}

void AEateries::SetRoomShown(int32 Place)
{
	// Only the room the player is in has its people drawn and moving.
	for (int32 i = 0; i < People.Num(); ++i)
	{
		if (AActor* Who = People[i])
		{
			const bool bShow = PeopleRoom[i] == Place;
			Who->SetActorHiddenInGame(!bShow);
			Who->SetActorTickEnabled(bShow);
		}
	}
}

void AEateries::GetCounters(TArray<FVector>& OutAt, TArray<uint8>& OutMenu) const
{
	for (int32 Place = 0; Place < CounterAt.Num(); ++Place)
	{
		OutAt.Add(CounterAt[Place]);
		OutMenu.Add(EateryRooms::MenuOf(Place));
	}
}

bool AEateries::IsOpen(int32 Place) const
{
	const ADayNight* Sky = ADayNight::Get(GetWorld());
	const float Hour = Sky ? Sky->GetHour() : 12.f;
	const EateryRooms::FPlace& P = EateryRooms::Places[Place];
	// Hours past midnight wrap round (the cellar shuts at 2).
	return P.Opens < P.Closes ? (Hour >= P.Opens && Hour < P.Closes) : (Hour >= P.Opens || Hour < P.Closes);
}

void AEateries::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	// The places on the maps, once there is a HUD to tell.
	if (!bIconsSent)
	{
		if (APestiHUD* Hud = APestiHUD::Get(this))
		{
			for (const FVector& Door : StreetDoor)
			{
				APestiHUD::FMapIcon Icon;
				Icon.At = FVector2D(Door.X, Door.Y);
				Icon.Kind = APestiHUD::EMapIcon::Food;
				Hud->AddMapIcon(Icon);
			}
			bIconsSent = true;
		}
	}
	TickDoors();
}

void AEateries::TickDoors()
{
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	APestiPerson* Person = PC ? Cast<APestiPerson>(PC->GetPawn()) : nullptr;
	if (!Person)
	{
		return;
	}
	const FVector Here = Person->GetActorLocation();

	if (Inside != INDEX_NONE)
	{
		// Taken out some other way (busted, a mission, the test grounds): he is outdoors again.
		if (FVector::Dist2D(Here, RoomAt[Inside]) > EateryRooms::LostRoom)
		{
			Inside = INDEX_NONE;
			SetRoomShown(INDEX_NONE);
			if (AWeather* Rain = AWeather::Get(GetWorld()))
			{
				Rain->SetIndoors(false);
			}
			return;
		}
		if (FVector::Dist2D(Here, RoomAt[Inside]) < EateryRooms::DoorReach && !Person->IsFighting())
		{
			if (APestiHUD* Hud = APestiHUD::Get(this))
			{
				Hud->Prompt(TEXT("E"), TEXT("Ki az utcára"), TEXT("Out to the street"), false);
			}
			if (PC->WasInputKeyJustPressed(EKeys::E))
			{
				GoOut();
			}
		}
		return;
	}

	if (Person->IsRiding() || Person->IsFighting())
	{
		return;
	}
	for (int32 Place = 0; Place < StreetDoor.Num(); ++Place)
	{
		if (FVector::Dist2D(Here, StreetDoor[Place]) > EateryRooms::DoorReach || FMath::Abs(Here.Z - StreetDoor[Place].Z) > 300.0)
		{
			continue;
		}
		const EateryRooms::FPlace& P = EateryRooms::Places[Place];
		if (APestiHUD* Hud = APestiHUD::Get(this))
		{
			Hud->Prompt(TEXT("E"), FString::Printf(TEXT("Be: %s"), P.Name), FString::Printf(TEXT("Go in: %s"), P.English), false);
		}
		if (!PC->WasInputKeyJustPressed(EKeys::E))
		{
			return;
		}
		const APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this);
		if (Game && Game->GetWantedLevel() > 0)
		{
			APestiHUD::Show(this, EateryRooms::NoteKey, 4.f, FColor(255, 200, 60),
				TEXT("A rendőrség a nyomodban van, most nem mehetsz be.  (The police are after you; you can't go in now.)"));
		}
		else if (!IsOpen(Place))
		{
			const int32 Opens = FMath::RoundToInt(P.Opens);
			const int32 Closes = FMath::RoundToInt(P.Closes) % 24;
			APestiHUD::Show(this, EateryRooms::NoteKey, 4.f, FColor(255, 200, 60),
				FString::Printf(TEXT("Zárva. Nyitva %d órától %d óráig.  (Closed. Open from %d:00 to %d:00.)"), Opens, Closes, Opens, Closes));
		}
		else if (EateryRooms::KindOf(Place) == EateryModel::EEatery::Nemzeti && !(Game && Game->HasSuit()))
		{
			APestiHUD::Show(this, EateryRooms::NoteKey, 4.f, FColor(255, 200, 60),
				TEXT("„Bocsánat, uram, öltöny nélkül nem lehet.”  (Beg pardon, sir, not without a suit.)"));
		}
		else
		{
			GoIn(Place);
		}
		return;
	}
}

void AEateries::GoIn(int32 Place)
{
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	APestiPerson* Person = PC ? Cast<APestiPerson>(PC->GetPawn()) : nullptr;
	if (!Person)
	{
		return;
	}
	SetRoomShown(Place);
	const FVector Spot = RoomAt[Place] + FVector(EateryRooms::InFront, 0.0, PersonModel::MiddleHeight + 10.0);
	Person->SetActorLocationAndRotation(Spot, FRotator::ZeroRotator, false, nullptr, ETeleportType::TeleportPhysics);
	Person->GetCharacterMovement()->StopMovementImmediately();
	PC->SetControlRotation(FRotator(-10.0, 0.0, 0.0));
	if (PC->PlayerCameraManager)
	{
		PC->PlayerCameraManager->StartCameraFade(1.f, 0.f, EateryRooms::Fade, FLinearColor::Black);
	}
	Inside = Place;
	if (AWeather* Rain = AWeather::Get(GetWorld()))
	{
		Rain->SetIndoors(true);
	}
}

void AEateries::GoOut()
{
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	APestiPerson* Person = PC ? Cast<APestiPerson>(PC->GetPawn()) : nullptr;
	if (!Person || Inside == INDEX_NONE)
	{
		return;
	}
	const int32 Place = Inside;
	Inside = INDEX_NONE;
	SetRoomShown(INDEX_NONE);
	const FVector Spot = StreetDoor[Place] + FVector(0.0, 0.0, PersonModel::MiddleHeight + 10.0);
	Person->SetActorLocationAndRotation(Spot, FRotator(0.0, StreetYaw[Place], 0.0), false, nullptr, ETeleportType::TeleportPhysics);
	Person->GetCharacterMovement()->StopMovementImmediately();
	PC->SetControlRotation(FRotator(-10.0, StreetYaw[Place], 0.0));
	if (PC->PlayerCameraManager)
	{
		PC->PlayerCameraManager->StartCameraFade(1.f, 0.f, EateryRooms::Fade, FLinearColor::Black);
	}
	if (AWeather* Rain = AWeather::Get(GetWorld()))
	{
		Rain->SetIndoors(false);
	}
}
