#pragma once

#include "CoreMinimal.h"
#include "FGInventorySettingsWidget.h"
#include "GrappleSettingsWidget.generated.h"

class UWidget;
class UGrappleSettingsRowWidget;
class UPanelWidget;
class AGrapplingHookTool;

UCLASS()
class ASGGRAPPLINGHOOK_API UGrappleSettingsWidget : public UFGInventorySettingsWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION()
	void OnReelSpeedRatioChanged(float NewRatio);
	UFUNCTION()
	void OnMaxCableLengthRatioChanged(float NewRatio);
	UFUNCTION()
	void OnEjectionSpeedRatioChanged(float NewRatio);

	void RefreshFromEquipment();
	void UpdateRowVisibility();

	AGrapplingHookTool* GetGrapple() const;

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UGrappleSettingsRowWidget> RowReelSpeed;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UGrappleSettingsRowWidget> RowCableLength;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UGrappleSettingsRowWidget> RowEjectionSpeed;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> NoUpgradesPanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> Separator_1; // Reel | Length
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> Separator_2; // Length | Ejection

private:
	bool bSyncingFromEquipment = false;
};
