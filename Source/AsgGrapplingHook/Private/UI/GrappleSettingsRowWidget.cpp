#include "UI/GrappleSettingsRowWidget.h"

#include "Components/Slider.h"
#include "Components/TextBlock.h"

void UGrappleSettingsRowWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Slider_Reel)
	{
		Slider_Reel->SetMinValue(0.f);
		Slider_Reel->SetMaxValue(1.f);
		Slider_Reel->OnValueChanged.AddDynamic(this, &UGrappleSettingsRowWidget::HandleSliderChanged);
	}
}

void UGrappleSettingsRowWidget::NativeDestruct()
{
	if (Slider_Reel)
	{
		Slider_Reel->OnValueChanged.RemoveDynamic(this, &UGrappleSettingsRowWidget::HandleSliderChanged);
	}
	Super::NativeDestruct();
}

void UGrappleSettingsRowWidget::HandleSliderChanged(float Value)
{
	OnRatioChanged.Broadcast(FMath::Clamp(Value, 0.f, 1.f));
}

void UGrappleSettingsRowWidget::SetRatio(float Ratio)
{
	Ratio = FMath::Clamp(Ratio, 0.f, 1.f);
	if (Slider_Reel)
	{
		// SetValue does not broadcast OnValueChanged, so this won't loop back through OnRatioChanged.
		Slider_Reel->SetValue(Ratio);
	}
	OnRowRefreshed(Ratio);
}

float UGrappleSettingsRowWidget::GetRatio() const
{
	return Slider_Reel ? Slider_Reel->GetValue() : 1.f;
}

void UGrappleSettingsRowWidget::SetValueText(const FText& Text)
{
	if (Value_Reel)
	{
		Value_Reel->SetText(Text);
	}
}
