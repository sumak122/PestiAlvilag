// Fill out your copyright notice in the Description page of Project Settings.

#include "Animals.h"

#include "AnimalSound.h"
#include "Camera/PlayerCameraManager.h"
#include "CarBody.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GarayYard.h"
#include "LaciYard.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "OsmGraybox.h"
#include "PestiPerson.h"
#include "ProceduralMeshComponent.h"
#include "StableYard.h"
#include "TrafficCar.h"

namespace Animals
{
	/** Which map a place is given in: the city, or one of the yards (X into the yard from its gate). */
	enum class EFrame : uint8 { City, LaciYard, StableYard, GarayYard };

	struct FPlace
	{
		AnimalModel::EKind Kind;
		EFrame Frame;
		FVector2D At;
		double Roam;
		bool bTame;
		bool bLazy;
	};

	using AnimalModel::EKind;

	// Where each one lives (cm; the city's X east, Y south).
	const FPlace Places[] = {
		// Laci's two: Lafi about the yard, Bütyök asleep at the back of the shed.
		{ EKind::Lafi, EFrame::LaciYard, FVector2D(1250.0, -350.0), 380.0, true, false },
		{ EKind::Butyok, EFrame::LaciYard, FVector2D(1950.0, -480.0), 120.0, true, true },
		// A tabby in the carters' yard, a black cat among Schwimmer's timber.
		{ EKind::TabbyCat, EFrame::StableYard, FVector2D(1800.0, -400.0), 300.0, false, false },
		{ EKind::BlackCat, EFrame::GarayYard, FVector2D(1300.0, 1000.0), 300.0, false, false },
		// The Teleki tér market: a mutt after the scraps, a ginger cat.
		{ EKind::Mutt, EFrame::City, FVector2D(99969.0, 28889.0), 700.0, false, false },
		{ EKind::GingerCat, EFrame::City, FVector2D(99500.0, 29400.0), 400.0, false, false },
		// Tisza Kálmán tér: another mutt in the park, a tabby.
		{ EKind::Mutt, EFrame::City, FVector2D(60000.0, 500.0), 800.0, false, false },
		{ EKind::TabbyCat, EFrame::City, FVector2D(60400.0, 900.0), 400.0, false, false },
		// A black puli on Baross tér.
		{ EKind::Puli, EFrame::City, FVector2D(90531.0, -35706.0), 600.0, false, false },
		// A black cat on Blaha Lujza tér, and a tabby by Bözsi néni's door.
		{ EKind::BlackCat, EFrame::City, FVector2D(-6500.0, 1500.0), 350.0, false, false },
		{ EKind::TabbyCat, EFrame::City, FVector2D(58638.0, 21478.0), 220.0, false, false },
	};

	// Walking and running (cm/s), and how far one walk or run cycle carries them (as a share of the body length).
	constexpr float CatWalk = 65.f;
	constexpr float DogWalk = 85.f;
	constexpr float CatRun = 480.f;
	constexpr float DogRun = 400.f;
	constexpr float WalkStride = 0.95f;
	constexpr float RunStride = 2.4f;
	constexpr float LeapSpeed = 620.f;
	constexpr float LeapSeconds = 0.45f;
	constexpr float LeapHeight = 35.f;
	constexpr float FleeSeconds = 2.2f;
	constexpr float BlendSeconds = 0.3f;

	// A car (or the player's car, cart or bike) coming at one: it leaps clear when the car is this
	// many seconds away (plus this far), within this far of its path, going faster than this.
	constexpr double CarNoticeSpeed = 120.0;
	constexpr double CarWarnSeconds = 1.4;
	constexpr double CarWarnExtra = 400.0;
	constexpr double CarPathHalfWidth = 240.0;

	// The player on foot: a stray cat runs when he comes this close, or runs at it from this far.
	constexpr double CatShy = 280.0;
	constexpr double CatShyRunning = 650.0;
	constexpr double RunningSpeed = 380.0;
	// A dog watches him from this far, barks now and then, growls when he is close, runs if he runs right at it.
	constexpr double DogWatch = 900.0;
	constexpr double DogGrowl = 320.0;
	constexpr double DogShy = 200.0;
	// Laci's cats meow at him when he stands this close.
	constexpr double TameGreet = 320.0;

	constexpr float SoundFalloff = 3500.f;
}

AAnimals::AAnimals()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	BaseMaterial = CarBody::LoadPaintMaterial();
}

UMaterialInstanceDynamic* AAnimals::Paint(const FLinearColor& Color)
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

void AAnimals::BeginPlay()
{
	Super::BeginPlay();
	Random.Initialize(1932);
	for (int32 k = 0; k < static_cast<int32>(AnimalModel::EKind::Count); ++k)
	{
		AnimalModel::Build(Models.AddDefaulted_GetRef(), static_cast<AnimalModel::EKind>(k));
	}
}

bool AAnimals::GroundAt(const FVector& Near, double Below, FVector& OutGround) const
{
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(AnimalGround), false, this);
	for (const AActor* Each : Ignore)
	{
		Params.AddIgnoredActor(Each);
	}
	if (!GetWorld()->LineTraceSingleByChannel(Hit, Near, Near - FVector(0.0, 0.0, Below), ECC_Visibility, Params)
		|| Hit.ImpactNormal.Z < 0.8)
	{
		return false;
	}
	OutGround = Hit.ImpactPoint;
	return true;
}

bool AAnimals::PathClear(const FVector& From, const FVector& To) const
{
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(AnimalPath), false, this);
	for (const AActor* Each : Ignore)
	{
		Params.AddIgnoredActor(Each);
	}
	const FVector Lift(0.0, 0.0, 25.0);
	return !GetWorld()->LineTraceSingleByChannel(Hit, From + Lift, To + Lift, ECC_Visibility, Params);
}

void AAnimals::SetUp()
{
	bSetUp = true;
	const AActor* Laci = nullptr;
	const AActor* Stable = nullptr;
	const AActor* Garay = nullptr;
	for (TActorIterator<ALaciYard> It(GetWorld()); It; ++It)
	{
		Laci = *It;
	}
	for (TActorIterator<AStableYard> It(GetWorld()); It; ++It)
	{
		Stable = *It;
	}
	for (TActorIterator<AGarayYard> It(GetWorld()); It; ++It)
	{
		Garay = *It;
	}

	for (const Animals::FPlace& Place : Animals::Places)
	{
		if (Place.Frame == Animals::EFrame::City)
		{
			// On the open ground of the map (not a roof, a stall or a bench), near the spot.
			for (int32 Try = 0; Try < 12; ++Try)
			{
				const FVector2D At = Place.At + FVector2D(Random.FRandRange(-1.f, 1.f), Random.FRandRange(-1.f, 1.f)) * (Try * 30.0);
				FHitResult Hit;
				FCollisionQueryParams Params(SCENE_QUERY_STAT(AnimalPlace), false, this);
				if (GetWorld()->LineTraceSingleByChannel(Hit, FVector(At, 20000.0), FVector(At, -5000.0), ECC_Visibility, Params)
					&& Cast<AOsmGraybox>(Hit.GetActor()) && Hit.ImpactNormal.Z > 0.9)
				{
					AddPet(Place.Kind, Hit.ImpactPoint, Place.Roam, Place.bTame, Place.bLazy);
					break;
				}
			}
			continue;
		}

		// In a yard: from its gate, turned as the yard is, at its own ground height.
		const AActor* Yard = Place.Frame == Animals::EFrame::LaciYard ? Laci : Place.Frame == Animals::EFrame::StableYard ? Stable : Garay;
		if (!Yard)
		{
			continue;
		}
		const FVector Spot = Yard->GetActorTransform().TransformPosition(FVector(Place.At, 0.0));
		FVector Ground;
		if (GroundAt(Spot + FVector(0.0, 0.0, 150.0), 300.0, Ground))
		{
			AddPet(Place.Kind, Ground, Place.Roam, Place.bTame, Place.bLazy);
		}
	}
}

void AAnimals::AddPet(AnimalModel::EKind Kind, const FVector& Near, double Roam, bool bTame, bool bLazy)
{
	using namespace AnimalModel;
	FPet Pet;
	Pet.Kind = Kind;
	Pet.bDog = ShapeOf(Kind).bDog;
	Pet.bTame = bTame;
	Pet.bLazy = bLazy;
	Pet.Home = Near;
	Pet.At = Near;
	Pet.Roam = Roam;
	Pet.Yaw = Random.FRandRange(0.f, 360.f);
	Pet.Phase = Random.FRand();
	Pet.Idle = Random.FRand();
	Pet.Resting = bLazy ? EPose::Lie : (Random.FRand() < 0.5f ? EPose::Sit : EPose::Lie);
	Pet.Showing = Pet.Resting;
	Pet.Busy = Random.FRandRange(2.f, 8.f);

	Pet.Root = NewObject<USceneComponent>(this);
	Pet.Root->SetupAttachment(RootComponent);
	Pet.Root->SetMobility(EComponentMobility::Movable);
	Pet.Root->RegisterComponent();
	AddInstanceComponent(Pet.Root);
	Pet.Hips = NewObject<USceneComponent>(this);
	Pet.Hips->SetupAttachment(Pet.Root);
	Pet.Hips->SetMobility(EComponentMobility::Movable);
	Pet.Hips->RegisterComponent();
	AddInstanceComponent(Pet.Hips);

	const FAnimal& Model = Models[static_cast<int32>(Kind)];
	const FVector Pivot = BodyPivot(Kind);
	for (int32 p = 0; p < NumParts; ++p)
	{
		const PersonKit::FPiece& Piece = Model.Parts[p];
		UProceduralMeshComponent* Mesh = NewObject<UProceduralMeshComponent>(this);
		Mesh->SetupAttachment(Pet.Hips);
		// Every piece hangs from the hips: the body itself sits back from them, the rest at their own pivots.
		Mesh->SetRelativeLocation(Piece.Pivot - Pivot);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetCanEverAffectNavigation(false);
		Mesh->SetCachedMaxDrawDistance(ShowWithin);
		// Moved every frame: kept out of the ray-traced lighting and distance fields (it still casts its shadow).
		Mesh->bVisibleInRayTracing = false;
		Mesh->bAffectDistanceFieldLighting = false;
		for (int32 s = 0; s < Piece.Sections.Num(); ++s)
		{
			const PersonKit::FSection& Section = Piece.Sections[s];
			Mesh->CreateMeshSection_LinearColor(s, Section.Vertices, Section.Triangles, Section.Normals,
				TArray<FVector2D>(), TArray<FLinearColor>(), TArray<FProcMeshTangent>(), false);
			Mesh->SetMaterial(s, Paint(Section.Color));
		}
		Mesh->RegisterComponent();
		AddInstanceComponent(Mesh);
		Pet.Parts[p] = Mesh;
	}
	Pet.Last = Pose(Kind, Pet.Showing, Pet.Idle);
	Pet.From = Pet.Last;
	Pets.Add(Pet);
	PoseIt(Pets.Last(), 0.f);
}

void AAnimals::GatherMovers(float DeltaSeconds)
{
	TMap<TWeakObjectPtr<AActor>, FVector> Seen;
	Movers.Reset();
	Ignore.Reset();
	auto Add = [this, DeltaSeconds, &Seen](AActor* Actor, bool bVehicle, bool bPlayer)
	{
		FMover Mover;
		Mover.Actor = Actor;
		Mover.At = Actor->GetActorLocation();
		Mover.bVehicle = bVehicle;
		Mover.bPlayer = bPlayer;
		const TWeakObjectPtr<AActor> Key(Actor);
		if (const FVector* Before = LastSeen.Find(Key))
		{
			Mover.Velocity = FVector2D(Mover.At - *Before) / DeltaSeconds;
		}
		Seen.Add(Key, Mover.At);
		Movers.Add(Mover);
		Ignore.Add(Actor);
	};

	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		if (APawn* Player = PC->GetPawn())
		{
			const APestiPerson* Walker = Cast<APestiPerson>(Player);
			Add(Player, !Walker || Walker->IsRiding(), true);
		}
	}
	const double Near = ShowWithin + 3000.0;
	for (TActorIterator<ATrafficCar> It(GetWorld()); It; ++It)
	{
		if (FVector::DistSquared2D(It->GetActorLocation(), Camera) < Near * Near)
		{
			Add(*It, true, false);
		}
	}
	LastSeen = MoveTemp(Seen);
}

void AAnimals::SetMood(FPet& Pet, EMood Mood, float Seconds)
{
	Pet.Mood = Mood;
	Pet.Busy = Seconds;
}

void AAnimals::Speak(const FPet& Pet, uint8 Kind, float Strength, float Delay)
{
	if (!Voice)
	{
		Voice = NewObject<UAnimalSound>(this);
		Voice->SetupAttachment(RootComponent);
		Voice->bAllowSpatialization = true;
		Voice->bOverrideAttenuation = true;
		Voice->AttenuationOverrides.bAttenuate = true;
		Voice->AttenuationOverrides.bSpatialize = true;
		Voice->AttenuationOverrides.FalloffDistance = Animals::SoundFalloff;
		Voice->RegisterComponent();
		AddInstanceComponent(Voice);
	}
	Voice->SetWorldLocation(Pet.At + FVector(0.0, 0.0, 30.0));
	if (!Voice->IsPlaying())
	{
		Voice->Start();
	}
	// Small dogs and cats speak higher: the puli a little, the cats by their own voice.
	const float Pitch = Pet.Kind == AnimalModel::EKind::Puli ? 1.3f : Pet.Kind == AnimalModel::EKind::Butyok ? 0.85f : 1.f;
	if (Kind < static_cast<uint8>(AnimalSynth::EKind::Count))
	{
		Voice->Queue(static_cast<AnimalSynth::EKind>(Kind), Strength, Pitch, Delay);
	}
}

void AAnimals::RunFrom(FPet& Pet, const FVector2D& Away, bool bLeap)
{
	// The way away from it that is open: straight, else turned a little either way, else sideways.
	FVector2D Way = Away.GetSafeNormal();
	if (Way.IsNearlyZero())
	{
		Way = FVector2D(FRotator(0.0, Random.FRandRange(0.f, 360.f), 0.0).Vector());
	}
	const double Reach = bLeap ? Animals::LeapSpeed * Animals::LeapSeconds + 60.0 : 250.0;
	for (const double Turn : { 0.0, 35.0, -35.0, 70.0, -70.0, 110.0, -110.0 })
	{
		const FVector2D Try = Way.GetRotated(Turn);
		if (PathClear(Pet.At, Pet.At + FVector(Try * Reach, 0.0)))
		{
			Way = Try;
			break;
		}
	}
	Pet.Heading = Way;
	if (bLeap)
	{
		Pet.Speed = Animals::LeapSpeed;
		Pet.Hop = 0.f;
		SetMood(Pet, EMood::Leap, Animals::LeapSeconds);
	}
	else
	{
		Pet.Speed = Pet.bDog ? Animals::DogRun : Animals::CatRun;
		SetMood(Pet, EMood::Flee, Animals::FleeSeconds * Random.FRandRange(0.8f, 1.2f));
	}
}

void AAnimals::React(FPet& Pet)
{
	if (Pet.Mood == EMood::Leap)
	{
		return;
	}
	for (const FMover& Mover : Movers)
	{
		const FVector2D Offset = FVector2D(Pet.At - Mover.At);
		if (FMath::Abs(Pet.At.Z - Mover.At.Z) > 400.0)
		{
			continue;
		}
		const double Speed = Mover.Velocity.Size();
		if (Mover.bVehicle)
		{
			if (Speed < Animals::CarNoticeSpeed)
			{
				continue;
			}
			// Coming at it: leap clear to the nearer side, then run.
			const FVector2D Going = Mover.Velocity / Speed;
			const FVector2D Right(-Going.Y, Going.X);
			const double Ahead = FVector2D::DotProduct(Offset, Going);
			const double Aside = FVector2D::DotProduct(Offset, Right);
			if (Ahead > -150.0 && Ahead < Speed * Animals::CarWarnSeconds + Animals::CarWarnExtra && FMath::Abs(Aside) < Animals::CarPathHalfWidth)
			{
				const double Side = FMath::Abs(Aside) > 10.0 ? FMath::Sign(Aside) : (Random.FRand() < 0.5f ? -1.0 : 1.0);
				RunFrom(Pet, Right * Side + Going * 0.25, true);
				if (Pet.Quiet <= 0.f)
				{
					Speak(Pet, static_cast<uint8>(Pet.bDog ? AnimalSynth::EKind::Bark : AnimalSynth::EKind::Hiss), 0.9f);
					Pet.Quiet = 2.f;
				}
				return;
			}
			continue;
		}
		if (!Mover.bPlayer)
		{
			continue;
		}

		// The player on foot.
		const double Distance = Offset.Size();
		const bool bRunning = Speed > Animals::RunningSpeed;
		const bool bAtIt = bRunning && FVector2D::DotProduct(Mover.Velocity, Offset) > 0.0;
		if (Pet.bDog)
		{
			if (Distance < Animals::DogShy && bAtIt)
			{
				RunFrom(Pet, Offset, false);
				Speak(Pet, static_cast<uint8>(AnimalSynth::EKind::Bark), 1.f);
				Pet.Quiet = 1.5f;
				return;
			}
			if (Distance < Animals::DogWatch && Pet.Mood != EMood::Flee)
			{
				if (Pet.Mood != EMood::Watch)
				{
					SetMood(Pet, EMood::Watch, 0.f);
				}
				Pet.Busy = 1.5f;
				Pet.Heading = (-Offset).GetSafeNormal();
				if (Pet.Quiet <= 0.f)
				{
					if (Distance < Animals::DogGrowl && !bRunning)
					{
						Speak(Pet, static_cast<uint8>(AnimalSynth::EKind::Growl), 0.9f);
						Pet.Quiet = Random.FRandRange(1.5f, 3.f);
					}
					else if (Random.FRand() < 0.7f)
					{
						// One to three barks.
						const int32 Barks = Random.RandRange(1, 3);
						for (int32 b = 0; b < Barks; ++b)
						{
							Speak(Pet, static_cast<uint8>(AnimalSynth::EKind::Bark), Random.FRandRange(0.7f, 1.f), b * Random.FRandRange(0.25f, 0.35f));
						}
						Pet.Quiet = Random.FRandRange(2.f, 4.5f);
					}
					else
					{
						Pet.Quiet = Random.FRandRange(1.f, 2.f);
					}
				}
			}
			return;
		}

		if (Pet.bTame)
		{
			// Laci's cats: only get out of the way of someone running right at them, and meow at a friend.
			if (Distance < 160.0 && bAtIt && !Pet.bLazy && Pet.Mood != EMood::Walk)
			{
				const FVector2D Way = Offset.GetSafeNormal();
				Pet.Target = Pet.At + FVector(Way * 250.0, 0.0);
				Pet.Heading = Way;
				Pet.Speed = Animals::CatWalk * 2.f;
				SetMood(Pet, EMood::Walk, 3.f);
			}
			else if (Distance < Animals::TameGreet && !bRunning && Pet.Quiet <= 0.f)
			{
				if (Random.FRand() < 0.5f)
				{
					Speak(Pet, static_cast<uint8>(AnimalSynth::EKind::Meow), 0.8f);
				}
				Pet.Quiet = Random.FRandRange(6.f, 12.f);
			}
			return;
		}

		// A stray cat: off it goes, with a hiss if he got right up to it.
		if (Pet.Mood != EMood::Flee && (Distance < Animals::CatShy || (Distance < Animals::CatShyRunning && bAtIt)))
		{
			RunFrom(Pet, Offset, false);
			if (Distance < Animals::CatShy && Pet.Quiet <= 0.f && Random.FRand() < 0.4f)
			{
				Speak(Pet, static_cast<uint8>(AnimalSynth::EKind::Hiss), 0.8f);
				Pet.Quiet = 3.f;
			}
			return;
		}
	}
}

void AAnimals::Think(FPet& Pet)
{
	using AnimalModel::EPose;
	switch (Pet.Mood)
	{
	case EMood::Leap:
	{
		// Landed: run on the same way a little.
		const FVector2D Away = Pet.Heading;
		Pet.Hop = -1.f;
		RunFrom(Pet, Away, false);
		break;
	}
	case EMood::Flee:
	case EMood::Watch:
		SetMood(Pet, EMood::Rest, Random.FRandRange(2.f, 5.f));
		Pet.Resting = Pet.bDog ? EPose::Stand : EPose::Sit;
		break;
	case EMood::Walk:
	case EMood::Rest:
	default:
	{
		// Far from home (after running off): walk back. Otherwise rest a while, or wander.
		const double FromHome = FVector::Dist2D(Pet.At, Pet.Home);
		const float WanderChance = Pet.bLazy ? 0.08f : 0.45f;
		if (FromHome > Pet.Roam * 1.5 || (Pet.Mood == EMood::Rest && Random.FRand() < WanderChance))
		{
			for (int32 Try = 0; Try < 4; ++Try)
			{
				FVector2D Toward = FVector2D(Pet.Home) + FVector2D(Random.FRandRange(-1.f, 1.f), Random.FRandRange(-1.f, 1.f)) * Pet.Roam;
				if (FromHome > Pet.Roam * 1.5)
				{
					// The long way home, a stretch at a time.
					Toward = FVector2D(Pet.At) + (FVector2D(Pet.Home) - FVector2D(Pet.At)).GetClampedToMaxSize(600.0);
				}
				const FVector Goal(Toward, Pet.At.Z);
				if (PathClear(Pet.At, Goal))
				{
					Pet.Target = Goal;
					Pet.Heading = (Toward - FVector2D(Pet.At)).GetSafeNormal();
					Pet.Speed = Pet.bDog ? Animals::DogWalk : Animals::CatWalk;
					SetMood(Pet, EMood::Walk, 15.f);
					return;
				}
			}
		}
		const float Pick = Random.FRand();
		Pet.Resting = Pet.bLazy || Pick < 0.4f ? EPose::Lie : Pick < 0.8f ? EPose::Sit : EPose::Stand;
		SetMood(Pet, EMood::Rest, Pet.bLazy ? Random.FRandRange(15.f, 40.f) : Random.FRandRange(4.f, 12.f));
		break;
	}
	}
}

void AAnimals::Move(FPet& Pet, float DeltaSeconds)
{
	const bool bMoving = Pet.Mood == EMood::Walk || Pet.Mood == EMood::Flee || Pet.Mood == EMood::Leap;
	if (!bMoving)
	{
		if (Pet.Mood == EMood::Watch)
		{
			Pet.Yaw = FMath::FixedTurn(Pet.Yaw, static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(Pet.Heading.Y, Pet.Heading.X))), 200.f * DeltaSeconds);
		}
		return;
	}
	if (Pet.Mood == EMood::Walk)
	{
		const FVector2D ToTarget = FVector2D(Pet.Target - Pet.At);
		if (ToTarget.Size() < 10.0)
		{
			Pet.Busy = 0.f;
			return;
		}
		Pet.Heading = ToTarget.GetSafeNormal();
	}

	const FVector2D Step = Pet.Heading * (Pet.Speed * DeltaSeconds);
	const FVector Next = Pet.At + FVector(Step, 0.0);
	// Something in the way (a wall, a crate): a runner turns along it, a walker stops.
	const FVector Ahead = Pet.At + FVector(Pet.Heading * 45.0, 0.0);
	if (!PathClear(Pet.At, Ahead))
	{
		if (Pet.Mood == EMood::Walk)
		{
			Pet.Busy = 0.f;
			return;
		}
		Pet.Heading = Pet.Heading.GetRotated(Random.FRand() < 0.5f ? 80.0 : -80.0);
		return;
	}
	FVector Ground;
	if (GroundAt(Next + FVector(0.0, 0.0, 60.0), 200.0, Ground))
	{
		Pet.At = Ground;
	}
	else if (Pet.Mood == EMood::Walk)
	{
		// No ground ahead (an edge, a pit): stop.
		Pet.Busy = 0.f;
		return;
	}
	else
	{
		Pet.At = Next;
	}
	const float Wanted = static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(Pet.Heading.Y, Pet.Heading.X)));
	Pet.Yaw = FMath::FixedTurn(Pet.Yaw, Wanted, (Pet.Mood == EMood::Walk ? 240.f : 900.f) * DeltaSeconds);

	const AnimalModel::FShape Shape = AnimalModel::ShapeOf(Pet.Kind);
	const float Stride = static_cast<float>(Shape.Length) * (Pet.Mood == EMood::Walk ? Animals::WalkStride : Animals::RunStride);
	Pet.Phase = FMath::Fmod(Pet.Phase + Pet.Speed * DeltaSeconds / FMath::Max(Stride, 1.f), 1.f);
}

void AAnimals::PoseIt(FPet& Pet, float DeltaSeconds)
{
	using namespace AnimalModel;
	EPose Wanted = Pet.Resting;
	double Phase = Pet.Idle;
	switch (Pet.Mood)
	{
	case EMood::Walk:
		Wanted = EPose::Walk;
		Phase = Pet.Phase;
		break;
	case EMood::Flee:
	case EMood::Leap:
		Wanted = EPose::Run;
		Phase = Pet.Phase;
		break;
	case EMood::Watch:
		Wanted = EPose::Stand;
		break;
	default:
		break;
	}
	if (Wanted != Pet.Showing)
	{
		Pet.From = Pet.Last;
		Pet.Showing = Wanted;
		Pet.Blend = 0.f;
	}
	Pet.Blend = FMath::Min(1.f, Pet.Blend + DeltaSeconds / Animals::BlendSeconds);
	const FPose Target = Pose(Pet.Kind, Wanted, Phase);
	const double A = FMath::SmoothStep(0.f, 1.f, Pet.Blend);
	auto Mix = [A](const FTurn& X, const FTurn& Y)
	{
		FTurn Out;
		Out.Pitch = FMath::Lerp(X.Pitch, Y.Pitch, A);
		Out.Yaw = FMath::Lerp(X.Yaw, Y.Yaw, A);
		Out.Roll = FMath::Lerp(X.Roll, Y.Roll, A);
		return Out;
	};
	FPose Now;
	Now.BodyTurn = Mix(Pet.From.BodyTurn, Target.BodyTurn);
	Now.Drop = FMath::Lerp(Pet.From.Drop, Target.Drop, A);
	for (int32 p = 0; p < NumParts; ++p)
	{
		Now.Parts[p] = Mix(Pet.From.Parts[p], Target.Parts[p]);
	}
	Pet.Last = Now;

	// The head follows the player when he is near and it isn't running.
	Now.Parts[Head].Yaw += Pet.Look;

	const double Lift = Pet.Hop >= 0.f ? Animals::LeapHeight * FMath::Sin(PI * FMath::Clamp(Pet.Hop / Animals::LeapSeconds, 0.f, 1.f)) : 0.0;
	Pet.Root->SetWorldLocationAndRotation(Pet.At + FVector(0.0, 0.0, Lift), FRotator(0.0, Pet.Yaw, 0.0));
	const FVector Pivot = BodyPivot(Pet.Kind);
	Pet.Hips->SetRelativeLocationAndRotation(Pivot - FVector(0.0, 0.0, Now.Drop), FRotator(Now.BodyTurn.Pitch, Now.BodyTurn.Yaw, Now.BodyTurn.Roll));
	for (int32 p = 0; p < NumParts; ++p)
	{
		if (p != Body)
		{
			Pet.Parts[p]->SetRelativeRotation(FRotator(Now.Parts[p].Pitch, Now.Parts[p].Yaw, Now.Parts[p].Roll));
		}
	}
}

void AAnimals::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (DeltaSeconds <= 0.f)
	{
		return;
	}
	const APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC || !PC->PlayerCameraManager)
	{
		return;
	}
	Camera = PC->PlayerCameraManager->GetCameraLocation();

	// The yards and the city are built in the first moments of play: wait for them.
	if (!bSetUp)
	{
		SetUpWait -= DeltaSeconds;
		if (SetUpWait <= 0.f)
		{
			SetUp();
		}
		return;
	}

	GatherMovers(DeltaSeconds);
	const FVector PlayerAt = Movers.Num() > 0 && Movers[0].bPlayer ? Movers[0].At : Camera;

	for (FPet& Pet : Pets)
	{
		const bool bShow = FVector::DistSquared(Pet.At, Camera) < FMath::Square(static_cast<double>(ShowWithin));
		if (bShow != Pet.bShown)
		{
			Pet.bShown = bShow;
			Pet.Root->SetVisibility(bShow, true);
		}
		if (!bShow)
		{
			continue;
		}

		Pet.Quiet -= DeltaSeconds;
		Pet.Idle = FMath::Fmod(Pet.Idle + DeltaSeconds * 0.12f, 1.f);
		if (Pet.Hop >= 0.f)
		{
			Pet.Hop += DeltaSeconds;
		}
		React(Pet);
		Pet.Busy -= DeltaSeconds;
		if (Pet.Busy <= 0.f)
		{
			Think(Pet);
		}
		Move(Pet, DeltaSeconds);

		// Watch the player when he is near, unless on the move.
		float LookWanted = 0.f;
		const FVector ToPlayer = PlayerAt - Pet.At;
		if ((Pet.Mood == EMood::Rest || Pet.Mood == EMood::Watch) && ToPlayer.SizeSquared2D() < FMath::Square(700.0))
		{
			LookWanted = FMath::Clamp(FMath::FindDeltaAngleDegrees(Pet.Yaw, static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(ToPlayer.Y, ToPlayer.X)))), -70.f, 70.f);
			if (Pet.bLazy)
			{
				LookWanted *= 0.4f;
			}
		}
		Pet.Look = FMath::FInterpTo(Pet.Look, LookWanted, DeltaSeconds, 3.f);
		PoseIt(Pet, DeltaSeconds);
	}
}
