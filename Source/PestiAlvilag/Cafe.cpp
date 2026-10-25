// Fill out your copyright notice in the Description page of Project Settings.

#include "Cafe.h"

#include "Components/SceneComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PersonModel.h"
#include "UObject/ConstructorHelpers.h"

namespace Cafe
{
	enum EShape : int32 { Cube, Cylinder, Sphere, Cone, NumShapes };

	const FLinearColor ShopGreen(0.02f, 0.10f, 0.05f);
	const FLinearColor Gilt(0.75f, 0.52f, 0.12f);
	const FLinearColor LitWindow(0.95f, 0.70f, 0.32f);
	const FLinearColor DoorWood(0.12f, 0.07f, 0.04f);
	const FLinearColor AwningRed(0.45f, 0.05f, 0.04f);
	const FLinearColor AwningCream(0.85f, 0.80f, 0.66f);
	const FLinearColor Iron(0.03f, 0.03f, 0.03f);
	const FLinearColor Marble(0.82f, 0.80f, 0.76f);
	const FLinearColor Bentwood(0.22f, 0.11f, 0.05f);
	const FLinearColor China(0.95f, 0.94f, 0.90f);
	const FLinearColor Planter(0.25f, 0.16f, 0.08f);
	const FLinearColor Box(0.06f, 0.16f, 0.05f);

	// Along the wall from the home door (Y), out to the street (X), up (Z).
	constexpr double FrontFrom = 200.0;
	constexpr double FrontTo = 1100.0;
	constexpr double FrontMid = (FrontFrom + FrontTo) * 0.5;
	constexpr double FrontHeight = 330.0;
	constexpr double SignLow = 345.0;
	constexpr double SignHigh = 395.0;

	// The awning slopes from the wall above the windows out over the pavement.
	constexpr double AwningOut = 230.0;
	constexpr double AwningDrop = 75.0;
	constexpr double StripeWidth = 50.0;
	constexpr double ValanceDepth = 30.0;

	// The terrace: a row of tables on the pavement, chairs either side.
	constexpr double TableX = 340.0;
	const double TableY[] = { 300.0, 540.0, 780.0, 1020.0 };
	constexpr double ChairOff = 62.0;
	constexpr double ParasolTop = 235.0;

	// Who sits where: table, side (-1 the door side, +1 the far side), coat.
	struct FSeat
	{
		int32 Table;
		int32 Side;
		FLinearColor Coat;
	};
	const FSeat Seats[] = {
		{ 0, 1, FLinearColor(0.18f, 0.16f, 0.12f) },
		{ 1, -1, FLinearColor(0.06f, 0.08f, 0.14f) },
		{ 1, 1, FLinearColor(0.30f, 0.12f, 0.10f) },
		{ 3, -1, FLinearColor(0.12f, 0.12f, 0.12f) },
	};

	// Seated: hips on the chair (its seat is 48 cm up), arms reaching
	// forward onto the marble.
	constexpr double SeatedMiddle = PersonModel::MiddleHeight - PersonKit::SeatDrop + 2.0;
	constexpr float ArmsOnTable = 67.f;
	constexpr float SipLift = 15.f;
	constexpr float SipEvery = 9.f;
	constexpr float SipLength = 2.f;

	// Where the waiter comes out, and where he stands at a table.
	const FVector WaiterDoor(150.0, 560.0, 0.0);
	constexpr double WaiterAtTable = 120.0;
	const FLinearColor WaiterJacket(0.85f, 0.84f, 0.80f);
	constexpr float WaiterSpeed = 140.f;
	constexpr float WaiterWait = 5.f;
	constexpr float WaiterTurn = 360.f;
	constexpr float WaiterStride = 110.f;
	constexpr float WaiterSwing = 22.f;
	constexpr float TrayArm = 70.f;
	constexpr double Arrived = 15.0;

	FVector OnGround(const UWorld* World, const FVector& At, const AActor* Ignore)
	{
		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(CafeGround), false, Ignore);
		if (World->LineTraceSingleByChannel(Hit, At + FVector(0.0, 0.0, 150.0), At - FVector(0.0, 0.0, 500.0), ECC_Visibility, Params))
		{
			return FVector(At.X, At.Y, Hit.ImpactPoint.Z);
		}
		return At;
	}
}

ACafe::ACafe()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	BaseMaterial = Material.Object;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMesh(TEXT("/Engine/BasicShapes/Cone.Cone"));
	ShapeMeshes = { CubeMesh.Object, CylinderMesh.Object, SphereMesh.Object, ConeMesh.Object };

	// The gilt name over the awning, facing the street.
	SignText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("SignText"));
	SignText->SetupAttachment(RootComponent);
	SignText->SetRelativeLocation(FVector(16.0, Cafe::FrontMid, (Cafe::SignLow + Cafe::SignHigh) * 0.5));
	SignText->SetHorizontalAlignment(EHTA_Center);
	SignText->SetVerticalAlignment(EVRTA_TextCenter);
	SignText->SetWorldSize(38.f);
	SignText->SetTextRenderColor(FColor(230, 180, 70));
	SignText->SetText(FText::FromString(TEXT("KÁVÉHÁZ")));
}

void ACafe::Add(int32 Shape, const FLinearColor& Color, const FVector& At, const FVector& Scale, const FRotator& Turn, bool bCollide)
{
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
		Instances->SetStaticMesh(ShapeMeshes[Shape]);
		if (BaseMaterial)
		{
			UMaterialInstanceDynamic* Paint = UMaterialInstanceDynamic::Create(BaseMaterial, this);
			Paint->SetVectorParameterValue(TEXT("Color"), Color);
			Instances->SetMaterial(0, Paint);
		}
		Instances->SetCollisionProfileName(bCollide ? TEXT("BlockAll") : TEXT("NoCollision"));
		Instances->RegisterComponent();
		BatchIndex.Add(Key, Batches.Add(Instances));
	}
	Instances->AddInstance(FTransform(Turn, At, Scale), false);
}

void ACafe::BeginPlay()
{
	Super::BeginPlay();

	BuildFront();
	BuildTerrace();
	SeatGuests();
}

void ACafe::BuildFront()
{
	using namespace Cafe;
	const double Width = FrontTo - FrontFrom;

	// The dark green shop front with its lit windows and the door in.
	Add(Cube, ShopGreen, FVector(6.0, FrontMid, FrontHeight * 0.5), FVector(0.1, Width / 100.0, FrontHeight / 100.0), FRotator::ZeroRotator, true);
	struct FOpening { double Mid; double Width; bool bDoor; };
	const FOpening Openings[] = { { 350.0, 260.0, false }, { 560.0, 120.0, true }, { 770.0, 260.0, false }, { 990.0, 170.0, false } };
	for (const FOpening& Opening : Openings)
	{
		if (Opening.bDoor)
		{
			Add(Cube, DoorWood, FVector(12.0, Opening.Mid, 130.0), FVector(0.04, Opening.Width / 100.0, 2.6));
			Add(Cube, LitWindow, FVector(13.0, Opening.Mid, 200.0), FVector(0.03, Opening.Width / 100.0 - 0.3, 0.8));
		}
		else
		{
			Add(Cube, LitWindow, FVector(12.0, Opening.Mid, 180.0), FVector(0.04, Opening.Width / 100.0, 2.2));
			// A glazing bar across and a gilt line under the window.
			Add(Cube, ShopGreen, FVector(14.0, Opening.Mid, 230.0), FVector(0.03, Opening.Width / 100.0, 0.05));
			Add(Cube, Gilt, FVector(14.0, Opening.Mid, 66.0), FVector(0.03, Opening.Width / 100.0, 0.03));
		}
	}
	// Pilasters between the openings, and the cornice and sign board above.
	for (const double Y : { FrontFrom + 10.0, 480.0, 640.0, 900.0, FrontTo - 10.0 })
	{
		Add(Cube, ShopGreen, FVector(14.0, Y, FrontHeight * 0.5), FVector(0.12, 0.2, FrontHeight / 100.0));
	}
	Add(Cube, ShopGreen, FVector(12.0, FrontMid, SignLow + (SignHigh - SignLow) * 0.5), FVector(0.1, Width / 100.0, (SignHigh - SignLow) / 100.0));
	Add(Cube, Gilt, FVector(15.0, FrontMid, SignHigh + 3.0), FVector(0.06, Width / 100.0, 0.06));
	Add(Cube, Gilt, FVector(15.0, FrontMid, SignLow - 3.0), FVector(0.06, Width / 100.0, 0.06));

	// The striped awning, its scalloped edge, and the iron arms that hold it.
	const double Slope = FMath::RadiansToDegrees(FMath::Atan2(AwningDrop, AwningOut));
	const double Length = FMath::Sqrt(AwningOut * AwningOut + AwningDrop * AwningDrop);
	const int32 Stripes = FMath::RoundToInt(Width / StripeWidth);
	for (int32 i = 0; i < Stripes; ++i)
	{
		const double Y = FrontFrom + StripeWidth * (i + 0.5);
		const FLinearColor& Cloth = i % 2 ? AwningCream : AwningRed;
		Add(Cube, Cloth, FVector(AwningOut * 0.5 + 5.0, Y, FrontHeight - AwningDrop * 0.5), FVector(Length / 100.0, StripeWidth / 100.0, 0.02), FRotator(-Slope, 0.0, 0.0));
		Add(Cube, Cloth, FVector(AwningOut + 5.0, Y, FrontHeight - AwningDrop - ValanceDepth * 0.5), FVector(0.02, StripeWidth / 100.0, ValanceDepth / 100.0));
	}
	for (const double Y : { FrontFrom + 5.0, FrontTo - 5.0 })
	{
		Add(Cube, Iron, FVector(AwningOut * 0.5, Y, FrontHeight - AwningDrop - 20.0), FVector(Length / 100.0, 0.03, 0.03), FRotator(Slope * 0.5, 0.0, 0.0));
	}
}

void ACafe::BuildTerrace()
{
	using namespace Cafe;

	for (int32 t = 0; t < static_cast<int32>(UE_ARRAY_COUNT(TableY)); ++t)
	{
		const double Y = TableY[t];
		// A round marble top on an iron stem.
		Add(Cylinder, Iron, FVector(TableX, Y, 2.0), FVector(0.42, 0.42, 0.04), FRotator::ZeroRotator, true);
		Add(Cylinder, Iron, FVector(TableX, Y, 37.0), FVector(0.06, 0.06, 0.7), FRotator::ZeroRotator, true);
		Add(Cylinder, Marble, FVector(TableX, Y, 73.0), FVector(0.72, 0.72, 0.03), FRotator::ZeroRotator, true);
		// Cups and saucers.
		for (const double Side : { -1.0, 1.0 })
		{
			Add(Cylinder, China, FVector(TableX + 8.0 * Side, Y + 18.0 * Side, 75.0), FVector(0.14, 0.14, 0.01));
			Add(Cylinder, China, FVector(TableX + 8.0 * Side, Y + 18.0 * Side, 79.0), FVector(0.07, 0.07, 0.07));
		}

		// Bentwood chairs either side, facing the table.
		for (const double Side : { -1.0, 1.0 })
		{
			const double ChairY = Y + ChairOff * Side;
			const double BackY = Y + (ChairOff + 20.0) * Side;
			Add(Cylinder, Bentwood, FVector(TableX, ChairY, 46.0), FVector(0.4, 0.4, 0.04), FRotator::ZeroRotator, true);
			for (const FVector2D Leg : { FVector2D(-14.0, -14.0), FVector2D(14.0, -14.0), FVector2D(-14.0, 14.0), FVector2D(14.0, 14.0) })
			{
				Add(Cylinder, Bentwood, FVector(TableX + Leg.X, ChairY + Leg.Y, 22.0), FVector(0.03, 0.03, 0.44));
			}
			Add(Cube, Bentwood, FVector(TableX, BackY, 72.0), FVector(0.36, 0.03, 0.08));
			Add(Cube, Bentwood, FVector(TableX - 15.0, BackY, 62.0), FVector(0.03, 0.03, 0.34));
			Add(Cube, Bentwood, FVector(TableX + 15.0, BackY, 62.0), FVector(0.03, 0.03, 0.34));
		}

		// A parasol over each table, red and cream by turns.
		Add(Cylinder, Iron, FVector(TableX, Y, (73.0 + ParasolTop) * 0.5), FVector(0.03, 0.03, (ParasolTop - 73.0) / 100.0));
		Add(Cone, t % 2 ? AwningCream : AwningRed, FVector(TableX, Y, ParasolTop + 15.0), FVector(2.1, 2.1, 0.35));
		Add(Sphere, Gilt, FVector(TableX, Y, ParasolTop + 34.0), FVector(0.06, 0.06, 0.06));
	}

	// Planters at both ends of the terrace.
	for (const double Y : { FrontFrom - 30.0, FrontTo + 30.0 })
	{
		Add(Cube, Planter, FVector(TableX, Y, 22.0), FVector(0.9, 0.35, 0.44), FRotator::ZeroRotator, true);
		Add(Cube, Box, FVector(TableX, Y, 52.0), FVector(0.86, 0.32, 0.2));
	}
}

void ACafe::SeatGuests()
{
	using namespace Cafe;
	const FTransform Frame = GetActorTransform();
	const double FrameYaw = GetActorRotation().Yaw;

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	FRandomStream Random(1932);
	for (const FSeat& Seat : Seats)
	{
		const FVector Chair = Frame.TransformPosition(FVector(TableX, TableY[Seat.Table] + ChairOff * Seat.Side, 0.0));
		// The pavement is level here; a trace would land on the chair seat.
		const FVector Ground = Chair;
		// Facing the table: towards -Y from the far side, +Y from the near one.
		const double Yaw = FrameYaw + (Seat.Side > 0 ? -90.0 : 90.0);
		if (ACafeGuest* Guest = GetWorld()->SpawnActor<ACafeGuest>(ACafeGuest::StaticClass(), Ground + FVector(0.0, 0.0, SeatedMiddle), FRotator(0.0, Yaw, 0.0), Params))
		{
			PersonKit::FOutfit Clothes = PersonModel::RandomOutfit(Random);
			Clothes.bSeated = true;
			Clothes.Held = PersonKit::EHeld::None;
			Clothes.Coat = Seat.Coat;
			Guest->SetOutfit(Clothes);
		}
	}

	TArray<FVector> Stands;
	for (const double Y : TableY)
	{
		Stands.Add(OnGround(GetWorld(), Frame.TransformPosition(FVector(TableX + WaiterAtTable, Y, 0.0)), this));
	}
	const FVector Door = OnGround(GetWorld(), Frame.TransformPosition(WaiterDoor), this);
	if (ACafeWaiter* Waiter = GetWorld()->SpawnActor<ACafeWaiter>(ACafeWaiter::StaticClass(), Door + FVector(0.0, 0.0, 90.0), FRotator(0.0, FrameYaw, 0.0), Params))
	{
		Waiter->SetRounds(Door, Stands);
	}
}

ACafeGuest::ACafeGuest()
{
	// Sitting on a chair: no capsule in the way of the table and chairs.
	Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Outfit.bSeated = true;
}

void ACafeGuest::BeginPlay()
{
	Super::BeginPlay();

	// Hands on the table (the legs are made seated).
	PoseArms();
	// Not all at the same moment.
	SipClock = FMath::Fmod(static_cast<float>(GetActorLocation().X + GetActorLocation().Y) * 0.01f, Cafe::SipEvery);
	if (SipClock < 0.f)
	{
		SipClock += Cafe::SipEvery;
	}
}

void ACafeGuest::Tick(float DeltaSeconds)
{
	// Sitting still apart from a sip now and then, not walking like a story person.
	AActor::Tick(DeltaSeconds);

	SipClock += DeltaSeconds;
	if (SipClock > Cafe::SipEvery)
	{
		SipClock -= Cafe::SipEvery;
	}
	const float Sip = SipClock < Cafe::SipLength ? FMath::Sin(SipClock / Cafe::SipLength * PI) : 0.f;
	PoseArms();
	if (USceneComponent* RightArm = Limb(3))
	{
		RightArm->SetRelativeRotation(FRotator(Cafe::ArmsOnTable + Cafe::SipLift * Sip, 0.f, 0.f));
	}
}

void ACafeGuest::PoseArms()
{
	for (int32 i = 2; i < 4; ++i)
	{
		if (USceneComponent* Arm = Limb(i))
		{
			Arm->SetRelativeRotation(FRotator(Cafe::ArmsOnTable, 0.f, 0.f));
		}
	}
}

ACafeWaiter::ACafeWaiter()
{
	CoatColor = Cafe::WaiterJacket;
	// A white jacket and long apron, black trousers and tie, bare-headed.
	Outfit.Hat = PersonKit::EHat::None;
	Outfit.Top = PersonKit::ETop::Jacket;
	Outfit.bApron = true;
	Outfit.bMoustache = true;
	Outfit.Tie = FLinearColor(0.02f, 0.02f, 0.02f);
	Outfit.Trousers = FLinearColor(0.02f, 0.02f, 0.02f);
	WalkSpeed = Cafe::WaiterSpeed;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	Tray = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Tray"));
	Tray->SetupAttachment(Capsule);
	Tray->SetStaticMesh(CylinderMesh.Object);
	Tray->SetRelativeLocation(FVector(52.0, -25.0, 36.0));
	Tray->SetRelativeScale3D(FVector(0.4, 0.4, 0.015));
	Tray->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ACafeWaiter::BeginPlay()
{
	Super::BeginPlay();

	if (BaseMaterial)
	{
		UMaterialInstanceDynamic* Silver = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		Silver->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.6f, 0.6f, 0.62f));
		Tray->SetMaterial(0, Silver);
	}
}

void ACafeWaiter::SetRounds(const FVector& InDoor, const TArray<FVector>& InTables)
{
	Door = InDoor;
	Tables = InTables;
	Table = 0;
	bGoingToTable = true;
	Wait = Cafe::WaiterWait;
}

void ACafeWaiter::Tick(float DeltaSeconds)
{
	// His own rounds instead of following someone.
	AActor::Tick(DeltaSeconds);

	float Legs = 0.f;
	if (Wait > 0.f || Tables.IsEmpty())
	{
		Wait -= DeltaSeconds;
	}
	else
	{
		const FVector Goal = bGoingToTable ? Tables[Table % Tables.Num()] : Door;
		const FVector Here = GetActorLocation();
		const FVector2D ToGoal = FVector2D(Goal) - FVector2D(Here);
		const double Gap = ToGoal.Size();
		if (Gap < Cafe::Arrived)
		{
			// At the table he waits on the guests; at the door he fetches the next order.
			Wait = Cafe::WaiterWait;
			if (!bGoingToTable)
			{
				Table = (Table + 1) % Tables.Num();
			}
			bGoingToTable = !bGoingToTable;
		}
		else
		{
			FVector NextAt = Here + FVector(ToGoal.GetSafeNormal() * FMath::Min(static_cast<double>(WalkSpeed * DeltaSeconds), Gap), 0.0);
			NextAt.Z = Cafe::OnGround(GetWorld(), NextAt, this).Z + Capsule->GetUnscaledCapsuleHalfHeight();
			SetActorLocation(NextAt);
			const float WantYaw = static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(ToGoal.Y, ToGoal.X)));
			SetActorRotation(FRotator(0.f, FMath::FixedTurn(static_cast<float>(GetActorRotation().Yaw), WantYaw, Cafe::WaiterTurn * DeltaSeconds), 0.f));
			Phase += DeltaSeconds * WalkSpeed / Cafe::WaiterStride * PI;
			Legs = FMath::Sin(Phase) * Cafe::WaiterSwing;
		}
	}

	// Legs swing as he walks; the left arm holds the tray up, the right swings a little.
	const float Pitches[] = { Legs, -Legs, Cafe::TrayArm, -Legs * 0.6f };
	for (int32 i = 0; i < static_cast<int32>(UE_ARRAY_COUNT(Pitches)); ++i)
	{
		if (USceneComponent* Part = Limb(i))
		{
			Part->SetRelativeRotation(FRotator(Pitches[i], 0.f, 0.f));
		}
	}
}
