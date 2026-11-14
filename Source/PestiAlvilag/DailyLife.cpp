// Fill out your copyright notice in the Description page of Project Settings.

#include "DailyLife.h"

#include "Camera/PlayerCameraManager.h"
#include "CarBody.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DayNight.h"
#include "Dom/JsonObject.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "FireEngine.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Pedestrians.h"
#include "PestiHud.h"
#include "PestiPerson.h"
#include "ProceduralMeshComponent.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "ShopModel.h"
#include "SpotSound.h"
#include "TrafficCart.h"
#include "TrafficManager.h"
#include "Tram.h"
#include "UObject/ConstructorHelpers.h"
#include "Weather.h"

namespace DailyLifeRun
{
	// The map's files.
	const TCHAR* const FurnitureFile = TEXT("Data/OsmGraybox/street_furniture.json");
	const TCHAR* const DetailsFile = TEXT("Data/OsmGraybox/city_details.json");

	// A person's middle over his feet (cm).
	constexpr double MiddleUp = 90.0;
	// Seen: nearer than SeeFar, within the view and not behind a building; always seen this near (cm).
	constexpr double SeeFar = 16000.0;
	constexpr double SeeNear = 500.0;
	constexpr double ViewCos = 0.3;
	// A line to be read is only said this near the player (cm).
	constexpr double HearSay = 900.0;
	constexpr float SaySeconds = 3.5f;

	// The lamplighter: he sets out once it gets this dark, gives up when it is night (all lit at once), starts over by morning.
	constexpr float RoundDarkFrom = 0.12f;
	constexpr float RoundDarkTo = 0.75f;
	constexpr float RoundReset = 0.06f;
	// Most dusks he comes by.
	constexpr float RoundShare = 0.75f;
	// The lamps of his round: this near the player, this far apart at most, this many.
	constexpr double RoundReach = 6000.0;
	constexpr double RoundStep = 3200.0;
	constexpr int32 RoundMost = 12;
	// He starts at a lamp at least this far from the player.
	constexpr double RoundStartFrom = 1500.0;
	constexpr float LighterSpeed = 125.f;
	// At each lamp: the pole up, held up, down again, and the lamp lit this long after he got there (s).
	constexpr float PoleRaise = 1.2f;
	constexpr float PoleHold = 1.4f;
	constexpr float PoleLower = 1.0f;
	constexpr float PoleLight = 1.7f;
	// Where he stands: back from the lantern towards the post, and to the left so his right hand is under it (cm).
	constexpr double StandBack = 30.0;
	constexpr double StandLeft = 22.0;
	// The pole leans a little forward to the lantern (degrees).
	constexpr float PoleLean = 7.f;
	// The player this far off and he is not seen: the round is done at once (cm); and at most this long (s).
	constexpr double RoundGiveUp = 15000.0;
	constexpr double RoundLongest = 600.0;
	// The dark cover over a lantern not lit yet, a little bigger than its glass.
	constexpr double CoverGrow = 1.08;

	// Courtyards: by day, dry, near; a share of them each day, two at once at most.
	constexpr float YardFrom = 8.f;
	constexpr float YardTo = 17.5f;
	constexpr double YardNear = 4500.0;
	constexpr double YardFar = 7000.0;
	constexpr float YardShare = 0.4f;
	constexpr int32 YardsAtOnce = 2;
	// Beats a second, pegs and sheets, the football.
	constexpr float BeatsPerSecond = 1.1f;
	constexpr int32 SheetsToPeg = 4;
	constexpr double PegGap = 110.0;
	constexpr float PegSeconds = 3.5f;
	constexpr float BallFriction = 1.1f;
	constexpr float KickSeconds = 0.5f;
	constexpr float KidRun = 280.f;
	constexpr double KidReach = 45.0;
	constexpr double PlayerKick = 75.0;

	// Street people: at most this far (and at least this far, so they come unseen), and their share of places each day.
	constexpr double SellerFrom = 3000.0;
	constexpr double SellerTo = 11000.0;
	constexpr double SellerFar = 13000.0;
	constexpr float OrganShare = 0.3f;
	constexpr float GrinderShare = 0.08f;
	constexpr float ChestnutShare = 0.6f;
	// Heard this far (cm).
	constexpr double OrganReach = 4500.0;
	constexpr double GrinderReach = 2500.0;
	constexpr double FiddleReach = 5000.0;

	// Rare events: one now and then (s), and not every time.
	constexpr float EventEveryFrom = 480.f;
	constexpr float EventEveryTo = 900.f;
	constexpr float EventShare = 0.6f;
	constexpr float FirstEventFrom = 240.f;
	constexpr float FirstEventTo = 420.f;
	// Walking in a column (cm/s); the mourners keep this far behind the hearse (cm).
	constexpr float ColumnWalk = 100.f;
	constexpr double HearseGap = 380.0;
	// Over when this far from the player, or this long (s); gone at last this long after (s).
	constexpr double EventFar = 14000.0;
	constexpr double WeddingLongest = 600.0;
	constexpr double FuneralLongest = 480.0;
	constexpr double FireLongest = 240.0;
	constexpr double EndingLongest = 150.0;
	// The hearse is there this near the cemetery gate (cm).
	constexpr double GateNear = 1500.0;
	// The way passes this near the player (cm).
	constexpr double PassNear = 3000.0;
	// Road block slots (the fire engine keeps 3).
	constexpr int32 WeddingSlot = 1;
	constexpr int32 FuneralSlot = 2;

	// Trams: a stop this near, people get on and off at a share of them.
	constexpr double TramNear = 6000.0;
	constexpr double WaitNear = 2500.0;
	constexpr float TramShare = 0.6f;
	constexpr int32 RidersAtOnce = 8;
	constexpr double DoorReach = 40.0;
	constexpr double RiderLongest = 120.0;

	// Shutters: night is from 20:00 until each shop opens (7 to 9); shops this near, a share of them have shutters.
	constexpr float ShutFrom = 20.f;
	constexpr float OpenFrom = 7.f;
	constexpr float OpenSpread = 2.f;
	constexpr float OpenLatest = 10.f;
	constexpr double ShopNear = 3500.0;
	constexpr double ShopFar = 6000.0;
	constexpr float ShutterShare = 0.55f;
	constexpr int32 ShuttersAtOnce = 30;
	constexpr float LiftSeconds = 4.f;
	// The keeper comes from this far along the pavement and stands here to push it up (shop frame, cm).
	constexpr double KeeperFrom = 900.0;
	const FVector2D KeeperPush(110.0, -40.0);
	constexpr float KeeperWalk = 120.f;

	inline FVector2D Flat(const FVector& V) { return FVector2D(V.X, V.Y); }
	inline FVector2D Heading(double Degrees)
	{
		const double R = FMath::DegreesToRadians(Degrees);
		return FVector2D(FMath::Cos(R), FMath::Sin(R));
	}
	inline double YawOf(const FVector2D& V) { return FMath::RadiansToDegrees(FMath::Atan2(V.Y, V.X)); }
	/** A point in a frame at Foot turned Turn degrees: X along, Y to the right. */
	inline FVector2D InFrame(const FVector2D& Foot, double Turn, double X, double Y)
	{
		const FVector2D Along = Heading(Turn);
		const FVector2D Right(-Along.Y, Along.X);
		return Foot + Along * X + Right * Y;
	}
	/** A shape's turn in the world: about the yaw, then its own pitch/roll. */
	inline FQuat Turned(double TurnYaw, const FRotator& Own)
	{
		return FQuat(FRotator(0.0, TurnYaw, 0.0)) * FQuat(Own);
	}

	TSharedPtr<FJsonObject> ReadJson(const TCHAR* File)
	{
		FString Text;
		TSharedPtr<FJsonObject> Root;
		const FString Path = FPaths::Combine(FPaths::ProjectDir(), File);
		if (!FFileHelper::LoadFileToString(Text, *Path) || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root))
		{
			return nullptr;
		}
		return Root;
	}

	inline double NumberAt(const TArray<TSharedPtr<FJsonValue>>& Items, int32 Index)
	{
		return Items.IsValidIndex(Index) && Items[Index].IsValid() ? Items[Index]->AsNumber() : 0.0;
	}
}

ADailyLife::ADailyLife()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	BaseMaterial = CarBody::LoadPaintMaterial();
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	CubeMesh = Cube.Object;
}

void ADailyLife::BeginPlay()
{
	Super::BeginPlay();
	Dice.Initialize(FMath::Rand());
	DaySeed = static_cast<uint32>(FMath::Rand());
	for (int32 k = 0; k < static_cast<int32>(ESeller::Count); ++k)
	{
		Sellers[k].Kind = static_cast<ESeller>(k);
		Sellers[k].NextTry = 20.0 + 10.0 * k;
	}
	NextEvent = GetWorld()->GetTimeSeconds() + Dice.FRandRange(DailyLifeRun::FirstEventFrom, DailyLifeRun::FirstEventTo);
	LoadPlaces();
}

void ADailyLife::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	EndRound(true);
	for (FYardScene& Scene : YardScenes)
	{
		EndYardScene(Scene);
	}
	YardScenes.Reset();
	for (FSeller& Seller : Sellers)
	{
		EndSeller(Seller);
	}
	EndEvent();
	for (FShutter& Shutter : Shutters)
	{
		RemoveShutter(Shutter);
	}
	Shutters.Reset();
	for (APedestrian* Person : People)
	{
		if (IsValid(Person))
		{
			Person->Destroy();
		}
	}
	People.Reset();
	Riders.Reset();
	Super::EndPlay(EndPlayReason);
}

void ADailyLife::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Now = GetWorld()->GetTimeSeconds();
	bHasCamera = CameraView(Eye, Look);
	if (!bHasCamera)
	{
		return;
	}
	if (const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		PlayerAt = Player->GetActorLocation();
	}
	else
	{
		PlayerAt = Eye;
	}
	GroundGuess = PlayerAt.Z - 100.0;

	// A new day at midnight.
	const float Hour = HourNow();
	if (HourBefore >= 0.f && Hour < HourBefore - 12.f)
	{
		++Days;
	}
	HourBefore = Hour;

	TickLamplighter(DeltaSeconds);
	TickCourtyards(DeltaSeconds);
	TickSellers(DeltaSeconds);
	TickEvents(DeltaSeconds);
	TickTrams(DeltaSeconds);
	TickShutters(DeltaSeconds);
}

// ---- Shared ------------------------------------------------------------------

bool ADailyLife::CameraView(FVector& OutEye, FVector& OutLook) const
{
	const APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0);
	if (!Camera)
	{
		return false;
	}
	OutEye = Camera->GetCameraLocation();
	OutLook = Camera->GetCameraRotation().Vector();
	return true;
}

bool ADailyLife::IsSeen(const FVector& At) const
{
	const FVector Target = At + FVector(0.0, 0.0, 120.0);
	const FVector To = Target - Eye;
	const double Distance = To.Size();
	if (Distance < DailyLifeRun::SeeNear)
	{
		return true;
	}
	if (Distance > DailyLifeRun::SeeFar || FVector::DotProduct(To / Distance, Look) < DailyLifeRun::ViewCos)
	{
		return false;
	}
	FHitResult Hit;
	FCollisionQueryParams Query(SCENE_QUERY_STAT(DailyLifeSeen), false, this);
	if (GetWorld()->LineTraceSingleByObjectType(Hit, Eye, Target, FCollisionObjectQueryParams(ECC_WorldStatic), Query))
	{
		return Hit.Distance > Distance - 150.0;
	}
	return true;
}

bool ADailyLife::GroundAt(const FVector2D& At, double& OutZ) const
{
	FHitResult Hit;
	FCollisionQueryParams Query(SCENE_QUERY_STAT(DailyLifeGround), false, this);
	const FVector Top(At.X, At.Y, GroundGuess + 300.0);
	const FVector Bottom(At.X, At.Y, GroundGuess - 1500.0);
	if (GetWorld()->LineTraceSingleByObjectType(Hit, Top, Bottom, FCollisionObjectQueryParams(ECC_WorldStatic), Query)
		&& Hit.ImpactPoint.Z < GroundGuess + 220.0)
	{
		OutZ = Hit.ImpactPoint.Z;
		return true;
	}
	OutZ = GroundGuess;
	return false;
}

APedestrian* ADailyLife::MakePerson(const FVector& Ground, double Yaw, const PersonKit::FOutfit& Outfit)
{
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	APedestrian* Person = GetWorld()->SpawnActor<APedestrian>(APedestrian::StaticClass(), Ground + FVector(0.0, 0.0, DailyLifeRun::MiddleUp),
		FRotator(0.0, Yaw, 0.0), Params);
	if (!Person)
	{
		return nullptr;
	}
	Person->SetOutfit(Outfit);
	People.Add(Person);
	return Person;
}

void ADailyLife::RemovePerson(APedestrian* Person)
{
	if (!Person)
	{
		return;
	}
	People.Remove(Person);
	if (IsValid(Person))
	{
		Person->Destroy();
	}
}

UProceduralMeshComponent* ADailyLife::MakeMesh(const PersonKit::FPiece& Shape, const FTransform& Place)
{
	UProceduralMeshComponent* Mesh = NewObject<UProceduralMeshComponent>(this);
	Mesh->SetupAttachment(RootComponent);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCanEverAffectNavigation(false);
	// Some of them move: all kept out of the ray-traced lighting and the distance fields.
	Mesh->bVisibleInRayTracing = false;
	Mesh->bAffectDistanceFieldLighting = false;
	for (int32 s = 0; s < Shape.Sections.Num(); ++s)
	{
		const PersonKit::FSection& Section = Shape.Sections[s];
		Mesh->CreateMeshSection_LinearColor(s, Section.Vertices, Section.Triangles, Section.Normals,
			TArray<FVector2D>(), TArray<FLinearColor>(), TArray<FProcMeshTangent>(), false);
		if (BaseMaterial)
		{
			UMaterialInstanceDynamic* Paint = UMaterialInstanceDynamic::Create(BaseMaterial, this);
			Paint->SetVectorParameterValue(TEXT("Color"), Section.Color);
			Mesh->SetMaterial(s, Paint);
		}
	}
	Mesh->RegisterComponent();
	AddInstanceComponent(Mesh);
	Mesh->SetWorldTransform(Place);
	return Mesh;
}

void ADailyLife::RemoveMesh(UProceduralMeshComponent* Mesh)
{
	if (IsValid(Mesh))
	{
		RemoveInstanceComponent(Mesh);
		Mesh->DestroyComponent();
	}
}

USpotSound* ADailyLife::MakeSound(CitySynth::ESpot Kind, const FVector& At)
{
	USpotSound* Sound = NewObject<USpotSound>(this);
	Sound->SetupAttachment(RootComponent);
	Sound->RegisterComponent();
	Sound->SetWorldLocation(At);
	Sound->SetSpot(Kind, FMath::Rand());
	Sound->SetHeard(0.f, 1.f);
	Sound->SetHour(HourNow());
	Sound->Start();
	return Sound;
}

void ADailyLife::Hear(USpotSound* Sound, double Reach) const
{
	if (!IsValid(Sound))
	{
		return;
	}
	const double Distance = FVector::Dist(Eye, Sound->GetComponentLocation());
	const float Far = static_cast<float>(FMath::Clamp(Distance / Reach, 0.0, 1.0));
	Sound->SetHeard(Distance < Reach ? 1.f - Far : 0.f, Far);
}

void ADailyLife::Say(const FString& Who, const FString& Hu, const FString& En) const
{
	if (APestiHUD* Hud = APestiHUD::Get(this))
	{
		if (!Hud->IsSaying())
		{
			Hud->Say(Who, Hu, En, DailyLifeRun::SaySeconds);
		}
	}
}

float ADailyLife::HourNow() const
{
	const ADayNight* Sky = ADayNight::Get(GetWorld());
	return Sky ? Sky->GetHour() : 12.f;
}

float ADailyLife::DarkNow() const
{
	const ADayNight* Sky = ADayNight::Get(GetWorld());
	return Sky ? Sky->GetDarkness() : 0.f;
}

float ADailyLife::RainNow() const
{
	const AWeather* Rain = AWeather::Get(GetWorld());
	return Rain ? Rain->GetRain() : 0.f;
}

int32 ADailyLife::DayNumber() const
{
	return Days;
}

float ADailyLife::DayChance(int32 Kind, int32 Index) const
{
	const uint32 Mixed = HashCombine(HashCombine(GetTypeHash(DayNumber()), GetTypeHash(Kind)), HashCombine(GetTypeHash(Index), DaySeed));
	FRandomStream Pick(static_cast<int32>(Mixed));
	Pick.FRand();
	return Pick.FRand();
}

ATrafficManager* ADailyLife::Traffic() const
{
	if (!TrafficFound.IsValid())
	{
		for (TActorIterator<ATrafficManager> It(GetWorld()); It; ++It)
		{
			TrafficFound = *It;
			break;
		}
	}
	return TrafficFound.Get();
}

// ---- Places ------------------------------------------------------------------

void ADailyLife::LoadPlaces()
{
	using namespace DailyLifeRun;
	if (const TSharedPtr<FJsonObject> Furniture = ReadJson(FurnitureFile))
	{
		const TArray<TSharedPtr<FJsonValue>>* List = nullptr;
		if (Furniture->TryGetArrayField(TEXT("shops"), List))
		{
			for (const TSharedPtr<FJsonValue>& Item : *List)
			{
				const TSharedPtr<FJsonObject> Shop = Item.IsValid() ? Item->AsObject() : nullptr;
				const TArray<TSharedPtr<FJsonValue>>* At = nullptr;
				if (!Shop.IsValid() || !Shop->TryGetArrayField(TEXT("at"), At) || At->Num() < 3)
				{
					continue;
				}
				FShopFront Front;
				Front.Foot = FVector2D(NumberAt(*At, 0), NumberAt(*At, 1));
				Front.Yaw = NumberAt(*At, 2);
				Front.Width = Shop->GetNumberField(TEXT("width"));
				Front.Seed = static_cast<uint32>(Shop->GetNumberField(TEXT("seed")));
				bool bShut = false;
				Shop->TryGetBoolField(TEXT("closed"), bShut);
				Front.bClosed = bShut;
				Shops.Add(Front);
			}
		}
		if (Furniture->TryGetArrayField(TEXT("tram_stops"), List))
		{
			for (const TSharedPtr<FJsonValue>& Item : *List)
			{
				const TSharedPtr<FJsonObject> Stop = Item.IsValid() ? Item->AsObject() : nullptr;
				const TArray<TSharedPtr<FJsonValue>>* Wait = nullptr;
				if (Stop.IsValid() && Stop->TryGetArrayField(TEXT("wait"), Wait) && Wait->Num() >= 2)
				{
					TramWaits.Add(FVector2D(NumberAt(*Wait, 0), NumberAt(*Wait, 1)));
				}
			}
		}
	}

	const TSharedPtr<FJsonObject> Details = ReadJson(DetailsFile);
	if (!Details.IsValid())
	{
		return;
	}
	const TArray<TSharedPtr<FJsonValue>>* List = nullptr;
	if (Details->TryGetArrayField(TEXT("courtyards"), List))
	{
		for (const TSharedPtr<FJsonValue>& Item : *List)
		{
			const TSharedPtr<FJsonObject> Data = Item.IsValid() ? Item->AsObject() : nullptr;
			const TArray<TSharedPtr<FJsonValue>>* Gate = nullptr;
			const TArray<TSharedPtr<FJsonValue>>* Corners = nullptr;
			if (!Data.IsValid() || !Data->TryGetArrayField(TEXT("gate"), Gate) || Gate->Num() < 3
				|| !Data->TryGetArrayField(TEXT("yard"), Corners) || Corners->Num() < 6)
			{
				continue;
			}
			FCourtyard Yard;
			Yard.Gate = FVector2D(NumberAt(*Gate, 0), NumberAt(*Gate, 1));
			Yard.GateYaw = NumberAt(*Gate, 2);
			Yard.Depth = Data->GetNumberField(TEXT("depth"));
			for (int32 i = 0; i + 1 < Corners->Num(); i += 2)
			{
				Yard.Outline.Add(FVector2D(NumberAt(*Corners, i), NumberAt(*Corners, i + 1)));
			}
			for (const FVector2D& Corner : Yard.Outline)
			{
				Yard.Middle += Corner;
			}
			Yard.Middle /= static_cast<double>(Yard.Outline.Num());
			const TArray<TSharedPtr<FJsonValue>>* Props = nullptr;
			if (Data->TryGetArrayField(TEXT("props"), Props))
			{
				for (const TSharedPtr<FJsonValue>& PropValue : *Props)
				{
					const TArray<TSharedPtr<FJsonValue>>* Row = nullptr;
					if (!PropValue.IsValid() || !PropValue->TryGetArray(Row) || Row->Num() < 4)
					{
						continue;
					}
					FYardThing Thing;
					Thing.Kind = (*Row)[0]->AsString();
					Thing.At = FVector2D(NumberAt(*Row, 1), NumberAt(*Row, 2));
					Thing.Yaw = NumberAt(*Row, 3);
					Thing.Length = Row->Num() > 4 ? NumberAt(*Row, 4) : 0.0;
					Yard.Things.Add(Thing);
				}
			}
			Yards.Add(Yard);
		}
	}
	if (Details->TryGetArrayField(TEXT("kiosks"), List))
	{
		for (const TSharedPtr<FJsonValue>& Item : *List)
		{
			const TArray<TSharedPtr<FJsonValue>>* Row = nullptr;
			if (Item.IsValid() && Item->TryGetArray(Row) && Row->Num() >= 3)
			{
				Kiosks.Add(FVector(NumberAt(*Row, 0), NumberAt(*Row, 1), NumberAt(*Row, 2)));
			}
		}
	}
	if (Details->TryGetArrayField(TEXT("churches"), List))
	{
		for (const TSharedPtr<FJsonValue>& Item : *List)
		{
			const TArray<TSharedPtr<FJsonValue>>* Row = nullptr;
			if (Item.IsValid() && Item->TryGetArray(Row) && Row->Num() >= 4)
			{
				// The middle of its front, facing out: the door is a little out from it.
				ChurchDoors.Add(FVector2D(NumberAt(*Row, 1), NumberAt(*Row, 2)) + Heading(NumberAt(*Row, 3)) * 250.0);
			}
		}
	}
	const TSharedPtr<FJsonObject>* Cemetery = nullptr;
	if (Details->TryGetObjectField(TEXT("cemetery"), Cemetery) && Cemetery->IsValid())
	{
		const TArray<TSharedPtr<FJsonValue>>* Gates = nullptr;
		if ((*Cemetery)->TryGetArrayField(TEXT("gates"), Gates))
		{
			for (const TSharedPtr<FJsonValue>& Item : *Gates)
			{
				const TArray<TSharedPtr<FJsonValue>>* Row = nullptr;
				if (Item.IsValid() && Item->TryGetArray(Row) && Row->Num() >= 3 && (*Row)[0]->AsString() == TEXT("main"))
				{
					CemeteryGate = FVector2D(NumberAt(*Row, 1), NumberAt(*Row, 2));
					bHasCemetery = true;
				}
			}
		}
	}
}

bool ADailyLife::IsInside(const TArray<FVector2D>& Outline, const FVector2D& At)
{
	bool bInside = false;
	for (int32 i = 0, j = Outline.Num() - 1; i < Outline.Num(); j = i++)
	{
		const FVector2D& A = Outline[i];
		const FVector2D& B = Outline[j];
		if ((A.Y > At.Y) != (B.Y > At.Y) && At.X < (B.X - A.X) * (At.Y - A.Y) / (B.Y - A.Y) + A.X)
		{
			bInside = !bInside;
		}
	}
	return bInside;
}

FVector ADailyLife::PathPoint(const TArray<FVector>& Path, const TArray<double>& Along, double S, FVector& OutDir)
{
	OutDir = FVector::ForwardVector;
	if (Path.Num() == 0)
	{
		return FVector::ZeroVector;
	}
	if (Path.Num() == 1 || Along.Num() != Path.Num())
	{
		return Path[0];
	}
	const int32 Last = Path.Num() - 1;
	int32 Seg = 0;
	while (Seg < Last - 1 && Along[Seg + 1] < S)
	{
		++Seg;
	}
	const double SegLength = FMath::Max(Along[Seg + 1] - Along[Seg], 1.0);
	const double F = FMath::Clamp((S - Along[Seg]) / SegLength, 0.0, 1.0);
	const FVector A = Path[Seg];
	const FVector B = Path[Seg + 1];
	OutDir = (B - A).GetSafeNormal2D();
	if (OutDir.IsNearlyZero())
	{
		OutDir = FVector::ForwardVector;
	}
	return A + (B - A) * F;
}

void ADailyLife::Measure(const TArray<FVector>& Path, TArray<double>& OutAlong)
{
	OutAlong.Reset();
	double Sum = 0.0;
	for (int32 i = 0; i < Path.Num(); ++i)
	{
		if (i > 0)
		{
			Sum += FVector::Dist2D(Path[i - 1], Path[i]);
		}
		OutAlong.Add(Sum);
	}
}

// ---- The lamplighter ---------------------------------------------------------

void ADailyLife::TickLamplighter(float DeltaSeconds)
{
	using namespace DailyLifeRun;
	ADayNight* Sky = ADayNight::Get(GetWorld());
	if (!Sky)
	{
		return;
	}
	const float Dark = Sky->GetDarkness();

	if (Round == ERound::Waiting)
	{
		// At dusk, before the lamps come on.
		if (Dark >= RoundDarkFrom && !Sky->AreLampsLit() && Now >= NextRoundTry)
		{
			if (DayChance(0, 0) >= RoundShare)
			{
				Round = ERound::Done;
			}
			else if (StartRound())
			{
				Round = ERound::Working;
			}
			else
			{
				NextRoundTry = Now + 15.0;
			}
		}
		return;
	}

	APedestrian* Man = Lamplighter.Get();
	if (Round == ERound::Done)
	{
		// Off he goes after his round, on the way he walked last, until nobody sees him.
		if (Man)
		{
			LeaveClock += DeltaSeconds;
			const FVector Middle(LighterAt, LighterGround + MiddleUp);
			if ((LeaveClock > 3.f && !IsSeen(Middle)) || LeaveClock > 90.f)
			{
				RemovePerson(Man);
				Lamplighter.Reset();
			}
			else
			{
				LighterAt += Heading(WalkYaw) * (LighterSpeed * DeltaSeconds);
				LighterGroundClock -= DeltaSeconds;
				if (LighterGroundClock <= 0.f)
				{
					LighterGroundClock = 0.3f;
					GroundAt(LighterAt, LighterGround);
				}
				Man->WalkTo(FVector(LighterAt, LighterGround + MiddleUp), WalkYaw, LighterSpeed, DeltaSeconds);
				DailyLifeModel::FLimbs Limbs;
				DailyLifeModel::PoleCarry(Limbs);
				Man->ArmsAs(Limbs);
				if (UProceduralMeshComponent* PoleMesh = LampPole.Get())
				{
					PoleMesh->SetRelativeRotation(DailyLifeModel::HoldUpright(DailyLifeModel::PoleCarried));
				}
			}
		}
		// A new dusk tomorrow.
		if (Dark < RoundReset)
		{
			Round = ERound::Waiting;
			NextRoundTry = 0.0;
		}
		return;
	}

	// Working: night has come, or the player is far away, and the rest light at once.
	const FVector Middle(LighterAt, LighterGround + MiddleUp);
	if (!Man || Dark > RoundDarkTo || Now - RoundStarted > RoundLongest
		|| (FVector2D::Distance(LighterAt, Flat(PlayerAt)) > RoundGiveUp && !IsSeen(Middle)))
	{
		EndRound(true);
		return;
	}
	// The lanterns still waiting look dark once the others shine.
	const bool bLit = Sky->AreLampsLit();
	for (FRoundLamp& Lamp : RoundLamps)
	{
		if (UStaticMeshComponent* Cover = Lamp.Cover.Get())
		{
			Cover->SetVisibility(bLit && !Lamp.bLit);
		}
	}
	if (!RoundLamps.IsValidIndex(RoundNext))
	{
		EndRound(false);
		return;
	}

	FRoundLamp& Lamp = RoundLamps[RoundNext];
	DailyLifeModel::FLimbs Limbs;
	if (AtLampTime < 0.f)
	{
		// Walking to the next lamp, the pole carried like a staff.
		const FVector2D Way = Lamp.Stand - LighterAt;
		const double Left = Way.Size();
		const double Step = LighterSpeed * DeltaSeconds;
		if (Left <= Step)
		{
			LighterAt = Lamp.Stand;
			AtLampTime = 0.f;
		}
		else
		{
			LighterAt += Way / Left * Step;
			WalkYaw = YawOf(Way);
			LighterYaw = WalkYaw;
		}
		LighterGroundClock -= DeltaSeconds;
		if (LighterGroundClock <= 0.f)
		{
			LighterGroundClock = 0.3f;
			GroundAt(LighterAt, LighterGround);
		}
		Man->WalkTo(FVector(LighterAt, LighterGround + MiddleUp), LighterYaw, LighterSpeed, DeltaSeconds);
		DailyLifeModel::PoleCarry(Limbs);
		Man->ArmsAs(Limbs);
		if (UProceduralMeshComponent* PoleMesh = LampPole.Get())
		{
			PoleMesh->SetRelativeRotation(DailyLifeModel::HoldUpright(DailyLifeModel::PoleCarried));
		}
		return;
	}

	// At the lamp: up with the pole, the gas lit, down again.
	AtLampTime += DeltaSeconds;
	LighterYaw = FMath::FixedTurn(static_cast<float>(LighterYaw), static_cast<float>(Lamp.FaceYaw), 360.f * DeltaSeconds);
	const float T = AtLampTime;
	float Reach = 1.f;
	if (T < PoleRaise)
	{
		Reach = FMath::SmoothStep(0.f, 1.f, T / PoleRaise);
	}
	else if (T > PoleRaise + PoleHold)
	{
		Reach = 1.f - FMath::SmoothStep(0.f, 1.f, (T - PoleRaise - PoleHold) / PoleLower);
	}
	const float Raise = DailyLifeModel::PoleReach(Limbs, Reach);
	Man->Strike(FVector(LighterAt, LighterGround + MiddleUp), LighterYaw, Limbs);
	if (UProceduralMeshComponent* PoleMesh = LampPole.Get())
	{
		PoleMesh->SetRelativeRotation(DailyLifeModel::HoldUpright(Raise, PoleLean));
	}
	if (!Lamp.bLit && T >= PoleLight)
	{
		Lamp.bLit = true;
		Sky->SetLampUnlit(Lamp.Lamp, false);
		if (UStaticMeshComponent* Cover = Lamp.Cover.Get())
		{
			Cover->DestroyComponent();
		}
		Lamp.Cover.Reset();
	}
	if (!bGreeted && FVector2D::Distance(LighterAt, Flat(PlayerAt)) < 400.0)
	{
		bGreeted = true;
		Say(TEXT("L\u00e1mpagy\u00fajt\u00f3"), TEXT("J\u00f3 est\u00e9t, fiatalember!"), TEXT("Good evening, young man!"));
	}
	if (T >= PoleRaise + PoleHold + PoleLower)
	{
		++RoundNext;
		AtLampTime = -1.f;
	}
}

bool ADailyLife::StartRound()
{
	using namespace DailyLifeRun;
	ADayNight* Sky = ADayNight::Get(GetWorld());
	if (!Sky)
	{
		return false;
	}
	const FVector2D Player2D = Flat(PlayerAt);

	// The street lamps round the player (the ones on posts, whose glass we know).
	TArray<int32> Near;
	for (int32 i = 0; i < Sky->NumLamps(); ++i)
	{
		if (Sky->IsLampOut(i) || Sky->GetLampGlass(i).GetScale3D().IsNearlyZero())
		{
			continue;
		}
		if (FVector2D::Distance(Flat(Sky->GetLampAt(i)), Player2D) < RoundReach)
		{
			Near.Add(i);
		}
	}
	if (Near.Num() < 3)
	{
		return false;
	}

	// Where he stands at each: under the crook, facing out along it.
	auto StandFor = [Sky](int32 Lamp, FVector2D& OutStand, double& OutFace)
	{
		const FTransform Glass = Sky->GetLampGlass(Lamp);
		FVector Out = Glass.GetRotation().GetForwardVector();
		Out.Z = 0.0;
		if (!Out.Normalize())
		{
			Out = FVector::ForwardVector;
		}
		const FVector2D Out2D(Out.X, Out.Y);
		const FVector2D Right2D(-Out.Y, Out.X);
		OutStand = Flat(Glass.GetLocation()) - Out2D * StandBack - Right2D * StandLeft;
		OutFace = YawOf(Out2D);
	};

	// He starts at the nearest lamp out of sight, not too near the player.
	Near.Sort([this, Sky](const int32 A, const int32 B)
	{
		return FVector::DistSquared2D(Sky->GetLampAt(A), PlayerAt) < FVector::DistSquared2D(Sky->GetLampAt(B), PlayerAt);
	});
	int32 First = INDEX_NONE;
	for (const int32 Lamp : Near)
	{
		FVector2D Stand;
		double Face = 0.0;
		StandFor(Lamp, Stand, Face);
		double Ground = 0.0;
		GroundAt(Stand, Ground);
		if (FVector2D::Distance(Stand, Player2D) >= RoundStartFrom && !IsSeen(FVector(Stand, Ground)))
		{
			First = Lamp;
			break;
		}
	}
	if (First == INDEX_NONE)
	{
		return false;
	}

	// Then on to the nearest lamp he can walk straight to, one after another.
	FCollisionQueryParams Query(SCENE_QUERY_STAT(DailyLifeRound), false, this);
	TArray<int32> Left = Near;
	Left.Remove(First);
	TArray<int32> Chain = { First };
	while (Chain.Num() < RoundMost)
	{
		FVector2D From;
		double FromFace = 0.0;
		StandFor(Chain.Last(), From, FromFace);
		int32 Best = INDEX_NONE;
		double BestDistance = RoundStep;
		for (const int32 Lamp : Left)
		{
			FVector2D To;
			double ToFace = 0.0;
			StandFor(Lamp, To, ToFace);
			const double Distance = FVector2D::Distance(From, To);
			if (Distance >= BestDistance)
			{
				continue;
			}
			FHitResult Hit;
			const double Z = Sky->GetLampAt(Lamp).Z - 450.0;
			if (GetWorld()->LineTraceSingleByObjectType(Hit, FVector(From, Z), FVector(To, Z), FCollisionObjectQueryParams(ECC_WorldStatic), Query))
			{
				continue;
			}
			Best = Lamp;
			BestDistance = Distance;
		}
		if (Best == INDEX_NONE)
		{
			break;
		}
		Chain.Add(Best);
		Left.Remove(Best);
	}
	if (Chain.Num() < 2)
	{
		return false;
	}

	// His lamps stay dark until he comes.
	RoundLamps.Reset();
	for (const int32 Lamp : Chain)
	{
		FRoundLamp& Entry = RoundLamps.AddDefaulted_GetRef();
		Entry.Lamp = Lamp;
		StandFor(Lamp, Entry.Stand, Entry.FaceYaw);
		Sky->SetLampUnlit(Lamp, true);
		if (CubeMesh)
		{
			UStaticMeshComponent* Cover = NewObject<UStaticMeshComponent>(this);
			Cover->SetStaticMesh(CubeMesh);
			Cover->SetupAttachment(RootComponent);
			Cover->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Cover->SetCanEverAffectNavigation(false);
			Cover->SetCastShadow(false);
			Cover->bVisibleInRayTracing = false;
			Cover->bAffectDistanceFieldLighting = false;
			if (BaseMaterial)
			{
				UMaterialInstanceDynamic* Paint = UMaterialInstanceDynamic::Create(BaseMaterial, this);
				Paint->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.035f, 0.035f, 0.03f));
				Cover->SetMaterial(0, Paint);
			}
			Cover->RegisterComponent();
			AddInstanceComponent(Cover);
			FTransform Place = Sky->GetLampGlass(Lamp);
			Place.SetScale3D(Place.GetScale3D() * CoverGrow);
			Cover->SetWorldTransform(Place);
			Cover->SetVisibility(false);
			Entry.Cover = Cover;
		}
	}

	// The lamplighter himself, with his pole.
	const FRoundLamp& Start = RoundLamps[0];
	LighterAt = Start.Stand;
	GroundAt(LighterAt, LighterGround);
	LighterYaw = Start.FaceYaw;
	WalkYaw = Start.FaceYaw;
	FRandomStream Look2(static_cast<int32>(DaySeed) + Days);
	const PersonKit::FOutfit Outfit = DailyLifeModel::LamplighterLook(Look2);
	APedestrian* Man = MakePerson(FVector(LighterAt, LighterGround), LighterYaw, Outfit);
	if (!Man)
	{
		EndRound(true);
		return false;
	}
	PersonKit::FPiece PoleShape;
	DailyLifeModel::Pole(PoleShape, DailyLifeModel::PoleUp, DailyLifeModel::PoleDown, true);
	LampPole = Man->AddProp(PoleShape, PersonKit::ForearmRight, DailyLifeModel::HandOnForearm(1.0, Outfit.Height),
		DailyLifeModel::HoldUpright(DailyLifeModel::PoleCarried));
	Lamplighter = Man;
	RoundNext = 0;
	AtLampTime = -1.f;
	LighterGroundClock = 0.f;
	RoundStarted = Now;
	bGreeted = false;
	return true;
}

void ADailyLife::EndRound(bool bAtOnce)
{
	if (ADayNight* Sky = ADayNight::Get(GetWorld()))
	{
		for (FRoundLamp& Lamp : RoundLamps)
		{
			if (!Lamp.bLit)
			{
				Sky->SetLampUnlit(Lamp.Lamp, false);
			}
		}
	}
	for (FRoundLamp& Lamp : RoundLamps)
	{
		if (UStaticMeshComponent* Cover = Lamp.Cover.Get())
		{
			Cover->DestroyComponent();
		}
	}
	RoundLamps.Reset();
	if (Round == ERound::Working)
	{
		Round = ERound::Done;
	}
	LeaveClock = 0.f;
	if (bAtOnce)
	{
		RemovePerson(Lamplighter.Get());
		Lamplighter.Reset();
	}
}

// ---- Courtyard life ----------------------------------------------------------

void ADailyLife::TickCourtyards(float DeltaSeconds)
{
	using namespace DailyLifeRun;
	const float Hour = HourNow();
	const bool bTime = Hour >= YardFrom && Hour < YardTo && RainNow() < 0.1f;
	const FVector2D Player2D = Flat(PlayerAt);

	for (int32 s = YardScenes.Num() - 1; s >= 0; --s)
	{
		FYardScene& Scene = YardScenes[s];
		const double Far = FVector2D::Distance(Yards[Scene.Yard].Middle, Player2D);
		bool bAnySeen = false;
		for (int32 k = 0; k < Scene.Folk.Num(); ++k)
		{
			if (Scene.Folk[k].IsValid() && Scene.FolkAt.IsValidIndex(k) && IsSeen(FVector(Scene.FolkAt[k], Scene.Ground)))
			{
				bAnySeen = true;
				break;
			}
		}
		if ((Far > YardFar || !bTime) && !bAnySeen)
		{
			EndYardScene(Scene);
			YardScenes.RemoveAt(s);
			continue;
		}
		Scene.Clock += DeltaSeconds;
		switch (Scene.Play)
		{
		case EYardPlay::Carpet:
			TickCarpet(Scene, DeltaSeconds);
			break;
		case EYardPlay::Washing:
			TickWashing(Scene, DeltaSeconds);
			break;
		case EYardPlay::Football:
			TickFootball(Scene, DeltaSeconds);
			break;
		default:
			break;
		}
	}

	if (!bTime || Now < NextYardLook || YardScenes.Num() >= YardsAtOnce)
	{
		return;
	}
	NextYardLook = Now + 2.0;
	for (int32 y = 0; y < Yards.Num(); ++y)
	{
		if (FVector2D::Distance(Yards[y].Middle, Player2D) > YardNear || DayChance(1, y) >= YardShare)
		{
			continue;
		}
		bool bBusy = false;
		for (const FYardScene& Scene : YardScenes)
		{
			bBusy |= Scene.Yard == y;
		}
		if (!bBusy && StartYardScene(y))
		{
			break;
		}
	}
}

bool ADailyLife::StartYardScene(int32 YardIndex)
{
	using namespace DailyLifeRun;
	const FCourtyard& Yard = Yards[YardIndex];
	FYardScene Scene;
	Scene.Yard = YardIndex;
	GroundAt(Yard.Middle, Scene.Ground);
	const FYardThing* Rack = nullptr;
	const FYardThing* Line = nullptr;
	for (const FYardThing& Thing : Yard.Things)
	{
		if (Thing.Kind == TEXT("rack"))
		{
			Rack = &Thing;
		}
		else if (Thing.Kind == TEXT("line") && Thing.Length > 300.0)
		{
			Line = &Thing;
		}
	}
	const float Pick = DayChance(2, YardIndex);
	Scene.Play = Pick < 0.35f && Rack ? EYardPlay::Carpet : Pick < 0.7f && Line ? EYardPlay::Washing : EYardPlay::Football;
	FRandomStream Looks(static_cast<int32>(DaySeed) + YardIndex * 31 + Days);

	if (Scene.Play == EYardPlay::Carpet)
	{
		// A woman at the rack, beating the rug hanging on it.
		const FVector2D Spot = InFrame(Rack->At, Rack->Yaw, -95.0, 30.0);
		if (IsSeen(FVector(Spot, Scene.Ground)))
		{
			return false;
		}
		const PersonKit::FOutfit Outfit = DailyLifeModel::HousewifeLook(Looks);
		APedestrian* Woman = MakePerson(FVector(Spot, Scene.Ground), Rack->Yaw, Outfit);
		if (!Woman)
		{
			return false;
		}
		PersonKit::FPiece BeaterShape;
		DailyLifeModel::CarpetBeater(BeaterShape);
		Scene.Beater = Woman->AddProp(BeaterShape, PersonKit::ForearmRight, DailyLifeModel::HandOnForearm(1.0, Outfit.Height), FRotator(180.f, 0.f, 0.f));
		PersonKit::FPiece DustShape;
		DailyLifeModel::DustPuff(DustShape);
		const FVector2D DustAt = InFrame(Rack->At, Rack->Yaw, -15.0, 30.0);
		UProceduralMeshComponent* Dust = MakeMesh(DustShape, FTransform(FRotator(0.0, Rack->Yaw, 0.0), FVector(DustAt, Scene.Ground + 140.0)));
		Dust->SetVisibility(false);
		Dust->SetCastShadow(false);
		Scene.Dust = Dust;
		Scene.Folk.Add(Woman);
		Scene.Spots.Add(Spot);
		Scene.Facings.Add(Rack->Yaw);
		Scene.FolkAt.Add(Spot);
		Scene.FolkYaw.Add(Rack->Yaw);
		Scene.Looks.Add(Outfit);
		// Each starts at her own point of the beat.
		Scene.Clock = Looks.FRand();
	}
	else if (Scene.Play == EYardPlay::Washing)
	{
		// A woman pegging out sheets from her basket along the line, then going back in.
		Scene.LineYaw = Line->Yaw;
		const double Half = Line->Length * 0.5;
		Scene.LineFrom = InFrame(Line->At, Line->Yaw, -Half, 0.0);
		Scene.LineTo = InFrame(Line->At, Line->Yaw, Half, 0.0);
		const int32 Pegs = FMath::Min(SheetsToPeg, FMath::FloorToInt32((Line->Length - 160.0) / PegGap));
		if (Pegs < 2)
		{
			return false;
		}
		for (int32 p = 0; p < Pegs; ++p)
		{
			Scene.Spots.Add(InFrame(Line->At, Line->Yaw, -Half + 90.0 + p * PegGap, -60.0));
			Scene.Facings.Add(Line->Yaw + 90.0);
		}
		// Last, the way out: back through the gate.
		Scene.Spots.Add(Yards[YardIndex].Gate - Heading(Yards[YardIndex].GateYaw) * 150.0);
		Scene.Facings.Add(Yards[YardIndex].GateYaw);
		if (IsSeen(FVector(Scene.Spots[0], Scene.Ground)))
		{
			return false;
		}
		const PersonKit::FOutfit Outfit = DailyLifeModel::HousewifeLook(Looks);
		APedestrian* Woman = MakePerson(FVector(Scene.Spots[0], Scene.Ground), Scene.Facings[0], Outfit);
		if (!Woman)
		{
			return false;
		}
		PersonKit::FPiece BasketShape;
		DailyLifeModel::WashBasket(BasketShape);
		const FVector2D BasketAt = InFrame(Line->At, Line->Yaw, -Half + 90.0 - 70.0, -110.0);
		Scene.Things.Add(MakeMesh(BasketShape, FTransform(FRotator(0.0, Line->Yaw + 20.0, 0.0), FVector(BasketAt, Scene.Ground))));
		Scene.Folk.Add(Woman);
		Scene.FolkAt.Add(Scene.Spots[0]);
		Scene.FolkYaw.Add(Scene.Facings[0]);
		Scene.Looks.Add(Outfit);
		Scene.Pegged = 0;
		Scene.PegClock = 0.f;
	}
	else
	{
		// Two or three kids kicking a ball about.
		const int32 Kids = DayChance(3, YardIndex) < 0.5f ? 2 : 3;
		for (int32 Try = 0; Try < 40 && Scene.Spots.Num() < Kids; ++Try)
		{
			const FVector2D Spot = Yard.Middle + FVector2D(Looks.FRandRange(-600.0f, 600.0f), Looks.FRandRange(-600.0f, 600.0f));
			bool bRoom = IsInside(Yard.Outline, Spot);
			for (const FVector2D& Other : Scene.Spots)
			{
				bRoom &= FVector2D::Distance(Other, Spot) > 300.0;
			}
			// Not too near the walls either.
			for (int32 c = 0; c < 8 && bRoom; ++c)
			{
				bRoom &= IsInside(Yard.Outline, Spot + Heading(c * 45.0) * 120.0);
			}
			if (bRoom)
			{
				Scene.Spots.Add(Spot);
			}
		}
		if (Scene.Spots.Num() < 2)
		{
			return false;
		}
		for (const FVector2D& Spot : Scene.Spots)
		{
			if (IsSeen(FVector(Spot, Scene.Ground)))
			{
				return false;
			}
		}
		for (int32 k = 0; k < Scene.Spots.Num(); ++k)
		{
			const double Face = YawOf(Scene.Spots[(k + 1) % Scene.Spots.Num()] - Scene.Spots[k]);
			const PersonKit::FOutfit Outfit = DailyLifeModel::KidLook(Looks, k == 2);
			APedestrian* Kid = MakePerson(FVector(Scene.Spots[k], Scene.Ground), Face, Outfit);
			if (!Kid)
			{
				EndYardScene(Scene);
				return false;
			}
			Scene.Folk.Add(Kid);
			Scene.Facings.Add(Face);
			Scene.FolkAt.Add(Scene.Spots[k]);
			Scene.FolkYaw.Add(Face);
			Scene.Looks.Add(Outfit);
		}
		PersonKit::FPiece BallShape;
		DailyLifeModel::Football(BallShape);
		Scene.Ball = MakeMesh(BallShape, FTransform(FVector(Scene.Spots[0], Scene.Ground + DailyLifeModel::BallRadius)));
		Scene.Holder = 0;
		Scene.HoldClock = 0.f;
		Scene.KickClock = -1.f;
	}
	YardScenes.Add(Scene);
	return true;
}

void ADailyLife::EndYardScene(FYardScene& Scene)
{
	for (const TWeakObjectPtr<APedestrian>& Person : Scene.Folk)
	{
		RemovePerson(Person.Get());
	}
	Scene.Folk.Reset();
	for (const TWeakObjectPtr<UProceduralMeshComponent>& Thing : Scene.Things)
	{
		RemoveMesh(Thing.Get());
	}
	Scene.Things.Reset();
	RemoveMesh(Scene.Dust.Get());
	RemoveMesh(Scene.Ball.Get());
}

void ADailyLife::TickCarpet(FYardScene& Scene, float DeltaSeconds)
{
	using namespace DailyLifeRun;
	APedestrian* Woman = Scene.Folk.Num() > 0 ? Scene.Folk[0].Get() : nullptr;
	if (!Woman)
	{
		return;
	}
	// She beats for a while, then has a rest.
	const float Cycle = FMath::Fmod(Scene.Clock, 20.f);
	DailyLifeModel::FLimbs Limbs;
	if (Cycle < 14.f)
	{
		const float Phase = Scene.Clock * BeatsPerSecond;
		DailyLifeModel::Beating(Limbs, Phase);
		// The blow lands at the end of each beat: a puff of dust.
		const float Beat = FMath::Frac(Phase);
		if (Beat < Scene.LastBeat)
		{
			Scene.DustAge = 0.f;
		}
		Scene.LastBeat = Beat;
	}
	else
	{
		Limbs.Arms[1] = FRotator(20.f, 0.f, -6.f);
		Limbs.Elbows[1] = 20.f;
		Limbs.Arms[0] = FRotator(10.f, 0.f, 24.f);
		Limbs.Elbows[0] = 95.f;
	}
	Woman->Strike(FVector(Scene.FolkAt[0], Scene.Ground + MiddleUp), Scene.FolkYaw[0], Limbs);

	Scene.DustAge += DeltaSeconds;
	if (UProceduralMeshComponent* Dust = Scene.Dust.Get())
	{
		const bool bShow = Scene.DustAge < 0.8f;
		Dust->SetVisibility(bShow);
		if (bShow)
		{
			Dust->SetWorldScale3D(FVector(1.0 + Scene.DustAge * 1.6));
		}
	}
}

void ADailyLife::TickWashing(FYardScene& Scene, float DeltaSeconds)
{
	using namespace DailyLifeRun;
	APedestrian* Woman = Scene.Folk.Num() > 0 ? Scene.Folk[0].Get() : nullptr;
	if (!Woman)
	{
		return;
	}
	const int32 Pegs = Scene.Spots.Num() - 1;
	const int32 Target = FMath::Min(Scene.Pegged, Pegs);
	const FVector2D Way = Scene.Spots[Target] - Scene.FolkAt[0];
	const double Left = Way.Size();
	constexpr float Walk = 90.f;
	if (Left > 5.0)
	{
		// To the next spot along the line, or out.
		const double Step = FMath::Min(Left, static_cast<double>(Walk * DeltaSeconds));
		Scene.FolkAt[0] += Way / Left * Step;
		Scene.FolkYaw[0] = YawOf(Way);
		Woman->WalkTo(FVector(Scene.FolkAt[0], Scene.Ground + MiddleUp), Scene.FolkYaw[0], Walk, DeltaSeconds);
		return;
	}
	if (Scene.Pegged >= Pegs)
	{
		// Back indoors through the gate.
		RemovePerson(Woman);
		Scene.Folk[0].Reset();
		return;
	}
	if (Scene.PegClock <= 0.f)
	{
		// A wet sheet up on the line.
		const FLinearColor Colours[] = { DailyLifeModel::Linen(), FLinearColor(0.62f, 0.70f, 0.82f), FLinearColor(0.85f, 0.70f, 0.70f), DailyLifeModel::Linen() };
		PersonKit::FPiece SheetShape;
		DailyLifeModel::Sheet(SheetShape, 90.0, 95.0, Colours[Scene.Pegged % 4]);
		const FVector2D SheetAt = Scene.Spots[Target] + Heading(Scene.Facings[Target]) * 48.0;
		UProceduralMeshComponent* Hung = MakeMesh(SheetShape, FTransform(FRotator(0.0, Scene.Facings[Target], 0.0), FVector(SheetAt, Scene.Ground + 212.0)));
		Hung->SetCastShadow(true);
		Scene.Things.Add(Hung);
	}
	Scene.PegClock += DeltaSeconds;
	Scene.FolkYaw[0] = FMath::FixedTurn(static_cast<float>(Scene.FolkYaw[0]), static_cast<float>(Scene.Facings[Target]), 360.f * DeltaSeconds);
	DailyLifeModel::FLimbs Limbs;
	DailyLifeModel::Pegging(Limbs, Scene.PegClock * 0.5f);
	Woman->Strike(FVector(Scene.FolkAt[0], Scene.Ground + MiddleUp), Scene.FolkYaw[0], Limbs);
	if (Scene.PegClock >= PegSeconds)
	{
		++Scene.Pegged;
		Scene.PegClock = 0.f;
	}
}

void ADailyLife::TickFootball(FYardScene& Scene, float DeltaSeconds)
{
	using namespace DailyLifeRun;
	const FCourtyard& Yard = Yards[Scene.Yard];
	const int32 Kids = Scene.Folk.Num();
	if (Kids < 2)
	{
		return;
	}

	// The player on foot can kick it too.
	if (const APestiPerson* Walker = Cast<APestiPerson>(UGameplayStatics::GetPlayerPawn(this, 0)))
	{
		const FVector2D Foot = Flat(Walker->GetActorLocation());
		const double Gap = FVector2D::Distance(Foot, Scene.BallAt);
		if (Gap < PlayerKick && Scene.BallVelocity.Size() < 500.0)
		{
			FVector2D Away = Scene.BallAt - Foot;
			if (!Away.Normalize())
			{
				Away = Heading(Walker->GetActorRotation().Yaw);
			}
			const double Run = Flat(Walker->GetVelocity()).Size();
			Scene.BallVelocity = Away * (500.0 + Run * 0.8);
			Scene.BallRise = 220.f;
			Scene.Holder = INDEX_NONE;
			Scene.KickClock = -1.f;
			Scene.bKicked = true;
		}
	}

	// The ball: at the holder's feet, or rolling and bouncing, off the yard's walls.
	if (Scene.Holder != INDEX_NONE)
	{
		Scene.BallAt = Scene.FolkAt[Scene.Holder] + Heading(Scene.FolkYaw[Scene.Holder]) * 35.0;
		Scene.BallVelocity = FVector2D::ZeroVector;
	}
	else
	{
		const FVector2D Next = Scene.BallAt + Scene.BallVelocity * DeltaSeconds;
		if (IsInside(Yard.Outline, Next))
		{
			Scene.BallAt = Next;
		}
		else
		{
			Scene.BallVelocity *= -0.5;
		}
		Scene.BallVelocity *= FMath::Exp(-BallFriction * DeltaSeconds);
		Scene.BallRise -= 980.f * DeltaSeconds;
		Scene.BallHop += Scene.BallRise * DeltaSeconds;
		if (Scene.BallHop < 0.f)
		{
			Scene.BallHop = 0.f;
			Scene.BallRise = Scene.BallRise < -60.f ? -Scene.BallRise * 0.45f : 0.f;
		}
	}
	const double Rolling = Scene.BallVelocity.Size();
	Scene.BallSpin = FMath::Fmod(Scene.BallSpin + static_cast<float>(FMath::RadiansToDegrees(Rolling * DeltaSeconds / DailyLifeModel::BallRadius)), 360.f);
	if (UProceduralMeshComponent* Ball = Scene.Ball.Get())
	{
		const double Toward = Rolling > 1.0 ? YawOf(Scene.BallVelocity) : 0.0;
		Ball->SetWorldLocationAndRotation(FVector(Scene.BallAt, Scene.Ground + DailyLifeModel::BallRadius + Scene.BallHop),
			FRotator(-Scene.BallSpin, Toward, 0.0));
	}

	// The kid nearest a loose ball goes for it.
	int32 Chaser = INDEX_NONE;
	if (Scene.Holder == INDEX_NONE)
	{
		double Nearest = TNumericLimits<double>::Max();
		for (int32 k = 0; k < Kids; ++k)
		{
			const double Gap = FVector2D::Distance(Scene.FolkAt[k], Scene.BallAt);
			if (Scene.Folk[k].IsValid() && Gap < Nearest)
			{
				Nearest = Gap;
				Chaser = k;
			}
		}
		if (Chaser != INDEX_NONE && Rolling > 300.0 && Nearest > 250.0)
		{
			// Still flying: they wait for it.
			Chaser = INDEX_NONE;
		}
	}

	for (int32 k = 0; k < Kids; ++k)
	{
		APedestrian* Kid = Scene.Folk[k].Get();
		if (!Kid)
		{
			continue;
		}
		const FVector Middle(Scene.FolkAt[k], Scene.Ground + MiddleUp);
		DailyLifeModel::FLimbs Limbs;
		if (k == Scene.Holder)
		{
			Scene.HoldClock += DeltaSeconds;
			if (Scene.Receiver == INDEX_NONE || Scene.Receiver == k)
			{
				Scene.Receiver = (k + 1 + Dice.RandRange(0, Kids - 2)) % Kids;
			}
			Scene.FolkYaw[k] = FMath::FixedTurn(static_cast<float>(Scene.FolkYaw[k]),
				static_cast<float>(YawOf(Scene.FolkAt[Scene.Receiver] - Scene.FolkAt[k])), 400.f * DeltaSeconds);
			if (Scene.KickClock < 0.f && Scene.HoldClock > 0.9f)
			{
				Scene.KickClock = 0.f;
			}
			if (Scene.KickClock >= 0.f)
			{
				Scene.KickClock += DeltaSeconds;
				const float Swing = Scene.KickClock / KickSeconds;
				DailyLifeModel::Kicking(Limbs, Swing);
				if (Swing >= 0.55f)
				{
					// Off it goes to the next kid.
					FVector2D Pass = Scene.FolkAt[Scene.Receiver] - Scene.BallAt;
					Pass.Normalize();
					Scene.BallVelocity = Pass * Dice.FRandRange(450.0f, 650.0f);
					Scene.BallRise = Dice.FRandRange(0.f, 200.f);
					Scene.Holder = INDEX_NONE;
					Scene.KickClock = -1.f;
				}
			}
			else
			{
				DailyLifeModel::Ready(Limbs, 0.3f);
			}
			Kid->Strike(Middle, Scene.FolkYaw[k], Limbs);
			continue;
		}
		if (k == Chaser)
		{
			// Running to the ball.
			const FVector2D Way = Scene.BallAt - Scene.FolkAt[k];
			const double Gap = Way.Size();
			if (Gap < KidReach)
			{
				Scene.Holder = k;
				Scene.HoldClock = 0.f;
				Scene.Receiver = INDEX_NONE;
				Scene.bKicked = false;
			}
			else
			{
				Scene.FolkAt[k] += Way / Gap * FMath::Min(Gap, static_cast<double>(KidRun * DeltaSeconds));
				Scene.FolkYaw[k] = YawOf(Way);
				Kid->WalkTo(FVector(Scene.FolkAt[k], Scene.Ground + MiddleUp), Scene.FolkYaw[k], KidRun, DeltaSeconds);
				continue;
			}
		}
		// Back to his place, then waiting for the ball.
		const FVector2D Home = Scene.Spots[k] - Scene.FolkAt[k];
		const double HomeGap = Home.Size();
		if (HomeGap > 60.0)
		{
			constexpr float Trot = 150.f;
			Scene.FolkAt[k] += Home / HomeGap * FMath::Min(HomeGap, static_cast<double>(Trot * DeltaSeconds));
			Scene.FolkYaw[k] = YawOf(Home);
			Kid->WalkTo(FVector(Scene.FolkAt[k], Scene.Ground + MiddleUp), Scene.FolkYaw[k], Trot, DeltaSeconds);
			continue;
		}
		Scene.FolkYaw[k] = FMath::FixedTurn(static_cast<float>(Scene.FolkYaw[k]), static_cast<float>(YawOf(Scene.BallAt - Scene.FolkAt[k])), 300.f * DeltaSeconds);
		DailyLifeModel::Ready(Limbs, 0.5f + 0.3f * FMath::Sin(Scene.Clock * 1.3f + k));
		Kid->Strike(Middle, Scene.FolkYaw[k], Limbs);
	}

	// Kicked away by the player: one of them calls out to him.
	if (Scene.bKicked && Now >= Scene.NextShout && FVector2D::Distance(Flat(PlayerAt), Scene.BallAt) < HearSay)
	{
		Scene.NextShout = Now + 20.0;
		Scene.bKicked = false;
		Say(TEXT("Gyerek"), TEXT("Ide, b\u00e1csi, ide passzolja!"), TEXT("Here, mister, pass it here!"));
	}
}

// ---- Street people -----------------------------------------------------------

bool ADailyLife::SellerHours(ESeller Kind, float Hour) const
{
	switch (Kind)
	{
	case ESeller::Organ:
		return Hour >= 9.f && Hour < 18.f;
	case ESeller::Grinder:
		return Hour >= 9.f && Hour < 16.f;
	case ESeller::Chestnut:
		return Hour >= 15.f && Hour < 22.f;
	default:
		return false;
	}
}

void ADailyLife::TickSellers(float DeltaSeconds)
{
	using namespace DailyLifeRun;
	const float Hour = HourNow();
	const float Wet = RainNow();
	for (FSeller& Seller : Sellers)
	{
		if (Seller.bActive)
		{
			const bool bOff = !SellerHours(Seller.Kind, Hour) || Wet > 0.3f;
			const double Far = FVector::Dist2D(Seller.At, PlayerAt);
			if (!Seller.Person.IsValid() || ((Far > SellerFar || bOff) && !IsSeen(Seller.At)))
			{
				EndSeller(Seller);
				Seller.NextTry = Now + 90.0;
				continue;
			}
			PoseSeller(Seller, DeltaSeconds);
			continue;
		}
		if (Now < Seller.NextTry || !SellerHours(Seller.Kind, Hour) || Wet > 0.2f)
		{
			continue;
		}
		Seller.NextTry = Now + 6.0;
		StartSeller(Seller);
	}
}

bool ADailyLife::StartSeller(FSeller& Seller)
{
	using namespace DailyLifeRun;
	const FVector2D Player2D = Flat(PlayerAt);
	// The nearest of today's places for him, far enough to come unseen.
	int32 BestPlace = INDEX_NONE;
	FVector2D BestSpot = FVector2D::ZeroVector;
	double BestYaw = 0.0;
	double BestDistance = SellerTo;
	auto Consider = [&](int32 Place, const FVector2D& Spot, double Turn)
	{
		const double Distance = FVector2D::Distance(Spot, Player2D);
		if (Distance < SellerFrom || Distance >= BestDistance)
		{
			return;
		}
		BestPlace = Place;
		BestSpot = Spot;
		BestYaw = Turn;
		BestDistance = Distance;
	};
	switch (Seller.Kind)
	{
	case ESeller::Organ:
		// Out on the pavement beside a courtyard's gate, playing to the street.
		for (int32 y = 0; y < Yards.Num(); ++y)
		{
			if (DayChance(10, y) < OrganShare)
			{
				Consider(y, InFrame(Yards[y].Gate, Yards[y].GateYaw, 130.0, 230.0), Yards[y].GateYaw);
			}
		}
		break;
	case ESeller::Grinder:
		// In front of a shop, his barrow along the pavement.
		for (int32 s = 0; s < Shops.Num(); ++s)
		{
			if (!Shops[s].bClosed && DayChance(11, s) < GrinderShare)
			{
				Consider(s, InFrame(Shops[s].Foot, Shops[s].Yaw, ShopModel::FrontX + 150.0, Shops[s].Width * 0.25), Shops[s].Yaw + 90.0);
			}
		}
		break;
	case ESeller::Chestnut:
		// Beside a kiosk.
		for (int32 k = 0; k < Kiosks.Num(); ++k)
		{
			if (DayChance(12, k) < ChestnutShare)
			{
				Consider(k, InFrame(Flat(Kiosks[k]), Kiosks[k].Z, 40.0, -330.0), Kiosks[k].Z);
			}
		}
		break;
	default:
		break;
	}
	if (BestPlace == INDEX_NONE)
	{
		return false;
	}
	double Ground = 0.0;
	GroundAt(BestSpot, Ground);
	const FVector Foot(BestSpot, Ground);
	if (IsSeen(Foot))
	{
		return false;
	}

	Seller.Place = BestPlace;
	Seller.At = Foot;
	Seller.Yaw = BestYaw;
	Seller.Clock = 0.f;
	Seller.NextShout = Now + 10.0;
	FRandomStream Looks(static_cast<int32>(DaySeed) + BestPlace * 7 + static_cast<int32>(Seller.Kind) * 1013 + Days);
	const FQuat Turn(FRotator(0.0, BestYaw, 0.0));
	auto Place = [&](const FVector& Local)
	{
		return Foot + Turn.RotateVector(Local);
	};
	PersonKit::FOutfit Outfit;
	FVector ManAt = FVector::ZeroVector;
	PersonKit::FPiece StandShape;
	PersonKit::FPiece TurningShape;
	FVector TurningAt = FVector::ZeroVector;
	switch (Seller.Kind)
	{
	case ESeller::Organ:
		DailyLifeModel::Organ(StandShape);
		DailyLifeModel::OrganCrank(TurningShape);
		TurningAt = DailyLifeModel::OrganCrankAt();
		Outfit = DailyLifeModel::OrganGrinderLook(Looks);
		ManAt = FVector(-48.0, 26.0, 0.0);
		break;
	case ESeller::Grinder:
		DailyLifeModel::GrinderBarrow(StandShape);
		DailyLifeModel::GrinderStone(TurningShape);
		TurningAt = DailyLifeModel::StoneAt();
		Outfit = DailyLifeModel::KnifeGrinderLook(Looks);
		ManAt = FVector(-46.0, 0.0, 0.0);
		break;
	default:
		DailyLifeModel::ChestnutStove(StandShape);
		Outfit = DailyLifeModel::ChestnutLook(Looks);
		ManAt = FVector(-50.0, 16.0, 0.0);
		break;
	}
	Seller.Stand = MakeMesh(StandShape, FTransform(Turn, Foot));
	if (TurningShape.Sections.Num() > 0)
	{
		Seller.Turning = MakeMesh(TurningShape, FTransform(Turn, Place(TurningAt)));
	}
	APedestrian* Man = MakePerson(Place(ManAt), BestYaw, Outfit);
	if (!Man)
	{
		EndSeller(Seller);
		return false;
	}
	Seller.Person = Man;
	const FVector Hand = DailyLifeModel::HandOnForearm(1.0, Outfit.Height);
	if (Seller.Kind == ESeller::Grinder)
	{
		PersonKit::FPiece KnifeShape;
		DailyLifeModel::Knife(KnifeShape);
		Seller.Held = Man->AddProp(KnifeShape, PersonKit::ForearmRight, Hand, FRotator(-90.f, 0.f, 0.f));
		PersonKit::FPiece SparkShape;
		DailyLifeModel::Sparks(SparkShape, Looks.RandRange(0, 999));
		UProceduralMeshComponent* Spray = MakeMesh(SparkShape, FTransform(Turn, Place(DailyLifeModel::StoneAt() + FVector(-12.0, 0.0, 18.0))));
		Spray->SetCastShadow(false);
		Seller.Sparks = Spray;
		Seller.Sound = MakeSound(CitySynth::ESpot::Grinder, Place(DailyLifeModel::StoneAt()));
	}
	else if (Seller.Kind == ESeller::Chestnut)
	{
		PersonKit::FPiece ScoopShape;
		DailyLifeModel::Scoop(ScoopShape);
		Seller.Held = Man->AddProp(ScoopShape, PersonKit::ForearmRight, Hand, DailyLifeModel::HoldUpright(90.f, 95.f));
		// The glow of the grate at dusk.
		UPointLightComponent* Light = NewObject<UPointLightComponent>(this);
		Light->SetupAttachment(RootComponent);
		Light->SetIntensityUnits(ELightUnits::Candelas);
		Light->SetIntensity(0.f);
		Light->SetLightColor(FLinearColor(1.f, 0.45f, 0.12f));
		Light->SetAttenuationRadius(450.f);
		Light->SetCastShadows(false);
		Light->RegisterComponent();
		AddInstanceComponent(Light);
		Light->SetWorldLocation(Place(FVector(0.0, 0.0, 70.0)));
		Seller.Glow = Light;
	}
	else
	{
		Seller.Sound = MakeSound(CitySynth::ESpot::Organ, Place(FVector(0.0, 0.0, 100.0)));
	}
	Seller.bActive = true;
	PoseSeller(Seller, 0.f);
	return true;
}

void ADailyLife::EndSeller(FSeller& Seller)
{
	RemovePerson(Seller.Person.Get());
	Seller.Person.Reset();
	RemoveMesh(Seller.Stand.Get());
	RemoveMesh(Seller.Turning.Get());
	RemoveMesh(Seller.Sparks.Get());
	if (USpotSound* Sound = Seller.Sound.Get())
	{
		Sound->Stop();
		Sound->DestroyComponent();
	}
	if (UPointLightComponent* Light = Seller.Glow.Get())
	{
		Light->DestroyComponent();
	}
	Seller.Sound.Reset();
	Seller.Glow.Reset();
	Seller.Held.Reset();
	Seller.bActive = false;
}

void ADailyLife::PoseSeller(FSeller& Seller, float DeltaSeconds)
{
	using namespace DailyLifeRun;
	APedestrian* Man = Seller.Person.Get();
	if (!Man)
	{
		return;
	}
	Seller.Clock += DeltaSeconds;
	const FQuat Turn(FRotator(0.0, Seller.Yaw, 0.0));
	DailyLifeModel::FLimbs Limbs;
	FVector ManAt = FVector::ZeroVector;
	const bool bNear = FVector::Dist2D(Seller.At, PlayerAt) < HearSay;
	switch (Seller.Kind)
	{
	case ESeller::Organ:
	{
		const float Turns = Seller.Clock * 0.8f;
		DailyLifeModel::Cranking(Limbs, Turns);
		ManAt = FVector(-48.0, 26.0, 0.0);
		if (UProceduralMeshComponent* Crank = Seller.Turning.Get())
		{
			Crank->SetWorldRotation(DailyLifeRun::Turned(Seller.Yaw, FRotator(-FMath::Fmod(Turns, 1.f) * 360.f, 0.f, 0.f)));
		}
		Hear(Seller.Sound.Get(), OrganReach);
		if (bNear && Now >= Seller.NextShout)
		{
			Seller.NextShout = Now + 25.0;
			Say(TEXT("Verklis"), TEXT("Tess\u00e9k, tess\u00e9k! A papag\u00e1j megmondja a j\u00f6vend\u0151t!"), TEXT("Step up, step up! The parrot tells your fortune!"));
		}
		break;
	}
	case ESeller::Grinder:
	{
		const float Strokes = Seller.Clock * 1.2f;
		DailyLifeModel::Grinding(Limbs, Strokes);
		ManAt = FVector(-46.0, 0.0, 0.0);
		if (UProceduralMeshComponent* Stone = Seller.Turning.Get())
		{
			Stone->SetWorldRotation(DailyLifeRun::Turned(Seller.Yaw, FRotator(-FMath::Fmod(Seller.Clock * 540.f, 360.f), 0.f, 0.f)));
		}
		if (UProceduralMeshComponent* Spray = Seller.Sparks.Get())
		{
			// The sparks flicker as the knife is pressed on.
			const bool bShow = FMath::Frac(Seller.Clock * 6.f) < 0.55f;
			Spray->SetVisibility(bShow);
			if (bShow)
			{
				Spray->SetWorldRotation(DailyLifeRun::Turned(Seller.Yaw, FRotator(0.f, 0.f, FMath::Fmod(Seller.Clock * 977.f, 40.f) - 20.f)));
			}
		}
		Hear(Seller.Sound.Get(), GrinderReach);
		if (bNear && Now >= Seller.NextShout)
		{
			Seller.NextShout = Now + 25.0;
			Say(TEXT("K\u00f6sz\u00f6r\u0171s"), TEXT("K\u00e9st, oll\u00f3t k\u00f6sz\u00f6r\u00fcl\u00f6k!"), TEXT("Knives and scissors sharpened!"));
		}
		break;
	}
	default:
	{
		const float Raise = DailyLifeModel::Stirring(Limbs, Seller.Clock * 0.4f);
		ManAt = FVector(-50.0, 16.0, 0.0);
		if (UProceduralMeshComponent* Held = Seller.Held.Get())
		{
			Held->SetRelativeRotation(DailyLifeModel::HoldUpright(Raise, 95.f));
		}
		if (UPointLightComponent* Light = Seller.Glow.Get())
		{
			const float Flicker = 0.85f + 0.15f * FMath::Sin(Seller.Clock * 7.f) * FMath::Sin(Seller.Clock * 3.1f);
			Light->SetIntensity(40.f * FMath::Clamp(DarkNow() * 1.5f, 0.f, 1.f) * Flicker);
		}
		if (bNear && Now >= Seller.NextShout)
		{
			Seller.NextShout = Now + 25.0;
			Say(TEXT("Gesztenyes\u00fct\u0151"), TEXT("Forr\u00f3 gesztenye! Itt a forr\u00f3 gesztenye!"), TEXT("Hot chestnuts! Get your hot chestnuts here!"));
		}
		break;
	}
	}
	Man->Strike(Seller.At + Turn.RotateVector(ManAt) + FVector(0.0, 0.0, MiddleUp), Seller.Yaw, Limbs);
}

// ---- Rare events -------------------------------------------------------------

void ADailyLife::TickEvents(float DeltaSeconds)
{
	using namespace DailyLifeRun;
	if (Event == EEvent::None)
	{
		if (Now < NextEvent)
		{
			return;
		}
		NextEvent = Now + Dice.FRandRange(EventEveryFrom, EventEveryTo);
		if (Dice.FRand() > EventShare)
		{
			return;
		}
		// Which can happen now: weddings and funerals by day and dry, the fire brigade any time.
		const float Hour = HourNow();
		const bool bDry = RainNow() < 0.2f;
		TArray<EEvent, TInlineAllocator<3>> Kinds;
		TArray<float, TInlineAllocator<3>> Weights;
		if (bDry && Hour >= 10.f && Hour < 16.f && ChurchDoors.Num() > 0)
		{
			Kinds.Add(EEvent::Wedding);
			Weights.Add(0.4f);
		}
		if (bDry && Hour >= 9.f && Hour < 15.f && bHasCemetery)
		{
			Kinds.Add(EEvent::Funeral);
			Weights.Add(0.35f);
		}
		Kinds.Add(EEvent::Fire);
		Weights.Add(0.25f);
		float Sum = 0.f;
		for (const float Weight : Weights)
		{
			Sum += Weight;
		}
		float Roll = Dice.FRand() * Sum;
		EEvent Kind = Kinds.Last();
		for (int32 i = 0; i < Kinds.Num(); ++i)
		{
			Roll -= Weights[i];
			if (Roll <= 0.f)
			{
				Kind = Kinds[i];
				break;
			}
		}
		if (!StartEvent(Kind))
		{
			// No good place for it now: try again soon.
			NextEvent = Now + 30.0;
		}
		return;
	}

	const double Age = Now - EventStarted;
	if (Event == EEvent::Fire)
	{
		AFireEngine* Fire = Brigade;
		if (!IsValid(Fire))
		{
			EndEvent();
			return;
		}
		if (!bEventEnding && (Fire->HasArrived() || Age > FireLongest || FVector::Dist2D(Fire->GetActorLocation(), PlayerAt) > EventFar + 2000.0))
		{
			BeginEnding();
		}
	}
	else
	{
		TickMarchers(DeltaSeconds);
		if (!bEventEnding && Age > (Event == EEvent::Wedding ? WeddingLongest : FuneralLongest))
		{
			BeginEnding();
		}
	}
	if (bEventEnding && (!IsEventSeen() || Now - EndingSince > EndingLongest))
	{
		EndEvent();
	}
}

void ADailyLife::BeginEnding()
{
	if (!bEventEnding)
	{
		bEventEnding = true;
		EndingSince = Now;
	}
}

bool ADailyLife::IsEventSeen() const
{
	for (const FMarcher& Marcher : Marchers)
	{
		if (const APedestrian* Person = Marcher.Person.Get())
		{
			if (IsSeen(Person->GetActorLocation() - FVector(0.0, 0.0, DailyLifeRun::MiddleUp)))
			{
				return true;
			}
		}
	}
	if (const ATrafficCart* Cart = Hearse.Get())
	{
		if (Cart->IsInTraffic() && IsSeen(Cart->GetActorLocation()))
		{
			return true;
		}
	}
	if (IsValid(Brigade) && IsSeen(Brigade->GetActorLocation()))
	{
		return true;
	}
	return false;
}

bool ADailyLife::StartEvent(EEvent Kind)
{
	bool bStarted = false;
	switch (Kind)
	{
	case EEvent::Wedding:
		bStarted = StartWedding();
		break;
	case EEvent::Funeral:
		bStarted = StartFuneral();
		break;
	case EEvent::Fire:
		bStarted = StartFire();
		break;
	default:
		break;
	}
	if (bStarted)
	{
		Event = Kind;
		EventStarted = Now;
		bEventEnding = false;
		bMournersAngry = false;
		NextEventShout = Now + 8.0;
		ColumnSpeed = 0.f;
	}
	return bStarted;
}

void ADailyLife::EndEvent()
{
	for (FMarcher& Marcher : Marchers)
	{
		RemovePerson(Marcher.Person.Get());
	}
	Marchers.Reset();
	if (USpotSound* Sound = EventSound.Get())
	{
		Sound->Stop();
		Sound->DestroyComponent();
	}
	EventSound.Reset();
	if (ATrafficManager* Manager = Traffic())
	{
		Manager->SetRoadBlock(DailyLifeRun::WeddingSlot, TArray<FVector>());
		Manager->SetRoadBlock(DailyLifeRun::FuneralSlot, TArray<FVector>());
		// The hearse goes too, unless the player has taken it.
		ATrafficCart* Cart = Hearse.Get();
		if (Cart && Cart->IsInTraffic())
		{
			Manager->RemoveCart(Cart);
		}
	}
	Hearse.Reset();
	if (IsValid(Brigade))
	{
		Brigade->Destroy();
	}
	Brigade = nullptr;
	EventWay.Reset();
	EventAlong.Reset();
	Trail.Reset();
	EventS = 0.0;
	Event = EEvent::None;
	bEventEnding = false;
	bMournersAngry = false;
}

bool ADailyLife::PickRoadStart(double RangeFrom, double RangeTo, const FVector2D* Toward, int32& OutLane, int32& OutPoint, FVector& OutAt)
{
	using namespace DailyLifeRun;
	const ATrafficManager* Manager = Traffic();
	if (!Manager)
	{
		return false;
	}
	const FVector2D Player2D = Flat(PlayerAt);
	TArray<FIntPoint> Found;
	for (int32 l = 0; l < Manager->NumLanes(); ++l)
	{
		const TArray<FVector>& Points = Manager->GetLane(l).Points;
		for (int32 p = 0; p + 1 < Points.Num(); ++p)
		{
			const FVector2D Spot = Flat(Points[p]);
			const double Distance = FVector2D::Distance(Spot, Player2D);
			if (Distance < RangeFrom || Distance > RangeTo)
			{
				continue;
			}
			if (Toward)
			{
				// Further from the goal than the player, heading for it, and passing him on the way.
				const FVector2D Goal = *Toward;
				if (FVector2D::Distance(Spot, Goal) < FVector2D::Distance(Player2D, Goal) + 1000.0)
				{
					continue;
				}
				const FVector2D LaneWay = (Flat(Points[p + 1]) - Spot).GetSafeNormal();
				if (FVector2D::DotProduct(LaneWay, (Goal - Spot).GetSafeNormal()) < 0.3)
				{
					continue;
				}
				const FVector2D Closest = FMath::ClosestPointOnSegment2D(Player2D, Spot, Goal);
				if (FVector2D::Distance(Closest, Player2D) > PassNear)
				{
					continue;
				}
			}
			Found.Add(FIntPoint(l, p));
		}
	}
	for (int32 Try = 0; Try < 12 && Found.Num() > 0; ++Try)
	{
		const int32 Pick = Dice.RandRange(0, Found.Num() - 1);
		const FIntPoint Spot = Found[Pick];
		const FVector At = Manager->GetLane(Spot.X).Points[Spot.Y];
		if (!IsSeen(At))
		{
			OutLane = Spot.X;
			OutPoint = Spot.Y;
			OutAt = At;
			return true;
		}
		Found.RemoveAtSwap(Pick);
	}
	return false;
}

bool ADailyLife::StartWedding()
{
	using namespace DailyLifeRun;
	ATrafficManager* Manager = Traffic();
	if (!Manager || ChurchDoors.Num() == 0)
	{
		return false;
	}
	// To the church nearest the player.
	const FVector2D Player2D = Flat(PlayerAt);
	FVector2D Door = ChurchDoors[0];
	for (const FVector2D& Other : ChurchDoors)
	{
		if (FVector2D::Distance(Other, Player2D) < FVector2D::Distance(Door, Player2D))
		{
			Door = Other;
		}
	}
	if (FVector2D::Distance(Door, Player2D) > 30000.0)
	{
		return false;
	}
	int32 Lane = INDEX_NONE;
	int32 Point = INDEX_NONE;
	FVector Start = FVector::ZeroVector;
	if (!PickRoadStart(4000.0, 9000.0, &Door, Lane, Point, Start))
	{
		return false;
	}
	TArray<FVector> Path;
	if (!Manager->FindPath(Start, FVector(Door, Start.Z), Path) || Path.Num() < 2 || IsSeen(Path[0]))
	{
		return false;
	}
	double DoorGround = Path.Last().Z;
	GroundAt(Door, DoorGround);
	Path.Add(FVector(Door, DoorGround));
	EventWay = Path;
	Measure(EventWay, EventAlong);
	EventS = 0.0;

	// The fiddler in front, the bride and groom, then the guests two by two.
	FRandomStream Looks(Dice.RandRange(0, 1 << 30));
	double Ground = Path[0].Z;
	GroundAt(Flat(Path[0]), Ground);
	const FVector Foot(Flat(Path[0]), Ground);
	auto AddMarcher = [&](const PersonKit::FOutfit& Outfit, double Behind, double Side, int32 Doing) -> APedestrian*
	{
		APedestrian* Person = MakePerson(Foot, 0.0, Outfit);
		if (!Person)
		{
			return nullptr;
		}
		FMarcher& Marcher = Marchers.AddDefaulted_GetRef();
		Marcher.Person = Person;
		Marcher.Behind = Behind;
		Marcher.Side = Side;
		Marcher.Ground = Ground;
		Marcher.Doing = Doing;
		return Person;
	};
	const PersonKit::FOutfit FiddlerOutfit = DailyLifeModel::FiddlerLook();
	if (APedestrian* Fiddler = AddMarcher(FiddlerOutfit, 0.0, 0.0, 1))
	{
		PersonKit::FPiece FiddleShape;
		DailyLifeModel::Fiddle(FiddleShape);
		Fiddler->AddProp(FiddleShape, PersonKit::Body, DailyLifeModel::FiddleAt(FiddlerOutfit.Height), DailyLifeModel::FiddleTurn());
		PersonKit::FPiece BowShape;
		DailyLifeModel::FiddleBow(BowShape);
		Fiddler->AddProp(BowShape, PersonKit::ForearmRight, DailyLifeModel::HandOnForearm(1.0, FiddlerOutfit.Height), FRotator(0.f, 0.f, 90.f));
		EventSound = MakeSound(CitySynth::ESpot::Fiddle, Fiddler->GetActorLocation());
	}
	const PersonKit::FOutfit BrideOutfit = DailyLifeModel::BrideLook();
	if (APedestrian* Bride = AddMarcher(BrideOutfit, 170.0, -35.0, 2))
	{
		PersonKit::FPiece VeilShape;
		DailyLifeModel::Veil(VeilShape, BrideOutfit.Height);
		Bride->AddProp(VeilShape, PersonKit::Body, DailyLifeModel::HeadMiddle(BrideOutfit), FRotator::ZeroRotator);
		PersonKit::FPiece BouquetShape;
		DailyLifeModel::Bouquet(BouquetShape);
		DailyLifeModel::FLimbs Holding;
		const float Raise = DailyLifeModel::HoldingUp(Holding);
		Bride->AddProp(BouquetShape, PersonKit::ForearmRight, DailyLifeModel::HandOnForearm(1.0, BrideOutfit.Height), DailyLifeModel::HoldUpright(Raise));
	}
	AddMarcher(DailyLifeModel::GroomLook(), 170.0, 35.0, 0);
	const int32 Guests = Looks.RandRange(6, 8);
	for (int32 i = 0; i < Guests; ++i)
	{
		const PersonKit::FOutfit Outfit = i == Guests - 1 && Looks.FRand() < 0.6f ? DailyLifeModel::KidLook(Looks, true) : DailyLifeModel::GuestLook(Looks, i % 2 == 0);
		AddMarcher(Outfit, 300.0 + (i / 2) * 100.0, i % 2 ? 45.0 : -45.0, 0);
	}
	return Marchers.Num() > 0;
}

bool ADailyLife::StartFuneral()
{
	using namespace DailyLifeRun;
	ATrafficManager* Manager = Traffic();
	if (!Manager || !bHasCemetery)
	{
		return false;
	}
	int32 Lane = INDEX_NONE;
	int32 Point = INDEX_NONE;
	FVector Start = FVector::ZeroVector;
	if (!PickRoadStart(5000.0, 10000.0, &CemeteryGate, Lane, Point, Start))
	{
		return false;
	}
	ATrafficCart* Cart = Manager->SpawnHearse(Lane, Point);
	if (!Cart)
	{
		return false;
	}
	Cart->HeadFor(FVector(CemeteryGate, Start.Z));
	Hearse = Cart;

	// The mourners walk in its trail: the lane behind it, then where it goes.
	Trail.Reset();
	const TArray<FVector>& Points = Manager->GetLane(Lane).Points;
	for (int32 p = 0; p <= Point; ++p)
	{
		Trail.Add(Points[p]);
	}
	Measure(Trail, EventAlong);
	EventS = EventAlong.Num() > 0 ? EventAlong.Last() : 0.0;

	FRandomStream Looks(Dice.RandRange(0, 1 << 30));
	double Ground = Start.Z;
	GroundAt(Flat(Start), Ground);
	const int32 Mourners = Looks.RandRange(6, 10);
	for (int32 i = 0; i < Mourners; ++i)
	{
		const PersonKit::FOutfit Outfit = DailyLifeModel::MournerLook(Looks, i % 3 == 1);
		APedestrian* Person = MakePerson(FVector(Flat(Start), Ground), 0.0, Outfit);
		if (!Person)
		{
			continue;
		}
		FMarcher& Marcher = Marchers.AddDefaulted_GetRef();
		Marcher.Person = Person;
		Marcher.Behind = (i / 2) * 110.0;
		Marcher.Side = i % 2 ? 45.0 : -45.0;
		Marcher.Ground = Ground;
		// The first two carry wreaths.
		if (i < 2)
		{
			Marcher.Doing = 2;
			PersonKit::FPiece WreathShape;
			DailyLifeModel::Wreath(WreathShape);
			DailyLifeModel::FLimbs Holding;
			const float Raise = DailyLifeModel::HoldingUp(Holding);
			Marcher.Held = Person->AddProp(WreathShape, PersonKit::ForearmRight, DailyLifeModel::HandOnForearm(1.0, Outfit.Height), DailyLifeModel::HoldUpright(Raise));
		}
	}
	return true;
}

bool ADailyLife::StartFire()
{
	using namespace DailyLifeRun;
	ATrafficManager* Manager = Traffic();
	if (!Manager)
	{
		return false;
	}
	const FVector2D Player2D = Flat(PlayerAt);
	for (int32 Try = 0; Try < 5; ++Try)
	{
		int32 Lane = INDEX_NONE;
		int32 Point = INDEX_NONE;
		FVector Start = FVector::ZeroVector;
		if (!PickRoadStart(7000.0, 11000.0, nullptr, Lane, Point, Start))
		{
			return false;
		}
		// Racing past the player to somewhere beyond him.
		const FVector2D Beyond = Player2D + (Player2D - Flat(Start)).GetSafeNormal() * 10000.0;
		TArray<FVector> Way;
		if (!Manager->FindPath(Start, FVector(Beyond, Start.Z), Way) || Way.Num() < 2 || IsSeen(Way[0]))
		{
			continue;
		}
		double Closest = TNumericLimits<double>::Max();
		for (int32 i = 0; i + 1 < Way.Num(); ++i)
		{
			const FVector2D Near = FMath::ClosestPointOnSegment2D(Player2D, Flat(Way[i]), Flat(Way[i + 1]));
			Closest = FMath::Min(Closest, FVector2D::Distance(Near, Player2D));
		}
		if (Closest > PassNear)
		{
			continue;
		}
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AFireEngine* Fire = GetWorld()->SpawnActor<AFireEngine>(AFireEngine::StaticClass(), Way[0] + FVector(0.0, 0.0, 100.0), FRotator::ZeroRotator, Params);
		if (!Fire)
		{
			return false;
		}
		Fire->Setup(Way, Manager);
		Brigade = Fire;
		return true;
	}
	return false;
}

void ADailyLife::TickMarchers(float DeltaSeconds)
{
	using namespace DailyLifeRun;
	const bool bWedding = Event == EEvent::Wedding;
	const double Before = EventS;
	ATrafficCart* Cart = Hearse.Get();

	if (bWedding)
	{
		// They walk on, and at the church go in one by one.
		EventS += ColumnWalk * DeltaSeconds;
		if (USpotSound* Sound = EventSound.Get())
		{
			if (Marchers.Num() > 0 && Marchers[0].Person.IsValid())
			{
				Sound->SetWorldLocation(Marchers[0].Person->GetActorLocation());
			}
			Hear(Sound, FiddleReach);
		}
	}
	else if (Cart && Cart->IsInTraffic())
	{
		// Behind the hearse, along where it has been.
		const FVector Now3D = Cart->GetActorLocation();
		if (Trail.Num() == 0 || FVector::Dist2D(Trail.Last(), Now3D) > 40.0)
		{
			const double Added = Trail.Num() > 0 ? FVector::Dist2D(Trail.Last(), Now3D) : 0.0;
			Trail.Add(Now3D);
			EventAlong.Add((EventAlong.Num() > 0 ? EventAlong.Last() : 0.0) + Added);
		}
		EventS = EventAlong.Num() > 0 ? EventAlong.Last() + FVector::Dist2D(Trail.Last(), Now3D) : 0.0;
		if (!bEventEnding && FVector2D::Distance(Flat(Now3D), CemeteryGate) < GateNear)
		{
			BeginEnding();
		}
	}
	else if (!bMournersAngry)
	{
		// Somebody took the hearse.
		bMournersAngry = true;
		AngrySince = Now;
		if (FVector::Dist2D(PlayerAt, Marchers.Num() > 0 && Marchers[0].Person.IsValid() ? Marchers[0].Person->GetActorLocation() : PlayerAt) < 3000.0)
		{
			Say(TEXT("Gy\u00e1szol\u00f3"), TEXT("Gyal\u00e1zat! Ellopta a halottaskocsit!"), TEXT("Shame on him! He stole the hearse!"));
		}
		BeginEnding();
	}
	ColumnSpeed = DeltaSeconds > 0.f ? static_cast<float>((EventS - Before) / DeltaSeconds) : 0.f;
	ColumnSpeed = FMath::Clamp(ColumnSpeed, 0.f, 250.f);

	const TArray<FVector>& Path = bWedding ? EventWay : Trail;
	const double Total = EventAlong.Num() > 0 ? EventAlong.Last() : 0.0;
	const float Clock = static_cast<float>(Now - EventStarted);
	TArray<FVector> Block;
	FVector Front = FVector::ZeroVector;
	FVector FrontDir = FVector::ForwardVector;
	FVector Tail = FVector::ZeroVector;
	FVector TailDir = FVector::ForwardVector;
	bool bAnyone = false;
	double Nearest = TNumericLimits<double>::Max();

	for (int32 m = 0; m < Marchers.Num(); ++m)
	{
		FMarcher& Marcher = Marchers[m];
		APedestrian* Person = Marcher.Person.Get();
		if (!Person || Marcher.bGone)
		{
			continue;
		}
		const double S = bWedding ? EventS - Marcher.Behind : EventS - HearseGap - Marcher.Behind;
		if (bWedding && S >= Total)
		{
			// Into the church.
			Marcher.bGone = true;
			RemovePerson(Person);
			continue;
		}
		FVector Dir = FVector::ForwardVector;
		const FVector OnPath = PathPoint(Path, EventAlong, FMath::Clamp(S, 0.0, Total), Dir);
		const FVector2D Right(-Dir.Y, Dir.X);
		const FVector2D Spot = Flat(OnPath) + Right * Marcher.Side;
		Marcher.GroundClock -= DeltaSeconds;
		if (Marcher.GroundClock <= 0.f)
		{
			Marcher.GroundClock = 0.4f;
			GroundAt(Spot, Marcher.Ground);
		}
		const FVector Middle(Spot, Marcher.Ground + MiddleUp);
		const double Facing = YawOf(FVector2D(Dir.X, Dir.Y));
		Nearest = FMath::Min(Nearest, FVector2D::Distance(Spot, Flat(PlayerAt)));

		DailyLifeModel::FLimbs Limbs;
		if (Marcher.Doing == 1)
		{
			DailyLifeModel::Fiddling(Limbs, Clock * 1.6f);
		}
		else if (Marcher.Doing == 2)
		{
			DailyLifeModel::HoldingUp(Limbs);
		}
		if (bMournersAngry && Now - AngrySince < 10.0)
		{
			Person->ShakeFist(Middle, YawOf(Flat(PlayerAt) - Spot), Clock);
		}
		else if (ColumnSpeed < 5.f || bMournersAngry)
		{
			Person->Strike(Middle, Facing, Limbs);
		}
		else
		{
			Person->WalkTo(Middle, Facing, ColumnSpeed, DeltaSeconds);
			if (Marcher.Doing != 0)
			{
				Person->ArmsAs(Limbs);
			}
		}
		if (!bAnyone)
		{
			Front = Middle;
			FrontDir = Dir;
			bAnyone = true;
		}
		Tail = Middle;
		TailDir = Dir;
		if (m % 3 == 0)
		{
			Block.Add(Middle);
		}
	}

	// The traffic waits for them.
	if (ATrafficManager* Manager = Traffic())
	{
		const int32 Slot = bWedding ? WeddingSlot : FuneralSlot;
		if (bAnyone && !bMournersAngry)
		{
			Block.Add(Front + FrontDir * 350.0);
			Block.Add(Tail - TailDir * 250.0);
			Manager->SetRoadBlock(Slot, Block);
		}
		else
		{
			Manager->SetRoadBlock(Slot, TArray<FVector>());
		}
	}
	if (!bAnyone)
	{
		// All in the church (or gone): over.
		BeginEnding();
		return;
	}
	if (!bEventEnding && Nearest > EventFar)
	{
		BeginEnding();
	}
	if (!bMournersAngry && Nearest < HearSay && Now >= NextEventShout)
	{
		NextEventShout = Now + 30.0;
		if (bWedding)
		{
			Say(TEXT("N\u00e1szn\u00e9p"), TEXT("\u00c9ljen az ifj\u00fa p\u00e1r!"), TEXT("Long live the young couple!"));
		}
		else
		{
			Say(TEXT("Gy\u00e1szol\u00f3"), TEXT("Csendesen, fiam. Temet\u00e9s megy."), TEXT("Quietly, son. A funeral is passing."));
		}
	}
}

// ---- Trams -------------------------------------------------------------------

void ADailyLife::TickTrams(float DeltaSeconds)
{
	using namespace DailyLifeRun;
	TickRiders(DeltaSeconds);
	if (Now < NextTramLook)
	{
		return;
	}
	NextTramLook = Now + 0.5;
	ATrafficManager* Manager = Traffic();
	if (!Manager || TramWaits.Num() == 0)
	{
		return;
	}
	for (const TObjectPtr<ATram>& Car : Manager->GetTrams())
	{
		ATram* Tram = Car.Get();
		if (!IsValid(Tram) || !Tram->IsInService() || !Tram->IsAtStop())
		{
			continue;
		}
		// Each stop once.
		const int32 Stop = Tram->GetNextStop();
		const TWeakObjectPtr<ATram> Key(Tram);
		const int32* Done = TramStopsDone.Find(Key);
		if (Done && *Done == Stop)
		{
			continue;
		}
		TramStopsDone.Add(Key, Stop);
		const FVector TramAt = Tram->GetActorLocation();
		if (FVector::Dist2D(TramAt, PlayerAt) > TramNear || Riders.Num() >= RidersAtOnce || Dice.FRand() > TramShare)
		{
			continue;
		}
		// Where the people wait at this stop.
		int32 WaitIndex = INDEX_NONE;
		double WaitGap = WaitNear;
		for (int32 w = 0; w < TramWaits.Num(); ++w)
		{
			const double Gap = FVector2D::Distance(TramWaits[w], Flat(TramAt));
			if (Gap < WaitGap)
			{
				WaitGap = Gap;
				WaitIndex = w;
			}
		}
		if (WaitIndex == INDEX_NONE)
		{
			continue;
		}
		const FVector2D Wait = TramWaits[WaitIndex];
		const FVector Door = Tram->GetDoorNear(FVector(Wait, TramAt.Z));
		const FVector2D Along = Flat(Tram->GetActorForwardVector()).GetSafeNormal();

		// Some get off and walk away along the pavement.
		const int32 Off = FMath::Min(Dice.RandRange(0, 2), RidersAtOnce - Riders.Num());
		for (int32 i = 0; i < Off; ++i)
		{
			FRider Rider;
			if (!Tram->TakeOffPassenger(Rider.Outfit))
			{
				break;
			}
			const FVector2D Step = Flat(Door) + Along * Dice.FRandRange(-40.0f, 40.0f);
			double Ground = Door.Z;
			GroundAt(Step, Ground);
			const FVector2D Off2D = Wait + FVector2D(Dice.FRandRange(-80.0f, 80.0f), Dice.FRandRange(-80.0f, 80.0f));
			APedestrian* Person = MakePerson(FVector(Step, Ground), YawOf(Off2D - Step), Rider.Outfit);
			if (!Person)
			{
				break;
			}
			Rider.Person = Person;
			Rider.Tram = Tram;
			Rider.bBoarding = false;
			Rider.Wait = Wait;
			Rider.Ground = Ground;
			Rider.Started = Now;
			Rider.Speed = Dice.FRandRange(120.f, 150.f);
			Rider.Path.Add(Step);
			Rider.Path.Add(Off2D);
			Rider.Path.Add(Off2D + Along * (Dice.FRand() < 0.5f ? 1.0 : -1.0) * Dice.FRandRange(1500.0f, 2200.0f));
			Rider.Next = 1;
			Riders.Add(Rider);
		}

		// Others waiting step up and get on.
		const int32 On = FMath::Min(Dice.RandRange(0, 2), RidersAtOnce - Riders.Num());
		for (int32 i = 0; i < On; ++i)
		{
			FRider Rider;
			FVector Ground = FVector::ZeroVector;
			float FromYaw = 0.f;
			if (!APedestrians::TakeWitness(this, FVector(Wait, Door.Z), WaitNear, 300.0, Rider.Outfit, Ground, FromYaw))
			{
				break;
			}
			APedestrian* Person = MakePerson(Ground, FromYaw, Rider.Outfit);
			if (!Person)
			{
				break;
			}
			Rider.Person = Person;
			Rider.Tram = Tram;
			Rider.bBoarding = true;
			Rider.Wait = Wait;
			Rider.Ground = Ground.Z;
			Rider.Started = Now;
			Rider.Speed = 170.f;
			Rider.Path.Add(Flat(Ground));
			Rider.Path.Add(Flat(Door));
			Rider.Next = 1;
			Riders.Add(Rider);
			Tram->HoldAtStop(3.f);
		}
	}
}

void ADailyLife::TickRiders(float DeltaSeconds)
{
	using namespace DailyLifeRun;
	for (int32 r = Riders.Num() - 1; r >= 0; --r)
	{
		FRider& Rider = Riders[r];
		APedestrian* Person = Rider.Person.Get();
		if (!Person)
		{
			Riders.RemoveAtSwap(r);
			continue;
		}
		ATram* Tram = Rider.Tram.Get();
		if (Rider.bBoarding)
		{
			if (Tram && Tram->IsAtStop() && Rider.Path.Num() >= 2)
			{
				// The door moves with the tram: aim at it, and the tram waits.
				Rider.Path.Last() = Flat(Tram->GetDoorNear(FVector(Rider.Wait, Tram->GetActorLocation().Z)));
				Tram->HoldAtStop(1.f);
				if (FVector2D::Distance(Rider.Path[0], Rider.Path.Last()) < DoorReach)
				{
					if (Tram->SeatPassenger(Rider.Outfit))
					{
						RemovePerson(Person);
						Riders.RemoveAtSwap(r);
						continue;
					}
					Rider.bBoarding = false;
				}
			}
			else
			{
				// It went without him: off he walks.
				Rider.bBoarding = false;
			}
			if (!Rider.bBoarding)
			{
				Rider.Path.Reset();
				Rider.Path.Add(Flat(Person->GetActorLocation()));
				Rider.Path.Add(Rider.Wait + (Rider.Wait - Rider.Path[0]).GetSafeNormal() * 1500.0);
				Rider.Next = 1;
			}
		}

		// Path[0] is where he is now; on to the next point.
		if (!Rider.Path.IsValidIndex(Rider.Next))
		{
			if (!IsSeen(FVector(Rider.Path.Last(), Rider.Ground)) || Now - Rider.Started > RiderLongest)
			{
				RemovePerson(Person);
				Riders.RemoveAtSwap(r);
				continue;
			}
			// Still in sight: a little further the same way.
			const FVector2D Last = Rider.Path.Last();
			const FVector2D Before = Rider.Path.Num() >= 2 ? Rider.Path[Rider.Path.Num() - 2] : Last - FVector2D(1.0, 0.0);
			Rider.Path.Add(Last + (Last - Before).GetSafeNormal() * 800.0);
		}
		const FVector2D Target = Rider.Path[Rider.Next];
		const FVector2D Way = Target - Rider.Path[0];
		const double Left = Way.Size();
		const double Step = Rider.Speed * DeltaSeconds;
		if (Left <= Step)
		{
			Rider.Path[0] = Target;
			if (!Rider.bBoarding)
			{
				++Rider.Next;
			}
		}
		else
		{
			Rider.Path[0] += Way / Left * Step;
		}
		Rider.GroundClock -= DeltaSeconds;
		if (Rider.GroundClock <= 0.f)
		{
			Rider.GroundClock = 0.4f;
			GroundAt(Rider.Path[0], Rider.Ground);
		}
		const double Facing = Left > 1.0 ? YawOf(Way) : Person->GetActorRotation().Yaw;
		Person->WalkTo(FVector(Rider.Path[0], Rider.Ground + MiddleUp), Facing, Left > 1.0 ? Rider.Speed : 0.f, DeltaSeconds);
	}
}

// ---- Shop shutters -----------------------------------------------------------

bool ADailyLife::ShopFoot(FShopFront& Shop, FVector& OutFoot)
{
	if (!Shop.bGroundKnown)
	{
		// The pavement just in front of it (as AStreetFurniture puts the shop).
		double Ground = 0.0;
		if (!GroundAt(Shop.Foot + DailyLifeRun::Heading(Shop.Yaw) * 150.0, Ground))
		{
			return false;
		}
		Shop.Ground = Ground;
		Shop.bGroundKnown = true;
	}
	OutFoot = FVector(Shop.Foot, Shop.Ground);
	return true;
}

void ADailyLife::RemoveShutter(FShutter& Shutter)
{
	RemoveMesh(Shutter.Panel.Get());
	RemoveMesh(Shutter.Roller.Get());
	RemovePerson(Shutter.Keeper.Get());
	Shutter.Panel.Reset();
	Shutter.Roller.Reset();
	Shutter.Keeper.Reset();
}

void ADailyLife::TickShutters(float DeltaSeconds)
{
	using namespace DailyLifeRun;
	const float Hour = HourNow();
	auto OpensAt = [this](int32 Shop)
	{
		return OpenFrom + DayChance(21, Shop) * OpenSpread;
	};
	auto IsShut = [&](int32 Shop)
	{
		return Hour >= ShutFrom || Hour < OpensAt(Shop);
	};

	for (int32 i = Shutters.Num() - 1; i >= 0; --i)
	{
		FShutter& Shutter = Shutters[i];
		FShopFront& Shop = Shops[Shutter.Shop];
		FVector Foot = FVector::ZeroVector;
		ShopFoot(Shop, Foot);
		const FVector2D Foot2D = Flat(Foot);
		const double Far = FVector2D::Distance(Foot2D, Flat(PlayerAt));
		const bool bSeen = IsSeen(Foot + Heading(Shop.Yaw) * 60.0);
		APedestrian* Keeper = Shutter.Keeper.Get();

		// The keeper comes along, pushes it up with his hook and goes in.
		if (Keeper)
		{
			Shutter.Clock += DeltaSeconds;
			const FVector2D Push = InFrame(Foot2D, Shop.Yaw, KeeperPush.X, KeeperPush.Y);
			const FVector2D DoorAt = InFrame(Foot2D, Shop.Yaw, ShopModel::FrontX - 10.0, ShopModel::DoorMiddle(Shop.Width, Shop.Seed));
			const FVector2D Goal = Shutter.KeeperStep == 0 ? Push : DoorAt;
			const FVector2D Way = Goal - Shutter.KeeperAt;
			const double Left = Way.Size();
			DailyLifeModel::FLimbs Limbs;
			if (Shutter.KeeperStep == 1)
			{
				Shutter.Lift = FMath::Min(Shutter.Lift + DeltaSeconds / LiftSeconds, 1.f);
				const float Raise = DailyLifeModel::PushingUp(Limbs, Shutter.Lift);
				Keeper->Strike(FVector(Shutter.KeeperAt, Foot.Z + MiddleUp), Shop.Yaw + 180.0, Limbs);
				if (UProceduralMeshComponent* Hook = Shutter.Hook.Get())
				{
					Hook->SetRelativeRotation(DailyLifeModel::HoldUpright(Raise, -25.f));
				}
				if (Shutter.Lift >= 1.f)
				{
					Shutter.KeeperStep = 2;
					if (Far < HearSay)
					{
						Say(TEXT("Boltos"), TEXT("J\u00f3 reggelt!"), TEXT("Good morning!"));
					}
				}
			}
			else if (Left < 8.0)
			{
				if (Shutter.KeeperStep == 0)
				{
					Shutter.KeeperStep = 1;
				}
				else
				{
					// In through his door.
					RemovePerson(Keeper);
					Shutter.Keeper.Reset();
				}
			}
			else
			{
				Shutter.KeeperAt += Way / Left * FMath::Min(Left, static_cast<double>(KeeperWalk * DeltaSeconds));
				Keeper->WalkTo(FVector(Shutter.KeeperAt, Foot.Z + MiddleUp), YawOf(Way), KeeperWalk, DeltaSeconds);
				DailyLifeModel::PoleCarry(Limbs);
				Keeper->ArmsAs(Limbs);
				if (UProceduralMeshComponent* Hook = Shutter.Hook.Get())
				{
					Hook->SetRelativeRotation(DailyLifeModel::HoldUpright(DailyLifeModel::PoleCarried));
				}
			}
			if (UProceduralMeshComponent* Panel = Shutter.Panel.Get())
			{
				Panel->SetWorldScale3D(FVector(1.0, 1.0, FMath::Max(1.0 - Shutter.Lift, 0.03)));
			}
			continue;
		}

		// Gone once out of sight: far away, opened, or opening time missed.
		const bool bDone = Shutter.Lift >= 1.f || (!IsShut(Shutter.Shop) && Hour >= OpenLatest);
		if (!bSeen && (Far > ShopFar || bDone))
		{
			RemoveShutter(Shutter);
			Shutters.RemoveAtSwap(i);
			continue;
		}
		// Opening time: his keeper comes from along the pavement, from where nobody sees him come.
		if (Shutter.Lift <= 0.f && !IsShut(Shutter.Shop) && Hour < OpenLatest && Now >= Shutter.Clock)
		{
			Shutter.Clock = Now + 5.0;
			for (const double Side : { 1.0, -1.0 })
			{
				const FVector2D From = InFrame(Foot2D, Shop.Yaw, ShopModel::FrontX + 140.0, Side * KeeperFrom);
				double Ground = Foot.Z;
				GroundAt(From, Ground);
				if (IsSeen(FVector(From, Ground)))
				{
					continue;
				}
				FRandomStream Looks(static_cast<int32>(Shop.Seed) + Days);
				const PersonKit::FOutfit Outfit = DailyLifeModel::ShopkeeperLook(Looks);
				APedestrian* Man = MakePerson(FVector(From, Ground), Shop.Yaw - 90.0 * Side, Outfit);
				if (!Man)
				{
					break;
				}
				PersonKit::FPiece HookShape;
				DailyLifeModel::Pole(HookShape, 150.0, 20.0, false);
				Shutter.Hook = Man->AddProp(HookShape, PersonKit::ForearmRight, DailyLifeModel::HandOnForearm(1.0, Outfit.Height),
					DailyLifeModel::HoldUpright(DailyLifeModel::PoleCarried));
				Shutter.Keeper = Man;
				Shutter.KeeperAt = From;
				Shutter.KeeperStep = 0;
				Shutter.Clock = 0.f;
				break;
			}
		}
	}

	if (Now < NextShopLook)
	{
		return;
	}
	NextShopLook = Now + 2.0;
	// Down already where the player comes at night, out of his sight.
	const FVector2D Player2D = Flat(PlayerAt);
	for (int32 s = 0; s < Shops.Num() && Shutters.Num() < ShuttersAtOnce; ++s)
	{
		FShopFront& Shop = Shops[s];
		if (Shop.bClosed || !IsShut(s) || DayChance(20, s) >= ShutterShare || FVector2D::Distance(Shop.Foot, Player2D) > ShopNear)
		{
			continue;
		}
		bool bHas = false;
		for (const FShutter& Other : Shutters)
		{
			bHas |= Other.Shop == s;
		}
		FVector Foot = FVector::ZeroVector;
		if (bHas || !ShopFoot(Shop, Foot) || IsSeen(Foot + Heading(Shop.Yaw) * 60.0))
		{
			continue;
		}
		const double Inner = Shop.Width * 0.5 - ShopModel::PilasterWidth;
		if (Inner < 60.0)
		{
			continue;
		}
		const FRotator Turn(0.0, Shop.Yaw, 0.0);
		FShutter& Shutter = Shutters.AddDefaulted_GetRef();
		Shutter.Shop = s;
		PersonKit::FPiece PanelShape;
		DailyLifeModel::Shutter(PanelShape, Inner * 2.0, ShopModel::WindowTop - ShopModel::PlinthTop);
		Shutter.Panel = MakeMesh(PanelShape, FTransform(Turn, Foot + Turn.RotateVector(FVector(ShopModel::FrontX + ShopModel::StandOff + 1.0, 0.0, ShopModel::WindowTop))));
		PersonKit::FPiece BoxShape;
		DailyLifeModel::ShutterBox(BoxShape, Inner * 2.0);
		Shutter.Roller = MakeMesh(BoxShape, FTransform(Turn, Foot + Turn.RotateVector(FVector(ShopModel::FrontX + ShopModel::StandOff, 0.0, ShopModel::WindowTop))));
	}
}
