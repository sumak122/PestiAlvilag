// Fill out your copyright notice in the Description page of Project Settings.

#include "MissionDzsokiUtok.h"

#include "Brawl.h"
#include "ChaseThief.h"
#include "Components/StaticMeshComponent.h"
#include "Cutscene.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "MusicBarModel.h"
#include "MusicBars.h"
#include "PersonModel.h"
#include "PestiAlvilagGameModeBase.h"
#include "PestiHud.h"
#include "PestiPerson.h"
#include "StartRingComponent.h"
#include "StoryPerson.h"
#include "UObject/ConstructorHelpers.h"

namespace DzsokiUtok
{
	// The lad's way (cm): from across the street from the tavern west along
	// it, then up the side streets past the körút.
	const FVector2D Route[] = {
		FVector2D(20995.0, 35410.0), FVector2D(18092.0, 36448.0), FVector2D(16075.0, 37377.0), FVector2D(15995.0, 37205.0),
		FVector2D(15316.0, 37527.0), FVector2D(13896.0, 37932.0), FVector2D(6667.0, 39658.0), FVector2D(6718.0, 40549.0),
		FVector2D(6191.0, 46295.0), FVector2D(10335.0, 46065.0), FVector2D(12335.0, 54630.0) };

	constexpr float TalkRadius = 250.f;
	constexpr float BundleRadius = 200.f;
	// He runs once the talk is over, or after this long at the most (s).
	constexpr double WaitMost = 9.0;
	// Tackling only once he has been running this long (s).
	constexpr double NoTackleSeconds = 1.5;
	// Further behind than this for this long, and he is gone (cm, s).
	constexpr double LostDistance = 4000.0;
	constexpr double LostSeconds = 3.0;
	// Walking away from the fight this far leaves him to get away (cm).
	constexpr double LeftDistance = 2500.0;
	// He shouts back once he is this far along.
	constexpr float TauntShare = 0.4f;
	// Trying again puts the player this far beyond the ring along the wall (cm).
	constexpr double RetryOut = 250.0;

	// The dropped hammers in their cloth, by him (cm).
	const FVector DropOffset(40.0, 55.0, 0.0);
	const FVector ClothSize(34.0, 14.0, 6.0);
	const FVector HandleSize(30.0, 1.6, 1.6);
	const FLinearColor Cloth(0.42f, 0.07f, 0.06f);
	const FLinearColor Wood(0.30f, 0.17f, 0.08f);

	constexpr int32 TipKey = 38;
	constexpr float TipSeconds = 12.f;

	const TCHAR* const DzsokiName = TEXT("Dzsoki");
	const TCHAR* const ThiefName = TEXT("A kölyök");
}

AMissionDzsokiUtok::AMissionDzsokiUtok()
{
	MissionId = FName(Id);
	Reward = 900;
	TitleHu = TEXT("Dzsoki ütői");
	TitleEn = TEXT("Dzsoki's hammers");
	Number = 0;
	bSideStory = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	CubeMesh = Cube.Object;
}

APestiPerson* AMissionDzsokiUtok::PlayerOnFoot() const
{
	return Cast<APestiPerson>(GetPlayer());
}

void AMissionDzsokiUtok::SetUpMission()
{
	AMusicBars* Bars = AMusicBars::Find(GetWorld());
	if (Bars)
	{
		Bars->SetDzsokiOut(true);
		GiverAt = Bars->GetGiverSpot();
		GiverYaw = Bars->GetStreetYaw(AMusicBars::BarTavern);
	}
	else
	{
		GiverAt = FVector(DzsokiUtok::Route[0], GroundZ(DzsokiUtok::Route[0]));
	}

	// Dzsoki outside his tavern in his street clothes, looking out over the street.
	Dzsoki = SpawnForMission<AStoryPerson>(GiverAt + FVector(0.0, 0.0, PersonModel::MiddleHeight), GiverYaw);
	if (Dzsoki)
	{
		Dzsoki->SetOutfit(MusicBarModel::DzsokiStreetLook());
	}

	// The lad loafs across the street with the hammers under his jacket.
	const FVector2D From = DzsokiUtok::Route[0];
	const FVector2D Look = FVector2D(GiverAt) - From;
	Thief = SpawnForMission<AChaseThief>(FVector(From, GroundZ(From)) + FVector(0.0, 0.0, PersonModel::MiddleHeight + 5.0),
		FMath::RadiansToDegrees(FMath::Atan2(Look.Y, Look.X)));
	if (Thief)
	{
		TArray<FVector2D> Way;
		for (const FVector2D& Point : DzsokiUtok::Route)
		{
			Way.Add(Point);
		}
		Thief->SetRoute(Way);
	}

	FMissionStep Talk;
	Talk.GoalHu = TEXT("Beszélj Dzsokival a Fehér Galamb előtt");
	Talk.GoalEn = TEXT("Talk to Dzsoki outside the Fehér Galamb");
	Talk.Target = GiverAt;
	Talk.Radius = DzsokiUtok::TalkRadius;
	Talk.bPressE = true;
	Talk.SceneSeconds = 8.f;
	Talk.SceneWith = Dzsoki.Get();
	Talk.Place = TEXT("Vásár utca");
	Steps.Add(Talk);
	bStartsAtRing = true;
	StartLetter = TEXT("D");

	FMissionStep Chase;
	Chase.GoalHu = TEXT("Kapd el a kölyököt, aki az ütőket vitte!");
	Chase.GoalEn = TEXT("Catch the lad who took the hammers!");
	Chase.Target = FVector(From, GroundZ(From));
	Chase.bManual = true;
	Chase.bGuide = false;
	Steps.Add(Chase);

	FMissionStep Fight;
	Fight.GoalHu = TEXT("Verd meg a kölyököt!");
	Fight.GoalEn = TEXT("Beat the lad!");
	Fight.bManual = true;
	Fight.bGuide = false;
	Steps.Add(Fight);

	FMissionStep Take;
	Take.GoalHu = TEXT("Vedd fel az ütőket");
	Take.GoalEn = TEXT("Pick up the hammers");
	Take.Radius = DzsokiUtok::BundleRadius;
	Take.bPressE = true;
	Take.bGuide = false;
	Steps.Add(Take);

	FMissionStep Back;
	Back.GoalHu = TEXT("Vidd vissza az ütőket Dzsokinak");
	Back.GoalEn = TEXT("Take the hammers back to Dzsoki");
	Back.Target = GiverAt;
	Back.Radius = DzsokiUtok::TalkRadius;
	Back.bPressE = true;
	Back.SceneSeconds = 8.f;
	Back.SceneWith = Dzsoki.Get();
	Back.Place = TEXT("Vásár utca");
	Steps.Add(Back);
}

void AMissionDzsokiUtok::OnStepStarted(int32 Step)
{
	if (Step == 1)
	{
		ChaseFrom = GetWorld()->GetTimeSeconds();
	}
	else if (Step == 2)
	{
		Say(DzsokiUtok::ThiefName, TEXT("Na gyere, ha mersz!"), TEXT("Come on then, if you dare!"), 4.f);
	}
	else if (Step == 3)
	{
		SetStepTarget(BundleAt);
	}
}

void AMissionDzsokiUtok::OnStepDone(int32 Step)
{
	APawn* Player = GetPlayer();
	if (Step == 0)
	{
		if (Dzsoki && Player)
		{
			Dzsoki->FaceTowards(Player->GetActorLocation());
		}
		Say(DzsokiUtok::DzsokiName,
			TEXT("Elvitték az ütőimet a színpadról! Húsz éve játszom velük... Ott! Az a kölyök a túlsó oldalon, ő volt!"),
			TEXT("They took my hammers off the stage! Twenty years I've played with them... There! That lad across the street, it was him!"), 8.f);
	}
	else if (Step == 2)
	{
		if (APestiPerson* Person = PlayerOnFoot())
		{
			Person->EndFight();
		}
		Say(DzsokiUtok::ThiefName,
			TEXT("Jól van, jól van! Itt vannak! Egy aranyfogú úr fizetett érte... azt mondta, Berki Gyuszi küldi!"),
			TEXT("All right, all right! Here they are! A man with a gold tooth paid me for it... said Berki Gyuszi sent him!"), 7.f);
		if (Thief)
		{
			Thief->ShowPurse(false);
			const FTransform Body(FRotator(0.0, Thief->GetActorRotation().Yaw, 0.0), Thief->GetActorLocation());
			DropBundle(Body.TransformPosition(DzsokiUtok::DropOffset));
		}
	}
	else if (Step == 3)
	{
		PickUpBundle();
		if (Thief)
		{
			Thief->Slink();
		}
	}
	else if (Step == 4)
	{
		if (Dzsoki && Player)
		{
			Dzsoki->FaceTowards(Player->GetActorLocation());
		}
		Say(DzsokiUtok::DzsokiName,
			TEXT("Az ütőim! Ezt nem felejtem el, fiam. Berki Gyuszi, mi? Előbb a lányomat, most a hangszeremet... Ezt még megkeserüli!"),
			TEXT("My hammers! I won't forget this, son. Berki Gyuszi, eh? First my daughter, now my instrument... He'll be sorry for this!"), 8.f);
	}
}

void AMissionDzsokiUtok::TickStep(int32 Step, float DeltaSeconds)
{
	const double Now = GetWorld()->GetTimeSeconds();
	APestiPerson* Person = PlayerOnFoot();
	const APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this);

	// Beaten and gone round the corner: out of the story.
	if (Step >= 3 && Thief && Thief->HasGotAway())
	{
		Thief->Destroy();
		Thief = nullptr;
	}
	if (!Thief)
	{
		return;
	}

	if (Step == 1)
	{
		// Seen: he is off as soon as Dzsoki has pointed him out.
		if (!bRunning)
		{
			if (!ACutscene::IsPlaying(this) || Now - ChaseFrom > DzsokiUtok::WaitMost)
			{
				bRunning = true;
				RunAt = Now;
				ACutscene::StopAll(this);
				Thief->ShowPurse(true);
				Thief->RunFrom(Person);
				Say(DzsokiUtok::DzsokiName, TEXT("Fogd meg! Fogd meg a gazembert!"), TEXT("Stop him! Stop the scoundrel!"), 4.f);
				APestiHUD::Show(this, DzsokiUtok::TipKey, DzsokiUtok::TipSeconds, FColor(255, 210, 60),
					TEXT("Tipp: Shifttel futsz, a Space-szel átugrod vagy alacsúszod, ami az útjában van"
						"  (Tip: hold Shift to run, press Space to vault over or slide under what is in the way)"));
			}
			return;
		}
		SetStepTarget(Thief->GetActorLocation());

		const double Behind = Person ? FVector::Dist2D(Person->GetActorLocation(), Thief->GetActorLocation()) : DzsokiUtok::LostDistance * 2.0;
		if (!bTaunted && Thief->RouteShare() > DzsokiUtok::TauntShare)
		{
			bTaunted = true;
			Say(DzsokiUtok::ThiefName, TEXT("Nem kapsz el, te mamlasz!"), TEXT("You'll never catch me, you lump!"), 3.f);
		}

		// Gone: at the end of his way, or left far behind.
		FarSince = Behind > DzsokiUtok::LostDistance ? (FarSince < 0.0 ? Now : FarSince) : -1.0;
		if (Thief->HasGotAway() || (FarSince >= 0.0 && Now - FarSince > DzsokiUtok::LostSeconds))
		{
			Fail(TEXT("Elmenekült a kölyök"), TEXT("The lad got away"));
			return;
		}

		// Close behind him, running, both on their feet: down he goes.
		const bool bCanTackle = Person && Now - RunAt > DzsokiUtok::NoTackleSeconds && !Person->IsFreeRunning() && !Thief->IsFreeRunning()
			&& Person->GetCharacterMovement()->IsMovingOnGround() && Behind <= Brawl::TackleReach
			&& Person->GetVelocity().Size2D() >= Brawl::TackleSpeed && !Person->IsFighting();
		if (bCanTackle)
		{
			Person->TackleDown(Thief);
			Thief->FightBack(Person);
			FinishStep();
		}
		return;
	}

	if (Step == 2)
	{
		SetStepTarget(Thief->GetActorLocation());
		if (Thief->IsBeaten())
		{
			FinishStep();
			return;
		}
		if (Game && (Game->IsFainted() || Game->GetHealth() <= 0.f))
		{
			Fail(TEXT("Kiütöttek"), TEXT("You were knocked out"));
			return;
		}
		if (!Person || FVector::Dist2D(Person->GetActorLocation(), Thief->GetActorLocation()) > DzsokiUtok::LeftDistance)
		{
			Fail(TEXT("Elmenekült a kölyök"), TEXT("The lad got away"));
			return;
		}
		// Back at it if he walks off and comes back.
		if (!Person->IsFighting())
		{
			Person->StartFight(Thief);
		}
	}
}

void AMissionDzsokiUtok::OnFailed()
{
	if (APestiPerson* Person = PlayerOnFoot())
	{
		if (Person->GetAct() != Brawl::EAct::None)
		{
			Person->EndFight();
		}
	}
	if (Thief && Thief->IsFighting())
	{
		Thief->EndFight();
	}
}

bool AMissionDzsokiUtok::GetRetrySpot(FVector& OutAt, double& OutYaw) const
{
	// Along the wall beyond Dzsoki's ring, looking at him; walking in starts it again.
	const FVector Along = FRotator(0.0, GiverYaw, 0.0).RotateVector(FVector(0.0, 1.0, 0.0));
	const FVector2D Spot = FVector2D(GiverAt + Along * (UStartRingComponent::Radius() + DzsokiUtok::RetryOut));
	OutAt = FVector(Spot, GroundZ(Spot, GiverAt.Z) + PersonModel::MiddleHeight + 10.0);
	OutYaw = GiverYaw - 90.0;
	return true;
}

void AMissionDzsokiUtok::DropBundle(const FVector& At)
{
	PickUpBundle();
	BundleAt = FVector(FVector2D(At), GroundZ(FVector2D(At), At.Z - PersonModel::MiddleHeight));
	const FRotator Lie(0.0, 30.0, 0.0);
	auto Add = [this, &Lie](const FVector& Where, const FVector& Size, const FLinearColor& Color)
	{
		if (!CubeMesh)
		{
			return;
		}
		UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(this, NAME_None, RF_Transient);
		Part->SetupAttachment(RootComponent);
		Part->SetStaticMesh(CubeMesh);
		Part->SetWorldLocationAndRotation(Where, Lie);
		Part->SetWorldScale3D(Size / 100.0);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetCanEverAffectNavigation(false);
		if (BaseMaterial)
		{
			UMaterialInstanceDynamic* Paint = UMaterialInstanceDynamic::Create(BaseMaterial, this);
			Paint->SetVectorParameterValue(TEXT("Color"), Color);
			Part->SetMaterial(0, Paint);
		}
		Part->RegisterComponent();
		Bundle.Add(Part);
	};
	// The red cloth, and the two hammer handles sticking out of it.
	Add(BundleAt + FVector(0.0, 0.0, DzsokiUtok::ClothSize.Z * 0.5), DzsokiUtok::ClothSize, DzsokiUtok::Cloth);
	for (int32 Side = -1; Side <= 1; Side += 2)
	{
		const FVector Off = Lie.RotateVector(FVector(14.0, Side * 3.5, 0.0));
		Add(BundleAt + Off + FVector(0.0, 0.0, DzsokiUtok::ClothSize.Z + 0.8), DzsokiUtok::HandleSize, DzsokiUtok::Wood);
	}
}

void AMissionDzsokiUtok::PickUpBundle()
{
	for (UStaticMeshComponent* Part : Bundle)
	{
		if (Part)
		{
			Part->DestroyComponent();
		}
	}
	Bundle.Reset();
}

void AMissionDzsokiUtok::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	PickUpBundle();
	if (APestiPerson* Person = PlayerOnFoot())
	{
		if (Person->GetAct() != Brawl::EAct::None)
		{
			Person->EndFight();
		}
	}
	// Back inside to his band.
	if (AMusicBars* Bars = AMusicBars::Find(GetWorld()))
	{
		Bars->SetDzsokiOut(false);
	}
	Super::EndPlay(EndPlayReason);
}
