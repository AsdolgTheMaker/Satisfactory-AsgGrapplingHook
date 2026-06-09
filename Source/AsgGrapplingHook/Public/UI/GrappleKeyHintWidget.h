#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GrappleKeyHintWidget.generated.h"

class UTextBlock;
class UImage;
class UBorder;
class UInputAction;

UCLASS()
class ASGGRAPPLINGHOOK_API UGrappleKeyHintWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Grapple|Hint")
	void Refresh();

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(EditAnywhere, Category = "Grapple|Hint")
	TObjectPtr<UInputAction> Action;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> KeyText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> KeyTextBox;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> KeyIcon;

	UPROPERTY(EditDefaultsOnly, Category = "Grapple|Hint")
	float IconHeight = 28.f;
};
