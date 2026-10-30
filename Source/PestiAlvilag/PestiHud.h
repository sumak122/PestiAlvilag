// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "PestiHud.generated.h"

struct FCompositeFont;
class ALighters;
class SPestiHud;

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
 * lighters found (ALighters).
 *
 * Everything on screen goes through Show (by key, like the engine's debug
 * messages it replaces) or the goal, speech and announcement calls.
 */
UCLASS()
class PESTIALVILAG_API APestiHUD : public AHUD
{
	GENERATED_BODY()

public:
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

protected:
	virtual void BeginPlay() override;
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
	void Note(int32 Key, float Seconds, const FColor& Color, const FString& Text);

	TSharedPtr<SPestiHud> Widget;
	TSharedPtr<const FCompositeFont> DecoFont;
	TSharedPtr<const FCompositeFont> TextFont;
	TSharedPtr<const FCompositeFont> BoldFont;

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
};
