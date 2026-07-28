// Copyright 2025 Silvan Teufel / Teufel-Engineering.com All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RTSMouseCursorWidget.generated.h"

class UImage;
class UMaterialInterface;
class UTexture2D;

/**
 * Minimal software mouse-cursor widget. Its single Image brush is driven at runtime from a
 * Material (preferred) or a Texture, so the same widget serves both a material-driven cursor
 * and a plain icon cursor. Register it via APlayerController::SetMouseCursorWidget.
 *
 * Works with zero companion assets (it builds its own Image root), but a designer WBP subclass
 * may also provide an Image named "CursorImage" and it will be used instead (BindWidgetOptional).
 */
UCLASS()
class RTSUNITTEMPLATE_API URTSMouseCursorWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Optional designer-provided image; if absent, a root Image is built in code. */
	UPROPERTY(meta = (BindWidgetOptional))
	UImage* CursorImage = nullptr;

	/** Point the cursor image at a Material (preferred) or Texture, at the given draw size. */
	UFUNCTION(BlueprintCallable, Category = "RTSUnitTemplate|Cursor")
	void SetCursorImage(UMaterialInterface* Material, UTexture2D* Texture, FVector2D DrawSize);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	UPROPERTY(Transient) TObjectPtr<UMaterialInterface> PendingMaterial = nullptr;
	UPROPERTY(Transient) TObjectPtr<UTexture2D> PendingTexture = nullptr;
	FVector2D PendingSize = FVector2D(32.f, 32.f);

	void ApplyToImage();
};
