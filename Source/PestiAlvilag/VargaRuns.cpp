// Fill out your copyright notice in the Description page of Project Settings.

#include "VargaRuns.h"

#include "Components/SceneComponent.h"
#include "DeliveryModel.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GoalMarkerComponent.h"
#include "Mission.h"
#include "PersonModel.h"
#include "PestiAlvilagGameModeBase.h"
#include "PestiHud.h"
#include "PestiPerson.h"
#include "PestiText.h"
#include "RouteGuideComponent.h"
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
	// The addresses are by the kerb, this far left of a lane (cm).
	constexpr double KerbOffset = 260.0;
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
	if (Next < Drops.Num())
	{
		const FVector Drop = Drops[Next];
		Marker->ShowAt(Drop);
		Guide->Guide(At, Drop);
		Note(FString::Printf(TEXT("Csomag %d/%d: %s  (Parcel %d/%d)   %s %s   Id\u0151 %s%s"), Next + 1, Drops.Num(), *Streets[Next], Next + 1, Drops.Num(),
			*PestiText::Metres(FVector::Dist2D(At, Drop)), PestiText::Compass(At, Drop), *FormatTenths(Tenths), *BestText), 0.f);
		if (FVector::Dist2D(At, Drop) < VargaRun::DropRadius && FMath::Abs(At.Z - Drop.Z) < 400.0)
		{
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

bool AVargaRuns::StartRound()
{
	const ATrafficManager* Traffic = nullptr;
	{
		TActorIterator<ATrafficManager> It(GetWorld());
		Traffic = It ? *It : nullptr;
	}
	if (!Traffic || Traffic->NumLanes() == 0)
	{
		return false;
	}
	Drops.Reset();
	Streets.Reset();
	FCollisionQueryParams Params(SCENE_QUERY_STAT(VargaDrop), false, this);
	for (int32 Try = 0; Try < 400 && Drops.Num() < 3; ++Try)
	{
		const int32 LaneIndex = FMath::RandRange(0, Traffic->NumLanes() - 1);
		const FString Street = ATaxiCompany::PeriodStreetName(Traffic->FindStreetName(LaneIndex));
		const TArray<FVector>& Points = Traffic->GetLane(LaneIndex).Points;
		if (Street.IsEmpty() || Points.Num() < 2 || Streets.Contains(Street))
		{
			continue;
		}
		const int32 p = FMath::RandRange(1, Points.Num() - 1);
		const double Away = FVector::Dist2D(Points[p], ShopAt);
		if (Away < VargaRun::MinAway || Away > VargaRun::MaxAway)
		{
			continue;
		}
		bool bTooClose = false;
		for (const FVector& Other : Drops)
		{
			bTooClose = bTooClose || FVector::Dist2D(Other, Points[p]) < VargaRun::ApartFrom;
		}
		if (bTooClose)
		{
			continue;
		}
		// By the kerb on the left of the lane, on the pavement's ground.
		const FVector Ahead = (Points[p] - Points[p - 1]).GetSafeNormal2D();
		const FVector Left(Ahead.Y, -Ahead.X, 0.0);
		const FVector Spot = Points[p] + Left * VargaRun::KerbOffset;
		FHitResult Ground;
		if (!GetWorld()->LineTraceSingleByObjectType(Ground, Spot + FVector(0.0, 0.0, 5000.0), Spot - FVector(0.0, 0.0, 5000.0),
			FCollisionObjectQueryParams(ECC_WorldStatic), Params))
		{
			continue;
		}
		Drops.Add(Ground.ImpactPoint);
		Streets.Add(Street);
	}
	if (Drops.Num() < 3)
	{
		VargaSays(TEXT("Ma nincs kihordanival\u00F3, fiam."), TEXT("Nothing to deliver today, son."));
		return false;
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
	VargaSays(FString::Printf(TEXT("H\u00E1rom csomag: %s, %s \u00E9s %s. Siess, a vev\u0151 nem v\u00E1r! Ott a bicikli."), *Streets[0], *Streets[1], *Streets[2]),
		FString::Printf(TEXT("Three parcels: %s, %s and %s. Hurry, customers don't wait! There's the bicycle."), *Streets[0], *Streets[1], *Streets[2]));
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
	Marker->Hide();
	Guide->Stop();
	Note(FString(), 0.1f);
}
