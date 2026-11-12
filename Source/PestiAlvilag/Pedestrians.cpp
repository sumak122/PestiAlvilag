// Fill out your copyright notice in the Description page of Project Settings.

#include "Pedestrians.h"

#include "StreetName.h"
#include "Camera/PlayerCameraManager.h"
#include "CarBody.h"
#include "Components/SceneComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/FileHelper.h"
#include "Mission.h"
#include "Misc/Paths.h"
#include "OsmGraybox.h"
#include "PersonModel.h"
#include "PestiAlvilagGameModeBase.h"
#include "PestiCar.h"
#include "PestiHud.h"
#include "PestiPerson.h"
#include "PolicePatrols.h"
#include "ProceduralMeshComponent.h"
#include "StreetFights.h"
#include "StreetFurniture.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "TrafficCar.h"
#include "TrafficManager.h"
#include "Tram.h"
#include "UObject/ConstructorHelpers.h"
#include "UmbrellaModel.h"
#include "Weather.h"

DEFINE_LOG_CATEGORY_STATIC(LogPedestrians, Log, All);

namespace Pedestrians
{
	constexpr float MinSpeed = 105.f;
	constexpr float MaxSpeed = 160.f;
	// Each keeps to the right of the walking line by this much.
	constexpr float MinLane = 40.f;
	constexpr float MaxLane = 100.f;
	constexpr float Middle = 90.f;

	constexpr double EndCell = 1000.0;
	constexpr double JoinDistance = 700.0;
	constexpr double RefindLines = 2000.0;
	constexpr float GroundEvery = 0.3f;
	constexpr int32 SpawnPerFrame = 4;
	constexpr int32 PickTries = 48;
	constexpr double FillNear = 500.0;
	// Later arrivals only appear where the camera is not looking (more than
	// about 80 degrees off its view) or behind a building, so nobody pops up
	// in view; and the ones left behind only go once they are out of sight.
	constexpr double OutOfViewDot = 0.17;
	constexpr float SeenWithin = 1.f;
	constexpr double GiveUpShare = 2.0;
	constexpr double EyeHeight = 150.0;
	// Where he is drawn catches up with his place on the line at up to this
	// many times his walking pace, and turns at this rate (degrees a second).
	constexpr double CatchUp = 1.8;
	constexpr double SnapDistance = 1500.0;
	constexpr float TurnRate = 360.f;

	constexpr float LegSwing = 25.f;
	constexpr float ArmSwing = 18.f;
	constexpr float StrideLength = 110.f;
	// The pace at which the knees bend as much as in a brisk walk (cm/s).
	constexpr float BriskWalk = 140.f;

	// Umbrellas in the rain: how many people have one with them, how much
	// rain it takes before they open it (more for some than others) and
	// below which they shut it again; the umbrella arm raised forward and
	// the elbow bent (degrees), so the hand is at chest height before him;
	// the shaft tipped back towards his head and in over it (degrees), and
	// tipped forward and down while it hangs furled before he raises it; how
	// fast it goes up (share a second, from shut to open); and how much faster
	// those without one hurry along in a downpour.
	constexpr float UmbrellaShare = 0.7f;
	constexpr float UmbrellaFrom = 0.1f;
	constexpr float UmbrellaSpread = 0.25f;
	constexpr float UmbrellaUntil = 0.03f;
	constexpr float UmbrellaRaise = 30.f;
	constexpr float UmbrellaBend = 95.f;
	constexpr float UmbrellaTilt = 6.f;
	constexpr float UmbrellaHang = -130.f;
	constexpr float UmbrellaLean = 7.f;
	constexpr float UmbrellaSpeed = 0.8f;
	constexpr float RainHurry = 0.35f;
	// In a downpour (AWeather::GetRain 0..1): the most of the crowd gone
	// indoors at full rain; above how much rain those without an umbrella
	// make for a doorway (AStreetFurniture::TakeShelter) no further than
	// ShelterReach, how often they look, and below how much they walk on.
	constexpr float IndoorsShare = 0.55f;
	constexpr float ShelterFrom = 0.35f;
	constexpr float ShelterUntil = 0.12f;
	constexpr double ShelterReach = 900.0;
	constexpr float ShelterLook = 2.f;
	constexpr float ShelterHurry = 1.5f;
	constexpr double ShelterNear = 10.0;
	constexpr double ShelterMaxOut = 550.0;
	constexpr double IndoorsBeyond = 2000.0;
	// Arms folded against the cold in a doorway (degrees).
	constexpr float FoldUpper = 16.f;
	constexpr float FoldIn = 4.f;
	constexpr float FoldElbow = 100.f;
	constexpr float FoldOver = 6.f;
	constexpr float FoldAcross = 52.f;

	// Someone on foot who walks into him (closer than this, faster than
	// that) makes him stumble back; someone running at him makes him step
	// out of the way.
	constexpr double BumpDistance = 55.0;
	constexpr double BumpSpeed = 80.0;
	constexpr double RunningSpeed = 500.0;
	constexpr double RunPastDistance = 220.0;
	// A car coming at him: he sees it this far ahead (its speed times this,
	// plus a bit) and this far to either side of its path, and leaps clear;
	// unless he freezes in fright.
	constexpr double CarNoticeSpeed = 300.0;
	constexpr double CarWarnSeconds = 1.6;
	constexpr double CarWarnExtra = 250.0;
	constexpr double CarPathHalfWidth = 260.0;
	constexpr float FreezeChance = 0.1f;
	constexpr float MinNotice = 0.1f;
	constexpr float MaxNotice = 0.4f;
	// At a crossing he waits at the kerb while anything moving faster than
	// MoverSlowest would reach it within RoadClearSeconds, or is this near it
	// (cm); looking again this often (s), at most this long before he goes
	// anyway. On the way over he walks briskly, and runs if a car comes.
	constexpr double MoverSlowest = 150.0;
	constexpr double RoadClearSeconds = 3.5;
	constexpr double RoadNear = 600.0;
	constexpr float RoadLookEvery = 0.3f;
	constexpr float MaxWait = 25.f;
	constexpr float CrossHurry = 1.35f;
	constexpr float CrossRun = 2.3f;
	// Only cars this near the camera count (the walkers further off are not seen).
	constexpr double MoversNear = 9000.0;
	constexpr double LeapSpeed = 420.0;
	// A car up on the pavement (this close to his walking line, this near
	// him) sends him running off along it, this many times faster.
	constexpr double OnPavement = 230.0;
	constexpr double PavementNear = 900.0;
	constexpr float FleeShare = 2.6f;
	constexpr float FleeSeconds = 3.f;
	// The car's front and sides, from its middle (cm), and the speed below
	// which it only nudges him. Faster than KillSpeed it kills him.
	constexpr double CarHalfLength = 215.0;
	constexpr double CarHalfWidth = 95.0;
	constexpr double HitSpeed = 150.0;
	constexpr double KillSpeed = 1100.0;
	constexpr float CarSlowOnHit = 0.85f;
	constexpr double PoliceSee = 5000.0;
	// Witnesses this near run off too.
	constexpr double WitnessNear = 1500.0;
	// Lying down: his middle this far above the ground.
	constexpr double LyingHeight = 18.0;
	constexpr float FallSeconds = 0.45f;
	constexpr float LieSeconds = 2.5f;
	constexpr float GetUpSeconds = 1.2f;
	constexpr double BloodRadius = 70.0;
	constexpr float BloodSpreads = 6.f;
	// The dead are cleared away once out of sight and at least this far off.
	constexpr double ClearBeyond = 2500.0;
	constexpr double ShoutEvery = 4.0;
	const FLinearColor BloodColor(0.16f, 0.004f, 0.004f);

	// What people say (Hungarian, then English).
	struct FLine2 { const TCHAR* Hu; const TCHAR* En; };
	const FLine2 BumpLines[] = {
		{ TEXT("Vigy\u00e1zzon m\u00e1r, k\u00e9rem! H\u00e1t nem l\u00e1t?"), TEXT("Watch where you are going! Are you blind?") },
		{ TEXT("Nofene! L\u00f6kd\u00f6s\u0151dik?"), TEXT("Well now! Pushing, are we?") },
		{ TEXT("Micsoda modortalans\u00e1g!"), TEXT("What bad manners!") },
	};
	const FLine2 LeapLines[] = {
		{ TEXT("Seg\u00edts\u00e9g! Meg\u0151r\u00fclt ez?!"), TEXT("Help! Has he gone mad?!") },
		{ TEXT("J\u00e9zusm\u00e1ria! Majdnem elg\u00e1zolt!"), TEXT("Good Lord! He nearly ran me over!") },
		{ TEXT("H\u00e9! Ez j\u00e1rda, nem orsz\u00e1g\u00fat!"), TEXT("Hey! This is a pavement, not a highway!") },
	};
	const FLine2 GetUpLines[] = {
		{ TEXT("Jaj, a l\u00e1bam! Maga vad\u00e1llat!"), TEXT("Ow, my leg! You brute!") },
		{ TEXT("Rend\u0151r! Elg\u00e1zoltak!"), TEXT("Police! I've been run over!") },
	};
	const FLine2 WitnessLine = { TEXT("\u00daristen! Ment\u0151t! H\u00edvjanak ment\u0151t!"), TEXT("Good God! Call an ambulance!") };
	// Crowds round trouble (APedestrians::Gather): how far off a passer-by
	// notices it, how many of them stop (share), how many watch at most, how
	// far from the middle they stand (the first ring, and each further one),
	// one stays only this far from his own walk, and how often he looks.
	constexpr double GatherMerge = 700.0;
	constexpr double GawkReach = 1000.0;
	constexpr float GawkShare = 0.65f;
	constexpr int32 MostWatchers = 12;
	constexpr double RingNear = 330.0;
	constexpr double RingStep = 75.0;
	constexpr double GawkStray = 1300.0;
	constexpr float GawkLook = 1.f;
	constexpr float GawkHurry = 1.3f;
	constexpr double GawkNear = 12.0;
	constexpr float CheerArms = 140.f;
	constexpr float CheerWave = 20.f;
	constexpr float CheerSpeed = 6.f;
	// How long a crowd stays round a crash, a spilled load, someone knocked down (s).
	constexpr float CrowdKnockedDown = 20.f;
	// Calls from the crowd now and then, when the player is this near (cm).
	constexpr float CrowdShoutMin = 5.f;
	constexpr float CrowdShoutMax = 9.f;
	constexpr double CrowdHeard = 3000.0;
	// The pickpocket: a crowd this big, the player standing in it this long
	// (within PickNear of its middle), and this chance he tries; he takes
	// between these (fillér) and runs this long.
	constexpr int32 PickWatchers = 4;
	constexpr double PickNear = 650.0;
	constexpr double PickAfter = 6.0;
	constexpr float PickChance = 0.5f;
	constexpr int32 PickLeast = 40;
	constexpr int32 PickMost = 200;
	constexpr float PickRun = 14.f;
	const FLine2 FightCrowdLines[] = {
		{ TEXT("\u00dcsd!"), TEXT("Hit him!") },
		{ TEXT("Ez az, add neki!"), TEXT("That's it, give it to him!") },
		{ TEXT("Rend\u0151rt! Valaki h\u00edvjon rend\u0151rt!"), TEXT("Police! Someone call the police!") },
		{ TEXT("Na, ebb\u0151l baj lesz."), TEXT("Well, this'll end badly.") },
	};
	const FLine2 CrashCrowdLines[] = {
		{ TEXT("J\u00e9zusom!"), TEXT("Good Lord!") },
		{ TEXT("Ki volt ez az \u0151r\u00fclt?"), TEXT("Who was that madman?") },
		{ TEXT("H\u00edvjanak orvost!"), TEXT("Get a doctor!") },
		{ TEXT("L\u00e1ttam, ahogy j\u00f6tt!"), TEXT("I saw him coming!") },
	};
	const FLine2 PickLine = { TEXT("K\u00f6sz\u00f6n\u00f6m a t\u00e1rc\u00e1t, nagys\u00e1gos \u00far!"), TEXT("Much obliged for the wallet, sir!") };

	// Asked the way (Q): the nearest passer-by within reach stops this long,
	// turns to the player and after a moment points. He never turns further
	// from the player than MostAside to point, and points a little down.
	constexpr double AskReach = 250.0;
	// A click picks a fight with someone this close (cm) and this much in front (cosine).
	constexpr double FightReach = 230.0;
	constexpr double FightFacing = 0.3;
	constexpr double AskHeight = 200.0;
	constexpr float AskSeconds = 5.f;
	constexpr float PointAfter = 0.5f;
	constexpr float MostAside = 100.f;
	constexpr float AskTurnRate = 360.f;
	constexpr double PointDown = 0.25;
	// Closer than this he points straight at it; further, along the streets
	// at the first corner at least PointAhead away.
	constexpr double PointStraight = 2500.0;
	constexpr double PointAhead = 1200.0;
	// How he puts the distance.
	constexpr double NearBy = 3000.0;
	constexpr double NotFar = 30000.0;
	const FLine2 NearLine = { TEXT("Ott van, p\u00e1r l\u00e9p\u00e9sre, arra."), TEXT("It's right there, a few steps that way.") };
	const FLine2 NotFarLine = { TEXT("Arra, fiam, nincs messze, p\u00e1r perc gyalog."), TEXT("That way, son, it's not far, a few minutes on foot.") };
	const FLine2 FarLine = { TEXT("Arra indulj, fiam, de j\u00f3 messze van."), TEXT("Head that way, son, but it's a good way off.") };
	const FLine2 DontKnowLines[] = {
		{ TEXT("Nem tudom, fiam, nem vagyok ideval\u00f3si."), TEXT("Sorry, son, I'm not from round here.") },
		{ TEXT("Bocs\u00e1sson meg, sietek!"), TEXT("Excuse me, I'm in a hurry!") },
	};

	// Standers (AddStander): how fast they walk back to their spot, how far
	// round they turn to the player (seated ones less), how near on foot the
	// player must come for them to turn and call, and how often they call.
	constexpr float StanderPace = 110.f;
	constexpr float StanderTurnRate = 200.f;
	constexpr float StanderLookAside = 120.f;
	constexpr float SeatedLookAside = 50.f;
	constexpr double HomeNear = 8.0;
	constexpr double CallReach = 450.0;
	constexpr double CallHeight = 300.0;
	constexpr float MinCallEvery = 18.f;
	constexpr float MaxCallEvery = 30.f;
	// A car this near one standing has him jump away; one seated cries out.
	constexpr double StanderCarNear = 500.0;
	constexpr double SeatedCarNear = 700.0;

	FIntPoint CellOf(const FVector2D& At)
	{
		return FIntPoint(FMath::FloorToInt(At.X / EndCell), FMath::FloorToInt(At.Y / EndCell));
	}
}

APedestrian::APedestrian()
{
	// APedestrians moves him; he is no obstacle for cars or the player.
	PrimaryActorTick.bCanEverTick = false;
	Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void APedestrian::WalkTo(const FVector& Location, double Yaw, float Speed, float DeltaSeconds, float ArmsUp)
{
	Unfold();
	SetActorLocationAndRotation(Location, FRotator(0.0, Yaw, 0.0));
	if (ArmsUp > 0.f)
	{
		DropUmbrella();
	}
	else if (bWantsUmbrella || UmbrellaOpen > 0.f)
	{
		UpdateUmbrella(DeltaSeconds);
	}

	Stride += DeltaSeconds * Speed / Pedestrians::StrideLength * PI;
	const float Swing = FMath::Sin(Stride);
	PersonModel::StrideLegs(Pieces, Stride, Pedestrians::LegSwing, FMath::Clamp(Speed / Pedestrians::BriskWalk, 0.f, 1.6f));
	float Arms[] = { -Swing * Pedestrians::ArmSwing, Swing * Pedestrians::ArmSwing };
	if (ArmsUp > 0.f)
	{
		// Arms thrown up, flailing as he runs.
		const float Flail = 15.f * FMath::Sin(Stride * 2.f);
		Arms[0] = FMath::Lerp(Arms[0], 140.f + Flail, ArmsUp);
		Arms[1] = FMath::Lerp(Arms[1], 140.f - Flail, ArmsUp);
	}
	// The umbrella arm comes up first, then the umbrella opens.
	const int32 HoldingArm = UmbrellaOpen > 0.f ? UmbrellaArm() : -1;
	const float Raised = FMath::SmoothStep(0.f, 1.f, FMath::Clamp(UmbrellaOpen * 2.f, 0.f, 1.f));
	if (HoldingArm >= 0)
	{
		Arms[HoldingArm] = FMath::Lerp(Arms[HoldingArm] * 0.2f, Pedestrians::UmbrellaRaise, Raised);
	}
	for (int32 i = 0; i < 2; ++i)
	{
		if (USceneComponent* Part = Limb(PersonKit::ArmLeft + i))
		{
			Part->SetRelativeRotation(FRotator(Arms[i], 0.f, 0.f));
		}
	}
	if (HoldingArm >= 0)
	{
		PlaceUmbrella(HoldingArm, Raised);
	}
}

int32 APedestrian::UmbrellaArm() const
{
	// The right hand, unless it carries something else.
	const bool bRightBusy = Outfit.Held != PersonKit::EHeld::None && Outfit.Held != PersonKit::EHeld::Umbrella;
	return bRightBusy ? 0 : 1;
}

void APedestrian::WantUmbrella(bool bOpen, const FLinearColor& Cloth)
{
	bWantsUmbrella = bOpen;
	if (!Umbrella)
	{
		UmbrellaCloth = Cloth;
	}
}

void APedestrian::UpdateUmbrella(float DeltaSeconds)
{
	UmbrellaOpen = FMath::FInterpConstantTo(UmbrellaOpen, bWantsUmbrella ? 1.f : 0.f, DeltaSeconds, Pedestrians::UmbrellaSpeed);

	if (UmbrellaOpen > 0.f)
	{
		// Someone carrying his umbrella furled opens that one: out of his hand while it is up.
		if (Outfit.Held == PersonKit::EHeld::Umbrella)
		{
			PersonKit::FOutfit Bare = Outfit;
			Bare.Held = PersonKit::EHeld::None;
			SetOutfit(Bare);
			bFurledInUse = true;
		}
		if (!Umbrella)
		{
			Umbrella = NewObject<UProceduralMeshComponent>(this);
			Umbrella->SetupAttachment(Capsule);
			Umbrella->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Umbrella->SetCanEverAffectNavigation(false);
			// It moves with him all the time: kept out of the ray-traced lighting and the distance fields.
			Umbrella->bVisibleInRayTracing = false;
			Umbrella->bAffectDistanceFieldLighting = false;
			PersonKit::FPiece Shape;
			UmbrellaModel::Build(Shape, UmbrellaCloth);
			for (int32 s = 0; s < Shape.Sections.Num(); ++s)
			{
				const PersonKit::FSection& Section = Shape.Sections[s];
				Umbrella->CreateMeshSection_LinearColor(s, Section.Vertices, Section.Triangles, Section.Normals,
					TArray<FVector2D>(), TArray<FLinearColor>(), TArray<FProcMeshTangent>(), false);
				if (BaseMaterial)
				{
					UMaterialInstanceDynamic* Paint = UMaterialInstanceDynamic::Create(BaseMaterial, this);
					Paint->SetVectorParameterValue(TEXT("Color"), Section.Color);
					Umbrella->SetMaterial(s, Paint);
				}
			}
			Umbrella->RegisterComponent();
			AddInstanceComponent(Umbrella);
		}
		Umbrella->SetVisibility(true);
	}
	else
	{
		DropUmbrella();
	}
}

void APedestrian::PlaceUmbrella(int32 HoldingArm, float Raised)
{
	const int32 ForearmIndex = PersonKit::ForearmOf(PersonKit::ArmLeft + HoldingArm);
	if (!Umbrella || !Pieces.IsValidIndex(ForearmIndex) || !Pieces[ForearmIndex])
	{
		return;
	}
	UProceduralMeshComponent* Forearm = Pieces[ForearmIndex];
	Forearm->SetRelativeRotation(FRotator(Pedestrians::UmbrellaBend * Raised, 0.f, 0.f));

	// The grip in his fist (where PersonKit puts the hand, from the elbow).
	const double Side = HoldingArm == 1 ? 1.0 : -1.0;
	const double Tall = Outfit.Height / 178.0;
	const FVector Grip = Forearm->GetComponentTransform().TransformPosition(FVector(1.4, 2.3 * Side, -27.5 * Tall));
	const FVector InHand = Capsule->GetComponentTransform().InverseTransformPosition(Grip);

	// Furled while the arm comes up, then spread over his head, tipped back and in.
	const float Spread = FMath::SmoothStep(0.f, 1.f, FMath::Clamp(UmbrellaOpen * 2.f - 1.f, 0.f, 1.f));
	const double Width = FMath::Lerp(UmbrellaModel::Furled, 1.0, static_cast<double>(Spread));
	// Before that it hangs furled from his hand like a walking stick, and swings up as the arm rises.
	const float Swung = FMath::Lerp(Pedestrians::UmbrellaHang, Pedestrians::UmbrellaTilt, Raised);
	Umbrella->SetRelativeLocationAndRotation(InHand, FRotator(Swung, 0.f, Pedestrians::UmbrellaLean * Side * Raised));
	Umbrella->SetRelativeScale3D(FVector(Width, Width, 1.0));
}

void APedestrian::DropUmbrella()
{
	if (UmbrellaOpen <= 0.f && (!Umbrella || !Umbrella->IsVisible()) && !bFurledInUse)
	{
		return;
	}
	UmbrellaOpen = 0.f;
	if (Umbrella)
	{
		Umbrella->SetVisibility(false);
	}
	PersonModel::PoseElbows(Pieces, 0.f, 0.f);
	if (bFurledInUse)
	{
		bFurledInUse = false;
		PersonKit::FOutfit Carried = Outfit;
		Carried.Held = PersonKit::EHeld::Umbrella;
		SetOutfit(Carried);
	}
}

void APedestrian::Hold(const FVector& Location, const FRotator& Rotation, float Legs, float Knees, float Arms)
{
	Unfold();
	DropUmbrella();
	SetActorLocationAndRotation(Location, Rotation);
	// Not quite the same on both sides, so he does not look like a doll.
	PersonModel::PoseLegs(Pieces, Legs, Legs * 0.6f, Knees, Knees * 0.8f);
	for (int32 i = 0; i < 2; ++i)
	{
		if (USceneComponent* Part = Limb(PersonKit::ArmLeft + i))
		{
			Part->SetRelativeRotation(FRotator(Arms * (i == 0 ? 1.f : 0.85f), 0.f, 0.f));
		}
	}
}

void APedestrian::PointTo(const FVector& Location, double Yaw, double PointYaw, bool bPointing)
{
	Unfold();
	DropUmbrella();
	SetActorLocationAndRotation(Location, FRotator(0.0, Yaw, 0.0));
	PersonModel::PoseLegs(Pieces, 0.f, 0.f);
	PersonModel::PoseElbows(Pieces, 0.f, 0.f);
	// The arm on the side of the way, turned from hanging down to along it
	// (in his own frame: X ahead, Y to his right).
	const double Side = FRotator::NormalizeAxis(PointYaw - Yaw);
	const int32 PointingArm = Side >= 0.0 ? 1 : 0;
	const double Radians = FMath::DegreesToRadians(Side);
	const FVector Aim = FVector(FMath::Cos(Radians), FMath::Sin(Radians), -Pedestrians::PointDown).GetSafeNormal();
	for (int32 i = 0; i < 2; ++i)
	{
		if (USceneComponent* Part = Limb(PersonKit::ArmLeft + i))
		{
			if (bPointing && i == PointingArm)
			{
				Part->SetRelativeRotation(FQuat::FindBetweenNormals(FVector(0.0, 0.0, -1.0), Aim));
			}
			else
			{
				Part->SetRelativeRotation(FRotator::ZeroRotator);
			}
		}
	}
}

void APedestrian::Huddle(const FVector& Location, double Yaw)
{
	DropUmbrella();
	SetActorLocationAndRotation(Location, FRotator(0.0, Yaw, 0.0));
	PersonModel::PoseLegs(Pieces, 0.f, 0.f);
	// Upper arms a little forward, forearms across the chest, one over the other.
	for (int32 i = 0; i < 2; ++i)
	{
		const float Side = i == 0 ? 1.f : -1.f;
		if (USceneComponent* Upper = Limb(PersonKit::ArmLeft + i))
		{
			Upper->SetRelativeRotation(FRotator(Pedestrians::FoldUpper, 0.f, Pedestrians::FoldIn * Side));
		}
		if (USceneComponent* Lower = Limb(PersonKit::ForearmLeft + i))
		{
			Lower->SetRelativeRotation(FRotator(Pedestrians::FoldElbow + (i == 0 ? 0.f : Pedestrians::FoldOver), Pedestrians::FoldAcross * Side, 0.f));
		}
	}
	bArmsFolded = true;
}

void APedestrian::Unfold()
{
	if (bArmsFolded)
	{
		PersonModel::PoseElbows(Pieces, 0.f, 0.f);
		bArmsFolded = false;
	}
}

APedestrians::APedestrians()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	BaseMaterial = CarBody::LoadPaintMaterial();
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	CylinderMesh = Cylinder.Object;
}

void APedestrians::BeginPlay()
{
	Super::BeginPlay();

	Random.Initialize(1932);
	bReady = LoadLines();
	if (BaseMaterial)
	{
		BloodPaint = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		BloodPaint->SetVectorParameterValue(TEXT("Color"), Pedestrians::BloodColor);
	}
}

bool APedestrians::LoadLines()
{
	const FString Path = FPaths::Combine(FPaths::ProjectDir(), DataFile);
	FString Text;
	TSharedPtr<FJsonObject> Root;
	if (!FFileHelper::LoadFileToString(Text, *Path)
		|| !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) || !Root.IsValid())
	{
		UE_LOG(LogPedestrians, Error, TEXT("Could not read %s"), *Path);
		return false;
	}
	const TArray<TSharedPtr<FJsonValue>>* Rows = nullptr;
	if (!Root->TryGetArrayField(TEXT("sidewalks"), Rows))
	{
		UE_LOG(LogPedestrians, Warning, TEXT("No sidewalks in %s"), *Path);
		return false;
	}

	// Which rows go over the road.
	TSet<int32> CrossingRows;
	const TArray<TSharedPtr<FJsonValue>>* Crossings = nullptr;
	if (Root->TryGetArrayField(TEXT("crossings"), Crossings))
	{
		for (const TSharedPtr<FJsonValue>& Crossing : *Crossings)
		{
			CrossingRows.Add(static_cast<int32>(Crossing->AsNumber()));
		}
	}

	int32 RowIndex = -1;
	for (const TSharedPtr<FJsonValue>& Row : *Rows)
	{
		++RowIndex;
		const TArray<TSharedPtr<FJsonValue>>& Numbers = Row->AsArray();
		FLine Line;
		Line.bCrossing = CrossingRows.Contains(RowIndex);
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
		Ends.Add(Pedestrians::CellOf(Lines[Index].Points[0]), FIntPoint(Index, 0));
		Ends.Add(Pedestrians::CellOf(Lines[Index].Points.Last()), FIntPoint(Index, 1));
	}
	UE_LOG(LogPedestrians, Log, TEXT("Pedestrians: %d walking lines"), Lines.Num());
	return Lines.Num() > 0;
}

bool APedestrians::CameraView(FVector& OutLocation, FVector2D& OutLooking) const
{
	const APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC || !PC->PlayerCameraManager)
	{
		return false;
	}
	OutLocation = PC->PlayerCameraManager->GetCameraLocation();
	OutLooking = FVector2D(PC->PlayerCameraManager->GetCameraRotation().Vector()).GetSafeNormal();
	return true;
}

FVector2D APedestrians::PointOn(const FLine& Line, double Along, FVector2D& OutTangent) const
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

bool APedestrians::PickSpot(const FVector& Camera, const FVector2D& Looking, bool bInView, double MinDistance,
	int32& OutLine, double& OutAlong, double& OutGroundZ)
{
	if (FVector::Dist2D(Camera, NearLinesFrom) > Pedestrians::RefindLines)
	{
		NearLinesFrom = Camera;
		NearLines.Reset();
		NearLongest = 1.0;
		for (int32 i = 0; i < Lines.Num(); ++i)
		{
			for (const FVector2D& Point : Lines[i].Points)
			{
				if (FVector2D::Distance(Point, FVector2D(Camera)) < Range + Lines[i].Length)
				{
					NearLines.Add(i);
					NearLongest = FMath::Max(NearLongest, Lines[i].Length);
					break;
				}
			}
		}
	}
	if (NearLines.IsEmpty())
	{
		return false;
	}
	for (int32 Try = 0; Try < Pedestrians::PickTries; ++Try)
	{
		// A long line gets more people than a short one, so that they spread
		// evenly along the pavements rather than crowd the short bits.
		const int32 Index = NearLines[Random.RandRange(0, NearLines.Num() - 1)];
		if (Random.FRand() > Lines[Index].Length / NearLongest)
		{
			continue;
		}
		const double Along = Random.FRandRange(0.f, 1.f) * Lines[Index].Length;
		FVector2D Tangent;
		const FVector2D At = PointOn(Lines[Index], Along, Tangent);
		const FVector2D Away = At - FVector2D(Camera);
		const double Distance = Away.Size();
		if (Distance < MinDistance || Distance >= Range)
		{
			continue;
		}
		// Not on top of someone already walking there.
		bool bCrowded = false;
		for (const FWalker& Other : Walkers)
		{
			if (Other.bPlaced && FVector2D::DistSquared(Other.Shown, At) < FMath::Square(Spacing))
			{
				bCrowded = true;
				break;
			}
		}
		if (bCrowded)
		{
			continue;
		}
		const bool bBehind = FVector2D::DotProduct(Away / FMath::Max(Distance, 1.0), Looking) < Pedestrians::OutOfViewDot;
		// Only where the ground is known, so he is on it from the first frame.
		if (GroundUnder(At, Camera.Z, OutGroundZ) && (bInView || bBehind || BehindBuilding(Camera, At, OutGroundZ)))
		{
			OutLine = Index;
			OutAlong = Along;
			return true;
		}
	}
	return false;
}

void APedestrians::TurnAtEnd(FWalker& Walker)
{
	const FLine& Line = Lines[Walker.Line];
	const FVector2D End = Walker.Dir > 0 ? Line.Points.Last() : Line.Points[0];
	const FIntPoint Cell = Pedestrians::CellOf(End);

	TArray<FIntPoint> Joins;
	for (int32 dx = -1; dx <= 1; ++dx)
	{
		for (int32 dy = -1; dy <= 1; ++dy)
		{
			TArray<FIntPoint> Found;
			Ends.MultiFind(Cell + FIntPoint(dx, dy), Found);
			for (const FIntPoint& Other : Found)
			{
				const FLine& Next = Lines[Other.X];
				const FVector2D Start = Other.Y == 0 ? Next.Points[0] : Next.Points.Last();
				if (Other.X != Walker.Line && FVector2D::Distance(Start, End) < Pedestrians::JoinDistance)
				{
					Joins.Add(Other);
				}
			}
		}
	}

	if (Joins.IsEmpty())
	{
		// A dead end: turn round.
		Walker.Dir = -Walker.Dir;
		return;
	}
	const FIntPoint Join = Joins[Random.RandRange(0, Joins.Num() - 1)];
	Walker.Line = Join.X;
	Walker.Dir = Join.Y == 0 ? 1 : -1;
	Walker.Along = Join.Y == 0 ? 0.0 : Lines[Join.X].Length;
	// Over the road: first a look both ways at the kerb.
	Walker.bWaiting = Lines[Join.X].bCrossing;
	Walker.bRushing = false;
	Walker.Waited = 0.f;
	Walker.RoadClock = 0.f;
}

void APedestrians::GatherMovers(const FVector& Camera)
{
	Movers.Reset();
	auto AddMover = [this, &Camera](const AActor* Thing, double Speed)
	{
		if (Speed > Pedestrians::MoverSlowest && FVector::Dist2D(Thing->GetActorLocation(), Camera) < Pedestrians::MoversNear)
		{
			FMover& Mover = Movers.AddDefaulted_GetRef();
			Mover.At = FVector2D(Thing->GetActorLocation());
			Mover.Velocity = FVector2D(Thing->GetActorForwardVector()).GetSafeNormal() * Speed;
		}
	};
	for (TActorIterator<ATrafficCar> It(GetWorld()); It; ++It)
	{
		AddMover(*It, It->GetDrivingSpeed());
	}
	for (TActorIterator<ATram> It(GetWorld()); It; ++It)
	{
		if (!It->IsHidden())
		{
			AddMover(*It, It->GetDrivingSpeed());
		}
	}
	// The player, at the wheel or on a bicycle, by how he really moves.
	if (const AActor* Player = LastPlayer.Get(); Player && !Player->IsA<APestiPerson>() && PlayerVelocity.Size2D() > Pedestrians::MoverSlowest
		&& FVector::Dist2D(PlayerAt, Camera) < Pedestrians::MoversNear)
	{
		FMover& Mover = Movers.AddDefaulted_GetRef();
		Mover.At = FVector2D(PlayerAt);
		Mover.Velocity = FVector2D(PlayerVelocity);
	}
}

bool APedestrians::IsRoadBusy(const FLine& Line) const
{
	const FVector2D From = Line.Points[0];
	const FVector2D To = Line.Points.Last();
	for (const FMover& Mover : Movers)
	{
		const FVector2D Near = FMath::ClosestPointOnSegment2D(Mover.At, From, To);
		const FVector2D Ahead = Near - Mover.At;
		const double Distance = Ahead.Size();
		if (Distance < Pedestrians::RoadNear)
		{
			return true;
		}
		const double Speed = Mover.Velocity.Size();
		if (FVector2D::DotProduct(Ahead, Mover.Velocity) > 0.0 && Distance < Speed * Pedestrians::RoadClearSeconds)
		{
			return true;
		}
	}
	return false;
}

bool APedestrians::BehindBuilding(const FVector& Camera, const FVector2D& At, double GroundZ) const
{
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(PedestrianSight), false, this);
	const FVector Eye(At, GroundZ + Pedestrians::EyeHeight);
	return GetWorld()->LineTraceSingleByChannel(Hit, Camera, Eye, ECC_Visibility, Params)
		&& Cast<AOsmGraybox>(Hit.GetActor()) != nullptr
		&& FVector::Dist(Hit.ImpactPoint, Eye) > 100.0;
}

bool APedestrians::GroundUnder(const FVector2D& At, double Near, double& OutZ) const
{
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(PedestrianGround), false, this);
	const double Top = Near + 300.0;
	if (!GetWorld()->LineTraceSingleByChannel(Hit, FVector(At, Top), FVector(At, Near - 2000.0), ECC_Visibility, Params))
	{
		return false;
	}
	// Only the open ground: not a lamp, a bench or a car in the way.
	if (!Cast<AOsmGraybox>(Hit.GetActor()))
	{
		return false;
	}
	OutZ = Hit.ImpactPoint.Z;
	return true;
}

void APedestrians::Shout(const TCHAR* Hu, const TCHAR* En)
{
	const double Now = GetWorld()->GetTimeSeconds();
	APestiHUD* Hud = APestiHUD::Get(this);
	if (!Hud || Hud->IsSaying() || Now - LastShout < Pedestrians::ShoutEvery)
	{
		return;
	}
	LastShout = Now;
	Hud->Say(TEXT("J\u00e1r\u00f3kel\u0151"), Hu, En, 3.f);
}

void APedestrians::PickAFight(APawn* Player)
{
	APestiPerson* Person = Cast<APestiPerson>(Player);
	if (!AStreetFights::WantsToPunch(Person))
	{
		return;
	}
	// The nearest one standing or walking within reach, more or less in front of him.
	int32 Picked = INDEX_NONE;
	double Nearest = FMath::Square(Pedestrians::FightReach);
	for (int32 i = 0; i < Walkers.Num(); ++i)
	{
		const FWalker& Walker = Walkers[i];
		const bool bFree = Walker.React == EReact::None || Walker.React == EReact::Aside || Walker.React == EReact::Stumble
			|| Walker.React == EReact::Point || Walker.React == EReact::Flee;
		if (!IsValid(Walker.Person) || !Walker.bPlaced || !bFree
			|| FMath::Abs(Walker.GroundZ + Pedestrians::Middle - PlayerAt.Z) > Pedestrians::AskHeight)
		{
			continue;
		}
		const FVector2D Offset = Walker.Shown - FVector2D(PlayerAt);
		const double Gap = Offset.SizeSquared();
		if (Gap < Nearest && FVector2D::DotProduct(Offset.GetSafeNormal(), PlayerForward) > Pedestrians::FightFacing)
		{
			Nearest = Gap;
			Picked = i;
		}
	}
	if (Picked == INDEX_NONE)
	{
		return;
	}
	FWalker& Walker = Walkers[Picked];
	AStreetFights* Fights = AStreetFights::Get(this);
	// One sitting (the beggar, the flower woman) gets up to fight: the fighter is dressed standing.
	PersonKit::FOutfit Dress = Walker.Person->GetOutfit();
	Dress.bSeated = false;
	if (!Fights || !Fights->Begin(Person, Dress, FVector(Walker.Shown, Walker.GroundZ), Walker.ShownYaw))
	{
		return;
	}
	LeaveGawk(Walker);
	// The pickpocket: beaten, he hands the wallet back.
	if (Walker.Stolen > 0)
	{
		Fights->SetLoot(Walker.Stolen);
		Walker.Stolen = 0;
	}
	if (Walker.bStander)
	{
		// One who keeps to a spot is only hidden while the fighter stands in
		// for him; TickStander puts him back on his spot once the player is far off.
		ClearReact(Walker);
		Walker.bPlaced = false;
		Walker.Person->SetActorHiddenInGame(true);
		return;
	}
	// The fighter stands where he was: the walker himself is gone (another comes along out of sight).
	LeaveShelter(Walker);
	if (Walker.Blood)
	{
		Walker.Blood->DestroyComponent();
	}
	Walker.Person->Destroy();
	Walkers.RemoveAt(Picked);
}

void APedestrians::AskTheWay(APawn* Player, double Now)
{
	if (!Player->IsA<APestiPerson>())
	{
		return;
	}
	const APlayerController* PC = Cast<APlayerController>(Player->GetController());
	if (!PC || !PC->WasInputKeyJustPressed(EKeys::Q))
	{
		return;
	}
	FWalker* Asked = nullptr;
	double Nearest = FMath::Square(Pedestrians::AskReach);
	for (FWalker& Walker : Walkers)
	{
		const bool bFree = Walker.React == EReact::None || Walker.React == EReact::Flee || Walker.React == EReact::Point;
		if (!IsValid(Walker.Person) || !Walker.bPlaced || !bFree
			|| FMath::Abs(Walker.GroundZ + Pedestrians::Middle - PlayerAt.Z) > Pedestrians::AskHeight)
		{
			continue;
		}
		const double Gap = FVector2D::DistSquared(Walker.Shown, FVector2D(PlayerAt));
		if (Gap < Nearest)
		{
			Nearest = Gap;
			Asked = &Walker;
		}
	}
	APestiHUD* Hud = APestiHUD::Get(this);
	if (!Asked || !Hud)
	{
		return;
	}

	// Where the player should be going: the mission's goal, if there is one.
	FVector Target = FVector::ZeroVector;
	FString Place;
	bool bKnows = false;
	for (TActorIterator<AMission> It(GetWorld()); It; ++It)
	{
		if (It->GetWayTo(Target, Place))
		{
			bKnows = true;
			break;
		}
	}
	StartReact(*Asked, EReact::Point, Pedestrians::AskSeconds, FVector2D::ZeroVector);
	Asked->bPointing = bKnows;
	LastShout = Now;
	const TCHAR* Who = TEXT("J\u00e1r\u00f3kel\u0151");
	if (!bKnows)
	{
		const Pedestrians::FLine2& Line = Pedestrians::DontKnowLines[Random.RandRange(0, static_cast<int32>(UE_ARRAY_COUNT(Pedestrians::DontKnowLines)) - 1)];
		Hud->Say(Who, Line.Hu, Line.En, Pedestrians::AskSeconds);
		return;
	}
	double Distance = 0.0;
	Asked->PointYaw = WayFrom(Asked->Shown, FVector2D(Target), Distance);
	const Pedestrians::FLine2& Line = Distance < Pedestrians::NearBy ? Pedestrians::NearLine
		: Distance < Pedestrians::NotFar ? Pedestrians::NotFarLine : Pedestrians::FarLine;
	if (Place.IsEmpty())
	{
		Hud->Say(Who, Line.Hu, Line.En, Pedestrians::AskSeconds);
	}
	else
	{
		Hud->Say(Who, Place + TEXT("? ") + Line.Hu, Place + TEXT("? ") + Line.En, Pedestrians::AskSeconds);
	}
}

float APedestrians::WayFrom(const FVector2D& From, const FVector2D& To, double& OutDistance) const
{
	OutDistance = FVector2D::Distance(From, To);
	FVector2D Toward = To;
	TActorIterator<ATrafficManager> Streets(GetWorld());
	TArray<FVector> Route;
	if (OutDistance > Pedestrians::PointStraight && Streets
		&& Streets->FindPath(FVector(From, PlayerAt.Z), FVector(To, PlayerAt.Z), Route) && Route.Num() > 0)
	{
		Route.Add(FVector(To, PlayerAt.Z));
		// The plan starts at the street corner nearest him, which may be
		// behind him: go on from the part of the way he stands nearest.
		int32 First = 0;
		double Closest = TNumericLimits<double>::Max();
		for (int32 i = 0; i + 1 < Route.Num(); ++i)
		{
			const FVector2D Near = FMath::ClosestPointOnSegment2D(From, FVector2D(Route[i]), FVector2D(Route[i + 1]));
			const double Gap = FVector2D::DistSquared(From, Near);
			if (Gap < Closest)
			{
				Closest = Gap;
				First = i + 1;
			}
		}
		// Point along the street at the first corner a little way off, not at his feet.
		Toward = FVector2D(Route[First]);
		for (int32 i = First; i < Route.Num(); ++i)
		{
			Toward = FVector2D(Route[i]);
			if (FVector2D::Distance(From, Toward) >= Pedestrians::PointAhead)
			{
				break;
			}
		}
		// How far it is along the streets.
		OutDistance = FVector2D::Distance(From, FVector2D(Route[First]));
		for (int32 i = First; i + 1 < Route.Num(); ++i)
		{
			OutDistance += FVector2D::Distance(FVector2D(Route[i]), FVector2D(Route[i + 1]));
		}
	}
	const FVector2D Way = Toward - From;
	return static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(Way.Y, Way.X)));
}

void APedestrians::StartReact(FWalker& Walker, EReact React, float Seconds, const FVector2D& Shove)
{
	Walker.React = React;
	Walker.ReactLeft = Seconds;
	Walker.ReactTime = 0.f;
	Walker.Shove = Shove;
	Walker.NoticeAt = -1.f;
	Walker.bFrozen = false;
}

void APedestrians::ClearReact(FWalker& Walker)
{
	StartReact(Walker, EReact::None, 0.f, FVector2D::ZeroVector);
	if (Walker.Blood)
	{
		Walker.Blood->SetVisibility(false);
	}
}

void APedestrians::ReactToPlayer(FWalker& Walker, double Now)
{
	if (Walker.React == EReact::Down || Walker.React == EReact::GetUp || Walker.React == EReact::Dead)
	{
		return;
	}
	const FVector2D Offset = Walker.Shown - FVector2D(PlayerAt);
	if (Offset.SizeSquared() > FMath::Square(3000.0) || FMath::Abs(Walker.GroundZ - PlayerAt.Z) > 400.0)
	{
		Walker.NoticeAt = -1.f;
		return;
	}
	const double Speed = PlayerVelocity.Size2D();
	const FVector2D Going = FVector2D(PlayerVelocity).GetSafeNormal();

	if (Walker.bSeated)
	{
		// Sitting, he only tells you off when walked into, or cries out at a car coming near.
		if (bPlayerDriving)
		{
			if (Speed > Pedestrians::CarNoticeSpeed && Offset.Size() < Pedestrians::SeatedCarNear)
			{
				const Pedestrians::FLine2& Line = Pedestrians::LeapLines[Random.RandRange(0, static_cast<int32>(UE_ARRAY_COUNT(Pedestrians::LeapLines)) - 1)];
				Shout(Line.Hu, Line.En);
			}
		}
		else if (Speed > Pedestrians::BumpSpeed && Offset.Size() < Pedestrians::BumpDistance + 30.0)
		{
			const Pedestrians::FLine2& Line = Pedestrians::BumpLines[Random.RandRange(0, static_cast<int32>(UE_ARRAY_COUNT(Pedestrians::BumpLines)) - 1)];
			Shout(Line.Hu, Line.En);
		}
		return;
	}

	if (bPlayerDriving)
	{
		const FVector2D Right(-PlayerForward.Y, PlayerForward.X);
		const double Ahead = FVector2D::DotProduct(Offset, PlayerForward);
		const double Aside = FVector2D::DotProduct(Offset, Right);
		if (Speed > Pedestrians::HitSpeed && FMath::Abs(Ahead) < Pedestrians::CarHalfLength && FMath::Abs(Aside) < Pedestrians::CarHalfWidth)
		{
			RunOver(Walker, Speed);
			return;
		}
		if (Walker.React == EReact::Leap || Walker.React == EReact::Flee)
		{
			return;
		}

		// The car coming at him (forwards, or backing up).
		const double Front = FVector2D::DotProduct(FVector2D(PlayerVelocity), PlayerForward) >= 0.0 ? Ahead : -Ahead;
		if (Speed > Pedestrians::CarNoticeSpeed && Front > 0.0
			&& Front < Speed * Pedestrians::CarWarnSeconds + Pedestrians::CarWarnExtra + Pedestrians::CarHalfLength
			&& FMath::Abs(Aside) < Pedestrians::CarPathHalfWidth)
		{
			if (Walker.NoticeAt < 0.f)
			{
				// He takes a moment to see it, and may stand frozen in fright.
				Walker.NoticeAt = static_cast<float>(Now) + Random.FRandRange(Pedestrians::MinNotice, Pedestrians::MaxNotice);
				Walker.bFrozen = Random.FRand() < Pedestrians::FreezeChance;
			}
			if (Now >= Walker.NoticeAt && !Walker.bFrozen)
			{
				const double Side = FMath::Abs(Aside) > 10.0 ? FMath::Sign(Aside) : (Random.FRand() < 0.5f ? -1.0 : 1.0);
				StartReact(Walker, EReact::Leap, 0.7f, Right * (Side * Pedestrians::LeapSpeed));
				const Pedestrians::FLine2& Line = Pedestrians::LeapLines[Random.RandRange(0, static_cast<int32>(UE_ARRAY_COUNT(Pedestrians::LeapLines)) - 1)];
				Shout(Line.Hu, Line.En);
			}
			return;
		}
		Walker.NoticeAt = -1.f;
		Walker.bFrozen = false;

		// One who keeps to his spot jumps away from a car close by.
		if (Walker.bStander)
		{
			if (Walker.React == EReact::None && Speed > Pedestrians::CarNoticeSpeed * 0.5 && Offset.Size() < Pedestrians::StanderCarNear)
			{
				StartReact(Walker, EReact::Leap, 0.7f, Offset.GetSafeNormal() * Pedestrians::LeapSpeed);
				const Pedestrians::FLine2& Line = Pedestrians::LeapLines[Random.RandRange(0, static_cast<int32>(UE_ARRAY_COUNT(Pedestrians::LeapLines)) - 1)];
				Shout(Line.Hu, Line.En);
			}
			return;
		}

		// The car up on the pavement near him: he runs off along it.
		if (Walker.React == EReact::None && Speed > Pedestrians::CarNoticeSpeed * 0.5 && Offset.Size() < Pedestrians::PavementNear)
		{
			FVector2D Tangent;
			const FVector2D OnLine = PointOn(Lines[Walker.Line], Walker.Along, Tangent);
			const FVector2D FromLine = FVector2D(PlayerAt) - OnLine;
			if (FMath::Abs(FromLine.X * Tangent.Y - FromLine.Y * Tangent.X) < Pedestrians::OnPavement)
			{
				Walker.Dir = FVector2D::DotProduct(Tangent, Offset) >= 0.0 ? 1 : -1;
				StartReact(Walker, EReact::Flee, Pedestrians::FleeSeconds, FVector2D::ZeroVector);
				const Pedestrians::FLine2& Line = Pedestrians::LeapLines[Random.RandRange(0, static_cast<int32>(UE_ARRAY_COUNT(Pedestrians::LeapLines)) - 1)];
				Shout(Line.Hu, Line.En);
			}
		}
		return;
	}

	// On foot: walked into, he stumbles away and tells you off.
	const double Distance = Offset.Size();
	if (Speed > Pedestrians::BumpSpeed && Distance < Pedestrians::BumpDistance
		&& (Walker.React == EReact::None || Walker.React == EReact::Aside))
	{
		const FVector2D Away = (Going * 0.7 + Offset.GetSafeNormal() * 0.3).GetSafeNormal();
		StartReact(Walker, EReact::Stumble, 0.9f, Away * 220.0);
		const Pedestrians::FLine2& Line = Pedestrians::BumpLines[Random.RandRange(0, static_cast<int32>(UE_ARRAY_COUNT(Pedestrians::BumpLines)) - 1)];
		Shout(Line.Hu, Line.En);
		return;
	}
	// Running at him: he steps out of the way.
	if (Walker.React == EReact::None && Speed > Pedestrians::RunningSpeed && Distance < Pedestrians::RunPastDistance
		&& FVector2D::DotProduct(Offset, Going) > 0.0)
	{
		const FVector2D Right(-Going.Y, Going.X);
		const double Side = FVector2D::DotProduct(Offset, Right) >= 0.0 ? 1.0 : -1.0;
		StartReact(Walker, EReact::Aside, 0.8f, Right * (Side * 170.0));
	}
}

void APedestrians::RunOver(FWalker& Walker, double Speed)
{
	const bool bKilled = Speed > Pedestrians::KillSpeed;
	const FVector2D Going = FVector2D(PlayerVelocity).GetSafeNormal();
	const FVector2D Right(-Going.Y, Going.X);
	const double Side = FVector2D::DotProduct(Walker.Shown - FVector2D(PlayerAt), Right) >= 0.0 ? 1.0 : -1.0;
	// Thrown on ahead of the car and a little out to the side he was on.
	const FVector2D Thrown = (Going + Right * (Side * 0.5)).GetSafeNormal() * FMath::Min(Speed * 0.5, 700.0);
	StartReact(Walker, bKilled ? EReact::Dead : EReact::Down, Pedestrians::FallSeconds + Pedestrians::LieSeconds, Thrown);
	// He falls on his back, away from the car: so he faces it.
	Walker.ShownYaw = static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(-Going.Y, -Going.X)));
	LeaveGawk(Walker);
	if (!bKilled)
	{
		// People come over to see if he is all right.
		AddGathering(FVector(Walker.Shown + Thrown, Walker.GroundZ), Pedestrians::CrowdKnockedDown, false);
	}

	if (APestiCar* Car = Cast<APestiCar>(LastPlayer.Get()))
	{
		Car->ForwardSpeed *= Pedestrians::CarSlowOnHit;
	}

	// Word gets round: it costs him some of his street name.
	if (APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this))
	{
		Game->AddRenown(bKilled ? StreetName::ForRunningOver : StreetName::ForKnockingDown, TEXT("elg\u00e1zolt valakit"), TEXT("ran someone down"));
	}

	// A police car or a constable close by sees it.
	if (APolicePatrols::Witnessed(this, PlayerAt, Pedestrians::PoliceSee))
	{
		if (APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this))
		{
			Game->ReportCrime(bKilled ? 2 : 1);
		}
	}

	if (bKilled)
	{
		// Those who saw it cry out and run.
		Shout(Pedestrians::WitnessLine.Hu, Pedestrians::WitnessLine.En);
		for (FWalker& Other : Walkers)
		{
			if (&Other != &Walker && IsValid(Other.Person) && !Other.bIndoors && (Other.React == EReact::None || Other.React == EReact::Aside)
				&& FVector2D::Distance(Other.Shown, Walker.Shown) < Pedestrians::WitnessNear)
			{
				if (Other.bStander)
				{
					if (!Other.bPlaced)
					{
						continue;
					}
					// Those keeping to a spot run a few steps off, then come back.
					if (!Other.bSeated)
					{
						StartReact(Other, EReact::Leap, 1.2f, (Other.Shown - Walker.Shown).GetSafeNormal() * Pedestrians::LeapSpeed * 0.7);
					}
					continue;
				}
				FVector2D Tangent;
				PointOn(Lines[Other.Line], Other.Along, Tangent);
				Other.Dir = FVector2D::DotProduct(Tangent, Other.Shown - Walker.Shown) >= 0.0 ? 1 : -1;
				StartReact(Other, EReact::Flee, Pedestrians::FleeSeconds, FVector2D::ZeroVector);
			}
		}
	}
}

bool APedestrians::PoseReacting(FWalker& Walker, float DeltaSeconds)
{
	if (Walker.React == EReact::None || Walker.React == EReact::Flee)
	{
		return false;
	}
	APedestrian* Person = Walker.Person;
	const double Size = Person->GetActorScale3D().Z;
	const double Standing = Walker.GroundZ + MiddleUp(Walker);
	const double Lying = Walker.GroundZ + Pedestrians::LyingHeight * Size;
	Walker.ReactTime += DeltaSeconds;
	Walker.ReactLeft -= DeltaSeconds;
	Walker.Shown += Walker.Shove * DeltaSeconds;
	const FVector Location(Walker.Shown, Standing);

	switch (Walker.React)
	{
	case EReact::Aside:
	case EReact::Leap:
	{
		const bool bLeap = Walker.React == EReact::Leap;
		if (bLeap)
		{
			// Turns to jump the way he goes.
			const float Toward = static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(Walker.Shove.Y, Walker.Shove.X)));
			Walker.ShownYaw = FMath::FixedTurn(Walker.ShownYaw, Toward, 900.f * DeltaSeconds);
		}
		Person->WalkTo(Location, Walker.ShownYaw, bLeap ? 400.f : 180.f, DeltaSeconds, bLeap ? 1.f : 0.f);
		if (Walker.ReactLeft <= 0.f)
		{
			StartReact(Walker, EReact::None, 0.f, FVector2D::ZeroVector);
		}
		return true;
	}
	case EReact::Stumble:
	{
		Walker.Shove *= FMath::Max(0.0, 1.0 - 3.0 * DeltaSeconds);
		// Pushed from behind he pitches forward, from in front he reels back.
		const FVector2D Facing = FVector2D(FRotator(0.0, Walker.ShownYaw, 0.0).Vector());
		const float Back = FVector2D::DotProduct(Facing, Walker.Shove) > 0.0 ? -1.f : 1.f;
		const float Wave = FMath::Sin(PI * FMath::Clamp(Walker.ReactTime / 0.9f, 0.f, 1.f));
		Person->Hold(Location, FRotator(Back * 18.f * Wave, Walker.ShownYaw, 0.f), -Back * 18.f * Wave, -25.f * Wave, 70.f * Wave);
		if (Walker.ReactLeft <= 0.f)
		{
			StartReact(Walker, EReact::None, 0.f, FVector2D::ZeroVector);
		}
		return true;
	}
	case EReact::Down:
	case EReact::Dead:
	{
		const bool bDead = Walker.React == EReact::Dead;
		if (Walker.ReactTime < Pedestrians::FallSeconds)
		{
			const float Fall = Walker.ReactTime / Pedestrians::FallSeconds;
			Walker.Shove *= FMath::Max(0.0, 1.0 - 2.0 * DeltaSeconds);
			Person->Hold(FVector(Walker.Shown, FMath::Lerp(Standing, Lying, static_cast<double>(Fall))),
				FRotator(88.f * Fall * Fall, Walker.ShownYaw, 0.f), 40.f * Fall, -30.f * Fall, 150.f * Fall);
			return true;
		}
		Walker.Shove = FVector2D::ZeroVector;
		const FVector Down(Walker.Shown, Lying);
		if (bDead)
		{
			// Still, twisted a little, the blood spreading under him.
			Person->Hold(Down, FRotator(88.f, Walker.ShownYaw, 8.f), 25.f, -55.f, 120.f);
			if (!Walker.Blood && CylinderMesh)
			{
				Walker.Blood = NewObject<UStaticMeshComponent>(this);
				Walker.Blood->SetupAttachment(RootComponent);
				Walker.Blood->SetStaticMesh(CylinderMesh);
				Walker.Blood->SetMaterial(0, BloodPaint);
				Walker.Blood->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				Walker.Blood->SetCastShadow(false);
				Walker.Blood->SetCanEverAffectNavigation(false);
				Walker.Blood->SetCachedMaxDrawDistance(Range);
				Walker.Blood->RegisterComponent();
				AddInstanceComponent(Walker.Blood);
			}
			if (Walker.Blood)
			{
				const float Spread = FMath::Sqrt(FMath::Clamp((Walker.ReactTime - Pedestrians::FallSeconds) / Pedestrians::BloodSpreads, 0.05f, 1.f));
				const double Radius = Pedestrians::BloodRadius * Spread;
				// Under his chest and head, which lie behind his middle.
				const FVector2D Behind = -FVector2D(FRotator(0.0, Walker.ShownYaw, 0.0).Vector()) * (35.0 * Size);
				Walker.Blood->SetVisibility(true);
				Walker.Blood->SetWorldLocationAndRotation(FVector(Walker.Shown + Behind, Walker.GroundZ + 0.6), FRotator(0.0, Walker.ShownYaw, 0.0));
				Walker.Blood->SetWorldScale3D(FVector(Radius / 50.0 * 1.25, Radius / 50.0, 0.004));
			}
			return true;
		}
		Person->Hold(Down, FRotator(88.f, Walker.ShownYaw, 0.f), 20.f, -30.f, 150.f);
		if (Walker.ReactLeft <= 0.f)
		{
			StartReact(Walker, EReact::GetUp, Pedestrians::GetUpSeconds, FVector2D::ZeroVector);
			const Pedestrians::FLine2& Line = Pedestrians::GetUpLines[Random.RandRange(0, static_cast<int32>(UE_ARRAY_COUNT(Pedestrians::GetUpLines)) - 1)];
			Shout(Line.Hu, Line.En);
		}
		return true;
	}
	case EReact::Point:
	{
		// Turns to the one asking, then shows the way; when the way lies
		// behind him he turns partly towards it.
		const FVector2D ToPlayer = FVector2D(PlayerAt) - Walker.Shown;
		float Facing = static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(ToPlayer.Y, ToPlayer.X)));
		if (Walker.bPointing)
		{
			const float Off = static_cast<float>(FRotator::NormalizeAxis(Walker.PointYaw - Facing));
			if (FMath::Abs(Off) > Pedestrians::MostAside)
			{
				Facing = Walker.PointYaw - FMath::Sign(Off) * Pedestrians::MostAside;
			}
		}
		Walker.ShownYaw = FMath::FixedTurn(Walker.ShownYaw, Facing, Pedestrians::AskTurnRate * DeltaSeconds);
		Person->PointTo(Location, Walker.ShownYaw, Walker.PointYaw, Walker.bPointing && Walker.ReactTime > Pedestrians::PointAfter);
		if (Walker.ReactLeft <= 0.f)
		{
			StartReact(Walker, EReact::None, 0.f, FVector2D::ZeroVector);
		}
		return true;
	}
	case EReact::GetUp:
	{
		// Sits up, crouches, and stands.
		const float Up = FMath::SmoothStep(0.f, 1.f, FMath::Clamp(Walker.ReactTime / Pedestrians::GetUpSeconds, 0.f, 1.f));
		const float Bent = 1.f - Up;
		Person->Hold(FVector(Walker.Shown, FMath::Lerp(Lying, Standing, static_cast<double>(Up))),
			FRotator(88.f * Bent * Bent, Walker.ShownYaw, 0.f), 75.f * Bent, -110.f * Bent, 45.f * Bent);
		if (Walker.ReactLeft <= 0.f)
		{
			StartReact(Walker, EReact::None, 0.f, FVector2D::ZeroVector);
		}
		return true;
	}
	default:
		return false;
	}
}

void APedestrians::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	FVector Camera;
	FVector2D Looking;
	if (!bReady || !CameraView(Camera, Looking))
	{
		return;
	}

	// Bring the crowd up to strength: close by at the start, out of the way later.
	const bool bFilling = GetWorld()->GetTimeSeconds() < 2.0;
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	for (int32 n = 0; n < Pedestrians::SpawnPerFrame && Walkers.Num() - NumStanders < Crowd; ++n)
	{
		FWalker Walker;
		if (!PickSpot(Camera, Looking, bFilling, bFilling ? Pedestrians::FillNear : AppearBeyond, Walker.Line, Walker.Along, Walker.GroundZ))
		{
			break;
		}
		Walker.bGroundKnown = true;
		Walker.Dir = Random.FRand() < 0.5f ? 1 : -1;
		Walker.Speed = Random.FRandRange(Pedestrians::MinSpeed, Pedestrians::MaxSpeed);
		Walker.Lane = Random.FRandRange(Pedestrians::MinLane, Pedestrians::MaxLane);
		FVector2D Tangent;
		const FVector2D At = PointOn(Lines[Walker.Line], Walker.Along, Tangent);
		Walker.Person = GetWorld()->SpawnActor<APedestrian>(APedestrian::StaticClass(), FVector(At, Camera.Z), FRotator::ZeroRotator, Params);
		if (!Walker.Person)
		{
			break;
		}
		Walker.Person->SetOutfit(PersonModel::RandomOutfit(Random));
		const PersonKit::FOutfit& Dressed = Walker.Person->GetOutfit();
		Walker.bHasUmbrella = Dressed.Held == PersonKit::EHeld::Umbrella || Random.FRand() < Pedestrians::UmbrellaShare;
		Walker.UmbrellaColor = UmbrellaModel::ClothColor(Random.FRand(), Dressed.bWoman);
		Walker.UmbrellaPatience = Random.FRand();
		Walker.RainShy = Random.FRand();
		Walker.ShelterClock = Random.FRand() * Pedestrians::ShelterLook;
		Walker.Nosy = Random.FRand();
		Walker.GawkClock = Random.FRand() * Pedestrians::GawkLook;
		Walkers.Add(Walker);
	}

	// Where the player is, on foot or at the wheel, and how fast he goes
	// (the car moves itself, without physics, so from how far it went).
	const double Now = GetWorld()->GetTimeSeconds();
	APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	const bool bPlayerAbout = Player && !Player->IsHidden() && DeltaSeconds > 0.f;
	if (bPlayerAbout)
	{
		const FVector Here = Player->GetActorLocation();
		if (LastPlayer.Get() != Player)
		{
			LastPlayer = Player;
			LastPlayerAt = Here;
		}
		PlayerVelocity = FMath::Lerp(PlayerVelocity, (Here - LastPlayerAt) / DeltaSeconds, 0.5);
		LastPlayerAt = Here;
		PlayerAt = Here;
		PlayerForward = FVector2D(Player->GetActorForwardVector()).GetSafeNormal();
		bPlayerDriving = Player->IsA<APestiCar>();
		AskTheWay(Player, Now);
		PickAFight(Player);
	}

	GatherMovers(Camera);

	const AWeather* Sky = AWeather::Get(GetWorld());
	const float RainNow = Sky ? Sky->GetRain() : 0.f;
	ComeBackLeft = Pedestrians::SpawnPerFrame;
	TickGatherings(Now, bPlayerAbout);

	for (FWalker& Walker : Walkers)
	{
		if (!IsValid(Walker.Person))
		{
			continue;
		}
		if (Walker.bStander)
		{
			TickStander(Walker, Camera, bPlayerAbout, Now, DeltaSeconds);
			continue;
		}

		// Gone indoors out of the rain: nothing more to do until he comes out again.
		if (Walker.bIndoors && TickRain(Walker, RainNow, Camera, Looking, DeltaSeconds))
		{
			continue;
		}

		// Fallen far behind: once nobody can see him, start again somewhere
		// out of the way nearer the camera. The run-over are cleared away
		// the same way, once out of sight.
		const double Away = FVector::Dist2D(Walker.Person->GetActorLocation(), Camera);
		const bool bSeen = Walker.Person->WasRecentlyRendered(Pedestrians::SeenWithin)
			|| (Walker.Blood && Walker.Blood->IsVisible() && Walker.Blood->WasRecentlyRendered(Pedestrians::SeenWithin));
		const bool bClearDead = Walker.React == EReact::Dead && Away > Pedestrians::ClearBeyond && !bSeen;
		if ((bClearDead || (Away > Range * 1.2 && (!bSeen || Away > Range * Pedestrians::GiveUpShare)))
			&& PickSpot(Camera, Looking, false, AppearBeyond, Walker.Line, Walker.Along, Walker.GroundZ))
		{
			LeaveGawk(Walker);
			Walker.Stolen = 0;
			Walker.bPlaced = false;
			ClearReact(Walker);
			Walker.bWaiting = false;
			Walker.bRushing = false;
		}

		if (bPlayerAbout && Walker.bPlaced)
		{
			ReactToPlayer(Walker, Now);
		}
		if (TickRain(Walker, RainNow, Camera, Looking, DeltaSeconds))
		{
			continue;
		}
		if (TickGawk(Walker, Now, DeltaSeconds))
		{
			continue;
		}
		const bool bOnWalk = Walker.React == EReact::None || Walker.React == EReact::Flee;
		// In the rain: up with the umbrella, or hurry along without one.
		const bool bUmbrellaUp = Walker.bHasUmbrella && Walker.Person->HasUmbrellaUp();
		Walker.Person->WantUmbrella(Walker.bHasUmbrella
			&& RainNow > (bUmbrellaUp ? Pedestrians::UmbrellaUntil : Pedestrians::UmbrellaFrom + Pedestrians::UmbrellaSpread * Walker.UmbrellaPatience),
			Walker.UmbrellaColor);
		const float Hurry = bUmbrellaUp ? 1.f : 1.f + Pedestrians::RainHurry * RainNow;
		// At a crossing: wait at the kerb while the road is busy, then go over
		// briskly, running if something comes after all.
		const bool bOnCrossing = Lines[Walker.Line].bCrossing;
		Walker.RoadClock -= DeltaSeconds;
		if (bOnCrossing && Walker.RoadClock <= 0.f)
		{
			Walker.RoadClock = Pedestrians::RoadLookEvery;
			const bool bBusy = IsRoadBusy(Lines[Walker.Line]);
			if (Walker.bWaiting)
			{
				Walker.bWaiting = bBusy && Walker.Waited < Pedestrians::MaxWait;
			}
			else
			{
				Walker.bRushing = bBusy;
			}
		}
		if (Walker.bWaiting)
		{
			Walker.Waited += DeltaSeconds;
		}
		const float CrossShare = !bOnCrossing ? 1.f : Walker.bWaiting ? 0.f : Walker.bRushing ? Pedestrians::CrossRun : Pedestrians::CrossHurry;
		const float Pace = Walker.Speed * Hurry * CrossShare * (Walker.React == EReact::Flee ? Pedestrians::FleeShare : 1.f);
		if (Walker.React == EReact::Flee)
		{
			Walker.ReactLeft -= DeltaSeconds;
			if (Walker.ReactLeft <= 0.f)
			{
				StartReact(Walker, EReact::None, 0.f, FVector2D::ZeroVector);
			}
		}

		if (bOnWalk)
		{
			Walker.Along += Walker.Dir * Pace * DeltaSeconds;
		}
		const FLine& Line = Lines[Walker.Line];
		if (Walker.Along < 0.0 || Walker.Along > Line.Length)
		{
			Walker.Along = FMath::Clamp(Walker.Along, 0.0, Line.Length);
			TurnAtEnd(Walker);
		}

		FVector2D Tangent;
		const FVector2D OnLine = PointOn(Lines[Walker.Line], Walker.Along, Tangent);
		const FVector2D Heading = Tangent * Walker.Dir;
		// Keep to the right, so people going opposite ways pass each other.
		const FVector2D At = OnLine + FVector2D(-Heading.Y, Heading.X) * Walker.Lane;

		Walker.GroundClock -= DeltaSeconds;
		if (!Walker.bGroundKnown || Walker.GroundClock <= 0.f)
		{
			Walker.GroundClock = Pedestrians::GroundEvery;
			double Z;
			if (GroundUnder(At, Walker.bGroundKnown ? Walker.GroundZ : Camera.Z, Z))
			{
				Walker.GroundZ = Z;
				Walker.bGroundKnown = true;
			}
		}

		// Walk over to his place rather than jump there (a new line at a
		// corner, the other side of the line after turning back), and turn
		// to face the way he goes.
		const float Yaw = static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(Heading.Y, Heading.X)));
		const FVector2D Gap = At - Walker.Shown;
		const double Step = FMath::Max(Pace, Walker.Speed) * Pedestrians::CatchUp * DeltaSeconds;
		if (!Walker.bPlaced || (bOnWalk && Gap.Size() > Pedestrians::SnapDistance))
		{
			Walker.Shown = At;
			Walker.ShownYaw = Yaw;
			Walker.bPlaced = true;
		}
		else if (bOnWalk)
		{
			Walker.Shown += Gap.Size() > Step ? Gap.GetSafeNormal() * Step : Gap;
			Walker.ShownYaw = FMath::FixedTurn(Walker.ShownYaw, Yaw, Pedestrians::TurnRate * DeltaSeconds * (Walker.React == EReact::Flee ? 3.f : 1.f));
		}

		if (!PoseReacting(Walker, DeltaSeconds))
		{
			const double Size = Walker.Person->GetActorScale3D().Z;
			Walker.Person->WalkTo(FVector(Walker.Shown, Walker.GroundZ + Pedestrians::Middle * Size), Walker.ShownYaw, Pace, DeltaSeconds,
				Walker.React == EReact::Flee || (bOnCrossing && Walker.bRushing) ? 1.f : 0.f);
		}
	}
}

bool APedestrians::TickRain(FWalker& Walker, float RainNow, const FVector& Camera, const FVector2D& Looking, float DeltaSeconds)
{
	APedestrian* Person = Walker.Person;

	// Gone indoors out of a downpour: back out somewhere out of sight once it eases.
	const bool bWantsIn = Walker.RainShy < Pedestrians::IndoorsShare * RainNow;
	if (Walker.bIndoors)
	{
		if (bWantsIn || ComeBackLeft <= 0)
		{
			return true;
		}
		--ComeBackLeft;
		if (!PickSpot(Camera, Looking, false, AppearBeyond, Walker.Line, Walker.Along, Walker.GroundZ))
		{
			return true;
		}
		Walker.bIndoors = false;
		Walker.bPlaced = false;
		Walker.bWaiting = false;
		Walker.bRushing = false;
		Person->SetInWorld(true);
		return false;
	}
	const double FromCamera = FVector2D::Distance(Walker.Shown, FVector2D(Camera));
	if (bWantsIn && Walker.bPlaced && Walker.React == EReact::None && FromCamera > Pedestrians::IndoorsBeyond
		&& !Person->WasRecentlyRendered(Pedestrians::SeenWithin))
	{
		LeaveShelter(Walker);
		Walker.bIndoors = true;
		Walker.bPlaced = false;
		Person->SetInWorld(false);
		return true;
	}

	// Waiting in a doorway: walks over to it, then stands with his arms
	// folded until the rain stops (or something makes him move).
	if (Walker.Shelter != INDEX_NONE)
	{
		if (RainNow < Pedestrians::ShelterUntil || Walker.React != EReact::None || !Walker.bPlaced || FromCamera > Range)
		{
			LeaveShelter(Walker);
			return false;
		}
		const double Raise = Pedestrians::Middle * Person->GetActorScale3D().Z;
		const FVector2D Gap = Walker.ShelterAt - Walker.Shown;
		const double Left = Gap.Size();
		if (Left > Pedestrians::ShelterNear)
		{
			const float Pace = Walker.Speed * Pedestrians::ShelterHurry;
			const double Step = Pace * DeltaSeconds;
			Walker.Shown += Left > Step ? Gap / Left * Step : Gap;
			const float Toward = static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(Gap.Y, Gap.X)));
			Walker.ShownYaw = FMath::FixedTurn(Walker.ShownYaw, Toward, Pedestrians::TurnRate * DeltaSeconds);
			Person->WalkTo(FVector(Walker.Shown, Walker.GroundZ + Raise), Walker.ShownYaw, Pace, DeltaSeconds);
			return true;
		}
		Walker.Shown = Walker.ShelterAt;
		Walker.ShownYaw = FMath::FixedTurn(Walker.ShownYaw, Walker.ShelterYaw, Pedestrians::StanderTurnRate * DeltaSeconds);
		Person->Huddle(FVector(Walker.Shown, Walker.GroundZ + Raise), Walker.ShownYaw);
		return true;
	}

	// Caught in the rain without an umbrella: now and then he looks for a doorway near by.
	Walker.ShelterClock -= DeltaSeconds;
	if (RainNow > Pedestrians::ShelterFrom && !Walker.bHasUmbrella && Walker.React == EReact::None && Walker.bPlaced
		&& !Walker.bWaiting && !Lines[Walker.Line].bCrossing && Walker.ShelterClock <= 0.f)
	{
		Walker.ShelterClock = Pedestrians::ShelterLook;
		if (AStreetFurniture* Street = AStreetFurniture::Get(GetWorld()))
		{
			FVector Spot;
			float Facing = 0.f;
			int32 Taken = INDEX_NONE;
			if (Street->TakeShelter(Walker.Shown, Pedestrians::ShelterReach, Pedestrians::ShelterMaxOut, Spot, Facing, Taken))
			{
				Walker.Shelter = Taken;
				Walker.ShelterAt = FVector2D(Spot);
				Walker.ShelterYaw = Facing;
			}
		}
	}
	return false;
}

void APedestrians::LeaveShelter(FWalker& Walker)
{
	if (Walker.Shelter == INDEX_NONE)
	{
		return;
	}
	if (AStreetFurniture* Street = AStreetFurniture::Get(GetWorld()))
	{
		Street->LeaveShelter(Walker.Shelter);
	}
	Walker.Shelter = INDEX_NONE;
}

void APedestrians::Gather(const UObject* WorldContextObject, const FVector& At, float Seconds, bool bFight)
{
	const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	if (!World)
	{
		return;
	}
	TActorIterator<APedestrians> It(World);
	if (It)
	{
		It->AddGathering(At, Seconds, bFight);
	}
}

void APedestrians::AddGathering(const FVector& At, float Seconds, bool bFight)
{
	const double Now = GetWorld()->GetTimeSeconds();
	for (FGathering& Ring : Gatherings)
	{
		if (FVector2D::Distance(Ring.At, FVector2D(At)) < Pedestrians::GatherMerge)
		{
			// The same trouble: it lasts longer, and a fight's crowd follows it as it moves.
			Ring.Until = FMath::Max(Ring.Until, Now + Seconds);
			Ring.bFight = Ring.bFight || bFight;
			if (bFight)
			{
				Ring.At = FVector2D(At);
			}
			return;
		}
	}
	FGathering& Ring = Gatherings.AddDefaulted_GetRef();
	Ring.Id = NextGatheringId++;
	Ring.At = FVector2D(At);
	Ring.Until = Now + Seconds;
	Ring.bFight = bFight;
	Ring.NextShout = Now + Random.FRandRange(1.f, Pedestrians::CrowdShoutMin);
	// The nearest constables come to see what is going on.
	APolicePatrols::Investigate(this, At);
}

APedestrians::FGathering* APedestrians::FindGathering(int32 Id)
{
	for (FGathering& Ring : Gatherings)
	{
		if (Ring.Id == Id)
		{
			return &Ring;
		}
	}
	return nullptr;
}

void APedestrians::TickGatherings(double Now, bool bPlayerAbout)
{
	for (int32 i = Gatherings.Num() - 1; i >= 0; --i)
	{
		FGathering& Ring = Gatherings[i];
		if (Now > Ring.Until)
		{
			// Over: the watchers walk on (TickGawk finds their crowd gone).
			Gatherings.RemoveAt(i);
			continue;
		}
		const double FromPlayer = FVector2D::Distance(Ring.At, FVector2D(PlayerAt));

		// Someone calls out now and then.
		if (Ring.Watchers > 0 && Now >= Ring.NextShout)
		{
			Ring.NextShout = Now + Random.FRandRange(Pedestrians::CrowdShoutMin, Pedestrians::CrowdShoutMax);
			if (bPlayerAbout && FromPlayer < Pedestrians::CrowdHeard)
			{
				const Pedestrians::FLine2& Line = Ring.bFight
					? Pedestrians::FightCrowdLines[Random.RandRange(0, static_cast<int32>(UE_ARRAY_COUNT(Pedestrians::FightCrowdLines)) - 1)]
					: Pedestrians::CrashCrowdLines[Random.RandRange(0, static_cast<int32>(UE_ARRAY_COUNT(Pedestrians::CrashCrowdLines)) - 1)];
				Shout(Line.Hu, Line.En);
			}
		}

		// The player standing about in the crowd on foot: its pickpocket may try his luck, once.
		if (Ring.bPicked)
		{
			continue;
		}
		if (bPlayerAbout && !bPlayerDriving && Ring.Watchers >= Pedestrians::PickWatchers && FromPlayer < Pedestrians::PickNear)
		{
			if (Ring.PlayerSince < 0.0)
			{
				Ring.PlayerSince = Now;
			}
			else if (Now - Ring.PlayerSince > Pedestrians::PickAfter)
			{
				Ring.bPicked = true;
				if (Random.FRand() < Pedestrians::PickChance)
				{
					PickPocket(Ring);
				}
			}
		}
		else
		{
			Ring.PlayerSince = -1.0;
		}
	}
}

void APedestrians::PickPocket(FGathering& Ring)
{
	APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this);
	if (!Game || Game->GetMoney() <= 0)
	{
		return;
	}
	FWalker* Thief = nullptr;
	double Nearest = 1e12;
	for (FWalker& Walker : Walkers)
	{
		const double Gap = FVector2D::DistSquared(Walker.Shown, FVector2D(PlayerAt));
		if (Walker.Gawk == Ring.Id && IsValid(Walker.Person) && Walker.React == EReact::None && Gap < Nearest)
		{
			Nearest = Gap;
			Thief = &Walker;
		}
	}
	if (!Thief)
	{
		return;
	}
	const int32 Taken = FMath::Min(Game->GetMoney(), Random.RandRange(Pedestrians::PickLeast, Pedestrians::PickMost));
	Game->AddMoney(-Taken);
	Thief->Stolen = Taken;
	LeaveGawk(*Thief);
	// Off he runs, away from the player.
	const FVector2D Away = (Thief->Shown - FVector2D(PlayerAt)).GetSafeNormal();
	StartReact(*Thief, EReact::Flee, Pedestrians::PickRun, Away);
	if (APestiHUD* Hud = APestiHUD::Get(this))
	{
		Hud->Say(TEXT("Zsebtolvaj"), Pedestrians::PickLine.Hu, Pedestrians::PickLine.En, 3.f);
	}
}

bool APedestrians::TickGawk(FWalker& Walker, double Now, float DeltaSeconds)
{
	APedestrian* Person = Walker.Person;
	FGathering* Ring = Walker.Gawk != INDEX_NONE ? FindGathering(Walker.Gawk) : nullptr;

	// Watching: over to his place in the ring, then turned to it, cheering, pointing or just looking.
	if (Walker.Gawk != INDEX_NONE)
	{
		FVector2D Tangent;
		const FVector2D OwnWay = PointOn(Lines[Walker.Line], Walker.Along, Tangent);
		const FVector2D Spot = Ring ? Ring->At + Walker.GawkOffset : FVector2D::ZeroVector;
		if (!Ring || Walker.React != EReact::None || !Walker.bPlaced || Walker.Shelter != INDEX_NONE
			|| FVector2D::Distance(Spot, OwnWay) > Pedestrians::GawkStray)
		{
			LeaveGawk(Walker);
			return false;
		}
		const double Raise = Pedestrians::Middle * Person->GetActorScale3D().Z;
		const FVector2D Gap = Spot - Walker.Shown;
		const double Left = Gap.Size();
		if (Left > Pedestrians::GawkNear)
		{
			const float Pace = Walker.Speed * Pedestrians::GawkHurry;
			const double Step = Pace * DeltaSeconds;
			Walker.Shown += Left > Step ? Gap / Left * Step : Gap;
			const float Toward = static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(Gap.Y, Gap.X)));
			Walker.ShownYaw = FMath::FixedTurn(Walker.ShownYaw, Toward, Pedestrians::TurnRate * DeltaSeconds);
			Person->WalkTo(FVector(Walker.Shown, Walker.GroundZ + Raise), Walker.ShownYaw, Pace, DeltaSeconds);
			return true;
		}
		Walker.Shown = Spot;
		const FVector2D ToMiddle = Ring->At - Walker.Shown;
		const float Facing = static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(ToMiddle.Y, ToMiddle.X)));
		Walker.ShownYaw = FMath::FixedTurn(Walker.ShownYaw, Facing, Pedestrians::StanderTurnRate * DeltaSeconds);
		const FVector Location(Walker.Shown, Walker.GroundZ + Raise);
		switch (Walker.GawkPose)
		{
		case 0:
			if (Ring->bFight)
			{
				// Arms up, egging them on.
				const float Wave = FMath::Sin(static_cast<float>(Now) * Pedestrians::CheerSpeed + Walker.Nosy * 10.f);
				Person->Hold(Location, FRotator(0.f, Walker.ShownYaw, 0.f), 0.f, 0.f, Pedestrians::CheerArms + Pedestrians::CheerWave * Wave);
				break;
			}
			Person->PointTo(Location, Walker.ShownYaw, Facing, true);
			break;
		case 1:
			Person->PointTo(Location, Walker.ShownYaw, Facing, true);
			break;
		case 2:
			Person->Huddle(Location, Walker.ShownYaw);
			break;
		default:
			Person->PointTo(Location, Walker.ShownYaw, 0.0, false);
			break;
		}
		return true;
	}

	// Walking by: now and then he looks whether there is anything to see near him.
	Walker.GawkClock -= DeltaSeconds;
	if (Gatherings.Num() == 0 || Walker.GawkClock > 0.f)
	{
		return false;
	}
	Walker.GawkClock = Pedestrians::GawkLook;
	if (Walker.Nosy >= Pedestrians::GawkShare || Walker.React != EReact::None || !Walker.bPlaced || Walker.bWaiting
		|| Walker.Shelter != INDEX_NONE || Walker.Stolen > 0 || Lines[Walker.Line].bCrossing)
	{
		return false;
	}
	for (FGathering& Near : Gatherings)
	{
		if (Near.Watchers >= Pedestrians::MostWatchers || FVector2D::Distance(Near.At, Walker.Shown) > Pedestrians::GawkReach)
		{
			continue;
		}
		// A free place in the ring, not through a wall.
		FCollisionQueryParams Params(SCENE_QUERY_STAT(PedestrianGawk), false, this);
		const double EyeZ = Walker.GroundZ + Pedestrians::Middle;
		for (int32 Try = 0; Try < 6; ++Try)
		{
			const int32 Place = Near.NextSpot++;
			const double Angle = FMath::DegreesToRadians(Place * 137.5 + Random.FRandRange(-12.f, 12.f));
			const double Radius = Pedestrians::RingNear + (Place % 3) * Pedestrians::RingStep;
			const FVector2D Offset(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius);
			if (GetWorld()->LineTraceTestByObjectType(FVector(Near.At, EyeZ), FVector(Near.At + Offset, EyeZ),
				FCollisionObjectQueryParams(ECC_WorldStatic), Params))
			{
				continue;
			}
			Walker.Gawk = Near.Id;
			Walker.GawkOffset = Offset;
			Walker.GawkPose = Random.RandRange(0, 3);
			++Near.Watchers;
			return false;
		}
	}
	return false;
}

void APedestrians::LeaveGawk(FWalker& Walker)
{
	if (Walker.Gawk == INDEX_NONE)
	{
		return;
	}
	if (FGathering* Ring = FindGathering(Walker.Gawk))
	{
		Ring->Watchers = FMath::Max(0, Ring->Watchers - 1);
	}
	Walker.Gawk = INDEX_NONE;
}

double APedestrians::MiddleUp(const FWalker& Walker) const
{
	return Walker.bSeated ? Walker.SeatedUp : Pedestrians::Middle * Walker.Person->GetActorScale3D().Z;
}

void APedestrians::AddStander(const FVector2D& At, double Ground, double FaceYaw, const PersonKit::FOutfit& Outfit, double SeatedUp,
	const FString& Who, const FString& Hu, const FString& En)
{
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	APedestrian* Person = GetWorld()->SpawnActor<APedestrian>(APedestrian::StaticClass(), FVector(At, Ground + Pedestrians::Middle),
		FRotator(0.0, FaceYaw, 0.0), Params);
	if (!Person)
	{
		return;
	}
	Person->SetOutfit(Outfit);
	TInlineComponentArray<UPrimitiveComponent*> Shapes(Person);
	for (UPrimitiveComponent* Shape : Shapes)
	{
		Shape->SetCachedMaxDrawDistance(Range);
	}

	FWalker Walker;
	Walker.Person = Person;
	Walker.bStander = true;
	Walker.bSeated = Outfit.bSeated && SeatedUp > 0.0;
	Walker.SeatedUp = SeatedUp;
	Walker.Home = At;
	Walker.HomeYaw = static_cast<float>(FaceYaw);
	Walker.Shown = At;
	Walker.ShownYaw = static_cast<float>(FaceYaw);
	Walker.GroundZ = Ground;
	Walker.bGroundKnown = true;
	Walker.bPlaced = true;
	Walker.CallWho = Who;
	Walker.CallHu = Hu;
	Walker.CallEn = En;
	Person->PointTo(FVector(At, Ground + MiddleUp(Walker)), FaceYaw, 0.0, false);
	Walkers.Add(Walker);
	++NumStanders;
}

void APedestrians::TickStander(FWalker& Walker, const FVector& Camera, bool bPlayerAbout, double Now, float DeltaSeconds)
{
	APedestrian* Person = Walker.Person;
	if (!Walker.bPlaced)
	{
		// Handed over to a street fight: back on his spot, as he was, once the player is far off.
		if (FVector2D::Distance(Walker.Home, FVector2D(Camera)) > Range)
		{
			Walker.Shown = Walker.Home;
			Walker.ShownYaw = Walker.HomeYaw;
			Walker.bPlaced = true;
			Person->SetActorHiddenInGame(false);
			Person->PointTo(FVector(Walker.Home, Walker.GroundZ + MiddleUp(Walker)), Walker.HomeYaw, 0.0, false);
		}
		return;
	}
	if (FVector2D::Distance(Walker.Shown, FVector2D(Camera)) > Range)
	{
		// Far off: back at his spot as he was, once nobody sees him.
		if ((Walker.React != EReact::None || !Walker.Shown.Equals(Walker.Home, 1.0))
			&& !Person->WasRecentlyRendered(Pedestrians::SeenWithin))
		{
			ClearReact(Walker);
			Walker.Shown = Walker.Home;
			Walker.ShownYaw = Walker.HomeYaw;
			Person->PointTo(FVector(Walker.Home, Walker.GroundZ + MiddleUp(Walker)), Walker.HomeYaw, 0.0, false);
		}
		return;
	}

	if (bPlayerAbout)
	{
		ReactToPlayer(Walker, Now);
	}
	if (PoseReacting(Walker, DeltaSeconds))
	{
		return;
	}
	if (Walker.React == EReact::Flee)
	{
		StartReact(Walker, EReact::None, 0.f, FVector2D::ZeroVector);
	}

	// Pushed or scared off his spot: he walks back to it.
	const FVector2D Gap = Walker.Home - Walker.Shown;
	const double Left = Gap.Size();
	if (Left > Pedestrians::HomeNear)
	{
		const double Step = Pedestrians::StanderPace * DeltaSeconds;
		Walker.Shown += Left > Step ? Gap / Left * Step : Gap;
		const float Toward = static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(Gap.Y, Gap.X)));
		Walker.ShownYaw = FMath::FixedTurn(Walker.ShownYaw, Toward, Pedestrians::TurnRate * DeltaSeconds);
		Person->WalkTo(FVector(Walker.Shown, Walker.GroundZ + MiddleUp(Walker)), Walker.ShownYaw, Pedestrians::StanderPace, DeltaSeconds);
		return;
	}
	Walker.Shown = Walker.Home;

	// The player close by on foot: he turns to him (not all the way round)
	// and now and then calls out to him.
	float Facing = Walker.HomeYaw;
	const FVector2D ToPlayer = FVector2D(PlayerAt) - Walker.Home;
	if (bPlayerAbout && !bPlayerDriving && ToPlayer.SizeSquared() < FMath::Square(Pedestrians::CallReach)
		&& FMath::Abs(Walker.GroundZ - PlayerAt.Z) < Pedestrians::CallHeight)
	{
		const float Toward = static_cast<float>(FMath::RadiansToDegrees(FMath::Atan2(ToPlayer.Y, ToPlayer.X)));
		const float Most = Walker.bSeated ? Pedestrians::SeatedLookAside : Pedestrians::StanderLookAside;
		Facing = Walker.HomeYaw + FMath::Clamp(static_cast<float>(FRotator::NormalizeAxis(Toward - Walker.HomeYaw)), -Most, Most);
		APestiHUD* Hud = APestiHUD::Get(this);
		if (!Walker.CallHu.IsEmpty() && Now >= Walker.NextCall && Hud && !Hud->IsSaying())
		{
			Hud->Say(Walker.CallWho, Walker.CallHu, Walker.CallEn, 3.f);
			Walker.NextCall = Now + Random.FRandRange(Pedestrians::MinCallEvery, Pedestrians::MaxCallEvery);
			LastShout = Now;
		}
	}
	Walker.ShownYaw = FMath::FixedTurn(Walker.ShownYaw, Facing, Pedestrians::StanderTurnRate * DeltaSeconds);
	Person->PointTo(FVector(Walker.Home, Walker.GroundZ + MiddleUp(Walker)), Walker.ShownYaw, 0.0, false);
}
