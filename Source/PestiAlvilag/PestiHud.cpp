// Fill out your copyright notice in the Description page of Project Settings.

#include "PestiHud.h"

#include "Brushes/SlateColorBrush.h"
#include "DayNight.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Fonts/CompositeFont.h"
#include "Fonts/FontMeasure.h"
#include "Fonts/SlateFontInfo.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "LighterModel.h"
#include "Lighters.h"
#include "Misc/Paths.h"
#include "PestiAlvilagGameModeBase.h"
#include "PestiMenu.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBackgroundBlur.h"
#include "Widgets/SLeafWidget.h"

namespace PestiHud
{
	// The look: ink-dark plaques, gold and brass lines, cream lettering.
	const FLinearColor Ink = FLinearColor(FColor(14, 12, 9, 189));
	const FLinearColor Solid = FLinearColor(FColor(21, 18, 13));
	const FLinearColor Gold = FLinearColor(FColor(214, 178, 94));
	const FLinearColor Dim = FLinearColor(FColor(214, 178, 94, 64));
	const FLinearColor Fan = FLinearColor(FColor(214, 178, 94, 128));
	const FLinearColor Brass = FLinearColor(FColor(156, 132, 80));
	const FLinearColor Cream = FLinearColor(FColor(244, 232, 204));
	const FLinearColor Grey = FLinearColor(FColor(201, 188, 156));
	const FLinearColor Red = FLinearColor(FColor(176, 52, 44));
	const FLinearColor RedHi = FLinearColor(FColor(224, 87, 63));
	const FLinearColor RedEmpty = FLinearColor(FColor(176, 52, 44, 56));
	const FLinearColor Velvet = FLinearColor(FColor(58, 15, 18));
	const FLinearColor VelvetDark = FLinearColor(FColor(28, 10, 11));
	const FLinearColor NotFoundMark = FLinearColor(FColor(214, 178, 94, 90));
	const FLinearColor CabinetShade = FLinearColor(0.f, 0.f, 0.f, 0.6f);

	// The panels are laid out for a screen 1080 units high; Slate scales them.
	constexpr float Margin = 36.f;
	constexpr float Top = 34.f;
	constexpr float Step = 7.f;
	constexpr float GoalHeight = 104.f;
	constexpr float GoalMinWidth = 420.f;
	constexpr float StatusWidth = 330.f;
	constexpr float StatusHeight = 150.f;
	constexpr int32 HealthBars = 10;
	constexpr int32 WantedDiamonds = 3;
	constexpr float SpeechWidth = 1180.f;
	constexpr float SpeechFromBottom = 150.f;
	constexpr float AnnounceY = 330.f;
	// The key sign for a move: this far down the screen (share of its height).
	constexpr float PromptDown = 0.68f;
	// A fist fight: the other man's plaque across the top; the breath bar
	// turns red below this share.
	constexpr float FoeWidth = 520.f;
	constexpr float LowBreath = 0.25f;

	// The lighter cabinet: six niches across, five down, the chosen one large
	// on the right, the rewards along the bottom.
	// A letter: handwriting in ink on cream paper left of the middle, the
	// English on a plaque to the right, the street dimmed behind.
	const FLinearColor LetterShade = FLinearColor(0.f, 0.f, 0.f, 0.45f);
	const FLinearColor Paper = FLinearColor(FColor(240, 229, 203));
	const FLinearColor PaperEdge = FLinearColor(FColor(196, 178, 140));
	const FLinearColor PaperShadow = FLinearColor(0.f, 0.f, 0.f, 0.35f);
	const FLinearColor LetterInk = FLinearColor(FColor(43, 42, 74));
	constexpr float PaperWidth = 620.f;
	constexpr float PaperPad = 50.f;
	constexpr float HandPx = 31.f;
	constexpr float HandLine = 43.f;
	constexpr float LetterGap = 60.f;
	constexpr float PlaqueWidth = 500.f;
	constexpr float PlaquePad = 32.f;
	constexpr float PlaquePx = 19.f;
	constexpr float PlaqueLine = 28.f;

	constexpr float CabinetWidth = 1580.f;
	constexpr float CabinetHeight = 935.f;
	constexpr int32 NicheColumns = 6;
	constexpr float NicheWidth = 142.f;
	constexpr float NicheHeight = 128.f;
	constexpr float NicheGap = 12.f;
	constexpr int32 RewardEvery = 10;
	// The pictures are 160 by 200.
	constexpr float PictureWidth = 160.f;
	constexpr float PictureHeight = 200.f;

	// Keys of the lines that are not notes.
	constexpr int32 SpeechKeyA = 6;
	constexpr int32 SpeechKeyB = 8;
	constexpr int32 AnnounceKey = 9;

	// A one-frame line (Seconds 0) still shows when the HUD paints this frame.
	constexpr double OneFrame = 0.0;

	// The menus: the city blurred and darkened behind them, a faint gold
	// glow on the chosen setting, the side panels of the pause menu only on
	// screens this wide, and act 1's seven missions.
	constexpr float MenuBlur = 6.f;
	const FLinearColor MenuShade = FLinearColor(FColor(8, 6, 4, 150));
	const FLinearColor MenuGlow = FLinearColor(FColor(214, 178, 94, 30));
	constexpr float MenuSidesFrom = 1700.f;
	constexpr int32 MenuActOneMissions = 7;

	/** A key along the bottom of a menu and what it does. */
	struct FMenuKey
	{
		const TCHAR* Key;
		const TCHAR* Hu;
		const TCHAR* En;
	};

	const FSlateBrush* White()
	{
		static const FSlateColorBrush Brush(FLinearColor::White);
		return &Brush;
	}

	/** "Hungarian  (English)" as its two halves; false when it isn't written so. */
	bool SplitLanguages(const FString& Text, FString& Hu, FString& En)
	{
		const int32 Open = Text.Find(TEXT("  ("), ESearchCase::CaseSensitive, ESearchDir::FromEnd);
		if (Open == INDEX_NONE || !Text.EndsWith(TEXT(")")))
		{
			return false;
		}
		Hu = Text.Left(Open).TrimEnd();
		En = Text.Mid(Open + 3, Text.Len() - Open - 4);
		return true;
	}

	TSharedPtr<const FCompositeFont> LoadFont(const TCHAR* File)
	{
		const FString Path = FPaths::Combine(FPaths::ProjectDir(), TEXT("Data/Fonts"), File);
		if (!FPaths::FileExists(Path))
		{
			return nullptr;
		}
		return MakeShared<FStandaloneCompositeFont>(NAME_None, Path, EFontHinting::Default, EFontLoadingPolicy::LazyLoad);
	}
}

/** Paints the whole HUD each frame from what its APestiHUD holds. */
class SPestiHud : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SPestiHud) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, APestiHUD* InOwner)
	{
		Owner = InOwner;
		SetVisibility(EVisibility::HitTestInvisible);
	}

	virtual FVector2D ComputeDesiredSize(float) const override
	{
		return FVector2D(1.0, 1.0);
	}

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& CullingRect,
		FSlateWindowElementList& Out, int32 LayerId, const FWidgetStyle& Style, bool bParentEnabled) const override
	{
		const APestiHUD* Hud = Owner.Get();
		if (!Hud)
		{
			return LayerId;
		}
		FPainter Paint{ Geometry, Out, LayerId, *Hud, Hud->DecoFont, Hud->BoldFont, Hud->TextFont };
		const float Wide = Geometry.GetLocalSize().X;
		const float High = Geometry.GetLocalSize().Y;
		const double Now = Hud->Now();

		// An open menu hides the rest of the HUD.
		if (Hud->Menu.IsOpen())
		{
			PaintMenu(Paint, *Hud, Wide, High);
			return LayerId + 4;
		}

		float NotesTop = PestiHud::Top;
		if (!Hud->GoalHu.IsEmpty())
		{
			PaintGoal(Paint, *Hud);
			NotesTop += PestiHud::GoalHeight + 18.f;
		}
		PaintNotes(Paint, *Hud, Now, NotesTop);
		PaintStatus(Paint, Wide);
		if (Hud->Fight.Until >= Now)
		{
			PaintFight(Paint, Hud->Fight, Wide);
		}
		if (Hud->Speech.Until >= Now)
		{
			PaintSpeech(Paint, Hud->Speech, Wide * 0.5f, High - PestiHud::SpeechFromBottom);
		}
		if (Hud->Big.Until >= Now)
		{
			PaintAnnounce(Paint, Hud->Big, Wide * 0.5f);
		}
		if (Hud->KeySign.Until >= Now)
		{
			PaintPrompt(Paint, Hud->KeySign, Wide * 0.5f, High * PestiHud::PromptDown);
		}
		if (const ALighters* Collection = Hud->Cabinet.Get())
		{
			const FPainter Over{ Geometry, Out, LayerId + 3, *Hud, Hud->DecoFont, Hud->BoldFont, Hud->TextFont };
			PaintCabinet(Over, *Collection, Wide, High);
			return LayerId + 6;
		}
		if (!Hud->LetterHu.IsEmpty())
		{
			const FPainter Over{ Geometry, Out, LayerId + 3, *Hud, Hud->DecoFont, Hud->BoldFont, Hud->TextFont };
			PaintLetter(Over, *Hud, Wide, High);
			return LayerId + 6;
		}
		return LayerId + 3;
	}

private:
	/** The drawing calls the panels are made of, on three layers. */
	struct FPainter
	{
		const FGeometry& Geometry;
		FSlateWindowElementList& Out;
		int32 Layer;
		const APestiHUD& Hud;
		TSharedPtr<const FCompositeFont> DecoFace;
		TSharedPtr<const FCompositeFont> BoldFace;
		TSharedPtr<const FCompositeFont> TextFace;

		void Box(float X, float Y, float W, float H, const FLinearColor& Color, int32 Up = 0) const
		{
			FSlateDrawElement::MakeBox(Out, Layer + Up,
				Geometry.ToPaintGeometry(FVector2f(W, H), FSlateLayoutTransform(FVector2f(X, Y))),
				PestiHud::White(), ESlateDrawEffect::None, Color);
		}

		void Lines(const TArray<FVector2f>& Points, const FLinearColor& Color, float Thickness, int32 Up = 1) const
		{
			FSlateDrawElement::MakeLines(Out, Layer + Up, Geometry.ToPaintGeometry(), Points,
				ESlateDrawEffect::None, Color, true, Thickness);
		}

		void Line(float X0, float Y0, float X1, float Y1, const FLinearColor& Color, float Thickness, int32 Up = 1) const
		{
			Lines({ FVector2f(X0, Y0), FVector2f(X1, Y1) }, Color, Thickness, Up);
		}

		/** A square stood on its corner, Size across its sides, centred on X, Y. */
		void Diamond(float X, float Y, float Size, const FLinearColor& Color, int32 Up = 1) const
		{
			const FGeometry Turned = Geometry.MakeChild(FVector2f(Size, Size),
				FSlateLayoutTransform(FVector2f(X - Size * 0.5f, Y - Size * 0.5f)),
				FSlateRenderTransform(FQuat2f(UE_PI * 0.25f)), FVector2f(0.5f, 0.5f));
			FSlateDrawElement::MakeBox(Out, Layer + Up, Turned.ToPaintGeometry(), PestiHud::White(), ESlateDrawEffect::None, Color);
		}

		FSlateFontInfo Font(const TSharedPtr<const FCompositeFont>& Face, float Px, float SpacingPx = 0.f) const
		{
			// Slate sizes fonts in points at 96 dpi: 3 points make 4 units.
			FSlateFontInfo Info = Face.IsValid()
				? FSlateFontInfo(Face, Px * 0.75f)
				: FCoreStyle::GetDefaultFontStyle("Regular", Px * 0.75f);
			Info.LetterSpacing = FMath::RoundToInt(SpacingPx / Px * 1000.f);
			return Info;
		}
		FSlateFontInfo Deco(float Px, float SpacingPx = 0.f) const { return Font(DecoFace, Px, SpacingPx); }
		FSlateFontInfo Bold(float Px, float SpacingPx = 0.f) const { return Font(BoldFace, Px, SpacingPx); }
		FSlateFontInfo Plain(float Px, float SpacingPx = 0.f) const { return Font(TextFace, Px, SpacingPx); }

		/** A rectangle's outline. */
		void Frame(float X, float Y, float W, float H, const FLinearColor& Color, float Thickness, int32 Up = 1) const
		{
			Lines({ FVector2f(X, Y), FVector2f(X + W, Y), FVector2f(X + W, Y + H), FVector2f(X, Y + H), FVector2f(X, Y) }, Color, Thickness, Up);
		}

		void Picture(const FSlateBrush* Brush, float X, float Y, float W, float H, float Opacity = 1.f, int32 Up = 1) const
		{
			if (Brush)
			{
				FSlateDrawElement::MakeBox(Out, Layer + Up,
					Geometry.ToPaintGeometry(FVector2f(W, H), FSlateLayoutTransform(FVector2f(X, Y))),
					Brush, ESlateDrawEffect::None, FLinearColor(1.f, 1.f, 1.f, Opacity));
			}
		}

		FVector2f Measure(const FString& Words, const FSlateFontInfo& Info) const
		{
			const auto Size = FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(Words, Info);
			return FVector2f(static_cast<float>(Size.X), static_cast<float>(Size.Y));
		}

		/** Text standing on Baseline; Align 0 puts X at its left end, 0.5 in its middle, 1 at its right end. */
		float Text(const FString& Words, const FSlateFontInfo& Info, float X, float Baseline, const FLinearColor& Color, float Align = 0.f) const
		{
			const FVector2f Size = Measure(Words, Info);
			const float Left = X - Size.X * Align;
			const float TopY = Baseline - Size.Y * 0.78f;
			FSlateDrawElement::MakeText(Out, Layer + 2,
				Geometry.ToPaintGeometry(Size, FSlateLayoutTransform(FVector2f(Left, TopY))),
				Words, Info, ESlateDrawEffect::None, Color);
			return Size.X;
		}

		/** Text broken into lines no wider than Width, centred on X; gives the last line's baseline. */
		float Wrapped(const FString& Words, const FSlateFontInfo& Info, float X, float Baseline, float Width, float LineHeight, const FLinearColor& Color) const
		{
			TArray<FString> Parts;
			Words.ParseIntoArray(Parts, TEXT(" "));
			FString Current;
			for (const FString& Part : Parts)
			{
				const FString Longer = Current.IsEmpty() ? Part : Current + TEXT(" ") + Part;
				if (!Current.IsEmpty() && Measure(Longer, Info).X > Width)
				{
					Text(Current, Info, X, Baseline, Color, 0.5f);
					Current = Part;
					Baseline += LineHeight;
				}
				else
				{
					Current = Longer;
				}
			}
			if (!Current.IsEmpty())
			{
				Text(Current, Info, X, Baseline, Color, 0.5f);
			}
			return Baseline;
		}

		/** How many lines Wrapped would break the words into. */
		int32 LineCount(const FString& Words, const FSlateFontInfo& Info, float Width) const
		{
			TArray<FString> Parts;
			Words.ParseIntoArray(Parts, TEXT(" "));
			FString Current;
			int32 Count = Parts.Num() > 0 ? 1 : 0;
			for (const FString& Part : Parts)
			{
				const FString Longer = Current.IsEmpty() ? Part : Current + TEXT(" ") + Part;
				if (!Current.IsEmpty() && Measure(Longer, Info).X > Width)
				{
					Current = Part;
					++Count;
				}
				else
				{
					Current = Longer;
				}
			}
			return Count;
		}

		/** A dark plaque with its corners stepped in twice, edged in gold. */
		void Panel(float X, float Y, float W, float H) const
		{
			const float S = PestiHud::Step;
			Box(X + S, Y + S, W - 2.f * S, H - 2.f * S, PestiHud::Ink);
			Box(X + 2.f * S, Y, W - 4.f * S, S, PestiHud::Ink);
			Box(X + 2.f * S, Y + H - S, W - 4.f * S, S, PestiHud::Ink);
			Box(X, Y + 2.f * S, S, H - 4.f * S, PestiHud::Ink);
			Box(X + W - S, Y + 2.f * S, S, H - 4.f * S, PestiHud::Ink);
			const float R = X + W;
			const float B = Y + H;
			Lines({
				FVector2f(X + 2 * S, Y), FVector2f(R - 2 * S, Y), FVector2f(R - 2 * S, Y + S), FVector2f(R - S, Y + S),
				FVector2f(R - S, Y + 2 * S), FVector2f(R, Y + 2 * S), FVector2f(R, B - 2 * S), FVector2f(R - S, B - 2 * S),
				FVector2f(R - S, B - S), FVector2f(R - 2 * S, B - S), FVector2f(R - 2 * S, B), FVector2f(X + 2 * S, B),
				FVector2f(X + 2 * S, B - S), FVector2f(X + S, B - S), FVector2f(X + S, B - 2 * S), FVector2f(X, B - 2 * S),
				FVector2f(X, Y + 2 * S), FVector2f(X + S, Y + 2 * S), FVector2f(X + S, Y + S), FVector2f(X + 2 * S, Y + S),
				FVector2f(X + 2 * S, Y) }, PestiHud::Gold, 2.f);
			Line(X + 2 * S + 4, Y + S + 4, R - 2 * S - 4, Y + S + 4, PestiHud::Dim, 1.f);
			Line(X + 2 * S + 4, B - S - 4, R - 2 * S - 4, B - S - 4, PestiHud::Dim, 1.f);
		}

		/** A label sitting in a gap in a panel's top line, a diamond each side. */
		void Tab(const FString& Label, float CentreX, float Y) const
		{
			const FSlateFontInfo Info = Deco(20.f, 3.f);
			const float W = Measure(Label, Info).X + 34.f;
			Box(CentreX - W * 0.5f, Y - 3.f, W, 6.f, PestiHud::Solid, 1);
			Box(CentreX - W * 0.5f + 16.f, Y - 14.f, W - 32.f, 26.f, PestiHud::Solid, 1);
			Diamond(CentreX - W * 0.5f + 8.f, Y, 7.f, PestiHud::Gold, 2);
			Diamond(CentreX + W * 0.5f - 8.f, Y, 7.f, PestiHud::Gold, 2);
			Text(Label, Info, CentreX, Y + 8.f, PestiHud::Gold, 0.5f);
		}

		/** The deco sunrise: a half circle of rays over a point. */
		void Rays(float X, float Y, float Radius, int32 Count, const FLinearColor& Color) const
		{
			for (int32 i = 0; i < Count; ++i)
			{
				const float A = UE_PI + UE_PI * (i + 0.5f) / Count;
				Line(X + FMath::Cos(A) * Radius * 0.35f, Y + FMath::Sin(A) * Radius * 0.35f,
					X + FMath::Cos(A) * Radius, Y + FMath::Sin(A) * Radius, Color, 1.5f, 0);
			}
		}

		/** A band that fades out at both ends. */
		void Band(float CentreX, float Y, float W, float H) const
		{
			constexpr int32 Fades = 8;
			const float Edge = W * 0.12f;
			Box(CentreX - W * 0.5f + Edge, Y, W - 2.f * Edge, H, PestiHud::Ink);
			for (int32 i = 0; i < Fades; ++i)
			{
				FLinearColor Color = PestiHud::Ink;
				Color.A *= (i + 0.5f) / Fades;
				const float Part = Edge / Fades;
				Box(CentreX - W * 0.5f + i * Part, Y, Part, H, Color);
				Box(CentreX + W * 0.5f - (i + 1) * Part, Y, Part, H, Color);
			}
		}
	};

	void PaintGoal(const FPainter& Paint, const APestiHUD& Hud) const
	{
		const FSlateFontInfo HuFont = Paint.Bold(26.f);
		const FSlateFontInfo EnFont = Paint.Plain(19.f);
		const bool bFar = !Hud.GoalFar.IsEmpty();
		const float TextWidth = FMath::Max(Paint.Measure(Hud.GoalHu, HuFont).X, Paint.Measure(Hud.GoalEn, EnFont).X);
		const float X = PestiHud::Margin;
		const float Y = PestiHud::Top;
		const float W = FMath::Max(PestiHud::GoalMinWidth, TextWidth + 60.f + (bFar ? 140.f : 0.f));
		const float H = PestiHud::GoalHeight;
		Paint.Panel(X, Y, W, H);
		Paint.Tab(TEXT("C\u00c9L"), X + 70.f, Y);
		Paint.Text(Hud.GoalHu, HuFont, X + 30.f, Y + 50.f, PestiHud::Cream);
		Paint.Text(Hud.GoalEn, EnFont, X + 30.f, Y + 80.f, PestiHud::Grey);
		if (bFar)
		{
			Paint.Line(X + W - 128.f, Y + 26.f, X + W - 128.f, Y + H - 26.f, PestiHud::Dim, 1.f);
			Paint.Text(Hud.GoalFar, Paint.Deco(24.f), X + W - 66.f, Y + 52.f, PestiHud::Gold, 0.5f);
			Paint.Text(Hud.GoalWay, Paint.Plain(18.f, 2.f), X + W - 66.f, Y + 80.f, PestiHud::Grey, 0.5f);
		}
	}

	void PaintNotes(const FPainter& Paint, const APestiHUD& Hud, double Now, float Y) const
	{
		TArray<int32> Keys;
		for (const TPair<int32, APestiHUD::FNote>& Note : Hud.Notes)
		{
			if (Note.Value.Until >= Now && !Note.Value.Text.IsEmpty())
			{
				Keys.Add(Note.Key);
			}
		}
		if (Keys.Num() == 0)
		{
			return;
		}
		Keys.Sort();
		const FSlateFontInfo Info = Paint.Plain(17.f);
		constexpr float LineHeight = 25.f;
		float Widest = 0.f;
		for (const int32 Key : Keys)
		{
			Widest = FMath::Max(Widest, Paint.Measure(Hud.Notes[Key].Text, Info).X);
		}
		const float X = PestiHud::Margin;
		const float H = 30.f + LineHeight * Keys.Num();
		Paint.Panel(X, Y, Widest + 50.f, H);
		float Baseline = Y + 15.f + LineHeight * 0.8f;
		for (const int32 Key : Keys)
		{
			Paint.Diamond(X + 16.f, Baseline - 6.f, 5.f, PestiHud::Brass, 2);
			Paint.Text(Hud.Notes[Key].Text, Info, X + 28.f, Baseline, Hud.Notes[Key].Color);
			Baseline += LineHeight;
		}
	}

	void PaintStatus(const FPainter& Paint, float Wide) const
	{
		const APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(&Paint.Hud);
		if (!Game)
		{
			return;
		}
		const float W = PestiHud::StatusWidth;
		const float H = PestiHud::StatusHeight;
		const float X = Wide - PestiHud::Margin - W;
		const float Y = PestiHud::Top;
		Paint.Panel(X, Y, W, H);

		// Money: big brass figures with the unit small after them.
		Paint.Rays(X + W * 0.5f, Y + 2.f, 26.f, 9, PestiHud::Dim);
		const int32 Money = Game->GetMoney();
		Paint.Text(FString::Printf(TEXT("%d,%02d"), Money / 100, Money % 100), Paint.Deco(36.f, 1.f), X + W - 112.f, Y + 58.f, PestiHud::Gold, 1.f);
		Paint.Text(TEXT("peng\u0151"), Paint.Plain(20.f), X + W - 104.f, Y + 58.f, PestiHud::Grey);
		// The time of day, left of the money.
		if (const ADayNight* Clock = ADayNight::Get(Paint.Hud.GetWorld()))
		{
			Paint.Text(Clock->GetClockText(), Paint.Deco(20.f, 1.f), X + 28.f, Y + 56.f, PestiHud::Cream);
		}
		Paint.Line(X + 28.f, Y + 76.f, X + W - 28.f, Y + 76.f, PestiHud::Dim, 1.f);
		Paint.Diamond(X + W * 0.5f, Y + 76.f, 6.f, PestiHud::Gold, 2);

		// Health: ten bars between two bracket ends.
		Paint.Text(TEXT("EG\u00c9SZS\u00c9G"), Paint.Bold(15.f, 3.f), X + 28.f, Y + 104.f, PestiHud::Grey);
		Paint.Text(TEXT("health"), Paint.Plain(15.f, 1.f), X + W - 28.f, Y + 104.f, PestiHud::Brass, 1.f);
		const float BX = X + 40.f;
		const float BY = Y + 114.f;
		const float BW = W - 80.f;
		const float BH = 16.f;
		constexpr float Gap = 4.f;
		const float BarW = (BW - Gap * (PestiHud::HealthBars - 1)) / PestiHud::HealthBars;
		Paint.Lines({ FVector2f(BX - 6.f, BY - 4.f), FVector2f(BX - 12.f, BY - 4.f), FVector2f(BX - 12.f, BY + BH + 4.f), FVector2f(BX - 6.f, BY + BH + 4.f) }, PestiHud::Gold, 2.f);
		Paint.Lines({ FVector2f(BX + BW + 6.f, BY - 4.f), FVector2f(BX + BW + 12.f, BY - 4.f), FVector2f(BX + BW + 12.f, BY + BH + 4.f), FVector2f(BX + BW + 6.f, BY + BH + 4.f) }, PestiHud::Gold, 2.f);
		const float Health = Game->GetHealth() / Game->GetMaxHealth() * PestiHud::HealthBars;
		for (int32 i = 0; i < PestiHud::HealthBars; ++i)
		{
			const float Fill = FMath::Clamp(Health - i, 0.f, 1.f);
			const float BarX = BX + i * (BarW + Gap);
			Paint.Box(BarX, BY, BarW, BH, PestiHud::RedEmpty, 1);
			if (Fill > 0.f)
			{
				Paint.Box(BarX, BY, BarW * Fill, BH, PestiHud::Red, 2);
				Paint.Box(BarX, BY, BarW * Fill, BH * 0.45f, PestiHud::RedHi, 2);
			}
		}

		// Wanted: a small plaque under the panel, its diamonds lit red one by one.
		const int32 Wanted = Game->GetWantedLevel();
		if (Wanted <= 0)
		{
			return;
		}
		Paint.Panel(X + 90.f, Y + H + 4.f, W - 90.f, 46.f);
		for (int32 i = 0; i < PestiHud::WantedDiamonds; ++i)
		{
			const float DX = X + W - 40.f - (PestiHud::WantedDiamonds - 1 - i) * 34.f;
			const float DY = Y + H + 26.f;
			Paint.Diamond(DX, DY, 22.f, PestiHud::Gold, 1);
			Paint.Diamond(DX, DY, 17.f, i < Wanted ? PestiHud::RedHi : PestiHud::Solid, 2);
		}
		// Being arrested shows instead of the seconds left to lose them.
		const float Arrest = Game->GetArrestFraction();
		const FString Label = Arrest > 0.f
			? FString::Printf(TEXT("ELFOG\u00c1S %d%%"), FMath::RoundToInt(100.f * Arrest))
			: FString::Printf(TEXT("K\u00d6R\u00d6Z\u00c9S %d"), FMath::CeilToInt(Game->GetEscapeSecondsLeft()));
		Paint.Text(Label, Paint.Bold(15.f, 3.f), X + W - 124.f, Y + H + 33.f, PestiHud::RedHi, 1.f);
	}

	void PaintFight(const FPainter& Paint, const APestiHUD::FFight& Fight, float Wide) const
	{
		// The other man: his name over a long red bar, top middle.
		const float W = PestiHud::FoeWidth;
		const float X = (Wide - W) * 0.5f;
		const float Y = PestiHud::Top;
		Paint.Panel(X, Y, W, 78.f);
		if (!Fight.Name.IsEmpty())
		{
			Paint.Tab(Fight.Name, Wide * 0.5f, Y);
		}
		const float BX = X + 36.f;
		const float BW = W - 72.f;
		const float BY = Y + 34.f;
		Paint.Box(BX, BY, BW, 16.f, PestiHud::RedEmpty, 1);
		const float Health = FMath::Clamp(Fight.Health, 0.f, 1.f);
		if (Health > 0.f)
		{
			Paint.Box(BX, BY, BW * Health, 16.f, PestiHud::Red, 2);
			Paint.Box(BX, BY, BW * Health, 7.f, PestiHud::RedHi, 2);
		}
		Paint.Diamond(BX - 14.f, BY + 8.f, 8.f, PestiHud::Gold, 2);
		Paint.Diamond(BX + BW + 14.f, BY + 8.f, 8.f, PestiHud::Gold, 2);

		// His own breath: a plaque under the money and health, below the wanted one if that shows.
		const APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(&Paint.Hud);
		const float SW = PestiHud::StatusWidth;
		const float SX = Wide - PestiHud::Margin - SW;
		const float SY = PestiHud::Top + PestiHud::StatusHeight + 4.f + (Game && Game->GetWantedLevel() > 0 ? 50.f : 0.f);
		Paint.Panel(SX, SY, SW, 56.f);
		Paint.Text(TEXT("ER\u0150"), Paint.Bold(15.f, 3.f), SX + 28.f, SY + 24.f, PestiHud::Grey);
		Paint.Text(TEXT("breath"), Paint.Plain(15.f, 1.f), SX + SW - 28.f, SY + 24.f, PestiHud::Brass, 1.f);
		const float Breath = FMath::Clamp(Fight.Breath, 0.f, 1.f);
		Paint.Box(SX + 28.f, SY + 34.f, SW - 56.f, 9.f, PestiHud::Dim, 1);
		Paint.Box(SX + 28.f, SY + 34.f, (SW - 56.f) * Breath, 9.f, Breath < PestiHud::LowBreath ? PestiHud::RedHi : PestiHud::Gold, 2);
	}

	void PaintSpeech(const FPainter& Paint, const APestiHUD::FSpeech& Speech, float CentreX, float CentreY) const
	{
		const float W = PestiHud::SpeechWidth;
		const bool bTwo = !Speech.En.IsEmpty();
		Paint.Band(CentreX, CentreY - 52.f, W, bTwo ? 112.f : 80.f);
		const float Bottom = CentreY + (bTwo ? 60.f : 28.f);
		Paint.Line(CentreX - W * 0.5f + 120.f, CentreY - 52.f, CentreX + W * 0.5f - 120.f, CentreY - 52.f, PestiHud::Dim, 1.f);
		Paint.Line(CentreX - W * 0.5f + 120.f, Bottom, CentreX + W * 0.5f - 120.f, Bottom, PestiHud::Dim, 1.f);
		FString Hu = Speech.Hu;
		if (!Speech.Who.IsEmpty())
		{
			const FString Name = Speech.Who.ToUpper();
			const FSlateFontInfo Info = Paint.Deco(20.f, 3.f);
			const float NameW = Paint.Measure(Name, Info).X;
			Paint.Box(CentreX - NameW * 0.5f - 10.f, CentreY - 66.f, NameW + 20.f, 28.f, PestiHud::Solid, 1);
			Paint.Diamond(CentreX - NameW * 0.5f - 20.f, CentreY - 52.f, 8.f, PestiHud::Gold, 2);
			Paint.Diamond(CentreX + NameW * 0.5f + 20.f, CentreY - 52.f, 8.f, PestiHud::Gold, 2);
			Paint.Text(Name, Info, CentreX, CentreY - 45.f, PestiHud::Gold, 0.5f);
			Hu = TEXT("\u201e") + Hu + TEXT("\u201d");
		}
		Paint.Text(Hu, Paint.Bold(27.f), CentreX, CentreY + 4.f, PestiHud::Cream, 0.5f);
		if (bTwo)
		{
			Paint.Text(Speech.En, Paint.Plain(20.f), CentreX, CentreY + 38.f, PestiHud::Grey, 0.5f);
		}
	}

	void PaintAnnounce(const FPainter& Paint, const APestiHUD::FSpeech& Big, float CentreX) const
	{
		const float Y = PestiHud::AnnounceY;
		Paint.Rays(CentreX, Y - 46.f, 120.f, 15, PestiHud::Fan);
		Paint.Band(CentreX, Y - 50.f, 1000.f, 128.f);
		Paint.Line(CentreX - 340.f, Y - 50.f, CentreX + 340.f, Y - 50.f, PestiHud::Gold, 2.f);
		Paint.Line(CentreX - 340.f, Y + 78.f, CentreX + 340.f, Y + 78.f, PestiHud::Gold, 2.f);
		Paint.Line(CentreX - 300.f, Y + 84.f, CentreX + 300.f, Y + 84.f, PestiHud::Dim, 1.f);
		Paint.Diamond(CentreX, Y - 50.f, 12.f, PestiHud::Gold, 2);
		Paint.Text(Big.Hu, Paint.Deco(Big.Hu.Len() > 24 ? 40.f : 54.f, 4.f), CentreX, Y + 14.f, PestiHud::Gold, 0.5f);
		Paint.Text(Big.En, Paint.Plain(22.f, 1.f), CentreX, Y + 54.f, PestiHud::Cream, 0.5f);
	}

	void PaintPrompt(const FPainter& Paint, const APestiHUD::FPrompt& Sign, float CentreX, float CentreY) const
	{
		// The key in a little gold-edged plate, what it does beside it.
		const FSlateFontInfo KeyFont = Paint.Bold(14.f, 1.5f);
		const FSlateFontInfo HuFont = Paint.Deco(19.f, 2.f);
		const FSlateFontInfo EnFont = Paint.Plain(13.f, 1.f);
		const float KeyW = Paint.Measure(Sign.Key, KeyFont).X + 20.f;
		const float KeyH = 27.f;
		const float Words = FMath::Max(Paint.Measure(Sign.Hu, HuFont).X, Paint.Measure(Sign.En, EnFont).X);
		const float Gap = 13.f;
		const float Whole = KeyW + Gap + Words;
		const float Left = CentreX - Whole * 0.5f;
		Paint.Band(CentreX, CentreY - 28.f, Whole + 160.f, 56.f);
		const float KeyY = CentreY - KeyH * 0.5f;
		if (Sign.bNow)
		{
			Paint.Box(Left, KeyY, KeyW, KeyH, PestiHud::Gold, 1);
			Paint.Text(Sign.Key, KeyFont, Left + KeyW * 0.5f, CentreY + 5.f, PestiHud::Solid, 0.5f);
			Paint.Diamond(Left - 11.f, CentreY, 7.f, PestiHud::Gold, 2);
			Paint.Diamond(Left + Whole + 11.f, CentreY, 7.f, PestiHud::Gold, 2);
		}
		else
		{
			Paint.Box(Left, KeyY, KeyW, KeyH, PestiHud::Solid, 1);
			Paint.Frame(Left, KeyY, KeyW, KeyH, PestiHud::Gold, 1.5f, 2);
			Paint.Text(Sign.Key, KeyFont, Left + KeyW * 0.5f, CentreY + 5.f, PestiHud::Gold, 0.5f);
		}
		const float WordsX = Left + KeyW + Gap;
		Paint.Text(Sign.Hu, HuFont, WordsX, CentreY + 1.f, Sign.bNow ? PestiHud::Gold : PestiHud::Cream);
		Paint.Text(Sign.En, EnFont, WordsX, CentreY + 19.f, PestiHud::Grey);
	}

	/** One line of a letter as it is written out, and whether it sits on the right. */
	struct FLetterLine
	{
		FString Words;
		bool bRight = false;
	};

	/** Breaks Words at \n and then into lines no wider than Width; an empty line stays as a gap. */
	static TArray<FLetterLine> BreakLines(const FPainter& Paint, const FString& Words, const FSlateFontInfo& Info, float Width)
	{
		TArray<FLetterLine> Result;
		TArray<FString> Paragraphs;
		Words.ParseIntoArray(Paragraphs, TEXT("\n"), false);
		for (FString Paragraph : Paragraphs)
		{
			const bool bRight = Paragraph.StartsWith(TEXT(">"));
			if (bRight)
			{
				Paragraph.RightChopInline(1);
			}
			TArray<FString> Parts;
			Paragraph.ParseIntoArray(Parts, TEXT(" "));
			if (Parts.Num() == 0)
			{
				Result.Add(FLetterLine());
				continue;
			}
			FString Current;
			for (const FString& Part : Parts)
			{
				const FString Longer = Current.IsEmpty() ? Part : Current + TEXT(" ") + Part;
				if (!Current.IsEmpty() && Paint.Measure(Longer, Info).X > Width)
				{
					Result.Add(FLetterLine{ Current, bRight });
					Current = Part;
				}
				else
				{
					Current = Longer;
				}
			}
			Result.Add(FLetterLine{ Current, bRight });
		}
		return Result;
	}

	/** A letter held up: the handwritten page left of the middle, the English on a plaque to its right. */
	void PaintLetter(const FPainter& Paint, const APestiHUD& Hud, float Wide, float High) const
	{
		Paint.Box(0.f, 0.f, Wide, High, PestiHud::LetterShade);

		// The page, as tall as its writing.
		const FSlateFontInfo Hand = Paint.Font(Hud.HandFont, PestiHud::HandPx);
		const float PaperW = PestiHud::PaperWidth;
		const TArray<FLetterLine> Written = BreakLines(Paint, Hud.LetterHu, Hand, PaperW - 2.f * PestiHud::PaperPad);
		const float PaperH = FMath::Min(High - 40.f, 2.f * PestiHud::PaperPad + Written.Num() * PestiHud::HandLine);
		const float PaperX = Wide * 0.5f - PaperW - PestiHud::LetterGap * 0.5f;
		const float PaperY = (High - PaperH) * 0.5f;
		Paint.Box(PaperX + 8.f, PaperY + 10.f, PaperW, PaperH, PestiHud::PaperShadow, 0);
		Paint.Box(PaperX, PaperY, PaperW, PaperH, PestiHud::Paper, 1);
		Paint.Frame(PaperX, PaperY, PaperW, PaperH, PestiHud::PaperEdge, 1.5f, 1);
		float Baseline = PaperY + PestiHud::PaperPad + PestiHud::HandLine * 0.6f;
		for (const FLetterLine& Row : Written)
		{
			if (!Row.Words.IsEmpty())
			{
				const float X = Row.bRight ? PaperX + PaperW - PestiHud::PaperPad : PaperX + PestiHud::PaperPad;
				Paint.Text(Row.Words, Hand, X, Baseline, PestiHud::LetterInk, Row.bRight ? 1.f : 0.f);
			}
			Baseline += PestiHud::HandLine;
		}

		// The English, on a plaque as tall as its lines.
		const FSlateFontInfo Plain = Paint.Plain(PestiHud::PlaquePx);
		const float PlaqueX = Wide * 0.5f + PestiHud::LetterGap * 0.5f;
		const float Inner = PestiHud::PlaqueWidth - 2.f * PestiHud::PlaquePad;
		const TArray<FLetterLine> English = BreakLines(Paint, Hud.LetterEn, Plain, Inner);
		const float PlaqueH = 2.f * PestiHud::PlaquePad + 50.f + English.Num() * PestiHud::PlaqueLine + 40.f;
		const float PlaqueY = (High - PlaqueH) * 0.5f;
		Paint.Panel(PlaqueX, PlaqueY, PestiHud::PlaqueWidth, PlaqueH);
		const float TextLeft = PlaqueX + PestiHud::PlaquePad;
		Paint.Text(TEXT("LEV\u00c9L"), Paint.Deco(24.f, 3.f), TextLeft, PlaqueY + PestiHud::PlaquePad + 22.f, PestiHud::Gold);
		Paint.Text(TEXT("the letter"), Paint.Plain(15.f, 1.f), PlaqueX + PestiHud::PlaqueWidth - PestiHud::PlaquePad, PlaqueY + PestiHud::PlaquePad + 22.f, PestiHud::Grey, 1.f);
		float EnglishBaseline = PlaqueY + PestiHud::PlaquePad + 50.f + PestiHud::PlaqueLine * 0.7f;
		for (const FLetterLine& Said : English)
		{
			if (!Said.Words.IsEmpty())
			{
				Paint.Text(Said.Words, Plain, Said.bRight ? TextLeft + Inner : TextLeft, EnglishBaseline, PestiHud::Cream, Said.bRight ? 1.f : 0.f);
			}
			EnglishBaseline += PestiHud::PlaqueLine;
		}
		Paint.Text(TEXT("R  \u00b7  elrak\u00e1s  \u00b7  put away"), Paint.Plain(14.f, 1.f), TextLeft, PlaqueY + PlaqueH - PestiHud::PlaquePad, PestiHud::Grey);
	}

	void PaintCabinet(const FPainter& Paint, const ALighters& Collection, float Wide, float High) const
	{
		using namespace PestiHud;
		const APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(&Paint.Hud);
		const TArray<LighterModel::FLighter>& All = LighterModel::All();
		const int32 Total = All.Num();
		const int32 Found = Game ? Game->GetLightersFound() : 0;
		auto IsFound = [Game](int32 Index) { return Game && Game->HasFoundLighter(Index); };

		Paint.Box(0.f, 0.f, Wide, High, CabinetShade);
		const float W = CabinetWidth;
		const float H = CabinetHeight;
		const float X = (Wide - W) * 0.5f;
		const float Y = (High - H) * 0.5f;
		Paint.Panel(X, Y, W, H);
		Paint.Tab(TEXT("GY\u0170JTEM\u00c9NY"), X + W * 0.5f, Y);
		Paint.Rays(X + W * 0.5f, Y + 80.f, 90.f, 13, Dim);
		Paint.Text(TEXT("\u00d6ngy\u00fajt\u00f3k"), Paint.Deco(40.f, 2.f), X + W * 0.5f - 20.f, Y + 70.f, Gold, 1.f);
		Paint.Text(TEXT("Lighters"), Paint.Plain(22.f, 1.f), X + W * 0.5f, Y + 70.f, Grey);
		Paint.Text(FString::Printf(TEXT("%d / %d"), Found, Total), Paint.Bold(22.f, 2.f), X + W * 0.5f, Y + 104.f, Cream, 0.5f);

		// The niches.
		const float GridX = X + 50.f;
		const float GridY = Y + 130.f;
		const int32 Rows = FMath::DivideAndRoundUp(Total, NicheColumns);
		const int32 Chosen = FMath::Clamp(Collection.GetSelected(), 0, Total - 1);
		for (int32 i = 0; i < Total; ++i)
		{
			const float NX = GridX + (i % NicheColumns) * (NicheWidth + NicheGap);
			const float NY = GridY + (i / NicheColumns) * (NicheHeight + NicheGap);
			const bool bFound = IsFound(i);
			const bool bChosen = i == Chosen;
			Paint.Box(NX, NY, NicheWidth, NicheHeight, bFound ? Velvet : VelvetDark);
			Paint.Frame(NX, NY, NicheWidth, NicheHeight, bChosen ? Gold : Dim, bChosen ? 3.f : 1.5f);
			if (bChosen)
			{
				Paint.Frame(NX + 5.f, NY + 5.f, NicheWidth - 10.f, NicheHeight - 10.f, Gold, 1.f);
			}
			if (bFound && Collection.GetPicture(i))
			{
				const float S = FMath::Min((NicheWidth - 24.f) / PictureWidth, (NicheHeight - 30.f) / PictureHeight);
				Paint.Picture(Collection.GetPicture(i), NX + NicheWidth * 0.5f - PictureWidth * 0.5f * S, NY + NicheHeight - 10.f - PictureHeight * S,
					PictureWidth * S, PictureHeight * S);
			}
			else if (!bFound)
			{
				Paint.Text(TEXT("?"), Paint.Deco(40.f), NX + NicheWidth * 0.5f, NY + NicheHeight * 0.5f + 14.f, NotFoundMark, 0.5f);
			}
			Paint.Text(FString::FromInt(i + 1), Paint.Plain(13.f), NX + 8.f, NY + 18.f, Brass);
		}

		// The chosen one, large, with its story.
		const float GridBottom = GridY + Rows * (NicheHeight + NicheGap) - NicheGap;
		const float DX = GridX + NicheColumns * (NicheWidth + NicheGap) + 30.f;
		const float DW = X + W - 50.f - DX;
		const float Middle = DX + DW * 0.5f;
		Paint.Line(DX - 15.f, GridY, DX - 15.f, GridBottom, Dim, 1.f);
		Paint.Box(DX, GridY, DW, 300.f, Velvet);
		Paint.Frame(DX, GridY, DW, 300.f, Gold, 2.f);
		const LighterModel::FLighter& Lighter = All[Chosen];
		if (IsFound(Chosen))
		{
			Paint.Picture(Collection.GetPicture(Chosen), Middle - 110.f, GridY + 20.f, 220.f, 275.f);
			Paint.Text(Lighter.NameHu, Paint.Deco(30.f, 1.f), Middle, GridY + 352.f, Gold, 0.5f);
			Paint.Text(Lighter.NameEn, Paint.Plain(19.f), Middle, GridY + 382.f, Grey, 0.5f);
			Paint.Diamond(Middle, GridY + 405.f, 7.f, Gold, 2);
			const float Last = Paint.Wrapped(FString(TEXT("\u201e")) + Lighter.LineHu + TEXT("\u201d"), Paint.Bold(21.f), Middle, GridY + 445.f, DW - 30.f, 28.f, Cream);
			Paint.Wrapped(Lighter.LineEn, Paint.Plain(18.f), Middle, Last + 34.f, DW - 30.f, 24.f, Grey);
		}
		else
		{
			Paint.Text(TEXT("?"), Paint.Deco(120.f), Middle, GridY + 195.f, NotFoundMark, 0.5f);
			Paint.Text(TEXT("M\u00e9g nem tal\u00e1ltad meg"), Paint.Deco(30.f, 1.f), Middle, GridY + 352.f, Gold, 0.5f);
			Paint.Text(TEXT("Not found yet"), Paint.Plain(19.f), Middle, GridY + 382.f, Grey, 0.5f);
			Paint.Diamond(Middle, GridY + 405.f, 7.f, Gold, 2);
			Paint.Wrapped(TEXT("Pista b\u00e1csi a zsibv\u00e1s\u00e1ron tudhat valamit."), Paint.Bold(21.f), Middle, GridY + 445.f, DW - 30.f, 28.f, Cream);
			Paint.Wrapped(TEXT("Pista b\u00e1csi at the flea market may know something."), Paint.Plain(18.f), Middle, GridY + 507.f, DW - 30.f, 24.f, Grey);
		}

		// The rewards along the bottom.
		const float RY = GridBottom + 24.f;
		for (int32 i = 0; i < 3; ++i)
		{
			const int32 Needed = (i + 1) * RewardEvery;
			const bool bGot = Found >= Needed;
			const float RX = GridX + i * 310.f;
			Paint.Panel(RX, RY, 290.f, 62.f);
			Paint.Text(FString::FromInt(Needed), Paint.Deco(30.f), RX + 40.f, RY + 42.f, bGot ? Gold : Brass, 0.5f);
			Paint.Text(i < 2 ? FString(TEXT("+3 peng\u0151")) : FString(LighterModel::Golden().NameHu), Paint.Bold(20.f), RX + 78.f, RY + 40.f, bGot ? Cream : Grey);
			if (bGot)
			{
				Paint.Lines({ FVector2f(RX + 254.f, RY + 32.f), FVector2f(RX + 261.f, RY + 39.f), FVector2f(RX + 273.f, RY + 24.f) }, Gold, 3.f, 2);
			}
			if (i == 2)
			{
				Paint.Picture(Collection.GetPicture(Total), RX + 200.f, RY + 6.f, 40.f, 50.f, bGot ? 1.f : 0.25f, 2);
			}
		}
		Paint.Text(TEXT("L: bez\u00e1r\u00e1s (close)   \u00b7   nyilak: v\u00e1laszt\u00e1s (arrows: choose)"), Paint.Plain(17.f),
			X + W - 50.f, RY + 42.f, Grey, 1.f);
	}

	// The menus (FPestiMenu), laid out like the HUD for a screen 1080 units high.

	void PaintMenu(const FPainter& Paint, const APestiHUD& Hud, float Wide, float High) const
	{
		const FPestiMenu& Shown = Hud.Menu;
		Paint.Box(0.f, 0.f, Wide, High, PestiHud::MenuShade);
		const float CentreX = Wide * 0.5f;
		const float Y0 = (High - 1080.f) * 0.5f;
		const float KeysY = High - 50.f;
		switch (Shown.GetPage())
		{
		case EPestiMenuPage::Title:
			PaintTitle(Paint, Shown, CentreX, Y0);
			PaintMenuKeys(Paint, CentreX, KeysY, { { TEXT("W / S"), TEXT("v\u00e1laszt\u00e1s"), TEXT("choose") }, { TEXT("ENTER"), TEXT("rendben"), TEXT("OK") } });
			break;
		case EPestiMenuPage::Pause:
			PaintPause(Paint, Hud, Shown, Wide, Y0);
			PaintMenuKeys(Paint, CentreX, KeysY, { { TEXT("W / S"), TEXT("v\u00e1laszt\u00e1s"), TEXT("choose") }, { TEXT("ENTER"), TEXT("rendben"), TEXT("OK") },
				{ TEXT("ESC / TAB"), TEXT("vissza a j\u00e1t\u00e9kba"), TEXT("back to the game") } });
			break;
		case EPestiMenuPage::Settings:
			PaintSettings(Paint, Shown, CentreX, Y0);
			PaintMenuKeys(Paint, CentreX, KeysY, { { TEXT("W / S"), TEXT("v\u00e1laszt\u00e1s"), TEXT("choose") }, { TEXT("A / D"), TEXT("\u00e1ll\u00edt\u00e1s"), TEXT("change") },
				{ TEXT("ESC / TAB"), TEXT("vissza, mentve"), TEXT("back, saved") } });
			break;
		case EPestiMenuPage::Controls:
			PaintControls(Paint, CentreX, Y0);
			PaintMenuKeys(Paint, CentreX, KeysY, { { TEXT("ESC / TAB"), TEXT("vissza"), TEXT("back") } });
			break;
		case EPestiMenuPage::Credits:
			PaintCredits(Paint, CentreX, Y0);
			PaintMenuKeys(Paint, CentreX, KeysY, { { TEXT("ESC / TAB"), TEXT("vissza"), TEXT("back") } });
			break;
		case EPestiMenuPage::Confirm:
			PaintConfirm(Paint, Shown, CentreX, Y0);
			PaintMenuKeys(Paint, CentreX, KeysY, { { TEXT("W / S"), TEXT("v\u00e1laszt\u00e1s"), TEXT("choose") }, { TEXT("ENTER"), TEXT("rendben"), TEXT("OK") },
				{ TEXT("ESC / TAB"), TEXT("m\u00e9gse"), TEXT("cancel") } });
			break;
		default:
			break;
		}
	}

	/** A menu row: Hungarian big, English small under it; the chosen one gold on a dark band, a diamond either side. */
	void PaintMenuRow(const FPainter& Paint, float CentreX, float Y, const FPestiMenuRow& Row, bool bOn, const FString& Extra) const
	{
		const bool bExtra = !Extra.IsEmpty();
		const FSlateFontInfo HuFont = Paint.Deco(34.f, 4.f);
		if (bOn)
		{
			const float Upper = Y - 44.f;
			const float Lower = Y + (bExtra ? 52.f : 28.f);
			Paint.Band(CentreX, Upper, 660.f, Lower - Upper);
			Paint.Line(CentreX - 220.f, Upper, CentreX + 220.f, Upper, PestiHud::Gold, 1.5f);
			Paint.Line(CentreX - 220.f, Lower, CentreX + 220.f, Lower, PestiHud::Gold, 1.5f);
			const float HalfWidth = Paint.Measure(Row.Hu, HuFont).X * 0.5f;
			Paint.Diamond(CentreX - HalfWidth - 30.f, Y - 12.f, 12.f, PestiHud::Gold, 2);
			Paint.Diamond(CentreX + HalfWidth + 30.f, Y - 12.f, 12.f, PestiHud::Gold, 2);
		}
		const FLinearColor HuColor = !Row.bEnabled ? PestiHud::Dim : (bOn ? PestiHud::Gold : PestiHud::Cream);
		Paint.Text(Row.Hu, HuFont, CentreX, Y, HuColor, 0.5f);
		if (PestiSettings::English())
		{
			Paint.Text(Row.En, Paint.Plain(17.f, 2.f), CentreX, Y + 22.f, !Row.bEnabled ? PestiHud::Dim : (bOn ? PestiHud::Cream : PestiHud::Grey), 0.5f);
		}
		if (bExtra)
		{
			Paint.Text(Extra, Paint.Plain(16.f, 1.f), CentreX, Y + 44.f, PestiHud::Brass, 0.5f);
		}
	}

	/** The keys along the bottom of a menu, each in a little brass-edged plate with what it does beside it. */
	void PaintMenuKeys(const FPainter& Paint, float CentreX, float Baseline, const TArray<PestiHud::FMenuKey>& Hints) const
	{
		const FSlateFontInfo KeyFont = Paint.Bold(16.f);
		const FSlateFontInfo SayFont = Paint.Plain(16.f, 1.f);
		TArray<FString> Labels;
		float Total = 0.f;
		for (const PestiHud::FMenuKey& Hint : Hints)
		{
			const FString Label = PestiSettings::English() ? FString::Printf(TEXT("%s  (%s)"), Hint.Hu, Hint.En) : FString(Hint.Hu);
			Labels.Add(Label);
			Total += Paint.Measure(Hint.Key, KeyFont).X + 28.f + Paint.Measure(Label, SayFont).X + 40.f;
		}
		float X = CentreX - (Total - 40.f) * 0.5f;
		for (int32 i = 0; i < Hints.Num(); ++i)
		{
			const float KeyWidth = Paint.Measure(Hints[i].Key, KeyFont).X + 18.f;
			Paint.Frame(X, Baseline - 18.f, KeyWidth, 24.f, PestiHud::Brass, 1.5f);
			Paint.Text(Hints[i].Key, KeyFont, X + KeyWidth * 0.5f, Baseline, PestiHud::Cream, 0.5f);
			const float SayWidth = Paint.Text(Labels[i], SayFont, X + KeyWidth + 10.f, Baseline, PestiHud::Grey);
			X += KeyWidth + 10.f + SayWidth + 40.f;
		}
	}

	/** A small arrow head, pointing left (-1) or right (1). */
	void PaintChevron(const FPainter& Paint, float X, float Y, float Way, const FLinearColor& Color) const
	{
		Paint.Lines({ FVector2f(X - 6.f * Way, Y - 9.f), FVector2f(X + 3.f * Way, Y), FVector2f(X - 6.f * Way, Y + 9.f) }, Color, 2.5f);
	}

	void PaintTitle(const FPainter& Paint, const FPestiMenu& Shown, float CentreX, float Y0) const
	{
		const bool bEnglish = PestiSettings::English();
		// The name over a big sunrise of rays.
		Paint.Rays(CentreX, Y0 + 300.f, 330.f, 23, PestiHud::Fan);
		Paint.Text(TEXT("PESTI MES\u00c9K"), Paint.Deco(118.f, 10.f), CentreX, Y0 + 270.f, PestiHud::Gold, 0.5f);
		Paint.Line(CentreX - 400.f, Y0 + 310.f, CentreX + 400.f, Y0 + 310.f, PestiHud::Gold, 2.f);
		Paint.Line(CentreX - 340.f, Y0 + 318.f, CentreX + 340.f, Y0 + 318.f, PestiHud::Dim, 1.f);
		Paint.Diamond(CentreX, Y0 + 310.f, 16.f, PestiHud::Gold, 2);
		Paint.Diamond(CentreX, Y0 + 310.f, 9.f, PestiHud::Solid, 3);
		Paint.Text(TEXT("BUDAPEST  \u00b7  1932"), Paint.Bold(24.f, 10.f), CentreX, Y0 + 360.f, PestiHud::Cream, 0.5f);
		if (bEnglish)
		{
			Paint.Text(TEXT("Tales of Pest"), Paint.Plain(19.f, 2.f), CentreX, Y0 + 392.f, PestiHud::Grey, 0.5f);
		}

		// Continue says where the save starts and how far it has got.
		FString SaveLine;
		if (const APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(&Paint.Hud))
		{
			SaveLine = FString::Printf(TEXT("B\u00f6zsi n\u00e9ni h\u00e1za  \u00b7  %s  \u00b7  %d k\u00fcldet\u00e9s k\u00e9sz"),
				*APestiAlvilagGameModeBase::FormatMoney(Game->GetMoney()), Game->GetMissionsDone());
		}
		const TArray<FPestiMenuRow> List = Shown.Rows();
		float RowY = Y0 + 500.f;
		for (int32 i = 0; i < List.Num(); ++i)
		{
			const bool bContinue = List[i].Action == EPestiMenuAction::Continue;
			PaintMenuRow(Paint, CentreX, RowY, List[i], i == Shown.GetChoice(), bContinue ? SaveLine : FString());
			RowY += bContinue ? 118.f : 96.f;
		}
	}

	void PaintPause(const FPainter& Paint, const APestiHUD& Hud, const FPestiMenu& Shown, float Wide, float Y0) const
	{
		const bool bEnglish = PestiSettings::English();
		const float CentreX = Wide * 0.5f;
		const float W = 620.f;
		const float H = 680.f;
		const float X = CentreX - W * 0.5f;
		const float Y = Y0 + 160.f;
		Paint.Panel(X, Y, W, H);
		Paint.Rays(CentreX, Y + 4.f, 60.f, 11, PestiHud::Dim);
		Paint.Tab(TEXT("SZ\u00dcNET"), CentreX, Y);
		if (bEnglish)
		{
			Paint.Text(TEXT("Paused"), Paint.Plain(17.f, 3.f), CentreX, Y + 52.f, PestiHud::Grey, 0.5f);
		}
		const TArray<FPestiMenuRow> List = Shown.Rows();
		for (int32 i = 0; i < List.Num(); ++i)
		{
			PaintMenuRow(Paint, CentreX, Y + 130.f + 92.f * i, List[i], i == Shown.GetChoice(), FString());
		}
		Paint.Text(TEXT("Menteni otthon lehet, B\u00f6zsi n\u00e9ni ajtaj\u00e1n\u00e1l."), Paint.Bold(19.f), CentreX, Y + H + 50.f, PestiHud::Cream, 0.5f);
		if (bEnglish)
		{
			Paint.Text(TEXT("You save at home, at B\u00f6zsi n\u00e9ni's door."), Paint.Plain(16.f, 1.f), CentreX, Y + H + 76.f, PestiHud::Grey, 0.5f);
		}

		// The side panels only where the screen is wide enough for them.
		if (Wide < PestiHud::MenuSidesFrom)
		{
			return;
		}
		const float SideW = 440.f;
		const float SideY = Y0 + 300.f;
		// Left, the goal now.
		if (!Hud.GoalHu.IsEmpty())
		{
			const float GX = CentreX - 840.f;
			const float Inner = SideW - 60.f;
			const FSlateFontInfo HuFont = Paint.Bold(24.f);
			const FSlateFontInfo EnFont = Paint.Plain(18.f);
			const int32 HuLines = Paint.LineCount(Hud.GoalHu, HuFont, Inner);
			const int32 EnLines = Hud.GoalEn.IsEmpty() ? 0 : Paint.LineCount(Hud.GoalEn, EnFont, Inner);
			const float GH = 60.f + 30.f * HuLines + (EnLines > 0 ? 10.f + 24.f * EnLines : 0.f);
			Paint.Panel(GX, SideY, SideW, GH);
			Paint.Tab(TEXT("C\u00c9L"), GX + 70.f, SideY);
			const float Last = Paint.Wrapped(Hud.GoalHu, HuFont, GX + SideW * 0.5f, SideY + 52.f, Inner, 30.f, PestiHud::Cream);
			if (EnLines > 0)
			{
				Paint.Wrapped(Hud.GoalEn, EnFont, GX + SideW * 0.5f, Last + 34.f, Inner, 24.f, PestiHud::Grey);
			}
		}
		// Right, how far the story has got.
		const APestiAlvilagGameModeBase* Game = APestiAlvilagGameModeBase::Get(&Paint.Hud);
		if (!Game)
		{
			return;
		}
		const float RX = CentreX + 400.f;
		Paint.Panel(RX, SideY, SideW, 330.f);
		Paint.Tab(TEXT("\u00c1LL\u00c1S"), RX + 80.f, SideY);
		const int32 Money = Game->GetMoney();
		const FString Rows[][3] = {
			{ TEXT("K\u00fcldet\u00e9sek"), TEXT("missions"), FString::Printf(TEXT("%d / %d"), Game->GetMissionsDone(), PestiHud::MenuActOneMissions) },
			{ TEXT("P\u00e9nz"), TEXT("money"), FString::Printf(TEXT("%d,%02d P"), Money / 100, FMath::Abs(Money % 100)) },
			{ TEXT("\u00d6ngy\u00fajt\u00f3k"), TEXT("lighters"), FString::Printf(TEXT("%d / %d"), Game->GetLightersFound(), LighterModel::All().Num()) },
			{ TEXT("Taxifuvarok"), TEXT("taxi fares"), FString::FromInt(Game->GetTaxiJobs()) },
			{ TEXT("K\u00f3stolt \u00e9telek"), TEXT("dishes tried"), FString::FromInt(Game->GetDishesTried()) },
		};
		const FSlateFontInfo NameFont = Paint.Bold(21.f);
		float RowY = SideY + 64.f;
		for (const FString (&Row)[3] : Rows)
		{
			const float NameW = Paint.Text(Row[0], NameFont, RX + 32.f, RowY, PestiHud::Cream);
			if (bEnglish)
			{
				Paint.Text(Row[1], Paint.Plain(16.f, 1.f), RX + 42.f + NameW, RowY, PestiHud::Brass);
			}
			Paint.Text(Row[2], Paint.Deco(24.f, 1.f), RX + SideW - 32.f, RowY, PestiHud::Gold, 1.f);
			Paint.Line(RX + 32.f, RowY + 16.f, RX + SideW - 32.f, RowY + 16.f, PestiHud::Dim, 1.f);
			RowY += 54.f;
		}
	}

	/** Ten bars between two stepped end caps, like the health. */
	void PaintSlider(const FPainter& Paint, float X, float Y, float W, int32 Bars) const
	{
		constexpr float Gap = 4.f;
		constexpr float BarHeight = 14.f;
		const float BarWidth = (W - Gap * (PestiSettings::Steps - 1)) / PestiSettings::Steps;
		Paint.Lines({ FVector2f(X - 6.f, Y - 4.f), FVector2f(X - 12.f, Y - 4.f), FVector2f(X - 12.f, Y + BarHeight + 4.f), FVector2f(X - 6.f, Y + BarHeight + 4.f) }, PestiHud::Gold, 2.f);
		Paint.Lines({ FVector2f(X + W + 6.f, Y - 4.f), FVector2f(X + W + 12.f, Y - 4.f), FVector2f(X + W + 12.f, Y + BarHeight + 4.f), FVector2f(X + W + 6.f, Y + BarHeight + 4.f) }, PestiHud::Gold, 2.f);
		for (int32 i = 0; i < PestiSettings::Steps; ++i)
		{
			Paint.Box(X + i * (BarWidth + Gap), Y, BarWidth, BarHeight, i < Bars ? PestiHud::Gold : PestiHud::MenuGlow, 1);
		}
	}

	void PaintSettings(const FPainter& Paint, const FPestiMenu& Shown, float CentreX, float Y0) const
	{
		const bool bEnglish = PestiSettings::English();
		const float W = 980.f;
		const float H = 680.f;
		const float X = CentreX - W * 0.5f;
		const float Y = Y0 + 170.f;
		Paint.Panel(X, Y, W, H);
		Paint.Rays(CentreX, Y + 4.f, 60.f, 11, PestiHud::Dim);
		Paint.Tab(TEXT("BE\u00c1LL\u00cdT\u00c1SOK"), CentreX, Y);
		if (bEnglish)
		{
			Paint.Text(TEXT("Settings"), Paint.Plain(17.f, 3.f), CentreX, Y + 52.f, PestiHud::Grey, 0.5f);
		}
		const float ValueX = X + W - 400.f;
		const float ValueW = 330.f;
		for (int32 i = 0; i < FPestiMenu::SettingCount; ++i)
		{
			const float RowY = Y + 120.f + 80.f * i;
			const bool bOn = i == Shown.GetChoice();
			if (bOn)
			{
				Paint.Box(X + 30.f, RowY - 38.f, W - 60.f, 66.f, PestiHud::MenuGlow);
				Paint.Line(X + 30.f, RowY - 38.f, X + W - 30.f, RowY - 38.f, PestiHud::Gold, 1.5f);
				Paint.Line(X + 30.f, RowY + 28.f, X + W - 30.f, RowY + 28.f, PestiHud::Gold, 1.5f);
				Paint.Diamond(X + 46.f, RowY - 5.f, 10.f, PestiHud::Gold, 2);
			}
			FString NameHu;
			FString NameEn;
			FString ValueHu;
			FString ValueEn;
			int32 Bars = -1;
			FPestiMenu::Setting(i, NameHu, NameEn, Bars, ValueHu, ValueEn);
			Paint.Text(NameHu, Paint.Bold(25.f), X + 70.f, RowY, bOn ? PestiHud::Gold : PestiHud::Cream);
			if (bEnglish)
			{
				Paint.Text(NameEn, Paint.Plain(16.f, 1.f), X + 70.f, RowY + 22.f, PestiHud::Grey);
			}
			if (Bars >= 0)
			{
				PaintSlider(Paint, ValueX, RowY - 18.f, ValueW, Bars);
				continue;
			}
			const FLinearColor Arrow = bOn ? PestiHud::Gold : PestiHud::Brass;
			PaintChevron(Paint, ValueX + 4.f, RowY - 10.f, -1.f, Arrow);
			PaintChevron(Paint, ValueX + ValueW - 4.f, RowY - 10.f, 1.f, Arrow);
			Paint.Text(ValueHu, Paint.Deco(24.f, 1.f), ValueX + ValueW * 0.5f, RowY - 4.f, bOn ? PestiHud::Gold : PestiHud::Cream, 0.5f);
			if (bEnglish)
			{
				Paint.Text(ValueEn, Paint.Plain(15.f, 1.f), ValueX + ValueW * 0.5f, RowY + 20.f, PestiHud::Grey, 0.5f);
			}
		}
	}

	void PaintControls(const FPainter& Paint, float CentreX, float Y0) const
	{
		const bool bEnglish = PestiSettings::English();
		static const TCHAR* const Keys[][3] = {
			{ TEXT("W A S D"), TEXT("J\u00e1r\u00e1s, vezet\u00e9s"), TEXT("Walk, drive") },
			{ TEXT("Eg\u00e9r"), TEXT("K\u00f6r\u00fcln\u00e9z\u00e9s"), TEXT("Look around (mouse)") },
			{ TEXT("Shift"), TEXT("Fut\u00e1s"), TEXT("Run") },
			{ TEXT("Sz\u00f3k\u00f6z"), TEXT("Ugr\u00e1s, \u00e1tugr\u00e1s; aut\u00f3ban k\u00e9zif\u00e9k"), TEXT("Jump, vault; handbrake in a car") },
			{ TEXT("E"), TEXT("Besz\u00e9d, haszn\u00e1lat"), TEXT("Talk, use") },
			{ TEXT("F"), TEXT("Besz\u00e1ll\u00e1s, kisz\u00e1ll\u00e1s"), TEXT("Get in or out") },
			{ TEXT("T"), TEXT("Villamos, busz, taxi"), TEXT("Ride a tram, bus or taxi") },
			{ TEXT("Bal eg\u00e9rgomb"), TEXT("\u00dct\u00e9s"), TEXT("Punch (left mouse)") },
			{ TEXT("Jobb eg\u00e9rgomb"), TEXT("V\u00e9dekez\u00e9s"), TEXT("Guard (right mouse)") },
			{ TEXT("C"), TEXT("M\u00e1sik n\u00e9zet"), TEXT("Change the view") },
			{ TEXT("H"), TEXT("F\u00e9nysz\u00f3r\u00f3"), TEXT("Headlights") },
			{ TEXT("L"), TEXT("\u00d6ngy\u00fajt\u00f3-gy\u0171jtem\u00e9ny"), TEXT("Lighter collection") },
			{ TEXT("Esc / Tab"), TEXT("Men\u00fc"), TEXT("Menu") },
		};
		const int32 Count = static_cast<int32>(UE_ARRAY_COUNT(Keys));
		const float W = 1000.f;
		const float H = 120.f + 50.f * Count;
		const float X = CentreX - W * 0.5f;
		const float Y = Y0 + (1080.f - H) * 0.5f - 20.f;
		Paint.Panel(X, Y, W, H);
		Paint.Rays(CentreX, Y + 4.f, 60.f, 11, PestiHud::Dim);
		Paint.Tab(TEXT("IR\u00c1NY\u00cdT\u00c1S"), CentreX, Y);
		if (bEnglish)
		{
			Paint.Text(TEXT("Controls"), Paint.Plain(17.f, 3.f), CentreX, Y + 52.f, PestiHud::Grey, 0.5f);
		}
		for (int32 i = 0; i < Count; ++i)
		{
			const float RowY = Y + 100.f + 50.f * i;
			Paint.Text(Keys[i][0], Paint.Bold(20.f, 1.f), X + 60.f, RowY, PestiHud::Gold);
			Paint.Text(Keys[i][1], Paint.Bold(21.f), X + 300.f, RowY, PestiHud::Cream);
			if (bEnglish)
			{
				Paint.Text(Keys[i][2], Paint.Plain(17.f), X + W - 60.f, RowY, PestiHud::Grey, 1.f);
			}
			Paint.Line(X + 60.f, RowY + 15.f, X + W - 60.f, RowY + 15.f, PestiHud::Dim, 1.f);
		}
	}

	void PaintCredits(const FPainter& Paint, float CentreX, float Y0) const
	{
		const bool bEnglish = PestiSettings::English();
		static const TCHAR* const Lines[][2] = {
			{ TEXT("Daniel j\u00e1t\u00e9ka, az eredeti Pesti Alvil\u00e1g nyom\u00e1n"), TEXT("A game by Daniel, after his original Pesti Alvil\u00e1g") },
			{ TEXT("K\u00f3d \u00e9s modellek: Claude (Anthropic)"), TEXT("Code and models: Claude (Anthropic)") },
			{ TEXT("T\u00e9rk\u00e9p: \u00a9 OpenStreetMap k\u00f6zrem\u0171k\u00f6d\u0151k"), TEXT("Map data: \u00a9 OpenStreetMap contributors") },
			{ TEXT("Bet\u0171k: Limelight \u00e9s Josefin Sans (SIL OFL)"), TEXT("Fonts: Limelight and Josefin Sans (SIL OFL)") },
		};
		const int32 Count = static_cast<int32>(UE_ARRAY_COUNT(Lines));
		const float W = 900.f;
		const float H = 200.f + 76.f * Count;
		const float X = CentreX - W * 0.5f;
		const float Y = Y0 + (1080.f - H) * 0.5f - 20.f;
		Paint.Panel(X, Y, W, H);
		Paint.Rays(CentreX, Y + 4.f, 60.f, 11, PestiHud::Dim);
		Paint.Tab(TEXT("K\u00c9SZ\u00cdT\u0150K"), CentreX, Y);
		Paint.Text(TEXT("PESTI MES\u00c9K"), Paint.Deco(48.f, 6.f), CentreX, Y + 110.f, PestiHud::Gold, 0.5f);
		Paint.Diamond(CentreX, Y + 138.f, 10.f, PestiHud::Gold, 2);
		Paint.Line(CentreX - 200.f, Y + 138.f, CentreX + 200.f, Y + 138.f, PestiHud::Dim, 1.f);
		for (int32 i = 0; i < Count; ++i)
		{
			const float RowY = Y + 200.f + 76.f * i;
			Paint.Text(Lines[i][0], Paint.Bold(22.f), CentreX, RowY, PestiHud::Cream, 0.5f);
			if (bEnglish)
			{
				Paint.Text(Lines[i][1], Paint.Plain(17.f), CentreX, RowY + 26.f, PestiHud::Grey, 0.5f);
			}
		}
	}

	void PaintConfirm(const FPainter& Paint, const FPestiMenu& Shown, float CentreX, float Y0) const
	{
		const float W = 760.f;
		const float H = 380.f;
		const float X = CentreX - W * 0.5f;
		const float Y = Y0 + 330.f;
		Paint.Panel(X, Y, W, H);
		Paint.Tab(TEXT("BIZTOS?"), CentreX, Y);
		FString AskHu;
		FString AskEn;
		Shown.Question(AskHu, AskEn);
		const float Last = Paint.Wrapped(AskHu, Paint.Bold(24.f), CentreX, Y + 70.f, W - 100.f, 30.f, PestiHud::Cream);
		if (PestiSettings::English())
		{
			Paint.Wrapped(AskEn, Paint.Plain(18.f), CentreX, Last + 32.f, W - 100.f, 24.f, PestiHud::Grey);
		}
		const TArray<FPestiMenuRow> List = Shown.Rows();
		for (int32 i = 0; i < List.Num(); ++i)
		{
			PaintMenuRow(Paint, CentreX, Y + 230.f + 86.f * i, List[i], i == Shown.GetChoice(), FString());
		}
	}

	TWeakObjectPtr<const APestiHUD> Owner;
};

APestiHUD* APestiHUD::Get(const UObject* WorldContextObject)
{
	const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	const APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	return PC ? Cast<APestiHUD>(PC->GetHUD()) : nullptr;
}

void APestiHUD::SetLetter(const FString& Hu, const FString& En)
{
	LetterHu = Hu;
	LetterEn = En;
}

void APestiHUD::ClearLetter()
{
	LetterHu.Reset();
	LetterEn.Reset();
}

void APestiHUD::SetCabinet(const ALighters* Shown)
{
	// Here, where ALighters is fully known: the weak pointer needs its whole type.
	Cabinet = Shown;
}

APestiHUD::APestiHUD()
{
	// The menus read their keys while the game stands still.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bTickEvenWhenPaused = true;
}

void APestiHUD::BeginPlay()
{
	Super::BeginPlay();
	SetTickableWhenPaused(true);

	DecoFont = PestiHud::LoadFont(TEXT("Limelight-Regular.ttf"));
	BoldFont = PestiHud::LoadFont(TEXT("JosefinSans-SemiBold.ttf"));
	TextFont = PestiHud::LoadFont(TEXT("JosefinSans-Regular.ttf"));
	HandFont = PestiHud::LoadFont(TEXT("Parisienne-Regular.ttf"));

	if (GEngine && GEngine->GameViewport)
	{
		Widget = SNew(SPestiHud, this);
		GEngine->GameViewport->AddViewportWidgetContent(Widget.ToSharedRef(), 5);

		const TWeakObjectPtr<APestiHUD> Self(this);
		Blur = SNew(SBackgroundBlur)
			.BlurStrength(PestiHud::MenuBlur)
			.Visibility_Lambda([Self]()
			{
				const APestiHUD* Hud = Self.Get();
				return Hud && Hud->Menu.IsOpen() ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
			});
		GEngine->GameViewport->AddViewportWidgetContent(Blur.ToSharedRef(), 4);
	}
	Menu.Start(GetWorld());
}

void APestiHUD::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Menu.Tick(GetWorld(), GetOwningPlayerController());
}

void APestiHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (Widget.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(Widget.ToSharedRef());
	}
	if (Blur.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(Blur.ToSharedRef());
	}
	Widget.Reset();
	Blur.Reset();
	Super::EndPlay(EndPlayReason);
}

double APestiHUD::Now() const
{
	return GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
}

void APestiHUD::Show(const UObject* WorldContextObject, int32 Key, float Seconds, const FColor& Color, const FString& Text)
{
	APestiHUD* Hud = Get(WorldContextObject);
	if (!Hud)
	{
		return;
	}
	FString Hu = Text;
	FString En;
	const bool bTwo = PestiHud::SplitLanguages(Text, Hu, En);
	if (Key == PestiHud::SpeechKeyA || Key == PestiHud::SpeechKeyB)
	{
		Hud->Say(FString(), Hu, bTwo ? En : FString(), Text.IsEmpty() ? -1.f : Seconds);
	}
	else if (Key == PestiHud::AnnounceKey)
	{
		Hud->Announce(Hu, bTwo ? En : FString(), Text.IsEmpty() ? -1.f : Seconds);
	}
	else
	{
		Hud->Note(Key, Seconds, Color, Text);
	}
}

void APestiHUD::Note(int32 Key, float Seconds, const FColor& Color, const FString& Text)
{
	if (Text.IsEmpty())
	{
		Notes.Remove(Key);
		return;
	}
	// Plain white reads as the panels' cream.
	const FLinearColor Shade = Color == FColor::White ? PestiHud::Cream : FLinearColor(Color);
	// "Hungarian  (English)" keeps only its Hungarian when English subtitles are off.
	FString Kept = Text;
	FString LineHu;
	FString LineEn;
	if (!PestiSettings::English() && PestiHud::SplitLanguages(Text, LineHu, LineEn))
	{
		Kept = LineHu;
	}
	Notes.Add(Key, FNote{ Kept, Shade, Now() + FMath::Max(PestiHud::OneFrame, static_cast<double>(Seconds)) });
}

void APestiHUD::SetGoal(const FString& Hu, const FString& En, const FString& Far, const FString& Way)
{
	GoalHu = Hu;
	GoalEn = PestiSettings::English() ? En : FString();
	GoalFar = Far;
	GoalWay = Way;
}

void APestiHUD::ClearGoal()
{
	SetGoal(FString(), FString(), FString(), FString());
}

void APestiHUD::Say(const FString& Who, const FString& Hu, const FString& En, float Seconds)
{
	Speech = FSpeech{ Who, Hu, PestiSettings::English() ? En : FString(), Now() + Seconds };
}

void APestiHUD::Announce(const FString& Hu, const FString& En, float Seconds)
{
	Big = FSpeech{ FString(), Hu, PestiSettings::English() ? En : FString(), Now() + Seconds };
}

void APestiHUD::Prompt(const FString& Key, const FString& Hu, const FString& En, bool bNow)
{
	KeySign = FPrompt{ Key, Hu, PestiSettings::English() ? En : FString(), bNow, Now() + PestiHud::OneFrame };
}

void APestiHUD::ShowFight(const FString& FoeName, float FoeHealth, float Breath)
{
	Fight = FFight{ FoeName, FoeHealth, Breath, Now() + PestiHud::OneFrame };
}
