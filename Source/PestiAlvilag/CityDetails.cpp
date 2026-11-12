// Fill out your copyright notice in the Description page of Project Settings.

#include "CityDetails.h"

#include "ChurchModel.h"
#include "CityTheatreModel.h"
#include "CityDetailsModel.h"
#include "CityLifeModel.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "DayNight.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "NewYorkModel.h"
#include "ParkModel.h"
#include "Pedestrians.h"
#include "PersonKit.h"
#include "PersonModel.h"
#include "PestiHud.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "ShapeSet.h"
#include "StoryPerson.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogCityDetails, Log, All);

namespace CityDetailsPlace
{
	const TCHAR* const DataFile = TEXT("Data/OsmGraybox/city_details.json");
	// A gate counts as open when its foot is this near an open courtyard's gate (cm).
	constexpr double OpenGateNear = 100.0;
	// Hiding from the police counts this far in from a gateway's street end (cm).
	constexpr double HideStepIn = 120.0;
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
	// The street's life is gathered into one shape set per square patch this wide (cm).
	constexpr double LifePatch = 12000.0;
	constexpr float LifeDrawDistance = 12000.f;
	constexpr float ChurchDrawDistance = 80000.f;
	// The coal man stands this far out from the wall and along it from the hatch (cm), and is not drawn beyond this.
	const FVector2D CoalManAt(110.0, -60.0);
	constexpr float CoalManDistance = 9000.f;

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

bool ACityDetails::IsHidingPlace(const FVector2D& At)
{
	// Read once: each open gateway (foot, the way out, width, depth) and its yard's outline.
	struct FHideout
	{
		FVector2D Foot = FVector2D::ZeroVector;
		FVector2D Out = FVector2D::ZeroVector;
		double HalfWidth = 0.0;
		double Depth = 0.0;
		TArray<FVector2D> Yard;
	};
	static TArray<FHideout> Hideouts;
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
					if (!Yard.IsValid() || !Yard->TryGetArrayField(TEXT("gate"), Gate) || Gate->Num() < 3)
					{
						continue;
					}
					FHideout Hideout;
					Hideout.Foot = CityDetailsPlace::Pair(*Gate);
					Hideout.Out = CityDetailsPlace::Ahead(CityDetailsPlace::Number(*Gate, 2), 1.0);
					Hideout.HalfWidth = Yard->GetNumberField(TEXT("width")) * 0.5;
					Hideout.Depth = Yard->GetNumberField(TEXT("depth"));
					const TArray<TSharedPtr<FJsonValue>>* Outline = nullptr;
					if (Yard->TryGetArrayField(TEXT("yard"), Outline))
					{
						for (int32 i = 0; i + 1 < Outline->Num(); i += 2)
						{
							Hideout.Yard.Add(CityDetailsPlace::Pair(*Outline, i));
						}
					}
					Hideouts.Add(MoveTemp(Hideout));
				}
			}
		}
	}
	for (const FHideout& Hideout : Hideouts)
	{
		// In the gateway, a step or more in from the street.
		const FVector2D Offset = At - Hideout.Foot;
		const double In = -FVector2D::DotProduct(Offset, Hideout.Out);
		const double Across = FMath::Abs(FVector2D::CrossProduct(Hideout.Out, Offset));
		if (In > CityDetailsPlace::HideStepIn && In < Hideout.Depth + 100.0 && Across < Hideout.HalfWidth)
		{
			return true;
		}
		// In the yard (even-odd rule).
		bool bIn = false;
		const int32 Count = Hideout.Yard.Num();
		for (int32 i = 0, j = Count - 1; i < Count; j = i++)
		{
			const FVector2D& P = Hideout.Yard[i];
			const FVector2D& Q = Hideout.Yard[j];
			if ((P.Y > At.Y) != (Q.Y > At.Y) && At.X < (Q.X - P.X) * (At.Y - P.Y) / (Q.Y - P.Y) + P.X)
			{
				bIn = !bIn;
			}
		}
		if (bIn)
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
		BuildChurches(*Root);
		BuildNewYork(*Root);
		BuildCityTheatre(*Root);
		BuildStreetLife(*Root);
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

void ACityDetails::BuildChurches(const FJsonObject& Root)
{
	using namespace ChurchModel;
	const TArray<TSharedPtr<FJsonValue>>* Churches = nullptr;
	if (!Root.TryGetArrayField(TEXT("churches"), Churches))
	{
		return;
	}
	for (const TSharedPtr<FJsonValue>& ChurchValue : *Churches)
	{
		// [kind, x, y, yaw, width, depth]
		const TArray<TSharedPtr<FJsonValue>>& C = ChurchValue->AsArray();
		if (C.Num() < 6)
		{
			continue;
		}
		const FVector2D Front = CityDetailsPlace::Pair(C, 1);
		const double FrontYaw = C[3]->AsNumber();
		const EChurch Which = C[0]->AsString() == TEXT("rokus") ? EChurch::Rokus : EChurch::JezusSzive;
		const double Ground = GroundAt(Front + CityDetailsPlace::Ahead(FrontYaw, 300.0));
		AShapeSet* Set = PutUp(FVector(Front, Ground), FrontYaw, CityDetailsPlace::ChurchDrawDistance);
		if (!Set)
		{
			continue;
		}
		TArray<FShape> Shapes;
		TArray<FLamp> Lights;
		Church(Which, C[4]->AsNumber(), C[5]->AsNumber(), Shapes, Lights);
		// Its steps and flower stall, and the folk about the door.
		TArray<FChurchFolk> Folk;
		Forecourt(Which, Shapes, Folk);
		Set->AddShapes(Shapes);
		for (const FLamp& Light : Lights)
		{
			Set->AddLamp(Light.At, Light.Radius);
		}

		// The folk are passers-by that keep to their spots: bumped, driven
		// at or asked the way, they react as anyone in the street would.
		TActorIterator<APedestrians> Crowd(GetWorld());
		if (!Crowd)
		{
			continue;
		}
		const FRotator Turn(0.0, FrontYaw, 0.0);
		int32 OldWomen = 0;
		for (const FChurchFolk& One : Folk)
		{
			const PersonKit::FOutfit Clothes = FolkOutfit(One.Who, One.Who == EChurchFolk::OldWoman ? OldWomen++ : 0);
			const FVector Spot = FVector(Front, 0.0) + Turn.RotateVector(FVector(One.At, 0.0));
			const double SeatedUp = One.SeatHeight > 0.0 ? ParkModel::SeatedMiddle(Clothes, One.SeatHeight, PersonModel::MiddleHeight) : 0.0;
			const TCHAR* Name = TEXT("");
			const TCHAR* Hu = TEXT("");
			const TCHAR* En = TEXT("");
			FolkCall(One.Who, Name, Hu, En);
			Crowd->AddStander(FVector2D(Spot), Ground + One.Up, FrontYaw + One.Yaw, Clothes, SeatedUp, Name, Hu, En);
		}
	}
}

void ACityDetails::BuildNewYork(const FJsonObject& Root)
{
	using namespace NewYorkModel;
	const TArray<TSharedPtr<FJsonValue>>* Runs = nullptr;
	if (!Root.TryGetArrayField(TEXT("newyork"), Runs) || Runs->Num() == 0)
	{
		return;
	}
	// [part, x, y, yaw, length] each, in the city; the set stands at the first one's start.
	TArray<FPiece> Pieces;
	FVector2D SetAt = FVector2D::ZeroVector;
	double SetYaw = 0.0;
	for (const TSharedPtr<FJsonValue>& RunValue : *Runs)
	{
		const TArray<TSharedPtr<FJsonValue>>& R = RunValue->AsArray();
		if (R.Num() < 5)
		{
			continue;
		}
		const FString Part = R[0]->AsString();
		FPiece Piece;
		Piece.Part = Part == TEXT("korut") ? EPiece::Korut : Part == TEXT("corner") ? EPiece::Corner : EPiece::Dohany;
		Piece.Start = CityDetailsPlace::Pair(R, 1);
		Piece.Yaw = R[3]->AsNumber();
		Piece.Length = R[4]->AsNumber();
		if (Pieces.Num() == 0)
		{
			SetAt = Piece.Start;
			SetYaw = Piece.Yaw;
		}
		// Into the set's frame.
		Piece.Start = FVector2D(FRotator(0.0, -SetYaw, 0.0).RotateVector(FVector(Piece.Start - SetAt, 0.0)));
		Piece.Yaw -= SetYaw;
		Pieces.Add(Piece);
	}
	if (Pieces.Num() == 0)
	{
		return;
	}
	const double Ground = GroundAt(SetAt + CityDetailsPlace::Ahead(SetYaw, CityDetailsPlace::OutFromWall));
	AShapeSet* Set = PutUp(FVector(SetAt, Ground), SetYaw, CityDetailsPlace::ChurchDrawDistance);
	if (!Set)
	{
		return;
	}
	TArray<FShape> Shapes;
	TArray<FLamp> Lights;
	TArray<FSign> Lettering;
	Palace(Pieces, Shapes, Lights, Lettering);
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

void ACityDetails::BuildCityTheatre(const FJsonObject& Root)
{
	using namespace CityTheatreModel;
	// [x, y, yaw, length]: the front's start at the foot, the way out to the square, its length.
	const TArray<TSharedPtr<FJsonValue>>* FrontValues = nullptr;
	if (!Root.TryGetArrayField(TEXT("city_theatre"), FrontValues) || FrontValues->Num() < 4)
	{
		return;
	}
	const FVector2D Start = CityDetailsPlace::Pair(*FrontValues, 0);
	const double FrontYaw = (*FrontValues)[2]->AsNumber();
	const double FrontLength = (*FrontValues)[3]->AsNumber();
	const FVector2D Middle = Start + FVector2D(FRotator(0.0, FrontYaw, 0.0).RotateVector(FVector(0.0, FrontLength * 0.5, 0.0)));
	const double Ground = GroundAt(Middle + CityDetailsPlace::Ahead(FrontYaw, CityDetailsPlace::OutFromWall + CityTheatreModel::MiddleOut + CityTheatreModel::CanopyDepth));
	AShapeSet* Set = PutUp(FVector(Start, Ground), FrontYaw, CityDetailsPlace::ChurchDrawDistance);
	if (!Set)
	{
		return;
	}
	TArray<FShape> Shapes;
	TArray<FLamp> Lights;
	TArray<FSign> Lettering;
	Theatre(FrontLength, Shapes, Lights, Lettering);
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

void ACityDetails::BuildStreetLife(const FJsonObject& Root)
{
	using namespace CityLifeModel;

	// Everything goes into its patch's list, in world space round the patch's middle.
	TMap<FIntPoint, TArray<FShape>> Patches;
	auto PatchOf = [](const FVector2D& Spot)
	{
		return FIntPoint(FMath::FloorToInt(Spot.X / CityDetailsPlace::LifePatch), FMath::FloorToInt(Spot.Y / CityDetailsPlace::LifePatch));
	};
	auto MiddleOf = [](const FIntPoint& Patch)
	{
		return FVector((Patch.X + 0.5) * CityDetailsPlace::LifePatch, (Patch.Y + 0.5) * CityDetailsPlace::LifePatch, 0.0);
	};
	auto DropShapes = [&](const TArray<FShape>& Local, const FVector& Spot, double SpotYaw)
	{
		const FIntPoint Patch = PatchOf(FVector2D(Spot.X, Spot.Y));
		CityLifeModel::Place(Local, Spot - MiddleOf(Patch), SpotYaw, Patches.FindOrAdd(Patch));
	};
	// Each list in the file is of [x, y, yaw, ...] entries.
	auto ForEachEntry = [&Root](const TCHAR* Field, int32 MinCount, TFunctionRef<void(const TArray<TSharedPtr<FJsonValue>>&)> Use)
	{
		const TArray<TSharedPtr<FJsonValue>>* Entries = nullptr;
		if (!Root.TryGetArrayField(Field, Entries))
		{
			return;
		}
		for (const TSharedPtr<FJsonValue>& EntryValue : *Entries)
		{
			const TArray<TSharedPtr<FJsonValue>>& E = EntryValue->AsArray();
			if (E.Num() >= MinCount)
			{
				Use(E);
			}
		}
	};

	ForEachEntry(TEXT("hydrants"), 3, [&](const TArray<TSharedPtr<FJsonValue>>& E)
	{
		const FVector2D Spot = CityDetailsPlace::Pair(E);
		TArray<FShape> Local;
		Hydrant(Local);
		DropShapes(Local, FVector(Spot, GroundAt(Spot)), E[2]->AsNumber());
	});
	ForEachEntry(TEXT("manholes"), 4, [&](const TArray<TSharedPtr<FJsonValue>>& E)
	{
		const FVector2D Spot = CityDetailsPlace::Pair(E);
		TArray<FShape> Local;
		Manhole(static_cast<int32>(E[3]->AsNumber()), Local);
		DropShapes(Local, FVector(Spot, GroundAt(Spot)), E[2]->AsNumber());
	});
	ForEachEntry(TEXT("hatches"), 4, [&](const TArray<TSharedPtr<FJsonValue>>& E)
	{
		const FVector2D Spot = CityDetailsPlace::Pair(E);
		const double HatchYaw = E[2]->AsNumber();
		TArray<FShape> Local;
		CellarHatch(static_cast<int32>(E[3]->AsNumber()), Local);
		DropShapes(Local, FVector(Spot, GroundAt(Spot + CityDetailsPlace::Ahead(HatchYaw, CityDetailsPlace::OutFromWall))), HatchYaw);
	});
	ForEachEntry(TEXT("troughs"), 3, [&](const TArray<TSharedPtr<FJsonValue>>& E)
	{
		const FVector2D Spot = CityDetailsPlace::Pair(E);
		TArray<FShape> Local;
		Trough(Local);
		DropShapes(Local, FVector(Spot, GroundAt(Spot)), E[2]->AsNumber());
	});
	ForEachEntry(TEXT("coal_drops"), 4, [&](const TArray<TSharedPtr<FJsonValue>>& E)
	{
		const FVector2D Spot = CityDetailsPlace::Pair(E);
		const double DropYaw = E[2]->AsNumber();
		const uint32 Seed = static_cast<uint32>(E[3]->AsNumber());
		const FVector Foot(Spot, GroundAt(Spot + CityDetailsPlace::Ahead(DropYaw, CityDetailsPlace::OutFromWall)));
		TArray<FShape> Local;
		CoalDrop(Seed, Local);
		DropShapes(Local, Foot, DropYaw);
		PutCoalMan(Foot, DropYaw, Seed);
	});
	// The people at the windows: the facades stand on the flat ground, so the sill's height is the height in the world.
	ForEachEntry(TEXT("window_folk"), 6, [&](const TArray<TSharedPtr<FJsonValue>>& E)
	{
		TArray<FShape> Local;
		WindowFolk(static_cast<int32>(E[4]->AsNumber()), static_cast<uint32>(E[5]->AsNumber()), Local);
		DropShapes(Local, FVector(CityDetailsPlace::Pair(E), E[2]->AsNumber()), E[3]->AsNumber());
	});

	int32 Count = 0;
	for (const TPair<FIntPoint, TArray<FShape>>& Patch : Patches)
	{
		if (AShapeSet* Set = PutUp(MiddleOf(Patch.Key), 0.0, CityDetailsPlace::LifeDrawDistance))
		{
			Set->AddShapes(Patch.Value);
			Count += Patch.Value.Num();
		}
	}
	UE_LOG(LogCityDetails, Log, TEXT("Street life: %d shapes in %d sets"), Count, Patches.Num());
}

void ACityDetails::PutCoalMan(const FVector& Foot, double FacingYaw, uint32 Seed)
{
	// The coal man by the chute, turned to the hatch: dark, dusty clothes,
	// a flat cap and a leather apron.
	const FRotator Turn(0.0, FacingYaw, 0.0);
	const FVector Spot = Foot + Turn.RotateVector(FVector(CityDetailsPlace::CoalManAt, PersonModel::MiddleHeight));
	PersonKit::FOutfit Clothes;
	Clothes.Height = 170.f + static_cast<float>(Seed % 12u);
	Clothes.Build = 1.1f;
	Clothes.Hat = PersonKit::EHat::FlatCap;
	Clothes.Top = PersonKit::ETop::Waistcoat;
	Clothes.bTie = false;
	Clothes.bApron = true;
	Clothes.bBoots = true;
	Clothes.bMoustache = Seed % 2u == 0u;
	Clothes.Skin = FLinearColor(0.42f, 0.30f, 0.24f);
	Clothes.HatColor = FLinearColor(0.05f, 0.05f, 0.05f);
	Clothes.Shirt = FLinearColor(0.30f, 0.29f, 0.27f);
	Clothes.Waistcoat = FLinearColor(0.06f, 0.06f, 0.06f);
	Clothes.Trousers = FLinearColor(0.07f, 0.07f, 0.07f);
	Clothes.Apron = FLinearColor(0.10f, 0.07f, 0.05f);
	const FTransform Spawn(FRotator(0.0, FacingYaw + 150.0, 0.0), Spot);
	AStoryPerson* CoalMan = GetWorld()->SpawnActorDeferred<AStoryPerson>(AStoryPerson::StaticClass(), Spawn, this, nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!CoalMan)
	{
		return;
	}
	CoalMan->SetOutfit(Clothes);
	CoalMan->FinishSpawning(Spawn);
	CoalMan->SetActorTickEnabled(false);
	TInlineComponentArray<UPrimitiveComponent*> Shapes(CoalMan);
	for (UPrimitiveComponent* Shape : Shapes)
	{
		Shape->SetCachedMaxDrawDistance(CityDetailsPlace::CoalManDistance);
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
