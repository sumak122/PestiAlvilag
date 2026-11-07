// Fill out your copyright notice in the Description page of Project Settings.

#include "MissionKifutofiu.h"

#include "Bicycle.h"
#include "BicycleModel.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PersonModel.h"
#include "PestiHud.h"
#include "PestiPerson.h"
#include "ProceduralMeshComponent.h"
#include "StoryPerson.h"

namespace Kifutofiu
{
	using DeliveryModel::EPlace;

	// The three deliveries, in the order of the steps after talking to Varga.
	const EPlace Stops[] = { EPlace::Cafe, EPlace::Station, EPlace::Doctor };
	constexpr int32 NumStops = 3;
	constexpr int32 TalkStep = 0;
	constexpr int32 FirstStop = 1;
	constexpr int32 PayStep = FirstStop + NumStops;

	// The clock for the three parcels (s), and when it turns red.
	constexpr float ClockSeconds = 450.f;
	constexpr float ClockWarning = 60.f;

	constexpr float TalkRadius = 250.f;
	constexpr float DoorRadius = 250.f;
	// A parcel is handed over only with the bicycle this close to the player (cm).
	constexpr double BikeNear = 1500.0;
	// The thrown-out parcel lands this far to the side of the bicycle, and is
	// picked up from this close, with the bicycle this close (cm).
	constexpr double ThrowSide = 130.0;
	constexpr double PickUpReach = 200.0;
	constexpr double PickUpBikeNear = 800.0;

	// The player's bicycle stands on the pavement left of the shop door, along the wall.
	const FVector BikeAt(95.0, -270.0, 0.0);
	constexpr double BikeYaw = -90.0;
	// Bicycles and people are spawned this high over the ground (cm).
	constexpr double BikeLift = 80.0;
	constexpr double PersonLift = 10.0;
	// Trying again: on the pavement before the shop, facing its door.
	const FVector RetryAt(260.0, -60.0, 0.0);

	// HUD lines: the clock, and short notes.
	constexpr int32 ClockKey = 36;
	constexpr int32 HintKey = 37;
	constexpr float HintSeconds = 6.f;

	const TCHAR* const VargaName = TEXT("Varga Lajos");
	const TCHAR* const MazsolaName = TEXT("Mazsola");
	const TCHAR* const WaiterName = TEXT("F\u0151pinc\u00e9r");
	const TCHAR* const LadyName = TEXT("\u00darin\u0151");
	const TCHAR* const PorterName = TEXT("Hord\u00e1r");
	const TCHAR* const MaidName = TEXT("Szobal\u00e1ny");
}

AMissionKifutofiu::AMissionKifutofiu()
{
	// Must match PestiStart::Kifutofiu in the game mode.
	MissionId = TEXT("Kifutofiu");
	Reward = 870;
	TitleHu = TEXT("Kifut\u00f3fi\u00fa");
	TitleEn = TEXT("The errand boy");
}

FVector AMissionKifutofiu::SpotAt(DeliveryModel::EPlace Spot, const FVector& Local) const
{
	const FVector Flat = DeliveryModel::ToCity(Spot, Local);
	const int32 Index = static_cast<int32>(Spot);
	double Ground = 0.0;
	if (SpotGround.IsValidIndex(Index))
	{
		Ground = SpotGround[Index];
	}
	else
	{
		Ground = GroundZ(FVector2D(DeliveryModel::ToCity(Spot, DeliveryModel::PersonAt(Spot))), 0.0);
	}
	return FVector(Flat.X, Flat.Y, Ground + Local.Z);
}

double AMissionKifutofiu::SpotYaw(DeliveryModel::EPlace Spot, double LocalYaw) const
{
	return DeliveryModel::PlaceYaw(Spot) + LocalYaw;
}

AStoryPerson* AMissionKifutofiu::SpawnPerson(DeliveryModel::EPlace Spot, const FVector& Local, double LocalYaw, const PersonKit::FOutfit& Outfit)
{
	const FVector At = SpotAt(Spot, Local) + FVector(0.0, 0.0, PersonModel::MiddleHeight + Kifutofiu::PersonLift);
	AStoryPerson* Person = SpawnForMission<AStoryPerson>(At, SpotYaw(Spot, LocalYaw));
	if (Person)
	{
		Person->SetOutfit(Outfit);
	}
	return Person;
}

ABicycle* AMissionKifutofiu::SpawnBike(DeliveryModel::EPlace Spot, const FVector& Local, double LocalYaw)
{
	const FTransform Where(FRotator(0.0, SpotYaw(Spot, LocalYaw), 0.0), SpotAt(Spot, Local) + FVector(0.0, 0.0, Kifutofiu::BikeLift));
	ABicycle* Spawned = GetWorld()->SpawnActorDeferred<ABicycle>(ABicycle::StaticClass(), Where, nullptr, nullptr,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (Spawned)
	{
		Spawned->bShopBike = true;
		Spawned->FinishSpawning(Where);
	}
	return Spawned;
}

UProceduralMeshComponent* AMissionKifutofiu::MakeMesh(const PersonKit::FPiece& Piece, const FVector& At, double Yaw)
{
	UProceduralMeshComponent* Mesh = NewObject<UProceduralMeshComponent>(this);
	Mesh->SetupAttachment(GetRootComponent());
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCanEverAffectNavigation(false);
	Mesh->RegisterComponent();
	AddInstanceComponent(Mesh);
	for (int32 i = 0; i < Piece.Sections.Num(); ++i)
	{
		const PersonKit::FSection& Section = Piece.Sections[i];
		Mesh->CreateMeshSection_LinearColor(i, Section.Vertices, Section.Triangles, Section.Normals,
			TArray<FVector2D>(), TArray<FLinearColor>(), TArray<FProcMeshTangent>(), false);
		if (BaseMaterial)
		{
			UMaterialInstanceDynamic* Paint = UMaterialInstanceDynamic::Create(BaseMaterial, this);
			Paint->SetVectorParameterValue(TEXT("Color"), Section.Color);
			Mesh->SetMaterial(i, Paint);
		}
	}
	const FRotator Turn(0.0, Yaw, 0.0);
	Mesh->SetWorldLocationAndRotation(At + Turn.RotateVector(Piece.Pivot), Turn);
	return Mesh;
}

void AMissionKifutofiu::SetUpMission()
{
	using DeliveryModel::EPlace;

	// Each place's pavement height, found before anyone stands on it.
	for (int32 i = 0; i < static_cast<int32>(EPlace::Count); ++i)
	{
		const EPlace Spot = static_cast<EPlace>(i);
		SpotGround.Add(GroundZ(FVector2D(DeliveryModel::ToCity(Spot, DeliveryModel::PersonAt(Spot))), 0.0));
	}

	// Each place dressed up: the café's tables, the luggage at Keleti, the doctor's bay trees.
	for (const EPlace Spot : { EPlace::Shop, EPlace::Cafe, EPlace::Station, EPlace::Doctor })
	{
		PersonKit::FPiece Props;
		DeliveryModel::BuildScene(Props, Spot);
		Scenery.Add(MakeMesh(Props, SpotAt(Spot, FVector::ZeroVector), SpotYaw(Spot, 0.0)));
	}
	PersonKit::FPiece Parcel;
	BicycleModel::BuildParcel(Parcel, FVector::ZeroVector, BicycleModel::ParcelHalf(0));
	FallenParcel = MakeMesh(Parcel, FVector::ZeroVector, 0.0);
	FallenParcel->SetVisibility(false);

	// The bicycles and the people.
	Bike = SpawnBike(EPlace::Shop, Kifutofiu::BikeAt, Kifutofiu::BikeYaw);
	MazsolaBike = SpawnBike(EPlace::Shop, DeliveryModel::MazsolaBikeAt(), DeliveryModel::MazsolaBikeYaw());

	Varga = SpawnPerson(EPlace::Shop, DeliveryModel::PersonAt(EPlace::Shop), DeliveryModel::PersonYaw(EPlace::Shop), DeliveryModel::VargaOutfit());
	VargaAt = SpotAt(EPlace::Shop, DeliveryModel::PersonAt(EPlace::Shop));
	Mazsola = SpawnPerson(EPlace::Shop, DeliveryModel::MazsolaAt(), DeliveryModel::MazsolaYaw(), DeliveryModel::MazsolaOutfit());
	SpawnPerson(EPlace::Station, DeliveryModel::PorterAt(), DeliveryModel::PorterYaw(), DeliveryModel::PorterOutfit());

	FMissionStep Talk;
	Talk.GoalHu = TEXT("Menj Varga Lajos f\u0171szereshez a N\u00e9psz\u00ednh\u00e1z utc\u00e1ban");
	Talk.GoalEn = TEXT("Go to Varga Lajos's grocery on N\u00e9psz\u00ednh\u00e1z utca");
	Talk.Target = VargaAt;
	Talk.Radius = Kifutofiu::TalkRadius;
	Talk.bPressE = true;
	Talk.Place = TEXT("N\u00e9psz\u00ednh\u00e1z utca");
	Steps.Add(Talk);

	const TCHAR* const GoalsHu[] = {
		TEXT("Vidd a csomagot az Oppenheim k\u00e1v\u00e9h\u00e1zba"),
		TEXT("Vidd a csomagot az \u00farin\u0151nek a Keleti taxi\u00e1llom\u00e1s\u00e1hoz"),
		TEXT("Vidd a csomagot Hal\u00e1sz doktornak, N\u00e9psz\u00ednh\u00e1z utca 7."),
	};
	const TCHAR* const GoalsEn[] = {
		TEXT("Take the parcel to the Oppenheim caf\u00e9"),
		TEXT("Take the parcel to the lady at the Keleti taxi stand"),
		TEXT("Take the parcel to Dr. Hal\u00e1sz, N\u00e9psz\u00ednh\u00e1z utca 7"),
	};
	const TCHAR* const Places[] = { TEXT("N\u00e9psz\u00ednh\u00e1z utca"), TEXT("Baross t\u00e9r"), TEXT("N\u00e9psz\u00ednh\u00e1z utca") };
	for (int32 i = 0; i < Kifutofiu::NumStops; ++i)
	{
		const EPlace Spot = Kifutofiu::Stops[i];
		Receivers.Add(SpawnPerson(Spot, DeliveryModel::PersonAt(Spot), DeliveryModel::PersonYaw(Spot), DeliveryModel::OutfitAt(Spot)));

		FMissionStep Stop;
		Stop.GoalHu = GoalsHu[i];
		Stop.GoalEn = GoalsEn[i];
		Stop.Target = SpotAt(Spot, DeliveryModel::PersonAt(Spot));
		Stop.Radius = Kifutofiu::DoorRadius;
		Stop.bPressE = true;
		Stop.bManual = true;
		Stop.Place = Places[i];
		Steps.Add(Stop);
	}

	FMissionStep Pay;
	Pay.GoalHu = TEXT("Menj vissza Varga \u00farhoz a p\u00e9nzed\u00e9rt");
	Pay.GoalEn = TEXT("Go back to Mr Varga for your pay");
	Pay.Target = VargaAt;
	Pay.Radius = Kifutofiu::TalkRadius;
	Pay.bPressE = true;
	Pay.Place = TEXT("N\u00e9psz\u00ednh\u00e1z utca");
	Steps.Add(Pay);
}

void AMissionKifutofiu::SayLater(float Seconds, const FString& Who, const FString& LineHu, const FString& LineEn)
{
	const double Last = Lines.Num() > 0 ? Lines.Last().At : GetWorld()->GetTimeSeconds();
	Lines.Add({ FMath::Max(Last, GetWorld()->GetTimeSeconds() + Seconds), Who, LineHu, LineEn });
}

void AMissionKifutofiu::Hint(const FString& Text) const
{
	APestiHUD::Show(this, Kifutofiu::HintKey, Kifutofiu::HintSeconds, FColor(255, 210, 60), Text);
}

void AMissionKifutofiu::OnStepStarted(int32 Step)
{
	if (Step == Kifutofiu::FirstStop)
	{
		Hint(TEXT("Tipp: F-fel \u00fclsz fel a biciklire; a csomagot gyalog, E-vel adod \u00e1t  (Tip: F gets on the bicycle; hand each parcel over on foot with E)"));
	}
}

void AMissionKifutofiu::OnStepDone(int32 Step)
{
	const APawn* Player = GetPlayer();
	const FVector PlayerAt = Player ? Player->GetActorLocation() : FVector::ZeroVector;
	if (Step == Kifutofiu::TalkStep)
	{
		if (Varga && Player)
		{
			Varga->FaceTowards(PlayerAt);
		}
		if (Mazsola && Player)
		{
			Mazsola->FaceTowards(PlayerAt);
		}
		if (Bike)
		{
			Bike->SetParcels(Kifutofiu::NumStops);
			KnocksSeen = Bike->GetHardKnocks();
		}
		TimeLeft = Kifutofiu::ClockSeconds;
		Say(Kifutofiu::VargaName,
			TEXT("Maga az \u00faj kifut\u00f3? A fiam \u00e1gynak esett, ma maga viszi ki az \u00e1rut."),
			TEXT("You're the new errand boy? My lad's taken to his bed, so today you take the goods out."), 6.f);
		SayLater(6.f, Kifutofiu::VargaName,
			TEXT("H\u00e1rom csomag van a kos\u00e1rban: az Oppenheim k\u00e1v\u00e9h\u00e1znak, egy \u00farin\u0151nek a Keletin\u00e9l, \u00e9s Hal\u00e1sz doktornak a hetes sz\u00e1m al\u00e1. H\u00e9t \u00e9s f\u00e9l perc, nem t\u00f6bb!"),
			TEXT("Three parcels in the basket: for the Oppenheim caf\u00e9, a lady at Keleti, and Dr. Hal\u00e1sz at number 7. Seven and a half minutes, no more!"));
		SayLater(7.f, Kifutofiu::MazsolaName,
			TEXT("Vigy\u00e1zz a kos\u00e1rra, \u00fajonc. Ha nekim\u00e9sz valaminek, kirep\u00fcl a csomag."),
			TEXT("Mind the basket, new boy. Ride into something hard and the parcel flies out."));
		return;
	}
	if (Step >= Kifutofiu::FirstStop && Step < Kifutofiu::PayStep)
	{
		const int32 Stop = Step - Kifutofiu::FirstStop;
		AStoryPerson* Receiver = Receivers.IsValidIndex(Stop) ? Receivers[Stop].Get() : nullptr;
		if (Receiver && Player)
		{
			Receiver->FaceTowards(PlayerAt);
		}
		Lines.Reset();
		if (Stop == 0)
		{
			Say(Kifutofiu::WaiterName,
				TEXT("Na v\u00e9gre, m\u00e1r fogy\u00f3ban a k\u00e1v\u00e9. K\u00f6sz\u00f6n\u00f6m, fiam!"),
				TEXT("At last, we're nearly out of coffee. Thank you, son!"), 5.f);
		}
		else if (Stop == 1)
		{
			Say(Kifutofiu::LadyName,
				TEXT("Az \u00faj kalapom! M\u00e1r azt hittem, n\u00e9lk\u00fcle kell felsz\u00e1llnom a vonatra."),
				TEXT("My new hat! I thought I'd have to board the train without it."), 5.f);
			SayLater(5.f, Kifutofiu::PorterName,
				TEXT("Ezt is a kocsira, nagys\u00e1ga?"),
				TEXT("This one on the cart too, madam?"));
		}
		else
		{
			Say(Kifutofiu::MaidName,
				TEXT("A doktor \u00far m\u00e1r v\u00e1rta. Tudod, \u0151 az, akinek Steyr aut\u00f3ja van. Az eg\u00e9sz utca irigyli."),
				TEXT("The doctor was expecting this. You know, he's the one with the Steyr motor car. The whole street envies it."), 7.f);
		}
		if (Step + 1 == Kifutofiu::PayStep)
		{
			// All three handed over: the clock stops.
			const int32 Left = FMath::Max(0, FMath::FloorToInt32(TimeLeft));
			TimeLeft = -1.f;
			APestiHUD::Show(this, Kifutofiu::ClockKey, 0.f, FColor::White, FString());
			APestiHUD::Show(this, Kifutofiu::HintKey, 8.f, FColor(255, 210, 60),
				FString::Printf(TEXT("Mind a h\u00e1rom csomag \u00e1tadva, %d:%02d maradt  (All three parcels delivered, %d:%02d to spare)"), Left / 60, Left % 60, Left / 60, Left % 60));
		}
		return;
	}
	if (Step == Kifutofiu::PayStep)
	{
		if (Varga && Player)
		{
			Varga->FaceTowards(PlayerAt);
		}
		if (Mazsola && Player)
		{
			Mazsola->FaceTowards(PlayerAt);
		}
		bKeepBike = true;
		Lines.Reset();
		Say(Kifutofiu::VargaName,
			TEXT("\u00dcgyes! Itt a b\u00e9re. A biciklit megtarthatja, \u00fagyis rozoga m\u00e1r."),
			TEXT("Well done! Here's your pay. Keep the bicycle, it's a rattletrap anyway."), 6.f);
		SayLater(6.f, Kifutofiu::MazsolaName,
			TEXT("Nem rossz, \u00fajonc. Vas\u00e1rnap versenyt futunk a K\u00f6r\u00faton. Ha mersz, gyere el."),
			TEXT("Not bad, new boy. On Sunday we race round the K\u00f6r\u00fat. Come along if you dare."));
	}
}

void AMissionKifutofiu::TickStep(int32 Step, float DeltaSeconds)
{
	// Lines said a little later, in turn.
	const double Now = GetWorld()->GetTimeSeconds();
	if (Lines.Num() > 0 && Now >= Lines[0].At)
	{
		Say(Lines[0].Who, Lines[0].Hu, Lines[0].En, 7.f);
		Lines.RemoveAt(0);
	}

	if (Step < Kifutofiu::FirstStop || Step >= Kifutofiu::PayStep)
	{
		return;
	}
	TickClock(DeltaSeconds);
	if (HasFailed())
	{
		return;
	}
	if (!TickFallenParcel())
	{
		TickDelivery(Step);
	}
}

void AMissionKifutofiu::TickClock(float DeltaSeconds)
{
	if (TimeLeft < 0.f)
	{
		return;
	}
	TimeLeft -= DeltaSeconds;
	if (TimeLeft <= 0.f)
	{
		TimeLeft = -1.f;
		APestiHUD::Show(this, Kifutofiu::ClockKey, 0.f, FColor::White, FString());
		Fail(TEXT("Lej\u00e1rt az id\u0151"), TEXT("Out of time"));
		return;
	}
	const int32 Seconds = FMath::CeilToInt32(TimeLeft);
	const FColor Shade = TimeLeft < Kifutofiu::ClockWarning ? FColor(235, 70, 50) : FColor(255, 210, 60);
	APestiHUD::Show(this, Kifutofiu::ClockKey, 0.f, Shade,
		FString::Printf(TEXT("Id\u0151: %d:%02d  (Time left: %d:%02d)"), Seconds / 60, Seconds % 60, Seconds / 60, Seconds % 60));
}

bool AMissionKifutofiu::TickFallenParcel()
{
	if (!Bike || !FallenParcel)
	{
		return false;
	}

	// Ridden hard into something: one parcel flies out of the basket.
	const int32 Knocks = Bike->GetHardKnocks();
	if (Knocks != KnocksSeen)
	{
		KnocksSeen = Knocks;
		if (!bParcelDown && Bike->GetParcels() > 0)
		{
			Bike->SetParcels(Bike->GetParcels() - 1);
			bParcelDown = true;
			const FVector Side = Bike->GetActorRightVector().GetSafeNormal2D() * Kifutofiu::ThrowSide;
			const FVector2D Land(Bike->GetActorLocation() + Side);
			FallenParcel->SetWorldLocationAndRotation(FVector(Land, GroundZ(Land, Bike->GetActorLocation().Z)),
				FRotator(0.0, Bike->GetActorRotation().Yaw + 35.0, 0.0));
			FallenParcel->SetVisibility(true);
			Hint(TEXT("Kirep\u00fclt egy csomag! Szedd fel E-vel  (A parcel flew out! Pick it up with E)"));
		}
	}
	if (!bParcelDown)
	{
		return false;
	}

	const APawn* Player = GetPlayer();
	if (!Player || !Player->IsA<APestiPerson>()
		|| FVector::Dist2D(Player->GetActorLocation(), FallenParcel->GetComponentLocation()) > Kifutofiu::PickUpReach)
	{
		return false;
	}
	if (APestiHUD* Hud = APestiHUD::Get(this))
	{
		Hud->Prompt(TEXT("E"), TEXT("Felveszed a csomagot"), TEXT("Pick up the parcel"), true);
	}
	const APlayerController* PC = Cast<APlayerController>(Player->GetController());
	if (!PC || !PC->WasInputKeyJustPressed(EKeys::E))
	{
		return false;
	}
	if (FVector::Dist2D(Player->GetActorLocation(), Bike->GetActorLocation()) > Kifutofiu::PickUpBikeNear)
	{
		Hint(TEXT("Hozd k\u00f6zelebb a biciklit, hogy visszatedd a kos\u00e1rba  (Bring the bicycle closer to put it back in the basket)"));
		return true;
	}
	Bike->SetParcels(Bike->GetParcels() + 1);
	bParcelDown = false;
	FallenParcel->SetVisibility(false);
	return true;
}

void AMissionKifutofiu::TickDelivery(int32 Step)
{
	const APawn* Player = GetPlayer();
	if (!Player || !Player->IsA<APestiPerson>() || !Steps.IsValidIndex(Step)
		|| FVector::Dist2D(Player->GetActorLocation(), Steps[Step].Target) > Steps[Step].Radius)
	{
		return;
	}
	const APlayerController* PC = Cast<APlayerController>(Player->GetController());
	if (!PC || !PC->WasInputKeyJustPressed(EKeys::E))
	{
		return;
	}
	const bool bBikeNear = Bike && FVector::Dist2D(Player->GetActorLocation(), Bike->GetActorLocation()) < Kifutofiu::BikeNear;
	if (bBikeNear && Bike->GetParcels() > 0)
	{
		Bike->SetParcels(Bike->GetParcels() - 1);
		FinishStep();
	}
	else if (bBikeNear && bParcelDown)
	{
		Hint(TEXT("A csomag kirep\u00fclt a kos\u00e1rb\u00f3l, el\u0151bb szedd fel  (The parcel flew out of the basket; pick it up first)"));
	}
	else
	{
		Hint(TEXT("A csomag a bicikli kosar\u00e1ban van, hozd ide a biciklit  (The parcel is in the bicycle's basket; bring the bicycle here)"));
	}
}

bool AMissionKifutofiu::GetRetrySpot(FVector& OutAt, double& OutYaw) const
{
	OutAt = SpotAt(DeliveryModel::EPlace::Shop, Kifutofiu::RetryAt) + FVector(0.0, 0.0, PersonModel::MiddleHeight + Kifutofiu::PersonLift);
	OutYaw = SpotYaw(DeliveryModel::EPlace::Shop, 180.0);
	return true;
}

void AMissionKifutofiu::OnFailed()
{
	Lines.Reset();
	TimeLeft = -1.f;
	APestiHUD::Show(this, Kifutofiu::ClockKey, 0.f, FColor::White, FString());
	// Off the bicycle, so that trying again finds him on his feet.
	if (Bike)
	{
		Bike->PutRiderOff();
	}
	if (MazsolaBike)
	{
		MazsolaBike->PutRiderOff();
	}
}

void AMissionKifutofiu::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (EndPlayReason == EEndPlayReason::Destroyed)
	{
		APestiHUD::Show(this, Kifutofiu::ClockKey, 0.f, FColor::White, FString());
		for (ABicycle* Ridden : { Bike.Get(), MazsolaBike.Get() })
		{
			if (!IsValid(Ridden))
			{
				continue;
			}
			// The bicycle Varga gave him stays; Mazsola's goes (with him).
			if (bKeepBike && Ridden == Bike)
			{
				Ridden->SetParcels(0);
				continue;
			}
			Ridden->PutRiderOff();
			Ridden->Destroy();
		}
	}
	Bike = nullptr;
	MazsolaBike = nullptr;
	Super::EndPlay(EndPlayReason);
}
