// Fill out your copyright notice in the Description page of Project Settings.

#include "Knockables.h"

#include "Camera/PlayerCameraManager.h"
#include "CarBody.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "OsmGraybox.h"
#include "Pedestrians.h"
#include "PestiCar.h"
#include "ProceduralMeshComponent.h"
#include "SweeperModel.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogKnockables, Log, All);

namespace Knockables
{
	// The player on foot is a circle this wide, his car a box this long and wide (half sizes, cm).
	constexpr float PersonRadius = 38.f;
	constexpr float CarHalfLength = 230.f;
	constexpr float CarHalfWidth = 92.f;
	// Slower than this, nothing is pushed (cm/s).
	constexpr float MinPushSpeed = 40.f;
	// Things fly off a little faster than what hits them, and up a little.
	constexpr float CarPushShare = 1.15f;
	constexpr float PersonPushShare = 0.9f;
	constexpr float CarLiftShare = 0.22f;
	constexpr float MaxCarLift = 450.f;
	constexpr float PersonLift = 60.f;
	constexpr float CarSpin = 420.f;
	constexpr float PersonSpin = 160.f;
	// What hitting one thing costs the car: a share of its speed per kilogram.
	constexpr float CarSlowPerKg = 0.004f;
	constexpr float MaxCarSlow = 0.25f;
	// A crate spills its goods when it is hit this hard or tips this far (cos of the tilt).
	constexpr float SpillSpeed = 220.f;
	constexpr float SpillTilt = 0.6f;
	constexpr float LumpMass = 0.25f;
	constexpr float LumpScatter = 220.f;
	// Put things back a few at a time, now and then.
	constexpr float ResetEvery = 0.5f;
	constexpr int32 ResetPerCheck = 12;
	constexpr float UnseenFor = 3.f;
	constexpr double MovedDistance = 5.0;

	// Where things stand in front of a shop or a gate, from the shop's frame
	// (X out from the wall, Y along it): this far out, this far in from the end.
	constexpr double OutFromWall = 65.0;
	constexpr double InFromEnd = 50.0;
	constexpr double Spacing = 62.0;

	// The street sweeper: he looks for a job this often, for things knocked
	// over at least this long ago, between these distances from the player.
	constexpr float SweepLookEvery = 6.f;
	constexpr double SweepAfter = 25.0;
	constexpr double SweepNotNear = 500.0;
	constexpr double SweepNear = 4500.0;
	// He comes from this far off, walking at this pace, and stops this far from the thing.
	constexpr double SweepFromDistance = 1600.0;
	constexpr double SweepFromCamera = 1200.0;
	constexpr float SweeperPace = 125.f;
	constexpr double SweepStandOff = 75.0;
	constexpr double Arrived = 12.0;
	// Sweeping takes this long, a lump swept up every so often; a thing moved
	// further than CarryFrom is carried back, nearer it is just stood up.
	constexpr float SweepFor = 4.5f;
	constexpr float LumpEvery = 0.35f;
	constexpr double CarryFrom = 60.0;
	constexpr double CarryAhead = 45.0;
	constexpr double CarryUp = 30.0;
	// He gives up after this long, and is gone once unseen (or after LeaveAtMost) on the way off.
	constexpr float GiveUpAfter = 120.f;
	constexpr float LeaveAtMost = 60.f;
	constexpr float MinRest = 20.f;
	constexpr float MaxRest = 45.f;
	constexpr double SweeperMiddle = 90.0;
}

AKnockables::AKnockables()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	BaseMaterial = CarBody::LoadPaintMaterial();
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	SphereMesh = Sphere.Object;
}

void AKnockables::BeginPlay()
{
	Super::BeginPlay();

	Random.Initialize(1932);
	for (int32 k = 0; k < static_cast<int32>(KnockModel::EKind::Count); ++k)
	{
		KnockModel::Build(Models.Add(k), static_cast<KnockModel::EKind>(k), 17 + k);
	}

	TArray<TPair<FTransform, KnockModel::EKind>> Places;
	LoadPlaces(Places);
	for (const TPair<FTransform, KnockModel::EKind>& Place : Places)
	{
		AddItem(Place.Value, Place.Key.GetLocation(), Place.Key.Rotator().Yaw);
	}
	UE_LOG(LogKnockables, Log, TEXT("Knockables: %d things set out"), Items.Num());
}

bool AKnockables::LoadPlaces(TArray<TPair<FTransform, KnockModel::EKind>>& OutPlaces) const
{
	using KnockModel::EKind;
	const FString Path = FPaths::Combine(FPaths::ProjectDir(), DataFile);
	FString Text;
	TSharedPtr<FJsonObject> Root;
	if (!FFileHelper::LoadFileToString(Text, *Path)
		|| !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) || !Root.IsValid())
	{
		UE_LOG(LogKnockables, Error, TEXT("Could not read %s"), *Path);
		return false;
	}

	// A row of things along a wall at At (the middle of the front, Z the
	// pavement), Yaw facing the street, from one end of a front Width wide.
	auto Row = [&OutPlaces](const FVector& At, double Yaw, double Width, double End, const TArray<EKind>& Kinds)
	{
		const FRotator Facing(0.0, Yaw, 0.0);
		for (int32 i = 0; i < Kinds.Num(); ++i)
		{
			const double Y = End * (Width * 0.5 - Knockables::InFromEnd - i * Knockables::Spacing);
			if (FMath::Abs(Y) < 60.0 && Width > 200.0)
			{
				break;
			}
			const FVector Local(Knockables::OutFromWall, Y, 0.0);
			OutPlaces.Emplace(FTransform(FRotator(0.0, Yaw + (i % 2 == 0 ? 3.0 : -4.0), 0.0), At + Facing.RotateVector(Local)), Kinds[i]);
		}
	};

	// Goods set out by the shops that sell them.
	const TArray<TSharedPtr<FJsonValue>>* Shops = nullptr;
	if (Root->TryGetArrayField(TEXT("shops"), Shops))
	{
		for (const TSharedPtr<FJsonValue>& Value : *Shops)
		{
			const TSharedPtr<FJsonObject>& Shop = Value->AsObject();
			const TArray<TSharedPtr<FJsonValue>>* At = nullptr;
			if (!Shop.IsValid() || !Shop->TryGetArrayField(TEXT("at"), At) || At->Num() < 3 || Shop->GetBoolField(TEXT("closed")))
			{
				continue;
			}
			const FString Trade = Shop->GetStringField(TEXT("trade"));
			const double Width = Shop->GetNumberField(TEXT("width"));
			const int64 Seed = static_cast<int64>(Shop->GetNumberField(TEXT("seed")));
			// The shop's own goods (bOutside) stand by the door; ours at the end away from the middle.
			const double End = (Seed + static_cast<int64>(Width)) % 2 == 0 ? 1.0 : -1.0;
			TArray<EKind> Kinds;
			if (Trade == TEXT("greengrocer"))
			{
				Kinds = { EKind::Apples, EKind::Cabbages, EKind::Potatoes };
			}
			else if (Trade == TEXT("grocer"))
			{
				Kinds = { EKind::Potatoes, EKind::Crate };
			}
			else if (Trade == TEXT("tavern"))
			{
				Kinds = { EKind::Barrel, EKind::Barrel };
			}
			else if (Trade == TEXT("dairy"))
			{
				Kinds = { EKind::Churn, EKind::Churn };
			}
			else if (Trade == TEXT("baker"))
			{
				Kinds = { EKind::Basket };
			}
			else if (Trade == TEXT("hardware"))
			{
				Kinds = { EKind::Dustbin };
			}
			if (!Kinds.IsEmpty())
			{
				const FVector Front((*At)[0]->AsNumber(), (*At)[1]->AsNumber(), 0.0);
				Row(Front, (*At)[2]->AsNumber(), Width, End, Kinds);
			}
		}
	}

	// Dustbins put out by most house gates.
	const TArray<TSharedPtr<FJsonValue>>* Gates = nullptr;
	if (Root->TryGetArrayField(TEXT("gates"), Gates))
	{
		int32 Count = 0;
		for (const TSharedPtr<FJsonValue>& Value : *Gates)
		{
			const TSharedPtr<FJsonObject>& Gate = Value->AsObject();
			const TArray<TSharedPtr<FJsonValue>>* At = nullptr;
			if (!Gate.IsValid() || !Gate->TryGetArrayField(TEXT("at"), At) || At->Num() < 3 || ++Count % 3 == 0)
			{
				continue;
			}
			// Beside the gateway, not in it.
			const double Width = Gate->GetNumberField(TEXT("width"));
			const FVector Middle((*At)[0]->AsNumber(), (*At)[1]->AsNumber(), 0.0);
			const double Yaw = (*At)[2]->AsNumber();
			const double Side = Count % 2 == 0 ? 1.0 : -1.0;
			const FRotator Facing(0.0, Yaw, 0.0);
			for (int32 i = 0; i < (Count % 4 == 1 ? 2 : 1); ++i)
			{
				const FVector Local(Knockables::OutFromWall - 20.0, Side * (Width * 0.5 + 40.0 + i * 54.0), 0.0);
				OutPlaces.Emplace(FTransform(FRotator(0.0, Yaw + i * 20.0, 0.0), Middle + Facing.RotateVector(Local)), EKind::Dustbin);
			}
		}
	}

	// Crates and baskets beside the market stalls: full ones by the produce sellers.
	const TSharedPtr<FJsonObject>* Market = nullptr;
	const TArray<TSharedPtr<FJsonValue>>* Stalls = nullptr;
	if (Root->TryGetObjectField(TEXT("market"), Market) && (*Market)->TryGetArrayField(TEXT("stalls"), Stalls))
	{
		int32 Count = 0;
		for (const TSharedPtr<FJsonValue>& Value : *Stalls)
		{
			const TSharedPtr<FJsonObject>& Stall = Value->AsObject();
			const TArray<TSharedPtr<FJsonValue>>* At = nullptr;
			if (!Stall.IsValid() || !Stall->TryGetArrayField(TEXT("at"), At) || At->Num() < 3)
			{
				continue;
			}
			++Count;
			const FString Goods = Stall->GetStringField(TEXT("goods"));
			const bool bProduce = Goods == TEXT("produce");
			if (!bProduce && Count % 3 != 0)
			{
				continue;
			}
			const FVector Middle((*At)[0]->AsNumber(), (*At)[1]->AsNumber(), 0.0);
			const double Yaw = (*At)[2]->AsNumber();
			const FRotator Facing(0.0, Yaw, 0.0);
			const double Side = Count % 2 == 0 ? 1.0 : -1.0;
			const TArray<EKind> Kinds = bProduce ? TArray<EKind>{ EKind::Apples, EKind::Cabbages } : TArray<EKind>{ Count % 2 == 0 ? EKind::Basket : EKind::Crate };
			for (int32 i = 0; i < Kinds.Num(); ++i)
			{
				const FVector Local(-30.0 + i * 64.0, Side * 190.0, 0.0);
				OutPlaces.Emplace(FTransform(FRotator(0.0, Yaw + 90.0, 0.0), Middle + Facing.RotateVector(Local)), Kinds[i]);
			}
		}
	}
	return true;
}

UMaterialInstanceDynamic* AKnockables::Paint(const FLinearColor& Color)
{
	const FString Key = Color.ToString();
	if (UMaterialInstanceDynamic** Found = Paints.Find(Key))
	{
		return *Found;
	}
	UMaterialInstanceDynamic* Made = BaseMaterial ? UMaterialInstanceDynamic::Create(BaseMaterial, this) : nullptr;
	if (Made)
	{
		Made->SetVectorParameterValue(TEXT("Color"), Color);
	}
	Paints.Add(Key, Made);
	return Made;
}

bool AKnockables::GroundAt(const FVector2D& At, double Near, FVector& OutGround) const
{
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(KnockablesGround), false, this);
	if (!GetWorld()->LineTraceSingleByObjectType(Hit, FVector(At, Near + 400.0), FVector(At, Near - 2000.0), FCollisionObjectQueryParams(ECC_WorldStatic), Params))
	{
		return false;
	}
	// Only the open ground, never a roof or a kerbside thing.
	if (!Cast<AOsmGraybox>(Hit.GetActor()))
	{
		return false;
	}
	OutGround = Hit.ImpactPoint;
	return true;
}

void AKnockables::AddItem(KnockModel::EKind Kind, const FVector& At, double Yaw)
{
	const KnockModel::FThing* Model = Models.Find(static_cast<int32>(Kind));
	FVector Ground;
	if (!Model || !GroundAt(FVector2D(At), 0.0, Ground))
	{
		return;
	}

	// Nothing solid where it would stand (a stall, a lamp, a tree, a wall).
	const FVector Middle = Ground + FVector(0.0, 0.0, Model->Half.Z + 3.0);
	const FQuat Turn = FRotator(0.0, Yaw, 0.0).Quaternion();
	FCollisionQueryParams Params(SCENE_QUERY_STAT(KnockablesRoom), false, this);
	FCollisionObjectQueryParams Solid;
	Solid.AddObjectTypesToQuery(ECC_WorldStatic);
	Solid.AddObjectTypesToQuery(ECC_WorldDynamic);
	Solid.AddObjectTypesToQuery(ECC_PhysicsBody);
	if (GetWorld()->OverlapAnyTestByObjectType(Middle, Turn, Solid, FCollisionShape::MakeBox(Model->Half - FVector(1.0)), Params))
	{
		return;
	}

	UProceduralMeshComponent* Mesh = NewObject<UProceduralMeshComponent>(this);
	Mesh->SetupAttachment(RootComponent);
	Mesh->SetWorldLocationAndRotation(Ground + FVector(0.0, 0.0, 1.0), Turn);
	Mesh->bUseComplexAsSimpleCollision = false;
	Mesh->SetCanEverAffectNavigation(false);
	Mesh->SetCachedMaxDrawDistance(CullDistance);
	Mesh->SetCastShadow(true);

	FItem Item;
	Item.Kind = Kind;
	Item.Mesh = Mesh;
	int32 Section = 0;
	auto AddSections = [&Section, Mesh, this](const PersonKit::FPiece& Piece)
	{
		for (const PersonKit::FSection& From : Piece.Sections)
		{
			Mesh->CreateMeshSection_LinearColor(Section, From.Vertices, From.Triangles, From.Normals,
				TArray<FVector2D>(), TArray<FLinearColor>(), TArray<FProcMeshTangent>(), false);
			Mesh->SetMaterial(Section, Paint(From.Color));
			++Section;
		}
	};
	AddSections(Model->Mesh);
	Item.FirstLoad = Section;
	AddSections(Model->Load);
	Item.Loads = Section - Item.FirstLoad;

	// A simple shape to tumble with: a box, or an upright drum.
	TArray<FVector> Hull;
	const FVector& H = Model->Half;
	if (Model->bRound)
	{
		for (int32 i = 0; i < 12; ++i)
		{
			const double A = 2.0 * PI * i / 12.0;
			const FVector Rim(H.X * FMath::Cos(A), H.Y * FMath::Sin(A), 0.0);
			Hull.Add(Rim);
			Hull.Add(Rim + FVector(0.0, 0.0, H.Z * 2.0));
		}
	}
	else
	{
		for (const double X : { -H.X, H.X })
		{
			for (const double Y : { -H.Y, H.Y })
			{
				Hull.Add(FVector(X, Y, 0.0));
				Hull.Add(FVector(X, Y, H.Z * 2.0));
			}
		}
	}
	Mesh->AddCollisionConvexMesh(Hull);

	// A body of its own that bumps the ground, the walls and other things,
	// but not people or cars: those push it by hand (Tick).
	Mesh->SetCollisionObjectType(ECC_PhysicsBody);
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Mesh->SetCollisionResponseToAllChannels(ECR_Block);
	Mesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	Mesh->SetCollisionResponseToChannel(ECC_Vehicle, ECR_Ignore);
	Mesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	Mesh->RegisterComponent();
	AddInstanceComponent(Mesh);
	Mesh->SetSimulatePhysics(true);
	Mesh->SetMassOverrideInKg(NAME_None, Model->Mass, true);
	Mesh->SetLinearDamping(0.3f);
	Mesh->SetAngularDamping(0.8f);
	Mesh->PutRigidBodyToSleep();

	Item.Home = Mesh->GetComponentTransform();
	Items.Add(Item);
}

void AKnockables::Push(FItem& Item, const FVector& Direction, float Speed, float Lift, float Spin)
{
	UProceduralMeshComponent* Mesh = Item.Mesh;
	const FVector Velocity = Mesh->GetPhysicsLinearVelocity();
	const double Along = FVector::DotProduct(Velocity, Direction);
	if (Along >= Speed)
	{
		return;
	}
	Mesh->WakeRigidBody();
	Mesh->SetPhysicsLinearVelocity(Velocity + Direction * (Speed - Along) + FVector(0.0, 0.0, Lift));
	// Tip it over away from what hit it, with a little twist.
	const FVector Tumble = FVector::CrossProduct(FVector::UpVector, Direction) * Spin + FVector(0.0, 0.0, Random.FRandRange(-0.3f, 0.3f) * Spin);
	Mesh->SetPhysicsAngularVelocityInDegrees(Mesh->GetPhysicsAngularVelocityInDegrees() + Tumble);
	Item.bMoved = true;
	Item.MovedAt = GetWorld()->GetTimeSeconds();
	if (Speed > Knockables::SpillSpeed)
	{
		Spill(Item);
	}
}

void AKnockables::Spill(FItem& Item)
{
	const KnockModel::FThing* Model = Models.Find(static_cast<int32>(Item.Kind));
	if (Item.bSpilled || Item.Loads == 0 || !Model || !SphereMesh)
	{
		return;
	}
	Item.bSpilled = true;
	for (int32 s = Item.FirstLoad; s < Item.FirstLoad + Item.Loads; ++s)
	{
		Item.Mesh->SetMeshSectionVisible(s, false);
	}

	// The goods roll out as loose lumps.
	const FTransform& Frame = Item.Mesh->GetComponentTransform();
	const FVector Moving = Item.Mesh->GetPhysicsLinearVelocity();
	const double Scale = Model->LumpRadius / 50.0;
	for (int32 i = 0; i < Model->Lumps; ++i)
	{
		UStaticMeshComponent* Lump = NewObject<UStaticMeshComponent>(this);
		Lump->SetupAttachment(RootComponent);
		Lump->SetStaticMesh(SphereMesh);
		const FVector Local(Random.FRandRange(-0.7f, 0.7f) * Model->Half.X, Random.FRandRange(-0.7f, 0.7f) * Model->Half.Y, Model->Half.Z * 2.0 + Model->LumpRadius);
		Lump->SetWorldLocation(Frame.TransformPosition(Local));
		Lump->SetWorldScale3D(FVector(Scale, Scale, Scale * 0.9));
		Lump->SetMaterial(0, Paint(Model->LumpColor));
		Lump->SetCachedMaxDrawDistance(CullDistance);
		Lump->SetCanEverAffectNavigation(false);
		Lump->SetCollisionObjectType(ECC_PhysicsBody);
		Lump->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Lump->SetCollisionResponseToAllChannels(ECR_Block);
		Lump->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		Lump->SetCollisionResponseToChannel(ECC_Vehicle, ECR_Ignore);
		Lump->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
		Lump->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
		Lump->RegisterComponent();
		AddInstanceComponent(Lump);
		Lump->SetSimulatePhysics(true);
		Lump->SetMassOverrideInKg(NAME_None, Knockables::LumpMass, true);
		Lump->SetLinearDamping(0.4f);
		Lump->SetAngularDamping(0.5f);
		const FVector Scatter(Random.FRandRange(-1.f, 1.f), Random.FRandRange(-1.f, 1.f), Random.FRandRange(0.2f, 0.8f));
		Lump->SetPhysicsLinearVelocity(Moving * 0.8 + Scatter * Knockables::LumpScatter);
		Item.Lumps.Add(Lump);
	}
}

void AKnockables::PutBack(FItem& Item)
{
	for (UStaticMeshComponent* Lump : Item.Lumps)
	{
		if (Lump)
		{
			Lump->DestroyComponent();
		}
	}
	Item.Lumps.Reset();
	for (int32 s = Item.FirstLoad; s < Item.FirstLoad + Item.Loads; ++s)
	{
		Item.Mesh->SetMeshSectionVisible(s, true);
	}
	Item.bSpilled = false;
	Item.Mesh->SetWorldTransform(Item.Home, false, nullptr, ETeleportType::ResetPhysics);
	Item.Mesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
	Item.Mesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	Item.Mesh->PutRigidBodyToSleep();
	Item.bMoved = false;
}

void AKnockables::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (Items.IsEmpty() || DeltaSeconds <= 0.f)
	{
		return;
	}

	// Who does the pushing: the player, or the car he is driving.
	APawn* Pusher = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Pusher)
	{
		return;
	}
	const FVector Here = Pusher->GetActorLocation();
	if (LastPusher.Get() != Pusher)
	{
		LastPusher = Pusher;
		LastPusherAt = Here;
	}
	// Its speed from how far it went (the car moves itself, without physics).
	PusherVelocity = FMath::Lerp(PusherVelocity, (Here - LastPusherAt) / DeltaSeconds, 0.5);
	LastPusherAt = Here;
	const double Speed = PusherVelocity.Size2D();
	APestiCar* Car = Cast<APestiCar>(Pusher);
	const double Now = GetWorld()->GetTimeSeconds();

	if (Speed > Knockables::MinPushSpeed)
	{
		const FVector Forward = Pusher->GetActorForwardVector().GetSafeNormal2D();
		const FVector Right(-Forward.Y, Forward.X, 0.0);
		const FVector Going = PusherVelocity.GetSafeNormal2D();
		for (FItem& Item : Items)
		{
			if (Item.bCarried)
			{
				continue;
			}
			const KnockModel::FThing* Model = Models.Find(static_cast<int32>(Item.Kind));
			const FVector At = Item.Mesh->Bounds.Origin;
			const FVector Offset = At - Here;
			const double Reach = FMath::Max(Model->Half.X, Model->Half.Y);
			if (Offset.SizeSquared2D() > FMath::Square(Knockables::CarHalfLength + 2.0 * Reach))
			{
				continue;
			}
			if (FMath::Abs(Offset.Z) > 250.0)
			{
				continue;
			}
			if (Car)
			{
				const double Ahead = FVector::DotProduct(Offset, Forward);
				const double Aside = FVector::DotProduct(Offset, Right);
				if (FMath::Abs(Ahead) > Knockables::CarHalfLength + Reach || FMath::Abs(Aside) > Knockables::CarHalfWidth + Reach)
				{
					continue;
				}
				// Thrown on ahead of the car and a little out to the side it was on.
				const FVector Direction = (Going + Right * FMath::Sign(Aside) * 0.4).GetSafeNormal();
				Push(Item, Direction, static_cast<float>(Speed) * Knockables::CarPushShare, FMath::Min(static_cast<float>(Speed) * Knockables::CarLiftShare, Knockables::MaxCarLift), Knockables::CarSpin);
				if (Now - Item.LastHit > 1.0)
				{
					Item.LastHit = Now;
					Car->ForwardSpeed *= 1.f - FMath::Min(Model->Mass * Knockables::CarSlowPerKg, Knockables::MaxCarSlow);
				}
			}
			else
			{
				if (Offset.Size2D() > Knockables::PersonRadius + Reach)
				{
					continue;
				}
				// Kicked along the way he walks, and out of his path.
				const FVector Direction = (Going + Offset.GetSafeNormal2D()).GetSafeNormal();
				Push(Item, Direction, static_cast<float>(Speed) * Knockables::PersonPushShare, Knockables::PersonLift, Knockables::PersonSpin);
			}
		}
	}

	// Crates that have gone over (bumped by a barrel, say) spill too.
	for (FItem& Item : Items)
	{
		if (Item.bMoved && !Item.bCarried && !Item.bSpilled && Item.Loads > 0 && Item.Mesh->GetUpVector().Z < Knockables::SpillTilt)
		{
			Spill(Item);
		}
	}

	const APlayerController* PC = GetWorld()->GetFirstPlayerController();
	const FVector Camera = PC && PC->PlayerCameraManager ? PC->PlayerCameraManager->GetCameraLocation() : Here;
	TickSweeper(DeltaSeconds, Here, Camera);

	// Now and then, put back a few that were moved, where nobody is looking.
	ResetClock += DeltaSeconds;
	if (ResetClock < Knockables::ResetEvery)
	{
		return;
	}
	ResetClock = 0.f;
	for (int32 n = 0; n < Knockables::ResetPerCheck && Items.Num() > 0; ++n)
	{
		ResetNext = (ResetNext + 1) % Items.Num();
		FItem& Item = Items[ResetNext];
		if (!Item.bMoved || GetWorld()->GetTimeSeconds() - Item.MovedAt < ResetAfter)
		{
			continue;
		}
		if (Item.bCarried || (SweepState != ESweep::None && ResetNext == SweepItem))
		{
			continue;
		}
		const bool bFar = FVector::Dist(Item.Home.GetLocation(), Camera) > ResetDistance && FVector::Dist(Item.Mesh->GetComponentLocation(), Camera) > ResetDistance;
		bool bSeen = Item.Mesh->WasRecentlyRendered(Knockables::UnseenFor);
		for (const UStaticMeshComponent* Lump : Item.Lumps)
		{
			bSeen = bSeen || (Lump && Lump->WasRecentlyRendered(Knockables::UnseenFor));
		}
		if (bFar && !bSeen)
		{
			PutBack(Item);
		}
	}
}

bool AKnockables::FindSweeperStart(const FItem& Item, const FVector& Camera, FVector& OutStart) const
{
	const FVector Home = Item.Home.GetLocation();
	// First away from the camera, then round the other ways.
	const FVector2D Away = FVector2D(Home - Camera).GetSafeNormal();
	const double AwayYaw = Away.IsNearlyZero() ? 0.0 : FMath::Atan2(Away.Y, Away.X);
	for (int32 Try = 0; Try < 8; ++Try)
	{
		const double Turn = AwayYaw + (Try % 2 == 0 ? 1.0 : -1.0) * ((Try + 1) / 2) * PI / 4.0;
		const FVector2D End = FVector2D(Home) + FVector2D(FMath::Cos(Turn), FMath::Sin(Turn)) * Knockables::SweepFromDistance;
		FVector Ground;
		if (!GroundAt(End, Home.Z, Ground) || FMath::Abs(Ground.Z - Home.Z) > 150.0)
		{
			continue;
		}
		if (FVector::Dist(Ground, Camera) < Knockables::SweepFromCamera)
		{
			continue;
		}
		// No building between there and the thing (lamps and benches he just steps round).
		TArray<FHitResult> Hits;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(SweeperWay), false, this);
		GetWorld()->LineTraceMultiByObjectType(Hits, Home + FVector(0.0, 0.0, 100.0), Ground + FVector(0.0, 0.0, 100.0),
			FCollisionObjectQueryParams(ECC_WorldStatic), Params);
		bool bBlocked = false;
		for (const FHitResult& Hit : Hits)
		{
			bBlocked = bBlocked || Cast<AOsmGraybox>(Hit.GetActor()) != nullptr;
		}
		if (!bBlocked)
		{
			OutStart = Ground;
			return true;
		}
	}
	return false;
}

bool AKnockables::SweeperStep(APedestrian* Man, const FVector& Target, float DeltaSeconds)
{
	const FVector At = Man->GetActorLocation();
	const FVector2D Way = FVector2D(Target - At);
	const double Left = Way.Size();
	if (Left < Knockables::Arrived)
	{
		return true;
	}
	const double Step = FMath::Min(Left, static_cast<double>(Knockables::SweeperPace * DeltaSeconds));
	const FVector2D Next = FVector2D(At) + Way / Left * Step;
	FVector Ground;
	if (GroundAt(Next, SweeperGroundZ, Ground))
	{
		SweeperGroundZ = static_cast<float>(Ground.Z);
	}
	const double FaceYaw = FMath::RadiansToDegrees(FMath::Atan2(Way.Y, Way.X));
	Man->WalkTo(FVector(Next, SweeperGroundZ + Knockables::SweeperMiddle), FaceYaw, Knockables::SweeperPace, DeltaSeconds);
	return Left - Step < Knockables::Arrived;
}

void AKnockables::SendSweeperOff()
{
	if (Items.IsValidIndex(SweepItem) && Items[SweepItem].bCarried)
	{
		// Put down where he stands, to be tidied another time.
		Items[SweepItem].bCarried = false;
	}
	SweepState = ESweep::Leaving;
	SweepClock = 0.f;
}

void AKnockables::TickSweeper(float DeltaSeconds, const FVector& PlayerAt, const FVector& Camera)
{
	if (SweepState == ESweep::None)
	{
		SweepWait -= DeltaSeconds;
		if (SweepWait > 0.f)
		{
			return;
		}
		SweepWait = Knockables::SweepLookEvery;
		// The nearest thing knocked over a while ago, not right by the player.
		const double Now = GetWorld()->GetTimeSeconds();
		int32 Best = INDEX_NONE;
		double BestDistance = Knockables::SweepNear;
		for (int32 i = 0; i < Items.Num(); ++i)
		{
			const FItem& Item = Items[i];
			if (!Item.bMoved || Item.bCarried || Now - Item.MovedAt < Knockables::SweepAfter)
			{
				continue;
			}
			const double Distance = FVector::Dist2D(Item.Mesh->GetComponentLocation(), PlayerAt);
			if (Distance < Knockables::SweepNotNear || Distance >= BestDistance)
			{
				continue;
			}
			Best = i;
			BestDistance = Distance;
		}
		FVector Start;
		if (Best == INDEX_NONE || !FindSweeperStart(Items[Best], Camera, Start))
		{
			return;
		}
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		const FVector2D ToThing = FVector2D(Items[Best].Mesh->GetComponentLocation() - Start);
		APedestrian* Man = GetWorld()->SpawnActor<APedestrian>(APedestrian::StaticClass(),
			Start + FVector(0.0, 0.0, Knockables::SweeperMiddle), FRotator(0.0, FMath::RadiansToDegrees(FMath::Atan2(ToThing.Y, ToThing.X)), 0.0), Params);
		if (!Man)
		{
			return;
		}
		Man->SetOutfit(SweeperModel::Outfit(Random));
		Man->CarryBroom(false, 0.f);
		Sweeper = Man;
		SweepItem = Best;
		SweepFrom = Start;
		SweeperGroundZ = static_cast<float>(Start.Z);
		SweepState = ESweep::Coming;
		SweepClock = 0.f;
		SweepJob = 0.f;
		return;
	}

	APedestrian* Man = Sweeper;
	if (!IsValid(Man) || !Items.IsValidIndex(SweepItem))
	{
		if (IsValid(Man))
		{
			Man->Destroy();
		}
		if (Items.IsValidIndex(SweepItem))
		{
			Items[SweepItem].bCarried = false;
		}
		Sweeper = nullptr;
		SweepState = ESweep::None;
		SweepItem = INDEX_NONE;
		SweepWait = Random.FRandRange(Knockables::MinRest, Knockables::MaxRest);
		return;
	}
	FItem& Item = Items[SweepItem];
	SweepClock += DeltaSeconds;
	SweepJob += DeltaSeconds;
	if (SweepState != ESweep::Leaving && SweepJob > Knockables::GiveUpAfter)
	{
		SendSweeperOff();
	}

	const FVector Here = Man->GetActorLocation();
	const FVector Thing = Item.Mesh->Bounds.Origin;
	const FVector2D ToThing = FVector2D(Thing - Here);
	switch (SweepState)
	{
	case ESweep::Coming:
	{
		if (!Item.bMoved)
		{
			// Put back already (out of sight, far away).
			SendSweeperOff();
			break;
		}
		const FVector2D Toward = ToThing.GetSafeNormal();
		const FVector Stop = Thing - FVector(Toward * Knockables::SweepStandOff, 0.0);
		if (SweeperStep(Man, Stop, DeltaSeconds))
		{
			SweepState = ESweep::Sweeping;
			SweepClock = 0.f;
		}
		Man->CarryBroom(false, SweepClock);
		break;
	}
	case ESweep::Sweeping:
	{
		// Kicked away again while he sweeps: after it.
		if (ToThing.Size() > Knockables::SweepStandOff * 2.5)
		{
			SweepState = ESweep::Coming;
			SweepClock = 0.f;
			break;
		}
		const double FaceYaw = FMath::RadiansToDegrees(FMath::Atan2(ToThing.Y, ToThing.X));
		Man->PointTo(Here, FaceYaw, FaceYaw, false);
		Man->CarryBroom(true, SweepClock);
		// The spilt goods swept up one by one.
		if (Item.Lumps.Num() > 0 && Random.FRand() < DeltaSeconds / Knockables::LumpEvery)
		{
			if (UStaticMeshComponent* Lump = Item.Lumps.Pop())
			{
				Lump->DestroyComponent();
			}
		}
		if (SweepClock >= Knockables::SweepFor && Item.Lumps.Num() == 0)
		{
			if (FVector::Dist2D(Thing, Item.Home.GetLocation()) > Knockables::CarryFrom)
			{
				Item.bCarried = true;
				SweepState = ESweep::Carrying;
			}
			else
			{
				// Only fallen over where it stood: stood up again.
				PutBack(Item);
				SendSweeperOff();
			}
			SweepClock = 0.f;
		}
		break;
	}
	case ESweep::Carrying:
	{
		const FVector Home = Item.Home.GetLocation();
		const FVector2D ToHome = FVector2D(Home - Here).GetSafeNormal();
		const bool bThere = SweeperStep(Man, Home - FVector(ToHome * Knockables::CarryAhead, 0.0), DeltaSeconds);
		Man->CarryInArms();
		// The thing held in front of him at his waist.
		const FVector Held = Man->GetActorLocation() + Man->GetActorForwardVector() * Knockables::CarryAhead
			+ FVector(0.0, 0.0, Knockables::CarryUp - Knockables::SweeperMiddle);
		Item.Mesh->SetWorldLocationAndRotation(Held, Item.Home.GetRotation(), false, nullptr, ETeleportType::TeleportPhysics);
		Item.Mesh->SetPhysicsLinearVelocity(FVector::ZeroVector);
		Item.Mesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
		if (bThere)
		{
			Item.bCarried = false;
			PutBack(Item);
			SendSweeperOff();
		}
		break;
	}
	case ESweep::Leaving:
	{
		const bool bGone = SweeperStep(Man, SweepFrom, DeltaSeconds);
		Man->CarryBroom(false, SweepClock);
		const bool bSeen = Man->WasRecentlyRendered(1.f) && FVector::Dist(Here, Camera) < 7000.0;
		if (((bGone || SweepClock > 6.f) && !bSeen) || SweepClock > Knockables::LeaveAtMost)
		{
			Man->Destroy();
			Sweeper = nullptr;
			SweepState = ESweep::None;
			SweepItem = INDEX_NONE;
			SweepWait = Random.FRandRange(Knockables::MinRest, Knockables::MaxRest);
		}
		break;
	}
	default:
		break;
	}
}
