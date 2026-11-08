// Fill out your copyright notice in the Description page of Project Settings.

#include "LaciYard.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PersonModel.h"
#include "LaciOrders.h"
#include "LaciYardModel.h"
#include "PestiAlvilagGameModeBase.h"
#include "PestiCar.h"
#include "PestiHud.h"
#include "PoliceCar.h"
#include "StoryPerson.h"
#include "UObject/ConstructorHelpers.h"

namespace LaciYardPlace
{
	// Things in the yard are not drawn from further off than this (cm).
	constexpr float DrawDistance = 20000.f;

	// The garage. A car counts as over the pit inside these (yard frame, cm),
	// once it has stood still (slower than StillSpeed cm/s) for StillTime s.
	constexpr double BayX0 = LaciYardModel::PitX0 - 80.0;
	constexpr double BayX1 = LaciYardModel::PitX1 + 80.0;
	constexpr double BayHalf = 150.0;
	constexpr float StillSpeed = 40.f;
	constexpr float StillTime = 0.8f;
	// What the work costs (fillér), and the mission that opens the garage.
	constexpr int32 Price = 300;
	const FName OpenedBy(TEXT("ElsoAuto"));
	// A police car chasing the player this close (cm) and Laci won't touch the car.
	constexpr double PoliceTooNear = 3000.0;
	// The fade to black (s), how long the screen stays black, the fade back.
	constexpr float FadeOut = 0.6f;
	constexpr double BlackFor = 1.4;
	constexpr float FadeIn = 0.8f;
	// How long Laci's words stay up (s).
	constexpr float SaySeconds = 6.f;
}

ALaciYard::ALaciYard()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.1f;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicShapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	BaseMaterial = BasicShapeMaterial.Object;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMesh(TEXT("/Engine/BasicShapes/Cone.Cone"));
	Shapes = { CubeMesh.Object, CylinderMesh.Object, SphereMesh.Object, ConeMesh.Object };
}

FVector ALaciYard::ToCity(const FVector& Local, double Ground)
{
	const FRotator Turn(0.0, LaciYardModel::CityYaw, 0.0);
	return FVector(LaciYardModel::CityAt, Ground) + Turn.RotateVector(Local);
}

AStoryPerson* ALaciYard::GetLaci() const
{
	return Laci;
}

void ALaciYard::BeginPlay()
{
	Super::BeginPlay();

	TArray<MarketModel::FShape> Pieces;
	LaciYardModel::Build(Pieces);
	for (const MarketModel::FShape& Piece : Pieces)
	{
		AddShape(Piece.Shape, Piece.Color, Piece.At, Piece.Size, FRotator(Piece.Pitch, Piece.Yaw, Piece.Roll), Piece.bCollide);
	}

	// The painted board over the gate, the lettering on the wall.
	TArray<LaciYardModel::FSign> Signs;
	LaciYardModel::Signs(Signs);
	for (const LaciYardModel::FSign& Sign : Signs)
	{
		UTextRenderComponent* Text = NewObject<UTextRenderComponent>(this, NAME_None, RF_Transient);
		Text->SetupAttachment(RootComponent);
		Text->SetRelativeLocationAndRotation(Sign.At, FRotator(0.0, Sign.Yaw, 0.0));
		Text->SetText(FText::FromString(Sign.Text));
		Text->SetHorizontalAlignment(EHTA_Center);
		Text->SetVerticalAlignment(EVRTA_TextCenter);
		Text->SetWorldSize(Sign.Height);
		Text->SetTextRenderColor(Sign.Color.ToFColor(true));
		Text->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Text->RegisterComponent();
	}

	// Laci, by the workshop.
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	const FVector MechanicAt = GetActorTransform().TransformPosition(LaciYardModel::Spots::Laci) + FVector(0.0, 0.0, PersonModel::MiddleHeight);
	const FRotator MechanicTurn(0.0, GetActorRotation().Yaw + LaciYardModel::Spots::LaciYaw, 0.0);
	Laci = GetWorld()->SpawnActor<AStoryPerson>(AStoryPerson::StaticClass(), MechanicAt, MechanicTurn, Params);
	if (Laci)
	{
		Laci->SetOutfit(LaciYardModel::Laci());
	}
}

void ALaciYard::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	// The garage on the maps, once it is open (after mission 7).
	const APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this);
	APestiHUD* Hud = APestiHUD::Get(this);
	if (Game && Hud && Game->IsMissionDone(LaciYardPlace::OpenedBy))
	{
		Hud->MarkPlace(GetActorLocation(), TEXT("G"));
	}
	TickLeftovers();
	TickGarage(DeltaSeconds);
}

void ALaciYard::LaciSays(const FString& Hu, const FString& En) const
{
	if (APestiHUD* Hud = APestiHUD::Get(this))
	{
		Hud->Say(TEXT("Laci"), Hu, En, LaciYardPlace::SaySeconds);
	}
}

void ALaciYard::TickLeftovers()
{
	const APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this);
	if (!Game || !Game->IsMissionDone(LaciYardPlace::OpenedBy))
	{
		return;
	}
	const TArray<int32>& Cars = Game->GetLaciCars();
	const int32 Wanted = 1 + Cars.Num();
	if (LeftoversShown >= Wanted)
	{
		return;
	}
	TArray<MarketModel::FShape> Pieces;
	for (; LeftoversShown < Wanted; ++LeftoversShown)
	{
		LaciYardModel::FStrippedCar Stripped;
		if (LeftoversShown == 0)
		{
			// The doctor's Steyr from mission 7.
			const CarModel::FStyle Steyr = CarModel::SteyrStyle();
			Stripped.Paint = Steyr.PaintColor;
			Stripped.Rim = Steyr.SpokeColor;
			Stripped.bBig = true;
		}
		else
		{
			Stripped = ALaciOrders::Leftovers(Cars[LeftoversShown - 1]);
		}
		LaciYardModel::BuildLeftovers(Pieces, LeftoversShown, Stripped);
		LaciYardModel::BuildPlate(Pieces, LeftoversShown);
	}
	for (const MarketModel::FShape& Piece : Pieces)
	{
		AddShape(Piece.Shape, Piece.Color, Piece.At, Piece.Size, FRotator(Piece.Pitch, Piece.Yaw, Piece.Roll), Piece.bCollide);
	}
}

void ALaciYard::TickGarage(float DeltaSeconds)
{
	const double Now = GetWorld()->GetTimeSeconds();
	APlayerController* PC = GetWorld()->GetFirstPlayerController();

	// At work: the car is mended while the screen is black, then it clears.
	if (InWork || DoneAt > 0.0 || ClearAt > 0.0)
	{
		if (DoneAt > 0.0 && Now >= DoneAt)
		{
			DoneAt = -1.0;
			FinishWork();
		}
		if (ClearAt > 0.0 && Now >= ClearAt)
		{
			ClearAt = -1.0;
			if (PC)
			{
				if (PC->PlayerCameraManager)
				{
					PC->PlayerCameraManager->StartCameraFade(1.f, 0.f, LaciYardPlace::FadeIn, FLinearColor::Black, false, false);
				}
				PC->SetIgnoreMoveInput(false);
			}
			LaciSays(TEXT("H\u00e1rom peng\u0151, \u00e9s olyan, mint \u00fajkor\u00e1ban. A hekusok sem ismernek r\u00e1."),
				TEXT("Three peng\u0151, and it's good as new. The coppers won't know it either."));
		}
		return;
	}

	APestiCar* Car = PC ? Cast<APestiCar>(PC->GetPawn()) : nullptr;
	// A car brought on Laci's order is taken in by ALaciOrders, not mended.
	TActorIterator<ALaciOrders> Orders(GetWorld());
	if (Car && Orders && Orders->GetOrderCar() == Car)
	{
		Car = nullptr;
	}
	if (!Car)
	{
		StoodFor = 0.f;
		bWaitForLeave = false;
		return;
	}
	const FVector Local = GetActorTransform().InverseTransformPosition(Car->GetActorLocation());
	const bool bOverPit = Local.X > LaciYardPlace::BayX0 && Local.X < LaciYardPlace::BayX1
		&& FMath::Abs(Local.Y - LaciYardModel::GateMid) < LaciYardPlace::BayHalf;
	if (!bOverPit)
	{
		StoodFor = 0.f;
		bWaitForLeave = false;
		return;
	}
	if (bWaitForLeave)
	{
		return;
	}
	StoodFor = Car->GetVelocity().Size() < LaciYardPlace::StillSpeed ? StoodFor + DeltaSeconds : 0.f;
	if (StoodFor < LaciYardPlace::StillTime)
	{
		return;
	}
	bWaitForLeave = true;

	APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this);
	if (!Game)
	{
		return;
	}
	if (!Game->IsMissionDone(LaciYardPlace::OpenedBy))
	{
		LaciSays(TEXT("Ide ne \u00e1llj, fiam, ez nem t\u00e9r. Nem jav\u00edtok ak\u00e1rkinek."),
			TEXT("Don't park here, son, this isn't a square. I don't mend just anybody's car."));
		return;
	}
	if (!Car->IsDamaged() && Game->GetWantedLevel() == 0)
	{
		LaciSays(TEXT("Ennek semmi baja. Ha \u00f6sszet\u00f6r\u00f6d, hozd vissza!"),
			TEXT("Nothing wrong with this one. Bring it back once you've smashed it up!"));
		return;
	}
	for (TActorIterator<APoliceCar> It(GetWorld()); It; ++It)
	{
		if (It->IsChasing() && FVector::Dist(It->GetActorLocation(), Car->GetActorLocation()) < LaciYardPlace::PoliceTooNear)
		{
			LaciSays(TEXT("Megbolondult\u00e1l? A zsaruk orra el\u0151tt? T\u0171nj el, azt\u00e1n gyere vissza!"),
				TEXT("Are you mad? Right under the coppers' noses? Lose them, then come back!"));
			return;
		}
	}
	if (Game->GetMoney() < LaciYardPlace::Price)
	{
		LaciSays(TEXT("H\u00e1rom peng\u0151, fiam. Ha nincs p\u00e9nz, nincs fest\u00e9k."),
			TEXT("Three peng\u0151, son. No money, no paint."));
		return;
	}
	StartWork(Car);
}

void ALaciYard::StartWork(APestiCar* Car)
{
	InWork = Car;
	Car->ForwardSpeed = 0.f;
	Car->SideSpeed = 0.f;
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		PC->SetIgnoreMoveInput(true);
		if (PC->PlayerCameraManager)
		{
			// Stays black until the fade back starts.
			PC->PlayerCameraManager->StartCameraFade(0.f, 1.f, LaciYardPlace::FadeOut, FLinearColor::Black, false, true);
		}
	}
	const double Now = GetWorld()->GetTimeSeconds();
	DoneAt = Now + LaciYardPlace::FadeOut + 0.1;
	ClearAt = Now + LaciYardPlace::FadeOut + LaciYardPlace::BlackFor;
}

void ALaciYard::FinishWork()
{
	APestiCar* Car = InWork.Get();
	InWork = nullptr;
	APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this);
	if (!Car || !Game)
	{
		return;
	}
	// A colour from Laci's tins, as far from the old one as he has.
	const FLinearColor Old = Car->GetPaint();
	int32 Best = 0;
	float BestApart = -1.f;
	for (int32 i = 0; i < LaciYardModel::NumPaints; ++i)
	{
		const FLinearColor& Paint = LaciYardModel::Paints[i];
		const float Apart = FMath::Abs(Paint.R - Old.R) + FMath::Abs(Paint.G - Old.G) + FMath::Abs(Paint.B - Old.B) + FMath::FRandRange(0.f, 0.15f);
		if (Apart > BestApart)
		{
			BestApart = Apart;
			Best = i;
		}
	}
	Car->Repaint(LaciYardModel::Paints[Best]);
	Game->AddMoney(-LaciYardPlace::Price);
	if (Game->GetWantedLevel() > 0)
	{
		Game->LoseThePolice(TEXT("New paint: the police lost you!"));
	}
}

void ALaciYard::AddShape(int32 Shape, const FLinearColor& Color, const FVector& At, const FVector& Size, const FRotator& Turn, bool bCollide)
{
	if (!Shapes.IsValidIndex(Shape) || !Shapes[Shape])
	{
		return;
	}
	// One batch per shape, colour and collision; the instances are placed in the yard's frame.
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
		Instances->SetCullDistances(0, LaciYardPlace::DrawDistance);
		Instances->SetCanEverAffectNavigation(false);
		Instances->RegisterComponent();
		BatchIndex.Add(Key, Batches.Add(Instances));
	}
	Instances->AddInstance(FTransform(Turn, At, Size / 100.0), false);
}
