#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PrikolOverlay.generated.h"

class UPrikolMedia;
enum class EPrikolMode : uint8 { Hidden, Fullscreen, Animation };

/** A single display mode makes simultaneous PNG/GIF display impossible. */
UCLASS()
class DZ1_API UPrikolOverlay : public UUserWidget
{
    GENERATED_BODY()
public:
    void SetMedia(UPrikolMedia* InMedia) { Media = InMedia; }
    void Show(EPrikolMode InMode);
    EPrikolMode GetMode() const { return Mode; }
    int32 GetFrame() const { return Frame; }
    void AdvanceAnimation(float DeltaSeconds);
protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeTick(const FGeometry& Geometry, float DeltaSeconds) override;
    virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& Geometry,
        const FSlateRect& CullingRect, FSlateWindowElementList& Elements, int32 Layer,
        const FWidgetStyle& Style, bool bParentEnabled) const override;
private:
    UPROPERTY() TObjectPtr<UPrikolMedia> Media;
    EPrikolMode Mode = EPrikolMode::Hidden;
    int32 Frame = 0;
    float Elapsed = 0.f;
};
