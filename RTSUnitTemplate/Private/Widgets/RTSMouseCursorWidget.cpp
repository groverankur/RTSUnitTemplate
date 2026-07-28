// Copyright 2025 Silvan Teufel / Teufel-Engineering.com All Rights Reserved.

#include "Widgets/RTSMouseCursorWidget.h"
#include "Components/Image.h"
#include "Blueprint/WidgetTree.h"
#include "Materials/MaterialInterface.h"
#include "Engine/Texture2D.h"

TSharedRef<SWidget> URTSMouseCursorWidget::RebuildWidget()
{
	// Build a bare Image root only when a designer WBP hasn't supplied its own tree.
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		if (!CursorImage)
		{
			CursorImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("CursorImage"));
		}
		WidgetTree->RootWidget = CursorImage;
	}

	TSharedRef<SWidget> Built = Super::RebuildWidget();
	ApplyToImage();
	return Built;
}

void URTSMouseCursorWidget::SetCursorImage(UMaterialInterface* Material, UTexture2D* Texture, FVector2D DrawSize)
{
	PendingMaterial = Material;
	PendingTexture = Texture;
	PendingSize = DrawSize;
	ApplyToImage();
}

void URTSMouseCursorWidget::ApplyToImage()
{
	if (!CursorImage)
	{
		return;
	}

	// Material wins over texture (a material can encode animation/effects); either drives one brush.
	if (PendingMaterial)
	{
		CursorImage->SetBrushFromMaterial(PendingMaterial);
	}
	else if (PendingTexture)
	{
		CursorImage->SetBrushFromTexture(PendingTexture, false);
	}

	CursorImage->SetDesiredSizeOverride(PendingSize);
}
