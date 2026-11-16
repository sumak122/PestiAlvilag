// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

class UWorld;

/**
 * The spawn menu for testing (Esc > PRÓBATEREP > IDEHOZÁS, or F6): a page
 * of rows, each a group with one pick shown (A / D picks another, Enter does
 * it): a vehicle to stand next to the player, traffic coming up the nearest
 * street (cars, police, the milk, coal and beer carts, a rider), a stray
 * animal, people (a fight, a tough, a witness, a crowd, constables), the
 * clock, the wanted stars, and a jump to the main places of the map.
 * FPestiMenu keeps the picks and PestiHud paints the page.
 */
namespace TestSpawn
{
	enum class EGroup : uint8 { Vehicle, Traffic, Animal, People, Clock, Wanted, Jump, Count };

	constexpr int32 NumGroups = static_cast<int32>(EGroup::Count);

	/** How many picks a group has. */
	int32 NumPicks(EGroup Group);
	/** A group's name (in capitals) and a short line on what Enter does with it, in both languages. */
	void GroupName(EGroup Group, FString& Hu, FString& En, FString& NoteHu, FString& NoteEn);
	/** A pick's name, in both languages. */
	void PickName(EGroup Group, int32 Pick, FString& Hu, FString& En);
	/** Only on foot (not driving or riding). */
	bool NeedsFoot(EGroup Group);
	/** Brings in or does the pick; the menu is closed by then. Says on the screen what it did, or why it could not. */
	void DoPick(UWorld* World, EGroup Group, int32 Pick);
}
