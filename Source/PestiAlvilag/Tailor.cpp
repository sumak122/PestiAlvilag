// Fill out your copyright notice in the Description page of Project Settings.

#include "Tailor.h"

#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "PersonModel.h"
#include "PestiAlvilagGameModeBase.h"
#include "PestiHud.h"
#include "PestiPerson.h"
#include "ShapeSet.h"
#include "StoryPerson.h"
#include "TailorModel.h"

namespace TailorPlace
{
	// The shop is drawn from this far off (cm).
	constexpr float DrawDistance = 12000.f;
	// The player is at the counter this near the buying spot (cm).
	constexpr double CounterNear = 150.0;
	constexpr int32 NoteKey = 83;
	constexpr float NoteSeconds = 4.f;
	constexpr double NoteEvery = 3.5;
	constexpr float SaySeconds = 6.f;
}

ATailor::ATailor()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

FVector ATailor::BuySpot(const UWorld* World)
{
	if (World)
	{
		for (TActorIterator<ATailor> It(World); It; ++It)
		{
			return It->GetActorTransform().TransformPosition(TailorModel::BuySpot);
		}
	}
	return FVector::ZeroVector;
}

void ATailor::BeginPlay()
{
	Super::BeginPlay();

	const FTransform Here = GetActorTransform();
	TArray<MarketModel::FShape> Shapes;
	TArray<BozsiHouseModel::FLamp> Lamps;
	TArray<BozsiHouseModel::FSign> Signs;
	TailorModel::Build(Shapes, Lamps);
	TailorModel::Signs(Signs);
	Shop = GetWorld()->SpawnActor<AShapeSet>(AShapeSet::StaticClass(), Here.GetLocation(), Here.Rotator());
	if (Shop)
	{
		Shop->SetDrawDistance(TailorPlace::DrawDistance);
		Shop->AddShapes(Shapes);
		for (const BozsiHouseModel::FLamp& Lamp : Lamps)
		{
			Shop->AddLamp(Lamp.At, Lamp.Radius);
		}
		for (const BozsiHouseModel::FSign& Sign : Signs)
		{
			Shop->AddSign(Sign.Text, Sign.At, Sign.Yaw, Sign.Height, Sign.Color);
		}
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const FVector At = Here.TransformPosition(TailorModel::TailorSpot) + FVector(0.0, 0.0, PersonModel::MiddleHeight);
	Kertesz = GetWorld()->SpawnActor<AStoryPerson>(AStoryPerson::StaticClass(), At, FRotator(0.0, Here.Rotator().Yaw + TailorModel::TailorYaw, 0.0), Params);
	if (Kertesz)
	{
		Kertesz->SetOutfit(TailorModel::Tailor());
	}
}

void ATailor::KerteszSays(const FString& Hu, const FString& En) const
{
	if (APestiHUD* Hud = APestiHUD::Get(this))
	{
		Hud->Say(TEXT("Kert\u00e9sz"), Hu, En, TailorPlace::SaySeconds);
	}
}

void ATailor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	APestiPerson* Person = Cast<APestiPerson>(UGameplayStatics::GetPlayerPawn(this, 0));
	APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this);
	const FVector Counter = GetActorTransform().TransformPosition(TailorModel::BuySpot);
	const bool bThere = Person && Game && FVector::Dist2D(Person->GetActorLocation(), Counter) < TailorPlace::CounterNear;
	if (!bThere)
	{
		bAtCounter = false;
		return;
	}
	if (!bAtCounter)
	{
		bAtCounter = true;
		if (Kertesz)
		{
			Kertesz->FaceTowards(Person->GetActorLocation());
		}
		if (Game->HasSuit())
		{
			KerteszSays(TEXT("J\u00f3 napot, uram! Hogy \u00e1ll a zak\u00f3? Ugye mondtam, hogy kiv\u00e1l\u00f3an."),
				TEXT("Good day, sir! How is the jacket wearing? Didn't I say it would fit perfectly."));
		}
		else
		{
			KerteszSays(TEXT("Parancsoljon, uram! Egy j\u00f3 \u00f6lt\u00f6ny csod\u00e1t tesz: aki l\u00e1tja, r\u00f6gt\u00f6n \u00farnak n\u00e9zi."),
				TEXT("At your service, sir! A good suit works wonders: whoever sees you takes you for a gentleman."));
		}
	}
	if (Game->HasSuit())
	{
		return;
	}

	const int32 Price = APestiAlvilagGameModeBase::PriceFor(this, TailorModel::SuitPrice);
	const double Now = GetWorld()->GetTimeSeconds();
	const APlayerController* PC = Cast<APlayerController>(Person->GetController());
	if (PC && PC->WasInputKeyJustPressed(EKeys::E))
	{
		if (Game->GetMoney() < Price)
		{
			KerteszSays(TEXT("Sajn\u00e1lom, uram, hitelbe nem varrok. J\u00f6jj\u00f6n vissza, ha \u00f6sszegy\u0171lt a p\u00e9nz."),
				TEXT("I'm sorry, sir, I don't sew on credit. Come back when you have the money."));
			return;
		}
		Game->AddMoney(-Price);
		Game->SetHasSuit(true);
		Person->RefreshDress();
		KerteszSays(TEXT("Mintha \u00f6nre szabt\u00e1k volna! Mert \u00f6nre is szabtuk. Viselje eg\u00e9szs\u00e9ggel, uram."),
			TEXT("As if it was made for you! Because it was. Wear it in good health, sir."));
		return;
	}
	if (Now - LastNote > TailorPlace::NoteEvery)
	{
		LastNote = Now;
		const FString Money = APestiAlvilagGameModeBase::FormatMoney(Price);
		APestiHUD::Show(this, TailorPlace::NoteKey, TailorPlace::NoteSeconds, FColor(255, 210, 60),
			FString(TEXT("E: \u00f6lt\u00f6ny, ")) + Money + TEXT("  (E: a suit, ") + Money + TEXT(")"));
	}
}
