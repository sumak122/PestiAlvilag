// Fill out your copyright notice in the Description page of Project Settings.

#include "FoodMenus.h"

#include "Brushes/SlateColorBrush.h"
#include "Cafe.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Texture2D.h"
#include "Eateries.h"
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
#include "PestiHud.h"

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
			// The places to eat (AEateries).
			{ TEXT("rigo_jancsi"), TEXT("Rig\u00f3 Jancsi"), TEXT("Rig\u00f3 Jancsi cake"),
				TEXT("A cube of chocolate sponge and chocolate cream, named after the gypsy fiddler who ran off with a princess."), 60 },
			{ TEXT("eszterhazy"), TEXT("Eszterh\u00e1zy torta"), TEXT("Eszterh\u00e1zy cake"),
				TEXT("Walnut layers with vanilla cream and white fondant, feathered with chocolate."), 60 },
			{ TEXT("kremes"), TEXT("Kr\u00e9mes"), TEXT("Cream slice"),
				TEXT("Thick vanilla custard between two crisp sheets of puff pastry, buried in sugar."), 40 },
			{ TEXT("gesztenyepure"), TEXT("Gesztenyep\u00fcr\u00e9"), TEXT("Chestnut pur\u00e9e"),
				TEXT("Sweet chestnut pressed into threads, with rum, under a cap of whipped cream."), 50 },
			{ TEXT("forro_csoki"), TEXT("Forr\u00f3 csokol\u00e1d\u00e9"), TEXT("Hot chocolate"),
				TEXT("Thick hot chocolate with whipped cream, a winter treat for the gentry."), 44 },
			{ TEXT("babfozelek"), TEXT("Babf\u0151zel\u00e9k"), TEXT("Bean stew"),
				TEXT("Thick bean stew with a roux and a few slices of sausage on top. Fills a worker till evening."), 30 },
			{ TEXT("krumplifozelek"), TEXT("Krumplif\u0151zel\u00e9k"), TEXT("Potato stew"),
				TEXT("Potatoes in a sour cream and paprika sauce. Cheap, warm and honest."), 24 },
			{ TEXT("paradicsomleves"), TEXT("Paradicsomleves"), TEXT("Tomato soup"),
				TEXT("Sweet tomato soup with little dumplings floating in it."), 20 },
			{ TEXT("rakott_krumpli"), TEXT("Rakott krumpli"), TEXT("Layered potatoes"),
				TEXT("Potatoes, eggs and sausage in layers, baked under sour cream."), 40 },
			{ TEXT("lekvaros_teszta"), TEXT("Lekv\u00e1ros t\u00e9szta"), TEXT("Noodles with jam"),
				TEXT("Square noodles with plum jam and sugar: a sweet main course for small purses."), 20 },
			{ TEXT("tej"), TEXT("Poh\u00e1r tej"), TEXT("Glass of milk"),
				TEXT("A cold glass of fresh milk from the Central Dairy, with a crescent roll."), 12 },
			{ TEXT("kakao"), TEXT("Kaka\u00f3"), TEXT("Cocoa"),
				TEXT("A mug of hot cocoa made with milk, the favourite of every schoolboy."), 16 },
			{ TEXT("tejbegriz"), TEXT("Tejbegr\u00edz"), TEXT("Semolina pudding"),
				TEXT("Semolina cooked in milk, with grated chocolate and a spoon of raspberry syrup."), 20 },
			{ TEXT("porkolt"), TEXT("P\u00f6rk\u00f6lt"), TEXT("Beef stew"),
				TEXT("Beef stewed with onions and paprika until it falls apart, with nokedli and a pickle."), 110 },
			{ TEXT("toltott_kaposzta"), TEXT("T\u00f6lt\u00f6tt k\u00e1poszta"), TEXT("Stuffed cabbage"),
				TEXT("Cabbage leaves rolled round pork and rice, cooked in sauerkraut, with sour cream."), 100 },
			{ TEXT("froccs"), TEXT("Fr\u00f6ccs"), TEXT("Wine spritzer"),
				TEXT("White wine and soda water from the siphon, half and half. The drink of Pest."), 20 },
			{ TEXT("bor"), TEXT("Poh\u00e1r bor"), TEXT("Glass of wine"),
				TEXT("A glass of red from the cask, from the hills of Eger."), 30 },
			{ TEXT("zsiros_kenyer"), TEXT("Zs\u00edros keny\u00e9r"), TEXT("Dripping on bread"),
				TEXT("A slice of bread spread with lard, paprika and red onion rings. Goes with wine."), 10 },
			{ TEXT("sonkas_zsemle"), TEXT("Sonk\u00e1s zsemle"), TEXT("Ham roll"),
				TEXT("A crisp roll with ham and a leaf of lettuce, wrapped for the train."), 30 },
			{ TEXT("korozott"), TEXT("K\u00f6r\u00f6z\u00f6tt"), TEXT("Paprika cheese spread"),
				TEXT("Ewe's curd cheese with paprika, caraway and onion on rye bread."), 20 },
			{ TEXT("kocsonya"), TEXT("Kocsonya"), TEXT("Pork aspic"),
				TEXT("Cold jellied pork with egg and carrot, a winter favourite with vinegar and onion."), 40 },
			{ TEXT("husleves"), TEXT("H\u00fasleves"), TEXT("Chicken soup"),
				TEXT("Golden chicken broth with thin noodles and carrots, as on every Friday night."), 50 },
			{ TEXT("solet"), TEXT("S\u00f3let"), TEXT("Cholent"),
				TEXT("Beans and barley baked slowly overnight with goose and a brown egg."), 80 },
			{ TEXT("flodni"), TEXT("Fl\u00f3dni"), TEXT("Fl\u00f3dni"),
				TEXT("Layers of poppy seed, walnut, apple and plum jam between thin pastry."), 40 },
			{ TEXT("barchesz"), TEXT("Barchesz"), TEXT("Plaited bread"),
				TEXT("A shiny plaited white loaf, sweet and soft."), 16 },
			{ TEXT("eroleves"), TEXT("Er\u0151leves"), TEXT("Consomm\u00e9"),
				TEXT("Clear beef broth in a cup, with a raw egg yolk and marrow toast."), 120 },
			{ TEXT("fogas"), TEXT("Balatoni fogas"), TEXT("Pike-perch from Balaton"),
				TEXT("The pride of Hungarian kitchens: pike-perch fried in butter, with lemon and parsley."), 380 },
			{ TEXT("belszin"), TEXT("B\u00e9lsz\u00edn"), TEXT("Beef tenderloin"),
				TEXT("A thick tenderloin steak with roast potatoes. Only the rich eat like this."), 480 },
			{ TEXT("pezsgo"), TEXT("T\u00f6rley pezsg\u0151"), TEXT("T\u00f6rley champagne"),
				TEXT("Two glasses of T\u00f6rley from Budafok, the Hungarian champagne."), 300 },
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
			{ TEXT("Hauer cukr\u00e1szda"), TEXT("Hauer pastry shop"), TEXT("\u201eMivel szolg\u00e1lhatok?\u201d  (What may I bring you?)"), { 5, 18, 19, 20, 21, 22, 0 } },
			{ TEXT("Kif\u0151zde"), TEXT("Cook-shop"), TEXT("\u201eMi legyen, fiam?\u201d  (What'll it be, son?)"), { 25, 23, 24, 26, 27, 6 } },
			{ TEXT("K\u00f6zponti Tejcsarnok"), TEXT("Milk bar"), TEXT("\u201eTess\u00e9k, friss a tej!\u201d  (Here, the milk is fresh!)"), { 28, 29, 2, 30, 1 } },
			{ TEXT("Fuvaros vend\u00e9gl\u0151"), TEXT("Carters' inn"), TEXT("\u201eNa, mit hozzak?\u201d  (Well, what shall I bring?)"), { 6, 31, 32, 9, 12, 33 } },
			{ TEXT("Boroz\u00f3"), TEXT("Wine cellar"), TEXT("\u201eEgy fr\u00f6ccs\u00f6t?\u201d  (A spritzer?)"), { 33, 34, 3, 35 } },
			{ TEXT("Kispipa vend\u00e9gl\u0151"), TEXT("Kispipa inn"), TEXT("\u201eTess\u00e9k helyet foglalni, a zongorista mindj\u00e1rt kezd!\u201d  (Do sit down, the pianist starts soon!)"), { 7, 8, 32, 10, 11, 34, 12 } },
			{ TEXT("\u00c1ll\u00f3b\u00fcf\u00e9"), TEXT("Standing buffet"), TEXT("\u201eMit adhatok? Indul a vonat?\u201d  (What can I get you? Train leaving?)"), { 36, 37, 38, 15, 12, 0 } },
			{ TEXT("K\u00f3ser \u00e9tkezde"), TEXT("Kosher eating house"), TEXT("\u201e\u00dclj\u00f6n le, j\u00f3 ember!\u201d  (Sit down, good man!)"), { 39, 40, 41, 42 } },
			{ TEXT("Nemzeti \u00e9tterem"), TEXT("Hotel Nemzeti restaurant"), TEXT("\u201eM\u00e9lt\u00f3ztass\u00e9k, uram!\u201d  (If you please, sir!)"), { 43, 44, 45, 5, 46, 0 } },
		};
		return Menus[FMath::Clamp(static_cast<int32>(Which), 0, static_cast<int32>(EFoodMenu::Count) - 1)];
	}
}

namespace FoodMenus
{
	constexpr int32 HintLine = 31;
	constexpr int32 SpeechLine = 6;
	// A good meal puts some strength back (health, of 100).
	constexpr float Heal = 35.f;
	// How close to the café waiter you must be to call him over.
	constexpr float WaiterRadius = 250.f;
	constexpr float RestaurantRadius = 450.f;
	constexpr float VendorRadius = 280.f;
	constexpr float CounterRadius = 170.f;
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
		Out.Add({ It->GetActorLocation(), FoodMenus::WaiterRadius, static_cast<EFoodMenu>(It->GetMenu()), *It });
	}
	// The counters of the places to eat.
	for (TActorIterator<AEateries> It(GetWorld()); It; ++It)
	{
		TArray<FVector> Counters;
		TArray<uint8> Menus;
		It->GetCounters(Counters, Menus);
		for (int32 i = 0; i < Counters.Num(); ++i)
		{
			Out.Add({ Counters[i], FoodMenus::CounterRadius, static_cast<EFoodMenu>(Menus[i]) });
		}
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
		APestiHUD::Show(this, FoodMenus::SpeechLine, Seconds, FColor(255, 210, 60), Text);
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
		APestiHUD::Show(this, FoodMenus::HintLine, 0.f, FColor(255, 210, 60),
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
	// His street name may take something off.
	const int32 Price = APestiAlvilagGameModeBase::PriceFor(this, Dish.Price);
	if (!Game || Game->GetMoney() < Price)
	{
		Say(FString::Printf(TEXT("\u201eSajnos ez t\u00f6bbe ker\u00fcl.\u201d  (I'm afraid that costs more.)   %s is %s"),
			Dish.Name, *APestiAlvilagGameModeBase::FormatMoney(Price)));
		return;
	}
	Game->AddMoney(-Price);
	Game->HealPlayer(FoodMenus::Heal);
	const bool bNew = Game->TryDish(FName(Dish.Id));
	Close();
	FString Text = FString::Printf(TEXT("\u201eJ\u00f3 \u00e9tv\u00e1gyat!\u201d  (Enjoy your meal!)   %s, %s"),
		Dish.Name, *APestiAlvilagGameModeBase::FormatMoney(Price));
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
					.Text(FText::FromString(APestiAlvilagGameModeBase::FormatMoney(APestiAlvilagGameModeBase::PriceFor(this, Dish.Price))))
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
									.Text_Lambda([this, Chosen]() { return FText::FromString(APestiAlvilagGameModeBase::FormatMoney(APestiAlvilagGameModeBase::PriceFor(this, Chosen().Price))); })
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
