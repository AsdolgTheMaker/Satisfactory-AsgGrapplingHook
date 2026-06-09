#include "UI/GrappleControlsHintWidget.h"

#include "UI/GrappleKeyHintWidget.h"
#include "GrappleLocalConfig.h"
#include "Components/TextBlock.h"
#include "Components/HorizontalBox.h"
#include "Components/VerticalBox.h"

void UGrappleControlsHintWidget::NativeConstruct()
{
	Super::NativeConstruct();
	Phase = EPhase::Active;
	CurrentOpacity = 0.f;
	TargetOpacity = 0.f;
	SetRenderOpacity(0.f);
	RefreshKeyLabels();
	RefreshTutorial(false);
}

void UGrappleControlsHintWidget::OnInputDeviceTypeChanged_Implementation(EInputDeviceType deviceType)
{
	RefreshKeyLabels();
}

void UGrappleControlsHintWidget::RefreshKeyLabels()
{
	if (HintReelIn)
	{
		HintReelIn->Refresh();
	}
	if (HintReelOut)
	{
		HintReelOut->Refresh();
	}
}

void UGrappleControlsHintWidget::NotifyTutorialProgress()
{
	RefreshTutorial(true);
}

void UGrappleControlsHintWidget::RefreshTutorial(bool bAnimate)
{
	const UGrappleLocalConfig* Config = UGrappleLocalConfig::Get();
	const int32 ReelInTarget = UGrappleLocalConfig::ControlsHintReelInTarget;
	const int32 ReelOutTarget = UGrappleLocalConfig::ControlsHintReelOutTarget;
	const int32 ReelInCount = FMath::Min(Config->ControlsHintReelSuccesses, ReelInTarget);
	const int32 ReelOutCount = FMath::Min(Config->ControlsHintReelOutSuccesses, ReelOutTarget);

	if (TutorialCounter_ReelIn)
	{
		TutorialCounter_ReelIn->SetText(FText::Format(INVTEXT("{0}/{1}"), FText::AsNumber(ReelInCount), FText::AsNumber(ReelInTarget)));
	}
	if (TutorialCounter_ReelOut)
	{
		TutorialCounter_ReelOut->SetText(FText::Format(INVTEXT("{0}/{1}"), FText::AsNumber(ReelOutCount), FText::AsNumber(ReelOutTarget)));
	}

	const bool bReelInDone = ReelInCount >= ReelInTarget;
	const bool bReelOutDone = ReelOutCount >= ReelOutTarget;

	RowInTarget = bReelInDone ? CompletedRowOpacity : 1.f;
	CounterInTarget = bReelInDone ? 0.f : 1.f;
	RowOutTarget = bReelOutDone ? CompletedRowOpacity : 1.f;
	CounterOutTarget = bReelOutDone ? 0.f : 1.f;

	if (!bAnimate)
	{
		RowInOpacity = RowInTarget;
		CounterInOpacity = CounterInTarget;
		RowOutOpacity = RowOutTarget;
		CounterOutOpacity = CounterOutTarget;
		if (RowReelIn)
		{
			RowReelIn->SetRenderOpacity(RowInOpacity);
		}
		if (RowReelOut)
		{
			RowReelOut->SetRenderOpacity(RowOutOpacity);
		}
		if (TutorialCounter_ReelIn)
		{
			TutorialCounter_ReelIn->SetRenderOpacity(CounterInOpacity);
		}
		if (TutorialCounter_ReelOut)
		{
			TutorialCounter_ReelOut->SetRenderOpacity(CounterOutOpacity);
		}
	}

	if (bAnimate && Phase == EPhase::Active && bReelInDone && bReelOutDone)
	{
		StartFinale();
	}
}

void UGrappleControlsHintWidget::StartFinale()
{
	Phase = EPhase::FinaleFadeOut;
	TargetOpacity = 0.f;
}

void UGrappleControlsHintWidget::PlayShow()
{
	if (Phase != EPhase::Active)
	{
		return;
	}
	RefreshKeyLabels();
	RefreshTutorial(false);
	TargetOpacity = 1.f;
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UGrappleControlsHintWidget::PlayHide()
{
	if (Phase != EPhase::Active)
	{
		return;
	}
	TargetOpacity = 0.f;
}

void UGrappleControlsHintWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	const float ShowRate = FadeDuration > KINDA_SMALL_NUMBER ? 1.f / FadeDuration : 1000.f;
	const float TransitionRate = 1.f / (FadeDuration + 0.5f);
	const float WidgetRate = Phase == EPhase::Active ? ShowRate : TransitionRate;

	CurrentOpacity = FMath::FInterpConstantTo(CurrentOpacity, TargetOpacity, InDeltaTime, WidgetRate);
	SetRenderOpacity(CurrentOpacity);

	RowInOpacity = FMath::FInterpConstantTo(RowInOpacity, RowInTarget, InDeltaTime, TransitionRate);
	if (RowReelIn)
	{
		RowReelIn->SetRenderOpacity(RowInOpacity);
	}
	RowOutOpacity = FMath::FInterpConstantTo(RowOutOpacity, RowOutTarget, InDeltaTime, TransitionRate);
	if (RowReelOut)
	{
		RowReelOut->SetRenderOpacity(RowOutOpacity);
	}
	CounterInOpacity = FMath::FInterpConstantTo(CounterInOpacity, CounterInTarget, InDeltaTime, TransitionRate);
	if (TutorialCounter_ReelIn)
	{
		TutorialCounter_ReelIn->SetRenderOpacity(CounterInOpacity);
	}
	CounterOutOpacity = FMath::FInterpConstantTo(CounterOutOpacity, CounterOutTarget, InDeltaTime, TransitionRate);
	if (TutorialCounter_ReelOut)
	{
		TutorialCounter_ReelOut->SetRenderOpacity(CounterOutOpacity);
	}

	switch (Phase)
	{
	case EPhase::Active:
		if (TargetOpacity <= 0.f && CurrentOpacity <= KINDA_SMALL_NUMBER)
		{
			SetVisibility(ESlateVisibility::Collapsed);
		}
		break;
	case EPhase::FinaleFadeOut:
		if (CurrentOpacity <= KINDA_SMALL_NUMBER)
		{
			if (Rows)
			{
				Rows->SetVisibility(ESlateVisibility::Collapsed);
			}
			if (Label_AfterTutorial)
			{
				Label_AfterTutorial->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			}
			TargetOpacity = 1.f;
			Phase = EPhase::FinaleFadeIn;
		}
		break;
	case EPhase::FinaleFadeIn:
		if (CurrentOpacity >= 1.f - KINDA_SMALL_NUMBER)
		{
			FinaleHoldRemaining = AfterTutorialSeconds;
			Phase = EPhase::FinaleHold;
		}
		break;
	case EPhase::FinaleHold:
		FinaleHoldRemaining -= InDeltaTime;
		if (FinaleHoldRemaining <= 0.f)
		{
			TargetOpacity = 0.f;
			Phase = EPhase::FinaleFadeAway;
		}
		break;
	case EPhase::FinaleFadeAway:
		if (CurrentOpacity <= KINDA_SMALL_NUMBER)
		{
			SetVisibility(ESlateVisibility::Collapsed);
			Phase = EPhase::Done;
		}
		break;
	default:
		break;
	}
}
