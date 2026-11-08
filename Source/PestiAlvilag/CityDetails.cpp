// Fill out your copyright notice in the Description page of Project Settings.

#include "CityDetails.h"

#include "CityDetailsModel.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "DayNight.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "PestiHud.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "ShapeSet.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogCityDetails, Log, All);

namespace CityDetailsPlace
{
	const TCHAR* const DataFile = TEXT("Data/OsmGraybox/city_details.json");
	// A gate counts as open when its foot is this near an open courtyard's gate (cm).
	constexpr double OpenGateNear = 100.0;
	// How far things are drawn (cm): the big ones from far off.
	constexpr float BigDrawDistance = 60000.f;
	constexpr float YardDrawDistance = 15000.f;
	constexpr float SmallDrawDistance = 12000.f;
	// How often the clocks' hands are looked at (s), and how far the time
	// must have moved before they are put again (hours: half a minute).
	constexpr float HandsCheckEvery = 1.f;
	constexpr float HandsStep = 1.f / 120.f;
	// The ground height is taken this far out in front of a wall (cm).
	constexpr double OutFromWall = 120.0;

	FVector2D Pair(const TArray<TSharedPtr<FJsonValue>>& Values, int32 First = 0)
	{
		return Values.Num() > First + 1 ? FVector2D(Values[First]->AsNumber(), Values[First + 1]->AsNumber()) : FVector2D::ZeroVector;
	}

	double Number(const TArray<TSharedPtr<FJsonValue>>& Values, int32 Index)
	{
		return Values.IsValidIndex(Index) ? Values[Index]->AsNumber() : 0.0;
	}

	FVector2D Ahead(double Yaw, double Distance)
	{
		const double R = FMath::DegreesToRadians(Yaw);
		return FVector2D(FMath::Cos(R), FMath::Sin(R)) * Distance;
	}
}

ACityDetails::ACityDetails()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicShapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	BaseMaterial = BasicShapeMaterial.Object;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	CubeMesh = Cube.Object;
}

TSharedPtr<FJsonObject> ACityDetails::LoadDetails()
{
	const FString Path = FPaths::Combine(FPaths::ProjectDir(), CityDetailsPlace::DataFile);
	FString Text;
	TSharedPtr<FJsonObject> Root;
	if (!FFileHelper::LoadFileToString(Text, *Path)
		|| !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) || !Root.IsValid())
	{
		UE_LOG(LogCityDetails, Warning, TEXT("Could not read %s"), *Path);
		return nullptr;
	}
	return Root;
}

bool ACityDetails::IsOpenGate(const FVector2D& Foot)
{
	// Read once: the gates' feet of the open courtyards.
	static TArray<FVector2D> OpenGates;
	static bool bRead = false;
	if (!bRead)
	{
		bRead = true;
		const TArray<TSharedPtr<FJsonValue>>* Yards = nullptr;
		if (const TSharedPtr<FJsonObject> Root = LoadDetails())
		{
			if (Root->TryGetArrayField(TEXT("courtyards"), Yards))
			{
				for (const TSharedPtr<FJsonValue>& YardValue : *Yards)
				{
					const TSharedPtr<FJsonObject> Yard = YardValue->AsObject();
					const TArray<TSharedPtr<FJsonValue>>* Gate = nullptr;
					if (Yard.IsValid() && Yard->TryGetArrayField(TEXT("gate"), Gate))
					{
						OpenGates.Add(CityDetailsPlace::Pair(*Gate));
					}
				}
			}
		}
	}
	for (const FVector2D& Gate : OpenGates)
	{
		if (FVector2D::Distance(Gate, Foot) < CityDetailsPlace::OpenGateNear)
		{
			return true;
		}
	}
	return false;
}

double ACityDetails::GroundAt(const FVector2D& At) const
{
	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	const FVector Top(At, 10000.0);
	const FVector Bottom(At, -10000.0);
	return GetWorld()->LineTraceSingleByChannel(Hit, Top, Bottom, ECC_Visibility, Params) ? Hit.ImpactPoint.Z : 0.0;
}

AShapeSet* ACityDetails::PutUp(const FVector& At, double SetYaw, float DrawDistance) const
{
	AShapeSet* Set = GetWorld()->SpawnActor<AShapeSet>(AShapeSet::StaticClass(), At, FRotator(0.0, SetYaw, 0.0));
	if (Set)
	{
		Set->SetDrawDistance(DrawDistance);
	}
	return Set;
}

void ACityDetails::BeginPlay()
{
	Super::BeginPlay();

	Hands = NewObject<UInstancedStaticMeshComponent>(this, TEXT("ClockHands"));
	Hands->SetupAttachment(RootComponent);
	Hands->SetUsingAbsoluteLocation(true);
	Hands->SetUsingAbsoluteRotation(true);
	Hands->SetUsingAbsoluteScale(true);
	Hands->SetWorldTransform(FTransform::Identity);
	Hands->SetStaticMesh(CubeMesh);
	if (BaseMaterial)
	{
		UMaterialInstanceDynamic* Paint = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		Paint->SetVectorParameterValue(TEXT("Color"), CityDetailsModel::Colors::Hand);
		Hands->SetMaterial(0, Paint);
	}
	Hands->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Hands->SetCanEverAffectNavigation(false);
	Hands->SetCullDistances(0, CityDetailsPlace::SmallDrawDistance);
	// The hands move now and then; moving meshes stay out of ray tracing (it slows the game right down).
	Hands->bVisibleInRayTracing = false;
	Hands->RegisterComponent();

	BuildCorvin();
	if (const TSharedPtr<FJsonObject> Root = LoadDetails())
	{
		BuildCourtyards(*Root);
		BuildAdverts(*Root);
		BuildKiosks(*Root);
		BuildClocks(*Root);
		ReadMapIcons(*Root);
	}
}

void ACityDetails::BuildCorvin()
{
	using namespace CityDetailsModel;
	const FVector2D Pavement = CorvinAt + CityDetailsPlace::Ahead(CorvinYaw, 300.0);
	AShapeSet* Set = PutUp(FVector(CorvinAt, GroundAt(Pavement)), CorvinYaw, CityDetailsPlace::BigDrawDistance);
	if (!Set)
	{
		return;
	}
	TArray<FShape> Shapes;
	TArray<FLamp> Lights;
	TArray<FSign> Lettering;
	Corvin(Shapes, Lights, Lettering);
	Set->AddShapes(Shapes);
	for (const FLamp& Light : Lights)
	{
		Set->AddLamp(Light.At, Light.Radius);
	}
	for (const FSign& Sign : Lettering)
	{
		Set->AddSign(Sign.Text, Sign.At, Sign.Yaw, Sign.Height, Sign.Color);
	}
}

void ACityDetails::BuildCourtyards(const FJsonObject& Root)
{
	using namespace CityDetailsModel;
	const TArray<TSharedPtr<FJsonValue>>* Yards = nullptr;
	if (!Root.TryGetArrayField(TEXT("courtyards"), Yards))
	{
		return;
	}
	for (const TSharedPtr<FJsonValue>& YardValue : *Yards)
	{
		const TSharedPtr<FJsonObject> Yard = YardValue->AsObject();
		const TArray<TSharedPtr<FJsonValue>>* Gate = nullptr;
		if (!Yard.IsValid() || !Yard->TryGetArrayField(TEXT("gate"), Gate))
		{
			continue;
		}
		const FVector2D Foot = CityDetailsPlace::Pair(*Gate);
		const double GateYaw = CityDetailsPlace::Number(*Gate, 2);
		const double Width = Yard->GetNumberField(TEXT("width"));
		const double Depth = Yard->GetNumberField(TEXT("depth"));
		const uint32 Seed = static_cast<uint32>(Yard->GetNumberField(TEXT("seed")));
		const double Ground = GroundAt(Foot + CityDetailsPlace::Ahead(GateYaw, CityDetailsPlace::OutFromWall));

		// The gateway, in the gate's frame.
		if (AShapeSet* Way = PutUp(FVector(Foot, Ground), GateYaw, CityDetailsPlace::YardDrawDistance))
		{
			TArray<FShape> Shapes;
			TArray<FLamp> Lights;
			Gateway(Width, Depth, Shapes, Lights);
			Way->AddShapes(Shapes);
			for (const FLamp& Light : Lights)
			{
				Way->AddLamp(Light.At, Light.Radius);
			}
		}

		// The courtyard, in a frame not turned, round the gate's foot.
		AShapeSet* Court = PutUp(FVector(Foot, Ground), 0.0, CityDetailsPlace::YardDrawDistance);
		if (!Court)
		{
			continue;
		}
		const FVector2D ExitAt = -CityDetailsPlace::Ahead(GateYaw, Depth);
		const double ExitHalf = Width * 0.5 - 34.0;
		TArray<FShape> Shapes;
		const TArray<TSharedPtr<FJsonValue>>* Walls = nullptr;
		if (Yard->TryGetArrayField(TEXT("walls"), Walls))
		{
			int32 Index = 0;
			for (const TSharedPtr<FJsonValue>& WallValue : *Walls)
			{
				const TArray<TSharedPtr<FJsonValue>>& W = WallValue->AsArray();
				if (W.Num() < 6)
				{
					continue;
				}
				FYardWall Wall;
				Wall.P = CityDetailsPlace::Pair(W, 0) - Foot;
				Wall.Q = CityDetailsPlace::Pair(W, 2) - Foot;
				Wall.Height = W[4]->AsNumber();
				Wall.Inward = W[5]->AsNumber();
				YardWall(Wall, ExitAt, ExitHalf, Seed + static_cast<uint32>(Index++) * 7919u, Shapes);
			}
		}
		const TArray<TSharedPtr<FJsonValue>>* Things = nullptr;
		if (Yard->TryGetArrayField(TEXT("props"), Things))
		{
			for (const TSharedPtr<FJsonValue>& ThingValue : *Things)
			{
				const TArray<TSharedPtr<FJsonValue>>& T = ThingValue->AsArray();
				if (T.Num() < 4)
				{
					continue;
				}
				FYardThing Thing;
				Thing.Kind = T[0]->AsString();
				Thing.At = CityDetailsPlace::Pair(T, 1) - Foot;
				Thing.Yaw = T[3]->AsNumber();
				Thing.Length = CityDetailsPlace::Number(T, 4);
				YardThing(Thing, Shapes);
			}
		}
		Court->AddShapes(Shapes);
		// A lamp over the gateway's yard end.
		Court->AddLamp(FVector(ExitAt + CityDetailsPlace::Ahead(GateYaw, -60.0), 300.0), 800.f);
	}
}

void ACityDetails::BuildAdverts(const FJsonObject& Root)
{
	using namespace CityDetailsModel;
	const TArray<TSharedPtr<FJsonValue>>* Adverts = nullptr;
	if (!Root.TryGetArrayField(TEXT("wall_ads"), Adverts))
	{
		return;
	}
	for (const TSharedPtr<FJsonValue>& AdValue : *Adverts)
	{
		const TSharedPtr<FJsonObject> Ad = AdValue->AsObject();
		const TArray<TSharedPtr<FJsonValue>>* At = nullptr;
		const TArray<TSharedPtr<FJsonValue>>* Seen = nullptr;
		if (!Ad.IsValid() || !Ad->TryGetArrayField(TEXT("at"), At) || !Ad->TryGetArrayField(TEXT("ground"), Seen))
		{
			continue;
		}
		const double AdYaw = CityDetailsPlace::Number(*At, 2);
		AShapeSet* Set = PutUp(FVector(CityDetailsPlace::Pair(*At), GroundAt(CityDetailsPlace::Pair(*Seen))), AdYaw, CityDetailsPlace::BigDrawDistance);
		if (!Set)
		{
			continue;
		}
		TArray<FShape> Shapes;
		TArray<FSign> Lettering;
		WallAdvert(static_cast<int32>(Ad->GetNumberField(TEXT("kind"))), Ad->GetNumberField(TEXT("width")),
			Ad->GetNumberField(TEXT("z0")), Ad->GetNumberField(TEXT("z1")), Shapes, Lettering);
		Set->AddShapes(Shapes);
		for (const FSign& Sign : Lettering)
		{
			Set->AddSign(Sign.Text, Sign.At, Sign.Yaw, Sign.Height, Sign.Color);
		}
	}
}

void ACityDetails::BuildKiosks(const FJsonObject& Root)
{
	using namespace CityDetailsModel;
	const TArray<TSharedPtr<FJsonValue>>* Kiosks = nullptr;
	if (!Root.TryGetArrayField(TEXT("kiosks"), Kiosks))
	{
		return;
	}
	for (const TSharedPtr<FJsonValue>& KioskValue : *Kiosks)
	{
		const TArray<TSharedPtr<FJsonValue>>& K = KioskValue->AsArray();
		if (K.Num() < 4)
		{
			continue;
		}
		const FVector2D Spot = CityDetailsPlace::Pair(K);
		AShapeSet* Set = PutUp(FVector(Spot, GroundAt(Spot)), K[2]->AsNumber(), CityDetailsPlace::SmallDrawDistance);
		if (!Set)
		{
			continue;
		}
		TArray<FShape> Shapes;
		TArray<FLamp> Lights;
		TArray<FSign> Lettering;
		Kiosk(static_cast<int32>(K[3]->AsNumber()), Shapes, Lights, Lettering);
		Set->AddShapes(Shapes);
		for (const FLamp& Light : Lights)
		{
			Set->AddLamp(Light.At, Light.Radius);
		}
		for (const FSign& Sign : Lettering)
		{
			Set->AddSign(Sign.Text, Sign.At, Sign.Yaw, Sign.Height, Sign.Color);
		}
	}
}

void ACityDetails::BuildClocks(const FJsonObject& Root)
{
	using namespace CityDetailsModel;
	const TArray<TSharedPtr<FJsonValue>>* Clocks = nullptr;
	if (!Root.TryGetArrayField(TEXT("clocks"), Clocks))
	{
		return;
	}
	for (const TSharedPtr<FJsonValue>& ClockValue : *Clocks)
	{
		const TArray<TSharedPtr<FJsonValue>>& C = ClockValue->AsArray();
		if (C.Num() < 3)
		{
			continue;
		}
		const FVector2D Spot = CityDetailsPlace::Pair(C);
		const FVector Foot(Spot, GroundAt(Spot));
		const double ClockYaw = C[2]->AsNumber();
		AShapeSet* Set = PutUp(Foot, ClockYaw, CityDetailsPlace::SmallDrawDistance);
		if (!Set)
		{
			continue;
		}
		TArray<FShape> Shapes;
		Clock(Shapes);
		Set->AddShapes(Shapes);
		ClockSpots.Add(FTransform(FRotator(0.0, ClockYaw, 0.0), Foot));
	}
}

void ACityDetails::ReadMapIcons(const FJsonObject& Root)
{
	using EIcon = APestiHUD::EMapIcon;
	struct FKindName
	{
		const TCHAR* Key;
		EIcon Value;
	};
	static const FKindName Kinds[] = {
		{ TEXT("cafe"), EIcon::Cafe }, { TEXT("market"), EIcon::Market }, { TEXT("tram"), EIcon::Tram },
		{ TEXT("phone"), EIcon::Phone }, { TEXT("yard"), EIcon::Yard }, { TEXT("theatre"), EIcon::Theatre },
		{ TEXT("station"), EIcon::Station }, { TEXT("store"), EIcon::Store }, { TEXT("church"), EIcon::Church },
	};
	const TArray<TSharedPtr<FJsonValue>>* Icons = nullptr;
	if (!Root.TryGetArrayField(TEXT("map_icons"), Icons))
	{
		return;
	}
	for (const TSharedPtr<FJsonValue>& IconValue : *Icons)
	{
		const TArray<TSharedPtr<FJsonValue>>& I = IconValue->AsArray();
		if (I.Num() < 3)
		{
			continue;
		}
		const FString Name = I[0]->AsString();
		for (const FKindName& Kind : Kinds)
		{
			if (Name == Kind.Key)
			{
				IconSpots.Add(CityDetailsPlace::Pair(I, 1));
				IconKinds.Add(static_cast<uint8>(Kind.Value));
				break;
			}
		}
	}
}

void ACityDetails::SetHands(float Hour)
{
	using namespace CityDetailsModel;
	Hands->ClearInstances();
	TArray<FClockFace> Faces;
	ClockFaces(Faces);
	TArray<FShape> Pieces;
	for (const FClockFace& Face : Faces)
	{
		ClockHands(Face, Hour, Pieces);
	}
	for (const FTransform& Spot : ClockSpots)
	{
		for (const FShape& Piece : Pieces)
		{
			const FTransform Local(FRotator(Piece.Pitch, Piece.Yaw, Piece.Roll), Piece.At, Piece.Size / 100.0);
			Hands->AddInstance(Local * Spot, true);
		}
	}
}

void ACityDetails::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bIconsSent)
	{
		if (APestiHUD* Hud = APestiHUD::Get(this))
		{
			TArray<APestiHUD::FMapIcon> Icons;
			for (int32 i = 0; i < IconSpots.Num(); ++i)
			{
				APestiHUD::FMapIcon& Icon = Icons.AddDefaulted_GetRef();
				Icon.At = IconSpots[i];
				Icon.Kind = static_cast<APestiHUD::EMapIcon>(IconKinds[i]);
			}
			Hud->SetMapIcons(Icons);
			bIconsSent = true;
		}
	}

	HandsCheckIn -= DeltaSeconds;
	if (HandsCheckIn > 0.f || ClockSpots.Num() == 0 || !Hands)
	{
		return;
	}
	HandsCheckIn = CityDetailsPlace::HandsCheckEvery;
	const ADayNight* Sky = ADayNight::Get(GetWorld());
	const float Hour = Sky ? Sky->GetHour() : 12.f;
	if (FMath::Abs(Hour - ShownHour) >= CityDetailsPlace::HandsStep)
	{
		ShownHour = Hour;
		SetHands(Hour);
	}
}
