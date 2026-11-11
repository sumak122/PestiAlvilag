// Fill out your copyright notice in the Description page of Project Settings.

#include "VargaRuns.h"

#include "Components/SceneComponent.h"
#include "DeliveryModel.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GoalMarkerComponent.h"
#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Mission.h"
#include "PersonModel.h"
#include "PestiAlvilagGameModeBase.h"
#include "PestiHud.h"
#include "PestiPerson.h"
#include "PestiText.h"
#include "RouteGuideComponent.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "ShopModel.h"
#include "StoryPerson.h"
#include "StreetName.h"
#include "TaxiCompany.h"
#include "TrafficManager.h"

namespace VargaRun
{
	// The mission that opens the job.
	const FName OpenedBy(TEXT("Kifutofiu"));
	const TCHAR* const VargaName = TEXT("Varga Lajos");
	// On-screen note key (under the goal).
	constexpr int32 NoteKey = 72;
	// How close to him the player stands to talk (cm).
	constexpr double TalkRange = 260.0;
	// A parcel is handed over this close to its address, and the round ends this close to the shop (cm).
	constexpr double DropRadius = 350.0;
	constexpr double ShopRadius = 400.0;
	// Addresses this far from the shop, and from each other, in a straight line (cm).
	constexpr double MinAway = 15000.0;
	constexpr double MaxAway = 55000.0;
	constexpr double ApartFrom = 12000.0;
	// The doors: read from the street furniture's data; the one who waits stands this far out in front (cm).
	const TCHAR* const DoorsFile = TEXT("Data/OsmGraybox/street_furniture.json");
	constexpr double StandOut = 100.0;
	// The street a door is on: its nearest traffic lane, no further than this (cm).
	constexpr double StreetNear = 1800.0;
	// Bözsi néni's house has its own gateway; no parcels there.
	const FVector2D BozsiAt(58771.0, 21395.0);
	// A receiver with his parcel goes back in after this long (s).
	constexpr float GoInAfter = 3.f;

	struct FTrade
	{
		const TCHAR* Key;
		const TCHAR* Hu;
		const TCHAR* En;
	};
	// What each kind of shop is called in an address ("Népszínház utca, a pékség").
	const FTrade Trades[] = {
		{ TEXT("tavern"), TEXT("a kocsma"), TEXT("the tavern") },
		{ TEXT("baker"), TEXT("a p\u00E9ks\u00E9g"), TEXT("the bakery") },
		{ TEXT("drapery"), TEXT("a r\u0151f\u00F6sbolt"), TEXT("the draper's") },
		{ TEXT("hardware"), TEXT("a vaskeresked\u00E9s"), TEXT("the ironmonger's") },
		{ TEXT("grocer"), TEXT("a f\u0171szer\u00FCzlet"), TEXT("the grocer's") },
		{ TEXT("barber"), TEXT("a borb\u00E9ly"), TEXT("the barber's") },
		{ TEXT("butcher"), TEXT("a hentes"), TEXT("the butcher's") },
		{ TEXT("pharmacy"), TEXT("a patika"), TEXT("the chemist's") },
		{ TEXT("watches"), TEXT("az \u00F3r\u00E1s"), TEXT("the watchmaker's") },
		{ TEXT("hats"), TEXT("a kalapos"), TEXT("the hatter's") },
		{ TEXT("dairy"), TEXT("a tejcsarnok"), TEXT("the dairy") },
		{ TEXT("stationery"), TEXT("a pap\u00EDrbolt"), TEXT("the stationer's") },
		{ TEXT("tobacco"), TEXT("a trafik"), TEXT("the tobacconist's") },
		{ TEXT("shoes"), TEXT("a cip\u00E9sz"), TEXT("the shoemaker's") },
		{ TEXT("greengrocer"), TEXT("a z\u00F6lds\u00E9ges"), TEXT("the greengrocer's") },
		{ TEXT("cafe"), TEXT("a k\u00E1v\u00E9m\u00E9r\u00E9s"), TEXT("the coffee shop") },
	};
	// The gold time: the straight-line length of the round, made this much longer by the streets, at a bicycle's pace (cm/s).
	constexpr double Winding = 1.4;
	constexpr double BikePace = 650.0;
	// Silver and bronze within these shares of the gold time.
	constexpr float SilverShare = 1.3f;
	constexpr float BronzeShare = 1.7f;
	// The pay (fillér): for the round, and on top for bronze, silver, gold.
	constexpr int32 RoundPay = 80;
	const int32 MedalPay[4] = { 0, 30, 60, 100 };
	const TCHAR* const MedalHu[4] = { TEXT("\u00E9rem n\u00E9lk\u00FCl"), TEXT("bronz"), TEXT("ez\u00FCst"), TEXT("arany") };
	const TCHAR* const MedalEn[4] = { TEXT("no medal"), TEXT("bronze"), TEXT("silver"), TEXT("gold") };
}

AVargaRuns::AVargaRuns()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Marker = CreateDefaultSubobject<UGoalMarkerComponent>(TEXT("Marker"));
	Marker->SetupAttachment(RootComponent);
	Guide = CreateDefaultSubobject<URouteGuideComponent>(TEXT("Guide"));
	Guide->SetupAttachment(RootComponent);
}

void AVargaRuns::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearReceivers();
	if (IsValid(Varga.Get()))
	{
		Varga->Destroy();
	}
	Super::EndPlay(EndPlayReason);
}

FString AVargaRuns::FormatTenths(int32 Tenths)
{
	const int32 Total = FMath::Max(Tenths, 0);
	return FString::Printf(TEXT("%d:%02d.%d"), Total / 600, (Total / 10) % 60, Total % 10);
}

bool AVargaRuns::IsMissionUnderway() const
{
	for (TActorIterator<AMission> It(GetWorld()); It; ++It)
	{
		if (It->IsUnderway())
		{
			return true;
		}
	}
	return false;
}

void AVargaRuns::Note(const FString& Line, float Seconds) const
{
	APestiHUD::Show(this, VargaRun::NoteKey, Seconds, FColor(255, 200, 60), Line);
}

void AVargaRuns::VargaSays(const FString& Hu, const FString& En) const
{
	if (APestiHUD* Hud = APestiHUD::Get(this))
	{
		Hud->Say(VargaRun::VargaName, Hu, En, 7.f);
	}
}

void AVargaRuns::PlaceVarga()
{
	using DeliveryModel::EPlace;
	const FVector Local = DeliveryModel::PersonAt(EPlace::Shop);
	const FVector Flat = DeliveryModel::ToCity(EPlace::Shop, Local);
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(VargaGround), false, this);
	const double Ground = GetWorld()->LineTraceSingleByObjectType(Hit, Flat + FVector(0.0, 0.0, 10000.0), Flat - FVector(0.0, 0.0, 10000.0),
		FCollisionObjectQueryParams(ECC_WorldStatic), Params) ? Hit.ImpactPoint.Z : 0.0;
	ShopAt = FVector(Flat.X, Flat.Y, Ground);
	const double Yaw = DeliveryModel::PlaceYaw(EPlace::Shop) + DeliveryModel::PersonYaw(EPlace::Shop);
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	Varga = GetWorld()->SpawnActor<AStoryPerson>(AStoryPerson::StaticClass(), ShopAt + FVector(0.0, 0.0, PersonModel::MiddleHeight + 10.0),
		FRotator(0.0, Yaw, 0.0), SpawnParams);
	if (Varga)
	{
		Varga->SetOutfit(DeliveryModel::VargaOutfit());
	}
}

void AVargaRuns::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this);
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	APawn* Player = PC ? PC->GetPawn() : nullptr;
	if (!Game || !Player || !Game->IsMissionDone(VargaRun::OpenedBy))
	{
		return;
	}
	// His shop on the maps, so the rounds are not missed.
	if (!ShopAt.IsZero())
	{
		if (APestiHUD* MapHud = APestiHUD::Get(this))
		{
			MapHud->MarkPlace(ShopAt, TEXT("V"));
		}
	}
	const bool bMission = IsMissionUnderway();
	if (bMission)
	{
		// A mission takes over: the round is off, and Varga stays out of its way.
		if (bRunning)
		{
			StopRound();
		}
		return;
	}
	if (!IsValid(Varga.Get()))
	{
		PlaceVarga();
		if (!Varga)
		{
			return;
		}
	}

	// Talking to him, on foot: E for a round.
	const APestiPerson* Person = Cast<APestiPerson>(Player);
	const bool bByVarga = Person && !Person->IsRiding() && !Person->IsCarrying()
		&& FVector::Dist2D(Person->GetActorLocation(), Varga->GetActorLocation()) < VargaRun::TalkRange;
	if (bByVarga && !bRunning)
	{
		if (APestiHUD* Hud = APestiHUD::Get(this))
		{
			Hud->Prompt(TEXT("E"), TEXT("Kihord\u00E1s Varg\u00E1nak"), TEXT("A delivery round for Varga"), false);
		}
		if (PC->WasInputKeyJustPressed(EKeys::E) && StartRound())
		{
			return;
		}
	}
	if (!bRunning)
	{
		return;
	}

	Clock += DeltaSeconds;
	const FVector At = Player->GetActorLocation();
	const int32 Tenths = FMath::FloorToInt(Clock * 10.f);
	const int32 Best = Game->GetBestRoundTenths();
	const FString BestText = Best > 0 ? FString::Printf(TEXT("   (legjobb / best %s)"), *FormatTenths(Best)) : FString();
	// Those who have their parcels go back in.
	for (int32 i = 0; i < Receivers.Num(); ++i)
	{
		if (GoneIn.IsValidIndex(i) && GoneIn[i] > 0.f)
		{
			GoneIn[i] -= DeltaSeconds;
			if (GoneIn[i] <= 0.f && IsValid(Receivers[i].Get()))
			{
				Receivers[i]->Destroy();
				Receivers[i] = nullptr;
			}
		}
	}
	if (Next < Drops.Num())
	{
		const FVector Drop = Drops[Next];
		if (Receivers.IsValidIndex(Next) && IsValid(Receivers[Next].Get()) && FVector::Dist2D(At, Drop) < 2000.0)
		{
			Receivers[Next]->FaceTowards(At);
		}
		Marker->ShowAt(Drop);
		Guide->Guide(At, Drop);
		Note(FString::Printf(TEXT("Csomag %d/%d: %s  (Parcel %d/%d)   %s %s   Id\u0151 %s%s"), Next + 1, Drops.Num(), *Streets[Next], Next + 1, Drops.Num(),
			*PestiText::Metres(FVector::Dist2D(At, Drop)), PestiText::Compass(At, Drop), *FormatTenths(Tenths), *BestText), 0.f);
		if (FVector::Dist2D(At, Drop) < VargaRun::DropRadius && FMath::Abs(At.Z - Drop.Z) < 400.0)
		{
			if (GoneIn.IsValidIndex(Next))
			{
				GoneIn[Next] = VargaRun::GoInAfter;
			}
			++Next;
			if (APestiHUD* Hud = APestiHUD::Get(this))
			{
				Hud->Say(TEXT("Vev\u0151"), TEXT("K\u00F6sz\u00F6n\u00F6m, fiam!"), TEXT("Thank you, lad!"), 2.5f);
			}
		}
		return;
	}
	Marker->ShowAt(ShopAt);
	Guide->Guide(At, ShopAt);
	Note(FString::Printf(TEXT("Vissza Varg\u00E1hoz  (Back to Varga)   %s %s   Id\u0151 %s%s"),
		*PestiText::Metres(FVector::Dist2D(At, ShopAt)), PestiText::Compass(At, ShopAt), *FormatTenths(Tenths), *BestText), 0.f);
	if (FVector::Dist2D(At, ShopAt) < VargaRun::ShopRadius)
	{
		FinishRound();
	}
}

void AVargaRuns::ReadDoors()
{
	bDoorsRead = true;
	Doors.Reset();
	const FString Path = FPaths::Combine(FPaths::ProjectDir(), FString(VargaRun::DoorsFile));
	FString Text;
	TSharedPtr<FJsonObject> Root;
	if (!FFileHelper::LoadFileToString(Text, *Path)
		|| !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) || !Root.IsValid())
	{
		return;
	}
	// A door from the data: "at" is the foot of the wall and the way it faces out; Along is how far along the front the door is.
	auto AddDoor = [this](const FJsonObject& Item, double Along, const FString& Number, const FString& Trade)
	{
		const TArray<TSharedPtr<FJsonValue>>* AtField = nullptr;
		if (!Item.TryGetArrayField(TEXT("at"), AtField) || AtField->Num() < 3)
		{
			return;
		}
		const double Yaw = (*AtField)[2]->AsNumber();
		const FRotator Turn(0.0, Yaw, 0.0);
		const FVector Foot((*AtField)[0]->AsNumber(), (*AtField)[1]->AsNumber(), 0.0);
		if (FVector2D::Distance(FVector2D(Foot.X, Foot.Y), VargaRun::BozsiAt) < 1500.0)
		{
			return;
		}
		FDoor Door;
		Door.At = Foot + Turn.RotateVector(FVector(VargaRun::StandOut, Along, 0.0));
		Door.Yaw = Yaw;
		Door.Number = Number;
		Door.Trade = Trade;
		Doors.Add(Door);
	};
	const TArray<TSharedPtr<FJsonValue>>* Gates = nullptr;
	if (Root->TryGetArrayField(TEXT("gates"), Gates))
	{
		for (const TSharedPtr<FJsonValue>& Value : *Gates)
		{
			const TSharedPtr<FJsonObject> Gate = Value->AsObject();
			FString Number;
			if (Gate.IsValid() && Gate->TryGetStringField(TEXT("number"), Number) && !Number.IsEmpty())
			{
				AddDoor(*Gate, 0.0, Number, FString());
			}
		}
	}
	const TArray<TSharedPtr<FJsonValue>>* Shops = nullptr;
	if (Root->TryGetArrayField(TEXT("shops"), Shops))
	{
		for (const TSharedPtr<FJsonValue>& Value : *Shops)
		{
			const TSharedPtr<FJsonObject> Shop = Value->AsObject();
			bool bClosed = true;
			bool bKeeper = true;
			FString Trade;
			// Open, and nobody of the shop's own standing in the doorway already.
			if (!Shop.IsValid() || !Shop->TryGetBoolField(TEXT("closed"), bClosed) || bClosed
				|| !Shop->TryGetBoolField(TEXT("keeper"), bKeeper) || bKeeper || !Shop->TryGetStringField(TEXT("trade"), Trade))
			{
				continue;
			}
			const double Width = Shop->GetNumberField(TEXT("width"));
			const uint32 Seed = static_cast<uint32>(Shop->GetNumberField(TEXT("seed")));
			AddDoor(*Shop, ShopModel::DoorMiddle(Width, Seed), FString(), Trade);
		}
	}
}

void AVargaRuns::ClearReceivers()
{
	for (AStoryPerson* Receiver : Receivers)
	{
		if (IsValid(Receiver))
		{
			Receiver->Destroy();
		}
	}
	Receivers.Reset();
	GoneIn.Reset();
}

bool AVargaRuns::StartRound()
{
	const ATrafficManager* Traffic = nullptr;
	{
		TActorIterator<ATrafficManager> It(GetWorld());
		Traffic = It ? *It : nullptr;
	}
	if (!bDoorsRead)
	{
		ReadDoors();
	}
	if (!Traffic || Doors.Num() == 0)
	{
		VargaSays(TEXT("Ma nincs kihordanival\u00F3, fiam."), TEXT("Nothing to deliver today, son."));
		return false;
	}
	ClearReceivers();
	Drops.Reset();
	Streets.Reset();
	StreetsEn.Reset();
	TArray<FString> StreetNames;
	TArray<double> DropYaws;
	TArray<bool> DropIsShop;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(VargaDrop), false, this);
	for (int32 Try = 0; Try < 400 && Drops.Num() < 3; ++Try)
	{
		const FDoor& Door = Doors[FMath::RandRange(0, Doors.Num() - 1)];
		const double Away = FVector::Dist2D(Door.At, ShopAt);
		if (Away < VargaRun::MinAway || Away > VargaRun::MaxAway)
		{
			continue;
		}
		bool bTooClose = false;
		for (const FVector& Other : Drops)
		{
			bTooClose = bTooClose || FVector::Dist2D(Other, Door.At) < VargaRun::ApartFrom;
		}
		if (bTooClose)
		{
			continue;
		}
		// The street it is on: the lane running past the door.
		const FVector AlongWall = FRotator(0.0, Door.Yaw + 90.0, 0.0).Vector();
		int32 Lane = INDEX_NONE;
		int32 Point = INDEX_NONE;
		if (!Traffic->FindLanePointNear(Door.At, AlongWall, Lane, Point))
		{
			continue;
		}
		const FVector LaneAt = Traffic->GetLane(Lane).Points[Point];
		const FString Street = ATaxiCompany::PeriodStreetName(Traffic->FindStreetName(Lane));
		if (Street.IsEmpty() || StreetNames.Contains(Street) || FVector::Dist2D(LaneAt, Door.At) > VargaRun::StreetNear)
		{
			continue;
		}
		// The pavement in front of the door (traced from under the awnings).
		FHitResult Ground;
		const FVector Probe(Door.At.X, Door.At.Y, LaneAt.Z);
		if (!GetWorld()->LineTraceSingleByObjectType(Ground, Probe + FVector(0.0, 0.0, 150.0), Probe - FVector(0.0, 0.0, 600.0),
			FCollisionObjectQueryParams(ECC_WorldStatic), Params))
		{
			continue;
		}
		Drops.Add(Ground.ImpactPoint);
		StreetNames.Add(Street);
		DropYaws.Add(Door.Yaw);
		DropIsShop.Add(Door.Number.IsEmpty());
		if (!Door.Number.IsEmpty())
		{
			Streets.Add(FString::Printf(TEXT("%s %s."), *Street, *Door.Number));
			StreetsEn.Add(FString::Printf(TEXT("%s %s"), *Street, *Door.Number));
			continue;
		}
		const VargaRun::FTrade* Kind = nullptr;
		for (const VargaRun::FTrade& Each : VargaRun::Trades)
		{
			Kind = Door.Trade == Each.Key ? &Each : Kind;
		}
		Streets.Add(FString::Printf(TEXT("%s, %s"), *Street, Kind ? Kind->Hu : TEXT("a bolt")));
		StreetsEn.Add(FString::Printf(TEXT("%s on %s"), Kind ? Kind->En : TEXT("the shop"), *Street));
	}
	if (Drops.Num() < 3)
	{
		VargaSays(TEXT("Ma nincs kihordanival\u00F3, fiam."), TEXT("Nothing to deliver today, son."));
		return false;
	}

	// Someone waits at each door for the parcel: the shopkeeper in his apron, or one of the tenants.
	FRandomStream Random(FMath::Rand());
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	for (int32 i = 0; i < Drops.Num(); ++i)
	{
		AStoryPerson* Receiver = GetWorld()->SpawnActor<AStoryPerson>(AStoryPerson::StaticClass(),
			Drops[i] + FVector(0.0, 0.0, PersonModel::MiddleHeight + 10.0), FRotator(0.0, DropYaws[i], 0.0), SpawnParams);
		if (Receiver)
		{
			PersonKit::FOutfit Clothes = PersonModel::RandomOutfit(Random);
			Clothes.Held = PersonKit::EHeld::None;
			if (DropIsShop[i])
			{
				Clothes.bApron = Random.FRand() < 0.7f;
			}
			Receiver->SetOutfit(Clothes);
		}
		Receivers.Add(Receiver);
		GoneIn.Add(0.f);
	}

	// The gold time, from the round's length.
	double Length = FVector::Dist2D(ShopAt, Drops[0]);
	for (int32 i = 1; i < Drops.Num(); ++i)
	{
		Length += FVector::Dist2D(Drops[i - 1], Drops[i]);
	}
	Length += FVector::Dist2D(Drops.Last(), ShopAt);
	Par = static_cast<float>(Length * VargaRun::Winding / VargaRun::BikePace);
	Next = 0;
	Clock = 0.f;
	bRunning = true;
	VargaSays(FString::Printf(TEXT("H\u00E1rom csomag: %s, %s \u00E9s %s. Mindenhol v\u00E1rj\u00E1k. Siess, a vev\u0151 nem v\u00E1r! Ott a bicikli."), *Streets[0], *Streets[1], *Streets[2]),
		FString::Printf(TEXT("Three parcels: %s, %s and %s. Someone's waiting at each door. Hurry, customers don't wait! There's the bicycle."), *StreetsEn[0], *StreetsEn[1], *StreetsEn[2]));
	return true;
}

void AVargaRuns::FinishRound()
{
	APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this);
	const int32 Tenths = FMath::FloorToInt(Clock * 10.f);
	const int32 OldBest = Game ? Game->GetBestRoundTenths() : 0;
	const int32 Medal = Clock <= Par ? 3 : Clock <= Par * VargaRun::SilverShare ? 2 : Clock <= Par * VargaRun::BronzeShare ? 1 : 0;
	const int32 Pay = VargaRun::RoundPay + VargaRun::MedalPay[Medal];
	StopRound();
	if (Game)
	{
		Game->AddMoney(Pay);
		Game->NoteRound(Tenths, Medal);
		Game->AddRenown(StreetName::ForDeliveryRound, TEXT("kihord\u00E1s Varg\u00E1nak"), TEXT("a delivery round for Varga"));
	}
	const bool bRecord = OldBest <= 0 || Tenths < OldBest;
	Note(FString::Printf(TEXT("Id\u0151 / Time %s   \u00B7   %s / %s%s"), *FormatTenths(Tenths), VargaRun::MedalHu[Medal], VargaRun::MedalEn[Medal],
		bRecord ? TEXT("   \u00B7   \u00DAj cs\u00FAcs! / New best!") : TEXT("")), 8.f);
	const FString Money = APestiAlvilagGameModeBase::FormatMoney(Pay);
	if (Medal >= 3)
	{
		VargaSays(FString::Printf(TEXT("Ez igen, mint a sz\u00E9l! Itt a %s, meg\u00E9rdemled."), *Money),
			FString::Printf(TEXT("Now that's like the wind! Here's %s, you've earned it."), *Money));
	}
	else if (Medal > 0)
	{
		VargaSays(FString::Printf(TEXT("Nem rossz. Itt a %s."), *Money), FString::Printf(TEXT("Not bad. Here's %s."), *Money));
	}
	else
	{
		VargaSays(FString::Printf(TEXT("Lass\u00FA voltal, fiam, a vev\u0151k morogtak. Itt a %s."), *Money),
			FString::Printf(TEXT("You were slow, son; the customers grumbled. Here's %s."), *Money));
	}
}

void AVargaRuns::StopRound()
{
	bRunning = false;
	ClearReceivers();
	Marker->Hide();
	Guide->Stop();
	Note(FString(), 0.1f);
}
