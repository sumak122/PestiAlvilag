// Fill out your copyright notice in the Description page of Project Settings.

#include "FoodMenus.h"

#include "Brushes/SlateColorBrush.h"
#include "Cafe.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "FoodVendor.h"
#include "GameFramework/PlayerController.h"
#include "ImageUtils.h"
#include "KeletiStation.h"
#include "Misc/Paths.h"
#include "PestiAlvilagGameModeBase.h"
#include "PestiPerson.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

namespace Food
{
	const TArray<FDish>& Dishes()
	{
		static const TArray<FDish> All = {
			// The café.
			{ TEXT("feketekave"), TEXT("Feketek\u00e1v\u00e9"), TEXT("Black coffee"),
				TEXT("A small cup of strong coffee, with a glass of water and two lumps of sugar, as every Pest caf\u00e9 serves it."), 32 },
			{ TEXT("tejeskave"), TEXT("Tejesk\u00e1v\u00e9"), TEXT("White coffee"),
				TEXT("Coffee with hot milk and a cap of whipped milk. Half the city has it for breakfast."), 40 },
			{ TEXT("vajas_kifli"), TEXT("Vajas kifli"), TEXT("Butter crescent"),
				TEXT("A crescent roll fresh from the baker, still warm, with a curl of butter."), 10 },
			{ TEXT("pogacsa"), TEXT("Pog\u00e1csa"), TEXT("Savoury scones"),
				TEXT("Little scones with crackling and a cracked top, four to a plate. Good with wine, better with coffee."), 24 },
			{ TEXT("almas_retes"), TEXT("Alm\u00e1s r\u00e9tes"), TEXT("Apple strudel"),
				TEXT("Paper-thin pastry rolled round apples and cinnamon, dusted with sugar."), 36 },
			{ TEXT("dobos_torta"), TEXT("Dobos torta"), TEXT("Dobos cake"),
				TEXT("Six thin sponge layers with chocolate cream between and a hard caramel top. J\u00f3zsef Dobos's own Pest invention."), 50 },
			// The station restaurant.
			{ TEXT("gulyasleves"), TEXT("Guly\u00e1sleves"), TEXT("Goulash soup"),
				TEXT("Beef, potatoes and paprika simmered for hours: the herdsmen's soup that the whole world knows."), 80 },
			{ TEXT("halaszle"), TEXT("Hal\u00e1szl\u00e9"), TEXT("Fisherman's soup"),
				TEXT("Fiery red carp soup with hot paprika, the way they cook it by the Tisza."), 120 },
			{ TEXT("paprikas_csirke"), TEXT("Paprik\u00e1s csirke"), TEXT("Chicken paprikash"),
				TEXT("Chicken in a paprika and sour cream sauce, with a heap of nokedli dumplings."), 140 },
			{ TEXT("rantott_szelet"), TEXT("R\u00e1ntott szelet"), TEXT("Breaded cutlet"),
				TEXT("A pork cutlet fried golden in breadcrumbs, with parsley potatoes and a wedge of lemon."), 160 },
			{ TEXT("turos_csusza"), TEXT("T\u00far\u00f3s csusza"), TEXT("Noodles with curd cheese"),
				TEXT("Broad noodles with curd cheese, sour cream and crisp crackling on top."), 70 },
			{ TEXT("palacsinta"), TEXT("Palacsinta"), TEXT("Pancakes"),
				TEXT("Two thin pancakes rolled round apricot jam and dusted with sugar."), 50 },
			{ TEXT("sor"), TEXT("S\u00f6r"), TEXT("Beer"),
				TEXT("Half a litre of Dreher from K\u0151b\u00e1nya, cold from the cellar."), 36 },
			// Street food.
			{ TEXT("gesztenye"), TEXT("S\u00fclt gesztenye"), TEXT("Roast chestnuts"),
				TEXT("Hot from the iron pan, in a twist of newspaper. They warm your hands as much as your stomach."), 20 },
			{ TEXT("sult_tok"), TEXT("S\u00fclt t\u00f6k"), TEXT("Baked pumpkin"),
				TEXT("Slices of sweet pumpkin baked soft in the oven. Autumn on a street corner."), 16 },
			{ TEXT("virsli"), TEXT("Virsli"), TEXT("Hot sausages"),
				TEXT("A pair of steaming frankfurters with mustard and a crusty roll."), 40 },
			{ TEXT("debreceni"), TEXT("Debreceni"), TEXT("Debrecen sausage"),
				TEXT("A spicy paprika sausage from the grill, with bread and a fresh green pepper."), 60 },
			{ TEXT("perec"), TEXT("Perec"), TEXT("Pretzel"),
				TEXT("A salted pretzel from the basket, crisp outside and soft inside."), 6 },
		};
		return All;
	}

	const FMenu& Menu(EFoodMenu Which)
	{
		static const FMenu Menus[] = {
			{ TEXT("K\u00e1v\u00e9h\u00e1z"), TEXT("Caf\u00e9"), TEXT("\u201eParancsoljon, uram!\u201d  (At your service, sir!)"), { 0, 1, 2, 3, 4, 5 } },
			{ TEXT("Keleti \u00c9tterem"), TEXT("Keleti station restaurant"), TEXT("\u201eTess\u00e9k helyet foglalni!\u201d  (Please, take a seat!)"), { 6, 7, 8, 9, 10, 11, 12 } },
			{ TEXT("Gesztenyes\u00fct\u0151"), TEXT("Chestnut roaster"), TEXT("\u201eForr\u00f3 gesztenye! Forr\u00f3!\u201d  (Hot chestnuts! Hot!)"), { 13, 14 } },
			{ TEXT("Virslis"), TEXT("Sausage cart"), TEXT("\u201eFriss virsli, forr\u00f3 virsli!\u201d  (Fresh sausages, hot sausages!)"), { 15, 16, 17 } },
			{ TEXT("Perec\u00e1rus"), TEXT("Pretzel seller"), TEXT("\u201ePerecet tess\u00e9k!\u201d  (Pretzels, here!)"), { 17, 3, 2 } },
		};
		return Menus[FMath::Clamp(static_cast<int32>(Which), 0, static_cast<int32>(EFoodMenu::Count) - 1)];
	}
}

namespace FoodMenus
{
	constexpr int32 HintLine = 31;
	constexpr int32 SpeechLine = 6;
	// How close to the café waiter you must be to call him over.
	constexpr float WaiterRadius = 250.f;
	constexpr float RestaurantRadius = 450.f;
	constexpr float VendorRadius = 280.f;
	// Where to order at the station restaurant (in Keleti's frame, on its terrace).
	const FVector2D RestaurantOrderAt(-350.0, -3590.0);

	const FLinearColor Dim(0.f, 0.f, 0.f, 0.55f);
	const FLinearColor Panel(0.055f, 0.045f, 0.035f, 0.95f);
	const FLinearColor Row(0.11f, 0.09f, 0.07f, 1.f);
	const FLinearColor Gold(0.86f, 0.64f, 0.24f, 1.f);
	const FLinearColor Cream(0.96f, 0.91f, 0.80f, 1.f);
	const FLinearColor Grey(0.62f, 0.58f, 0.50f, 1.f);
	const FLinearColor Ink(0.08f, 0.06f, 0.04f, 1.f);

	const FSlateBrush* White()
	{
		static const FSlateColorBrush Brush(FLinearColor::White);
		return &Brush;
	}
}

AFoodMenus::AFoodMenus()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AFoodMenus::BeginPlay()
{
	Super::BeginPlay();
	LoadPictures();
}

void AFoodMenus::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (MenuWidget.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(MenuWidget.ToSharedRef());
	}
	MenuWidget.Reset();
	Super::EndPlay(EndPlayReason);
}

void AFoodMenus::LoadPictures()
{
	const TArray<Food::FDish>& All = Food::Dishes();
	Pictures.SetNum(All.Num());
	PictureBrushes.SetNum(All.Num());
	for (int32 i = 0; i < All.Num(); ++i)
	{
		const FString Path = FPaths::Combine(FPaths::ProjectDir(), TEXT("Data/Food"), FString(All[i].Id) + TEXT(".png"));
		Pictures[i] = FImageUtils::ImportFileAsTexture2D(Path);
		FSlateBrush& Brush = PictureBrushes[i];
		Brush.DrawAs = ESlateBrushDrawType::Image;
		Brush.ImageSize = FVector2D(256.0, 256.0);
		if (Pictures[i])
		{
			Brush.SetResourceObject(Pictures[i]);
		}
		else
		{
			// No picture: a dark plain square instead.
			Brush = *FoodMenus::White();
			Brush.TintColor = FSlateColor(FoodMenus::Row);
		}
	}
}

void AFoodMenus::GatherOrderPoints(TArray<FOrderPoint>& Out) const
{
	for (TActorIterator<ACafeWaiter> It(GetWorld()); It; ++It)
	{
		Out.Add({ It->GetActorLocation(), FoodMenus::WaiterRadius, EFoodMenu::Cafe, *It });
	}
	if (TActorIterator<AKeletiStation>(GetWorld()))
	{
		Out.Add({ FVector(AKeletiStation::ToCity(FoodMenus::RestaurantOrderAt), 0.0), FoodMenus::RestaurantRadius, EFoodMenu::StationRestaurant });
	}
	for (TActorIterator<AFoodVendor> It(GetWorld()); It; ++It)
	{
		Out.Add({ It->GetOrderPoint(), FoodMenus::VendorRadius, It->GetMenu() });
	}
}

int32 AFoodMenus::DishAt(int32 RowIndex) const
{
	const TArray<int32>& Rows = Food::Menu(OpenMenu).Dishes;
	return Rows.IsValidIndex(RowIndex) ? Rows[RowIndex] : 0;
}

void AFoodMenus::Say(const FString& Text, float Seconds) const
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(FoodMenus::SpeechLine, Seconds, FColor(255, 210, 60), Text);
	}
}

void AFoodMenus::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	APestiPerson* Player = PC ? Cast<APestiPerson>(PC->GetPawn()) : nullptr;

	// The menu is open: choose, order or leave.
	if (MenuWidget.IsValid())
	{
		if (!Player)
		{
			Close();
			return;
		}
		const int32 Count = Food::Menu(OpenMenu).Dishes.Num();
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
			Order();
		}
		else if (PC->WasInputKeyJustPressed(EKeys::Q) || PC->WasInputKeyJustPressed(EKeys::BackSpace) || PC->WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Right))
		{
			Close();
		}
		return;
	}

	// On foot by a place that sells food.
	if (!Player || Player->IsCarrying() || Player->IsRiding() || Player->IsHidden())
	{
		return;
	}
	TArray<FOrderPoint> Points;
	GatherOrderPoints(Points);
	const FOrderPoint* Nearest = nullptr;
	double NearestDistance = TNumericLimits<double>::Max();
	for (const FOrderPoint& Point : Points)
	{
		const double Distance = FVector::Dist2D(Point.Location, Player->GetActorLocation());
		if (Distance < Point.Radius && Distance < NearestDistance)
		{
			Nearest = &Point;
			NearestDistance = Distance;
		}
	}
	if (!Nearest)
	{
		return;
	}
	const Food::FMenu& Menu = Food::Menu(Nearest->Menu);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(FoodMenus::HintLine, 0.f, FColor(255, 210, 60),
			FString::Printf(TEXT("E: %s  (%s, %s)"),
				Nearest->Waiter ? TEXT("call the waiter for the menu") : TEXT("look at the menu"), Menu.Title, Menu.English));
	}
	if (PC->WasInputKeyJustPressed(EKeys::E))
	{
		Open(Nearest->Menu, Nearest->Waiter);
	}
}

void AFoodMenus::Open(EFoodMenu Which, ACafeWaiter* Waiter)
{
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC || !GEngine || !GEngine->GameViewport)
	{
		return;
	}
	OpenMenu = Which;
	Selected = 0;
	Attending = Waiter;
	if (Waiter)
	{
		Waiter->Attend(PC->GetPawn());
	}
	MenuWidget = BuildMenuWidget();
	GEngine->GameViewport->AddViewportWidgetContent(MenuWidget.ToSharedRef(), 10);
	PC->SetIgnoreMoveInput(true);
	PC->SetIgnoreLookInput(true);
	Say(Food::Menu(Which).Greeting, 4.f);
}

void AFoodMenus::Close()
{
	if (MenuWidget.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(MenuWidget.ToSharedRef());
	}
	MenuWidget.Reset();
	OpenMenu = EFoodMenu::Count;
	if (ACafeWaiter* Waiter = Attending.Get())
	{
		Waiter->Attend(nullptr);
	}
	Attending.Reset();
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		PC->SetIgnoreMoveInput(false);
		PC->SetIgnoreLookInput(false);
	}
}

void AFoodMenus::Order()
{
	APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this);
	const Food::FDish& Dish = Food::Dishes()[DishAt(Selected)];
	if (!Game || Game->GetMoney() < Dish.Price)
	{
		Say(FString::Printf(TEXT("\u201eSajnos ez t\u00f6bbe ker\u00fcl.\u201d  (I'm afraid that costs more.)   %s is %s"),
			Dish.Name, *APestiAlvilagGameModeBase::FormatMoney(Dish.Price)));
		return;
	}
	Game->AddMoney(-Dish.Price);
	const bool bNew = Game->TryDish(FName(Dish.Id));
	Close();
	FString Text = FString::Printf(TEXT("\u201eJ\u00f3 \u00e9tv\u00e1gyat!\u201d  (Enjoy your meal!)   %s, %s"),
		Dish.Name, *APestiAlvilagGameModeBase::FormatMoney(Dish.Price));
	if (bNew)
	{
		Text += FString::Printf(TEXT("   New dish tried: %d of %d"), Game->GetDishesTried(), Food::Dishes().Num());
	}
	Say(Text);
}

TSharedRef<SWidget> AFoodMenus::BuildMenuWidget()
{
	using namespace FoodMenus;
	const Food::FMenu& Menu = Food::Menu(OpenMenu);
	const APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(this);
	const FSlateFontInfo TitleFont = FCoreStyle::GetDefaultFontStyle("Bold", 30);
	const FSlateFontInfo NameFont = FCoreStyle::GetDefaultFontStyle("Bold", 18);
	const FSlateFontInfo SmallFont = FCoreStyle::GetDefaultFontStyle("Regular", 12);
	const FSlateFontInfo TextFont = FCoreStyle::GetDefaultFontStyle("Regular", 14);
	const FSlateFontInfo BigFont = FCoreStyle::GetDefaultFontStyle("Bold", 24);

	// The dishes, one row each; the chosen row turns gold.
	TSharedRef<SVerticalBox> List = SNew(SVerticalBox);
	for (int32 Index = 0; Index < Menu.Dishes.Num(); ++Index)
	{
		const Food::FDish& Dish = Food::Dishes()[Menu.Dishes[Index]];
		const bool bTried = Game && Game->HasTriedDish(FName(Dish.Id));
		auto RowColor = [this, Index](const FLinearColor& On, const FLinearColor& Off)
		{
			return [this, Index, On, Off]() { return FSlateColor(Index == Selected ? On : Off); };
		};
		List->AddSlot()
		.AutoHeight()
		.Padding(0.f, 3.f)
		[
			SNew(SBorder)
			.BorderImage(White())
			.BorderBackgroundColor_Lambda(RowColor(Gold, FoodMenus::Row))
			.Padding(FMargin(14.f, 8.f))
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
						.Text(FText::FromString(bTried ? FString(Dish.Name) + TEXT("   (tried)") : FString(Dish.Name)))
						.Font(NameFont)
						.ColorAndOpacity_Lambda(RowColor(Ink, Cream))
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(STextBlock)
						.Text(FText::FromString(Dish.English))
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
					.Text(FText::FromString(APestiAlvilagGameModeBase::FormatMoney(Dish.Price)))
					.Font(NameFont)
					.ColorAndOpacity_Lambda(RowColor(Ink, Gold))
				]
			]
		];
	}

	auto Chosen = [this]() -> const Food::FDish& { return Food::Dishes()[DishAt(Selected)]; };

	return SNew(SBorder)
		.BorderImage(White())
		.BorderBackgroundColor(Dim)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SBox)
			.WidthOverride(1000.f)
			[
				SNew(SBorder)
				.BorderImage(White())
				.BorderBackgroundColor(Panel)
				.Padding(28.f)
				[
					SNew(SVerticalBox)
					// The heading: the place, and the money in hand.
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
								SNew(STextBlock).Text(FText::FromString(Menu.Title)).Font(TitleFont).ColorAndOpacity(Gold)
							]
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(STextBlock).Text(FText::FromString(Menu.English)).Font(TextFont).ColorAndOpacity(Grey)
							]
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign(VAlign_Bottom)
						[
							SNew(STextBlock)
							.Text_Lambda([this]()
							{
								const APestiAlvilagGameModeBase* Mode = APestiAlvilagGameModeBase::Get(this);
								return FText::FromString(FString::Printf(TEXT("P\u00e9nz (money): %s"),
									*APestiAlvilagGameModeBase::FormatMoney(Mode ? Mode->GetMoney() : 0)));
							})
							.Font(TextFont)
							.ColorAndOpacity(Cream)
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
					// The dishes on the left, the chosen one large on the right.
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.FillWidth(1.f)
						.Padding(0.f, 0.f, 24.f, 0.f)
						[
							List
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SBox)
							.WidthOverride(380.f)
							[
								SNew(SVerticalBox)
								+ SVerticalBox::Slot()
								.AutoHeight()
								.HAlign(HAlign_Center)
								[
									SNew(SBorder)
									.BorderImage(White())
									.BorderBackgroundColor(Gold)
									.Padding(4.f)
									[
										SNew(SBox)
										.WidthOverride(340.f)
										.HeightOverride(340.f)
										[
											SNew(SImage).Image_Lambda([this]() -> const FSlateBrush* { return &PictureBrushes[DishAt(Selected)]; })
										]
									]
								]
								+ SVerticalBox::Slot()
								.AutoHeight()
								.Padding(0.f, 12.f, 0.f, 0.f)
								[
									SNew(STextBlock)
									.Text_Lambda([Chosen]() { return FText::FromString(Chosen().Name); })
									.Font(BigFont)
									.ColorAndOpacity(Cream)
								]
								+ SVerticalBox::Slot()
								.AutoHeight()
								[
									SNew(STextBlock)
									.Text_Lambda([Chosen]() { return FText::FromString(Chosen().English); })
									.Font(TextFont)
									.ColorAndOpacity(Grey)
								]
								+ SVerticalBox::Slot()
								.AutoHeight()
								.Padding(0.f, 8.f, 0.f, 0.f)
								[
									SNew(STextBlock)
									.Text_Lambda([Chosen]() { return FText::FromString(Chosen().About); })
									.Font(TextFont)
									.ColorAndOpacity(Cream)
									.AutoWrapText(true)
								]
								+ SVerticalBox::Slot()
								.AutoHeight()
								.Padding(0.f, 10.f, 0.f, 0.f)
								[
									SNew(STextBlock)
									.Text_Lambda([Chosen]() { return FText::FromString(APestiAlvilagGameModeBase::FormatMoney(Chosen().Price)); })
									.Font(BigFont)
									.ColorAndOpacity(Gold)
								]
							]
						]
					]
					// The keys.
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.f, 18.f, 0.f, 0.f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("W / S: choose      E: order      Q: leave")))
						.Font(TextFont)
						.ColorAndOpacity(Grey)
					]
				]
			]
		];
}
