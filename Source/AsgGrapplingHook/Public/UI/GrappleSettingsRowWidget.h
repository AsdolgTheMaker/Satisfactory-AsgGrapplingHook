#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GrappleSettingsRowWidget.generated.h"

class USlider;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGrappleRowRatioChanged, float, NewRatio);

UCLASS()
class ASGGRAPPLINGHOOK_API UGrappleSettingsRowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "Grapple|Row")
	FOnGrappleRowRatioChanged OnRatioChanged;

	UFUNCTION(BlueprintCallable, Category = "Grapple|Row")
	void SetRatio(float Ratio);

	UFUNCTION(BlueprintPure, Category = "Grapple|Row")
	float GetRatio() const;

	UFUNCTION(BlueprintCallable, Category = "Grapple|Row")
	void SetValueText(const FText& Text);

	UFUNCTION(BlueprintImplementableEvent, Category = "Grapple|Row")
	void OnRowRefreshed(float Ratio);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION()
	void HandleSliderChanged(float Value);

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USlider> Slider_Reel;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Value_Reel;
};
