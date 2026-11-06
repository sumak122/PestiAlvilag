// Fill out your copyright notice in the Description page of Project Settings.

#include "MissionZsebtolvaj.h"

#include "ChaseModel.h"
#include "ChaseThief.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "FleaMarket.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HallMarketModel.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "MarketHall.h"
#include "PersonModel.h"
#include "PestiAlvilagGameModeBase.h"
#include "PestiHud.h"
#include "PestiPerson.h"
#include "StoryPerson.h"
#include "UObject/ConstructorHelpers.h"

namespace Zsebtolvaj
{
	// In the market hall's frame (cm): Bözsi néni at a vegetable stall on
	// the nave, facing it; the pickpocket browsing at a stall further in;
	// trying again starts by the gate end of the nave, looking at her.
	const FVector2D BozsiAt(1300.0, 760.0);
	constexpr double BozsiYaw = 90.0;
	const FVector2D ThiefAt(2350.0, 420.0);
	constexpr double ThiefYaw = 90.0;
	const FVector2D RetryAt(800.0, 350.0);

	constexpr float TalkRadius = 250.f;
	constexpr float PurseRadius = 200.f;

	// The snatch: he sidles up for at most this long (s). Tackling only
	// once he has been running this long (s).
	constexpr double SneakSeconds = 10.0;
	// Before that he shops at the stalls near her at the front end of the
	// nave (the cross hall round his old spot has none), between these
	// (hall frame, cm), keeping this far from her and from where a retry
	// puts the player, and where nobody else stands this close (cm).
	constexpr double BrowseFromX = 450.0;
	constexpr double BrowseToX = 1550.0;
	constexpr double BrowseHalfY = 850.0;
	constexpr double BrowseKeepOff = 300.0;
	constexpr double BrowseTaken = 120.0;
	constexpr double NoTackleSeconds = 1.5;
	// Further behind than this for this long, and he is gone (cm, s).
	constexpr double LostDistance = 4000.0;
	constexpr double LostSeconds = 3.0;
	// Walking away from the fight this far leaves him to get away (cm).
	constexpr double LeftDistance = 2500.0;
	// He shouts back once he is this far along.
	constexpr float TauntShare = 0.35f;

	// The dropped purse, on its side by him (cm).
	const FVector DropOffset(40.0, 55.0, 0.0);
	const FVector PurseSize(14.0, 10.0, 5.0);
	const FVector ClaspSize(12.0, 1.5, 5.6);
	const FLinearColor Leather(0.24f, 0.11f, 0.05f);
	const FLinearColor Brass(0.62f, 0.48f, 0.20f);

	// Tips under the goal, once each.
	constexpr int32 TipKey = 38;
	constexpr float TipSeconds = 12.f;

	const TCHAR* const BozsiName = TEXT("B\u00f6zsi n\u00e9ni");
	const TCHAR* const ThiefName = TEXT("Zsebtolvaj");
}

AMissionZsebtolvaj::AMissionZsebtolvaj()
{
	MissionId = TEXT("Zsebtolvaj");
	Reward = 660;
	TitleHu = TEXT("Zsebtolvaj");
	TitleEn = TEXT("The pickpocket");

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	CubeMesh = Cube.Object;
}

FVector AMissionZsebtolvaj::HallPoint(const FVector2D& Local) const
{
	const FVector2D At = AMarketHall::ToCity(Local);
	return FVector(At, GroundZ(At));
}

APestiPerson* AMissionZsebtolvaj::PlayerOnFoot() const
{
	return Cast<APestiPerson>(GetPlayer());
}

void AMissionZsebtolvaj::SetUpMission()
{
	// Bözsi néni goes shopping at the hall for this.
	BozsiAtStall = AFleaMarket::FindKeeper(GetWorld(), Zsebtolvaj::BozsiName);
	if (BozsiAtStall)
	{
		BozsiAtStall->SetInWorld(false);
	}

	const double HallYaw = AMarketHall::Yaw();
	const FVector BozsiFeet = HallPoint(Zsebtolvaj::BozsiAt);
	Bozsi = SpawnForMission<AStoryPerson>(BozsiFeet + FVector(0.0, 0.0, PersonModel::MiddleHeight), HallYaw + Zsebtolvaj::BozsiYaw);
	if (Bozsi)
	{
		PersonKit::FOutfit Shopper;
		if (BozsiAtStall)
		{
			Shopper = BozsiAtStall->GetOutfit();
		}
		else
		{
			Shopper.bWoman = true;
			Shopper.Height = 160.f;
			Shopper.Hat = PersonKit::EHat::None;
			Shopper.Top = PersonKit::ETop::Dress;
			Shopper.bTie = false;
		}
		Shopper.Held = PersonKit::EHeld::Basket;
		Bozsi->SetOutfit(Shopper);
	}

	// The pickpocket is just one more shopper going from stall to stall
	// near her, nothing to mark him out till he strikes. He starts at the
	// free spot nearest his old place; his way out is worked out from the
	// hall.
	TArray<FVector2D> BrowseAt;
	TArray<double> BrowseYaw;
	int32 FirstSpot = INDEX_NONE;
	{
		TArray<HallMarket::FStall> HallStalls;
		TArray<HallMarket::FShopper> HallShoppers;
		TArray<HallMarket::FLamp> HallLamps;
		HallMarket::Layout(HallStalls, HallShoppers, HallLamps);
		double Nearest = TNumericLimits<double>::Max();
		for (const HallMarket::FStall& Stall : HallStalls)
		{
			const double Facing = FMath::DegreesToRadians(Stall.Yaw);
			const FVector2D Spot = FVector2D(Stall.At) + FVector2D(FMath::Cos(Facing), FMath::Sin(Facing)) * HallMarket::ShopperOut;
			if (Spot.X < Zsebtolvaj::BrowseFromX || Spot.X > Zsebtolvaj::BrowseToX || FMath::Abs(Spot.Y) > Zsebtolvaj::BrowseHalfY
				|| FVector2D::Distance(Spot, Zsebtolvaj::BozsiAt) < Zsebtolvaj::BrowseKeepOff
				|| FVector2D::Distance(Spot, Zsebtolvaj::RetryAt) < Zsebtolvaj::BrowseKeepOff)
			{
				continue;
			}
			bool bTaken = false;
			for (const HallMarket::FShopper& Other : HallShoppers)
			{
				bTaken = bTaken || FVector2D::Distance(Spot, FVector2D(Other.At)) < Zsebtolvaj::BrowseTaken;
			}
			if (bTaken)
			{
				continue;
			}
			const double Off = FVector2D::Distance(Spot, Zsebtolvaj::ThiefAt);
			if (Off < Nearest)
			{
				Nearest = Off;
				FirstSpot = BrowseAt.Num();
			}
			BrowseAt.Add(AMarketHall::ToCity(Spot));
			BrowseYaw.Add(HallYaw + Stall.Yaw + 180.0);
		}
	}
	const FVector ThiefFeet = HallPoint(Zsebtolvaj::ThiefAt);
	const FVector ThiefStart = BrowseAt.IsValidIndex(FirstSpot) ? FVector(BrowseAt[FirstSpot], GroundZ(BrowseAt[FirstSpot])) : ThiefFeet;
	const double ThiefStartYaw = BrowseAt.IsValidIndex(FirstSpot) ? BrowseYaw[FirstSpot] : HallYaw + Zsebtolvaj::ThiefYaw;
	Thief = SpawnForMission<AChaseThief>(ThiefStart + FVector(0.0, 0.0, PersonModel::MiddleHeight + 5.0), ThiefStartYaw);
	if (Thief)
	{
		Thief->Browse(BrowseAt, BrowseYaw, FirstSpot);
		TArray<FVector2D> Route;
		for (const FVector2D& Local : ChaseModel::ThiefRoute())
		{
			Route.Add(AMarketHall::ToCity(Local));
		}
		Thief->SetRoute(Route);
	}

	FMissionStep Find;
	Find.GoalHu = TEXT("Keresd meg B\u00f6zsi n\u00e9nit a R\u00e1k\u00f3czi t\u00e9ri v\u00e1s\u00e1rcsarnokban");
	Find.GoalEn = TEXT("Find Aunt B\u00f6zsi in the R\u00e1k\u00f3czi t\u00e9r market hall");
	Find.Target = BozsiFeet;
	Find.Radius = Zsebtolvaj::TalkRadius;
	Find.bPressE = true;
	Steps.Add(Find);

	FMissionStep Chase;
	Chase.GoalHu = TEXT("Kapd el a zsebtolvajt!");
	Chase.GoalEn = TEXT("Catch the pickpocket!");
	Chase.Target = ThiefFeet;
	Chase.bManual = true;
	Chase.bGuide = false;
	Steps.Add(Chase);

	FMissionStep Fight;
	Fight.GoalHu = TEXT("Verd meg a tolvajt!");
	Fight.GoalEn = TEXT("Beat the thief!");
	Fight.bManual = true;
	Fight.bGuide = false;
	Steps.Add(Fight);

	FMissionStep Take;
	Take.GoalHu = TEXT("Vedd fel a p\u00e9nzt\u00e1rc\u00e1t");
	Take.GoalEn = TEXT("Pick up the purse");
	Take.Radius = Zsebtolvaj::PurseRadius;
	Take.bPressE = true;
	Take.bGuide = false;
	Steps.Add(Take);

	FMissionStep Back;
	Back.GoalHu = TEXT("Vidd vissza a p\u00e9nzt\u00e1rc\u00e1t B\u00f6zsi n\u00e9ninek");
	Back.GoalEn = TEXT("Take the purse back to Aunt B\u00f6zsi");
	Back.Target = BozsiFeet;
	Back.Radius = Zsebtolvaj::TalkRadius;
	Back.bPressE = true;
	Steps.Add(Back);
}

void AMissionZsebtolvaj::OnStepStarted(int32 Step)
{
	if (Step == 1 && Thief && Bozsi)
	{
		// While she talks he sidles up behind her.
		Thief->SneakUpTo(Bozsi);
		SneakFrom = GetWorld()->GetTimeSeconds();
	}
	else if (Step == 2)
	{
		Say(Zsebtolvaj::ThiefName, TEXT("Na gyere, ha mersz!"), TEXT("Come on then, if you dare!"), 4.f);
		APestiHUD::Show(this, Zsebtolvaj::TipKey, Zsebtolvaj::TipSeconds, FColor(255, 210, 60),
			TEXT("Tipp: ha h\u00e1trah\u00fazza az \u00f6kl\u00e9t, a jobb gombbal ellent\u00e1madhatsz; megragadva falnak l\u00f6kheted"
				"  (Tip: when he pulls back his fist, right click to counter; grab him and shove him into a wall)"));
	}
	else if (Step == 3)
	{
		SetStepTarget(PurseAt);
	}
}

void AMissionZsebtolvaj::OnStepDone(int32 Step)
{
	APawn* Player = GetPlayer();
	if (Step == 0)
	{
		if (Bozsi && Player)
		{
			Bozsi->FaceTowards(Player->GetActorLocation());
		}
		Say(Zsebtolvaj::BozsiName,
			TEXT("Ha m\u00e1r itt l\u00e1batlankodik, vigye majd haza a kosarat. Csak megveszem a krumplit."),
			TEXT("Since you're under my feet anyway, carry the basket home. I'll just get the potatoes."), 6.f);
	}
	else if (Step == 2)
	{
		if (APestiPerson* Person = PlayerOnFoot())
		{
			Person->EndFight();
		}
		Say(Zsebtolvaj::ThiefName, TEXT("El\u00e9g, el\u00e9g! Itt a p\u00e9nz, vigye!"), TEXT("Enough, enough! Here's the money, take it!"), 5.f);
		if (Thief)
		{
			Thief->ShowPurse(false);
			const FTransform Body(FRotator(0.0, Thief->GetActorRotation().Yaw, 0.0), Thief->GetActorLocation());
			DropPurse(Body.TransformPosition(Zsebtolvaj::DropOffset));
		}
	}
	else if (Step == 3)
	{
		PickUpPurse();
		if (Thief)
		{
			Thief->Slink();
		}
	}
	else if (Step == 4)
	{
		if (Bozsi && Player)
		{
			Bozsi->FaceTowards(Player->GetActorLocation());
		}
		Say(Zsebtolvaj::BozsiName,
			TEXT("Na... ezt nem gondoltam volna mag\u00e1r\u00f3l. Tess\u00e9k, ez a p\u00e1r peng\u0151 a mag\u00e1\u00e9."),
			TEXT("Well... I wouldn't have thought it of you. Here, these few peng\u0151 are yours."), 7.f);
	}
}

void AMissionZsebtolvaj::TickStep(int32 Step, float DeltaSeconds)
{
	const double Now = GetWorld()->GetTimeSeconds();
	APestiPerson* Person = PlayerOnFoot();
	APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this);

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
		// The snatch: he reaches her (or gives up sidling) and is off with her purse.
		if (!bSnatched)
		{
			if (Thief->IsAtMark() || Now - SneakFrom > Zsebtolvaj::SneakSeconds)
			{
				bSnatched = true;
				SnatchAt = Now;
				Thief->ShowPurse(true);
				Thief->RunFrom(Cast<APestiPerson>(GetPlayer()));
				Say(Zsebtolvaj::BozsiName, TEXT("Jaj! A p\u00e9nzt\u00e1rc\u00e1m! Fogj\u00e1k meg, tolvaj!"), TEXT("Oh! My purse! Stop, thief!"), 4.f);
				APestiHUD::Show(this, Zsebtolvaj::TipKey, Zsebtolvaj::TipSeconds, FColor(255, 210, 60),
					TEXT("Tipp: Shifttel futsz, a Space-szel \u00e1tugrod vagy alacs\u00faszod, ami az \u00fatj\u00e1ban van"
						"  (Tip: hold Shift to run, press Space to vault over or slide under what is in the way)"));
			}
			return;
		}
		SetStepTarget(Thief->GetActorLocation());

		const double Behind = GetPlayer() ? FVector::Dist2D(GetPlayer()->GetActorLocation(), Thief->GetActorLocation()) : Zsebtolvaj::LostDistance * 2.0;
		if (!bTaunted && Thief->RouteShare() > Zsebtolvaj::TauntShare)
		{
			bTaunted = true;
			Say(Zsebtolvaj::ThiefName, TEXT("Nem kapsz el, te mamlasz!"), TEXT("You'll never catch me, you lump!"), 3.f);
		}

		// Gone: at the end of his way, or left far behind.
		FarSince = Behind > Zsebtolvaj::LostDistance ? (FarSince < 0.0 ? Now : FarSince) : -1.0;
		if (Thief->HasGotAway() || (FarSince >= 0.0 && Now - FarSince > Zsebtolvaj::LostSeconds))
		{
			Fail(TEXT("Elmenek\u00fclt a tolvaj"), TEXT("The thief got away"));
			return;
		}

		// Close behind him, running, both on their feet: down he goes.
		const bool bCanTackle = Person && Now - SnatchAt > Zsebtolvaj::NoTackleSeconds && !Person->IsFreeRunning() && !Thief->IsFreeRunning()
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
		if ((Game && (Game->IsFainted() || Game->GetHealth() <= 0.f)))
		{
			Fail(TEXT("Ki\u00fct\u00f6ttek"), TEXT("You were knocked out"));
			return;
		}
		if (!Person || FVector::Dist2D(Person->GetActorLocation(), Thief->GetActorLocation()) > Zsebtolvaj::LeftDistance)
		{
			Fail(TEXT("Elmenek\u00fclt a tolvaj"), TEXT("The thief got away"));
			return;
		}
		// Back at it if he walks off into a car and comes back on foot.
		if (!Person->IsFighting())
		{
			Person->StartFight(Thief);
		}
	}
}

void AMissionZsebtolvaj::OnFailed()
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

bool AMissionZsebtolvaj::GetRetrySpot(FVector& OutAt, double& OutYaw) const
{
	OutAt = HallPoint(Zsebtolvaj::RetryAt) + FVector(0.0, 0.0, PersonModel::MiddleHeight + 10.0);
	const FVector2D Look = AMarketHall::ToCity(Zsebtolvaj::BozsiAt) - AMarketHall::ToCity(Zsebtolvaj::RetryAt);
	OutYaw = FMath::RadiansToDegrees(FMath::Atan2(Look.Y, Look.X));
	return true;
}

void AMissionZsebtolvaj::DropPurse(const FVector& At)
{
	PickUpPurse();
	PurseAt = FVector(FVector2D(At), GroundZ(FVector2D(At), At.Z - PersonModel::MiddleHeight));
	auto Add = [this](const FVector& Where, const FVector& Size, const FLinearColor& Color)
	{
		if (!CubeMesh)
		{
			return;
		}
		UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(this, NAME_None, RF_Transient);
		Part->SetupAttachment(RootComponent);
		Part->SetStaticMesh(CubeMesh);
		Part->SetWorldLocationAndRotation(Where, FRotator(0.0, 30.0, 0.0));
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
		Purse.Add(Part);
	};
	// Lying on its side: the leather, the brass clasp along its top edge.
	Add(PurseAt + FVector(0.0, 0.0, Zsebtolvaj::PurseSize.Z * 0.5), Zsebtolvaj::PurseSize, Zsebtolvaj::Leather);
	const FVector Edge = FRotator(0.0, 30.0, 0.0).RotateVector(FVector(0.0, Zsebtolvaj::PurseSize.Y * 0.5, 0.0));
	Add(PurseAt + Edge + FVector(0.0, 0.0, Zsebtolvaj::PurseSize.Z * 0.5), Zsebtolvaj::ClaspSize, Zsebtolvaj::Brass);
}

void AMissionZsebtolvaj::PickUpPurse()
{
	for (UStaticMeshComponent* Part : Purse)
	{
		if (Part)
		{
			Part->DestroyComponent();
		}
	}
	Purse.Reset();
}

void AMissionZsebtolvaj::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	PickUpPurse();
	if (APestiPerson* Person = PlayerOnFoot())
	{
		if (Person->GetAct() != Brawl::EAct::None)
		{
			Person->EndFight();
		}
	}
	// Back to her stall.
	if (BozsiAtStall)
	{
		BozsiAtStall->SetInWorld(true);
	}
	Super::EndPlay(EndPlayReason);
}
