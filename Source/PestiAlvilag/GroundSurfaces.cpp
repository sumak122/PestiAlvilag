// Fill out your copyright notice in the Description page of Project Settings.

#include "GroundSurfaces.h"

#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TextureResource.h"
#include "UObject/Package.h"

#if WITH_EDITOR
#include "Materials/Material.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "Materials/MaterialExpressionDivide.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionNoise.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionSubtract.h"
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionWorldPosition.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogGroundSurfaces, Log, All);

#if WITH_EDITOR

namespace GroundSurfaces::Private
{
	using SurfaceModel::EKind;

	// Pixels across one picture.
	constexpr int32 PictureSize = 512;
	// Big soft patches of lighter and darker ground (wear, damp, dust), so
	// the repeat of the picture doesn't show from afar: their size (cm)...
	constexpr float PatchSize = 900.f;
	// ...and how dark and light they get.
	constexpr float PatchDark = 0.82f;
	constexpr float PatchLight = 1.12f;

	struct FPictures
	{
		TWeakObjectPtr<UTexture2D> Colour;
		TWeakObjectPtr<UTexture2D> Bumps;
	};

	// Made once and kept for the whole session (they never change).
	FPictures Pictures[static_cast<int32>(EKind::Count)];
	TWeakObjectPtr<UMaterial> SharedMaterial;

	/** A texture from R, G, B, A rows, with its smaller copies for far away. */
	UTexture2D* MakeTexture(const TArray<uint8>& Pixels, int32 Size, bool bColour)
	{
		UTexture2D* Texture = UTexture2D::CreateTransient(Size, Size, PF_B8G8R8A8);
		if (!Texture)
		{
			return nullptr;
		}
		Texture->SRGB = bColour;
		Texture->LODGroup = TEXTUREGROUP_World;
		Texture->AddressX = TA_Wrap;
		Texture->AddressY = TA_Wrap;
		Texture->NeverStream = true;

		// The texture wants blue, green, red, alpha.
		TArray<uint8> Level;
		Level.SetNumUninitialized(Size * Size * 4);
		for (int32 i = 0; i < Size * Size; ++i)
		{
			Level[i * 4 + 0] = Pixels[i * 4 + 2];
			Level[i * 4 + 1] = Pixels[i * 4 + 1];
			Level[i * 4 + 2] = Pixels[i * 4 + 0];
			Level[i * 4 + 3] = Pixels[i * 4 + 3];
		}

		FTexturePlatformData* Data = Texture->GetPlatformData();
		int32 LevelSize = Size;
		for (int32 MipIndex = 0;; ++MipIndex)
		{
			FTexture2DMipMap* Mip = nullptr;
			if (MipIndex == 0)
			{
				Mip = &Data->Mips[0];
			}
			else
			{
				Mip = new FTexture2DMipMap(LevelSize, LevelSize, 1);
				Data->Mips.Add(Mip);
			}
			Mip->BulkData.Lock(LOCK_READ_WRITE);
			void* Dest = Mip->BulkData.Realloc(Level.Num());
			FMemory::Memcpy(Dest, Level.GetData(), Level.Num());
			Mip->BulkData.Unlock();

			if (LevelSize == 1)
			{
				break;
			}
			// Each pixel of the next one is the average of four.
			const int32 Half = LevelSize / 2;
			TArray<uint8> Smaller;
			Smaller.SetNumUninitialized(Half * Half * 4);
			for (int32 Y = 0; Y < Half; ++Y)
			{
				for (int32 X = 0; X < Half; ++X)
				{
					for (int32 c = 0; c < 4; ++c)
					{
						const int32 Sum = Level[((2 * Y) * LevelSize + 2 * X) * 4 + c] + Level[((2 * Y) * LevelSize + 2 * X + 1) * 4 + c]
							+ Level[((2 * Y + 1) * LevelSize + 2 * X) * 4 + c] + Level[((2 * Y + 1) * LevelSize + 2 * X + 1) * 4 + c];
						Smaller[(Y * Half + X) * 4 + c] = static_cast<uint8>((Sum + 2) / 4);
					}
				}
			}
			Level = MoveTemp(Smaller);
			LevelSize = Half;
		}

		Texture->UpdateResource();
		Texture->AddToRoot();
		return Texture;
	}

	const FPictures& PicturesOf(EKind Kind)
	{
		FPictures& Found = Pictures[static_cast<int32>(Kind)];
		if (!Found.Colour.IsValid() || !Found.Bumps.IsValid())
		{
			SurfaceModel::FImage Image;
			SurfaceModel::Build(Kind, PictureSize, Image);
			Found.Colour = MakeTexture(Image.Colour, Image.Size, true);
			Found.Bumps = MakeTexture(Image.Normal, Image.Size, false);
		}
		return Found;
	}

	template <typename TExpression>
	TExpression* AddExpression(UMaterial* Material)
	{
		TExpression* Expression = NewObject<TExpression>(Material);
		Expression->Material = Material;
		Material->GetExpressionCollection().AddExpression(Expression);
		return Expression;
	}

	/**
	 * The one material all the ground shares: the picture laid by world
	 * position (TileSize cm per repeat), its colour times Tint and the big
	 * patches, its roughness from the picture's alpha and the bumps from the
	 * second picture (which already points in world directions).
	 */
	UMaterial* MakeMaterial(UTexture2D* DefaultColour, UTexture2D* DefaultBumps)
	{
		UMaterial* Material = NewObject<UMaterial>(GetTransientPackage(), NAME_None, RF_Transient);
		Material->bTangentSpaceNormal = false;
		Material->bUsedWithInstancedStaticMeshes = true;

		auto* Where = AddExpression<UMaterialExpressionWorldPosition>(Material);
		auto* Flat = AddExpression<UMaterialExpressionComponentMask>(Material);
		Flat->R = 1;
		Flat->G = 1;
		Flat->B = 0;
		Flat->A = 0;
		Flat->Input.Connect(0, Where);

		auto* TileSize = AddExpression<UMaterialExpressionScalarParameter>(Material);
		TileSize->ParameterName = TEXT("TileSize");
		TileSize->DefaultValue = 200.f;
		TileSize->UpdateParameterGuid(true, false);

		auto* Coordinates = AddExpression<UMaterialExpressionDivide>(Material);
		Coordinates->A.Connect(0, Flat);
		Coordinates->B.Connect(0, TileSize);

		auto* Colour = AddExpression<UMaterialExpressionTextureSampleParameter2D>(Material);
		Colour->ParameterName = TEXT("Colour");
		Colour->Texture = DefaultColour;
		Colour->SamplerType = SAMPLERTYPE_Color;
		Colour->Coordinates.Connect(0, Coordinates);
		Colour->UpdateParameterGuid(true, false);

		auto* Bumps = AddExpression<UMaterialExpressionTextureSampleParameter2D>(Material);
		Bumps->ParameterName = TEXT("Bumps");
		Bumps->Texture = DefaultBumps;
		Bumps->SamplerType = SAMPLERTYPE_LinearColor;
		Bumps->Coordinates.Connect(0, Coordinates);
		Bumps->UpdateParameterGuid(true, false);

		auto* Tint = AddExpression<UMaterialExpressionVectorParameter>(Material);
		Tint->ParameterName = TEXT("Tint");
		Tint->DefaultValue = FLinearColor::White;
		Tint->UpdateParameterGuid(true, false);

		auto* Patches = AddExpression<UMaterialExpressionNoise>(Material);
		Patches->NoiseFunction = NOISEFUNCTION_ValueALU;
		Patches->Scale = 1.f / PatchSize;
		Patches->Levels = 2;
		Patches->OutputMin = PatchDark;
		Patches->OutputMax = PatchLight;

		auto* Tinted = AddExpression<UMaterialExpressionMultiply>(Material);
		Tinted->A.Connect(0, Colour);
		Tinted->B.Connect(0, Tint);
		auto* Patched = AddExpression<UMaterialExpressionMultiply>(Material);
		Patched->A.Connect(0, Tinted);
		Patched->B.Connect(0, Patches);

		// Bumps are stored as 0..1 for -1..1.
		auto* Doubled = AddExpression<UMaterialExpressionMultiply>(Material);
		Doubled->A.Connect(0, Bumps);
		Doubled->ConstB = 2.f;
		auto* Normal = AddExpression<UMaterialExpressionSubtract>(Material);
		Normal->A.Connect(0, Doubled);
		Normal->ConstB = 1.f;

		UMaterialEditorOnlyData* Inputs = Material->GetEditorOnlyData();
		Inputs->BaseColor.Connect(0, Patched);
		Inputs->Roughness.Connect(4, Colour);
		Inputs->Normal.Connect(0, Normal);

		Material->PreEditChange(nullptr);
		Material->PostEditChange();
		Material->AddToRoot();
		return Material;
	}
}

#endif

UMaterialInstanceDynamic* GroundSurfaces::Make(SurfaceModel::EKind Kind, UObject* Outer)
{
#if WITH_EDITOR
	using namespace GroundSurfaces::Private;

	const FPictures& Found = PicturesOf(Kind);
	if (!Found.Colour.IsValid() || !Found.Bumps.IsValid())
	{
		return nullptr;
	}
	if (!SharedMaterial.IsValid())
	{
		SharedMaterial = MakeMaterial(Found.Colour.Get(), Found.Bumps.Get());
		UE_LOG(LogGroundSurfaces, Log, TEXT("Made the ground material"));
	}

	UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(SharedMaterial.Get(), Outer);
	Material->SetTextureParameterValue(TEXT("Colour"), Found.Colour.Get());
	Material->SetTextureParameterValue(TEXT("Bumps"), Found.Bumps.Get());
	Material->SetScalarParameterValue(TEXT("TileSize"), static_cast<float>(SurfaceModel::TileCm(Kind)));
	return Material;
#else
	return nullptr;
#endif
}
