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
#include "PestiCar.h"
#include "ProceduralMeshComponent.h"
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
		if (Item.bMoved && !Item.bSpilled && Item.Loads > 0 && Item.Mesh->GetUpVector().Z < Knockables::SpillTilt)
		{
			Spill(Item);
		}
	}

	// Now and then, put back a few that were moved, where nobody is looking.
	ResetClock += DeltaSeconds;
	if (ResetClock < Knockables::ResetEvery)
	{
		return;
	}
	ResetClock = 0.f;
	const APlayerController* PC = GetWorld()->GetFirstPlayerController();
	const FVector Camera = PC && PC->PlayerCameraManager ? PC->PlayerCameraManager->GetCameraLocation() : Here;
	for (int32 n = 0; n < Knockables::ResetPerCheck && Items.Num() > 0; ++n)
	{
		ResetNext = (ResetNext + 1) % Items.Num();
		FItem& Item = Items[ResetNext];
		if (!Item.bMoved)
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
