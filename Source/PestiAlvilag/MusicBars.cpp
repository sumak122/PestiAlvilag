// Fill out your copyright notice in the Description page of Project Settings.

#include "MusicBars.h"

#include "BandPlayer.h"
#include "BandSound.h"
#include "Cafe.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "DayNight.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Mission.h"
#include "MissionDzsokiUdvarlo.h"
#include "MissionDzsokiUtok.h"
#include "MusicBarModel.h"
#include "PersonModel.h"
#include "PestiAlvilagGameModeBase.h"
#include "PestiHud.h"
#include "PestiPerson.h"
#include "ShapeSet.h"
#include "ShopModel.h"
#include "StoryPerson.h"
#include "Weather.h"

namespace BarRooms
{
	/** The two shop fronts that are the bars (street_furniture.json, renamed by street_furniture.py MUSIC_BARS): where, which way, how wide, its seed. */
	struct FFront
	{
		FVector2D At;
		double Yaw;
		double Width;
		uint32 Seed;
	};
	const FFront Fronts[2] = {
		{ FVector2D(21478.0, 34089.0), 70.5, 659.0, 816097267u },
		{ FVector2D(5026.0, 9194.0), 175.6, 949.0, 957486227u },
	};
	/** Where the rooms are built, far out west of the city (and away from the test grounds). */
	const FVector RoomPlaces[2] = { FVector(-330000.0, -20000.0, 0.0), FVector(-330000.0, 20000.0, 0.0) };
	/** How far out in front of the shop's door the player stands, outside and inside (cm). */
	constexpr double OutFront = 70.0;
	constexpr double InFront = 150.0;
	/** E works this close to a door (cm). */
	constexpr double DoorReach = 190.0;
	/** The black between outside and inside (s). */
	constexpr float Fade = 0.8f;
	/** The band plays from this hour till that one. */
	constexpr float OpensAt = 17.f;
	constexpr float ClosesAt = 3.f;
	/** Heard through the wall out to this far from the street door (cm), at most this loud. */
	constexpr double HeardOutside = 2500.0;
	constexpr float OutsideLoud = 0.45f;
	/** Further than this from his room while said to be inside, the player has been taken out some other way (cm). */
	constexpr double LostRoom = 4000.0;
	/** The lights in the rooms. */
	const FLinearColor LampLight(1.f, 0.78f, 0.52f);
	constexpr float LampCandela = 70.f;
	constexpr float LampRadius = 900.f;
	constexpr float FillCandela = 30.f;
	/** Once a second is enough for the missions. */
	constexpr float ChainEvery = 1.f;
	/** Note lines on screen. */
	constexpr int32 NoteKey = 61;
	const FName OpenedBy(TEXT("ElsoAuto"));

	const TCHAR* const Names[2] = { TEXT("Feh\u00E9r Galamb"), TEXT("R\u00F3zsa k\u00E1v\u00E9h\u00E1z") };
}

AMusicBars::AMusicBars()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;
}

AMusicBars* AMusicBars::Find(const UWorld* World)
{
	if (!World)
	{
		return nullptr;
	}
	TActorIterator<AMusicBars> It(const_cast<UWorld*>(World));
	return It ? *It : nullptr;
}

void AMusicBars::BeginPlay()
{
	Super::BeginPlay();

	for (int32 Bar = 0; Bar < 2; ++Bar)
	{
		// The pavement in front of the shop door.
		const BarRooms::FFront& Front = BarRooms::Fronts[Bar];
		const FRotator Facing(0.0, Front.Yaw, 0.0);
		const double DoorY = ShopModel::DoorMiddle(Front.Width, Front.Seed);
		const FVector Flat = FVector(Front.At, 0.0) + Facing.RotateVector(FVector(ShopModel::FrontX + ShopModel::StandOff + BarRooms::OutFront, DoorY, 0.0));
		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(MusicBarDoor), false, this);
		double Z = 0.0;
		if (GetWorld()->LineTraceSingleByChannel(Hit, Flat + FVector(0.0, 0.0, 3000.0), Flat - FVector(0.0, 0.0, 3000.0), ECC_Visibility, Params))
		{
			Z = Hit.ImpactPoint.Z;
		}
		StreetDoor[Bar] = FVector(Flat.X, Flat.Y, Z);
		StreetYaw[Bar] = Front.Yaw;
		RoomAt[Bar] = BarRooms::RoomPlaces[Bar];
		BuildRoom(Bar);

		UBandSound* Sound = NewObject<UBandSound>(this);
		Sound->SetupAttachment(Root);
		Sound->SetSmart(Bar == BarCafe);
		Sound->RegisterComponent();
		AddInstanceComponent(Sound);
		Sound->Start();
		Music.Add(Sound);
	}
}

void AMusicBars::BuildRoom(int32 Bar)
{
	using namespace MusicBarModel;
	const FVector At = RoomAt[Bar];
	AShapeSet* Set = GetWorld()->SpawnActor<AShapeSet>(AShapeSet::StaticClass(), At, FRotator::ZeroRotator);
	TArray<FShape> Shapes;
	TArray<FVector> Lamps;
	if (Bar == BarTavern)
	{
		BuildTavern(Shapes, FVector::ZeroVector, 0.0, true, &Lamps);
	}
	else
	{
		BuildCafeHall(Shapes, FVector::ZeroVector, 0.0, true, &Lamps);
	}
	if (Set)
	{
		Set->AddShapes(Shapes);
		if (Bar == BarTavern)
		{
			Set->AddSign(TEXT("FEH\u00C9R GALAMB"), TavernSignAt(), 180.0, 40.0, FLinearColor(0.92f, 0.86f, 0.62f));
		}
		else
		{
			Set->AddSign(TEXT("R\u00D3ZSA K\u00C1V\u00C9H\u00C1Z"), FVector(CafeDepth - 42.0, 0.0, 420.0), 180.0, 22.0, FLinearColor(0.20f, 0.12f, 0.04f));
		}
	}

	// The lamps burn day and night in here, and a soft light fills the room.
	const double Depth = Bar == BarTavern ? TavernDepth : CafeDepth;
	const double Height = Bar == BarTavern ? TavernHeight : CafeHeight;
	Lamps.Add(FVector(Depth * 0.5, 0.0, Height - 60.0));
	for (int32 i = 0; i < Lamps.Num(); ++i)
	{
		UPointLightComponent* Light = NewObject<UPointLightComponent>(this);
		Light->SetupAttachment(Root);
		Light->SetMobility(EComponentMobility::Movable);
		Light->SetWorldLocation(At + Lamps[i] - FVector(0.0, 0.0, 15.0));
		Light->SetCastShadows(false);
		Light->SetIntensityUnits(ELightUnits::Candelas);
		const bool bFill = i == Lamps.Num() - 1;
		Light->SetIntensity(bFill ? BarRooms::FillCandela : BarRooms::LampCandela);
		Light->SetAttenuationRadius(bFill ? BarRooms::LampRadius * 2.f : BarRooms::LampRadius);
		Light->SetLightColor(BarRooms::LampLight);
		Light->RegisterComponent();
		AddInstanceComponent(Light);
		Lights.Add(Light);
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	FRandomStream Random(Bar == BarTavern ? 1932 : 1911);

	// Guests at the tables, facing them, each on his chair.
	TArray<FVector2D> Tables;
	if (Bar == BarTavern)
	{
		Tables = { FVector2D(150.0, -330.0), FVector2D(150.0, -60.0), FVector2D(150.0, 210.0), FVector2D(380.0, -330.0), FVector2D(390.0, -60.0) };
	}
	else
	{
		for (int32 Row = 0; Row < 3; ++Row)
		{
			for (int32 Col = 0; Col < 5; ++Col)
			{
				const double X = 170.0 + Row * 290.0 + (Col % 2) * 60.0;
				const double Y = -560.0 + Col * 280.0;
				if (FMath::Abs(Y) < 120.0 && Row == 0)
				{
					continue;
				}
				Tables.Add(FVector2D(X, Y));
			}
		}
	}
	const int32 Chairs = Bar == BarTavern ? 4 : 3;
	const double ChairOut = Bar == BarTavern ? 62.0 : 55.0;
	TArray<FVector> Stands;
	for (int32 t = 0; t < Tables.Num(); ++t)
	{
		const FVector Table = At + FVector(Tables[t], 0.0);
		Stands.Add(Table + FVector(-70.0, 60.0, 0.0));
		for (int32 c = 0; c < Chairs; ++c)
		{
			// Not every chair is taken.
			if (Random.FRand() < 0.35f)
			{
				continue;
			}
			const double Turn = c * (360.0 / Chairs);
			const FVector Seat = Table + FRotator(0.0, Turn, 0.0).RotateVector(FVector(-ChairOut, 0.0, 0.0));
			if (ACafeGuest* Guest = GetWorld()->SpawnActor<ACafeGuest>(ACafeGuest::StaticClass(), Seat + FVector(0.0, 0.0, PersonModel::SeatedMiddle), FRotator(0.0, Turn, 0.0), Params))
			{
				PersonKit::FOutfit Clothes = PersonModel::RandomOutfit(Random);
				Clothes.bSeated = true;
				Clothes.Held = PersonKit::EHeld::None;
				// Hats off indoors, for the men.
				if (!Clothes.bWoman)
				{
					Clothes.Hat = PersonKit::EHat::None;
				}
				Guest->SetOutfit(Clothes);
			}
		}
	}
	// The waiter, from the counter (tavern) or the back of the hall (coffee house) round the tables.
	const FVector WaiterFrom = At + (Bar == BarTavern ? FVector(560.0, TavernHalfWidth - 200.0, 0.0) : FVector(1000.0, -650.0, 0.0));
	if (ACafeWaiter* Waiter = GetWorld()->SpawnActor<ACafeWaiter>(ACafeWaiter::StaticClass(), WaiterFrom + FVector(0.0, 0.0, PersonModel::MiddleHeight), FRotator::ZeroRotator, Params))
	{
		Waiter->SetRounds(WaiterFrom, Stands);
	}

	// The band on the stage.
	const TArray<FSpot> Spots = Bar == BarTavern ? TavernSpots() : CafeSpots();
	int32 Seed = Bar == BarTavern ? 0 : 10;
	for (const FSpot& Spot : Spots)
	{
		const FTransform Place(FRotator(0.0, Spot.Yaw, 0.0), At + Spot.At + FVector(0.0, 0.0, PersonModel::MiddleHeight));
		ABandPlayer* Player = GetWorld()->SpawnActorDeferred<ABandPlayer>(ABandPlayer::StaticClass(), Place, this, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!Player)
		{
			continue;
		}
		PersonKit::FOutfit Clothes;
		const bool bSmart = Bar == BarCafe;
		switch (Spot.Player)
		{
		case EPlayer::Cimbalom: Clothes = bSmart ? BandsmanLook(Seed + 7, true) : DzsokiLook(); break;
		case EPlayer::Fiddle: Clothes = bSmart ? GyusziLook() : BandsmanLook(Seed + 1, false); break;
		case EPlayer::Singer: Clothes = RozsiLook(); break;
		case EPlayer::Bass: Clothes = BandsmanLook(Seed + 4, bSmart); break;
		case EPlayer::Clarinet: Clothes = BandsmanLook(Seed + 2, bSmart); break;
		default: Clothes = BandsmanLook(Seed + 3, bSmart); break;
		}
		Player->SetPart(Spot.Player);
		Player->SetOutfit(Clothes);
		Player->FinishSpawning(Place);
		(Bar == BarTavern ? TavernBand : CafeBand).Add(Player);
		if (Bar == BarTavern && Spot.Player == EPlayer::Cimbalom)
		{
			StageDzsoki = Player;
		}
		if (Bar == BarTavern && Spot.Player == EPlayer::Singer)
		{
			StageRozsi = Player;
		}
	}

	// By day Dzsoki and Rózsi sit at the table by the stage.
	if (Bar == BarTavern)
	{
		const FVector Table = At + FVector(560.0, 120.0, 0.0);
		for (int32 k = 0; k < 2; ++k)
		{
			const double Turn = k == 0 ? 90.0 : 180.0;
			const FVector Seat = Table + FRotator(0.0, Turn, 0.0).RotateVector(FVector(-62.0, 0.0, 0.0));
			if (ACafeGuest* Sitter = GetWorld()->SpawnActor<ACafeGuest>(ACafeGuest::StaticClass(), Seat + FVector(0.0, 0.0, PersonModel::SeatedMiddle), FRotator(0.0, Turn, 0.0), Params))
			{
				PersonKit::FOutfit Clothes = k == 0 ? DzsokiLook() : RozsiLook();
				Clothes.bSeated = true;
				Sitter->SetOutfit(Clothes);
				DaySitters.Add(Sitter);
				if (k == 0)
				{
					TableDzsoki = Sitter;
				}
				else
				{
					TableRozsi = Sitter;
				}
			}
		}
	}
}

void AMusicBars::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	TickDoors();
	TickBands();
	ChainCheckIn -= DeltaSeconds;
	if (ChainCheckIn <= 0.f)
	{
		ChainCheckIn = BarRooms::ChainEvery;
		TickChain();
	}
}

void AMusicBars::TickDoors()
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
		// Taken out some other way (busted, a mission put him back, the test grounds): he is outdoors again.
		if (FVector::Dist2D(Here, RoomAt[Inside]) > BarRooms::LostRoom)
		{
			Inside = INDEX_NONE;
			if (AWeather* Rain = AWeather::Get(GetWorld()))
			{
				Rain->SetIndoors(false);
			}
			return;
		}
		if (FVector::Dist2D(Here, RoomAt[Inside]) < BarRooms::DoorReach && !Person->IsFighting())
		{
			if (APestiHUD* Hud = APestiHUD::Get(this))
			{
				Hud->Prompt(TEXT("E"), TEXT("Ki az utc\u00E1ra"), TEXT("Out to the street"), false);
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
	for (int32 Bar = 0; Bar < 2; ++Bar)
	{
		if (FVector::Dist2D(Here, StreetDoor[Bar]) > BarRooms::DoorReach || FMath::Abs(Here.Z - StreetDoor[Bar].Z) > 300.0)
		{
			continue;
		}
		APestiHUD* Hud = APestiHUD::Get(this);
		if (Hud)
		{
			Hud->Prompt(TEXT("E"), FString::Printf(TEXT("Be: %s"), BarRooms::Names[Bar]), FString::Printf(TEXT("Go in: %s"), BarRooms::Names[Bar]), false);
		}
		if (PC->WasInputKeyJustPressed(EKeys::E))
		{
			const APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this);
			if (Game && Game->GetWantedLevel() > 0)
			{
				APestiHUD::Show(this, BarRooms::NoteKey, 4.f, FColor(255, 200, 60),
					TEXT("A rend\u0151rs\u00E9g a nyomodban van, most nem mehetsz be.  (The police are after you; you can't go in now.)"));
			}
			else
			{
				GoIn(Bar);
			}
		}
		return;
	}
}

void AMusicBars::GoIn(int32 Bar)
{
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	APestiPerson* Person = PC ? Cast<APestiPerson>(PC->GetPawn()) : nullptr;
	if (!Person)
	{
		return;
	}
	const FVector Spot = RoomAt[Bar] + FVector(BarRooms::InFront, 0.0, PersonModel::MiddleHeight + 10.0);
	Person->SetActorLocationAndRotation(Spot, FRotator::ZeroRotator, false, nullptr, ETeleportType::TeleportPhysics);
	Person->GetCharacterMovement()->StopMovementImmediately();
	PC->SetControlRotation(FRotator(-10.0, 0.0, 0.0));
	if (PC->PlayerCameraManager)
	{
		PC->PlayerCameraManager->StartCameraFade(1.f, 0.f, BarRooms::Fade, FLinearColor::Black);
	}
	Inside = Bar;
	if (AWeather* Rain = AWeather::Get(GetWorld()))
	{
		Rain->SetIndoors(true);
	}
}

void AMusicBars::GoOut()
{
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	APestiPerson* Person = PC ? Cast<APestiPerson>(PC->GetPawn()) : nullptr;
	if (!Person || Inside == INDEX_NONE)
	{
		return;
	}
	const int32 Bar = Inside;
	Inside = INDEX_NONE;
	const FVector Spot = StreetDoor[Bar] + FVector(0.0, 0.0, PersonModel::MiddleHeight + 10.0);
	Person->SetActorLocationAndRotation(Spot, FRotator(0.0, StreetYaw[Bar], 0.0), false, nullptr, ETeleportType::TeleportPhysics);
	Person->GetCharacterMovement()->StopMovementImmediately();
	PC->SetControlRotation(FRotator(-10.0, StreetYaw[Bar], 0.0));
	if (PC->PlayerCameraManager)
	{
		PC->PlayerCameraManager->StartCameraFade(1.f, 0.f, BarRooms::Fade, FLinearColor::Black);
	}
	if (AWeather* Rain = AWeather::Get(GetWorld()))
	{
		Rain->SetIndoors(false);
	}
}

void AMusicBars::TickBands()
{
	const ADayNight* Sky = ADayNight::Get(GetWorld());
	const float Hour = Sky ? Sky->GetHour() : 12.f;
	bBandOn = Hour >= BarRooms::OpensAt || Hour < BarRooms::ClosesAt;

	// Who is to be seen: the bands while they play, the day sitters while
	// they don't; Dzsoki and Rózsi not while they are out in a mission.
	auto Show = [](AStoryPerson* Who, bool bShow)
	{
		if (Who && Who->IsHidden() == bShow)
		{
			Who->SetInWorld(bShow);
		}
	};
	for (int32 Bar = 0; Bar < 2; ++Bar)
	{
		for (ABandPlayer* Player : (Bar == BarTavern ? TavernBand : CafeBand))
		{
			const bool bOut = (Player == StageDzsoki && bDzsokiOut) || (Player == StageRozsi && bRozsiOut);
			Show(Player, bBandOn && !bOut);
			if (Player)
			{
				Player->SetPlaying(bBandOn);
			}
		}
	}
	for (AStoryPerson* Sitter : DaySitters)
	{
		const bool bOut = (Sitter == TableDzsoki && bDzsokiOut) || (Sitter == TableRozsi && bRozsiOut);
		Show(Sitter, !bBandOn && !bOut);
	}

	// The music: in the room, or through the wall by the street door.
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	const APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	for (int32 Bar = 0; Bar < Music.Num(); ++Bar)
	{
		UBandSound* Sound = Music[Bar];
		if (!Sound)
		{
			continue;
		}
		Sound->SetPlaying(bBandOn);
		if (Inside == Bar)
		{
			Sound->SetHeard(1.f, 0.f);
		}
		else if (Pawn && Inside == INDEX_NONE)
		{
			const double Apart = FVector::Dist(Pawn->GetActorLocation(), StreetDoor[Bar]);
			const float Near = static_cast<float>(FMath::Clamp(1.0 - Apart / BarRooms::HeardOutside, 0.0, 1.0));
			Sound->SetHeard(BarRooms::OutsideLoud * Near * Near, 1.f);
		}
		else
		{
			Sound->SetHeard(0.f, 1.f);
		}
	}
}

void AMusicBars::TickChain()
{
	const APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this);
	if (!Game || !Game->IsMissionDone(BarRooms::OpenedBy))
	{
		return;
	}
	// One of his missions about already (waiting, under way, or just passed and tidying up).
	for (TActorIterator<AMission> It(GetWorld()); It; ++It)
	{
		if (It->bSideStory && !It->IsActorBeingDestroyed())
		{
			return;
		}
	}
	// Not while a story mission is under way.
	for (TActorIterator<AMission> It(GetWorld()); It; ++It)
	{
		if (It->IsUnderway())
		{
			return;
		}
	}
	// The next one he has for the player.
	if (!Game->IsMissionDone(FName(AMissionDzsokiUtok::Id)))
	{
		GetWorld()->SpawnActor<AMissionDzsokiUtok>();
	}
	else if (!Game->IsMissionDone(FName(AMissionDzsokiUdvarlo::Id)))
	{
		// Rózsi goes walking out with Gyuszi in the evening.
		const ADayNight* Sky = ADayNight::Get(GetWorld());
		const float Hour = Sky ? Sky->GetHour() : 20.f;
		if (Hour >= BarRooms::OpensAt || Hour < BarRooms::ClosesAt)
		{
			GetWorld()->SpawnActor<AMissionDzsokiUdvarlo>();
		}
	}
}
