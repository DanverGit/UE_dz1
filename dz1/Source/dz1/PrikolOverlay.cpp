#include "PrikolOverlay.h"
#include "PrikolMedia.h"
#include "Engine/Texture2D.h"
#include "Rendering/DrawElements.h"
#include "Widgets/Layout/SBox.h"

TSharedRef<SWidget> UPrikolOverlay::RebuildWidget()
{
    return SNew(SBox);
}

void UPrikolOverlay::Show(EPrikolMode InMode)
{
    Mode = InMode;
    Frame = 0;
    Elapsed = 0.f;
    ForceVolatile(Mode == EPrikolMode::Animation);
    SetVisibility(Mode == EPrikolMode::Hidden ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
    InvalidateLayoutAndVolatility();
}

void UPrikolOverlay::AdvanceAnimation(float DeltaSeconds)
{
    if (Mode != EPrikolMode::Animation || !Media || Media->Frames.IsEmpty() ||
        Media->Frames.Num() != Media->FrameDurations.Num()) return;
    float TotalDuration = 0.f;
    for (float Duration : Media->FrameDurations) TotalDuration += FMath::Max(Duration, 0.01f);
    Elapsed += FMath::Fmod(FMath::Max(DeltaSeconds, 0.f), TotalDuration);
    while (Elapsed >= FMath::Max(Media->FrameDurations[Frame], 0.01f))
    {
        Elapsed -= FMath::Max(Media->FrameDurations[Frame], 0.01f);
        Frame = (Frame + 1) % Media->Frames.Num();
    }
}

void UPrikolOverlay::NativeTick(const FGeometry& Geometry, float DeltaSeconds)
{
    Super::NativeTick(Geometry, DeltaSeconds);
    AdvanceAnimation(DeltaSeconds);
}

int32 UPrikolOverlay::NativePaint(const FPaintArgs& Args, const FGeometry& Geometry,
    const FSlateRect& CullingRect, FSlateWindowElementList& Elements, int32 Layer,
    const FWidgetStyle& Style, bool bParentEnabled) const
{
    Layer = Super::NativePaint(Args, Geometry, CullingRect, Elements, Layer, Style, bParentEnabled);
    if (!Media || Mode == EPrikolMode::Hidden) return Layer;
    UTexture2D* Texture = Mode == EPrikolMode::Fullscreen ? Media->FullscreenImage.Get() :
        (Media->Frames.IsValidIndex(Frame) ? Media->Frames[Frame].Get() : nullptr);
    if (!Texture) return Layer;
    const FVector2D View = Geometry.GetLocalSize();
    FVector2D Size = View;
    FVector2D Position = FVector2D::ZeroVector;
    if (Mode == EPrikolMode::Animation)
    {
        const float Width = FMath::Min(400.f, FMath::Min(View.X, View.Y) * 0.6f);
        Size = FVector2D(Width, Width * Texture->GetSizeY() / FMath::Max(Texture->GetSizeX(), 1));
        Position = FVector2D(0.f, View.Y - Size.Y);
    }
    FSlateBrush Brush;
    Brush.SetResourceObject(Texture);
    Brush.ImageSize = FVector2D(Texture->GetSizeX(), Texture->GetSizeY());
    Brush.DrawAs = ESlateBrushDrawType::Image;
    FSlateDrawElement::MakeBox(Elements, ++Layer, Geometry.ToPaintGeometry(Size, FSlateLayoutTransform(Position)),
        &Brush, ESlateDrawEffect::None, FLinearColor::White);
    return Layer;
}
