// Fill out your copyright notice in the Description page of Project Settings.

#include "CarVisuals.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/SceneComponent.h"
#include "Components/SpotLightComponent.h"
#include "DayNight.h"
#include "Engine/Texture.h"
#include "Engine/World.h"
#include "EngineSound.h"
#include "ExhaustSmoke.h"
#include "PersonModel.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

namespace CarVisuals
{
	// How far the front wheels turn at full lock (degrees).
	constexpr float MaxSteer = 32.f;
	// Wheels further from the camera than this aren't worth turning (cm)...
	constexpr double WheelRange = 6000.0;
	// ...nor smoke worth making.
	constexpr double SmokeRange = 3500.0;
	// Moving further than this in a frame is a jump (spawned or moved), not driving (cm).
	constexpr float MaxTravel = 600.f;
	// Pulling away this hard (cm/s each second) counts as full throttle.
	constexpr float FullEffortAcceleration = 400.f;
	// Smoke only comes when the car pulls away hard: the pedal down this far
	// (or picking up speed this fast), below this speed (cm/s).
	constexpr float PuffThrottle = 0.6f;
	constexpr float PuffAcceleration = 300.f;
	constexpr float PuffTopSpeed = 1000.f;
	// Seconds between puffs while it does.
	constexpr float PuffInterval = 0.12f;
	// Smoke leaves the pipe backwards at this speed (cm/s).
	constexpr float PuffSpeed = 90.f;
	// Traffic engines are heard within this distance of the camera (cm),
	// fall silent again beyond the second, and only this many at once.
	constexpr double HearRange = 2500.0;
	constexpr double ForgetRange = 3000.0;
	constexpr int32 MaxHeardEngines = 6;
	int32 HeardEngines = 0;
	// Their sound fades out to nothing over this distance, and is quieter than the player's car.
	constexpr float EngineFalloff = 2200.f;
	constexpr float EngineVolume = 0.6f;
	// Traffic at this speed is flat out (cm/s).
	constexpr float StreetTopSpeed = 1400.f;

	// The body sits on springs. It leans out of a turn (at most MaxRoll
	// degrees), squats pulling away and dips its nose braking (MaxPitch),
	// half way there at this much push (cm/s each second)...
	constexpr float MaxRoll = 6.f;
	constexpr float RollPush = 1500.f;
	constexpr float MaxPitch = 4.f;
	constexpr float PitchPush = 1500.f;
	// ...and swings there and back a little before it settles (spring
	// stiffness and damping: about 1.5 swings a second, a small overshoot).
	constexpr float SwayStiffness = 90.f;
	constexpr float SwayDamping = 7.f;
	// The car's push is jumpy frame to frame: smoothed this fast.
	constexpr float PushSmoothing = 12.f;
	// On the setts the body bobs (cm) and rocks (degrees), and each wheel
	// hops on its own (cm), fully at this speed and above (cm/s).
	constexpr float RoughSpeed = 1500.f;
	constexpr float BodyBob = 1.4f;
	constexpr float BodyRock = 0.5f;
	constexpr float WheelHop = 0.7f;
	// An idling engine shakes the body a little (cm, degrees, shakes a second),
	// below this speed (cm/s).
	constexpr float IdleBob = 0.18f;
	constexpr float IdleRock = 0.12f;
	constexpr float IdleShakes = 9.f;
	constexpr float IdleSpeed = 60.f;
	constexpr float TwoPi = 6.2831853f;

	// Knocks dent the body between these heights (cm, car frame), wherever the box was touched.
	constexpr double KnockLow = -45.0;
	constexpr double KnockHigh = 15.0;
	// A worn car's radiator steams above this wear and smokes black above the second,
	// a puff every this many seconds, rising this fast (cm/s).
	constexpr double SteamWear = 0.45;
	constexpr double SmokeWear = 0.8;
	constexpr float SteamInterval = 0.16f;
	constexpr float SteamRise = 70.f;
	// Knocked-off parts weigh this much (kg: a lamp, a bumper), fly off at
	// this share of the car's speed plus this much up and out (cm/s), and
	// come to rest for good after this long (s).
	constexpr float LampKg = 4.f;
	constexpr float BumperKg = 14.f;
	constexpr float LooseCarry = 0.6f;
	constexpr float LooseUp = 250.f;
	constexpr float LooseOut = 150.f;
	constexpr double LooseRestTime = 8.0;

	/** Grows like X when small, but never past 1 (or -1). */
	float Soft(float X)
	{
		return X / (1.f + FMath::Abs(X));
	}

	// Lamps glow this many times as bright as a lit grey wall, by day and by night.
	const FLinearColor HeadlampLight(1.f, 0.88f, 0.62f);
	constexpr float HeadlampDay = 6.f;
	constexpr float HeadlampNight = 40.f;
	const FLinearColor TailLight(1.f, 0.03f, 0.02f);
	constexpr float TailDay = 2.f;
	constexpr float TailNight = 4.f;
	constexpr float BrakeDay = 8.f;
	constexpr float BrakeNight = 14.f;
	// Slowing down this hard (cm/s each second) shows on the brake lights,
	// which stay on this long after (s); a car that braked to a stop keeps
	// its foot on the brake until it pulls away.
	constexpr float BrakeDeceleration = 150.f;
	constexpr float BrakeHoldTime = 0.35f;
	constexpr float StandingSpeed = 15.f;
	constexpr float PullAway = 80.f;
	// The headlights light the road for the cars within this distance of the
	// camera (cm), the nearest few only: ten metres ahead the road is lit
	// four times as bright as a lit grey wall.
	constexpr double BeamRange = 4000.0;
	constexpr double BeamForget = 4500.0;
	constexpr int32 MaxBeamCars = 8;
	int32 BeamCars = 0;
	constexpr float BeamStrength = 400.f;
	constexpr float BeamReach = 3500.f;
	constexpr float BeamDip = 6.f;

	// A driver further than this isn't drawn (cm).
	constexpr float DriverRange = 6000.f;

	// Taxi drivers in black jackets and caps, policemen in navy.
	const FLinearColor TaxiDriverCloth(0.03f, 0.03f, 0.035f);
	const FLinearColor PoliceCloth(0.02f, 0.025f, 0.06f);

	/** One of the town's drivers. */
	PersonKit::FOutfit StreetDriver(const CarModel::FStyle& Style, int32 Look)
	{
		FRandomStream Random(Look * 7919 + 13);
		PersonKit::FOutfit Outfit = PersonModel::RandomOutfit(Random);
		if (Style.bTaxi || Style.bPolice)
		{
			// The company and the force hire men.
			for (int32 Try = 0; Try < 8 && Outfit.bWoman; ++Try)
			{
				Outfit = PersonModel::RandomOutfit(Random);
			}
			Outfit.bWoman = false;
			Outfit.Hat = PersonKit::EHat::FlatCap;
			Outfit.Top = PersonKit::ETop::Jacket;
			Outfit.bScarf = false;
			Outfit.bTie = true;
			const FLinearColor& Cloth = Style.bPolice ? PoliceCloth : TaxiDriverCloth;
			Outfit.HatColor = Cloth;
			Outfit.HatBand = Style.bPolice ? FLinearColor(0.55f, 0.45f, 0.15f) : Cloth;
			Outfit.Coat = Cloth;
			Outfit.Trousers = Cloth;
		}
		return Outfit;
	}

	/** The drivers are the same for every car of a kind with the same look: make each once. */
	const TArray<PersonKit::FSection>& StreetDriverShape(const CarModel::FStyle& Style, int32 Look)
	{
		static TMap<int32, TArray<PersonKit::FSection>> Shapes;
		const int32 Kind = Style.bPolice ? 2 : Style.bTaxi ? 1 : 0;
		const int32 Key = (Look * 3 + Kind) * static_cast<int32>(CarModel::EBody::Count) + static_cast<int32>(Style.Body);
		if (const TArray<PersonKit::FSection>* Found = Shapes.Find(Key))
		{
			return *Found;
		}
		TArray<PersonKit::FSection>& Built = Shapes.Add(Key);
		CarModel::BuildSeatedDriver(StreetDriver(Style, Look), CarModel::Layout(Style.Body), Built);
		return Built;
	}

	/** The bodies and wheels are the same for every car of a kind: make each once. */
	const CarModel::FBuilder& BodyShape(const CarModel::FStyle& Style)
	{
		static TMap<int32, CarModel::FBuilder> Shapes;
		const int32 Key = CarModel::ShapeKey(Style);
		if (const CarModel::FBuilder* Found = Shapes.Find(Key))
		{
			return *Found;
		}
		CarModel::FBuilder& Built = Shapes.Add(Key);
		CarModel::Build(Built, Style);
		return Built;
	}

	const CarModel::FBuilder& WheelShape(CarModel::EWheel Wheel)
	{
		static TMap<int32, CarModel::FBuilder> Shapes;
		const int32 Key = static_cast<int32>(Wheel);
		if (const CarModel::FBuilder* Found = Shapes.Find(Key))
		{
			return *Found;
		}
		CarModel::FBuilder& Built = Shapes.Add(Key);
		CarModel::BuildWheel(Built, Wheel);
		return Built;
	}
}

UCarVisuals::UCarVisuals()
{
	PrimaryComponentTick.bCanEverTick = true;

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicShapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	BaseMaterial = BasicShapeMaterial.Object;
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Translucent(TEXT("/Engine/EngineMaterials/Widget3DPassThrough_Translucent.Widget3DPassThrough_Translucent"));
	SeeThroughMaterial = Translucent.Object;
	static ConstructorHelpers::FObjectFinder<UTexture> White(TEXT("/Engine/EngineResources/WhiteSquareTexture.WhiteSquareTexture"));
	WhiteTexture = White.Object;
}

void UCarVisuals::Show(USceneComponent* Parent, const CarModel::FStyle& InStyle)
{
	AActor* Owner = GetOwner();
	if (!Owner || !Parent)
	{
		return;
	}
	Style = InStyle;
	Damage = CarDamage::FState();
	bLampOut[0] = bLampOut[1] = false;
	SwayRoll = SwayRollRate = SwayPitch = SwayPitchRate = 0.f;
	PushSide = PushAhead = 0.f;
	bVelocityKnown = false;

	auto MakeMesh = [Owner, Parent]()
	{
		UProceduralMeshComponent* Mesh = NewObject<UProceduralMeshComponent>(Owner);
		Mesh->SetupAttachment(Parent);
		Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Mesh->SetCanEverAffectNavigation(false);
		Mesh->RegisterComponent();
		Owner->AddInstanceComponent(Mesh);
		return Mesh;
	};
	if (!Body)
	{
		Body = MakeMesh();
		for (int32 i = 0; i < 4; ++i)
		{
			Wheels.Add(MakeMesh());
		}
	}

	const CarModel::FBuilder& Shape = CarVisuals::BodyShape(Style);
	Fill(Body, Shape);
	Body->SetRelativeLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);
	// The headlamp lenses: the middle of the lamp glass on each side of the front.
	const CarModel::FLayout Layout = CarModel::Layout(Style.Body);
	FVector Sum[2] = { FVector::ZeroVector, FVector::ZeroVector };
	int32 Count[2] = { 0, 0 };
	for (const FVector& V : Shape.Parts[CarModel::Lamp].Vertices)
	{
		if (V.X > Layout.FrontAxle)
		{
			const int32 Side = V.Y < 0.0 ? 0 : 1;
			Sum[Side] += V;
			++Count[Side];
		}
	}
	for (int32 Side = 0; Side < 2; ++Side)
	{
		LampAt[Side] = Count[Side] > 0 ? Sum[Side] / Count[Side] : FVector(Layout.Nose, Side ? 50.0 : -50.0, 10.0);
	}
	LampState = -1;
	TailState = -1;
	for (int32 i = 0; i < Wheels.Num(); ++i)
	{
		Wheels[i]->SetRelativeLocationAndRotation(CarModel::WheelHub(Style.Body, i), FRotator::ZeroRotator);
		Fill(Wheels[i], CarVisuals::WheelShape(Style.Wheel));
	}
	bShown = true;
	bMoved = false;
}

void UCarVisuals::ShowDriver(const PersonKit::FOutfit& Outfit)
{
	TArray<PersonKit::FSection> Sections;
	CarModel::BuildSeatedDriver(Outfit, CarModel::Layout(Style.Body), Sections);
	FillPerson(Driver, Sections);
}

void UCarVisuals::ShowStreetDriver(int32 Look)
{
	FillPerson(Driver, CarVisuals::StreetDriverShape(Style, FMath::Clamp(Look, 0, NumDriverLooks - 1)));
}

void UCarVisuals::HideDriver()
{
	HidePerson(Driver);
}

void UCarVisuals::ShowPassenger(const PersonKit::FOutfit& Outfit)
{
	TArray<PersonKit::FSection> Sections;
	CarModel::BuildBackSeatPassenger(Outfit, CarModel::Layout(Style.Body), Sections);
	FillPerson(Passenger, Sections);
}

void UCarVisuals::HidePassenger()
{
	HidePerson(Passenger);
}

void UCarVisuals::HidePerson(UProceduralMeshComponent* Person)
{
	if (Person)
	{
		Person->ClearAllMeshSections();
		Person->SetVisibility(false);
	}
}

void UCarVisuals::FillPerson(TObjectPtr<UProceduralMeshComponent>& Person, const TArray<PersonKit::FSection>& Sections)
{
	AActor* Owner = GetOwner();
	if (!Owner || !Body)
	{
		return;
	}
	if (!Person)
	{
		Person = NewObject<UProceduralMeshComponent>(Owner);
		Person->SetupAttachment(Body);
		Person->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Person->SetCanEverAffectNavigation(false);
		Person->SetCastShadow(false);
		Person->SetCachedMaxDrawDistance(CarVisuals::DriverRange);
		Person->RegisterComponent();
		Owner->AddInstanceComponent(Person);
	}
	Person->ClearAllMeshSections();
	for (int32 i = 0; i < Sections.Num(); ++i)
	{
		const PersonKit::FSection& Section = Sections[i];
		Person->CreateMeshSection_LinearColor(i, Section.Vertices, Section.Triangles, Section.Normals,
			TArray<FVector2D>(), TArray<FLinearColor>(), TArray<FProcMeshTangent>(), false);
		if (BaseMaterial)
		{
			UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(BaseMaterial, this);
			Material->SetVectorParameterValue(TEXT("Color"), Section.Color);
			Person->SetMaterial(i, Material);
		}
	}
	Person->SetVisibility(true);
}

void UCarVisuals::Fill(UProceduralMeshComponent* Mesh, const CarModel::FBuilder& Shape)
{
	Mesh->ClearAllMeshSections();
	for (int32 i = 0; i < CarModel::PartCount; ++i)
	{
		const CarModel::FPart& Part = Shape.Parts[i];
		if (Part.Triangles.Num() == 0)
		{
			continue;
		}
		const FLinearColor Color = CarModel::PartColor(static_cast<CarModel::EPart>(i), Style);
		if (i == CarModel::Glass)
		{
			if (!SeeThroughMaterial)
			{
				// No see-through material: leave the panes out, as open windows.
				continue;
			}
			Mesh->CreateMeshSection_LinearColor(i, Part.Vertices, Part.Triangles, Part.Normals,
				TArray<FVector2D>(), TArray<FLinearColor>(), TArray<FProcMeshTangent>(), false);
			UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(SeeThroughMaterial, this);
			if (WhiteTexture)
			{
				Material->SetTextureParameterValue(TEXT("SlateUI"), WhiteTexture);
			}
			Material->SetScalarParameterValue(TEXT("OpacityFromTexture"), 0.f);
			Material->SetVectorParameterValue(TEXT("TintColorAndOpacity"), Color);
			// Unlit, so it is darkened with the night instead of shining.
			if (ADayNight* Clock = ADayNight::Get(GetWorld()))
			{
				Clock->AddSeeThrough(Material, Color);
			}
			Mesh->SetMaterial(i, Material);
			continue;
		}
		Mesh->CreateMeshSection_LinearColor(i, Part.Vertices, Part.Triangles, Part.Normals,
			TArray<FVector2D>(), TArray<FLinearColor>(), TArray<FProcMeshTangent>(), false);
		if (BaseMaterial)
		{
			UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(BaseMaterial, this);
			Material->SetVectorParameterValue(TEXT("Color"), Color);
			Mesh->SetMaterial(i, Material);
			if (Mesh == Body && i == CarModel::Lamp)
			{
				LampPaint = Material;
			}
			else if (Mesh == Body && i == CarModel::TailLamp)
			{
				TailPaint = Material;
			}
		}
	}
}

void UCarVisuals::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	AActor* Owner = GetOwner();
	if (!bShown || !Owner || DeltaTime <= 0.f)
	{
		return;
	}

	// How far the car went forward and turned since last frame.
	const FVector Location = Owner->GetActorLocation();
	const float Yaw = static_cast<float>(Owner->GetActorRotation().Yaw);
	if (!bMoved)
	{
		LastLocation = Location;
		LastYaw = Yaw;
		bMoved = true;
		bVelocityKnown = false;
		return;
	}
	const FVector Step(Location.X - LastLocation.X, Location.Y - LastLocation.Y, 0.0);
	const float Travel = static_cast<float>(FVector::DotProduct(Location - LastLocation, Owner->GetActorForwardVector()));
	const float Turn = FRotator::NormalizeAxis(Yaw - LastYaw);
	LastLocation = Location;
	LastYaw = Yaw;
	if (Step.Size() > CarVisuals::MaxTravel)
	{
		bVelocityKnown = false;
		return;
	}
	const float Speed = Travel / DeltaTime;
	const float Pickup = (Speed - LastSpeed) / DeltaTime;
	LastSpeed = Speed;

	// How the car is being pushed about (cm/s each second, flat): what the body leans against.
	const FVector Going = Step / DeltaTime;
	const FVector Push = bVelocityKnown ? (Going - LastVelocity) / DeltaTime : FVector::ZeroVector;
	LastVelocity = Going;
	bVelocityKnown = true;
	Odometer = FMath::Fmod(Odometer + FMath::Abs(Travel), 1.0e6);

	// The wheels roll as far as the car goes. The front ones turn as far as
	// the turning needs (a car turns Travel / wheelbase * tan(angle)), or as
	// the driver turns the wheel.
	Spin = FMath::Fmod(Spin + FMath::RadiansToDegrees(Travel / static_cast<float>(CarModel::WheelRadius)), 360.f);
	float WantedSteer = SteerAngle;
	if (bDriverSteers)
	{
		WantedSteer = DriverSteer * CarVisuals::MaxSteer;
	}
	else if (FMath::Abs(Travel) > 0.5f)
	{
		const CarModel::FLayout Layout = CarModel::Layout(Style.Body);
		const float Wheelbase = static_cast<float>(Layout.FrontAxle - Layout.RearAxle);
		WantedSteer = FMath::RadiansToDegrees(FMath::Atan(Wheelbase * FMath::DegreesToRadians(Turn) / Travel));
	}
	WantedSteer = FMath::Clamp(WantedSteer, -CarVisuals::MaxSteer, CarVisuals::MaxSteer);
	SteerAngle = FMath::FInterpTo(SteerAngle, WantedSteer, DeltaTime, 10.f);

	const APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0);
	const double Distance = Camera ? FVector::Dist(Camera->GetCameraLocation(), Location) : 0.0;
	UpdateLights(DeltaTime, Speed, Pickup, Camera ? Distance : CarVisuals::BeamForget + 1.0);
	const bool bNear = Distance < CarVisuals::WheelRange && Owner->WasRecentlyRendered(0.2f);
	UpdateSway(DeltaTime, Speed, Push, bNear);

	UpdateWear(DeltaTime, Distance);

	if (bStreetEngine)
	{
		UpdateStreetEngine(Camera ? Distance : CarVisuals::ForgetRange + 1.0, Speed, Pickup);
	}

	// Faint puffs from the tail pipe, only while pulling away hard.
	PuffTimer -= DeltaTime;
	const bool bPullingAway = FMath::Abs(Speed) < CarVisuals::PuffTopSpeed && (DriverThrottle >= 0.f
		? DriverThrottle > CarVisuals::PuffThrottle && Pickup > 0.f
		: Pickup > CarVisuals::PuffAcceleration);
	if (bEngineRunning && bPullingAway && Distance < CarVisuals::SmokeRange && PuffTimer <= 0.f)
	{
		const float Effort = FMath::Clamp(Pickup / CarVisuals::FullEffortAcceleration, 0.f, 1.f);
		PuffTimer = CarVisuals::PuffInterval * FMath::FRandRange(0.7f, 1.3f);
		Puff(FMath::Lerp(6.f, 10.f, Effort), false);
	}
}

void UCarVisuals::UpdateSway(float DeltaTime, float Speed, const FVector& Push, bool bNear)
{
	const AActor* Owner = GetOwner();
	if (!Owner || !Body)
	{
		return;
	}

	// Lean against the push: out of the turn, back pulling away, forward braking.
	const float Side = static_cast<float>(FVector::DotProduct(Push, Owner->GetActorRightVector()));
	const float Ahead = static_cast<float>(FVector::DotProduct(Push, Owner->GetActorForwardVector()));
	PushSide = FMath::FInterpTo(PushSide, Side, DeltaTime, CarVisuals::PushSmoothing);
	PushAhead = FMath::FInterpTo(PushAhead, Ahead, DeltaTime, CarVisuals::PushSmoothing);
	// Roll + puts the right side down: a right turn pushes right, so the left side goes down.
	const float WantedRoll = -CarVisuals::MaxRoll * CarVisuals::Soft(PushSide / CarVisuals::RollPush);
	const float WantedPitch = CarVisuals::MaxPitch * CarVisuals::Soft(PushAhead / CarVisuals::PitchPush);

	// On springs: it swings towards the lean, a little past it, and back.
	const float SpringTime = FMath::Min(DeltaTime, 0.05f);
	SwayRollRate += (CarVisuals::SwayStiffness * (WantedRoll - SwayRoll) - CarVisuals::SwayDamping * SwayRollRate) * SpringTime;
	SwayPitchRate += (CarVisuals::SwayStiffness * (WantedPitch - SwayPitch) - CarVisuals::SwayDamping * SwayPitchRate) * SpringTime;
	SwayRoll = FMath::Clamp(SwayRoll + SwayRollRate * SpringTime, -1.5f * CarVisuals::MaxRoll, 1.5f * CarVisuals::MaxRoll);
	SwayPitch = FMath::Clamp(SwayPitch + SwayPitchRate * SpringTime, -1.5f * CarVisuals::MaxPitch, 1.5f * CarVisuals::MaxPitch);

	if (!bNear)
	{
		return;
	}

	// The setts: the faster, the rougher. The bumps come with the distance
	// driven, so a car standing still stands still.
	const float Rough = FMath::Clamp(FMath::Abs(Speed) / CarVisuals::RoughSpeed, 0.f, 1.f);
	const float Along = static_cast<float>(Odometer);
	float Bob = Rough * CarVisuals::BodyBob * (FMath::PerlinNoise1D(Along / 380.f) + 0.4f * FMath::PerlinNoise1D(Along / 110.f + 17.f));
	float Rock = Rough * CarVisuals::BodyRock * FMath::PerlinNoise1D(Along / 430.f + 53.f);
	float Nod = Rough * CarVisuals::BodyRock * 0.7f * FMath::PerlinNoise1D(Along / 510.f + 91.f);

	// Ticking over at a standstill, the engine shakes it.
	const float Idle = bEngineRunning ? 1.f - FMath::Clamp(FMath::Abs(Speed) / CarVisuals::IdleSpeed, 0.f, 1.f) : 0.f;
	if (Idle > 0.f)
	{
		IdlePhase = FMath::Fmod(IdlePhase + DeltaTime * CarVisuals::IdleShakes * CarVisuals::TwoPi, CarVisuals::TwoPi);
		Bob += Idle * CarVisuals::IdleBob * FMath::Sin(IdlePhase);
		Rock += Idle * CarVisuals::IdleRock * FMath::Sin(IdlePhase * 0.5f + 1.f);
	}

	// Tip the body about the axles' height, so its bottom stays down by the wheels.
	const FQuat Lean = FRotator(SwayPitch + Nod, 0.f, SwayRoll + Rock).Quaternion();
	const FVector Pivot(0.0, 0.0, CarModel::HubZ);
	Body->SetRelativeLocationAndRotation(Pivot - Lean.RotateVector(Pivot) + FVector(0.0, 0.0, Bob), Lean);

	for (int32 i = 0; i < Wheels.Num(); ++i)
	{
		const float Hop = Rough * CarVisuals::WheelHop * FMath::PerlinNoise1D(Along / 90.f + 23.f * i);
		// Rolling forward turns the top of the wheel forward: pitch down.
		Wheels[i]->SetRelativeLocationAndRotation(CarModel::WheelHub(Style.Body, i) + FVector(0.0, 0.0, Hop),
			FRotator(-Spin, i < 2 ? SteerAngle : 0.f, 0.f));
	}
}

void UCarVisuals::UpdateStreetEngine(double Distance, float Speed, float Pickup)
{
	if (bStreetEngineHeard && Distance > CarVisuals::ForgetRange)
	{
		StopStreetEngine();
	}
	else if (!bStreetEngineHeard && Distance < CarVisuals::HearRange && CarVisuals::HeardEngines < CarVisuals::MaxHeardEngines)
	{
		AActor* Owner = GetOwner();
		if (!StreetEngine && Owner && Body)
		{
			StreetEngine = NewObject<UEngineSound>(Owner);
			StreetEngine->SetupAttachment(Body);
			StreetEngine->bAllowSpatialization = true;
			StreetEngine->bOverrideAttenuation = true;
			StreetEngine->AttenuationOverrides.bAttenuate = true;
			StreetEngine->AttenuationOverrides.bSpatialize = true;
			StreetEngine->AttenuationOverrides.AttenuationShapeExtents = FVector(300.0, 0.0, 0.0);
			StreetEngine->AttenuationOverrides.FalloffDistance = CarVisuals::EngineFalloff;
			StreetEngine->RegisterComponent();
			Owner->AddInstanceComponent(StreetEngine);
		}
		if (StreetEngine)
		{
			StreetEngine->SetVolumeMultiplier(CarVisuals::EngineVolume);
			StreetEngine->Start();
			bStreetEngineHeard = true;
			++CarVisuals::HeardEngines;
		}
	}
	if (bStreetEngineHeard && StreetEngine)
	{
		const float Share = FMath::Clamp(FMath::Abs(Speed) / CarVisuals::StreetTopSpeed, 0.f, 1.f);
		const float Load = FMath::Clamp(Pickup / CarVisuals::FullEffortAcceleration, 0.f, 1.f);
		StreetEngine->SetEngine(0.1f + 0.5f * Share + 0.15f * Load, Load);
	}
}

void UCarVisuals::StopStreetEngine()
{
	if (!bStreetEngineHeard)
	{
		return;
	}
	if (StreetEngine)
	{
		StreetEngine->Stop();
	}
	bStreetEngineHeard = false;
	CarVisuals::HeardEngines = FMath::Max(0, CarVisuals::HeardEngines - 1);
}

void UCarVisuals::UpdateLights(float DeltaTime, float Speed, float Pickup, double Distance)
{
	using namespace CarVisuals;
	ADayNight* Clock = ADayNight::Get(GetWorld());
	if (!Clock || !Body)
	{
		return;
	}

	bHeadlights = bHeadlightsByHand ? bHeadlightsWanted : bEngineRunning && Clock->AreLampsLit();

	bool bBraking = false;
	if (DriverBrake >= 0)
	{
		bBraking = DriverBrake > 0 && bEngineRunning;
	}
	else
	{
		if (Pickup < -BrakeDeceleration && Speed > StandingSpeed)
		{
			BrakeHold = BrakeHoldTime;
			bHeldOnBrake = true;
		}
		else if (Pickup > PullAway)
		{
			bHeldOnBrake = false;
		}
		BrakeHold -= DeltaTime;
		bBraking = bEngineRunning && (BrakeHold > 0.f || (bHeldOnBrake && FMath::Abs(Speed) < StandingSpeed));
	}

	const int32 WantedLamp = bHeadlights ? 1 : 0;
	const int32 WantedTail = bBraking ? 2 : bHeadlights ? 1 : 0;
	const float Darkness = Clock->GetDarkness();
	const bool bRetint = FMath::Abs(Darkness - TintDarkness) > 0.02f;
	if (WantedLamp != LampState || (WantedLamp && bRetint))
	{
		if (WantedLamp)
		{
			if (!LampGlow)
			{
				LampGlow = Clock->MakeGlowMaterial(this);
			}
			Clock->SetGlow(LampGlow, HeadlampLight, FMath::Lerp(HeadlampDay, HeadlampNight, Darkness));
		}
		if (WantedLamp != LampState)
		{
			Body->SetMaterial(CarModel::Lamp, WantedLamp && LampGlow ? LampGlow.Get() : LampPaint.Get());
		}
		LampState = WantedLamp;
	}
	if (WantedTail != TailState || (WantedTail && bRetint))
	{
		if (WantedTail)
		{
			if (!TailGlow)
			{
				TailGlow = Clock->MakeGlowMaterial(this);
			}
			const float Strength = WantedTail == 2 ? FMath::Lerp(BrakeDay, BrakeNight, Darkness) : FMath::Lerp(TailDay, TailNight, Darkness);
			Clock->SetGlow(TailGlow, TailLight, Strength);
		}
		if ((WantedTail != 0) != (TailState > 0))
		{
			Body->SetMaterial(CarModel::TailLamp, WantedTail && TailGlow ? TailGlow.Get() : TailPaint.Get());
		}
		TailState = WantedTail;
	}
	if (bRetint)
	{
		TintDarkness = Darkness;
	}

	// The beams on the road, for the nearest cars only, and only in the dark.
	const bool bWantBeams = bHeadlights && Clock->AreLampsLit()
		&& (bBeamsShown ? Distance < BeamForget : Distance < BeamRange && BeamCars < MaxBeamCars);
	if (bWantBeams != bBeamsShown)
	{
		ShowBeams(bWantBeams);
	}
	if (bBeamsShown && bRetint)
	{
		for (USpotLightComponent* Beam : Beams)
		{
			Beam->SetIntensity(Clock->LampCandela(BeamStrength));
		}
	}
}

void UCarVisuals::ShowBeams(bool bShow)
{
	using namespace CarVisuals;
	AActor* Owner = GetOwner();
	// Not asked for when switching off: the clock may be gone as the game ends.
	ADayNight* Clock = bShow ? ADayNight::Get(GetWorld()) : nullptr;
	if (bShow && Beams.Num() == 0 && Owner && Body && Clock)
	{
		for (int32 Side = 0; Side < 2; ++Side)
		{
			USpotLightComponent* Beam = NewObject<USpotLightComponent>(Owner);
			Beam->SetupAttachment(Body);
			Beam->SetRelativeLocationAndRotation(LampAt[Side] + FVector(4.0, 0.0, 0.0), FRotator(-BeamDip, 0.f, 0.f));
			Beam->SetMobility(EComponentMobility::Movable);
			Beam->SetCastShadows(false);
			Beam->SetIntensityUnits(ELightUnits::Candelas);
			Beam->SetAttenuationRadius(BeamReach);
			Beam->SetInnerConeAngle(10.f);
			Beam->SetOuterConeAngle(26.f);
			Beam->SetLightColor(HeadlampLight);
			Beam->RegisterComponent();
			Owner->AddInstanceComponent(Beam);
			Beams.Add(Beam);
		}
	}
	for (int32 Side = 0; Side < Beams.Num(); ++Side)
	{
		USpotLightComponent* Beam = Beams[Side];
		if (bShow && Clock)
		{
			Beam->SetIntensity(Clock->LampCandela(BeamStrength));
		}
		// A broken headlamp shines no more.
		Beam->SetVisibility(bShow && !(Side < 2 && bLampOut[Side]));
	}
	if (bShow != bBeamsShown)
	{
		BeamCars = FMath::Max(0, BeamCars + (bShow ? 1 : -1));
		bBeamsShown = bShow;
	}
}

void UCarVisuals::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ShowBeams(false);
	StopStreetEngine();
	Super::EndPlay(EndPlayReason);
}

void UCarVisuals::Backfire()
{
	Puff(14.f, true);
	Puff(10.f, true);
}

void UCarVisuals::Puff(float Size, bool bDark)
{
	AActor* Owner = GetOwner();
	AExhaustSmoke* Smoke = AExhaustSmoke::Get(GetWorld());
	if (!Owner || !Smoke)
	{
		return;
	}
	const FVector Pipe = Owner->GetActorTransform().TransformPosition(CarModel::Layout(Style.Body).Exhaust);
	const FVector Back = -Owner->GetActorForwardVector() * CarVisuals::PuffSpeed;
	const FVector Wander(FMath::FRandRange(-20.f, 20.f), FMath::FRandRange(-20.f, 20.f), FMath::FRandRange(5.f, 25.f));
	Smoke->Puff(Pipe, Back + Wander, Size, bDark);
}

void UCarVisuals::PuffFrom(const FVector& Local, const FVector& Drift, float Size, bool bDark)
{
	AActor* Owner = GetOwner();
	AExhaustSmoke* Smoke = AExhaustSmoke::Get(GetWorld());
	if (!Owner || !Smoke)
	{
		return;
	}
	const FVector Wander(FMath::FRandRange(-20.f, 20.f), FMath::FRandRange(-20.f, 20.f), FMath::FRandRange(5.f, 25.f));
	Smoke->Puff(Owner->GetActorTransform().TransformPosition(Local), Drift + Wander, Size, bDark);
}

void UCarVisuals::Knock(const FVector& WorldPoint, const FVector& WorldInto, float Speed)
{
	if (!bShown || !Body || !Body->GetAttachParent())
	{
		return;
	}
	// Into the car's own frame (the body's parent: the body itself leans).
	const FTransform Frame = Body->GetAttachParent()->GetComponentTransform();
	FVector At = Frame.InverseTransformPosition(WorldPoint);
	At.Z = FMath::Clamp(At.Z, CarVisuals::KnockLow, CarVisuals::KnockHigh);
	const FVector Into = Frame.InverseTransformVectorNoScale(WorldInto);
	if (!CarDamage::Knock(Damage, At, Into, Speed))
	{
		return;
	}

	const CarModel::FBuilder& Clean = CarVisuals::BodyShape(Style);
	const CarModel::FLayout Layout = CarModel::Layout(Style.Body);
	const int32 Fresh = CarDamage::KnockOff(Clean, Layout, Damage);
	CarModel::FBuilder Dented;
	bool LampBroken[2] = { false, false };
	bool TailBroken[2] = { false, false };
	CarDamage::Apply(Clean, Layout, Damage, Dented, LampBroken, TailBroken);
	Refill(Body, Dented);

	bLampOut[0] = LampBroken[0];
	bLampOut[1] = LampBroken[1];
	if (Beams.Num() > 0)
	{
		ShowBeams(bBeamsShown);
	}
	for (int32 Which = 0; Which < CarDamage::LooseCount; ++Which)
	{
		if ((Fresh & (1 << Which)) != 0)
		{
			DropPart(Which);
		}
	}
}

void UCarVisuals::Refill(UProceduralMeshComponent* Mesh, const CarModel::FBuilder& Shape)
{
	for (int32 i = 0; i < CarModel::PartCount; ++i)
	{
		const CarModel::FPart& Part = Shape.Parts[i];
		if (Part.Triangles.Num() == 0)
		{
			Mesh->ClearMeshSection(i);
			continue;
		}
		if (i == CarModel::Glass && !SeeThroughMaterial)
		{
			continue;
		}
		// The paint stays with the section; a part new to this mesh (a broken lamp's dark glass) gets its own.
		const bool bUnpainted = Mesh->GetMaterial(i) == nullptr;
		Mesh->CreateMeshSection_LinearColor(i, Part.Vertices, Part.Triangles, Part.Normals,
			TArray<FVector2D>(), TArray<FLinearColor>(), TArray<FProcMeshTangent>(), false);
		if (bUnpainted && BaseMaterial && i != CarModel::Glass)
		{
			UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(BaseMaterial, this);
			Material->SetVectorParameterValue(TEXT("Color"), CarModel::PartColor(static_cast<CarModel::EPart>(i), Style));
			Mesh->SetMaterial(i, Material);
		}
	}
}

void UCarVisuals::DropPart(int32 Which)
{
	AActor* Owner = GetOwner();
	USceneComponent* Frame = Body ? Body->GetAttachParent() : nullptr;
	if (!Owner || !Frame)
	{
		return;
	}
	CarModel::FBuilder Shape;
	FVector Middle = FVector::ZeroVector;
	CarDamage::BuildLoose(CarVisuals::BodyShape(Style), CarModel::Layout(Style.Body), Which, Shape, Middle);

	// The part turns about its own middle as it tumbles.
	TArray<FVector> Hull;
	for (int32 i = 0; i < CarModel::PartCount; ++i)
	{
		for (FVector& V : Shape.Parts[i].Vertices)
		{
			V -= Middle;
			Hull.Add(V);
		}
	}
	if (Hull.Num() < 4)
	{
		return;
	}

	UProceduralMeshComponent* Piece = NewObject<UProceduralMeshComponent>(Owner, NAME_None, RF_Transient);
	Piece->SetWorldTransform(FTransform(Frame->GetComponentQuat(), Frame->GetComponentTransform().TransformPosition(Middle)));
	Piece->SetCanEverAffectNavigation(false);
	Piece->bUseComplexAsSimpleCollision = false;
	Fill(Piece, Shape);
	Piece->AddCollisionConvexMesh(Hull);
	// It lands on the road and walls, but never trips up people or cars.
	Piece->SetCollisionObjectType(ECC_PhysicsBody);
	Piece->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Piece->SetCollisionResponseToAllChannels(ECR_Block);
	Piece->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	Piece->SetCollisionResponseToChannel(ECC_Vehicle, ECR_Ignore);
	Piece->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	Piece->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	Piece->RegisterComponent();
	Piece->SetSimulatePhysics(true);
	Piece->SetMassOverrideInKg(NAME_None, Which == CarDamage::LampLeft || Which == CarDamage::LampRight ? CarVisuals::LampKg : CarVisuals::BumperKg, true);
	Piece->SetLinearDamping(0.2f);
	Piece->SetAngularDamping(0.5f);

	// On with the car, out from it and up, tumbling.
	const FVector Outward = FVector(Piece->GetComponentLocation().X - Frame->GetComponentLocation().X,
		Piece->GetComponentLocation().Y - Frame->GetComponentLocation().Y, 0.0).GetSafeNormal();
	const FVector Flung = LastVelocity * CarVisuals::LooseCarry + Outward * CarVisuals::LooseOut
		+ FVector(0.0, 0.0, CarVisuals::LooseUp * FMath::FRandRange(0.6f, 1.2f));
	Piece->SetPhysicsLinearVelocity(Flung);
	Piece->SetPhysicsAngularVelocityInDegrees(FVector(FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-1.f, 1.f)) * 500.f);
	LooseParts.Add(Piece);
	LooseSince.Add(GetWorld()->GetTimeSeconds());
}

void UCarVisuals::UpdateWear(float DeltaTime, double Distance)
{
	// Fallen parts stop costing anything once they have come to rest.
	const double Now = GetWorld()->GetTimeSeconds();
	for (int32 i = 0; i < LooseParts.Num(); ++i)
	{
		UProceduralMeshComponent* Piece = LooseParts[i];
		if (IsValid(Piece) && Piece->IsSimulatingPhysics() && Now - LooseSince[i] > CarVisuals::LooseRestTime)
		{
			Piece->SetSimulatePhysics(false);
			Piece->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}

	// A worn car's radiator steams, then smokes black, even once the engine has died.
	if (Damage.Wear < CarVisuals::SteamWear || !(bEngineRunning || IsEngineDead()) || Distance > CarVisuals::SmokeRange)
	{
		return;
	}
	SteamTimer -= DeltaTime;
	if (SteamTimer > 0.f)
	{
		return;
	}
	SteamTimer = CarVisuals::SteamInterval * FMath::FRandRange(0.7f, 1.3f);
	const bool bBlack = Damage.Wear > CarVisuals::SmokeWear;
	PuffFrom(CarDamage::RadiatorCap(CarModel::Layout(Style.Body)), FVector(0.0, 0.0, CarVisuals::SteamRise),
		bBlack ? FMath::FRandRange(10.f, 15.f) : FMath::FRandRange(7.f, 11.f), bBlack);
}
