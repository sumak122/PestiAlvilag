// Fill out your copyright notice in the Description page of Project Settings.

#include "FleaMarket.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "MarketModel.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PersonModel.h"
#include "PestiPerson.h"
#include "ProceduralMeshComponent.h"
#include "StoryPerson.h"
#include "UObject/ConstructorHelpers.h"
#include "PestiHud.h"

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

AFleaMarket::AFleaMarket()
{
	// Ticks only to let the player talk to the carpet seller.
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
	for (AStoryPerson* Person : People)
	{
		if (Person)
		{
			Person->Destroy();
		}
	}
	People.Reset();
	Keepers.Reset();
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

	const TObjectPtr<AStoryPerson>* Seller = Keepers.Find(FString(MarketBuild::CarpetSeller));
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	APestiPerson* Player = PC ? Cast<APestiPerson>(PC->GetPawn()) : nullptr;
	if (!Seller || !*Seller || !Player || Player->IsCarrying() || Player->IsRiding() || Player->IsHidden()
		|| FVector::Dist2D(Player->GetActorLocation(), (*Seller)->GetActorLocation()) > MarketBuild::TalkRadius)
	{
		return;
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
}

void AFleaMarket::Add(int32 Shape, const FLinearColor& Color, const FVector& At, const FVector& Size, const FRotator& Turn, bool bCollide)
{
	if (!Shapes.IsValidIndex(Shape) || !Shapes[Shape])
	{
		return;
	}
	const FString Key = FString::Printf(TEXT("%d %s %d"), Shape, *Color.ToString(), bCollide ? 1 : 0);
	UInstancedStaticMeshComponent* Instances = nullptr;
	if (const int32* Found = BatchIndex.Find(Key))
	{
		Instances = Batches[*Found];
	}
	else
	{
		Instances = NewObject<UInstancedStaticMeshComponent>(this, NAME_None, RF_Transient);
		Instances->SetupAttachment(RootComponent);
		Instances->SetStaticMesh(Shapes[Shape]);
		if (BaseMaterial)
		{
			UMaterialInstanceDynamic* Paint = UMaterialInstanceDynamic::Create(BaseMaterial, this);
			Paint->SetVectorParameterValue(TEXT("Color"), Color);
			Instances->SetMaterial(0, Paint);
		}
		Instances->SetCollisionProfileName(bCollide ? TEXT("BlockAll") : TEXT("NoCollision"));
		Instances->SetCullDistances(0, MarketBuild::GoodsDrawDistance);
		Instances->SetCanEverAffectNavigation(false);
		Instances->RegisterComponent();
		BatchIndex.Add(Key, Batches.Add(Instances));
	}
	Instances->AddInstance(FTransform(Turn, At, Size / 100.0), true);
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

	TArray<MarketModel::FShape> Pieces;
	const TArray<TSharedPtr<FJsonValue>>* Sheds = nullptr;
	if (Market.TryGetArrayField(TEXT("sheds"), Sheds))
	{
		for (const TSharedPtr<FJsonValue>& Value : *Sheds)
		{
			const TArray<TSharedPtr<FJsonValue>>& S = Value->AsArray();
			if (S.Num() >= 5)
			{
				const FVector Middle(S[0]->AsNumber(), S[1]->AsNumber(), GroundZ);
				MarketModel::BuildShed(Pieces, Middle, S[2]->AsNumber(), S[3]->AsNumber(), S[4]->AsNumber());
				BuildShedRoof(Middle, S[2]->AsNumber(), S[3]->AsNumber(), S[4]->AsNumber());
			}
		}
	}

	Roofs->CreateMeshSection_LinearColor(0, RoofVertices, RoofTriangles, RoofNormals,
		TArray<FVector2D>(), TArray<FLinearColor>(), TArray<FProcMeshTangent>(), false);
	if (BaseMaterial)
	{
		UMaterialInstanceDynamic* Paint = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		Paint->SetVectorParameterValue(TEXT("Color"), MarketModel::ShedRoof());
		Roofs->SetMaterial(0, Paint);
	}
	RoofVertices.Empty();
	RoofTriangles.Empty();
	RoofNormals.Empty();

	struct FSeller
	{
		FVector Feet;
		double Yaw;
		FString Keeper;
	};
	TArray<FSeller> Sellers;
	if (Stalls)
	{
		for (const TSharedPtr<FJsonValue>& Value : *Stalls)
		{
			const TSharedPtr<FJsonObject> Stall = Value->AsObject();
			const TArray<TSharedPtr<FJsonValue>>* At = nullptr;
			if (!Stall || !Stall->TryGetArrayField(TEXT("at"), At) || At->Num() < 3)
			{
				continue;
			}
			const FVector Middle((*At)[0]->AsNumber(), (*At)[1]->AsNumber(), GroundZ);
			const double Yaw = (*At)[2]->AsNumber();
			MarketModel::FRandom Random(GetTypeHash(FIntPoint(FMath::RoundToInt(Middle.X), FMath::RoundToInt(Middle.Y))));
			const double Back = MarketModel::BuildStall(Pieces, Middle, Yaw,
				MarketBuild::BuildOf(Stall->GetStringField(TEXT("build"))), MarketBuild::GoodsOf(Stall->GetStringField(TEXT("goods"))), Random);

			FString Keeper;
			Stall->TryGetStringField(TEXT("keeper"), Keeper);
			if (Stall->GetBoolField(TEXT("seller")) || !Keeper.IsEmpty())
			{
				const double Along = Random.Range(-60.0, 60.0);
				const double A = FMath::DegreesToRadians(Yaw);
				const FVector Feet = Middle + FVector(-Back * FMath::Cos(A) - Along * FMath::Sin(A), -Back * FMath::Sin(A) + Along * FMath::Cos(A), 0.0);
				Sellers.Add({ Feet, Yaw, Keeper });
			}
		}
	}
	for (const MarketModel::FShape& Piece : Pieces)
	{
		Add(Piece.Shape, Piece.Color, Piece.At, Piece.Size, FRotator(Piece.Pitch, Piece.Yaw, Piece.Roll), Piece.bCollide);
	}

	FRandomStream Random(1932);
	for (const FSeller& Seller : Sellers)
	{
		SpawnPerson(Seller.Feet, Seller.Yaw, Random, Seller.Keeper);
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

AStoryPerson* AFleaMarket::SpawnPerson(const FVector& Feet, double Yaw, FRandomStream& Random, const FString& Keeper, bool bShopper)
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
	AStoryPerson* Person = GetWorld()->SpawnActorDeferred<AStoryPerson>(AStoryPerson::StaticClass(), Spawn, this, nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Person)
	{
		return nullptr;
	}
	Person->SetOutfit(Outfit);
	Person->FinishSpawning(Spawn);
	// They stand at their stalls: no need to think every frame, nor to be
	// drawn from the other end of the district.
	Person->SetActorTickEnabled(false);
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
	return Person;
}
