#include "UI/InputActionRichTextDecorator.h"
#include "CommonInputSubsystem.h"
#include "CommonUITypes.h"
#include "InputAction.h"
#include "Components/RichTextBlock.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"

class FInputActionTextDecorator : public FRichTextDecorator
{
public:
	FInputActionTextDecorator(URichTextBlock* InOwner, UInputActionRichTextDecorator* InConfig)
		: FRichTextDecorator(InOwner)
		, Config(InConfig)
	{
	}

	virtual bool Supports(const FTextRunParseResults& RunParseResult, const FString& Text) const override
	{
		return RunParseResult.Name == TEXT("input");
	}

protected:
	virtual TSharedPtr<SWidget> CreateDecoratorWidget(
		const FTextRunInfo& RunInfo,
		const FTextBlockStyle& DefaultTextStyle) const override
	{
		const FString* ActionName = RunInfo.MetaData.Find(TEXT("action"));
		if (!ActionName || !Config)
			return nullptr;

		const TSoftObjectPtr<UInputAction>* Found = Config->ActionMap.Find(FName(**ActionName));
		if (!Found)
			return nullptr;

		UInputAction* IA = Found->LoadSynchronous();
		if (!IA)
			return nullptr;

		const ULocalPlayer* LP = Owner->GetOwningLocalPlayer();
		if (!LP)
			return nullptr;

		const UCommonInputSubsystem* InputSub = UCommonInputSubsystem::Get(LP);
		if (!InputSub)
			return nullptr;

		FSlateBrush IconBrush = CommonUI::GetIconForEnhancedInputAction(InputSub, IA);
		if (!IconBrush.HasUObject() && IconBrush.GetResourceObject() == nullptr)
			return nullptr;

		return SNew(SBox)
			.WidthOverride(Config->IconSize.X)
			.HeightOverride(Config->IconSize.Y)
			.VAlign(VAlign_Center)
			[
				SNew(SImage)
				.Image(new FSlateBrush(IconBrush))
			];
	}

private:
	UInputActionRichTextDecorator* Config;
};

TSharedPtr<ITextDecorator> UInputActionRichTextDecorator::CreateDecorator(URichTextBlock* InOwner)
{
	return MakeShareable(new FInputActionTextDecorator(InOwner, this));
}
