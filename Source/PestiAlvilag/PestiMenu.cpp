// Fill out your copyright notice in the Description page of Project Settings.

#include "PestiMenu.h"

#include "AudioDevice.h"
#include "CoreGlobals.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/GameUserSettings.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/ConfigCacheIni.h"
#include "PestiAlvilagGameModeBase.h"
#include "PestiPerson.h"
#include "PestiSaveGame.h"
#include "TestGrounds.h"

namespace PestiSettings
{
	namespace Store
	{
		const TCHAR* const Section = TEXT("PestiAlvilag.Settings");
		bool bLoaded = false;
		int32 VolumeSteps = Steps;
		int32 MouseSteps = 5;
		bool bInvert = false;
		bool bEnglishOn = true;

		void Load()
		{
			if (bLoaded || !GConfig)
			{
				return;
			}
			bLoaded = true;
			GConfig->GetInt(Section, TEXT("Volume"), VolumeSteps, GGameUserSettingsIni);
			GConfig->GetInt(Section, TEXT("MouseSpeed"), MouseSteps, GGameUserSettingsIni);
			GConfig->GetBool(Section, TEXT("InvertMouse"), bInvert, GGameUserSettingsIni);
			GConfig->GetBool(Section, TEXT("English"), bEnglishOn, GGameUserSettingsIni);
			VolumeSteps = FMath::Clamp(VolumeSteps, 0, Steps);
			MouseSteps = FMath::Clamp(MouseSteps, 1, Steps);
		}

		void Write()
		{
			if (!GConfig)
			{
				return;
			}
			GConfig->SetInt(Section, TEXT("Volume"), VolumeSteps, GGameUserSettingsIni);
			GConfig->SetInt(Section, TEXT("MouseSpeed"), MouseSteps, GGameUserSettingsIni);
			GConfig->SetBool(Section, TEXT("InvertMouse"), bInvert, GGameUserSettingsIni);
			GConfig->SetBool(Section, TEXT("English"), bEnglishOn, GGameUserSettingsIni);
			GConfig->Flush(false, GGameUserSettingsIni);
		}
	}

	int32 Volume() { Store::Load(); return Store::VolumeSteps; }
	int32 MouseSpeed() { Store::Load(); return Store::MouseSteps; }
	bool InvertMouse() { Store::Load(); return Store::bInvert; }
	bool English() { Store::Load(); return Store::bEnglishOn; }

	void SetVolume(UWorld* World, int32 Value)
	{
		Store::Load();
		Store::VolumeSteps = FMath::Clamp(Value, 0, Steps);
		Store::Write();
		ApplyVolume(World);
	}

	void SetMouseSpeed(int32 Value)
	{
		Store::Load();
		Store::MouseSteps = FMath::Clamp(Value, 1, Steps);
		Store::Write();
	}

	void SetInvertMouse(bool bOn)
	{
		Store::Load();
		Store::bInvert = bOn;
		Store::Write();
	}

	void SetEnglish(bool bOn)
	{
		Store::Load();
		Store::bEnglishOn = bOn;
		Store::Write();
	}

	void ApplyVolume(UWorld* World)
	{
		if (!World)
		{
			return;
		}
		if (FAudioDevice* Audio = World->GetAudioDeviceRaw())
		{
			Audio->SetTransientPrimaryVolume(static_cast<float>(Volume()) / Steps);
		}
	}

	void AdjustMouse(float& X, float& Y)
	{
		const float Speed = static_cast<float>(MouseSpeed()) / 5.f;
		X *= Speed;
		Y *= InvertMouse() ? -Speed : Speed;
	}
}

namespace PestiMenu
{
	// Set before a level is opened again from the menu; the next HUD to start uses it up.
	bool bSkipTitle = false;
	// The title screen stops the game this long after the level starts.
	constexpr double StopAfter = 0.5;

	bool Pressed(const APlayerController* PC, const FKey& A, const FKey& B, const FKey& C)
	{
		return PC->WasInputKeyJustPressed(A) || PC->WasInputKeyJustPressed(B) || PC->WasInputKeyJustPressed(C);
	}

	UGameUserSettings* Picture()
	{
		return GEngine ? GEngine->GetGameUserSettings() : nullptr;
	}

	void OpenLevelAgain(UWorld* World)
	{
		UGameplayStatics::OpenLevel(World, FName(*UGameplayStatics::GetCurrentLevelName(World)));
	}
}

void FPestiMenu::SkipTitleOnce()
{
	PestiMenu::bSkipTitle = true;
}

void FPestiMenu::Start(UWorld* World)
{
	PestiSettings::ApplyVolume(World);
	if (PestiMenu::bSkipTitle)
	{
		PestiMenu::bSkipTitle = false;
		return;
	}
	Open(World, EPestiMenuPage::Title);
	bStopSoon = true;
}

void FPestiMenu::Open(UWorld* World, EPestiMenuPage To)
{
	bHasSave = UGameplayStatics::DoesSaveGameExist(UPestiSaveGame::SlotName(), 0);
	const APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	const APestiPerson* Walker = PC ? Cast<APestiPerson>(PC->GetPawn()) : nullptr;
	bOnFoot = Walker && !Walker->IsRiding();
	bOnTestGrounds = World && ATestGrounds::IsPlayerHere(World);
	Page = To;
	Base = To;
	Choice = 0;
	Asking = EPestiMenuAction::None;
	if (To == EPestiMenuPage::Pause && World)
	{
		UGameplayStatics::SetGamePaused(World, true);
	}
}

void FPestiMenu::Close(UWorld* World)
{
	Page = EPestiMenuPage::Closed;
	Base = EPestiMenuPage::Closed;
	bStopSoon = false;
	if (World)
	{
		UGameplayStatics::SetGamePaused(World, false);
	}
}

TArray<FPestiMenuRow> FPestiMenu::Rows() const
{
	TArray<FPestiMenuRow> Out;
	if (Page == EPestiMenuPage::Tests)
	{
		Out.Add({ TEXT("VEREKED\u0150 UDVAR"), TEXT("Fight yard"), EPestiMenuAction::TestFight, bOnFoot });
		Out.Add({ TEXT("J\u00c1RM\u0170TELEP"), TEXT("Vehicle yard"), EPestiMenuAction::TestVehicles, bOnFoot });
		Out.Add({ TEXT("VISSZA A V\u00c1ROSBA"), TEXT("Back to the city"), EPestiMenuAction::TestBackToCity, bOnFoot && bOnTestGrounds });
		Out.Add({ TEXT("VISSZA"), TEXT("Back"), EPestiMenuAction::Back });
	}
	else if (Page == EPestiMenuPage::Confirm)
	{
		Out.Add({ TEXT("IGEN"), TEXT("Yes"), EPestiMenuAction::Yes });
		Out.Add({ TEXT("NEM"), TEXT("No"), EPestiMenuAction::No });
	}
	else if (Base == EPestiMenuPage::Title)
	{
		if (bHasSave)
		{
			Out.Add({ TEXT("FOLYTAT\u00c1S"), TEXT("Continue"), EPestiMenuAction::Continue });
		}
		Out.Add({ TEXT("\u00daJ J\u00c1T\u00c9K"), TEXT("New game"), EPestiMenuAction::NewGame });
		Out.Add({ TEXT("BE\u00c1LL\u00cdT\u00c1SOK"), TEXT("Settings"), EPestiMenuAction::Settings });
		Out.Add({ TEXT("K\u00c9SZ\u00cdT\u0150K"), TEXT("Credits"), EPestiMenuAction::Credits });
		Out.Add({ TEXT("KIL\u00c9P\u00c9S"), TEXT("Quit"), EPestiMenuAction::Quit });
	}
	else
	{
		Out.Add({ TEXT("FOLYTAT\u00c1S"), TEXT("Resume"), EPestiMenuAction::Continue });
		Out.Add({ TEXT("UTOLS\u00d3 MENT\u00c9S"), TEXT("Load last save"), EPestiMenuAction::LoadSave, bHasSave });
		Out.Add({ TEXT("BE\u00c1LL\u00cdT\u00c1SOK"), TEXT("Settings"), EPestiMenuAction::Settings });
		Out.Add({ TEXT("IR\u00c1NY\u00cdT\u00c1S"), TEXT("Controls"), EPestiMenuAction::Controls });
		Out.Add({ TEXT("PR\u00d3BATEREP"), TEXT("Test areas"), EPestiMenuAction::TestAreas });
		Out.Add({ TEXT("F\u0150MEN\u00dc"), TEXT("Main menu"), EPestiMenuAction::MainMenu });
		Out.Add({ TEXT("KIL\u00c9P\u00c9S"), TEXT("Quit game"), EPestiMenuAction::Quit });
	}
	return Out;
}

void FPestiMenu::Question(FString& Hu, FString& En) const
{
	switch (Asking)
	{
	case EPestiMenuAction::NewGame:
		Hu = TEXT("\u00daj j\u00e1t\u00e9kot kezdesz? A ment\u00e9sed t\u00f6rl\u0151dik.");
		En = TEXT("Start a new game? Your save will be deleted.");
		break;
	case EPestiMenuAction::LoadSave:
		Hu = TEXT("Visszat\u00e9rsz az utols\u00f3 ment\u00e9shez? Ami az\u00f3ta t\u00f6rt\u00e9nt, elv\u00e9sz.");
		En = TEXT("Go back to the last save? What happened since then will be lost.");
		break;
	case EPestiMenuAction::MainMenu:
		Hu = TEXT("Kil\u00e9psz a f\u0151men\u00fcbe? Ami nincs elmentve, elv\u00e9sz.");
		En = TEXT("Quit to the main menu? Anything not saved will be lost.");
		break;
	default:
		Hu = TEXT("Kil\u00e9psz a j\u00e1t\u00e9kb\u00f3l? Ami nincs elmentve, elv\u00e9sz.");
		En = TEXT("Quit the game? Anything not saved will be lost.");
		break;
	}
}

void FPestiMenu::Setting(int32 Row, FString& Hu, FString& En, int32& Bars, FString& ValueHu, FString& ValueEn)
{
	Bars = -1;
	ValueHu.Reset();
	ValueEn.Reset();
	const UGameUserSettings* Look = PestiMenu::Picture();
	auto OnOff = [&ValueHu, &ValueEn](bool bOn)
	{
		ValueHu = bOn ? TEXT("Be") : TEXT("Ki");
		ValueEn = bOn ? TEXT("On") : TEXT("Off");
	};
	switch (Row)
	{
	case 0:
		Hu = TEXT("Hanger\u0151");
		En = TEXT("volume");
		Bars = PestiSettings::Volume();
		break;
	case 1:
		Hu = TEXT("Eg\u00e9r \u00e9rz\u00e9kenys\u00e9ge");
		En = TEXT("mouse speed");
		Bars = PestiSettings::MouseSpeed();
		break;
	case 2:
		Hu = TEXT("Eg\u00e9r fel-le ford\u00edtva");
		En = TEXT("invert mouse up-down");
		ValueHu = PestiSettings::InvertMouse() ? TEXT("Igen") : TEXT("Nem");
		ValueEn = PestiSettings::InvertMouse() ? TEXT("Yes") : TEXT("No");
		break;
	case 3:
		Hu = TEXT("Angol felirat");
		En = TEXT("English subtitles");
		OnOff(PestiSettings::English());
		break;
	case 4:
	{
		Hu = TEXT("K\u00e9pmin\u0151s\u00e9g");
		En = TEXT("graphics");
		static const TCHAR* const LevelsHu[] = { TEXT("Alacsony"), TEXT("K\u00f6zepes"), TEXT("Magas"), TEXT("Epikus") };
		static const TCHAR* const LevelsEn[] = { TEXT("Low"), TEXT("Medium"), TEXT("High"), TEXT("Epic") };
		const int32 Level = Look ? Look->GetOverallScalabilityLevel() : -1;
		if (Level >= 0 && Level < static_cast<int32>(UE_ARRAY_COUNT(LevelsHu)))
		{
			ValueHu = LevelsHu[Level];
			ValueEn = LevelsEn[Level];
		}
		else
		{
			ValueHu = TEXT("Egy\u00e9ni");
			ValueEn = TEXT("Custom");
		}
		break;
	}
	case 5:
	{
		Hu = TEXT("K\u00e9perny\u0151");
		En = TEXT("screen");
		const bool bWindow = Look && Look->GetFullscreenMode() == EWindowMode::Windowed;
		ValueHu = bWindow ? TEXT("Ablak") : TEXT("Teljes k\u00e9perny\u0151");
		ValueEn = bWindow ? TEXT("Window") : TEXT("Full screen");
		break;
	}
	default:
		Hu = TEXT("F\u00fcgg\u0151leges szinkron");
		En = TEXT("V-sync (no tearing)");
		OnOff(Look && Look->IsVSyncEnabled());
		break;
	}
}

void FPestiMenu::ChangeSetting(UWorld* World, int32 Row, int32 By)
{
	UGameUserSettings* Look = PestiMenu::Picture();
	switch (Row)
	{
	case 0:
		PestiSettings::SetVolume(World, PestiSettings::Volume() + By);
		return;
	case 1:
		PestiSettings::SetMouseSpeed(PestiSettings::MouseSpeed() + By);
		return;
	case 2:
		PestiSettings::SetInvertMouse(!PestiSettings::InvertMouse());
		return;
	case 3:
		PestiSettings::SetEnglish(!PestiSettings::English());
		return;
	default:
		break;
	}
	if (!Look)
	{
		return;
	}
	if (Row == 4)
	{
		constexpr int32 Levels = 4;
		const int32 Level = Look->GetOverallScalabilityLevel();
		const int32 Next = Level < 0 ? 2 : (Level + By + Levels) % Levels;
		Look->SetOverallScalabilityLevel(Next);
	}
	else if (Row == 5)
	{
		const bool bWindow = Look->GetFullscreenMode() == EWindowMode::Windowed;
		Look->SetFullscreenMode(bWindow ? EWindowMode::WindowedFullscreen : EWindowMode::Windowed);
	}
	else
	{
		Look->SetVSyncEnabled(!Look->IsVSyncEnabled());
	}
	Look->ApplySettings(false);
}

void FPestiMenu::Ask(EPestiMenuAction What)
{
	BaseChoice = Choice;
	Asking = What;
	Page = EPestiMenuPage::Confirm;
	// "No" is chosen first, so a quick Enter loses nothing.
	Choice = 1;
}

void FPestiMenu::Move(int32 By)
{
	if (Page == EPestiMenuPage::Settings)
	{
		Choice = (Choice + By + SettingCount) % SettingCount;
		return;
	}
	const TArray<FPestiMenuRow> List = Rows();
	if (List.Num() == 0)
	{
		return;
	}
	// Step over rows that can't be chosen now.
	for (int32 Tries = 0; Tries < List.Num(); ++Tries)
	{
		Choice = (Choice + By + List.Num()) % List.Num();
		if (List[Choice].bEnabled)
		{
			return;
		}
	}
}

void FPestiMenu::Do(UWorld* World, APlayerController* PC, EPestiMenuAction Action)
{
	switch (Action)
	{
	case EPestiMenuAction::Continue:
		Close(World);
		break;
	case EPestiMenuAction::NewGame:
		if (bHasSave)
		{
			Ask(EPestiMenuAction::NewGame);
		}
		else
		{
			// No save: the city already starts as a new game.
			Close(World);
		}
		break;
	case EPestiMenuAction::LoadSave:
	case EPestiMenuAction::MainMenu:
		Ask(Action);
		break;
	case EPestiMenuAction::Quit:
		if (Base == EPestiMenuPage::Title)
		{
			UKismetSystemLibrary::QuitGame(World, PC, EQuitPreference::Quit, false);
		}
		else
		{
			Ask(EPestiMenuAction::Quit);
		}
		break;
	case EPestiMenuAction::Settings:
		BaseChoice = Choice;
		Page = EPestiMenuPage::Settings;
		Choice = 0;
		break;
	case EPestiMenuAction::Controls:
		BaseChoice = Choice;
		Page = EPestiMenuPage::Controls;
		break;
	case EPestiMenuAction::Credits:
		BaseChoice = Choice;
		Page = EPestiMenuPage::Credits;
		break;
	case EPestiMenuAction::TestAreas:
		BaseChoice = Choice;
		Page = EPestiMenuPage::Tests;
		Choice = 0;
		// Not on foot: straight to "back", the only row that can be chosen.
		if (!bOnFoot)
		{
			Choice = Rows().Num() - 1;
		}
		break;
	case EPestiMenuAction::TestFight:
	case EPestiMenuAction::TestVehicles:
		Close(World);
		ATestGrounds::Go(World, Action == EPestiMenuAction::TestFight ? ETestGround::Fight : ETestGround::Vehicles);
		break;
	case EPestiMenuAction::TestBackToCity:
		Close(World);
		ATestGrounds::GoBack(World);
		break;
	case EPestiMenuAction::Back:
		Page = Base;
		Choice = BaseChoice;
		break;
	case EPestiMenuAction::No:
		Page = Base;
		Choice = BaseChoice;
		Asking = EPestiMenuAction::None;
		break;
	case EPestiMenuAction::Yes:
		if (Asking == EPestiMenuAction::NewGame)
		{
			if (APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(World))
			{
				Game->StartNewGame();
			}
		}
		else if (Asking == EPestiMenuAction::LoadSave)
		{
			// The level starts again, reads the save and puts the player at the home door.
			SkipTitleOnce();
			PestiMenu::OpenLevelAgain(World);
		}
		else if (Asking == EPestiMenuAction::MainMenu)
		{
			PestiMenu::OpenLevelAgain(World);
		}
		else if (Asking == EPestiMenuAction::Quit)
		{
			UKismetSystemLibrary::QuitGame(World, PC, EQuitPreference::Quit, false);
		}
		break;
	default:
		break;
	}
}

void FPestiMenu::Tick(UWorld* World, APlayerController* PC)
{
	if (!World || !PC)
	{
		return;
	}
	if (bStopSoon && World->GetTimeSeconds() >= PestiMenu::StopAfter)
	{
		bStopSoon = false;
		UGameplayStatics::SetGamePaused(World, true);
	}

	const bool bMenuKey = PestiMenu::Pressed(PC, EKeys::Escape, EKeys::Tab, EKeys::Gamepad_Special_Right);
	if (Page == EPestiMenuPage::Closed)
	{
		if (bMenuKey)
		{
			Open(World, EPestiMenuPage::Pause);
		}
		return;
	}

	const bool bUp = PestiMenu::Pressed(PC, EKeys::Up, EKeys::W, EKeys::Gamepad_DPad_Up);
	const bool bDown = PestiMenu::Pressed(PC, EKeys::Down, EKeys::S, EKeys::Gamepad_DPad_Down);
	const bool bLeft = PestiMenu::Pressed(PC, EKeys::Left, EKeys::A, EKeys::Gamepad_DPad_Left);
	const bool bRight = PestiMenu::Pressed(PC, EKeys::Right, EKeys::D, EKeys::Gamepad_DPad_Right);
	const bool bOk = PestiMenu::Pressed(PC, EKeys::Enter, EKeys::SpaceBar, EKeys::Gamepad_FaceButton_Bottom)
		|| PC->WasInputKeyJustPressed(EKeys::E);
	const bool bBack = bMenuKey || PestiMenu::Pressed(PC, EKeys::BackSpace, EKeys::Gamepad_FaceButton_Right, EKeys::Q);

	switch (Page)
	{
	case EPestiMenuPage::Title:
	case EPestiMenuPage::Pause:
	case EPestiMenuPage::Confirm:
	case EPestiMenuPage::Tests:
	{
		if (bUp || (Page == EPestiMenuPage::Confirm && bLeft))
		{
			Move(-1);
		}
		else if (bDown || (Page == EPestiMenuPage::Confirm && bRight))
		{
			Move(1);
		}
		else if (bOk)
		{
			const TArray<FPestiMenuRow> List = Rows();
			if (List.IsValidIndex(Choice) && List[Choice].bEnabled)
			{
				Do(World, PC, List[Choice].Action);
			}
		}
		else if (bBack)
		{
			if (Page == EPestiMenuPage::Confirm)
			{
				Do(World, PC, EPestiMenuAction::No);
			}
			else if (Page == EPestiMenuPage::Tests)
			{
				Do(World, PC, EPestiMenuAction::Back);
			}
			else if (Page == EPestiMenuPage::Pause)
			{
				Close(World);
			}
		}
		break;
	}
	case EPestiMenuPage::Settings:
		if (bUp)
		{
			Move(-1);
		}
		else if (bDown)
		{
			Move(1);
		}
		else if (bLeft)
		{
			ChangeSetting(World, Choice, -1);
		}
		else if (bRight || bOk)
		{
			ChangeSetting(World, Choice, 1);
		}
		else if (bBack)
		{
			Page = Base;
			Choice = BaseChoice;
		}
		break;
	default:
		// The controls and the credits: any of the usual keys goes back.
		if (bOk || bBack)
		{
			Page = Base;
			Choice = BaseChoice;
		}
		break;
	}
}
