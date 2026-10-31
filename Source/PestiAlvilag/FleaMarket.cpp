// Fill out your copyright notice in the Description page of Project Settings.

#include "FleaMarket.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Lighters.h"
#include "MarketModel.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PersonModel.h"
#include "PestiAlvilagGameModeBase.h"
#include "PestiCar.h"
#include "PestiPerson.h"
#include "PoliceCar.h"
#include "PolicePatrols.h"
#include "ProceduralMeshComponent.h"
#include "StoryPerson.h"
#include "UObject/ConstructorHelpers.h"
#include "PestiHud.h"
#include "Wreck.h"

namespace MarketBuild
{
	// People are drawn up to this far away, the stalls and sheds further (cm).
	constexpr float PeopleDrawDistance = 9000.f;
	constexpr int32 GoodsDrawDistance = 14000;

	// The carpet seller, and how close to him you must be to talk.
	const TCHAR* const CarpetSeller = TEXT("Ibn Iszh\u00e1k");
	constexpr double TalkRadius = 260.0;
	constexpr int32 HintLine = 32;
	constexpr int32 SpeechLine = 6;
	const TCHAR* const CarpetLines[] = {
		TEXT("Ibn Iszh\u00e1k: \u201eSz\u0151nyeget, uram? Egyenesen Bagdadb\u00f3l!\u201d  (A carpet, sir? Straight from Baghdad!)"),
		TEXT("Ibn Iszh\u00e1k: \u201eA sasom? R\u00e9gi bar\u00e1tom. Most a Keleti f\u00f6l\u00f6tt k\u00f6r\u00f6z.\u201d  (My eagle? An old friend. These days he circles over the Keleti.)"),
		TEXT("Ibn Iszh\u00e1k: \u201eA tet\u0151kr\u0151l mindent l\u00e1tni. A t\u00f6meg pedig mindent elrejt.\u201d  (From the rooftops you see everything. And the crowd hides everything.)"),
		TEXT("Ibn Iszh\u00e1k: \u201eSemmi sem igaz, minden megengedett.\u201d  (Nothing is true, everything is permitted.)"),
	};

	bool ReadNumbers(const FJsonObject& Object, const TCHAR* Field, TArray<double>& Out)
	{
		const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
		if (!Object.TryGetArrayField(Field, Values))
		{
			return false;
		}
		for (const TSharedPtr<FJsonValue>& Value : *Values)
		{
			Out.Add(Value->AsNumber());
		}
		return true;
	}

	MarketModel::EBuild BuildOf(const FString& Name)
	{
		return Name == TEXT("blanket") ? MarketModel::EBuild::Blanket
			: Name == TEXT("shed") ? MarketModel::EBuild::Shed
			: MarketModel::EBuild::Table;
	}

	MarketModel::EGoods GoodsOf(const FString& Name)
	{
		using MarketModel::EGoods;
		return Name == TEXT("clothes") ? EGoods::Clothes
			: Name == TEXT("shoes") ? EGoods::Shoes
			: Name == TEXT("pots") ? EGoods::Pots
			: Name == TEXT("junk") ? EGoods::Junk
			: Name == TEXT("books") ? EGoods::Books
			: Name == TEXT("furniture") ? EGoods::Furniture
			: Name == TEXT("produce") ? EGoods::Produce
			: Name == TEXT("carpets") ? EGoods::Carpets
			: EGoods::Crockery;
	}
}

namespace MarketLife
{
	constexpr double Middle = PersonModel::MiddleHeight;
	// People further than this from the camera stand still where they are (cm).
	constexpr double ActiveRange = 6000.0;
	// The player is noticed within this distance.
	constexpr double ReactRange = 3000.0;
	constexpr float SeenWithin = 1.f;

	// Shoppers look at a stall for a while, then walk to another one near
	// their own spot, standing this far in front of it.
	constexpr double BrowseRadius = 1200.0;
	constexpr double FrontGap = 85.0;
	constexpr float MinLook = 4.f;
	constexpr float MaxLook = 11.f;
	constexpr float MinWalk = 85.f;
	constexpr float MaxWalk = 125.f;
	constexpr float TurnRate = 300.f;
	constexpr float MinGesture = 3.f;
	constexpr float MaxGesture = 7.f;
	constexpr int32 Gestures = 5;
	// Sellers call out their wares when the player comes this near, each at most so often (s).
	constexpr double CallRange = 700.0;
	constexpr float MinCallPause = 12.f;
	constexpr float MaxCallPause = 25.f;

	// Walked into, run at and driven at: as in the streets (APedestrians).
	constexpr double BumpDistance = 55.0;
	constexpr double BumpSpeed = 80.0;
	constexpr double RunningSpeed = 500.0;
	constexpr double RunPastDistance = 220.0;
	constexpr double CarNoticeSpeed = 300.0;
	constexpr double CarWarnSeconds = 1.0;
	constexpr double CarWarnExtra = 250.0;
	constexpr double CarPathHalfWidth = 260.0;
	constexpr float FreezeChance = 0.25f;
	constexpr float MinNotice = 0.15f;
	constexpr float MaxNotice = 0.6f;
	constexpr double LeapSpeed = 420.0;
	constexpr double CarNear = 900.0;
	constexpr double FleeSpeed = 380.0;
	constexpr float FleeSeconds = 3.f;
	constexpr double CarHalfLength = 215.0;
	constexpr double CarHalfWidth = 95.0;
	constexpr double HitSpeed = 150.0;
	constexpr double KillSpeed = 1100.0;
	constexpr float CarSlowOnHit = 0.85f;
	constexpr double PoliceSee = 5000.0;
	constexpr double WitnessNear = 1500.0;
	constexpr double LyingHeight = 18.0;
	constexpr float FallSeconds = 0.45f;
	constexpr float LieSeconds = 2.5f;
	constexpr float GetUpSeconds = 1.2f;
	constexpr float AngrySeconds = 3.5f;
	constexpr double BloodRadius = 70.0;
	constexpr float BloodSpreads = 6.f;
	constexpr double ClearBeyond = 2500.0;
	constexpr double ShoutEvery = 4.0;
	const FLinearColor BloodColor(0.16f, 0.004f, 0.004f);

	// A car faster than this smashes a stall it drives into (cm/s); the
	// pieces fly, and after a while lie still where they fell. No more than
	// so many pieces lie about at once.
	constexpr double SmashSpeed = 250.0;
	constexpr double SmashReach = 900.0;
	constexpr float CarSlowOnSmash = 0.8f;
	constexpr double ScareNear = 800.0;
	constexpr double SettleAfter = 8.0;
	constexpr int32 MaxWreckage = 700;
	// Smashed stalls stand again only a minute later at the earliest, once
	// the camera is this far away (cm) and nobody sees.
	constexpr double ResetDistance = 15000.0;
	constexpr double ResetAfter = 60.0;
	constexpr int32 ResetChecksPerFrame = 4;

	const TCHAR* const Seller = TEXT("\u00c1rus");
	const TCHAR* const Shopper = TEXT("V\u00e1s\u00e1rl\u00f3");

	struct FLine2 { const TCHAR* Hu; const TCHAR* En; };
	// Two lines for each kind of goods (MarketModel::EGoods order).
	const FLine2 GoodsLines[][2] = {
		{ { TEXT("Majdnem \u00faj kab\u00e1t, uram, csak egyszer hordt\u00e1k!"), TEXT("A nearly new coat, sir, worn only once!") },
		  { TEXT("Ez a kalap egy gr\u00f3f\u00e9 volt, esk\u00fcsz\u00f6m."), TEXT("This hat belonged to a count, I swear.") } },
		{ { TEXT("B\u0151rtalp, uram! Ebben B\u00e9csig is els\u00e9t\u00e1l."), TEXT("Leather soles, sir! You could walk to Vienna in these.") },
		  { TEXT("Pr\u00f3b\u00e1lja fel nyugodtan, nem harap."), TEXT("Go on, try them on, they don't bite.") } },
		{ { TEXT("Ebben f\u0151zte a nagyany\u00e1m a guly\u00e1st."), TEXT("My grandmother cooked her goulash in this one.") },
		  { TEXT("J\u00f3 vastag r\u00e9z, ez kitart egy \u00e9leten \u00e1t."), TEXT("Good thick copper, it will last you a lifetime.") } },
		{ { TEXT("Mindenf\u00e9le kincs, csak keresni kell!"), TEXT("All sorts of treasure, you just have to look!") },
		  { TEXT("Hogy j\u00e1r-e ez az \u00f3ra? H\u00e1t... n\u00e9ha."), TEXT("Does this clock work? Well... sometimes.") } },
		{ { TEXT("J\u00f3kai, Miksz\u00e1th, Pet\u0151fi, fill\u00e9rek\u00e9rt!"), TEXT("J\u00f3kai, Miksz\u00e1th, Pet\u0151fi, for a few fill\u00e9r!") },
		  { TEXT("Egy j\u00f3 k\u00f6nyv t\u00f6bbet \u00e9r egy peng\u0151n\u00e9l, fiatalember."), TEXT("A good book is worth more than a peng\u0151, young man.") } },
		{ { TEXT("Sz\u00e9p sz\u00e9k, csak egy kicsit billeg."), TEXT("A fine chair. It only wobbles a little.") },
		  { TEXT("T\u00f6m\u00f6r t\u00f6lgy, uram, nem feny\u0151!"), TEXT("Solid oak, sir, not pine!") } },
		{ { TEXT("Friss alma, sz\u00e9p alma, olcs\u00f3n adom!"), TEXT("Fresh apples, lovely apples, going cheap!") },
		  { TEXT("Krumplit, hagym\u00e1t, k\u00e1poszt\u00e1t, tess\u00e9k!"), TEXT("Potatoes, onions, cabbages, come and get them!") } },
		{ { TEXT("Porcel\u00e1n cs\u00e9sze, uram, a nagymam\u00e1m is ilyenb\u0151l itta a k\u00e1v\u00e9t."), TEXT("A china cup, sir. My grandmother drank her coffee from one just like it.") },
		  { TEXT("Herendi! Na j\u00f3, majdnem herendi."), TEXT("Herend china! Well, nearly Herend.") } },
		{ { TEXT("K\u00e9zzel csom\u00f3zott, uram, n\u00e9zze csak!"), TEXT("Hand-knotted, sir, just look at it!") },
		  { TEXT("Ezen m\u00e9g a szult\u00e1n is el\u00fcld\u00f6g\u00e9lne."), TEXT("Even the sultan would be happy to sit on this one.") } },
	};
	const FLine2 CallLines[] = {
		{ TEXT("Tess\u00e9k, tess\u00e9k, olcs\u00f3n adom!"), TEXT("Come along, come along, going cheap!") },
		{ TEXT("Ide n\u00e9zzen, uram, ilyet sehol nem tal\u00e1l!"), TEXT("Look here, sir, you won't find this anywhere else!") },
		{ TEXT("Csak ma, csak most, csak mag\u00e1nak!"), TEXT("Today only, just for you!") },
	};
	const FLine2 ShopperLines[] = {
		{ TEXT("Dr\u00e1ga ez, nagyon dr\u00e1ga."), TEXT("It's dear, very dear.") },
		{ TEXT("Csak n\u00e9zel\u0151d\u00f6m, k\u00f6sz\u00f6n\u00f6m."), TEXT("I'm only looking, thank you.") },
		{ TEXT("Hallotta? Megint felment a keny\u00e9r \u00e1ra."), TEXT("Did you hear? Bread has gone up again.") },
		{ TEXT("Itt mindent megtal\u00e1l az ember, csak azt nem, amit keres."), TEXT("You can find anything here, except what you came for.") },
		{ TEXT("Vigy\u00e1zzon a zseb\u00e9re, sok itt a zsebtolvaj!"), TEXT("Mind your pockets, there are pickpockets about!") },
		{ TEXT("Alkudni kell, fiam, k\u00fcl\u00f6nben kinevetnek."), TEXT("You have to haggle, son, or they laugh at you.") },
	};
	const FLine2 BumpLines[] = {
		{ TEXT("Vigy\u00e1zzon m\u00e1r, k\u00e9rem! H\u00e1t nem l\u00e1t?"), TEXT("Watch where you are going! Are you blind?") },
		{ TEXT("Ne tolakodjon, itt mindenki sorra ker\u00fcl!"), TEXT("Don't push, everyone gets their turn!") },
		{ TEXT("Micsoda modortalans\u00e1g!"), TEXT("What bad manners!") },
	};
	const FLine2 CarLines[] = {
		{ TEXT("J\u00e9zusom, egy aut\u00f3 a piacon!"), TEXT("Good Lord, a car in the market!") },
		{ TEXT("Seg\u00edts\u00e9g! Meg\u0151r\u00fclt ez?!"), TEXT("Help! Has he gone mad?!") },
		{ TEXT("Ez piac, nem l\u00f3verseny!"), TEXT("This is a market, not a racecourse!") },
	};
	const FLine2 AngryLines[] = {
		{ TEXT("A standom! Maga gazember, ezt megfizeti!"), TEXT("My stall! You scoundrel, you will pay for this!") },
		{ TEXT("Az eg\u00e9sz heti \u00e1ru! Rend\u0151r! Rend\u0151r!"), TEXT("A whole week's goods! Police! Police!") },
		{ TEXT("Meg\u0151r\u00fclt? Mindenem odavan!"), TEXT("Are you mad? Everything I had is ruined!") },
	};
	const FLine2 GetUpLines[] = {
		{ TEXT("Jaj, a l\u00e1bam! Maga vad\u00e1llat!"), TEXT("Ow, my leg! You brute!") },
		{ TEXT("Rend\u0151r! Elg\u00e1zoltak!"), TEXT("Police! I've been run over!") },
	};
	const FLine2 WitnessLine = { TEXT("Rend\u0151r! Rend\u0151r! El\u00fct\u00f6tte!"), TEXT("Police! Police! He ran him over!") };

	template <int32 N>
	const FLine2& AnyOf(const FLine2 (&Lines)[N], FRandomStream& Random)
	{
		return Lines[Random.RandRange(0, N - 1)];
	}

	FVector2D Facing(double Yaw)
	{
		const double A = FMath::DegreesToRadians(Yaw);
		return FVector2D(FMath::Cos(A), FMath::Sin(A));
	}

	float YawOf(const FVector2D& Direction)
	{
		return static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(Direction.Y, Direction.X)));
	}
}

void AMarketPerson::Pose(const FVector& Location, double Yaw, float Lean, float ArmLeft, float ArmRight,
	float LegLeft, float LegRight, float KneeLeft, float KneeRight, bool bPlace)
{
	if (bPlace)
	{
		// Leaning forward tips him nose down.
		SetActorLocationAndRotation(Location, FRotator(-Lean, Yaw, 0.0));
	}
	PersonModel::PoseLegs(Pieces, LegLeft, LegRight, KneeLeft, KneeRight);
	const float Arms[] = { ArmLeft, ArmRight };
	for (int32 i = 0; i < 2; ++i)
	{
		if (USceneComponent* Part = Limb(PersonKit::ArmLeft + i))
		{
			Part->SetRelativeRotation(FRotator(Arms[i], 0.f, 0.f));
		}
	}
}

AFleaMarket::AFleaMarket()
{
	// Ticks to move the people and let the player talk to them.
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	Ground = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Ground"));
	Ground->SetupAttachment(RootComponent);
	Ground->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Ground->SetCanEverAffectNavigation(false);

	Roofs = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Roofs"));
	Roofs->SetupAttachment(RootComponent);
	Roofs->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Roofs->SetCanEverAffectNavigation(false);

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicShapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	BaseMaterial = BasicShapeMaterial.Object;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMesh(TEXT("/Engine/BasicShapes/Cone.Cone"));
	Shapes = { CubeMesh.Object, CylinderMesh.Object, SphereMesh.Object, ConeMesh.Object };
}

void AFleaMarket::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	for (AMarketPerson* Person : People)
	{
		if (Person)
		{
			Person->Destroy();
		}
	}
	People.Reset();
	Keepers.Reset();
	Crowd.Reset();
	Super::EndPlay(EndPlayReason);
}

AStoryPerson* AFleaMarket::FindKeeper(const UWorld* World, const FString& Name)
{
	for (TActorIterator<AFleaMarket> It(World); It; ++It)
	{
		if (const TObjectPtr<AStoryPerson>* Found = It->Keepers.Find(Name))
		{
			return *Found;
		}
	}
	return nullptr;
}

void AFleaMarket::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(this, 0);
	if (!CameraManager || DeltaSeconds <= 0.f)
	{
		return;
	}
	const FVector Camera = CameraManager->GetCameraLocation();
	const double Now = GetWorld()->GetTimeSeconds();

	// Where the player is, on foot or at the wheel, and how fast he goes
	// (the car moves itself, without physics, so from how far it went).
	APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	bPlayerAbout = Player && !Player->IsHidden();
	if (bPlayerAbout)
	{
		const FVector Here = Player->GetActorLocation();
		if (LastPlayer.Get() != Player)
		{
			LastPlayer = Player;
			LastPlayerAt = Here;
		}
		PlayerVelocity = FMath::Lerp(PlayerVelocity, (Here - LastPlayerAt) / DeltaSeconds, 0.5);
		LastPlayerAt = Here;
		PlayerAt = Here;
		PlayerForward = FVector2D(Player->GetActorForwardVector()).GetSafeNormal();
		bPlayerDriving = Player->IsA<APestiCar>();
	}

	if (!TickCarpetSeller())
	{
		TickTalk(Now);
	}
	TickStalls(Camera, DeltaSeconds, Now);
	TickCrowd(DeltaSeconds, Camera, Now);
}

bool AFleaMarket::TickCarpetSeller()
{
	const TObjectPtr<AStoryPerson>* Seller = Keepers.Find(FString(MarketBuild::CarpetSeller));
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	APestiPerson* Player = PC ? Cast<APestiPerson>(PC->GetPawn()) : nullptr;
	if (!Seller || !*Seller || !Player || Player->IsCarrying() || Player->IsRiding() || Player->IsHidden()
		|| FVector::Dist2D(Player->GetActorLocation(), (*Seller)->GetActorLocation()) > MarketBuild::TalkRadius)
	{
		return false;
	}
	if (GEngine)
	{
		APestiHUD::Show(this, MarketBuild::HintLine, 0.f, FColor(255, 210, 60), TEXT("E: talk to the carpet seller"));
	}
	if (PC->WasInputKeyJustPressed(EKeys::E))
	{
		(*Seller)->FaceTowards(Player->GetActorLocation());
		const int32 Count = static_cast<int32>(UE_ARRAY_COUNT(MarketBuild::CarpetLines));
		if (GEngine)
		{
			APestiHUD::Show(this, MarketBuild::SpeechLine, 6.f, FColor(255, 210, 60), MarketBuild::CarpetLines[CarpetLine % Count]);
		}
		CarpetLine = (CarpetLine + 1) % Count;
	}
	return true;
}

FIntPoint AFleaMarket::Add(int32 Shape, const FLinearColor& Color, const FVector& At, const FVector& Size, const FRotator& Turn, bool bCollide)
{
	if (!Shapes.IsValidIndex(Shape) || !Shapes[Shape])
	{
		return FIntPoint(INDEX_NONE, INDEX_NONE);
	}
	const FString Key = FString::Printf(TEXT("%d %s %d"), Shape, *Color.ToString(), bCollide ? 1 : 0);
	int32 BatchNumber = INDEX_NONE;
	if (const int32* Found = BatchIndex.Find(Key))
	{
		BatchNumber = *Found;
	}
	else
	{
		UInstancedStaticMeshComponent* Made = NewObject<UInstancedStaticMeshComponent>(this, NAME_None, RF_Transient);
		Made->SetupAttachment(RootComponent);
		Made->SetStaticMesh(Shapes[Shape]);
		if (BaseMaterial)
		{
			UMaterialInstanceDynamic* Paint = UMaterialInstanceDynamic::Create(BaseMaterial, this);
			Paint->SetVectorParameterValue(TEXT("Color"), Color);
			Made->SetMaterial(0, Paint);
		}
		Made->SetCollisionProfileName(bCollide ? TEXT("BlockAll") : TEXT("NoCollision"));
		Made->SetCullDistances(0, MarketBuild::GoodsDrawDistance);
		Made->SetCanEverAffectNavigation(false);
		Made->RegisterComponent();
		BatchNumber = Batches.Add(Made);
		BatchIndex.Add(Key, BatchNumber);
	}
	const int32 Instance = Batches[BatchNumber]->AddInstance(FTransform(Turn, At, Size / 100.0), true);
	return FIntPoint(BatchNumber, Instance);
}

void AFleaMarket::Setup(const FJsonObject& Market)
{
	// The square is flat: find its height once, under the first stall.
	const TArray<TSharedPtr<FJsonValue>>* Stalls = nullptr;
	Market.TryGetArrayField(TEXT("stalls"), Stalls);
	if (Stalls && Stalls->Num() > 0)
	{
		const TArray<TSharedPtr<FJsonValue>>& At = (*Stalls)[0]->AsObject()->GetArrayField(TEXT("at"));
		const FVector Probe(At[0]->AsNumber(), At[1]->AsNumber(), 0.0);
		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(MarketGround), false, this);
		if (GetWorld()->LineTraceSingleByChannel(Hit, Probe + FVector(0.0, 0.0, 10000.0), Probe - FVector(0.0, 0.0, 10000.0), ECC_Visibility, Params))
		{
			GroundZ = Hit.ImpactPoint.Z;
		}
	}

	BuildGround(Market);

	// The sheds stand firm, whatever drives into them.
	TArray<MarketModel::FShape> ShedPieces;
	const TArray<TSharedPtr<FJsonValue>>* Sheds = nullptr;
	if (Market.TryGetArrayField(TEXT("sheds"), Sheds))
	{
		for (const TSharedPtr<FJsonValue>& Value : *Sheds)
		{
			const TArray<TSharedPtr<FJsonValue>>& S = Value->AsArray();
			if (S.Num() >= 5)
			{
				const FVector Middle(S[0]->AsNumber(), S[1]->AsNumber(), GroundZ);
				MarketModel::BuildShed(ShedPieces, Middle, S[2]->AsNumber(), S[3]->AsNumber(), S[4]->AsNumber());
				BuildShedRoof(Middle, S[2]->AsNumber(), S[3]->AsNumber(), S[4]->AsNumber());
			}
		}
	}
	for (const MarketModel::FShape& Piece : ShedPieces)
	{
		Add(Piece.Shape, Piece.Color, Piece.At, Piece.Size, FRotator(Piece.Pitch, Piece.Yaw, Piece.Roll), Piece.bCollide);
	}

	Roofs->CreateMeshSection_LinearColor(0, RoofVertices, RoofTriangles, RoofNormals,
		TArray<FVector2D>(), TArray<FLinearColor>(), TArray<FProcMeshTangent>(), false);
	if (BaseMaterial)
	{
		UMaterialInstanceDynamic* Paint = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		Paint->SetVectorParameterValue(TEXT("Color"), MarketModel::ShedRoof());
		Roofs->SetMaterial(0, Paint);
		BloodPaint = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		BloodPaint->SetVectorParameterValue(TEXT("Color"), MarketLife::BloodColor);
	}
	RoofVertices.Empty();
	RoofTriangles.Empty();
	RoofNormals.Empty();

	struct FSeller
	{
		FVector Feet;
		double Yaw;
		FString Keeper;
		int32 Stall;
	};
	TArray<FSeller> Sellers;
	if (Stalls)
	{
		for (const TSharedPtr<FJsonValue>& Value : *Stalls)
		{
			const TSharedPtr<FJsonObject> StallObject = Value->AsObject();
			const TArray<TSharedPtr<FJsonValue>>* At = nullptr;
			if (!StallObject || !StallObject->TryGetArrayField(TEXT("at"), At) || At->Num() < 3)
			{
				continue;
			}
			FStall& Stall = StallList.AddDefaulted_GetRef();
			Stall.Middle = FVector((*At)[0]->AsNumber(), (*At)[1]->AsNumber(), GroundZ);
			Stall.Yaw = (*At)[2]->AsNumber();
			Stall.Build = MarketBuild::BuildOf(StallObject->GetStringField(TEXT("build")));
			Stall.Goods = MarketBuild::GoodsOf(StallObject->GetStringField(TEXT("goods")));
			MarketModel::FRandom Random(GetTypeHash(FIntPoint(FMath::RoundToInt(Stall.Middle.X), FMath::RoundToInt(Stall.Middle.Y))));
			TArray<MarketModel::FShape> Pieces;
			const double Back = MarketModel::BuildStall(Pieces, Stall.Middle, Stall.Yaw, Stall.Build, Stall.Goods, Random);

			// Each piece on its own, so it can fly when a car drives through;
			// a box round the solid ones (the table, a chair, the carpet
			// frame) keeps the player out of the stall instead.
			const FVector2D Forward = MarketLife::Facing(Stall.Yaw);
			const FVector2D Right(-Forward.Y, Forward.X);
			FBox All(ForceInit);
			FBox Solid(ForceInit);
			for (int32 i = 0; i < Pieces.Num(); ++i)
			{
				const MarketModel::FShape& Piece = Pieces[i];
				const FRotator Turn(Piece.Pitch, Piece.Yaw, Piece.Roll);
				const FIntPoint Made = Add(Piece.Shape, Piece.Color, Piece.At, Piece.Size, Turn, false);
				if (Made.X == INDEX_NONE)
				{
					continue;
				}
				FPart& Part = Stall.Parts.AddDefaulted_GetRef();
				Part.Batch = Made.X;
				Part.Instance = Made.Y;
				Part.Shape = Piece.Shape;
				Part.Transform = FTransform(Turn, Piece.At, Piece.Size / 100.0);
				// The blanket on the ground is the first piece of a blanket stall.
				Part.bStays = Stall.Build == MarketModel::EBuild::Blanket && i == 0;

				const FVector2D Offset = FVector2D(Piece.At - Stall.Middle);
				const FVector Local(FVector2D::DotProduct(Offset, Forward), FVector2D::DotProduct(Offset, Right), Piece.At.Z - GroundZ);
				const FBox Box(Local - Piece.Size * 0.5, Local + Piece.Size * 0.5);
				All += Box;
				if (Piece.bCollide)
				{
					Solid += Box;
				}
			}
			if (All.IsValid)
			{
				Stall.Half = FVector2D(FMath::Max(FMath::Abs(All.Min.X), FMath::Abs(All.Max.X)), FMath::Max(FMath::Abs(All.Min.Y), FMath::Abs(All.Max.Y)));
			}
			if (Solid.IsValid)
			{
				const FVector Centre = Solid.GetCenter();
				UBoxComponent* Box = NewObject<UBoxComponent>(this, NAME_None, RF_Transient);
				Box->SetupAttachment(RootComponent);
				Box->SetBoxExtent(Solid.GetExtent());
				Box->SetWorldLocationAndRotation(FVector(FVector2D(Stall.Middle) + Forward * Centre.X + Right * Centre.Y, GroundZ + Centre.Z),
					FRotator(0.0, Stall.Yaw, 0.0));
				Box->SetCollisionProfileName(TEXT("BlockAll"));
				Box->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
				Box->SetCanEverAffectNavigation(false);
				Box->RegisterComponent();
				Stall.Blocker = Blockers.Add(Box);
			}
			FString Keeper;
			StallObject->TryGetStringField(TEXT("keeper"), Keeper);
			if (StallObject->GetBoolField(TEXT("seller")) || !Keeper.IsEmpty())
			{
				const double Along = Random.Range(-60.0, 60.0);
				const FVector2D Feet = FVector2D(Stall.Middle) - Forward * Back + Right * Along;
				Sellers.Add({ FVector(Feet, GroundZ), Stall.Yaw, Keeper, StallList.Num() - 1 });
			}
		}
	}

	FRandomStream Random(1932);
	CrowdRandom.Initialize(1933);
	for (const FSeller& Seller : Sellers)
	{
		if (SpawnPerson(Seller.Feet, Seller.Yaw, Random, Seller.Keeper))
		{
			FCrowd& Who = Crowd.Last();
			Who.Stall = Seller.Stall;
			StallList[Seller.Stall].Seller = Crowd.Num() - 1;
		}
	}
	const TArray<TSharedPtr<FJsonValue>>* Shoppers = nullptr;
	if (Market.TryGetArrayField(TEXT("shoppers"), Shoppers))
	{
		for (const TSharedPtr<FJsonValue>& Value : *Shoppers)
		{
			const TArray<TSharedPtr<FJsonValue>>& S = Value->AsArray();
			if (S.Num() >= 3)
			{
				SpawnPerson(FVector(S[0]->AsNumber(), S[1]->AsNumber(), GroundZ), S[2]->AsNumber(), Random, FString(), true);
			}
		}
	}
}

void AFleaMarket::BuildGround(const FJsonObject& Market)
{
	// Trodden earth over the whole square, a little above the lawn the map draws there.
	TArray<double> Ring;
	TArray<double> Indices;
	if (!MarketBuild::ReadNumbers(Market, TEXT("ground"), Ring) || !MarketBuild::ReadNumbers(Market, TEXT("tris"), Indices))
	{
		return;
	}
	TArray<FVector> Vertices;
	TArray<FVector> Normals;
	for (int32 i = 0; i + 1 < Ring.Num(); i += 2)
	{
		Vertices.Add(FVector(Ring[i], Ring[i + 1], GroundZ + 3.0));
		Normals.Add(FVector::UpVector);
	}
	TArray<int32> Triangles;
	for (int32 i = 0; i + 2 < Indices.Num(); i += 3)
	{
		const int32 A = static_cast<int32>(Indices[i]);
		int32 B = static_cast<int32>(Indices[i + 1]);
		int32 C = static_cast<int32>(Indices[i + 2]);
		if (!Vertices.IsValidIndex(A) || !Vertices.IsValidIndex(B) || !Vertices.IsValidIndex(C))
		{
			continue;
		}
		// Unreal shows a triangle from the side (C - A) ^ (B - A) points to: up.
		if (FVector::CrossProduct(Vertices[C] - Vertices[A], Vertices[B] - Vertices[A]).Z < 0.0)
		{
			Swap(B, C);
		}
		Triangles.Append({ A, B, C });
	}
	Ground->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals,
		TArray<FVector2D>(), TArray<FLinearColor>(), TArray<FProcMeshTangent>(), false);
	if (BaseMaterial)
	{
		UMaterialInstanceDynamic* Paint = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		Paint->SetVectorParameterValue(TEXT("Color"), MarketModel::Earth());
		Ground->SetMaterial(0, Paint);
	}
}

void AFleaMarket::BuildShedRoof(const FVector& Middle, double Yaw, double Width, double Depth)
{
	using namespace MarketModel;
	// A pyramid roof: four sloping faces from the eaves up to the apex,
	// seen from above and below.
	const FTransform Frame(FRotator(0.0, Yaw, 0.0), Middle);
	const double OX = Depth * 0.5 + ShedOverhang;
	const double OY = Width * 0.5 + ShedOverhang;
	const FVector Apex = Frame.TransformPosition(FVector(0.0, 0.0, ShedApex));
	const FVector Corners[4] = {
		Frame.TransformPosition(FVector(-OX, -OY, ShedEaves - 20.0)), Frame.TransformPosition(FVector(OX, -OY, ShedEaves - 20.0)),
		Frame.TransformPosition(FVector(OX, OY, ShedEaves - 20.0)), Frame.TransformPosition(FVector(-OX, OY, ShedEaves - 20.0)) };
	for (int32 i = 0; i < 4; ++i)
	{
		const FVector& A = Corners[i];
		const FVector& B = Corners[(i + 1) % 4];
		FVector Normal = FVector::CrossProduct(B - A, Apex - A).GetSafeNormal();
		if (Normal.Z < 0.0)
		{
			Normal = -Normal;
		}
		for (const double Side : { 1.0, -1.0 })
		{
			const int32 First = RoofVertices.Num();
			RoofVertices.Append({ A, B, Apex });
			RoofNormals.Append({ Normal * Side, Normal * Side, Normal * Side });
			// Unreal shows a triangle from the side (C - A) ^ (B - A) points to.
			const bool bAsIs = FVector::CrossProduct(Apex - A, B - A).Dot(Normal * Side) > 0.0;
			RoofTriangles.Append(bAsIs ? TArray<int32>{ First, First + 1, First + 2 } : TArray<int32>{ First, First + 2, First + 1 });
		}
	}
}


AMarketPerson* AFleaMarket::SpawnPerson(const FVector& Feet, double Yaw, FRandomStream& Random, const FString& Keeper, bool bShopper)
{
	PersonKit::FOutfit Outfit = PersonModel::RandomOutfit(Random);
	if (!bShopper)
	{
		// Sellers in working clothes: aprons, caps, no briefcases.
		Outfit.Held = PersonKit::EHeld::None;
		Outfit.bApron = Random.FRand() < 0.4;
		if (!Outfit.bWoman && Random.FRand() < 0.6)
		{
			Outfit.Hat = PersonKit::EHat::FlatCap;
		}
	}
	else if (Random.FRand() < 0.3)
	{
		Outfit.Held = PersonKit::EHeld::Basket;
	}
	if (Keeper == TEXT("Pista b\u00e1csi"))
	{
		// An old junk dealer: flat cap, waistcoat, grey moustache.
		Outfit = PersonKit::FOutfit();
		Outfit.Hat = PersonKit::EHat::FlatCap;
		Outfit.HatColor = FLinearColor(0.22f, 0.20f, 0.17f);
		Outfit.Top = PersonKit::ETop::Waistcoat;
		Outfit.Waistcoat = FLinearColor(0.20f, 0.17f, 0.13f);
		Outfit.Trousers = FLinearColor(0.16f, 0.15f, 0.13f);
		Outfit.Shirt = FLinearColor(0.62f, 0.60f, 0.54f);
		Outfit.bTie = false;
		Outfit.bBoots = true;
		Outfit.bMoustache = true;
		Outfit.Hair = FLinearColor(0.55f, 0.55f, 0.52f);
		Outfit.Height = 170.f;
	}
	else if (Keeper == MarketBuild::CarpetSeller)
	{
		// The carpet seller from Baghdad, who dresses rather like a certain
		// hooded man from there: a long white robe with its hood up, a red
		// sash, soft boots.
		Outfit = PersonKit::FOutfit();
		Outfit.Hat = PersonKit::EHat::Hood;
		Outfit.Top = PersonKit::ETop::Overcoat;
		Outfit.Coat = FLinearColor(0.70f, 0.66f, 0.56f);
		Outfit.HatColor = Outfit.Coat;
		Outfit.bSash = true;
		Outfit.Sash = FLinearColor(0.45f, 0.06f, 0.05f);
		Outfit.Shirt = FLinearColor(0.55f, 0.50f, 0.42f);
		Outfit.Trousers = FLinearColor(0.20f, 0.15f, 0.10f);
		Outfit.Shoes = FLinearColor(0.16f, 0.10f, 0.06f);
		Outfit.bBoots = true;
		Outfit.bTie = false;
		Outfit.Skin = FLinearColor(0.58f, 0.40f, 0.28f);
		Outfit.Hair = FLinearColor(0.04f, 0.03f, 0.02f);
		Outfit.Height = 176.f;
	}
	else if (Keeper == TEXT("B\u00f6zsi n\u00e9ni"))
	{
		// The landlady at her clothes stall: dark dress, shawl, hair in grey.
		Outfit = PersonKit::FOutfit();
		Outfit.bWoman = true;
		Outfit.Height = 160.f;
		Outfit.Hat = PersonKit::EHat::None;
		Outfit.Top = PersonKit::ETop::Dress;
		Outfit.Coat = FLinearColor(0.10f, 0.09f, 0.10f);
		Outfit.bScarf = true;
		Outfit.Scarf = FLinearColor(0.35f, 0.22f, 0.30f);
		Outfit.bTie = false;
		Outfit.Hair = FLinearColor(0.45f, 0.44f, 0.42f);
		Outfit.Build = 1.15f;
	}

	const FTransform Spawn(FRotator(0.0, Yaw, 0.0), Feet + FVector(0.0, 0.0, PersonModel::MiddleHeight));
	AMarketPerson* Person = GetWorld()->SpawnActorDeferred<AMarketPerson>(AMarketPerson::StaticClass(), Spawn, this, nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Person)
	{
		return nullptr;
	}
	Person->SetOutfit(Outfit);
	Person->FinishSpawning(Spawn);
	// The market moves them; nor are they drawn from the other end of the district.
	TInlineComponentArray<UPrimitiveComponent*> Parts(Person);
	for (UPrimitiveComponent* Part : Parts)
	{
		Part->SetCachedMaxDrawDistance(MarketBuild::PeopleDrawDistance);
	}
	People.Add(Person);
	if (!Keeper.IsEmpty())
	{
		Keepers.Add(Keeper, Person);
	}

	FCrowd& Who = Crowd.AddDefaulted_GetRef();
	Who.bSeller = !bShopper;
	Who.bKeeper = !Keeper.IsEmpty();
	Who.Home = FVector2D(Feet);
	Who.HomeYaw = static_cast<float>(Yaw);
	Who.At = Who.Home;
	Who.Yaw = Who.HomeYaw;
	Who.Speed = CrowdRandom.FRandRange(MarketLife::MinWalk, MarketLife::MaxWalk);
	Who.Gesture = CrowdRandom.RandRange(0, MarketLife::Gestures - 1);
	Who.GestureLeft = CrowdRandom.FRandRange(MarketLife::MinGesture, MarketLife::MaxGesture);
	Who.Phase = CrowdRandom.FRandRange(0.f, 10.f);
	Who.Line = CrowdRandom.RandRange(0, 5);
	Who.NextCall = CrowdRandom.FRandRange(0.f, MarketLife::MaxCallPause);
	StartAct(Who, EAct::Stand, bShopper ? CrowdRandom.FRandRange(1.f, MarketLife::MaxLook) : 0.f);
	return Person;
}

void AFleaMarket::StartAct(FCrowd& Who, EAct Act, float Seconds, const FVector2D& Shove)
{
	Who.Act = Act;
	Who.Left = Seconds;
	Who.Time = 0.f;
	Who.Shove = Shove;
	Who.NoticeAt = -1.f;
	Who.bFrozen = false;
}

void AFleaMarket::Shout(const TCHAR* Who, const TCHAR* Hu, const TCHAR* En, bool bOverOthers)
{
	const double Now = GetWorld()->GetTimeSeconds();
	APestiHUD* Hud = APestiHUD::Get(this);
	if (!Hud || Hud->IsSaying() || (!bOverOthers && Now - LastShout < MarketLife::ShoutEvery))
	{
		return;
	}
	LastShout = Now;
	Hud->Say(Who, Hu, En, 3.f);
}

bool AFleaMarket::ClearWay(const FVector2D& From, const FVector2D& To) const
{
	// The stalls' boxes and the sheds' posts are in the way; people are not.
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(MarketWay), false);
	if (APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		Params.AddIgnoredActor(Player);
	}
	// As high as the table tops, which are what is solid about a stall.
	const double Z = GroundZ + 78.0;
	return !GetWorld()->LineTraceSingleByChannel(Hit, FVector(From, Z), FVector(To, Z), ECC_Visibility, Params);
}

FVector2D AFleaMarket::StallFront(const FStall& Stall, FRandomStream& Random, float& OutYaw) const
{
	const FVector2D Forward = MarketLife::Facing(Stall.Yaw);
	const FVector2D Right(-Forward.Y, Forward.X);
	OutYaw = static_cast<float>(Stall.Yaw) + 180.f;
	return FVector2D(Stall.Middle) + Forward * (Stall.Half.X + MarketLife::FrontGap)
		+ Right * Random.FRandRange(-0.5f, 0.5f) * FMath::Max(Stall.Half.Y, 60.0);
}

void AFleaMarket::NextWalk(FCrowd& Who)
{
	if (Who.bSeller)
	{
		Who.Target = Who.Home;
		Who.TargetYaw = Who.HomeYaw;
	}
	else
	{
		// Another stall near his own spot, one he can walk straight to; now
		// and then back to where he started.
		bool bFound = false;
		for (int32 Try = 0; Try < 6 && !bFound && StallList.Num() > 0; ++Try)
		{
			if (CrowdRandom.FRand() < 0.2f)
			{
				Who.Target = Who.Home;
				Who.TargetYaw = Who.HomeYaw;
			}
			else
			{
				const FStall& Stall = StallList[CrowdRandom.RandRange(0, StallList.Num() - 1)];
				if (Stall.bBroken || FVector2D::Distance(FVector2D(Stall.Middle), Who.Home) > MarketLife::BrowseRadius)
				{
					continue;
				}
				Who.Target = StallFront(Stall, CrowdRandom, Who.TargetYaw);
			}
			bFound = FVector2D::Distance(Who.Target, Who.At) > 40.0 && ClearWay(Who.At, Who.Target);
		}
		if (!bFound)
		{
			StartAct(Who, EAct::Stand, CrowdRandom.FRandRange(1.f, 3.f));
			return;
		}
	}
	if (FVector2D::Distance(Who.Target, Who.At) < 25.0)
	{
		Who.At = Who.Target;
		Who.Yaw = Who.TargetYaw;
		StartAct(Who, EAct::Stand, Who.bSeller ? 0.f : CrowdRandom.FRandRange(MarketLife::MinLook, MarketLife::MaxLook));
		return;
	}
	StartAct(Who, EAct::Walk, 0.f);
}

void AFleaMarket::Flee(FCrowd& Who, const FVector2D& From, float Seconds)
{
	FVector2D Away = (Who.At - From).GetSafeNormal();
	if (Away.IsNearlyZero())
	{
		Away = MarketLife::Facing(CrowdRandom.FRandRange(0.f, 360.f));
	}
	// Straight away if he can, else a little to one side or the other.
	const float Turns[] = { 0.f, 40.f, -40.f, 80.f, -80.f };
	FVector2D Way = Away;
	for (const float Turn : Turns)
	{
		const FVector2D Try = FVector2D(FVector(Away, 0.0).RotateAngleAxis(Turn, FVector::UpVector));
		if (ClearWay(Who.At, Who.At + Try * 250.0))
		{
			Way = Try;
			break;
		}
	}
	StartAct(Who, EAct::Flee, Seconds, Way * MarketLife::FleeSpeed);
}

void AFleaMarket::PutHome(FCrowd& Who)
{
	Who.At = Who.Home;
	Who.Yaw = Who.HomeYaw;
	StartAct(Who, EAct::Stand, Who.bSeller ? 0.f : CrowdRandom.FRandRange(MarketLife::MinLook, MarketLife::MaxLook));
	if (BloodPools.IsValidIndex(Who.Blood) && BloodPools[Who.Blood])
	{
		BloodPools[Who.Blood]->SetVisibility(false);
	}
	Who.Blood = INDEX_NONE;
}

void AFleaMarket::React(int32 Index, double Now)
{
	using namespace MarketLife;
	FCrowd& Who = Crowd[Index];
	if (!bPlayerAbout || Who.Act == EAct::Down || Who.Act == EAct::GetUp || Who.Act == EAct::Dead)
	{
		return;
	}
	const FVector2D Offset = Who.At - FVector2D(PlayerAt);
	if (Offset.SizeSquared() > FMath::Square(ReactRange) || FMath::Abs(GroundZ - PlayerAt.Z) > 400.0)
	{
		Who.NoticeAt = -1.f;
		return;
	}
	const double Speed = PlayerVelocity.Size2D();
	const FVector2D Going = FVector2D(PlayerVelocity).GetSafeNormal();
	const TCHAR* Name = Who.bSeller ? MarketLife::Seller : MarketLife::Shopper;

	if (bPlayerDriving)
	{
		const FVector2D Right(-PlayerForward.Y, PlayerForward.X);
		const double Ahead = FVector2D::DotProduct(Offset, PlayerForward);
		const double Aside = FVector2D::DotProduct(Offset, Right);
		if (Speed > HitSpeed && FMath::Abs(Ahead) < CarHalfLength && FMath::Abs(Aside) < CarHalfWidth)
		{
			if (Who.bKeeper)
			{
				// The story needs the stall keepers: they always get clear in time.
				const double Side = Aside >= 0.0 ? 1.0 : -1.0;
				StartAct(Who, EAct::Leap, 0.7f, Right * (Side * LeapSpeed * 1.3));
			}
			else
			{
				RunOver(Index, Speed);
			}
			return;
		}
		if (Who.Act == EAct::Leap || Who.Act == EAct::Flee)
		{
			return;
		}

		// The car coming at him (forwards, or backing up).
		const double Front = FVector2D::DotProduct(FVector2D(PlayerVelocity), PlayerForward) >= 0.0 ? Ahead : -Ahead;
		if (Speed > CarNoticeSpeed && Front > 0.0 && Front < Speed * CarWarnSeconds + CarWarnExtra + CarHalfLength
			&& FMath::Abs(Aside) < CarPathHalfWidth)
		{
			if (Who.NoticeAt < 0.f)
			{
				// He takes a moment to see it, and may stand frozen in fright.
				Who.NoticeAt = static_cast<float>(Now) + CrowdRandom.FRandRange(MinNotice, MaxNotice);
				Who.bFrozen = !Who.bKeeper && CrowdRandom.FRand() < FreezeChance;
			}
			if (Now >= Who.NoticeAt && !Who.bFrozen)
			{
				const double Side = FMath::Abs(Aside) > 10.0 ? FMath::Sign(Aside) : (CrowdRandom.FRand() < 0.5f ? -1.0 : 1.0);
				StartAct(Who, EAct::Leap, 0.7f, Right * (Side * LeapSpeed));
				const FLine2& Line = AnyOf(CarLines, CrowdRandom);
				Shout(Name, Line.Hu, Line.En);
			}
			return;
		}
		const float NoticeAt = Who.NoticeAt;
		Who.NoticeAt = -1.f;
		Who.bFrozen = false;

		// A car in the market close by: he runs off, arms up.
		if ((Who.Act == EAct::Stand || Who.Act == EAct::Walk) && Speed > CarNoticeSpeed * 0.5 && Offset.Size() < CarNear && NoticeAt < 0.f)
		{
			Flee(Who, FVector2D(PlayerAt), FleeSeconds);
			const FLine2& Line = AnyOf(CarLines, CrowdRandom);
			Shout(Name, Line.Hu, Line.En);
		}
		return;
	}

	// On foot: walked into, he stumbles away and tells you off.
	const double Distance = Offset.Size();
	if (Speed > BumpSpeed && Distance < BumpDistance
		&& (Who.Act == EAct::Stand || Who.Act == EAct::Walk || Who.Act == EAct::Aside))
	{
		const FVector2D Away = (Going * 0.7 + Offset.GetSafeNormal() * 0.3).GetSafeNormal();
		StartAct(Who, EAct::Stumble, 0.9f, Away * 220.0);
		const FLine2& Line = AnyOf(BumpLines, CrowdRandom);
		Shout(Name, Line.Hu, Line.En);
		return;
	}
	// Running at him: he steps out of the way.
	if ((Who.Act == EAct::Stand || Who.Act == EAct::Walk) && Speed > RunningSpeed && Distance < RunPastDistance
		&& FVector2D::DotProduct(Offset, Going) > 0.0)
	{
		const FVector2D Right(-Going.Y, Going.X);
		const double Side = FVector2D::DotProduct(Offset, Right) >= 0.0 ? 1.0 : -1.0;
		StartAct(Who, EAct::Aside, 0.8f, Right * (Side * 170.0));
	}
}

void AFleaMarket::RunOver(int32 Index, double Speed)
{
	using namespace MarketLife;
	FCrowd& Who = Crowd[Index];
	const bool bKilled = Speed > KillSpeed;
	const FVector2D Going = FVector2D(PlayerVelocity).GetSafeNormal();
	const FVector2D Right(-Going.Y, Going.X);
	const double Side = FVector2D::DotProduct(Who.At - FVector2D(PlayerAt), Right) >= 0.0 ? 1.0 : -1.0;
	// Thrown on ahead of the car and a little out to the side he was on.
	const FVector2D Thrown = (Going + Right * (Side * 0.5)).GetSafeNormal() * FMath::Min(Speed * 0.5, 700.0);
	StartAct(Who, bKilled ? EAct::Dead : EAct::Down, FallSeconds + LieSeconds, Thrown);
	// He falls on his back, away from the car: so he faces it.
	Who.Yaw = YawOf(-Going);

	if (APestiCar* Car = Cast<APestiCar>(LastPlayer.Get()))
	{
		Car->ForwardSpeed *= CarSlowOnHit;
	}

	// A police car or a constable close by sees it.
	if (APolicePatrols::Witnessed(this, PlayerAt, PoliceSee))
	{
		if (APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this))
		{
			Game->ReportCrime(bKilled ? 2 : 1);
		}
	}

	if (bKilled)
	{
		// Those who saw it cry out and run.
		Shout(Shopper, WitnessLine.Hu, WitnessLine.En, true);
		const FVector2D Body = Who.At;
		for (int32 i = 0; i < Crowd.Num(); ++i)
		{
			FCrowd& Other = Crowd[i];
			if (i != Index && (Other.Act == EAct::Stand || Other.Act == EAct::Walk || Other.Act == EAct::Aside)
				&& FVector2D::Distance(Other.At, Body) < WitnessNear && People.IsValidIndex(i) && People[i] && !People[i]->IsHidden())
			{
				Flee(Other, Body, FleeSeconds);
			}
		}
	}
}

void AFleaMarket::TickCrowd(float DeltaSeconds, const FVector& Camera, double Now)
{
	using namespace MarketLife;
	for (int32 i = 0; i < Crowd.Num() && i < People.Num(); ++i)
	{
		FCrowd& Who = Crowd[i];
		AMarketPerson* Person = People[i];
		// A keeper the story has taken away for now.
		if (!IsValid(Person) || Person->IsHidden())
		{
			continue;
		}
		const bool bCalm = Who.Act == EAct::Stand || Who.Act == EAct::Walk;
		const double Away = FVector2D::Distance(Who.At, FVector2D(Camera));
		const bool bSeen = Person->WasRecentlyRendered(SeenWithin)
			|| (BloodPools.IsValidIndex(Who.Blood) && BloodPools[Who.Blood] && BloodPools[Who.Blood]->WasRecentlyRendered(SeenWithin));

		// The run over are cleared away once out of sight, and anyone who ran
		// far off comes back to his place when nobody sees.
		if ((Who.Act == EAct::Dead && Away > ClearBeyond && !bSeen)
			|| (bCalm && Away > ActiveRange && !bSeen && FVector2D::Distance(Who.At, Who.Home) > 50.0))
		{
			PutHome(Who);
			Person->Pose(FVector(Who.At, GroundZ + Middle), Who.Yaw, 0.f, 0.f, 0.f);
			continue;
		}
		if (bCalm && Away > ActiveRange)
		{
			continue;
		}

		React(i, Now);
		Who.Time += DeltaSeconds;
		Who.Phase += DeltaSeconds;
		const double Standing = GroundZ + Middle;
		const double Lying = GroundZ + LyingHeight;

		switch (Who.Act)
		{
		case EAct::Stand:
		{
			if (!Who.bSeller)
			{
				Who.Left -= DeltaSeconds;
				if (Who.Left <= 0.f)
				{
					NextWalk(Who);
					break;
				}
			}
			Who.GestureLeft -= DeltaSeconds;
			if (Who.GestureLeft <= 0.f)
			{
				Who.Gesture = CrowdRandom.RandRange(0, Gestures - 1);
				Who.GestureLeft = CrowdRandom.FRandRange(MinGesture, MaxGesture);
			}
			// A seller calls out his wares to the player passing by.
			if (Who.bSeller && !Who.bKeeper && bPlayerAbout && !bPlayerDriving && Now >= Who.NextCall
				&& FVector2D::Distance(Who.At, FVector2D(PlayerAt)) < CallRange)
			{
				Who.NextCall = Now + CrowdRandom.FRandRange(MinCallPause, MaxCallPause);
				Who.Gesture = 1;
				Who.GestureLeft = 3.f;
				const int32 Goods = StallList.IsValidIndex(Who.Stall) ? static_cast<int32>(StallList[Who.Stall].Goods) : 0;
				const FLine2& Line = CrowdRandom.FRand() < 0.5f && Goods < static_cast<int32>(UE_ARRAY_COUNT(GoodsLines))
					? GoodsLines[Goods][0] : AnyOf(CallLines, CrowdRandom);
				Shout(Seller, Line.Hu, Line.En);
			}
			if (!bSeen && Who.Time > 0.1f)
			{
				break;
			}
			// Breathing, and now and then something to do with his hands.
			const float P = Who.Phase;
			float Lean = 1.f * FMath::Sin(P * 1.3f);
			float Left = 6.f + 3.f * FMath::Sin(P * 1.1f);
			float RightArm = 6.f + 3.f * FMath::Sin(P * 0.9f + 1.f);
			float LookAround = 0.f;
			switch (Who.Gesture)
			{
			case 1:
				if (Who.bSeller)
				{
					// Calling out, one arm up and waving.
					RightArm = 125.f + 20.f * FMath::Sin(P * 5.f);
				}
				else
				{
					// Reaching out to the goods.
					Lean += 10.f;
					RightArm = 45.f + 6.f * FMath::Sin(P * 2.f);
				}
				break;
			case 2:
				// Tidying the goods (a seller), or haggling with both hands.
				if (Who.bSeller)
				{
					Lean += 14.f;
					Left = 55.f + 10.f * FMath::Sin(P * 3.f);
					RightArm = 50.f - 10.f * FMath::Sin(P * 3.f);
				}
				else
				{
					Left = 40.f + 25.f * FMath::Sin(P * 4.f);
					RightArm = 40.f + 25.f * FMath::Sin(P * 4.f + 1.5f);
				}
				break;
			case 3:
				// Hands together: counting coins, or holding something up to look at.
				Left = Who.bSeller ? 35.f : 60.f;
				RightArm = Left + 3.f * FMath::Sin(P * 6.f);
				Lean += Who.bSeller ? 6.f : 3.f;
				break;
			case 4:
				// Looking about him.
				LookAround = 28.f * FMath::Sin(P * 0.7f);
				break;
			default:
				break;
			}
			Person->Pose(FVector(Who.At, Standing), Who.Yaw + LookAround, Lean, Left, RightArm, 0.f, 0.f, 0.f, 0.f, !Who.bKeeper);
			break;
		}
		case EAct::Walk:
		{
			const FVector2D Gap = Who.Target - Who.At;
			const double Step = Who.Speed * DeltaSeconds;
			if (Gap.Size() <= Step + 2.0)
			{
				Who.At = Who.Target;
				Who.Yaw = Who.TargetYaw;
				StartAct(Who, EAct::Stand, Who.bSeller ? 0.f : CrowdRandom.FRandRange(MinLook, MaxLook));
				Person->Pose(FVector(Who.At, Standing), Who.Yaw, 0.f, 6.f, 6.f);
				break;
			}
			Who.At += Gap.GetSafeNormal() * Step;
			Who.Yaw = FMath::FixedTurn(Who.Yaw, YawOf(Gap), TurnRate * DeltaSeconds);
			Person->WalkTo(FVector(Who.At, Standing), Who.Yaw, Who.Speed, DeltaSeconds);
			break;
		}
		case EAct::Aside:
		case EAct::Leap:
		{
			const bool bLeap = Who.Act == EAct::Leap;
			Who.At += Who.Shove * DeltaSeconds;
			if (bLeap)
			{
				// Turns to jump the way he goes.
				Who.Yaw = FMath::FixedTurn(Who.Yaw, YawOf(Who.Shove), 900.f * DeltaSeconds);
			}
			Person->WalkTo(FVector(Who.At, Standing), Who.Yaw, bLeap ? 400.f : 180.f, DeltaSeconds, bLeap ? 1.f : 0.f);
			Who.Left -= DeltaSeconds;
			if (Who.Left <= 0.f)
			{
				NextWalk(Who);
			}
			break;
		}
		case EAct::Flee:
		{
			// Every so often he looks where he is running, and swerves round a stall.
			if (FMath::Fmod(Who.Time, 0.4f) < DeltaSeconds && !ClearWay(Who.At, Who.At + Who.Shove.GetSafeNormal() * 150.0))
			{
				Who.Shove = FVector2D(FVector(Who.Shove, 0.0).RotateAngleAxis(CrowdRandom.FRand() < 0.5f ? 60.f : -60.f, FVector::UpVector));
			}
			Who.At += Who.Shove * DeltaSeconds;
			Who.Yaw = FMath::FixedTurn(Who.Yaw, YawOf(Who.Shove), 900.f * DeltaSeconds);
			Person->WalkTo(FVector(Who.At, Standing), Who.Yaw, static_cast<float>(FleeSpeed), DeltaSeconds, 1.f);
			Who.Left -= DeltaSeconds;
			if (Who.Left <= 0.f)
			{
				NextWalk(Who);
			}
			break;
		}
		case EAct::Stumble:
		{
			Who.Shove *= FMath::Max(0.0, 1.0 - 3.0 * DeltaSeconds);
			Who.At += Who.Shove * DeltaSeconds;
			// Pushed from behind he pitches forward, from in front he reels back.
			const bool bFromBehind = FVector2D::DotProduct(MarketLife::Facing(Who.Yaw), Who.Shove) > 0.0;
			const float Wave = FMath::Sin(PI * FMath::Clamp(Who.Time / 0.9f, 0.f, 1.f));
			const float Lean = (bFromBehind ? 18.f : -18.f) * Wave;
			Person->Pose(FVector(Who.At, Standing), Who.Yaw, Lean, 70.f * Wave, 60.f * Wave,
				Lean, Lean * 0.6f, -25.f * Wave, -20.f * Wave);
			Who.Left -= DeltaSeconds;
			if (Who.Left <= 0.f)
			{
				NextWalk(Who);
			}
			break;
		}
		case EAct::Angry:
		{
			// Faces the driver, shaking his fists.
			if (bPlayerAbout)
			{
				Who.Yaw = FMath::FixedTurn(Who.Yaw, YawOf(FVector2D(PlayerAt) - Who.At), 400.f * DeltaSeconds);
			}
			const float Shake = 20.f * FMath::Sin(Who.Time * 14.f);
			Person->Pose(FVector(Who.At, Standing), Who.Yaw, -6.f, 125.f + Shake, 125.f - Shake);
			Who.Left -= DeltaSeconds;
			if (Who.Left <= 0.f)
			{
				NextWalk(Who);
			}
			break;
		}
		case EAct::Down:
		case EAct::Dead:
		{
			const bool bDead = Who.Act == EAct::Dead;
			if (Who.Time < FallSeconds)
			{
				const float Fall = Who.Time / FallSeconds;
				Who.Shove *= FMath::Max(0.0, 1.0 - 2.0 * DeltaSeconds);
				Who.At += Who.Shove * DeltaSeconds;
				Person->Pose(FVector(Who.At, FMath::Lerp(Standing, Lying, static_cast<double>(Fall))), Who.Yaw,
					-88.f * Fall * Fall, 150.f * Fall, 130.f * Fall, 40.f * Fall, 24.f * Fall, -30.f * Fall, -24.f * Fall);
				break;
			}
			Who.Shove = FVector2D::ZeroVector;
			const FVector Down(Who.At, Lying);
			if (bDead)
			{
				// Still, the blood spreading under him.
				Person->Pose(Down, Who.Yaw, -88.f, 120.f, 100.f, 25.f, 15.f, -55.f, -44.f);
				if (!BloodPools.IsValidIndex(Who.Blood) && Shapes.IsValidIndex(1) && Shapes[1])
				{
					// A free pool, or a new one.
					int32 Free = INDEX_NONE;
					for (int32 b = 0; b < BloodPools.Num() && Free == INDEX_NONE; ++b)
					{
						bool bUsed = false;
						for (const FCrowd& Other : Crowd)
						{
							bUsed = bUsed || Other.Blood == b;
						}
						Free = bUsed ? INDEX_NONE : b;
					}
					if (Free == INDEX_NONE)
					{
						UStaticMeshComponent* Pool = NewObject<UStaticMeshComponent>(this, NAME_None, RF_Transient);
						Pool->SetupAttachment(RootComponent);
						Pool->SetStaticMesh(Shapes[1]);
						Pool->SetMaterial(0, BloodPaint);
						Pool->SetCollisionEnabled(ECollisionEnabled::NoCollision);
						Pool->SetCastShadow(false);
						Pool->SetCanEverAffectNavigation(false);
						Pool->SetCachedMaxDrawDistance(MarketBuild::PeopleDrawDistance);
						Pool->RegisterComponent();
						Free = BloodPools.Add(Pool);
					}
					Who.Blood = Free;
				}
				if (BloodPools.IsValidIndex(Who.Blood) && BloodPools[Who.Blood])
				{
					UStaticMeshComponent* Pool = BloodPools[Who.Blood];
					const float Spread = FMath::Sqrt(FMath::Clamp((Who.Time - FallSeconds) / BloodSpreads, 0.05f, 1.f));
					const double Radius = BloodRadius * Spread;
					// Under his chest and head, which lie behind his middle.
					const FVector2D Behind = -MarketLife::Facing(Who.Yaw) * 35.0;
					Pool->SetVisibility(true);
					Pool->SetWorldLocationAndRotation(FVector(Who.At + Behind, GroundZ + 3.6), FRotator(0.0, Who.Yaw, 0.0));
					Pool->SetWorldScale3D(FVector(Radius / 50.0 * 1.25, Radius / 50.0, 0.004));
				}
				break;
			}
			Person->Pose(Down, Who.Yaw, -88.f, 150.f, 130.f, 20.f, 12.f, -30.f, -24.f);
			Who.Left -= DeltaSeconds;
			if (Who.Left <= 0.f)
			{
				StartAct(Who, EAct::GetUp, GetUpSeconds);
				const FLine2& Line = AnyOf(GetUpLines, CrowdRandom);
				Shout(Who.bSeller ? Seller : Shopper, Line.Hu, Line.En);
			}
			break;
		}
		case EAct::GetUp:
		{
			// Sits up, crouches, and stands.
			const float Up = FMath::SmoothStep(0.f, 1.f, FMath::Clamp(Who.Time / GetUpSeconds, 0.f, 1.f));
			const float Bent = 1.f - Up;
			Person->Pose(FVector(Who.At, FMath::Lerp(Lying, Standing, static_cast<double>(Up))), Who.Yaw,
				-88.f * Bent * Bent, 45.f * Bent, 40.f * Bent, 75.f * Bent, 45.f * Bent, -110.f * Bent, -88.f * Bent);
			if (Who.Time >= GetUpSeconds)
			{
				NextWalk(Who);
			}
			break;
		}
		default:
			break;
		}
	}
}

bool AFleaMarket::TickTalk(double Now)
{
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	APestiPerson* Player = PC ? Cast<APestiPerson>(PC->GetPawn()) : nullptr;
	if (!Player || Player->IsCarrying() || Player->IsRiding() || Player->IsHidden())
	{
		return false;
	}
	// The nearest one in reach. The stall keepers have their own business
	// with the player (a mission, a tip), so nobody chats right by them.
	const FVector2D Here(Player->GetActorLocation());
	for (const TPair<FString, TObjectPtr<AStoryPerson>>& Keeper : Keepers)
	{
		if (Keeper.Value && !Keeper.Value->IsHidden() && FVector2D::Distance(FVector2D(Keeper.Value->GetActorLocation()), Here) < 400.0)
		{
			return false;
		}
	}
	int32 Nearest = INDEX_NONE;
	double NearestDistance = MarketBuild::TalkRadius;
	for (int32 i = 0; i < Crowd.Num() && i < People.Num(); ++i)
	{
		const FCrowd& Who = Crowd[i];
		const double Distance = FVector2D::Distance(Who.At, Here);
		if (!Who.bKeeper && (Who.Act == EAct::Stand || Who.Act == EAct::Walk) && Distance < NearestDistance
			&& IsValid(People[i]) && !People[i]->IsHidden())
		{
			Nearest = i;
			NearestDistance = Distance;
		}
	}
	if (Nearest == INDEX_NONE)
	{
		return false;
	}
	APestiHUD::Show(this, MarketBuild::HintLine, 0.f, FColor(255, 210, 60), TEXT("E: besz\u00e9lget\u00e9s  (talk)"));
	if (!PC->WasInputKeyJustPressed(EKeys::E))
	{
		return true;
	}

	// He stops, turns to the player and has his say.
	FCrowd& Who = Crowd[Nearest];
	Who.Yaw = MarketLife::YawOf(Here - Who.At);
	StartAct(Who, EAct::Stand, Who.bSeller ? 0.f : CrowdRandom.FRandRange(5.f, 8.f));
	Who.Gesture = 0;
	Who.GestureLeft = 4.f;
	Who.NextCall = Now + MarketLife::MaxCallPause;
	const int32 Goods = StallList.IsValidIndex(Who.Stall) ? static_cast<int32>(StallList[Who.Stall].Goods) : 0;
	const bool bGoods = Who.bSeller && Goods < static_cast<int32>(UE_ARRAY_COUNT(MarketLife::GoodsLines));
	const int32 Count = static_cast<int32>(UE_ARRAY_COUNT(MarketLife::ShopperLines));
	const MarketLife::FLine2& Line = bGoods ? MarketLife::GoodsLines[Goods][Who.Line % 2] : MarketLife::ShopperLines[Who.Line % Count];
	++Who.Line;
	if (APestiHUD* Hud = APestiHUD::Get(this))
	{
		Hud->Say(Who.bSeller ? MarketLife::Seller : MarketLife::Shopper, Line.Hu, Line.En, 5.f);
	}
	return true;
}

void AFleaMarket::TickStalls(const FVector& Camera, float DeltaSeconds, double Now)
{
	using namespace MarketLife;

	// A car driven fast into a stall smashes it, just before it touches it.
	const double Speed = PlayerVelocity.Size2D();
	if (bPlayerAbout && bPlayerDriving && Speed > SmashSpeed)
	{
		const FVector2D Going = FVector2D(PlayerVelocity).GetSafeNormal();
		const FVector2D Car(PlayerAt);
		const FVector2D Points[] = { Car, Car + Going * (CarHalfLength * 0.9) };
		const double Margin = CarHalfWidth * 0.6 + Speed * DeltaSeconds * 2.0 + 20.0;
		for (int32 i = 0; i < StallList.Num(); ++i)
		{
			const FStall& Stall = StallList[i];
			if (Stall.bBroken || FVector2D::DistSquared(FVector2D(Stall.Middle), Car) > FMath::Square(SmashReach))
			{
				continue;
			}
			const FVector2D Forward = Facing(Stall.Yaw);
			const FVector2D Right(-Forward.Y, Forward.X);
			for (const FVector2D& Point : Points)
			{
				const FVector2D Offset = Point - FVector2D(Stall.Middle);
				if (FMath::Abs(FVector2D::DotProduct(Offset, Forward)) < Stall.Half.X + Margin
					&& FMath::Abs(FVector2D::DotProduct(Offset, Right)) < Stall.Half.Y + Margin)
				{
					Smash(i, Now);
					// Wrecking a stall in front of the police is a crime too.
					if (APolicePatrols::Witnessed(this, FVector(Stall.Middle), PoliceSee))
					{
						if (APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this))
						{
							Game->ReportCrime(1);
						}
					}
					break;
				}
			}
		}
	}

	// The pieces of a stall come to rest; a few stalls a frame are checked
	// to stand again, once far enough away and out of sight.
	for (FStall& Stall : StallList)
	{
		if (Stall.bBroken && !Stall.bSettled && Now - Stall.BrokenAt > SettleAfter)
		{
			Stall.bSettled = true;
			for (UStaticMeshComponent* Piece : Stall.Pieces)
			{
				Wreck::Settle(Piece);
			}
		}
	}
	for (int32 n = 0; n < ResetChecksPerFrame && StallList.Num() > 0; ++n)
	{
		NextReset = (NextReset + 1) % StallList.Num();
		FStall& Stall = StallList[NextReset];
		if (!Stall.bBroken || Now - Stall.BrokenAt < ResetAfter)
		{
			continue;
		}
		bool bSeen = FVector2D::Distance(FVector2D(Stall.Middle), FVector2D(Camera)) < ResetDistance;
		for (int32 p = 0; p < Stall.Pieces.Num() && !bSeen; ++p)
		{
			bSeen = IsValid(Stall.Pieces[p]) && Stall.Pieces[p]->WasRecentlyRendered(SeenWithin);
		}
		if (!bSeen)
		{
			PutBack(Stall);
		}
	}
}

void AFleaMarket::Smash(int32 Index, double Now)
{
	using namespace MarketLife;
	FStall& Stall = StallList[Index];

	// Too much wreckage about already: the oldest that nobody sees is cleared first.
	int32 Flying = 0;
	for (const FPart& Part : Stall.Parts)
	{
		Flying += Part.bStays ? 0 : 1;
	}
	while (Wreckage.Num() + Flying > MaxWreckage)
	{
		int32 Oldest = INDEX_NONE;
		for (int32 i = 0; i < StallList.Num(); ++i)
		{
			const FStall& Other = StallList[i];
			bool bSeen = false;
			for (int32 p = 0; p < Other.Pieces.Num() && !bSeen; ++p)
			{
				bSeen = IsValid(Other.Pieces[p]) && Other.Pieces[p]->WasRecentlyRendered(SeenWithin);
			}
			if (Other.bBroken && !bSeen && (Oldest == INDEX_NONE || Other.BrokenAt < StallList[Oldest].BrokenAt))
			{
				Oldest = i;
			}
		}
		if (Oldest == INDEX_NONE)
		{
			return;
		}
		PutBack(StallList[Oldest]);
	}

	Stall.bBroken = true;
	Stall.bSettled = false;
	Stall.BrokenAt = Now;
	if (Blockers.IsValidIndex(Stall.Blocker) && Blockers[Stall.Blocker])
	{
		Blockers[Stall.Blocker]->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// Each piece of it flies off with the car, some way to either side and
	// up into the air, tumbling; the faster the car, the further.
	TSet<int32> Touched;
	for (const FPart& Part : Stall.Parts)
	{
		if (Part.bStays || !Batches.IsValidIndex(Part.Batch) || !Batches[Part.Batch] || !Shapes.IsValidIndex(Part.Shape))
		{
			continue;
		}
		UInstancedStaticMeshComponent* Batch = Batches[Part.Batch];
		FTransform Gone = Part.Transform;
		Gone.SetScale3D(FVector(0.001));
		Batch->UpdateInstanceTransform(Part.Instance, Gone, true, false, true);
		Touched.Add(Part.Batch);

		FVector Velocity;
		FVector Spin;
		Wreck::Fling(PlayerVelocity, CrowdRandom, Velocity, Spin);
		UStaticMeshComponent* Piece = Wreck::Throw(this, Shapes[Part.Shape], Batch->GetMaterial(0), Part.Transform, Velocity, Spin,
			MarketBuild::GoodsDrawDistance);
		Stall.Pieces.Add(Piece);
		Wreckage.Add(Piece);
	}
	for (const int32 Batch : Touched)
	{
		Batches[Batch]->MarkRenderStateDirty();
	}

	if (APestiCar* Car = Cast<APestiCar>(LastPlayer.Get()))
	{
		Car->ForwardSpeed *= CarSlowOnSmash;
	}
	// A lighter lying on the table falls to the ground, still there to be found.
	ALighters::DropNear(GetWorld(), FVector2D(Stall.Middle), Stall.Half.GetMax() + 30.0, GroundZ);

	// The seller curses the driver; the shoppers close by run.
	const FVector2D Where(Stall.Middle);
	if (Crowd.IsValidIndex(Stall.Seller))
	{
		FCrowd& StallSeller = Crowd[Stall.Seller];
		if (StallSeller.Act == EAct::Stand || StallSeller.Act == EAct::Walk || StallSeller.Act == EAct::Aside)
		{
			StartAct(StallSeller, EAct::Angry, AngrySeconds);
		}
		const FLine2& Line = AnyOf(AngryLines, CrowdRandom);
		Shout(MarketLife::Seller, Line.Hu, Line.En, true);
	}
	for (int32 i = 0; i < Crowd.Num(); ++i)
	{
		FCrowd& Other = Crowd[i];
		if (i != Stall.Seller && !Other.bSeller && (Other.Act == EAct::Stand || Other.Act == EAct::Walk)
			&& FVector2D::Distance(Other.At, Where) < ScareNear)
		{
			Flee(Other, Where, FleeSeconds);
		}
	}
}

void AFleaMarket::PutBack(FStall& Stall)
{
	for (UStaticMeshComponent* Piece : Stall.Pieces)
	{
		if (IsValid(Piece))
		{
			Wreckage.RemoveSwap(Piece);
			Piece->DestroyComponent();
		}
	}
	Stall.Pieces.Reset();
	TSet<int32> Touched;
	for (const FPart& Part : Stall.Parts)
	{
		if (!Part.bStays && Batches.IsValidIndex(Part.Batch) && Batches[Part.Batch])
		{
			Batches[Part.Batch]->UpdateInstanceTransform(Part.Instance, Part.Transform, true, false, true);
			Touched.Add(Part.Batch);
		}
	}
	for (const int32 Batch : Touched)
	{
		Batches[Batch]->MarkRenderStateDirty();
	}
	if (Blockers.IsValidIndex(Stall.Blocker) && Blockers[Stall.Blocker])
	{
		Blockers[Stall.Blocker]->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}
	Stall.bBroken = false;
	Stall.bSettled = false;
}
