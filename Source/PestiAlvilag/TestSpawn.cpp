// Fill out your copyright notice in the Description page of Project Settings.

#include "TestSpawn.h"

#include "Animals.h"
#include "BoxingYard.h"
#include "BoxingYardModel.h"
#include "Brawl.h"
#include "DayNight.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "HomeDoor.h"
#include "LaciYard.h"
#include "Pedestrians.h"
#include "PersonModel.h"
#include "PestiAlvilagGameModeBase.h"
#include "PestiHud.h"
#include "PestiPerson.h"
#include "PoliceChase.h"
#include "PolicePatrols.h"
#include "StableYard.h"
#include "StreetFights.h"
#include "TestGrounds.h"
#include "TestGroundsModel.h"
#include "TrafficManager.h"
#include "Witnesses.h"

namespace TestSpawnPlace
{
	/** The line of the screen messages (as the test grounds' own). */
	constexpr int32 SayLine = 9;
	constexpr float SaySeconds = 4.f;
	/** How far in front of him things are put (cm): a vehicle, an animal, a fighter, a crowd. */
	constexpr double VehicleAhead = 700.0;
	constexpr double VehicleAheadDriving = 1100.0;
	constexpr double AnimalAhead = 400.0;
	constexpr double FighterAhead = 250.0;
	constexpr double CrowdAhead = 300.0;
	/** A vehicle lines up with a lane this near (cm), else it faces the way he does. */
	constexpr double LineUpWithin = 1500.0;
	constexpr double CarLift = 90.0;
	constexpr double CartLift = 150.0;
	/** How far off the constables still come to look (cm), and how long a crowd stays (s). */
	constexpr double ConstablesFrom = 8000.0;
	constexpr float CrowdSeconds = 25.f;
	/** A jump: the street nearest the place, within this (cm), and how far from the road towards the place he stands. */
	constexpr double JumpRoadWithin = 8000.0;
	constexpr double JumpTowards = 450.0;

	const int32 Hours[] = { 6, 9, 12, 15, 18, 20, 22, 0, 3 };

	/** A jump target on the map with no actor to ask (the map's place names, PestiHud MapPlaces). */
	struct FSquare
	{
		const TCHAR* Hu;
		const TCHAR* En;
		double X;
		double Y;
	};
	/** The places with an actor first (home, Laci, Bandi, the stable yard, the police), then these. */
	constexpr int32 ActorJumps = 5;
	const FSquare Squares[] = {
		{ TEXT("Blaha Lujza t\u00e9r"), TEXT("Blaha Lujza square"), 12000.0, -5000.0 },
		{ TEXT("Teleki t\u00e9r"), TEXT("Teleki square"), 101500.0, 22500.0 },
		{ TEXT("Tisza K\u00e1lm\u00e1n t\u00e9r"), TEXT("Tisza K\u00e1lm\u00e1n square"), 67500.0, -1000.0 },
		{ TEXT("Keleti p\u00e1lyaudvar"), TEXT("Keleti station"), 104000.0, -47000.0 },
		{ TEXT("Kerepesi temet\u0151"), TEXT("Kerepesi cemetery"), 128000.0, 12000.0 },
		{ TEXT("New York palota"), TEXT("New York palace"), 10500.0, -24500.0 },
	};
	constexpr int32 NumSquares = static_cast<int32>(UE_ARRAY_COUNT(Squares));

	/** The vehicle yard's kinds, one of each (the wrecks to ram left out). */
	inline TArray<TestGroundsModel::FVehicleSpot> VehicleKinds()
	{
		TArray<TestGroundsModel::FVehicleSpot> All;
		TestGroundsModel::Vehicles(All);
		TArray<TestGroundsModel::FVehicleSpot> Kinds;
		for (const TestGroundsModel::FVehicleSpot& Spot : All)
		{
			if (!Spot.bTarget)
			{
				Kinds.Add(Spot);
			}
		}
		return Kinds;
	}

	inline void VehicleName(const TestGroundsModel::FVehicleSpot& Spot, FString& Hu, FString& En)
	{
		using TestGroundsModel::EVehicle;
		using CarModel::EBody;
		switch (Spot.Kind)
		{
		case EVehicle::Taxi: Hu = TEXT("Taxi"); En = TEXT("Taxi"); return;
		case EVehicle::DeluxeTaxi: Hu = TEXT("K\u00fcl\u00f6nleges taxi"); En = TEXT("Special taxi"); return;
		case EVehicle::Police: Hu = TEXT("Rend\u0151raut\u00f3"); En = TEXT("Police car"); return;
		case EVehicle::HorseCart: Hu = TEXT("Lovaskocsi"); En = TEXT("Horse cart"); return;
		case EVehicle::Bicycle: Hu = TEXT("Ker\u00e9kp\u00e1r"); En = TEXT("Bicycle"); return;
		case EVehicle::ShopBicycle: Hu = TEXT("Boltos ker\u00e9kp\u00e1r"); En = TEXT("Shop bicycle"); return;
		case EVehicle::FireTruck: Hu = TEXT("T\u0171zolt\u00f3 fecskend\u0151"); En = TEXT("Fire engine"); return;
		case EVehicle::Motorbike: Hu = TEXT("Oldalkocsis motor"); En = TEXT("Motorbike with sidecar"); return;
		default: break;
		}
		switch (Spot.Body)
		{
		case EBody::Saloon: Hu = TEXT("Szed\u00e1n"); En = TEXT("Saloon"); break;
		case EBody::Tourer: Hu = TEXT("T\u00faraaut\u00f3"); En = TEXT("Tourer"); break;
		case EBody::Coupe: Hu = TEXT("Kup\u00e9"); En = TEXT("Coup\u00e9"); break;
		case EBody::Limousine: Hu = TEXT("Limuzin"); En = TEXT("Limousine"); break;
		case EBody::Van: Hu = TEXT("Furgon"); En = TEXT("Van"); break;
		case EBody::Balilla: Hu = TEXT("Fiat Balilla"); En = TEXT("Fiat Balilla"); break;
		case EBody::Opel: Hu = TEXT("Opel"); En = TEXT("Opel"); break;
		case EBody::Steyr: Hu = TEXT("Dr. Hal\u00e1sz Steyrje"); En = TEXT("Dr. Hal\u00e1sz's Steyr"); break;
		case EBody::Ssk: Hu = TEXT("Mercedes SSK"); En = TEXT("Mercedes SSK"); break;
		case EBody::FordT: Hu = TEXT("Ford T"); En = TEXT("Ford Model T"); break;
		case EBody::TownCar: Hu = TEXT("V\u00e1rosi kocsi"); En = TEXT("Town car"); break;
		default: Hu = TEXT("Aut\u00f3"); En = TEXT("Car"); break;
		}
	}

	inline void Say(UWorld* World, const FString& Text, const FColor& Color = FColor::White)
	{
		APestiHUD::Show(World, SayLine, SaySeconds, Color, Text);
	}

	inline ATrafficManager* FindTraffic(UWorld* World)
	{
		TActorIterator<ATrafficManager> It(World);
		return It ? *It : nullptr;
	}

	/** The ground under At (looked for from a little above it down to well below), or false. */
	inline bool GroundUnder(UWorld* World, const FVector& At, const AActor* Skip, FVector& OutGround)
	{
		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(TestSpawnGround), false, Skip);
		if (!World->LineTraceSingleByChannel(Hit, At + FVector(0.0, 0.0, 300.0), At - FVector(0.0, 0.0, 1200.0), ECC_Visibility, Params))
		{
			return false;
		}
		OutGround = Hit.ImpactPoint;
		return true;
	}

	/** The spot this far in front of the player's pawn, flat. */
	inline FVector InFront(const APawn* Pawn, double Ahead)
	{
		return Pawn->GetActorLocation() + Pawn->GetActorForwardVector().GetSafeNormal2D() * Ahead;
	}

	inline double YawTo(const FVector& From, const FVector& To)
	{
		const FVector Way = To - From;
		return FMath::RadiansToDegrees(FMath::Atan2(Way.Y, Way.X));
	}

	void SpawnVehicle(UWorld* World, APawn* Pawn, int32 Pick)
	{
		const TArray<TestGroundsModel::FVehicleSpot> Kinds = VehicleKinds();
		if (!Kinds.IsValidIndex(Pick))
		{
			return;
		}
		const TestGroundsModel::FVehicleSpot& Spot = Kinds[Pick];
		const bool bOnFoot = Cast<APestiPerson>(Pawn) != nullptr;
		const FVector Ahead = InFront(Pawn, bOnFoot ? VehicleAhead : VehicleAheadDriving);
		FVector Ground;
		if (!GroundUnder(World, Ahead, Pawn, Ground))
		{
			Say(World, TEXT("Nincs itt hely neki.  (No room for it here.)"));
			return;
		}
		// Along the street when there is one close by, else the way he faces.
		double Yaw = Pawn->GetActorRotation().Yaw;
		FVector LanePoint;
		FVector LaneWay;
		if (ATrafficManager* Lanes = FindTraffic(World); Lanes && Lanes->NearestLanePoint(Ahead, LineUpWithin, LanePoint, LaneWay))
		{
			Yaw = LaneWay.Rotation().Yaw;
		}
		const bool bCart = Spot.Kind == TestGroundsModel::EVehicle::HorseCart;
		const FTransform Place(FRotator(0.0, Yaw, 0.0), Ground + FVector(0.0, 0.0, bCart ? CartLift : CarLift));
		FString Hu;
		FString En;
		VehicleName(Spot, Hu, En);
		if (ATestGrounds::SpawnVehicle(World, Spot, Place, false))
		{
			Say(World, FString::Printf(TEXT("%s  (%s)"), *Hu, *En));
		}
	}

	void SpawnTraffic(UWorld* World, const APawn* Pawn, int32 Pick)
	{
		const ETestTraffic Kinds[] = { ETestTraffic::Car, ETestTraffic::Taxi, ETestTraffic::Police,
			ETestTraffic::MilkCart, ETestTraffic::CoalCart, ETestTraffic::BeerCart, ETestTraffic::Bike, ETestTraffic::Fiaker, ETestTraffic::PoliceBike, ETestTraffic::Lorry, ETestTraffic::PostVan, ETestTraffic::Ambulance, ETestTraffic::RubbishCart };
		ATrafficManager* Lanes = FindTraffic(World);
		const int32 Which = FMath::Clamp(Pick, 0, static_cast<int32>(UE_ARRAY_COUNT(Kinds)) - 1);
		if (!Lanes || !Lanes->SpawnForTest(Kinds[Which], Pawn->GetActorLocation()))
		{
			Say(World, TEXT("Nincs a k\u00f6zelben utca.  (No street near here.)"));
			return;
		}
		Say(World, TEXT("J\u00f6n az utc\u00e1n, n\u00e9zz k\u00f6r\u00fcl!  (Coming up the street, look around!)"));
	}

	void SpawnAnimal(UWorld* World, const APawn* Pawn, int32 Pick)
	{
		const AnimalModel::EKind Kinds[] = { AnimalModel::EKind::Mutt, AnimalModel::EKind::Puli,
			AnimalModel::EKind::TabbyCat, AnimalModel::EKind::BlackCat, AnimalModel::EKind::GingerCat };
		const int32 Which = FMath::Clamp(Pick, 0, static_cast<int32>(UE_ARRAY_COUNT(Kinds)) - 1);
		if (!AAnimals::AddStray(World, Kinds[Which], InFront(Pawn, AnimalAhead)))
		{
			Say(World, TEXT("Nincs itt hely neki.  (No room for it here.)"));
			return;
		}
		Say(World, Which < 2 ? TEXT("K\u00f3bor kutya: arra fut\u00f3 kocsik \u00e9s biciklik ut\u00e1n ered.  (A stray dog: it runs after passing carts and bikes.)")
			: TEXT("K\u00f3bor macska.  (A stray cat.)"));
	}

	void DoPeople(UWorld* World, APawn* Pawn, int32 Pick)
	{
		APestiPerson* Person = Cast<APestiPerson>(Pawn);
		const FVector At = Pawn->GetActorLocation();
		switch (Pick)
		{
		case 0:
		{
			// A man in street clothes comes at him, fists up: as when he hits a passer-by.
			AStreetFights* Fights = AStreetFights::Get(World);
			FVector Ground;
			const FVector Ahead = InFront(Pawn, FighterAhead);
			if (!Person || Person->IsRiding() || !Fights || !GroundUnder(World, Ahead, Pawn, Ground)
				|| !Fights->Begin(Person, BoxingYardModel::Watcher(FMath::RandRange(0, 999), false), Ground, YawTo(Ahead, At)))
			{
				Say(World, TEXT("Most nem lehet verekedni.  (No fight can start now.)"));
				return;
			}
			Say(World, TEXT("Vereked\u00e9s!  (Fight!)"));
			return;
		}
		case 1:
			APedestrians::SendChallenger(World);
			Say(World, TEXT("Egy suhanc nemsok\u00e1ra bel\u00e9d k\u00f6t, ha van j\u00e1r\u00f3kel\u0151 a k\u00f6zelben.  (A tough will pick on you soon, if anyone is walking near.)"));
			return;
		case 2:
			AWitnesses::Report(World, At, 1, true);
			Say(World, TEXT("Egy tan\u00fa a rend\u0151rdobozhoz fut.  (A witness runs to the police box.)"));
			return;
		case 3:
			APedestrians::Gather(World, InFront(Pawn, CrowdAhead), CrowdSeconds, false);
			Say(World, TEXT("B\u00e1m\u00e9szkod\u00f3k gy\u0171lnek.  (A crowd gathers.)"));
			return;
		default:
			APolicePatrols::Investigate(World, At, ConstablesFrom);
			Say(World, TEXT("A k\u00f6zeli rend\u0151r\u00f6k idej\u00f6nnek.  (The nearest constables come over.)"));
			return;
		}
	}

	/** Where a jump target is (on the ground, facing the right way when it says), or false. */
	bool JumpTarget(UWorld* World, int32 Pick, FVector& OutAt, double& OutYaw, bool& bOutExact)
	{
		bOutExact = false;
		OutYaw = 0.0;
		switch (Pick)
		{
		case 0:
		{
			TActorIterator<AHomeDoor> Door(World);
			if (!Door)
			{
				return false;
			}
			OutAt = Door->GetStandSpot();
			OutYaw = Door->GetStandYaw() + 180.0;
			bOutExact = true;
			return true;
		}
		case 1:
			if (TActorIterator<ALaciYard>(World))
			{
				OutAt = ALaciYard::ToCity(FVector::ZeroVector, 0.0);
				return true;
			}
			return false;
		case 2:
			if (TActorIterator<ABoxingYard>(World))
			{
				OutAt = ABoxingYard::ToCity(FVector::ZeroVector, 0.0);
				return true;
			}
			return false;
		case 3:
			if (TActorIterator<AStableYard>(World))
			{
				OutAt = AStableYard::ToCity(FVector::ZeroVector, 0.0);
				return true;
			}
			return false;
		case 4:
			if (const APoliceChase* Station = APoliceChase::Get(World); Station && Station->GetLetOut(OutAt, OutYaw))
			{
				OutAt.Z += PersonModel::MiddleHeight + 10.0;
				bOutExact = true;
				return true;
			}
			return false;
		default:
			break;
		}
		const int32 Square = Pick - ActorJumps;
		if (Square < 0 || Square >= NumSquares)
		{
			return false;
		}
		OutAt = FVector(Squares[Square].X, Squares[Square].Y, 0.0);
		return true;
	}

	void Jump(UWorld* World, APawn* Pawn, int32 Pick)
	{
		APestiPerson* Person = Cast<APestiPerson>(Pawn);
		APlayerController* PC = World->GetFirstPlayerController();
		if (!Person || Person->IsRiding() || !PC)
		{
			Say(World, TEXT("Ehhez sz\u00e1llj ki el\u0151bb.  (Get out of the vehicle first.)"));
			return;
		}
		FVector Target;
		double Yaw = 0.0;
		bool bExact = false;
		if (!JumpTarget(World, Pick, Target, Yaw, bExact))
		{
			Say(World, TEXT("Ez a hely most nincs meg.  (That place isn't there now.)"));
			return;
		}
		FVector Spot = Target;
		if (!bExact)
		{
			// On the street nearest the place, a few steps towards it, facing it.
			ATrafficManager* Lanes = FindTraffic(World);
			FVector Road;
			FVector Way;
			if (!Lanes || !Lanes->NearestLanePoint(Target, JumpRoadWithin, Road, Way))
			{
				Say(World, TEXT("Ez a hely most nincs meg.  (That place isn't there now.)"));
				return;
			}
			const FVector Towards = FVector(Target.X - Road.X, Target.Y - Road.Y, 0.0);
			const double Gap = Towards.Size();
			Spot = Road + Towards.GetSafeNormal() * FMath::Min(Gap * 0.6, JumpTowards);
			Spot.Z = Road.Z;
			FVector Ground;
			if (!GroundUnder(World, Spot, Person, Ground))
			{
				Ground = Road;
			}
			Spot = Ground + FVector(0.0, 0.0, PersonModel::MiddleHeight + 10.0);
			Yaw = Gap > 1.0 ? YawTo(Road, Target) : Way.Rotation().Yaw;
		}
		if (Person->GetAct() != Brawl::EAct::None)
		{
			Person->EndFight();
		}
		Person->SetActorLocationAndRotation(Spot, FRotator(0.0, Yaw, 0.0), false, nullptr, ETeleportType::TeleportPhysics);
		Person->GetCharacterMovement()->StopMovementImmediately();
		PC->SetControlRotation(FRotator(0.0, Yaw, 0.0));
		FString Hu;
		FString En;
		TestSpawn::PickName(TestSpawn::EGroup::Jump, Pick, Hu, En);
		Say(World, FString::Printf(TEXT("%s  (%s)"), *Hu, *En));
	}
}

namespace TestSpawn
{
	int32 NumPicks(EGroup Group)
	{
		switch (Group)
		{
		case EGroup::Vehicle: return TestSpawnPlace::VehicleKinds().Num();
		case EGroup::Traffic: return 13;
		case EGroup::Animal: return 5;
		case EGroup::People: return 5;
		case EGroup::Clock: return static_cast<int32>(UE_ARRAY_COUNT(TestSpawnPlace::Hours));
		case EGroup::Wanted: return 4;
		case EGroup::Jump: return TestSpawnPlace::ActorJumps + TestSpawnPlace::NumSquares;
		default: return 1;
		}
	}

	void GroupName(EGroup Group, FString& Hu, FString& En, FString& NoteHu, FString& NoteEn)
	{
		switch (Group)
		{
		case EGroup::Vehicle:
			Hu = TEXT("J\u00c1RM\u0170"); En = TEXT("Vehicle");
			NoteHu = TEXT("el\u00e9d \u00e1ll"); NoteEn = TEXT("stands in front of you");
			break;
		case EGroup::Traffic:
			Hu = TEXT("FORGALOM"); En = TEXT("Traffic");
			NoteHu = TEXT("j\u00f6n az utc\u00e1n"); NoteEn = TEXT("comes up the street");
			break;
		case EGroup::Animal:
			Hu = TEXT("\u00c1LLAT"); En = TEXT("Animal");
			NoteHu = TEXT("mell\u00e9d j\u00f6n"); NoteEn = TEXT("appears by you");
			break;
		case EGroup::People:
			Hu = TEXT("EMBEREK"); En = TEXT("People");
			NoteHu = TEXT("most r\u00f6gt\u00f6n"); NoteEn = TEXT("right now");
			break;
		case EGroup::Clock:
			Hu = TEXT("\u00d3RA"); En = TEXT("Clock");
			NoteHu = TEXT("ennyi lesz az id\u0151"); NoteEn = TEXT("sets the time");
			break;
		case EGroup::Wanted:
			Hu = TEXT("K\u00d6R\u00d6Z\u00c9S"); En = TEXT("Wanted");
			NoteHu = TEXT("ennyi csillag"); NoteEn = TEXT("this many stars");
			break;
		default:
			Hu = TEXT("UGR\u00c1S"); En = TEXT("Jump to");
			NoteHu = TEXT("csak gyalog"); NoteEn = TEXT("on foot only");
			break;
		}
	}

	void PickName(EGroup Group, int32 Pick, FString& Hu, FString& En)
	{
		switch (Group)
		{
		case EGroup::Vehicle:
		{
			const TArray<TestGroundsModel::FVehicleSpot> Kinds = TestSpawnPlace::VehicleKinds();
			if (Kinds.IsValidIndex(Pick))
			{
				TestSpawnPlace::VehicleName(Kinds[Pick], Hu, En);
			}
			break;
		}
		case EGroup::Traffic:
		{
			static const TCHAR* const Names[][2] = {
				{ TEXT("Aut\u00f3"), TEXT("Car") },
				{ TEXT("Taxi"), TEXT("Taxi") },
				{ TEXT("Rend\u0151raut\u00f3"), TEXT("Police car") },
				{ TEXT("Tejeskocsi"), TEXT("Milk cart") },
				{ TEXT("Szeneskocsi"), TEXT("Coal cart") },
				{ TEXT("S\u00f6r\u00f6skocsi"), TEXT("Beer cart") },
				{ TEXT("Biciklis"), TEXT("Bicycle rider") },
				{ TEXT("Fi\u00e1ker"), TEXT("Fiaker cab") },
				{ TEXT("Motoros rend\u0151r"), TEXT("Police motorbike") },
				{ TEXT("R\u00e1ba teheraut\u00f3"), TEXT("R\u00e1ba lorry") },
				{ TEXT("Postakocsi"), TEXT("Post van") },
				{ TEXT("Ment\u0151aut\u00f3"), TEXT("Ambulance") },
				{ TEXT("Szemeteskocsi"), TEXT("Rubbish cart") },
			};
			const int32 Row = FMath::Clamp(Pick, 0, 12);
			Hu = Names[Row][0];
			En = Names[Row][1];
			break;
		}
		case EGroup::Animal:
		{
			static const TCHAR* const Names[][2] = {
				{ TEXT("K\u00f3bor kutya"), TEXT("Stray dog") },
				{ TEXT("Puli"), TEXT("Puli") },
				{ TEXT("Cirmos macska"), TEXT("Tabby cat") },
				{ TEXT("Fekete macska"), TEXT("Black cat") },
				{ TEXT("V\u00f6r\u00f6s macska"), TEXT("Ginger cat") },
			};
			const int32 Row = FMath::Clamp(Pick, 0, 4);
			Hu = Names[Row][0];
			En = Names[Row][1];
			break;
		}
		case EGroup::People:
		{
			static const TCHAR* const Names[][2] = {
				{ TEXT("Vereked\u0151 j\u00e1r\u00f3kel\u0151"), TEXT("A passer-by to fight") },
				{ TEXT("Bel\u00e9dk\u00f6t\u0151 suhanc"), TEXT("A tough picks on you") },
				{ TEXT("Tan\u00fa a rend\u0151r\u00e9rt fut"), TEXT("A witness runs for the police") },
				{ TEXT("B\u00e1m\u00e9szkod\u00f3 t\u00f6meg"), TEXT("A crowd gathers") },
				{ TEXT("Rend\u0151r\u00f6k idej\u00f6nnek"), TEXT("Constables come over") },
			};
			const int32 Row = FMath::Clamp(Pick, 0, 4);
			Hu = Names[Row][0];
			En = Names[Row][1];
			break;
		}
		case EGroup::Clock:
		{
			const int32 Row = FMath::Clamp(Pick, 0, static_cast<int32>(UE_ARRAY_COUNT(TestSpawnPlace::Hours)) - 1);
			const int32 Hour = TestSpawnPlace::Hours[Row];
			const TCHAR* const PartHu = Hour >= 5 && Hour < 10 ? TEXT("reggel") : Hour >= 10 && Hour < 17 ? TEXT("nappal")
				: Hour >= 17 && Hour < 21 ? TEXT("este") : TEXT("\u00e9jjel");
			const TCHAR* const PartEn = Hour >= 5 && Hour < 10 ? TEXT("morning") : Hour >= 10 && Hour < 17 ? TEXT("day")
				: Hour >= 17 && Hour < 21 ? TEXT("evening") : TEXT("night");
			Hu = FString::Printf(TEXT("%02d:00  %s"), Hour, PartHu);
			En = FString::Printf(TEXT("%02d:00  %s"), Hour, PartEn);
			break;
		}
		case EGroup::Wanted:
		{
			const int32 Stars = FMath::Clamp(Pick, 0, 3);
			if (Stars == 0)
			{
				Hu = TEXT("Nincs");
				En = TEXT("None");
			}
			else
			{
				Hu = FString::Printf(TEXT("%d csillag"), Stars);
				En = FString::Printf(TEXT("%d stars"), Stars);
			}
			break;
		}
		default:
		{
			static const TCHAR* const Names[][2] = {
				{ TEXT("Otthon (a lak\u00e1sod ajtaja)"), TEXT("Home (your door)") },
				{ TEXT("Laci m\u0171helye"), TEXT("Laci's workshop") },
				{ TEXT("Bandi grundja"), TEXT("Bandi's boxing yard") },
				{ TEXT("Fuvarosudvar"), TEXT("The carters' yard") },
				{ TEXT("Rend\u0151rs\u00e9g, V\u00edg utca"), TEXT("Police station, V\u00edg utca") },
			};
			if (Pick >= 0 && Pick < TestSpawnPlace::ActorJumps)
			{
				Hu = Names[Pick][0];
				En = Names[Pick][1];
			}
			else
			{
				const int32 Square = FMath::Clamp(Pick - TestSpawnPlace::ActorJumps, 0, TestSpawnPlace::NumSquares - 1);
				Hu = TestSpawnPlace::Squares[Square].Hu;
				En = TestSpawnPlace::Squares[Square].En;
			}
			break;
		}
		}
	}

	bool NeedsFoot(EGroup Group)
	{
		return Group == EGroup::Jump;
	}

	void DoPick(UWorld* World, EGroup Group, int32 Pick)
	{
		APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		APawn* Pawn = PC ? PC->GetPawn() : nullptr;
		if (!Pawn)
		{
			return;
		}
		switch (Group)
		{
		case EGroup::Vehicle:
			TestSpawnPlace::SpawnVehicle(World, Pawn, Pick);
			break;
		case EGroup::Traffic:
			TestSpawnPlace::SpawnTraffic(World, Pawn, Pick);
			break;
		case EGroup::Animal:
			TestSpawnPlace::SpawnAnimal(World, Pawn, Pick);
			break;
		case EGroup::People:
			TestSpawnPlace::DoPeople(World, Pawn, Pick);
			break;
		case EGroup::Clock:
			if (ADayNight* Clock = ADayNight::Get(World))
			{
				const int32 Row = FMath::Clamp(Pick, 0, static_cast<int32>(UE_ARRAY_COUNT(TestSpawnPlace::Hours)) - 1);
				Clock->ReachHourIn(static_cast<float>(TestSpawnPlace::Hours[Row]), 0.f);
				FString Hu;
				FString En;
				PickName(Group, Pick, Hu, En);
				TestSpawnPlace::Say(World, Hu);
			}
			break;
		case EGroup::Wanted:
			if (APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(World))
			{
				Game->SetWantedForTest(FMath::Clamp(Pick, 0, 3));
			}
			break;
		default:
			TestSpawnPlace::Jump(World, Pawn, Pick);
			break;
		}
	}
}
