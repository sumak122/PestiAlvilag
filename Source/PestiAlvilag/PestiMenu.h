// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

class APestiHUD;
class APlayerController;
class UWorld;

/**
 * The player's own settings (volume, mouse, English subtitles), kept in
 * GameUserSettings.ini next to the engine's picture settings.
 */
namespace PestiSettings
{
	constexpr int32 Steps = 10;

	/** 0 to 10. */
	int32 Volume();
	/** 1 to 10; 5 is the normal speed. */
	int32 MouseSpeed();
	bool InvertMouse();
	bool English();
	/** Ray-traced lighting (the graphics card's ray tracing for Lumen): off unless switched on, as it costs a lot. */
	bool RayTracing();

	void SetVolume(UWorld* World, int32 Value);
	void SetMouseSpeed(int32 Value);
	void SetInvertMouse(bool bOn);
	void SetEnglish(bool bOn);
	void SetRayTracing(bool bOn);

	/** Sets the sound loudness from the saved volume (each time a level starts). */
	void ApplyVolume(UWorld* World);
	/** Switches the ray-traced lighting on or off as saved (each time a level starts, and when changed). */
	void ApplyRayTracing();
	/** The mouse movement of a frame, sped up or slowed and turned over as set. */
	void AdjustMouse(float& X, float& Y);
}

/** What a menu row does. */
enum class EPestiMenuAction : uint8
{
	None,
	Continue,
	NewGame,
	LoadSave,
	Settings,
	Controls,
	Credits,
	MainMenu,
	Quit,
	Yes,
	No,
	// The test grounds (ATestGrounds): the page, its rows, and back from it.
	TestAreas,
	TestFight,
	TestVehicles,
	TestBackToCity,
	/** Steps the weather on: by itself, clear, rain, fog (AWeather). */
	TestWeather,
	// The mission list on the test page, and starting the chosen mission.
	Missions,
	PlayMission,
	Back,
};

enum class EPestiMenuPage : uint8
{
	Closed,
	Title,
	Pause,
	Settings,
	Controls,
	Credits,
	Confirm,
	/** The test grounds: the fight yard, the vehicle yard, back to the city. */
	Tests,
	/** The act 1 missions, to start any of them for testing. */
	Missions,
};

struct FPestiMenuRow
{
	FString Hu;
	FString En;
	EPestiMenuAction Action = EPestiMenuAction::None;
	bool bEnabled = true;
};

/**
 * The menus: the title screen when the game starts, the pause menu on Esc
 * (or Tab, or the pad's Start button), the settings, the controls, the
 * credits and the "are you sure?" question. The game stands still while one
 * is open. APestiHUD owns it, ticks it and paints it.
 */
class FPestiMenu
{
public:
	/** The next level start goes straight into the game, without the title screen. */
	static void SkipTitleOnce();

	/** Called once when the HUD starts: the title screen, unless it was skipped. */
	void Start(UWorld* World);
	/** Reads the keys and moves through the menus. */
	void Tick(UWorld* World, APlayerController* PC);

	bool IsOpen() const { return Page != EPestiMenuPage::Closed; }
	EPestiMenuPage GetPage() const { return Page; }
	/** The page under the question or the settings: the title screen or the pause menu. */
	EPestiMenuPage GetBase() const { return Base; }
	int32 GetChoice() const { return Choice; }
	bool HasSave() const { return bHasSave; }
	/** The test grounds can be reached on foot only (not driving or riding). */
	bool IsOnFoot() const { return bOnFoot; }

	/** The rows of the title screen, the pause menu or the question. */
	TArray<FPestiMenuRow> Rows() const;
	/** The question being asked, in both languages. */
	void Question(FString& Hu, FString& En) const;

	/** The act 1 missions in the mission list. */
	static constexpr int32 MissionCount = 8;
	/** A mission in the list (0 for the first): its name (in capitals), and who gives it where. */
	static void MissionInfo(int32 Index, FString& NameHu, FString& NameEn, FString& WhereHu, FString& WhereEn);

	/** The settings rows. */
	static constexpr int32 SettingCount = 8;
	/** A settings row: its name, and either Bars (0 to 10) or a value to pick. */
	static void Setting(int32 Row, FString& Hu, FString& En, int32& Bars, FString& ValueHu, FString& ValueEn);

private:
	void Open(UWorld* World, EPestiMenuPage To);
	void Close(UWorld* World);
	void Do(UWorld* World, APlayerController* PC, EPestiMenuAction Action);
	void Ask(EPestiMenuAction What);
	void Move(int32 By);
	static void ChangeSetting(UWorld* World, int32 Row, int32 By);

	EPestiMenuPage Page = EPestiMenuPage::Closed;
	EPestiMenuPage Base = EPestiMenuPage::Closed;
	int32 Choice = 0;
	/** The row of the base page to come back to. */
	int32 BaseChoice = 0;
	EPestiMenuAction Asking = EPestiMenuAction::None;
	bool bHasSave = false;
	/** On foot when the menu opened, and out on the test grounds then. */
	bool bOnFoot = false;
	bool bOnTestGrounds = false;
	/** The weather the test page has set (an ETestWeather: 0 by itself, 1 clear, 2 rain, 3 fog). */
	int32 WeatherPick = 0;
	/** The title screen stops the game a moment after the level starts, once everything has had a frame to set itself up. */
	bool bStopSoon = false;
};
