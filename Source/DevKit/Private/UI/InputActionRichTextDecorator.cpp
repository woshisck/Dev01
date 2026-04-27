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
		{
			UE_LOG(LogTemp, Warning, TEXT("[InputIcon] ActionName=%s Config=%s"),
				ActionName ? **ActionName : TEXT("NULL"),
				Config ? TEXT("OK") : TEXT("NULL"));
			return nullptr;
		}

		UInputAction* IA = nullptr;

		if (const TSoftObjectPtr<UInputAction>* Found = Config->ActionMap.Find(FName(**ActionName)))
		{
			IA = Found->LoadSynchronous();
		}

		if (!IA && !Config->AutoResolvePath.IsEmpty())
		{
			const FString AssetName = FString::Printf(TEXT("IA_%s"), **ActionName);
			const FString FullPath = FString::Printf(TEXT("%s%s.%s"),
				*Config->AutoResolvePath, *AssetName, *AssetName);
			IA = LoadObject<UInputAction>(nullptr, *FullPath);
		}

		if (!IA)
		{
			UE_LOG(LogTemp, Warning, TEXT("[InputIcon] 未找到 InputAction '%s'（ActionMap=%d 条, AutoPath=%s）"),
				**ActionName, Config->ActionMap.Num(), *Config->AutoResolvePath);
			return nullptr;
		}

		const ULocalPlayer* LP = Owner->GetOwningLocalPlayer();
		if (!LP)
		{
			UE_LOG(LogTemp, Warning, TEXT("[InputIcon] GetOwningLocalPlayer 返回 null"));
			return nullptr;
		}

		const UCommonInputSubsystem* InputSub = UCommonInputSubsystem::Get(LP);
		if (!InputSub)
		{
			UE_LOG(LogTemp, Warning, TEXT("[InputIcon] CommonInputSubsystem 为 null"));
			return nullptr;
		}

		FSlateBrush IconBrush = CommonUI::GetIconForEnhancedInputAction(InputSub, IA);
		if (!IconBrush.HasUObject() && IconBrush.GetResourceObject() == nullptr)
		{
			UE_LOG(LogTemp, Warning, TEXT("[InputIcon] 未获取到图标 — IA=%s InputType=%d"),
				*IA->GetName(), (int32)InputSub->GetCurrentInputType());
			return nullptr;
		}

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
