// Fill out your copyright notice in the Description page of Project Settings.

#include "Lighters.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "FleaMarket.h"
#include "GameFramework/PlayerController.h"
#include "ImageUtils.h"
#include "KeletiStation.h"
#include "LighterModel.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/Paths.h"
#include "PestiAlvilagGameModeBase.h"
#include "PestiHud.h"
#include "PestiPerson.h"
#include "StoryPerson.h"
#include "UObject/ConstructorHelpers.h"

namespace Lighters
{
	// Bigger than life, so they can be spotted from a few steps away.
	constexpr double Scale = 2.2;
	// Walk this close and you pick it up.
	constexpr double PickUpRadius = 160.0;
	constexpr double PickUpHeight = 250.0;
	// Beyond this they aren't drawn at all.
	constexpr float DrawDistance = 5000.f;
	constexpr float GlintDistance = 6000.f;
	// The glint hangs this far over the lighter.
	constexpr double GlintOver = 32.0;
	constexpr double GlintArm = 26.0;
	constexpr double GlintShortArm = 12.0;
	constexpr double GlintThickness = 1.4;
	const FLinearColor GlintColor(1.f, 0.96f, 0.82f);
	// The light over the nearest one.
	constexpr double LightDistance = 2500.0;
	constexpr float LightIntensity = 600.f;
	constexpr float LightRadius = 320.f;
	// A thing to stand on (a column's plinth, a café table) is no higher than this over the ground.
	constexpr double StandOnHeight = 150.0;
	constexpr int32 MaxSurfaces = 8;
	// Tries again this often while the ground under some isn't built.
	constexpr float PlaceEvery = 1.f;

	constexpr int32 Columns = 6;
	constexpr int32 RewardEvery = 10;
	constexpr int32 RewardMoney = 300;
	constexpr double LuciferAfter = 5.0;
	constexpr double RingAfter = 12.0;

	// Pista bácsi's tip.
	const TCHAR* const PistaName = TEXT("Pista b\u00e1csi");
	const FName TipAfterMission(TEXT("Uzenet"));
	constexpr int32 TipPrice = 20;
	constexpr double TalkRadius = 250.0;

	constexpr int32 HintLine = 34;
	constexpr int32 RewardLine = 35;
	const FColor Yellow(255, 210, 60);

	/** "a" or "az" before a Hungarian name, by its first letter. */
	const TCHAR* Article(const FString& Name)
	{
		static const FString Vowels = TEXT("AEIOU\u00c1\u00c9\u00cd\u00d3\u00d6\u0150\u00da\u00dc\u0170");
		int32 Found = INDEX_NONE;
		return Name.Len() > 0 && Vowels.FindChar(FChar::ToUpper(Name[0]), Found) ? TEXT("az") : TEXT("a");
	}
}

ALighters::ALighters()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	GlintLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("GlintLight"));
	GlintLight->SetupAttachment(RootComponent);
	GlintLight->SetLightColor(FLinearColor(1.f, 0.85f, 0.55f));
	GlintLight->SetAttenuationRadius(Lighters::LightRadius);
	GlintLight->SetCastShadows(false);
	GlintLight->SetVisibility(false);

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicShapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	BaseMaterial = BasicShapeMaterial.Object;
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Translucent(TEXT("/Engine/EngineMaterials/Widget3DPassThrough_Translucent.Widget3DPassThrough_Translucent"));
	SeeThroughMaterial = Translucent.Object;
	static ConstructorHelpers::FObjectFinder<UTexture> White(TEXT("/Engine/EngineResources/WhiteSquareTexture.WhiteSquareTexture"));
	WhiteTexture = White.Object;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMesh(TEXT("/Engine/BasicShapes/Cone.Cone"));
	Shapes = { CubeMesh.Object, CylinderMesh.Object, SphereMesh.Object, ConeMesh.Object };
}

void ALighters::BeginPlay()
{
	Super::BeginPlay();

	// The lighters are placed in world space; the actor stays at the origin.
	SetActorLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);
	const int32 Count = LighterModel::All().Num();
	Done.Init(false, Count);
	Holders.SetNum(Count);
	Glints.SetNum(Count);
	LoadPictures();

	if (SeeThroughMaterial)
	{
		GlintPaint = UMaterialInstanceDynamic::Create(SeeThroughMaterial, this);
		if (WhiteTexture)
		{
			GlintPaint->SetTextureParameterValue(TEXT("SlateUI"), WhiteTexture);
		}
		GlintPaint->SetVectorParameterValue(TEXT("TintColorAndOpacity"), Lighters::GlintColor);
	}
	// The ground is built in the same frame: put them down a moment later.
	PlaceTimer = Lighters::PlaceEvery;
}

void ALighters::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (bCabinetOpen)
	{
		OpenCabinet(false);
	}
	Super::EndPlay(EndPlayReason);
}

void ALighters::LoadPictures()
{
	// The cabinet's pictures, Data/Lighters/0.png to 30.png (30 is the golden one).
	const int32 Count = LighterModel::All().Num() + 1;
	Pictures.SetNum(Count);
	PictureBrushes.SetNum(Count);
	for (int32 i = 0; i < Count; ++i)
	{
		const FString Path = FPaths::Combine(FPaths::ProjectDir(), TEXT("Data/Lighters"), FString::FromInt(i) + TEXT(".png"));
		Pictures[i] = FPaths::FileExists(Path) ? FImageUtils::ImportFileAsTexture2D(Path) : nullptr;
		FSlateBrush& Brush = PictureBrushes[i];
		Brush.DrawAs = ESlateBrushDrawType::Image;
		Brush.ImageSize = FVector2D(160.0, 200.0);
		if (Pictures[i])
		{
			Brush.SetResourceObject(Pictures[i]);
		}
	}
}

const FSlateBrush* ALighters::GetPicture(int32 Index) const
{
	return Pictures.IsValidIndex(Index) && Pictures[Index] ? &PictureBrushes[Index] : nullptr;
}

FVector2D ALighters::SpotOnMap(int32 Index) const
{
	const LighterModel::FSpot& Spot = LighterModel::Spots()[Index];
	const FVector2D At(Spot.X, Spot.Y);
	return Spot.Where == LighterModel::EWhere::Keleti ? AKeletiStation::ToCity(At) : At;
}

void ALighters::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	PlaceTimer -= DeltaSeconds;
	if (PlaceTimer <= 0.f && Done.Contains(false))
	{
		PlaceTimer = Lighters::PlaceEvery;
		PlaceLighters();
	}

	// After the last one: the golden lighter in your pocket, lit, though
	// you never bought it, and then a telephone.
	const double Now = GetWorld()->GetTimeSeconds();
	if (LuciferAt >= 0.0 && Now >= LuciferAt)
	{
		LuciferAt = -1.0;
		if (APestiHUD* Hud = APestiHUD::Get(this))
		{
			const LighterModel::FLighter& Golden = LighterModel::Golden();
			Hud->Announce(TEXT("LUCIFER"), TEXT("Megvan mind a harminc   \u00b7   All thirty found"), 9.f);
			Hud->Say(FString(), Golden.LineHu, Golden.LineEn, 9.f);
		}
	}
	if (RingAt >= 0.0 && Now >= RingAt)
	{
		RingAt = -1.0;
		if (APestiHUD* Hud = APestiHUD::Get(this))
		{
			Hud->Say(FString(), TEXT("Valahol megcs\u00f6rren egy telefon."), TEXT("Somewhere, a telephone starts to ring."), 7.f);
		}
	}

	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	APawn* Player = PC ? PC->GetPawn() : nullptr;
	if (!Player)
	{
		if (bCabinetOpen)
		{
			OpenCabinet(false);
		}
		return;
	}

	if (PC->WasInputKeyJustPressed(EKeys::L) || (bCabinetOpen
		&& (PC->WasInputKeyJustPressed(EKeys::Q) || PC->WasInputKeyJustPressed(EKeys::BackSpace) || PC->WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Right))))
	{
		OpenCabinet(!bCabinetOpen);
	}
	if (bCabinetOpen)
	{
		TickCabinet();
		return;
	}

	Glint(DeltaSeconds, Player->GetActorLocation());

	// Walk up to one and it is yours.
	const APestiPerson* Person = Cast<APestiPerson>(Player);
	if (Person && !Person->IsRiding() && !Person->IsHidden())
	{
		const FVector Feet = Person->GetActorLocation();
		for (int32 i = 0; i < Holders.Num(); ++i)
		{
			if (Holders[i])
			{
				const FVector At = Holders[i]->GetComponentLocation();
				if (FVector::Dist2D(At, Feet) < Lighters::PickUpRadius && FMath::Abs(At.Z - Feet.Z) < Lighters::PickUpHeight)
				{
					PickUp(i);
					break;
				}
			}
		}
	}

	TickTip(Player);
}

void ALighters::PlaceLighters()
{
	APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this);
	if (!Game)
	{
		return;
	}
	for (int32 i = 0; i < Done.Num(); ++i)
	{
		if (Done[i])
		{
			continue;
		}
		FVector At;
		if (Game->HasFoundLighter(i))
		{
			Done[i] = true;
		}
		else if (FindRestingPoint(i, At))
		{
			Build(i, At);
			Done[i] = true;
		}
	}
}

bool ALighters::FindRestingPoint(int32 Index, FVector& Out) const
{
	const LighterModel::FSpot& Spot = LighterModel::Spots()[Index];
	const FVector2D At = SpotOnMap(Index);

	// On a Keleti platform: the station knows its own floor, under its roof.
	if (Spot.Where == LighterModel::EWhere::Keleti)
	{
		TActorIterator<AKeletiStation> Station(GetWorld());
		if (!Station)
		{
			return false;
		}
		Out = FVector(At, Station->GetActorLocation().Z + Spot.Height);
		return true;
	}

	// Every solid surface straight down, from the top: the lowest is the
	// ground, and anything just over it is something to stand on (a
	// column's plinth, a café table).
	TArray<double> Surfaces;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(LighterGround), false, this);
	const FCollisionObjectQueryParams Solid(ECC_WorldStatic);
	FVector From(At, 10000.0);
	const FVector To(At, -10000.0);
	for (int32 i = 0; i < Lighters::MaxSurfaces; ++i)
	{
		FHitResult Hit;
		if (!GetWorld()->LineTraceSingleByObjectType(Hit, From, To, Solid, Params))
		{
			break;
		}
		if (Hit.bStartPenetrating || Hit.Distance < 1.0)
		{
			// Started inside it: look past it.
			Params.AddIgnoredComponent(Hit.GetComponent());
			continue;
		}
		Surfaces.Add(Hit.ImpactPoint.Z);
		From.Z = Hit.ImpactPoint.Z - 1.0;
	}
	if (Surfaces.Num() == 0)
	{
		return false;
	}
	const double Ground = Surfaces.Last();
	double Z = Ground + Spot.Height;
	if (Spot.Where == LighterModel::EWhere::Column || Spot.Where == LighterModel::EWhere::Cafe)
	{
		for (const double Surface : Surfaces)
		{
			if (Surface <= Ground + Lighters::StandOnHeight)
			{
				Z = Surface;
				break;
			}
		}
	}
	Out = FVector(At, Z);
	return true;
}

void ALighters::Build(int32 Index, const FVector& At)
{
	const LighterModel::FLighter& Lighter = LighterModel::All()[Index];
	const LighterModel::FSpot& Spot = LighterModel::Spots()[Index];

	USceneComponent* Holder = NewObject<USceneComponent>(this, NAME_None, RF_Transient);
	Holder->SetupAttachment(RootComponent);
	Holder->SetWorldLocationAndRotation(At, FRotator(0.0, Spot.Yaw, 0.0));
	Holder->SetWorldScale3D(FVector(Lighters::Scale));
	Holder->RegisterComponent();
	Holders[Index] = Holder;

	TArray<LighterModel::FPart> Parts;
	LighterModel::Build(Lighter.Kind, Lighter.Paint, Parts);
	for (const LighterModel::FPart& Part : Parts)
	{
		if (!Shapes.IsValidIndex(Part.Shape) || !Shapes[Part.Shape])
		{
			continue;
		}
		UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(this, NAME_None, RF_Transient);
		Mesh->SetupAttachment(Holder);
		Mesh->SetStaticMesh(Shapes[Part.Shape]);
		Mesh->SetRelativeLocationAndRotation(Part.At, FRotator(Part.Pitch, Part.Yaw, Part.Roll));
		Mesh->SetRelativeScale3D(Part.Size / 100.0);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetCanEverAffectNavigation(false);
		Mesh->SetCullDistance(Lighters::DrawDistance);
		if (BaseMaterial)
		{
			const FString Key = Part.Color.ToString();
			TObjectPtr<UMaterialInstanceDynamic>* Found = Paints.Find(Key);
			if (!Found)
			{
				UMaterialInstanceDynamic* Paint = UMaterialInstanceDynamic::Create(BaseMaterial, this);
				Paint->SetVectorParameterValue(TEXT("Color"), Part.Color);
				Found = &Paints.Add(Key, Paint);
			}
			Mesh->SetMaterial(0, *Found);
		}
		Mesh->RegisterComponent();
	}

	// The glint: a little star of light over it, an upright cross and two
	// short slanting arms, in one plane that turns to the camera.
	USceneComponent* Star = NewObject<USceneComponent>(this, NAME_None, RF_Transient);
	Star->SetupAttachment(RootComponent);
	Star->SetWorldLocation(At + FVector(0.0, 0.0, Lighters::GlintOver));
	Star->RegisterComponent();
	Glints[Index] = Star;
	const struct { double Length; double Roll; } Arms[] = {
		{ Lighters::GlintArm, 0.0 }, { Lighters::GlintArm, 90.0 }, { Lighters::GlintShortArm, 45.0 }, { Lighters::GlintShortArm, -45.0 } };
	for (const auto& Arm : Arms)
	{
		UStaticMeshComponent* Ray = NewObject<UStaticMeshComponent>(this, NAME_None, RF_Transient);
		Ray->SetupAttachment(Star);
		Ray->SetStaticMesh(Shapes[LighterModel::Cube]);
		Ray->SetRelativeRotation(FRotator(0.0, 0.0, Arm.Roll));
		Ray->SetRelativeScale3D(FVector(Lighters::GlintThickness, Arm.Length, Lighters::GlintThickness) / 100.0);
		Ray->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Ray->SetCanEverAffectNavigation(false);
		Ray->SetCastShadow(false);
		Ray->SetCullDistance(Lighters::GlintDistance);
		if (GlintPaint)
		{
			Ray->SetMaterial(0, GlintPaint);
		}
		Ray->RegisterComponent();
	}
}

void ALighters::DropNear(const UWorld* World, const FVector2D& Middle, double Radius, double GroundZ)
{
	for (TActorIterator<ALighters> It(World); It; ++It)
	{
		for (int32 i = 0; i < It->Holders.Num(); ++i)
		{
			USceneComponent* Holder = It->Holders[i];
			if (!Holder)
			{
				continue;
			}
			const FVector At = Holder->GetComponentLocation();
			if (At.Z > GroundZ + 2.0 && FVector2D::Distance(FVector2D(At), Middle) < Radius)
			{
				const FVector Fallen(At.X, At.Y, GroundZ + 1.0);
				Holder->SetWorldLocation(Fallen);
				if (It->Glints.IsValidIndex(i) && It->Glints[i])
				{
					It->Glints[i]->SetWorldLocation(Fallen + FVector(0.0, 0.0, Lighters::GlintOver));
				}
			}
		}
	}
}

void ALighters::Glint(float DeltaSeconds, const FVector& PlayerAt)
{
	GlintTime += DeltaSeconds;
	const float Pulse = 0.5f + 0.5f * FMath::Sin(GlintTime * 3.f);
	if (GlintPaint)
	{
		FLinearColor Color = Lighters::GlintColor;
		Color.A = 0.3f + 0.7f * Pulse * Pulse;
		GlintPaint->SetVectorParameterValue(TEXT("TintColorAndOpacity"), Color);
	}

	const APlayerController* PC = GetWorld()->GetFirstPlayerController();
	const FVector Eye = PC && PC->PlayerCameraManager ? PC->PlayerCameraManager->GetCameraLocation() : PlayerAt;
	int32 Nearest = INDEX_NONE;
	double NearestDistance = Lighters::LightDistance;
	for (int32 i = 0; i < Glints.Num(); ++i)
	{
		USceneComponent* Star = Glints[i];
		if (!Star)
		{
			continue;
		}
		const FVector At = Star->GetComponentLocation();
		const double Distance = FVector::Dist(At, Eye);
		if (Distance > Lighters::GlintDistance)
		{
			continue;
		}
		// Face the camera, and twinkle: it turns slowly and breathes.
		const double Yaw = (Eye - At).Rotation().Yaw;
		Star->SetWorldRotation(FRotator(0.0, Yaw, GlintTime * 40.0));
		Star->SetWorldScale3D(FVector(0.7 + 0.5 * Pulse));
		const double FromPlayer = FVector::Dist(At, PlayerAt);
		if (FromPlayer < NearestDistance)
		{
			Nearest = i;
			NearestDistance = FromPlayer;
		}
	}

	GlintLight->SetVisibility(Nearest != INDEX_NONE);
	if (Nearest != INDEX_NONE)
	{
		GlintLight->SetWorldLocation(Glints[Nearest]->GetComponentLocation() + FVector(0.0, 0.0, 10.0));
		GlintLight->SetIntensity(Lighters::LightIntensity * (0.3f + 0.7f * Pulse));
	}
}

void ALighters::PickUp(int32 Index)
{
	APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this);
	if (!Game)
	{
		return;
	}
	for (USceneComponent* Gone : { Holders[Index].Get(), Glints[Index].Get() })
	{
		if (!Gone)
		{
			continue;
		}
		TArray<USceneComponent*> Attached;
		Gone->GetChildrenComponents(true, Attached);
		for (USceneComponent* Child : Attached)
		{
			Child->DestroyComponent();
		}
		Gone->DestroyComponent();
	}
	Holders[Index] = nullptr;
	Glints[Index] = nullptr;

	Game->AddFoundLighter(Index);
	Selected = Index;
	const int32 Found = Game->GetLightersFound();
	const int32 Total = LighterModel::All().Num();
	const LighterModel::FLighter& Lighter = LighterModel::All()[Index];
	APestiHUD* Hud = APestiHUD::Get(this);
	if (Hud)
	{
		Hud->Announce(FString::Printf(TEXT("\u00d6NGY\u00daJT\u00d3  %d / %d"), Found, Total),
			FString::Printf(TEXT("%s   \u00b7   %s   \u00b7   L: gy\u0171jtem\u00e9ny (collection)"), Lighter.NameHu, Lighter.NameEn), 6.f);
	}

	// The rewards.
	if (Found < Total && Found % Lighters::RewardEvery == 0)
	{
		Game->AddMoney(Lighters::RewardMoney);
		APestiHUD::Show(this, Lighters::RewardLine, 10.f, Lighters::Yellow,
			FString::Printf(TEXT("Jutalom %d \u00f6ngy\u00fajt\u00f3\u00e9rt: +%s  (Reward for %d lighters)"),
				Found, *APestiAlvilagGameModeBase::FormatMoney(Lighters::RewardMoney), Found));
	}
	else if (Found == Total)
	{
		// The golden one, once the find itself has been read.
		LuciferAt = GetWorld()->GetTimeSeconds() + Lighters::LuciferAfter;
		RingAt = GetWorld()->GetTimeSeconds() + Lighters::RingAfter;
	}
}

void ALighters::OpenCabinet(bool bOpen)
{
	if (bOpen == bCabinetOpen)
	{
		return;
	}
	bCabinetOpen = bOpen;
	if (APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
	{
		PC->SetIgnoreMoveInput(bOpen);
		PC->SetIgnoreLookInput(bOpen);
	}
	if (APestiHUD* Hud = APestiHUD::Get(this))
	{
		Hud->SetCabinet(bOpen ? this : nullptr);
	}
}

void ALighters::TickCabinet()
{
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	const int32 Count = LighterModel::All().Num();
	const int32 Rows = Count / Lighters::Columns;
	const int32 Row = Selected / Lighters::Columns;
	const int32 Column = Selected % Lighters::Columns;
	auto Pressed = [PC](const FKey& A, const FKey& B, const FKey& Pad)
	{
		return PC->WasInputKeyJustPressed(A) || PC->WasInputKeyJustPressed(B) || PC->WasInputKeyJustPressed(Pad);
	};
	if (Pressed(EKeys::Left, EKeys::A, EKeys::Gamepad_DPad_Left))
	{
		Selected = Row * Lighters::Columns + (Column + Lighters::Columns - 1) % Lighters::Columns;
	}
	if (Pressed(EKeys::Right, EKeys::D, EKeys::Gamepad_DPad_Right))
	{
		Selected = Row * Lighters::Columns + (Column + 1) % Lighters::Columns;
	}
	if (Pressed(EKeys::Up, EKeys::W, EKeys::Gamepad_DPad_Up))
	{
		Selected = ((Row + Rows - 1) % Rows) * Lighters::Columns + Column;
	}
	if (Pressed(EKeys::Down, EKeys::S, EKeys::Gamepad_DPad_Down))
	{
		Selected = ((Row + 1) % Rows) * Lighters::Columns + Column;
	}
}

void ALighters::TickTip(APawn* Player)
{
	APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this);
	if (!Game || !Game->IsMissionDone(Lighters::TipAfterMission))
	{
		return;
	}
	if (!IsValid(Pista))
	{
		Pista = AFleaMarket::FindKeeper(GetWorld(), Lighters::PistaName);
	}
	const APestiPerson* Person = Cast<APestiPerson>(Player);
	if (!Pista || Pista->IsHidden() || !Person || Person->IsRiding() || Person->IsCarrying() || Person->IsHidden()
		|| FVector::Dist2D(Person->GetActorLocation(), Pista->GetActorLocation()) > Lighters::TalkRadius)
	{
		return;
	}
	APestiHUD::Show(this, Lighters::HintLine, 0.f, Lighters::Yellow,
		FString::Printf(TEXT("E: Pista b\u00e1csi tippje egy \u00f6ngy\u00fajt\u00f3r\u00f3l, %s  (a tip about a lighter)"),
			*APestiAlvilagGameModeBase::FormatMoney(Lighters::TipPrice)));
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC || !PC->WasInputKeyJustPressed(EKeys::E))
	{
		return;
	}
	Pista->FaceTowards(Person->GetActorLocation());
	APestiHUD* Hud = APestiHUD::Get(this);
	if (!Hud)
	{
		return;
	}

	// The nearest one still lying out there.
	int32 Nearest = INDEX_NONE;
	double NearestDistance = TNumericLimits<double>::Max();
	for (int32 i = 0; i < LighterModel::All().Num(); ++i)
	{
		if (!Game->HasFoundLighter(i))
		{
			const double Distance = FVector2D::Distance(SpotOnMap(i), FVector2D(Person->GetActorLocation()));
			if (Distance < NearestDistance)
			{
				Nearest = i;
				NearestDistance = Distance;
			}
		}
	}
	if (Nearest == INDEX_NONE)
	{
		Hud->Say(Lighters::PistaName, TEXT("Nincs t\u00f6bb, fiam. Mind a harmincat megtal\u00e1ltad."),
			TEXT("There are none left, son. You found all thirty."), 7.f);
		return;
	}
	if (Game->GetMoney() < Lighters::TipPrice)
	{
		Hud->Say(Lighters::PistaName, TEXT("H\u00fasz fill\u00e9r, fiam. Annyi sincs n\u00e1lad?"),
			TEXT("Twenty fill\u00e9r, son. Haven't you even got that?"), 6.f);
		return;
	}
	Game->AddMoney(-Lighters::TipPrice);
	const LighterModel::FSpot& Spot = LighterModel::Spots()[Nearest];
	FString Hu = FString(TEXT("Azt hallottam, van egy \u00f6ngy\u00fajt\u00f3 ")) + LighterModel::WhereHu(Spot.Where);
	FString En = FString(TEXT("I hear there's a lighter ")) + LighterModel::WhereEn(Spot.Where);
	if (Spot.Near)
	{
		Hu += FString::Printf(TEXT(", %s %s k\u00f6zel\u00e9ben"), Lighters::Article(Spot.Near), Spot.Near);
		En += FString::Printf(TEXT(", near %s"), Spot.Near);
	}
	Hud->Say(Lighters::PistaName, Hu + TEXT("."), En + TEXT("."), 10.f);
}
