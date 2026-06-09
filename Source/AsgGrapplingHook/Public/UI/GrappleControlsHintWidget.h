#pragma once

#include "CoreMinimal.h"
#include "UI/FGUserWidget.h"
#include "GrappleControlsHintWidget.generated.h"

class UGrappleKeyHintWidget;
class UHorizontalBox;
class UVerticalBox;
class UTextBlock;

UCLASS()
class ASGGRAPPLINGHOOK_API UGrappleControlsHintWidget : public UFGUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Grapple|Hint")
	void PlayShow();

	UFUNCTION(BlueprintCallable, Category = "Grapple|Hint")
	void PlayHide();

	void NotifyTutorialProgress();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void OnInputDeviceTypeChanged_Implementation(EInputDeviceType deviceType) override;

	void RefreshKeyLabels();
	void RefreshTutorial(bool bAnimate);
	void StartFinale();

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UGrappleKeyHintWidget> HintReelIn;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UGrappleKeyHintWidget> HintReelOut;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UHorizontalBox> RowReelIn;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UHorizontalBox> RowReelOut;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TutorialCounter_ReelIn;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TutorialCounter_ReelOut;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> Rows;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Label_AfterTutorial;

	UPROPERTY(EditDefaultsOnly, Category = "Grapple|Hint")
	float FadeDuration = 0.25f;

	UPROPERTY(EditDefaultsOnly, Category = "Grapple|Hint")
	float CompletedRowOpacity = 0.75f;

	UPROPERTY(EditDefaultsOnly, Category = "Grapple|Hint")
	float AfterTutorialSeconds = 6.f;

private:
	enum class EPhase : uint8
	{
		Active,
		FinaleFadeOut,
		FinaleFadeIn,
		FinaleHold,
		FinaleFadeAway,
		Done
	};
	EPhase Phase = EPhase::Active;

	float CurrentOpacity = 0.f;
	float TargetOpacity = 0.f;
	float RowInOpacity = 1.f;
	float RowInTarget = 1.f;
	float RowOutOpacity = 1.f;
	float RowOutTarget = 1.f;
	float CounterInOpacity = 1.f;
	float CounterInTarget = 1.f;
	float CounterOutOpacity = 1.f;
	float CounterOutTarget = 1.f;
	float FinaleHoldRemaining = 0.f;
};
