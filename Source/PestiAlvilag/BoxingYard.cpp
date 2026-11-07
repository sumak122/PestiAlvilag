// Fill out your copyright notice in the Description page of Project Settings.

#include "BoxingYard.h"

#include "BoxingYardModel.h"
#include "Brawl.h"
#include "Brushes/SlateColorBrush.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PersonModel.h"
#include "PestiAlvilagGameModeBase.h"
#include "PestiHud.h"
#include "PestiPerson.h"
#include "Ruffian.h"
#include "StartRingComponent.h"
#include "StoryPerson.h"
#include "Styling/CoreStyle.h"
#include "UObject/ConstructorHelpers.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

namespace BoxingYardPlace
{
	// Things in the yard are not drawn from further off than this (cm).
	constexpr float DrawDistance = 20000.f;
	// The yard opens with mission 3 done.
	const FName OpenedBy(TEXT("Zsebtolvaj"));
	// The five men in order, what each pays (fillér), and how keen each is (ARuffian::Attack).
	const ERuffian Order[] = { ERuffian::BoxBandi, ERuffian::BoxJeno, ERuffian::BoxMazsas, ERuffian::BoxGyuri, ERuffian::BoxVasoklu };
	constexpr int32 NumRivals = 5;
	const int32 Purse[] = { 50, 80, 120, 150, 250 };
	const float Eager[] = { 0.6f, 0.85f, 0.9f, 0.9f, 1.f };
	/** Vasöklű again, after all five are beaten. */
	constexpr int32 RematchPurse = 100;
	// Bandi calls the fight off when the player's health is down to this.
	constexpr float CallOffBelow = 25.f;
	// And gives him a little back: a wet towel and a drink of water.
	constexpr float TowelHeal = 20.f;
	// Out of the ring this far from its middle (cm), and the fight is given up.
	constexpr double WalkOutReach = BoxingYardModel::RingHalf + 350.0;
	// The man on the ground is gone this long after (s).
	constexpr double ClearAfterWin = 5.0;
	constexpr double ClearAfterLoss = 2.0;
	// Bandi talks to the player this close (cm), and greets him first from this far.
	constexpr double TalkReach = 260.0;
	constexpr double GreetReach = 900.0;
	/** Out of the gold ring this far beyond its edge before it starts a fight again (cm). */
	constexpr double RingArmGap = 150.0;
	constexpr float SaySeconds = 5.f;
	// The slate: names still to beat, the next one, and the beaten ones.
	const FLinearColor ChalkToBeat(0.80f, 0.78f, 0.72f);
	const FLinearColor ChalkNext(0.95f, 0.85f, 0.45f);
	const FLinearColor ChalkBeaten(0.30f, 0.29f, 0.27f);
	// The key note while near Bandi.
	constexpr int32 HintLine = 62;

	// The lessons menu's colours (as the food menus).
	const FLinearColor Dim(0.f, 0.f, 0.f, 0.55f);
	const FLinearColor Panel(0.055f, 0.045f, 0.035f, 0.95f);
	const FLinearColor Row(0.11f, 0.09f, 0.07f, 1.f);
	const FLinearColor Gold(0.86f, 0.64f, 0.24f, 1.f);
	const FLinearColor Cream(0.96f, 0.91f, 0.80f, 1.f);
	const FLinearColor Grey(0.62f, 0.58f, 0.50f, 1.f);
	const FLinearColor Faded(0.40f, 0.37f, 0.32f, 1.f);
	const FLinearColor Ink(0.08f, 0.06f, 0.04f, 1.f);

	const FSlateBrush* White()
	{
		static const FSlateColorBrush Brush(FLinearColor::White);
		return &Brush;
	}

	/** One of Bandi's lessons as the menu shows it. */
	struct FLessonText
	{
		const TCHAR* Name;
		const TCHAR* English;
		const TCHAR* How;
	};
	const FLessonText Lessons[] = {
		{ TEXT("Gyomros"), TEXT("Body blow"), TEXT("H\u00e1tra + kattint\u00e1s: elfogy a szusza  (back + click: takes his breath)") },
		{ TEXT("Kit\u00e9r\u0151 \u00fct\u00e9s"), TEXT("Slip counter"), TEXT("Sz\u00f3k\u00f6z, azt\u00e1n kattint\u00e1s  (Space, then click: a punch from the side)") },
		{ TEXT("Fejel\u00e9s"), TEXT("Headbutt"), TEXT("Fog\u00e1s k\u00f6zben Sz\u00f3k\u00f6z  (Space while holding him)") },
		{ TEXT("Falhoz v\u00e1g\u00e1s"), TEXT("Wall throw"), TEXT("Fog\u00e1s k\u00f6zben, fal mellett: F  (holding him by a wall: F)") },
		{ TEXT("Kitart\u00e1s"), TEXT("More breath"), TEXT("Tov\u00e1bb b\u00edrod szusszal  (you tire later)") },
		{ TEXT("Kem\u00e9ny \u00e1ll"), TEXT("Hard chin"), TEXT("Kevesebbet fog rajtad az \u00fct\u00e9s  (blows hurt less)") },
	};
	constexpr int32 NumLessons = static_cast<int32>(Brawl::ELesson::Count);
	static_assert(UE_ARRAY_COUNT(Lessons) == NumLessons, "One text per lesson");
}

ABoxingYard::ABoxingYard()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	StartRing = CreateDefaultSubobject<UStartRingComponent>(TEXT("StartRing"));
	StartRing->SetupAttachment(RootComponent);
	// Not a story mission's ring: the yard has its own mark on the maps.
	StartRing->SetOnMap(false);

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicShapeMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	BaseMaterial = BasicShapeMaterial.Object;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMesh(TEXT("/Engine/BasicShapes/Cone.Cone"));
	Shapes = { CubeMesh.Object, CylinderMesh.Object, SphereMesh.Object, ConeMesh.Object };
}

FVector ABoxingYard::ToCity(const FVector& Local, double Ground)
{
	const FRotator Turn(0.0, BoxingYardModel::CityYaw, 0.0);
	return FVector(BoxingYardModel::CityAt, Ground) + Turn.RotateVector(Local);
}

void ABoxingYard::BeginPlay()
{
	Super::BeginPlay();

	TArray<MarketModel::FShape> Pieces;
	BoxingYardModel::Build(Pieces);
	for (const MarketModel::FShape& Piece : Pieces)
	{
		AddShape(Piece.Shape, Piece.Color, Piece.At, Piece.Size, FRotator(Piece.Pitch, Piece.Yaw, Piece.Roll), Piece.bCollide);
	}

	// The board over the gate, the slate's names, the old painted board on the back wall.
	TArray<BoxingYardModel::FSign> Signs;
	BoxingYardModel::Signs(Signs);
	for (int32 i = 0; i < Signs.Num(); ++i)
	{
		const BoxingYardModel::FSign& Sign = Signs[i];
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
		// The slate's five come right after the gate's board (BoxingYardModel::Signs).
		if (i >= 1 && i <= BoxingYardPlace::NumRivals)
		{
			SlateNames.Add(Text);
		}
	}
	UpdateSlate();
}

void ABoxingYard::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (LessonsWidget.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(LessonsWidget.ToSharedRef());
	}
	LessonsWidget.Reset();
	Super::EndPlay(EndPlayReason);
}

void ABoxingYard::SpawnPeople()
{
	bPeopleUp = true;
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const FTransform Frame = GetActorTransform();
	const FVector RingMiddle = Frame.TransformPosition(BoxingYardModel::RingAt);

	// Bandi, by his corner.
	const FVector BandiAt = Frame.TransformPosition(BoxingYardModel::Spots::Bandi) + FVector(0.0, 0.0, PersonModel::MiddleHeight);
	Bandi = GetWorld()->SpawnActor<AStoryPerson>(AStoryPerson::StaticClass(), BandiAt,
		FRotator(0.0, GetActorRotation().Yaw + BoxingYardModel::Spots::BandiYaw, 0.0), Params);
	if (Bandi)
	{
		Bandi->SetOutfit(BoxingYardModel::Bandi());
	}

	// The watchers standing round the ring and the boys up on the timber pile (the seated ones are left out: a story person stands).
	TArray<BoxingYardModel::FWatcher> Places;
	BoxingYardModel::Watchers(Places);
	for (int32 i = 0; i < Places.Num(); ++i)
	{
		const BoxingYardModel::FWatcher& Place = Places[i];
		if (Place.bSeated && !Place.bBoy)
		{
			continue;
		}
		const FVector At = Frame.TransformPosition(Place.At) + FVector(0.0, 0.0, PersonModel::MiddleHeight);
		AStoryPerson* Watcher = GetWorld()->SpawnActor<AStoryPerson>(AStoryPerson::StaticClass(), At,
			FRotator(0.0, GetActorRotation().Yaw + Place.Yaw, 0.0), Params);
		if (!Watcher)
		{
			continue;
		}
		PersonKit::FOutfit Clothes = BoxingYardModel::Watcher(i, Place.bBoy);
		Clothes.bSeated = false;
		Watcher->SetOutfit(Clothes);
		Watcher->FaceTowards(RingMiddle);
		Watchers.Add(Watcher);
	}
}

void ABoxingYard::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this);
	if (!Game || !Game->IsMissionDone(BoxingYardPlace::OpenedBy))
	{
		return;
	}
	if (!bPeopleUp)
	{
		SpawnPeople();
		UpdateSlate();
	}
	// The yard on the maps.
	if (APestiHUD* Hud = APestiHUD::Get(this))
	{
		Hud->MarkPlace(GetActorTransform().TransformPosition(BoxingYardModel::RingAt), TEXT("\u00d6"));
	}
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	APestiPerson* Player = PC ? Cast<APestiPerson>(PC->GetPawn()) : nullptr;

	// The lessons are open: choose, learn or leave.
	if (LessonsWidget.IsValid())
	{
		if (!Player || !PC)
		{
			CloseLessons();
			return;
		}
		const int32 Count = BoxingYardPlace::NumLessons;
		if (PC->WasInputKeyJustPressed(EKeys::W) || PC->WasInputKeyJustPressed(EKeys::Up) || PC->WasInputKeyJustPressed(EKeys::Gamepad_DPad_Up))
		{
			Selected = (Selected + Count - 1) % Count;
		}
		if (PC->WasInputKeyJustPressed(EKeys::S) || PC->WasInputKeyJustPressed(EKeys::Down) || PC->WasInputKeyJustPressed(EKeys::Gamepad_DPad_Down))
		{
			Selected = (Selected + 1) % Count;
		}
		if (PC->WasInputKeyJustPressed(EKeys::E) || PC->WasInputKeyJustPressed(EKeys::Enter) || PC->WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Bottom))
		{
			Learn();
		}
		else if (PC->WasInputKeyJustPressed(EKeys::Q) || PC->WasInputKeyJustPressed(EKeys::BackSpace) || PC->WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Right))
		{
			CloseLessons();
		}
		return;
	}

	// The man on the ground is helped out of the ring; Bandi is back at his corner.
	const double Now = GetWorld()->GetTimeSeconds();
	if (ClearAt > 0.0 && Now >= ClearAt)
	{
		ClearAt = -1.0;
		if (Rival)
		{
			Rival->Destroy();
			Rival = nullptr;
		}
		if (Bandi)
		{
			Bandi->SetInWorld(true);
		}
		Bout = EBout::None;
	}

	if (!Player)
	{
		return;
	}
	if (Bout == EBout::Fighting)
	{
		TickBout(Player);
		return;
	}
	if (Bout == EBout::None)
	{
		TickIdle(Player, PC);
	}
}

void ABoxingYard::TickIdle(APestiPerson* Player, APlayerController* PC)
{
	using namespace BoxingYardPlace;
	const FTransform Frame = GetActorTransform();
	const FVector RingMiddle = Frame.TransformPosition(BoxingYardModel::RingAt);
	const bool bOnFoot = !Player->IsRiding() && !Player->IsHidden() && !Player->IsCarrying() && !Player->IsFighting();

	// The gold ring in the middle of the ring: the next fight.
	if (!StartRing->IsShown())
	{
		StartRing->ShowAt(RingMiddle, TEXT("\u00d6"));
	}
	const double FromRing = FVector::Dist2D(Player->GetActorLocation(), RingMiddle);
	if (FromRing > UStartRingComponent::Radius() + RingArmGap)
	{
		bRingArmed = true;
	}
	if (bOnFoot && bRingArmed && FromRing <= UStartRingComponent::Radius())
	{
		StartBout(Player, PC);
		return;
	}

	// Bandi: a word the first time, then E for his lessons.
	if (!Bandi || !bOnFoot)
	{
		return;
	}
	const double FromBandi = FVector::Dist2D(Player->GetActorLocation(), Bandi->GetActorLocation());
	if (FromBandi < GreetReach)
	{
		Bandi->FaceTowards(Player->GetActorLocation());
		if (!bGreeted)
		{
			bGreeted = true;
			BandiSays(TEXT("Szervusz! Bandi vagyok. Ha van mersz, l\u00e9pj a k\u00f6telek k\u00f6z\u00e9, \u00e9s ha tanulni akarsz, sz\u00f3lj!"),
				TEXT("Hello! I'm Bandi. If you've the nerve, step between the ropes, and if you want to learn, say so!"));
		}
	}
	if (FromBandi < TalkReach)
	{
		const APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this);
		APestiHUD::Show(this, HintLine, 0.f, FColor(255, 210, 60),
			FString::Printf(TEXT("E: Bandi tan\u00edt, %d buny\u00f3pont  (E: Bandi's lessons, %d fight points)"),
				Game ? Game->GetFightPoints() : 0, Game ? Game->GetFightPoints() : 0));
		if (PC->WasInputKeyJustPressed(EKeys::E))
		{
			OpenLessons();
		}
	}
}

void ABoxingYard::StartBout(APestiPerson* Player, APlayerController* PC)
{
	using namespace BoxingYardPlace;
	APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this);
	if (!Game)
	{
		return;
	}
	bRingArmed = false;
	StartRing->Hide();
	BoutIndex = FMath::Clamp(Game->GetBoxingWins(), 0, NumRivals - 1);

	// Both to their marks, facing each other.
	const FTransform Frame = GetActorTransform();
	const double Ground = GetActorLocation().Z;
	const FVector PlayerAt = Frame.TransformPosition(BoxingYardModel::Spots::PlayerMark);
	const FVector RivalAt = Frame.TransformPosition(BoxingYardModel::Spots::RivalMark);
	const double PlayerYaw = (RivalAt - PlayerAt).Rotation().Yaw;
	Player->SetActorLocationAndRotation(FVector(PlayerAt.X, PlayerAt.Y, Ground + 100.0), FRotator(0.0, PlayerYaw, 0.0), false, nullptr, ETeleportType::TeleportPhysics);
	Player->GetCharacterMovement()->StopMovementImmediately();
	if (PC)
	{
		PC->SetControlRotation(FRotator(-10.0, PlayerYaw, 0.0));
	}

	const FTransform Spot(FRotator(0.0, PlayerYaw + 180.0, 0.0), FVector(RivalAt.X, RivalAt.Y, Ground + PersonModel::MiddleHeight + 10.0));
	Rival = GetWorld()->SpawnActorDeferred<ARuffian>(ARuffian::StaticClass(), Spot, this, nullptr,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (!Rival)
	{
		return;
	}
	Rival->Kind = Order[BoutIndex];
	Rival->FinishSpawning(Spot);
	// Bandi spars himself the first time: the one standing by the corner steps away.
	if (BoutIndex == 0 && Bandi)
	{
		Bandi->SetInWorld(false);
	}
	Rival->Attack(Player, Eager[BoutIndex]);
	Player->StartFight(Rival);
	Bout = EBout::Fighting;

	const bool bRematch = Game->GetBoxingWins() >= NumRivals;
	if (APestiHUD* Hud = APestiHUD::Get(this))
	{
		Hud->Announce(FString::Printf(TEXT("%d. MENET: %s"), BoutIndex + 1, *Rival->GetFightName()),
			FString::Printf(TEXT("Bout %d of 5%s"), BoutIndex + 1, bRematch ? TEXT(", a rematch") : TEXT("")), 3.f);
	}
	switch (BoutIndex)
	{
	case 0:
		BandiSays(TEXT("Na, l\u00e1ssuk, mit tudsz. Csak \u00f3vatosan, \u00e9n is \u00e9lni akarok!"),
			TEXT("Right, let's see what you've got. Go easy, I want to live too!"));
		break;
	case 1:
		BandiSays(TEXT("Jen\u0151 gyors, mint a gyal\u00e1z\u00f3 kecske. Ne hagyd, hogy t\u00e1ncoljon!"),
			TEXT("Jen\u0151 is quick as a goat on a roof. Don't let him dance about!"));
		break;
	case 2:
		BandiSays(TEXT("M\u00e1zs\u00e1s zs\u00e1kot hord a p\u00e1lyaudvaron. Ha eltal\u00e1l, a h\u00e1zad is beleremeg."),
			TEXT("M\u00e1zs\u00e1s carries sacks at the station. If he lands one, your whole house shakes."));
		break;
	case 3:
		BandiSays(TEXT("Gyuri b\u00fajik a keszty\u0171je m\u00f6g\u00e9. Menj al\u00e1 vagy ker\u00fclj oldalra!"),
			TEXT("Gyuri hides behind his fists. Get under them or come round the side!"));
		break;
	default:
		BandiSays(TEXT("Vas\u00f6kl\u0171. Nem sz\u00e9p, nem okos, de m\u00e9g senki nem vitte le a l\u00e1b\u00e1r\u00f3l. Sok szerencs\u00e9t!"),
			TEXT("Vas\u00f6kl\u0171. Not pretty, not clever, but nobody has put him down yet. Good luck!"));
		break;
	}
}

void ABoxingYard::TickBout(APestiPerson* Player)
{
	using namespace BoxingYardPlace;
	APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this);
	if (!Rival || !Game)
	{
		EndBout(Player, false, false);
		return;
	}
	if (Rival->IsBeaten())
	{
		EndBout(Player, true, false);
		return;
	}
	const FVector RingMiddle = GetActorTransform().TransformPosition(BoxingYardModel::RingAt);
	if (FVector::Dist2D(Player->GetActorLocation(), RingMiddle) > WalkOutReach || Player->IsRiding())
	{
		EndBout(Player, false, true);
		return;
	}
	if (Game->GetHealth() <= CallOffBelow || Game->IsFainted())
	{
		EndBout(Player, false, false);
		return;
	}
	// Knocked out of the fight (he walked off it, or a scene ended it): back in, facing him.
	if (!Player->IsFighting() && Player->GetAct() != Brawl::EAct::Beaten)
	{
		Player->StartFight(Rival);
	}
}

void ABoxingYard::EndBout(APestiPerson* Player, bool bWon, bool bWalkedOut)
{
	using namespace BoxingYardPlace;
	APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this);
	const double Now = GetWorld()->GetTimeSeconds();
	Bout = EBout::Over;
	if (Player && Player->GetAct() != Brawl::EAct::None)
	{
		Player->EndFight();
	}
	if (bWon && Game)
	{
		const bool bRematch = Game->GetBoxingWins() >= NumRivals;
		const int32 Pay = bRematch ? RematchPurse : Purse[BoutIndex];
		Game->AddMoney(Pay);
		if (!bRematch)
		{
			Game->AddBoxingWin();
		}
		UpdateSlate();
		if (APestiHUD* Hud = APestiHUD::Get(this))
		{
			Hud->Announce(TEXT("GY\u0150ZT\u00c9L!"), FString::Printf(TEXT("You won!  +%s"), *APestiAlvilagGameModeBase::FormatMoney(Pay)), 3.f);
		}
		if (Game->GetBoxingWins() >= NumRivals && !bRematch)
		{
			BandiSays(TEXT("Lefektetted Vas\u00f6kl\u0171t! Ezt az eg\u00e9sz Fuvaros utca eml\u00edteni fogja. Te vagy a grund bajnoka!"),
				TEXT("You put Vas\u00f6kl\u0171 down! The whole of Fuvaros utca will talk of it. You're the champion of the grund!"));
		}
		else if (BoutIndex == 0)
		{
			BandiSays(TEXT("Hopp\u00e1, ez f\u00e1jt! J\u00f3 az \u00f6kl\u00f6d. Ha tanulni akarsz, sz\u00f3lj, a t\u00f6bbiek majd j\u00f6nnek."),
				TEXT("Ouch, that hurt! You've a good fist. If you want to learn, say so; the others will come."));
		}
		else
		{
			BandiSays(TEXT("Sz\u00e9p volt! Kih\u00fazom a tabl\u00e1r\u00f3l. J\u00f6het a k\u00f6vetkez\u0151, ha \u00fajra bel\u00e9psz."),
				TEXT("Well done! I'll cross him off the slate. The next one's ready when you step in again."));
		}
		ClearAt = Now + ClearAfterWin;
		return;
	}

	// Called off, or given up: the other man puts his fists down and stops coming on.
	if (Rival)
	{
		Rival->StandAgain(Rival->GetActorLocation(), Rival->GetActorRotation().Yaw);
	}
	if (bWalkedOut)
	{
		BandiSays(TEXT("Hov\u00e1 m\u00e9sz? Aki kil\u00e9p a k\u00f6telek k\u00f6z\u00fcl, az feladta."),
			TEXT("Where are you off to? Whoever steps out of the ropes has given up."));
	}
	else
	{
		if (Game && !Game->IsFainted())
		{
			Game->HealPlayer(TowelHeal);
		}
		BandiSays(TEXT("El\u00e9g, el\u00e9g! Ma nem a te napod. Egy kis v\u00edz, \u00e9s majd legk\u00f6zelebb."),
			TEXT("Enough, enough! Not your day. A drop of water, and next time."));
	}
	ClearAt = Now + ClearAfterLoss;
}

void ABoxingYard::UpdateSlate()
{
	using namespace BoxingYardPlace;
	const APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this);
	const int32 Wins = Game ? Game->GetBoxingWins() : 0;
	for (int32 i = 0; i < SlateNames.Num(); ++i)
	{
		if (UTextRenderComponent* Name = SlateNames[i])
		{
			const FLinearColor& Chalk = i < Wins ? ChalkBeaten : i == Wins ? ChalkNext : ChalkToBeat;
			Name->SetTextRenderColor(Chalk.ToFColor(true));
		}
	}
}

void ABoxingYard::BandiSays(const FString& Hu, const FString& En) const
{
	if (APestiHUD* Hud = APestiHUD::Get(this))
	{
		Hud->Say(TEXT("Bandi"), Hu, En, BoxingYardPlace::SaySeconds);
	}
}

void ABoxingYard::OpenLessons()
{
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC || !GEngine || !GEngine->GameViewport)
	{
		return;
	}
	Selected = 0;
	LessonsWidget = BuildLessonsWidget();
	GEngine->GameViewport->AddViewportWidgetContent(LessonsWidget.ToSharedRef(), 10);
	PC->SetIgnoreMoveInput(true);
	PC->SetIgnoreLookInput(true);
	BandiSays(TEXT("Mit tan\u00edtsak? Minden vereked\u00e9s ad egy kis buny\u00f3pontot."),
		TEXT("What shall I teach you? Every fight earns you a few fight points."));
}

void ABoxingYard::CloseLessons()
{
	if (LessonsWidget.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(LessonsWidget.ToSharedRef());
	}
	LessonsWidget.Reset();
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		PC->SetIgnoreMoveInput(false);
		PC->SetIgnoreLookInput(false);
	}
}

void ABoxingYard::Learn()
{
	APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this);
	if (!Game)
	{
		return;
	}
	const int32 Cost = Brawl::LessonCost(static_cast<Brawl::ELesson>(Selected));
	if (Game->KnowsLesson(Selected))
	{
		BandiSays(TEXT("Ezt m\u00e1r tudod. V\u00e1lassz m\u00e1st!"), TEXT("You know that one already. Pick another!"));
		return;
	}
	if (!Game->LearnLesson(Selected, Cost))
	{
		BandiSays(FString::Printf(TEXT("Ehhez m\u00e9g %d buny\u00f3pont kell. Verekedj egy kicsit!"), Cost - Game->GetFightPoints()),
			FString::Printf(TEXT("You need %d more fight points for that. Go and fight a bit!"), Cost - Game->GetFightPoints()));
		return;
	}
	const BoxingYardPlace::FLessonText& Lesson = BoxingYardPlace::Lessons[Selected];
	BandiSays(FString::Printf(TEXT("Figyelj, \u00edgy kell: %s. Na, pr\u00f3b\u00e1ld ki valakin!"), Lesson.Name),
		FString::Printf(TEXT("Watch, like this: %s. Now try it on somebody!"), Lesson.English));
}

TSharedRef<SWidget> ABoxingYard::BuildLessonsWidget()
{
	using namespace BoxingYardPlace;
	const FSlateFontInfo TitleFont = FCoreStyle::GetDefaultFontStyle("Bold", 30);
	const FSlateFontInfo NameFont = FCoreStyle::GetDefaultFontStyle("Bold", 20);
	const FSlateFontInfo SmallFont = FCoreStyle::GetDefaultFontStyle("Regular", 13);
	const FSlateFontInfo TextFont = FCoreStyle::GetDefaultFontStyle("Regular", 15);
	const FSlateFontInfo BigFont = FCoreStyle::GetDefaultFontStyle("Bold", 24);

	// One row per lesson; the chosen row turns gold, the ones learnt say so, the ones too dear are faded.
	TSharedRef<SVerticalBox> List = SNew(SVerticalBox);
	for (int32 Index = 0; Index < NumLessons; ++Index)
	{
		const FLessonText& Lesson = Lessons[Index];
		const int32 Cost = Brawl::LessonCost(static_cast<Brawl::ELesson>(Index));
		auto Known = [this, Index]()
		{
			const APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this);
			return Game && Game->KnowsLesson(Index);
		};
		auto Affordable = [this, Cost]()
		{
			const APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this);
			return Game && Game->GetFightPoints() >= Cost;
		};
		auto RowColor = [this, Index, Known, Affordable](const FLinearColor& On, const FLinearColor& Off)
		{
			return [this, Index, Known, Affordable, On, Off]()
			{
				if (Index == Selected)
				{
					return FSlateColor(On);
				}
				return FSlateColor(Known() || Affordable() ? Off : Faded);
			};
		};
		List->AddSlot()
		.AutoHeight()
		.Padding(0.f, 3.f)
		[
			SNew(SBorder)
			.BorderImage(White())
			.BorderBackgroundColor_Lambda([this, Index]() { return FSlateColor(Index == Selected ? Gold : Row); })
			.Padding(FMargin(16.f, 9.f))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1.f)
				.VAlign(VAlign_Center)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(STextBlock)
						.Text(FText::FromString(FString::Printf(TEXT("%s   %s"), Lesson.Name, Lesson.English)))
						.Font(NameFont)
						.ColorAndOpacity_Lambda(RowColor(Ink, Cream))
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(STextBlock)
						.Text(FText::FromString(Lesson.How))
						.Font(SmallFont)
						.ColorAndOpacity_Lambda(RowColor(Ink, Grey))
					]
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(16.f, 0.f, 0.f, 0.f)
				[
					SNew(STextBlock)
					.Text_Lambda([Known, Cost]()
					{
						return FText::FromString(Known() ? FString(TEXT("MEGTANULTAD  (learned)")) : FString::Printf(TEXT("%d pont"), Cost));
					})
					.Font(NameFont)
					.ColorAndOpacity_Lambda(RowColor(Ink, Gold))
				]
			]
		];
	}

	return SNew(SBorder)
		.BorderImage(White())
		.BorderBackgroundColor(Dim)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SBox)
			.WidthOverride(980.f)
			[
				SNew(SBorder)
				.BorderImage(White())
				.BorderBackgroundColor(Panel)
				.Padding(28.f)
				[
					SNew(SVerticalBox)
					// The heading, and the points in hand.
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.f, 0.f, 0.f, 6.f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.FillWidth(1.f)
						.VAlign(VAlign_Bottom)
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(STextBlock).Text(FText::FromString(TEXT("BANDI TAN\u00cdT"))).Font(TitleFont).ColorAndOpacity(Gold)
							]
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(STextBlock).Text(FText::FromString(TEXT("Bandi teaches"))).Font(TextFont).ColorAndOpacity(Grey)
							]
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Bottom)
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right)
							[
								SNew(STextBlock)
								.Text_Lambda([this]()
								{
									const APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this);
									return FText::AsNumber(Game ? Game->GetFightPoints() : 0);
								})
								.Font(BigFont)
								.ColorAndOpacity(Gold)
							]
							+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right)
							[
								SNew(STextBlock).Text(FText::FromString(TEXT("BUNY\u00d3PONT  (fight points)"))).Font(SmallFont).ColorAndOpacity(Grey)
							]
						]
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.f, 4.f, 0.f, 14.f)
					[
						SNew(SBox).HeightOverride(2.f)
						[
							SNew(SBorder).BorderImage(White()).BorderBackgroundColor(Gold)
						]
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						List
					]
					// How points come, and the keys.
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.f, 16.f, 0.f, 0.f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("Pontot minden vereked\u00e9s ad: \u00fct\u00e9s 1, nagy \u00fct\u00e9s 3, le\u00fct\u00e9s 5, gy\u0151zelem 10.  (Every fight earns points: a hit 1, a big blow 3, a knockdown 5, a win 10.)")))
						.Font(SmallFont)
						.ColorAndOpacity(Grey)
						.AutoWrapText(true)
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.f, 10.f, 0.f, 0.f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("W / S: v\u00e1laszt\u00e1s (choose)      E: megtanulom (learn)      Q: vissza (back)")))
						.Font(TextFont)
						.ColorAndOpacity(Grey)
					]
				]
			]
		];
}

void ABoxingYard::AddShape(int32 Shape, const FLinearColor& Color, const FVector& At, const FVector& Size, const FRotator& Turn, bool bCollide)
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
		Instances->SetCullDistances(0, BoxingYardPlace::DrawDistance);
		Instances->SetCanEverAffectNavigation(false);
		Instances->RegisterComponent();
		BatchIndex.Add(Key, Batches.Add(Instances));
	}
	Instances->AddInstance(FTransform(Turn, At, Size / 100.0), false);
}
