// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "PestiMenu.h"
#include "PestiHud.generated.h"

struct FCompositeFont;
class ALighters;
class SPestiHud;
class SWidget;
class URouteGuideComponent;

/**
 * The screen panels, in an art deco dress after 1930s Budapest shop signs:
 * dark plaques with stepped corners and gold lines, lettering in Limelight
 * and Josefin Sans (Data/Fonts).
 *
 * Top left the current goal, in Hungarian with the English under it, and how
 * far and which way; under it the other short notes (hints, controls). Top
 * right the money and the health bars, and red diamonds when the police want
 * you. At the bottom who is talking and what they say, and in the middle the
 * big announcements, and under the middle the key to press for a move
 * (a vault, a slide, a counter). In a fist fight, the other man's health
 * at the top and the player's breath under his health. Over everything, while it is open, the cabinet of
 * lighters found (ALighters). While a menu is open (FPestiMenu: the title
 * screen, Esc's pause menu) only the menu shows, over the city blurred.
 *
 * Bottom left the minimap (PestiMap's city plan), turned the way the camera
 * looks, with the player's arrow in the middle and the street's name under
 * it: a gold diamond for the goal (on the rim when it is further) and the
 * way there as a gold line, a red diamond and red line for the player's own
 * mark, and Bözsi néni's house. M opens the big map, with the game stopped:
 * the mouse or W A S D moves the cross, the wheel or Q and E zoom, a left
 * click (or Enter) puts the own mark there and a right click (or Delete)
 * takes it away.
 *
 * Everything on screen goes through Show (by key, like the engine's debug
 * messages it replaces) or the goal, speech and announcement calls.
 */
UCLASS()
class PESTIALVILAG_API APestiHUD : public AHUD
{
	GENERATED_BODY()

public:
	APestiHUD();

	/** The player's HUD, or null. */
	static APestiHUD* Get(const UObject* WorldContextObject);

	/**
	 * A line of text under a key: a new line under the same key replaces
	 * the old one, and empty text takes it away. Seconds 0 shows it for this
	 * frame only. Keys 6 and 8 are people talking (bottom of the screen), 9
	 * a big announcement; the rest are notes under the goal. "Hungarian  (English)"
	 * is split into its two languages.
	 */
	static void Show(const UObject* WorldContextObject, int32 Key, float Seconds, const FColor& Color, const FString& Text);

	/** The goal panel: what to do, and on the right a short word or two (distance and way, or a key). */
	void SetGoal(const FString& Hu, const FString& En, const FString& Far, const FString& Way);
	void ClearGoal();
	/** Someone speaks: their name over the line, Hungarian with the English under it. */
	void Say(const FString& Who, const FString& Hu, const FString& En, float Seconds);
	/** True while someone's line is on the screen. */
	bool IsSaying() const { return Speech.Until >= Now(); }
	/** A big line across the middle of the screen. */
	void Announce(const FString& Hu, const FString& En, float Seconds);
	/** The lighter collection cabinet over everything, or null to put it away. */
	void SetCabinet(const ALighters* Shown);
	/**
	 * A handwritten letter held up over the screen, with the English on a
	 * plaque beside it, until ClearLetter. In Hu, \n breaks a line, an empty
	 * line parts paragraphs and a line starting with > sits on the right.
	 */
	void SetLetter(const FString& Hu, const FString& En);
	void ClearLetter();
	/**
	 * A newspaper cutting held up over the screen: the headline over the
	 * story, the English on a plaque beside it, until ClearClipping. In Body
	 * and En, \n breaks a line and an empty line parts paragraphs.
	 */
	void SetClipping(const FString& Headline, const FString& Body, const FString& En);
	void ClearClipping();
	/**
	 * For this frame, a key sign under the middle of the screen: the key
	 * and what it does, in Hungarian with the English under it. bNow lights
	 * the key up: this is the moment to press it.
	 */
	void Prompt(const FString& Key, const FString& Hu, const FString& En, bool bNow);
	/**
	 * For this frame, a fist fight: the other man's name and health on a
	 * plaque at the top of the screen, and the player's breath under his
	 * health (shares, 0 to 1).
	 */
	void ShowFight(const FString& FoeName, float FoeHealth, float Breath);

	/** True while the title screen or another menu is open (the game stands still). */
	bool IsMenuOpen() const { return Menu.IsOpen(); }

	/** For this frame, the goal's diamond on the minimap (UGoalMarkerComponent calls it while it shows). */
	void MarkGoal(const FVector& At);
	/** For this frame, the way to the goal on the minimap, or to the player's own mark (URouteGuideComponent calls it). */
	void ShowRoute(const TArray<FVector>& Points, bool bOwnMark);
	/** True while the big map is open (the game stands still). */
	bool IsMapOpen() const { return bMapOpen; }

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	friend class SPestiHud;

	struct FNote
	{
		FString Text;
		FLinearColor Color;
		double Until = 0.0;
	};
	struct FPrompt
	{
		FString Key;
		FString Hu;
		FString En;
		bool bNow = false;
		double Until = -1.0;
	};
	struct FFight
	{
		FString Name;
		float Health = 1.f;
		float Breath = 1.f;
		double Until = -1.0;
	};
	struct FSpeech
	{
		FString Who;
		FString Hu;
		FString En;
		double Until = -1.0;
	};

	double Now() const;
	/** Where the player is and looks, the zoom, the street's name and the own mark's route. */
	void TickMinimap(APlayerController* PC);
	void OpenMap();
	void CloseMap();
	/** The big map's keys: move the cross, zoom, set or clear the own mark, close. */
	void TickMap(APlayerController* PC);
	void Note(int32 Key, float Seconds, const FColor& Color, const FString& Text);

	TSharedPtr<SPestiHud> Widget;
	/** Blurs the city behind an open menu. */
	TSharedPtr<SWidget> Blur;
	FPestiMenu Menu;
	TSharedPtr<const FCompositeFont> DecoFont;
	TSharedPtr<const FCompositeFont> TextFont;
	TSharedPtr<const FCompositeFont> BoldFont;
	TSharedPtr<const FCompositeFont> HandFont;
	TSharedPtr<const FCompositeFont> PaperFont;

	TMap<int32, FNote> Notes;
	FString GoalHu;
	FString GoalEn;
	FString GoalFar;
	FString GoalWay;
	FSpeech Speech;
	FSpeech Big;
	FPrompt KeySign;
	FFight Fight;
	TWeakObjectPtr<const ALighters> Cabinet;
	FString LetterHu;
	FString LetterEn;
	FString ClippingHeadline;
	FString ClippingBody;
	FString ClippingEn;

	// The minimap: what it shows this frame (until the time given).
	FVector2D GoalBlipAt = FVector2D::ZeroVector;
	double GoalBlipUntil = -1.0;
	TArray<FVector2D> GoalRoute;
	double GoalRouteUntil = -1.0;
	TArray<FVector2D> MarkRoute;
	double MarkRouteUntil = -1.0;
	/** The player's own mark, set on the big map. */
	bool bHasMark = false;
	FVector MarkAt = FVector::ZeroVector;
	/** Plans the way to the own mark. */
	UPROPERTY(Transient)
	TObjectPtr<URouteGuideComponent> MarkGuide;

	// Where the player is, which way the camera and the player face (degrees), and how much the minimap shows (cm across).
	bool bHasView = false;
	FVector2D ViewAt = FVector2D::ZeroVector;
	FVector2D LastViewAt = FVector2D::ZeroVector;
	float ViewYaw = 0.f;
	float FacingYaw = 0.f;
	float ViewSpeed = 0.f;
	double MiniAcross = 26000.0;
	FString StreetName;
	double NextStreetTime = 0.0;
	bool bHasHome = false;
	FVector2D HomeAt = FVector2D::ZeroVector;

	// The big map: its middle and the cross (world cm) and how much it shows (cm across).
	bool bMapOpen = false;
	FVector2D MapCentre = FVector2D::ZeroVector;
	FVector2D MapCursor = FVector2D::ZeroVector;
	double MapAcross = 90000.0;
	/** The big map's size on the screen, from the last time it was painted (for the keys). */
	mutable FVector2f MapWindow = FVector2f(1300.f, 940.f);
};
