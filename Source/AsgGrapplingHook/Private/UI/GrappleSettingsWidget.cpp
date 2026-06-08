#include "UI/GrappleSettingsWidget.h"

#include "UI/GrappleSettingsRowWidget.h"
#include "Equipment/GrapplingHookTool.h"
#include "Components/PanelWidget.h"

namespace
{
	FText FormatStatPercent(float Value, float Base)
	{
		const float Percent = (FMath::Abs(Base) > KINDA_SMALL_NUMBER ? Value / Base : 1.f) * 100.f;
		FNumberFormattingOptions Options;
		Options.MaximumFractionalDigits = 0;
		return FText::Format(INVTEXT("{0}%"), FText::AsNumber(Percent, &Options));
	}
}

AGrapplingHookTool* UGrappleSettingsWidget::GetGrapple() const
{
	return Cast<AGrapplingHookTool>(mEquipment);
}

void UGrappleSettingsWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (RowReelSpeed)
	{
		RowReelSpeed->OnRatioChanged.AddDynamic(this, &UGrappleSettingsWidget::OnReelSpeedRatioChanged);
	}
	if (RowCableLength)
	{
		RowCableLength->OnRatioChanged.AddDynamic(this, &UGrappleSettingsWidget::OnMaxCableLengthRatioChanged);
	}
	if (RowEjectionSpeed)
	{
		RowEjectionSpeed->OnRatioChanged.AddDynamic(this, &UGrappleSettingsWidget::OnEjectionSpeedRatioChanged);
	}

	RefreshFromEquipment();
}

void UGrappleSettingsWidget::NativeDestruct()
{
	if (RowReelSpeed)
	{
		RowReelSpeed->OnRatioChanged.RemoveDynamic(this, &UGrappleSettingsWidget::OnReelSpeedRatioChanged);
	}
	if (RowCableLength)
	{
		RowCableLength->OnRatioChanged.RemoveDynamic(this, &UGrappleSettingsWidget::OnMaxCableLengthRatioChanged);
	}
	if (RowEjectionSpeed)
	{
		RowEjectionSpeed->OnRatioChanged.RemoveDynamic(this, &UGrappleSettingsWidget::OnEjectionSpeedRatioChanged);
	}
	Super::NativeDestruct();
}

void UGrappleSettingsWidget::RefreshFromEquipment()
{
	AGrapplingHookTool* Grapple = GetGrapple();
	if (!Grapple)
	{
		return;
	}

	Grapple->RefreshGrappleConfig();

	TGuardValue<bool> SyncGuard(bSyncingFromEquipment, true);
	if (RowReelSpeed)
	{
		RowReelSpeed->SetRatio(Grapple->GetReelSpeedRatio());
		RowReelSpeed->SetValueText(FormatStatPercent(Grapple->GetReelSpeedValue(), Grapple->GetReelSpeedBase()));
	}
	if (RowCableLength)
	{
		RowCableLength->SetRatio(Grapple->GetMaxCableLengthRatio());
		RowCableLength->SetValueText(FormatStatPercent(Grapple->GetMaxCableLengthValue(), Grapple->GetMaxCableLengthBase()));
	}
	if (RowEjectionSpeed)
	{
		RowEjectionSpeed->SetRatio(Grapple->GetEjectionSpeedRatio());
		RowEjectionSpeed->SetValueText(FormatStatPercent(Grapple->GetEjectionSpeedValue(), Grapple->GetEjectionSpeedBase()));
	}

	UpdateRowVisibility();
}

void UGrappleSettingsWidget::OnReelSpeedRatioChanged(float NewRatio)
{
	if (bSyncingFromEquipment)
	{
		return;
	}
	AGrapplingHookTool* Grapple = GetGrapple();
	if (!Grapple || !RowCableLength || !RowEjectionSpeed)
	{
		return;
	}
	Grapple->SetGrappleConfig(NewRatio, RowCableLength->GetRatio(), RowEjectionSpeed->GetRatio());
	if (RowReelSpeed)
	{
		RowReelSpeed->SetValueText(FormatStatPercent(Grapple->GetReelSpeedValue(), Grapple->GetReelSpeedBase()));
	}
}

void UGrappleSettingsWidget::OnMaxCableLengthRatioChanged(float NewRatio)
{
	if (bSyncingFromEquipment)
	{
		return;
	}
	AGrapplingHookTool* Grapple = GetGrapple();
	if (!Grapple || !RowReelSpeed || !RowEjectionSpeed)
	{
		return;
	}
	Grapple->SetGrappleConfig(RowReelSpeed->GetRatio(), NewRatio, RowEjectionSpeed->GetRatio());
	if (RowCableLength)
	{
		RowCableLength->SetValueText(FormatStatPercent(Grapple->GetMaxCableLengthValue(), Grapple->GetMaxCableLengthBase()));
	}
}

void UGrappleSettingsWidget::OnEjectionSpeedRatioChanged(float NewRatio)
{
	if (bSyncingFromEquipment)
	{
		return;
	}
	AGrapplingHookTool* Grapple = GetGrapple();
	if (!Grapple || !RowReelSpeed || !RowCableLength)
	{
		return;
	}
	Grapple->SetGrappleConfig(RowReelSpeed->GetRatio(), RowCableLength->GetRatio(), NewRatio);
	if (RowEjectionSpeed)
	{
		RowEjectionSpeed->SetValueText(FormatStatPercent(Grapple->GetEjectionSpeedValue(), Grapple->GetEjectionSpeedBase()));
	}
}

void UGrappleSettingsWidget::UpdateRowVisibility()
{
	const AGrapplingHookTool* Grapple = GetGrapple();
	if (!Grapple)
	{
		return;
	}

	const bool bReel = Grapple->IsReelSpeedTunable();
	const bool bLength = Grapple->IsMaxCableLengthTunable();
	const bool bEjection = Grapple->IsEjectionSpeedTunable();

	auto SetVis = [](UWidget* Widget, bool bVisible)
	{
		if (Widget)
		{
			Widget->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		}
	};

	SetVis(RowReelSpeed, bReel);
	SetVis(RowCableLength, bLength);
	SetVis(RowEjectionSpeed, bEjection);

	SetVis(Separator_1, bReel && bLength);
	SetVis(Separator_2, bLength && bEjection);

	const bool bAnyTunable = bReel || bLength || bEjection;
	SetVis(NoUpgradesPanel, !bAnyTunable);
}
