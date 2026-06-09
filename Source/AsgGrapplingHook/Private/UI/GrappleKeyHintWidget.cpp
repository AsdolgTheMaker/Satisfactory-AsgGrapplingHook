#include "UI/GrappleKeyHintWidget.h"

#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Border.h"
#include "FGInputLibrary.h"
#include "FGGameInstance.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/PlayerController.h"
#include "Engine/LocalPlayer.h"
#include "Engine/Texture2D.h"

void UGrappleKeyHintWidget::NativeConstruct()
{
	Super::NativeConstruct();
	Refresh();
}

void UGrappleKeyHintWidget::Refresh()
{
	bool bGamepad = false;
	if (const UFGGameInstance* GameInstance = Cast<UFGGameInstance>(GetGameInstance()))
	{
		bGamepad = GameInstance->GetActiveInputDeviceType() == EInputDeviceType::Controller;
	}

	FKey Chosen;
	if (Action)
	{
		if (const APlayerController* PC = GetOwningPlayer())
		{
			if (const ULocalPlayer* LP = PC->GetLocalPlayer())
			{
				if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
				{
					const TArray<FKey> Keys = Subsystem->QueryKeysMappedToAction(Action);
					for (const FKey& Key : Keys)
					{
						if (Key.IsGamepadKey() == bGamepad)
						{
							Chosen = Key;
							break;
						}
					}
					if (!Chosen.IsValid() && Keys.Num() > 0)
					{
						Chosen = Keys[0];
					}
				}
			}
		}
	}

	UTexture2D* Texture = Chosen.IsValid() ? UFGInputLibrary::GetKeyTexture2DFromKey(Chosen) : nullptr;
	const bool bUseIcon = Texture != nullptr;

	if (KeyIcon)
	{
		if (bUseIcon)
		{
			KeyIcon->SetBrushFromTexture(Texture, false);
			const float Width = Texture->GetSurfaceWidth();
			const float Height = Texture->GetSurfaceHeight();
			KeyIcon->SetDesiredSizeOverride(FVector2D(Height > 0.f ? IconHeight * (Width / Height) : IconHeight, IconHeight));
		}
		KeyIcon->SetVisibility(bUseIcon ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (KeyText && !bUseIcon)
	{
		KeyText->SetText(Chosen.IsValid() ? UFGInputLibrary::GetAbbreviatedKeyName(Chosen) : FText::GetEmpty());
	}

	UWidget* TextElement = KeyTextBox ? static_cast<UWidget*>(KeyTextBox) : static_cast<UWidget*>(KeyText);
	if (TextElement)
	{
		TextElement->SetVisibility(bUseIcon ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
}
