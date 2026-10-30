// Fill out your copyright notice in the Description page of Project Settings.

#include "PolicePatrols.h"

#include "CarBody.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/TextRenderComponent.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Engine/Texture.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "OsmGraybox.h"
#include "PestiAlvilagGameModeBase.h"
#include "PestiCar.h"
#include "PestiHud.h"
#include "PoliceCar.h"
#include "PersonModel.h"
#include "PoliceModel.h"
#include "ProceduralMeshComponent.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "TrafficManager.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogPolicePatrols, Log, All);

namespace PolicePatrol
{
	// The two walk side by side, this far to the right of the walking line.
	const float Lanes[2] = { 45.f, 125.f };
	constexpr double EndCell = 1000.0;
	constexpr double JoinDistance = 700.0;
	constexpr float GroundEvery = 0.3f;
	// Drawn positions catch up with their place on the beat at up to this many
	// times their pace, and turn at this rate (degrees a second).
	constexpr double CatchUp = 1.8;
	constexpr float TurnRate = 300.f;
	constexpr double Arrived = 40.0;

	// A routine call to the station: every so often, when they pass this
	// close to their box (cm), and for this long (s).
	constexpr float MinCallEvery = 70.f;
	constexpr float MaxCallEvery = 150.f;
	constexpr double CallWhenWithin = 900.0;
	constexpr float CallSeconds = 9.f;
	constexpr float CrankSeconds = 1.5f;
	// Telephoning for help, and how near the box must be to bother (cm).
	constexpr float HelpCallSeconds = 4.f;
	constexpr double HelpBoxRange = 7000.0;

	// In a chase: straight at the player when he is in sight and this close (cm).
	constexpr double DirectRange = 3000.0;
	constexpr float SightEvery = 0.25f;
	constexpr float ReplanEvery = 2.f;
	// Close enough to lay hands on him (cm).
	constexpr double ArrestRange = 220.0;
	// Eyes above the middle (cm).
	constexpr double EyeUp = 70.0;

	// The player's car knocks a constable down when it comes this close this fast.
	constexpr double CarHitRadius = 150.0;
	constexpr double CarHitSpeed = 300.0;
	constexpr float DownSeconds = 4.f;
	constexpr double LyingHeight = 18.0;

	constexpr double ShoutEvery = 4.0;

	// What they say (Hungarian, then English).
	struct FLine2 { const TCHAR* Hu; const TCHAR* En; };
	const FLine2 StopLines[] = {
		{ TEXT("\u00c1llj! Rend\u0151rs\u00e9g!"), TEXT("Stop! Police!") },
		{ TEXT("\u00c1lljon meg! A t\u00f6rv\u00e9ny nev\u00e9ben!"), TEXT("Stop! In the name of the law!") },
		{ TEXT("Fogj\u00e1k meg! Ott fut!"), TEXT("Stop him! There he goes!") },
	};
	const FLine2 PhoneLine = { TEXT("K\u00f6zpont? Er\u0151s\u00edt\u00e9st k\u00e9rek, azonnal!"), TEXT("Headquarters? Send help, at once!") };
	const FLine2 DownLine = { TEXT("Jaj! Elg\u00e1zolt egy rend\u0151rt, az \u00e1tkozott!"), TEXT("Ow! The devil ran down a policeman!") };

	// "RENDŐRSÉG" (police), on the box's sign band.
	const TCHAR* SignWord = TEXT("REND\u0150RS\u00c9G");

	FIntPoint CellOf(const FVector2D& At)
	{
		return FIntPoint(FMath::FloorToInt(At.X / EndCell), FMath::FloorToInt(At.Y / EndCell));
	}

	/** A procedural mesh for one model piece, painted in its colours; the box's glass see-through. */
	UProceduralMeshComponent* MakeMesh(AActor* InOwner, USceneComponent* Parent, const PersonKit::FPiece& Piece,
		UMaterialInterface* BaseMaterial, UMaterialInterface* SeeThrough = nullptr, UTexture* White = nullptr)
	{
		UProceduralMeshComponent* Mesh = NewObject<UProceduralMeshComponent>(InOwner);
		Mesh->SetupAttachment(Parent);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetCanEverAffectNavigation(false);
		for (int32 s = 0; s < Piece.Sections.Num(); ++s)
		{
			const PersonKit::FSection& Section = Piece.Sections[s];
			Mesh->CreateMeshSection_LinearColor(s, Section.Vertices, Section.Triangles, Section.Normals,
				TArray<FVector2D>(), TArray<FLinearColor>(), TArray<FProcMeshTangent>(), false);
			if (SeeThrough && Section.Color == PoliceModel::BoxGlass)
			{
				UMaterialInstanceDynamic* Glass = UMaterialInstanceDynamic::Create(SeeThrough, InOwner);
				if (White)
				{
					Glass->SetTextureParameterValue(TEXT("SlateUI"), White);
				}
				Glass->SetScalarParameterValue(TEXT("OpacityFromTexture"), 0.f);
				FLinearColor Tint = Section.Color;
				Tint.A = 0.25f;
				Glass->SetVectorParameterValue(TEXT("TintColorAndOpacity"), Tint);
				Mesh->SetMaterial(s, Glass);
			}
			else if (BaseMaterial)
			{
				UMaterialInstanceDynamic* Paint = UMaterialInstanceDynamic::Create(BaseMaterial, InOwner);
				Paint->SetVectorParameterValue(TEXT("Color"), Section.Color);
				Mesh->SetMaterial(s, Paint);
			}
		}
		Mesh->RegisterComponent();
		InOwner->AddInstanceComponent(Mesh);
		return Mesh;
	}
}

// --- APoliceman ---------------------------------------------------------------

APoliceman::APoliceman()
{
}

void APoliceman::PutOnUniform(const PersonKit::FOutfit& Uniform)
{
	SetOutfit(Uniform);
	for (TObjectPtr<UProceduralMeshComponent>* Extra : { &Kit, &Truncheon, &Earpiece })
	{
		if (*Extra)
		{
			(*Extra)->DestroyComponent();
			*Extra = nullptr;
		}
	}
	USceneComponent* BodyPart = Limb(PersonKit::Body);
	USceneComponent* RightHand = Limb(PersonKit::ForearmRight);
	USceneComponent* LeftHand = Limb(PersonKit::ForearmLeft);
	if (!BodyPart || !RightHand || !LeftHand)
	{
		return;
	}
	PersonKit::FPiece Extras;
	PoliceModel::BuildUniform(Uniform, Extras);
	Kit = PolicePatrol::MakeMesh(this, BodyPart, Extras, BaseMaterial);
	PersonKit::FPiece Club;
	PoliceModel::BuildTruncheon(Uniform, Club);
	Truncheon = PolicePatrol::MakeMesh(this, RightHand, Club, BaseMaterial);
	Truncheon->SetVisibility(false);
	PersonKit::FPiece Ear;
	PoliceModel::BuildEarpiece(Uniform, Ear);
	Earpiece = PolicePatrol::MakeMesh(this, LeftHand, Ear, BaseMaterial);
	Earpiece->SetVisibility(false);
}

void APoliceman::ShowTruncheon(bool bShow)
{
	if (Truncheon)
	{
		Truncheon->SetVisibility(bShow);
	}
}

void APoliceman::WalkOn(const FVector& Location, double Yaw, float Speed, float DeltaSeconds)
{
	WalkTo(Location, Yaw, Speed, DeltaSeconds);
	PersonModel::PoseElbows(Pieces, 12.f, 12.f);
	if (Earpiece)
	{
		Earpiece->SetVisibility(false);
	}
}

void APoliceman::RunTo(const FVector& Location, double Yaw, float Speed, float DeltaSeconds)
{
	WalkTo(Location, Yaw, Speed, DeltaSeconds);
	// The truncheon held up and forward as he runs, the other arm pumping.
	float Swing = 0.f;
	if (USceneComponent* Arm = Limb(PersonKit::ArmRight))
	{
		Swing = static_cast<float>(Arm->GetRelativeRotation().Pitch);
		Arm->SetRelativeRotation(FRotator(30.f + Swing * 0.3f, 0.f, 0.f));
	}
	PersonModel::PoseElbows(Pieces, 70.f, 75.f - Swing * 0.3f);
	if (Earpiece)
	{
		Earpiece->SetVisibility(false);
	}
}

void APoliceman::PoseAtPhone(const FVector& Location, double Yaw, bool bCranking, float Seconds)
{
	Hold(Location, FRotator(0.0, Yaw, 0.0), 0.f, 0.f, 0.f);
	// The right hand on the crank (turning it to ring), the earpiece to his left ear.
	const float Crank = bCranking ? 10.f * FMath::Sin(Seconds * 14.f) : 0.f;
	if (USceneComponent* Arm = Limb(PersonKit::ArmRight))
	{
		Arm->SetRelativeRotation(FRotator(35.f + Crank * 0.5f, 0.f, 0.f));
	}
	if (USceneComponent* Arm = Limb(PersonKit::ArmLeft))
	{
		Arm->SetRelativeRotation(FRotator(30.f, 0.f, 35.f));
	}
	PersonModel::PoseElbows(Pieces, 145.f, 55.f + Crank);
	if (Earpiece)
	{
		Earpiece->SetVisibility(true);
	}
}

void APoliceman::Stand(const FVector& Location, double Yaw)
{
	Hold(Location, FRotator(0.0, Yaw, 0.0), 0.f, 0.f, 2.f);
	PersonModel::PoseElbows(Pieces, 5.f, 5.f);
	if (Earpiece)
	{
		Earpiece->SetVisibility(false);
	}
}

// --- APoliceBox ---------------------------------------------------------------

APoliceBox::APoliceBox()
{
	PrimaryActorTick.bCanEverTick = false;

	// Solid to the player and the cars: the walls and the roof, as one block.
	Collision = CreateDefaultSubobject<UBoxComponent>(TEXT("Collision"));
	Collision->SetBoxExtent(FVector(PoliceModel::BoxHalf, PoliceModel::BoxHalf, PoliceModel::BoxEaves * 0.5));
	Collision->SetRelativeLocation(FVector(0.0, 0.0, PoliceModel::BoxEaves * 0.5));
	Collision->SetCollisionProfileName(TEXT("BlockAll"));
	RootComponent = Collision;

	Body = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(Collision);
	Body->SetRelativeLocation(FVector(0.0, 0.0, -PoliceModel::BoxEaves * 0.5));
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Body->SetCanEverAffectNavigation(false);

	BaseMaterial = CarBody::LoadPaintMaterial();
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Translucent(TEXT("/Engine/EngineMaterials/Widget3DPassThrough_Translucent.Widget3DPassThrough_Translucent"));
	SeeThroughMaterial = Translucent.Object;
	static ConstructorHelpers::FObjectFinder<UTexture> White(TEXT("/Engine/EngineResources/WhiteSquareTexture.WhiteSquareTexture"));
	WhiteTexture = White.Object;

	// The word on each side of the sign band.
	const FVector Sides[] = { FVector(1.0, 0.0, 0.0), FVector(0.0, 1.0, 0.0), FVector(-1.0, 0.0, 0.0), FVector(0.0, -1.0, 0.0) };
	for (int32 i = 0; i < 4; ++i)
	{
		UTextRenderComponent* Text = CreateDefaultSubobject<UTextRenderComponent>(*FString::Printf(TEXT("Sign%d"), i));
		Text->SetupAttachment(Body);
		Text->SetRelativeLocation(Sides[i] * (PoliceModel::SignOut + 0.3) + FVector(0.0, 0.0, PoliceModel::SignZ));
		Text->SetRelativeRotation(FRotator(0.0, Sides[i].Rotation().Yaw, 0.0));
		Text->SetText(FText::FromString(PolicePatrol::SignWord));
		Text->SetHorizontalAlignment(EHTA_Center);
		Text->SetVerticalAlignment(EVRTA_TextCenter);
		Text->SetWorldSize(10.f);
		Text->SetTextRenderColor(FColor(15, 35, 25));
		Text->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void APoliceBox::BeginPlay()
{
	Super::BeginPlay();

	PersonKit::FPiece Piece;
	PoliceModel::BuildBox(Piece);
	for (int32 s = 0; s < Piece.Sections.Num(); ++s)
	{
		const PersonKit::FSection& Section = Piece.Sections[s];
		Body->CreateMeshSection_LinearColor(s, Section.Vertices, Section.Triangles, Section.Normals,
			TArray<FVector2D>(), TArray<FLinearColor>(), TArray<FProcMeshTangent>(), false);
		if (Section.Color == PoliceModel::BoxGlass)
		{
			if (!SeeThroughMaterial)
			{
				// No see-through material: leave the panes out, as clear glass.
				Body->ClearMeshSection(s);
				continue;
			}
			UMaterialInstanceDynamic* Glass = UMaterialInstanceDynamic::Create(SeeThroughMaterial, this);
			if (WhiteTexture)
			{
				Glass->SetTextureParameterValue(TEXT("SlateUI"), WhiteTexture);
			}
			Glass->SetScalarParameterValue(TEXT("OpacityFromTexture"), 0.f);
			FLinearColor Tint = Section.Color;
			Tint.A = 0.25f;
			Glass->SetVectorParameterValue(TEXT("TintColorAndOpacity"), Tint);
			Body->SetMaterial(s, Glass);
		}
		else if (BaseMaterial)
		{
			UMaterialInstanceDynamic* Paint = UMaterialInstanceDynamic::Create(BaseMaterial, this);
			Paint->SetVectorParameterValue(TEXT("Color"), Section.Color);
			Body->SetMaterial(s, Paint);
		}
	}
}

FVector2D APoliceBox::PhoneSpot() const
{
	return FVector2D(GetActorTransform().TransformPosition(PoliceModel::PhoneStand));
}

double APoliceBox::PhoneYaw() const
{
	return GetActorRotation().Yaw + 180.0;
}

FVector2D APoliceBox::DoorStep() const
{
	return FVector2D(GetActorLocation()) + FVector2D(GetActorForwardVector()) * (PoliceModel::BoxHalf + 70.0);
}

// --- UPoliceWhistle -----------------------------------------------------------

UPoliceWhistle::UPoliceWhistle(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bAutoActivate = false;
}

bool UPoliceWhistle::Init(int32& SampleRate)
{
	NumChannels = 1;
	SampleRateHz = static_cast<float>(FMath::Max(SampleRate, 8000));
	return true;
}

int32 UPoliceWhistle::OnGenerateAudio(float* OutAudio, int32 NumSamples)
{
	if (Blows.exchange(0) > 0)
	{
		BlastTime = 0.f;
	}
	// Two blasts: a short one, then a long one. The pea rattling in the
	// whistle trills the note about 30 times a second.
	constexpr float FirstEnds = 0.45f;
	constexpr float SecondStarts = 0.6f;
	constexpr float SecondEnds = 1.7f;
	const float DeltaT = 1.f / SampleRateHz;
	for (int32 i = 0; i < NumSamples; ++i)
	{
		float Out = 0.f;
		if (BlastTime >= 0.f)
		{
			auto Blast = [](float T, float From, float To)
			{
				if (T < From || T > To)
				{
					return 0.f;
				}
				return FMath::Min(1.f, (T - From) / 0.03f) * FMath::Min(1.f, (To - T) / 0.06f);
			};
			const float Gain = FMath::Max(Blast(BlastTime, 0.f, FirstEnds), Blast(BlastTime, SecondStarts, SecondEnds));
			const float Warble = FMath::Sin(BlastTime * 2.f * PI * 31.f);
			const float Freq = 2850.f + 170.f * Warble;
			WavePhase = FMath::Fmod(WavePhase + Freq * DeltaT, 1.f);
			NoiseSeed = NoiseSeed * 1664525u + 1013904223u;
			const float Hiss = (static_cast<float>(NoiseSeed >> 9) / static_cast<float>(1 << 23)) * 2.f - 1.f;
			Out = Gain * (0.55f * FMath::Sin(WavePhase * 2.f * PI) * (0.75f + 0.25f * Warble) + 0.06f * Hiss);
			BlastTime += DeltaT;
			if (BlastTime > SecondEnds)
			{
				BlastTime = -1.f;
			}
		}
		OutAudio[i] = Out;
	}
	return NumSamples;
}

// --- APolicePatrols -----------------------------------------------------------

APolicePatrols::APolicePatrols()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void APolicePatrols::BeginPlay()
{
	Super::BeginPlay();

	Random.Initialize(1919);
	bReady = LoadData();
	if (bReady)
	{
		SpawnPatrols();
	}

	Whistle = NewObject<UPoliceWhistle>(this);
	Whistle->SetupAttachment(RootComponent);
	Whistle->bAllowSpatialization = true;
	Whistle->bOverrideAttenuation = true;
	Whistle->AttenuationOverrides.bAttenuate = true;
	Whistle->AttenuationOverrides.bSpatialize = true;
	Whistle->AttenuationOverrides.FalloffDistance = 6000.f;
	Whistle->RegisterComponent();
	Whistle->SetVolumeMultiplier(0.5f);
}

bool APolicePatrols::LoadData()
{
	const FString Path = FPaths::Combine(FPaths::ProjectDir(), DataFile);
	FString Text;
	TSharedPtr<FJsonObject> Root;
	if (!FFileHelper::LoadFileToString(Text, *Path)
		|| !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) || !Root.IsValid())
	{
		UE_LOG(LogPolicePatrols, Error, TEXT("Could not read %s"), *Path);
		return false;
	}
	const TArray<TSharedPtr<FJsonValue>>* Rows = nullptr;
	const TArray<TSharedPtr<FJsonValue>>* BoxRows = nullptr;
	if (!Root->TryGetArrayField(TEXT("sidewalks"), Rows) || !Root->TryGetArrayField(TEXT("police_boxes"), BoxRows))
	{
		UE_LOG(LogPolicePatrols, Warning, TEXT("No walking lines or police boxes in %s"), *Path);
		return false;
	}

	for (const TSharedPtr<FJsonValue>& Row : *Rows)
	{
		const TArray<TSharedPtr<FJsonValue>>& Numbers = Row->AsArray();
		FLine Line;
		for (int32 i = 0; i + 1 < Numbers.Num(); i += 2)
		{
			const FVector2D Point(Numbers[i]->AsNumber(), Numbers[i + 1]->AsNumber());
			Line.Length += Line.Points.Num() > 0 ? FVector2D::Distance(Line.Points.Last(), Point) : 0.0;
			Line.Points.Add(Point);
			Line.Along.Add(Line.Length);
		}
		if (Line.Points.Num() < 2 || Line.Length < 1.0)
		{
			continue;
		}
		const int32 Index = Lines.Add(MoveTemp(Line));
		Ends.Add(PolicePatrol::CellOf(Lines[Index].Points[0]), FIntPoint(Index, 0));
		Ends.Add(PolicePatrol::CellOf(Lines[Index].Points.Last()), FIntPoint(Index, 1));
	}

	for (const TSharedPtr<FJsonValue>& Row : *BoxRows)
	{
		const TSharedPtr<FJsonObject>* Fields = nullptr;
		const TArray<TSharedPtr<FJsonValue>>* At = nullptr;
		if (!Row->TryGetObject(Fields) || !(*Fields)->TryGetArrayField(TEXT("at"), At) || At->Num() < 3)
		{
			continue;
		}
		BoxPlaces.Add(FVector((*At)[0]->AsNumber(), (*At)[1]->AsNumber(), (*At)[2]->AsNumber()));
		BoxNames.Add((*Fields)->GetStringField(TEXT("name")));
	}
	UE_LOG(LogPolicePatrols, Log, TEXT("Police: %d boxes, %d walking lines"), BoxPlaces.Num(), Lines.Num());
	return Lines.Num() > 0 && BoxPlaces.Num() > 0;
}

void APolicePatrols::SpawnPatrols()
{
	UWorld* World = GetWorld();
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	for (int32 b = 0; b < BoxPlaces.Num(); ++b)
	{
		const FVector2D BoxAt(BoxPlaces[b]);
		FHitResult Hit;
		const bool bGround = World->LineTraceSingleByChannel(Hit, FVector(BoxAt, 10000.0), FVector(BoxAt, -10000.0), ECC_Visibility);
		const double Z = bGround ? Hit.ImpactPoint.Z : 0.0;
		APoliceBox* NewBox = World->SpawnActor<APoliceBox>(APoliceBox::StaticClass(),
			FVector(BoxAt, Z + PoliceModel::BoxEaves * 0.5), FRotator(0.0, BoxPlaces[b].Z, 0.0), Params);
		Boxes.Add(NewBox);

		FPatrol Patrol;
		Patrol.Box = b;
		if (!NearestOnLines(BoxAt, Patrol.Line, Patrol.Along))
		{
			continue;
		}
		Patrol.Dir = Random.FRand() < 0.5f ? 1 : -1;
		Patrol.CallClock = Random.FRandRange(PolicePatrol::MinCallEvery * 0.3f, PolicePatrol::MaxCallEvery);
		for (int32 m = 0; m < 2; ++m)
		{
			FMan& Man = Patrol.Men[m];
			FVector2D Tangent;
			Man.At = BeatSpot(Patrol, m, Tangent);
			Man.Yaw = static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(Tangent.Y, Tangent.X)));
			Man.GroundZ = Z;
			Man.Person = World->SpawnActor<APoliceman>(APoliceman::StaticClass(), FVector(Man.At, Z + PersonModel::MiddleHeight), FRotator(0.0, Man.Yaw, 0.0), Params);
			if (Man.Person)
			{
				Man.Person->PutOnUniform(PoliceModel::Outfit(Random));
			}
		}
		Patrols.Add(Patrol);
	}
}

FVector2D APolicePatrols::PointOn(const FLine& Line, double Along, FVector2D& OutTangent) const
{
	int32 i = 0;
	while (i + 2 < Line.Points.Num() && Line.Along[i + 1] < Along)
	{
		++i;
	}
	const FVector2D A = Line.Points[i];
	const FVector2D B = Line.Points[i + 1];
	const double Span = FMath::Max(Line.Along[i + 1] - Line.Along[i], 1.0);
	OutTangent = (B - A).GetSafeNormal();
	return A + (B - A) * FMath::Clamp((Along - Line.Along[i]) / Span, 0.0, 1.0);
}

bool APolicePatrols::NearestOnLines(const FVector2D& At, int32& OutLine, double& OutAlong) const
{
	double Best = TNumericLimits<double>::Max();
	for (int32 l = 0; l < Lines.Num(); ++l)
	{
		const FLine& Line = Lines[l];
		for (int32 i = 0; i + 1 < Line.Points.Num(); ++i)
		{
			const FVector2D A = Line.Points[i];
			const FVector2D Span = Line.Points[i + 1] - A;
			const double T = FMath::Clamp(FVector2D::DotProduct(At - A, Span) / FMath::Max(Span.SizeSquared(), 1.0), 0.0, 1.0);
			const double Distance = FVector2D::DistSquared(At, A + Span * T);
			if (Distance < Best)
			{
				Best = Distance;
				OutLine = l;
				OutAlong = Line.Along[i] + (Line.Along[i + 1] - Line.Along[i]) * T;
			}
		}
	}
	return Best < TNumericLimits<double>::Max();
}

void APolicePatrols::TurnAtEnd(FPatrol& Patrol)
{
	const FLine& Line = Lines[Patrol.Line];
	const FVector2D End = Patrol.Dir > 0 ? Line.Points.Last() : Line.Points[0];
	const FVector2D BoxAt(BoxPlaces[Patrol.Box]);
	const FIntPoint Cell = PolicePatrol::CellOf(End);

	TArray<FIntPoint> Joins;
	for (int32 dx = -1; dx <= 1; ++dx)
	{
		for (int32 dy = -1; dy <= 1; ++dy)
		{
			TArray<FIntPoint> Found;
			Ends.MultiFind(Cell + FIntPoint(dx, dy), Found);
			for (const FIntPoint& Join : Found)
			{
				if (Join.X == Patrol.Line)
				{
					continue;
				}
				const FLine& Other = Lines[Join.X];
				const FVector2D Near = Join.Y == 0 ? Other.Points[0] : Other.Points.Last();
				const FVector2D Far = Join.Y == 0 ? Other.Points.Last() : Other.Points[0];
				// Only lines that keep them on their beat.
				if (FVector2D::Distance(Near, End) < PolicePatrol::JoinDistance && FVector2D::Distance(Far, BoxAt) < BeatRadius)
				{
					Joins.Add(Join);
				}
			}
		}
	}
	if (Joins.IsEmpty())
	{
		Patrol.Dir = -Patrol.Dir;
		return;
	}
	const FIntPoint Pick = Joins[Random.RandRange(0, Joins.Num() - 1)];
	Patrol.Line = Pick.X;
	Patrol.Dir = Pick.Y == 0 ? 1 : -1;
	Patrol.Along = Pick.Y == 0 ? 0.0 : Lines[Pick.X].Length;
}

FVector2D APolicePatrols::BeatSpot(const FPatrol& Patrol, int32 Which, FVector2D& OutTangent) const
{
	const FVector2D Point = PointOn(Lines[Patrol.Line], Patrol.Along, OutTangent);
	OutTangent *= static_cast<double>(Patrol.Dir);
	const FVector2D Right(-OutTangent.Y, OutTangent.X);
	return Point + Right * PolicePatrol::Lanes[Which];
}

void APolicePatrols::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bReady)
	{
		return;
	}

	// Where the player is, on foot or at the wheel, and how fast he goes.
	AActor* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (Player)
	{
		PlayerAt = Player->GetActorLocation();
		PlayerVelocity = Player == LastPlayer.Get() && DeltaSeconds > 0.f ? (PlayerAt - LastPlayerAt) / DeltaSeconds : FVector::ZeroVector;
		LastPlayerAt = PlayerAt;
		LastPlayer = Player;
		bPlayerDriving = Player->IsA<APestiCar>();
	}

	// A new time wanted: everyone may whistle and telephone again.
	const APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this);
	const bool bWanted = Game && Game->GetWantedLevel() > 0;
	if (bWanted && !bWasWanted)
	{
		for (FPatrol& Patrol : Patrols)
		{
			Patrol.bAlarmed = false;
			Patrol.bPhonedForHelp = false;
		}
	}
	bWasWanted = bWanted;

	for (FPatrol& Patrol : Patrols)
	{
		TickPatrol(Patrol, DeltaSeconds);
	}
}

void APolicePatrols::TickPatrol(FPatrol& Patrol, float DeltaSeconds)
{
	APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this);
	const bool bWanted = Game && Game->GetWantedLevel() > 0;
	const double PlayerDistance = FVector2D::Distance(Patrol.Men[0].At, FVector2D(PlayerAt));

	// Once he is wanted, the pairs near him go after him; once he isn't, they go back.
	const bool bBusy = Patrol.Men[0].Doing == EDoing::Chasing || Patrol.Men[1].Doing == EDoing::Chasing
		|| Patrol.Men[0].bForHelp || Patrol.Men[1].bForHelp;
	if (bWanted && !bBusy && PlayerDistance < ResponseRange
		&& Patrol.Men[0].Doing != EDoing::Down && Patrol.Men[1].Doing != EDoing::Down)
	{
		StartChase(Patrol, false);
	}
	for (int32 m = 0; m < 2; ++m)
	{
		FMan& Man = Patrol.Men[m];
		const bool bAfterHim = Man.Doing == EDoing::Chasing || Man.bForHelp;
		if (bAfterHim && (!bWanted || FVector2D::Distance(Man.At, FVector2D(PlayerAt)) > GiveUpRange))
		{
			BackToBeat(Patrol, m);
		}
	}

	// On the beat: walk the line together, and now and then telephone in.
	if (Patrol.Men[0].Doing == EDoing::Beat && Patrol.Men[1].Doing == EDoing::Beat)
	{
		Patrol.Along += Patrol.Dir * WalkSpeed * DeltaSeconds;
		if (Patrol.Along < 0.0 || Patrol.Along > Lines[Patrol.Line].Length)
		{
			Patrol.Along = FMath::Clamp(Patrol.Along, 0.0, Lines[Patrol.Line].Length);
			TurnAtEnd(Patrol);
		}
		Patrol.CallClock -= DeltaSeconds;
		if (Patrol.CallClock <= 0.f && Boxes.IsValidIndex(Patrol.Box) && Boxes[Patrol.Box]
			&& FVector2D::Distance(Patrol.Men[0].At, Boxes[Patrol.Box]->DoorStep()) < PolicePatrol::CallWhenWithin)
		{
			Patrol.CallClock = Random.FRandRange(PolicePatrol::MinCallEvery, PolicePatrol::MaxCallEvery);
			const int32 Caller = Random.RandRange(0, 1);
			SendToPhone(Patrol, Caller, false);
			Patrol.Men[1 - Caller].Doing = EDoing::Waiting;
			Patrol.Men[1 - Caller].Clock = 0.f;
		}
	}

	for (int32 m = 0; m < 2; ++m)
	{
		TickMan(Patrol, m, DeltaSeconds);
	}
}

void APolicePatrols::TickMan(FPatrol& Patrol, int32 Which, float DeltaSeconds)
{
	FMan& Man = Patrol.Men[Which];
	if (!Man.Person)
	{
		return;
	}
	APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this);
	APoliceBox* Box = Boxes.IsValidIndex(Patrol.Box) ? Boxes[Patrol.Box].Get() : nullptr;
	Man.Clock += DeltaSeconds;
	UpdateGround(Man, DeltaSeconds);

	// The player's car coming through knocks him down: a crime in itself.
	if (bPlayerDriving && Man.Doing != EDoing::Down && Man.Doing != EDoing::Phoning
		&& FVector2D::Distance(Man.At, FVector2D(PlayerAt)) < PolicePatrol::CarHitRadius
		&& PlayerVelocity.Size2D() > PolicePatrol::CarHitSpeed)
	{
		Man.Doing = EDoing::Down;
		Man.Clock = 0.f;
		Man.Person->ShowTruncheon(false);
		if (APestiCar* Car = Cast<APestiCar>(LastPlayer.Get()))
		{
			Car->ForwardSpeed *= 0.85f;
		}
		Shout(Man, PolicePatrol::DownLine.Hu, PolicePatrol::DownLine.En);
		if (Game)
		{
			Game->ReportCrime(2);
		}
	}

	switch (Man.Doing)
	{
	case EDoing::Beat:
	{
		FVector2D Tangent;
		const FVector2D Goal = BeatSpot(Patrol, Which, Tangent);
		const bool bThere = StepTowards(Man, Goal, WalkSpeed * static_cast<float>(PolicePatrol::CatchUp), DeltaSeconds);
		// Waiting there for his mate to come back, or walking on with him.
		if (bThere && Patrol.Men[1 - Which].Doing != EDoing::Beat)
		{
			Man.Person->Stand(MiddleOf(Man), Man.Yaw);
		}
		else
		{
			Man.Person->WalkOn(MiddleOf(Man), Man.Yaw, WalkSpeed, DeltaSeconds);
		}
		break;
	}
	case EDoing::Waiting:
	{
		// Facing the street beside the box until his mate comes out.
		FVector2D Tangent;
		BeatSpot(Patrol, Which, Tangent);
		const float StreetYaw = static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(Tangent.X, -Tangent.Y)));
		Man.Yaw = FMath::FixedTurn(Man.Yaw, StreetYaw, PolicePatrol::TurnRate * DeltaSeconds);
		Man.Person->Stand(MiddleOf(Man), Man.Yaw);
		break;
	}
	case EDoing::ToPhone:
	{
		const float Speed = Man.bForHelp ? RunSpeed : WalkSpeed;
		const bool bThere = FollowPath(Man, Speed, DeltaSeconds);
		if (Man.bForHelp)
		{
			Man.Person->RunTo(MiddleOf(Man), Man.Yaw, Speed, DeltaSeconds);
		}
		else
		{
			Man.Person->WalkOn(MiddleOf(Man), Man.Yaw, Speed, DeltaSeconds);
		}
		if (bThere)
		{
			Man.Doing = EDoing::Phoning;
			Man.Clock = 0.f;
			if (Man.bForHelp)
			{
				Shout(Man, PolicePatrol::PhoneLine.Hu, PolicePatrol::PhoneLine.En);
			}
		}
		break;
	}
	case EDoing::Phoning:
	{
		const double Facing = Box ? Box->PhoneYaw() : Man.Yaw;
		Man.Yaw = FMath::FixedTurn(Man.Yaw, static_cast<float>(Facing), PolicePatrol::TurnRate * DeltaSeconds);
		Man.Person->PoseAtPhone(MiddleOf(Man), Man.Yaw, Man.Clock < PolicePatrol::CrankSeconds, Man.Clock);
		if (Man.Clock > (Man.bForHelp ? PolicePatrol::HelpCallSeconds : PolicePatrol::CallSeconds))
		{
			if (Man.bForHelp)
			{
				Man.bForHelp = false;
				Patrol.bPhonedForHelp = true;
				const bool bStillWanted = Game && Game->GetWantedLevel() > 0;
				if (bStillWanted)
				{
					Game->CallInPolice();
					Man.Doing = EDoing::Chasing;
					Man.Path.Reset();
					Man.ReplanClock = 0.f;
					Man.Person->ShowTruncheon(true);
					break;
				}
			}
			BackToBeat(Patrol, Which);
		}
		break;
	}
	case EDoing::Chasing:
	{
		const FVector2D Target(PlayerAt);
		const double Distance = FVector2D::Distance(Man.At, Target);
		Man.SightClock -= DeltaSeconds;
		if (Man.SightClock <= 0.f)
		{
			Man.SightClock = PolicePatrol::SightEvery;
			Man.bSeesPlayer = Distance < SightRange * 1.5f && CanSee(Man, PlayerAt);
		}
		if (Man.bSeesPlayer && Game)
		{
			Game->NotifySeenByPolice();
			if (!Patrol.bAlarmed)
			{
				Patrol.bAlarmed = true;
				if (Whistle)
				{
					Whistle->SetWorldLocation(MiddleOf(Man));
					if (!Whistle->IsPlaying())
					{
						Whistle->Start();
					}
					Whistle->Blow();
				}
				const PolicePatrol::FLine2& Line = PolicePatrol::StopLines[Random.RandRange(0, static_cast<int32>(UE_ARRAY_COUNT(PolicePatrol::StopLines)) - 1)];
				Shout(Man, Line.Hu, Line.En);
			}
		}
		if (Distance < PolicePatrol::ArrestRange && Game)
		{
			Game->NotifyPoliceNear();
		}

		// Straight at him once in sight and near, otherwise along the streets.
		if (Man.bSeesPlayer && Distance < PolicePatrol::DirectRange)
		{
			Man.Path.Reset();
			if (Distance > PolicePatrol::ArrestRange * 0.6)
			{
				StepTowards(Man, Target, RunSpeed, DeltaSeconds);
				Man.Person->RunTo(MiddleOf(Man), Man.Yaw, RunSpeed, DeltaSeconds);
			}
			else
			{
				// Got him: hold him there, facing him.
				const FVector2D To = Target - Man.At;
				Man.Yaw = FMath::FixedTurn(Man.Yaw, static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(To.Y, To.X))), PolicePatrol::TurnRate * DeltaSeconds);
				Man.Person->Stand(MiddleOf(Man), Man.Yaw);
				Man.Person->ShowTruncheon(true);
			}
		}
		else
		{
			Man.ReplanClock -= DeltaSeconds;
			if (Man.ReplanClock <= 0.f || Man.Next >= Man.Path.Num())
			{
				Man.ReplanClock = PolicePatrol::ReplanEvery;
				PlanPath(Man, Target);
			}
			FollowPath(Man, RunSpeed, DeltaSeconds);
			Man.Person->RunTo(MiddleOf(Man), Man.Yaw, RunSpeed, DeltaSeconds);
		}
		break;
	}
	case EDoing::Down:
	{
		// On his back for a while, then up and back to it.
		const FVector Lying(Man.At, Man.GroundZ + PolicePatrol::LyingHeight);
		Man.Person->Hold(Lying, FRotator(90.0, Man.Yaw, 0.0), 10.f, -20.f, 20.f);
		if (Man.Clock > PolicePatrol::DownSeconds)
		{
			const bool bStillWanted = Game && Game->GetWantedLevel() > 0;
			if (bStillWanted)
			{
				Man.Doing = EDoing::Chasing;
				Man.Path.Reset();
				Man.ReplanClock = 0.f;
				Man.Person->ShowTruncheon(true);
			}
			else
			{
				BackToBeat(Patrol, Which);
			}
		}
		break;
	}
	case EDoing::Returning:
	{
		const bool bThere = FollowPath(Man, WalkSpeed * 1.3f, DeltaSeconds);
		Man.Person->WalkOn(MiddleOf(Man), Man.Yaw, WalkSpeed * 1.3f, DeltaSeconds);
		if (bThere)
		{
			// Back on the beat; the pair walks on once both are.
			Man.Doing = EDoing::Beat;
		}
		break;
	}
	}
}

bool APolicePatrols::StepTowards(FMan& Man, const FVector2D& Goal, float Speed, float DeltaSeconds)
{
	const FVector2D To = Goal - Man.At;
	const double Distance = To.Size();
	if (Distance > 1.0)
	{
		const double Step = FMath::Min(Distance, static_cast<double>(Speed * DeltaSeconds));
		Man.At += To / Distance * Step;
		if (Distance > 5.0)
		{
			const float WantYaw = static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(To.Y, To.X)));
			Man.Yaw = FMath::FixedTurn(Man.Yaw, WantYaw, PolicePatrol::TurnRate * 2.f * DeltaSeconds);
		}
	}
	return Distance < PolicePatrol::Arrived;
}

bool APolicePatrols::FollowPath(FMan& Man, float Speed, float DeltaSeconds)
{
	while (Man.Next < Man.Path.Num() - 1 && FVector2D::Distance(Man.At, Man.Path[Man.Next]) < PolicePatrol::Arrived * 3.0)
	{
		++Man.Next;
	}
	if (!Man.Path.IsValidIndex(Man.Next))
	{
		return true;
	}
	const bool bLast = Man.Next == Man.Path.Num() - 1;
	return StepTowards(Man, Man.Path[Man.Next], Speed, DeltaSeconds) && bLast;
}

void APolicePatrols::PlanPath(FMan& Man, const FVector2D& Goal)
{
	Man.Path.Reset();
	Man.Next = 0;
	// Along the streets when it is far or out of sight, otherwise straight there.
	if (!Traffic.IsValid())
	{
		TActorIterator<ATrafficManager> Found(GetWorld());
		Traffic = Found ? *Found : nullptr;
	}
	TArray<FVector> Streets;
	const FVector From(Man.At, Man.GroundZ + PolicePatrol::EyeUp);
	const FVector To(Goal, Man.GroundZ + PolicePatrol::EyeUp);
	if (FVector2D::Distance(Man.At, Goal) > 600.0 && !CanSee(Man, To) && Traffic.IsValid() && Traffic->FindPath(From, To, Streets))
	{
		for (const FVector& Point : Streets)
		{
			Man.Path.Add(FVector2D(Point));
		}
		// Skip a first corner that lies behind him.
		if (Man.Path.Num() > 1 && FVector2D::Distance(Man.At, Man.Path[1]) < FVector2D::Distance(Man.Path[0], Man.Path[1]))
		{
			Man.Next = 1;
		}
	}
	Man.Path.Add(Goal);
}

void APolicePatrols::UpdateGround(FMan& Man, float DeltaSeconds)
{
	Man.GroundClock -= DeltaSeconds;
	if (Man.GroundClock > 0.f)
	{
		return;
	}
	Man.GroundClock = PolicePatrol::GroundEvery;
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(PoliceGround), false, this);
	if (Man.Person)
	{
		Params.AddIgnoredActor(Man.Person);
	}
	const double Top = Man.GroundZ + 300.0;
	// Only the open ground: not a lamp, a bench or a car in the way.
	if (GetWorld()->LineTraceSingleByChannel(Hit, FVector(Man.At, Top), FVector(Man.At, Man.GroundZ - 2000.0), ECC_Visibility, Params)
		&& Cast<AOsmGraybox>(Hit.GetActor()))
	{
		Man.GroundZ = Hit.ImpactPoint.Z;
	}
}

FVector APolicePatrols::MiddleOf(const FMan& Man) const
{
	return FVector(Man.At, Man.GroundZ + PersonModel::MiddleHeight);
}

bool APolicePatrols::CanSee(const FMan& Man, const FVector& Target) const
{
	const FVector Eye = MiddleOf(Man) + FVector(0.0, 0.0, PolicePatrol::EyeUp);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(PoliceFootSight), false, this);
	if (Man.Person)
	{
		Params.AddIgnoredActor(Man.Person);
	}
	if (AActor* Player = LastPlayer.Get())
	{
		Params.AddIgnoredActor(Player);
	}
	// Only buildings and the like block the view, not people or cars.
	FHitResult Hit;
	return !GetWorld()->LineTraceSingleByObjectType(Hit, Eye, Target + FVector(0.0, 0.0, 30.0), FCollisionObjectQueryParams(ECC_WorldStatic), Params);
}

void APolicePatrols::StartChase(FPatrol& Patrol, bool bSawIt)
{
	// The nearer one runs after the player; the other runs to the box to
	// telephone for help, if it is near and nobody has yet.
	const APoliceBox* Box = Boxes.IsValidIndex(Patrol.Box) ? Boxes[Patrol.Box].Get() : nullptr;
	int32 Caller = INDEX_NONE;
	if (Box && !Patrol.bPhonedForHelp)
	{
		const double Distance0 = FVector2D::Distance(Patrol.Men[0].At, FVector2D(PlayerAt));
		const double Distance1 = FVector2D::Distance(Patrol.Men[1].At, FVector2D(PlayerAt));
		const int32 Farther = Distance0 > Distance1 ? 0 : 1;
		if (FVector2D::Distance(Patrol.Men[Farther].At, Box->DoorStep()) < PolicePatrol::HelpBoxRange)
		{
			Caller = Farther;
		}
	}
	for (int32 m = 0; m < 2; ++m)
	{
		FMan& Man = Patrol.Men[m];
		if (Man.Doing == EDoing::Down)
		{
			continue;
		}
		if (m == Caller)
		{
			SendToPhone(Patrol, m, true);
			continue;
		}
		Man.Doing = EDoing::Chasing;
		Man.Path.Reset();
		Man.ReplanClock = 0.f;
		Man.SightClock = 0.f;
		// Someone who saw it happen has the player in sight already.
		Man.bSeesPlayer = bSawIt;
		Man.Person->ShowTruncheon(true);
	}
}

void APolicePatrols::SendToPhone(FPatrol& Patrol, int32 Which, bool bForHelp)
{
	FMan& Man = Patrol.Men[Which];
	const APoliceBox* Box = Boxes.IsValidIndex(Patrol.Box) ? Boxes[Patrol.Box].Get() : nullptr;
	if (!Box)
	{
		return;
	}
	Man.Doing = EDoing::ToPhone;
	Man.bForHelp = bForHelp;
	Man.Clock = 0.f;
	// Along the streets to the door when it is out of sight, then in.
	PlanPath(Man, Box->DoorStep());
	Man.Path.Add(Box->PhoneSpot());
	Man.Person->ShowTruncheon(false);
}

void APolicePatrols::BackToBeat(FPatrol& Patrol, int32 Which)
{
	FMan& Man = Patrol.Men[Which];
	Man.bForHelp = false;
	Man.Person->ShowTruncheon(false);
	FVector2D Tangent;
	const FVector2D Spot = BeatSpot(Patrol, Which, Tangent);
	const APoliceBox* Box = Boxes.IsValidIndex(Patrol.Box) ? Boxes[Patrol.Box].Get() : nullptr;
	if (Man.Doing == EDoing::Phoning && Box)
	{
		// Out of the door first.
		Man.Path.Reset();
		Man.Next = 0;
		Man.Path.Add(Box->DoorStep());
		Man.Path.Add(Spot);
	}
	else
	{
		PlanPath(Man, Spot);
	}
	Man.Doing = EDoing::Returning;
	// A mate left waiting goes back to walking with him.
	FMan& Mate = Patrol.Men[1 - Which];
	if (Mate.Doing == EDoing::Waiting)
	{
		Mate.Doing = EDoing::Beat;
	}
}

void APolicePatrols::Shout(const FMan& Man, const TCHAR* Hu, const TCHAR* En)
{
	const double Now = GetWorld()->GetTimeSeconds();
	APestiHUD* Hud = APestiHUD::Get(this);
	// Only one the player can hear.
	if (!Hud || Hud->IsSaying() || Now - LastShout < PolicePatrol::ShoutEvery
		|| FVector2D::Distance(Man.At, FVector2D(PlayerAt)) > SightRange * 1.5f)
	{
		return;
	}
	LastShout = Now;
	Hud->Say(TEXT("Rend\u0151r"), Hu, En, 3.f);
}

bool APolicePatrols::Witnessed(const UObject* WorldContextObject, const FVector& At, double CarRange)
{
	UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	if (!World)
	{
		return false;
	}
	bool bSeen = false;
	for (TActorIterator<APoliceCar> It(World); It; ++It)
	{
		if (FVector::Dist2D(It->GetActorLocation(), At) < CarRange)
		{
			bSeen = true;
			break;
		}
	}
	for (TActorIterator<APolicePatrols> It(World); It; ++It)
	{
		APolicePatrols* Police = *It;
		if (!Police->bReady)
		{
			continue;
		}
		for (FPatrol& Patrol : Police->Patrols)
		{
			bool bThisPair = false;
			for (const FMan& Man : Patrol.Men)
			{
				if (Man.Doing != EDoing::Down && Man.Doing != EDoing::Phoning
					&& FVector2D::Distance(Man.At, FVector2D(At)) < Police->SightRange && Police->CanSee(Man, At))
				{
					bThisPair = true;
					break;
				}
			}
			if (bThisPair)
			{
				bSeen = true;
				const bool bBusy = Patrol.Men[0].Doing == EDoing::Chasing || Patrol.Men[1].Doing == EDoing::Chasing;
				if (!bBusy)
				{
					// Not yet wanted maybe, but about to be: forget any earlier alarm.
					Patrol.bAlarmed = false;
					Police->StartChase(Patrol, true);
				}
			}
		}
	}
	return bSeen;
}
