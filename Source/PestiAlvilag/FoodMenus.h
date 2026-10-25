// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Styling/SlateBrush.h"
#include "FoodMenus.generated.h"

class SWidget;
class UTexture2D;

/** The places that sell food, each with its own menu. */
enum class EFoodMenu : uint8
{
	Cafe,
	StationRestaurant,
	Chestnuts,
	Sausages,
	Pretzels,
	Count
};

namespace Food
{
	struct FDish
	{
		/** Also the picture's file name, Data/Food/<Id>.png. */
		const TCHAR* Id;
		const TCHAR* Name;
		const TCHAR* English;
		const TCHAR* About;
		/** Fillér. */
		int32 Price;
	};

	struct FMenu
	{
		const TCHAR* Title;
		const TCHAR* English;
		/** What the waiter or the seller says when you come to order. */
		const TCHAR* Greeting;
		TArray<int32> Dishes;
	};

	/** Every dish there is, in one list. */
	const TArray<FDish>& Dishes();
	const FMenu& Menu(EFoodMenu Which);
}

/**
 * Buying food, in the manner of the Yakuza games: on foot by a place that
 * sells food (the café by Bözsi néni's door, the Keleti station restaurant,
 * a street seller's cart), E opens its menu: the dishes on the left with
 * their prices, the chosen one's picture and a few words about it on the
 * right. W/S or the arrow keys choose, E or Enter orders, Q leaves. The
 * game remembers which dishes you have tried. The game mode spawns one.
 */
UCLASS()
class PESTIALVILAG_API AFoodMenus : public AActor
{
	GENERATED_BODY()

public:
	AFoodMenus();

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	struct FOrderPoint
	{
		FVector Location;
		float Radius;
		EFoodMenu Menu;
	};

	/** The places to order from: the café, the restaurant and every seller. */
	void GatherOrderPoints(TArray<FOrderPoint>& Out) const;
	void LoadPictures();
	void Open(EFoodMenu Which);
	void Close();
	void Order();
	void Say(const FString& Text, float Seconds = 6.f) const;
	TSharedRef<SWidget> BuildMenuWidget();
	/** The dish (an index into Food::Dishes) at a row of the open menu. */
	int32 DishAt(int32 RowIndex) const;

	/** The pictures, one per dish (null where the file is missing). */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UTexture2D>> Pictures;

	TArray<FSlateBrush> PictureBrushes;
	TSharedPtr<SWidget> MenuWidget;
	EFoodMenu OpenMenu = EFoodMenu::Count;
	int32 Selected = 0;
};
