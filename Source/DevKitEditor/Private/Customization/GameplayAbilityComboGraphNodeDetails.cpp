#include "Customization/GameplayAbilityComboGraphNodeDetails.h"

#include "AbilitySystem/Abilities/GA_MeleeAttack.h"
#include "AbilitySystem/Abilities/GA_PlayerDash.h"
#include "AbilitySystem/Abilities/GA_RangeAttack.h"
#include "AbilitySystem/Abilities/GA_WeaponSkill.h"
#include "ClassViewerFilter.h"
#include "ClassViewerModule.h"
#include "Data/GameplayAbilityComboGraph.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "Framework/Application/SlateApplication.h"
#include "Modules/ModuleManager.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "GameplayAbilityComboGraphNodeDetails"

namespace
{
	// Restricts the GameplayAbilityClass picker to the supported player ability families.
	class FComboAbilityClassFilter : public IClassViewerFilter
	{
	public:
		bool IsClassAllowed(
			const FClassViewerInitializationOptions& InInitOptions,
			const UClass* InClass,
			TSharedRef<FClassViewerFilterFuncs> InFilterFuncs) override
		{
			return InClass
				&& (InClass->IsChildOf<UGA_MeleeAttack>()
					|| InClass->IsChildOf<UGA_RangeAttack>()
					|| InClass->IsChildOf<UGA_WeaponSkill>()
					|| InClass->IsChildOf<UGA_PlayerDash>());
		}

		bool IsUnloadedClassAllowed(
			const FClassViewerInitializationOptions& InInitOptions,
			const TSharedRef<const IUnloadedBlueprintData> InUnloadedClassData,
			TSharedRef<FClassViewerFilterFuncs> InFilterFuncs) override
		{
			return InUnloadedClassData->IsChildOf(UGA_MeleeAttack::StaticClass())
				|| InUnloadedClassData->IsChildOf(UGA_RangeAttack::StaticClass())
				|| InUnloadedClassData->IsChildOf(UGA_WeaponSkill::StaticClass())
				|| InUnloadedClassData->IsChildOf(UGA_PlayerDash::StaticClass());
		}
	};
}

TSharedRef<IDetailCustomization> FGameplayAbilityComboGraphNodeDetails::MakeInstance()
{
	return MakeShared<FGameplayAbilityComboGraphNodeDetails>();
}

void FGameplayAbilityComboGraphNodeDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	TArray<TWeakObjectPtr<UObject>> Objects;
	DetailBuilder.GetObjectsBeingCustomized(Objects);
	EditingNode = Objects.Num() > 0 ? Cast<UGameplayAbilityComboGraphNode>(Objects[0].Get()) : nullptr;

	// Replace the GameplayAbilityClass picker with a filtered one that only shows
	// GA_MeleeAttack, GA_PlayerDash, and their subclasses.
	TSharedRef<IPropertyHandle> AbilityClassHandle = DetailBuilder.GetProperty(
		GET_MEMBER_NAME_CHECKED(UGameplayAbilityComboGraphNode, GameplayAbilityClass),
		UGameplayAbilityComboGraphNode::StaticClass());

	if (AbilityClassHandle->IsValidHandle())
	{
		IDetailCategoryBuilder& AbilityCategory = DetailBuilder.EditCategory(TEXT("Ability"));
		AbilityCategory.AddProperty(AbilityClassHandle)
			.CustomWidget()
			.NameContent()
			[
				AbilityClassHandle->CreatePropertyNameWidget()
			]
			.ValueContent()
			.MinDesiredWidth(200.f)
			[
				SNew(SComboButton)
				.OnGetMenuContent_Lambda([AbilityClassHandle]() -> TSharedRef<SWidget>
				{
					FClassViewerInitializationOptions Options;
					Options.Mode = EClassViewerMode::ClassPicker;
					Options.bShowUnloadedBlueprints = true;
					Options.bShowNoneOption = true;
					Options.ClassFilters.Add(MakeShared<FComboAbilityClassFilter>());

					return FModuleManager::LoadModuleChecked<FClassViewerModule>("ClassViewer")
						.CreateClassViewer(Options,
							FOnClassPicked::CreateLambda([AbilityClassHandle](UClass* PickedClass)
							{
								if (PickedClass)
								{
									AbilityClassHandle->SetValueFromFormattedString(PickedClass->GetPathName());
								}
								else
								{
									AbilityClassHandle->SetValueFromFormattedString(TEXT("None"));
								}
								FSlateApplication::Get().DismissAllMenus();
							}));
				})
				.ContentPadding(FMargin(2.f, 2.f))
				.ButtonContent()
				[
					SNew(STextBlock)
					.Text_Lambda([AbilityClassHandle]() -> FText
					{
						UObject* Val = nullptr;
						AbilityClassHandle->GetValue(Val);
						const UClass* Class = Cast<UClass>(Val);
						return Class
							? FText::FromString(Class->GetName())
							: LOCTEXT("None", "None");
					})
					.Font(IDetailLayoutBuilder::GetDetailFont())
				]
			];
	}

	IDetailCategoryBuilder& Category = DetailBuilder.EditCategory(TEXT("Combo Manager"));
	Category.AddCustomRow(LOCTEXT("ComboManagerNodeClassFilter", "Node Class"))
		.NameContent()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("NodeClassLabel", "Node Class"))
			.Font(IDetailLayoutBuilder::GetDetailFont())
		]
		.ValueContent()
		.MinDesiredWidth(240.f)
		[
			SNew(STextBlock)
			.Text(this, &FGameplayAbilityComboGraphNodeDetails::GetNodeClassText)
			.ToolTipText(this, &FGameplayAbilityComboGraphNodeDetails::GetNodeClassTooltipText)
			.Font(IDetailLayoutBuilder::GetDetailFont())
		];

	Category.AddCustomRow(LOCTEXT("ComboManagerSummaryFilter", "Combo Manager"))
		.WholeRowContent()
		[
			SNew(STextBlock)
			.Text(this, &FGameplayAbilityComboGraphNodeDetails::GetSummaryText)
			.AutoWrapText(true)
		];
}

FText FGameplayAbilityComboGraphNodeDetails::GetSummaryText() const
{
	const UGameplayAbilityComboGraphNode* Node = EditingNode.Get();
	if (!Node)
	{
		return LOCTEXT("NoNode", "No combo node selected.");
	}

	return FText::FromString(FString::Printf(
		TEXT("RootInput: %s | Montage: %s | ComboWindow: %d-%d/%d"),
		StaticEnum<EYogComboGraphInputAction>()
			? *StaticEnum<EYogComboGraphInputAction>()->GetNameStringByValue(static_cast<int64>(Node->RootInputAction))
			: TEXT("-"),
		*GetNameSafe(Node->Montage.Get()),
		Node->ComboWindowStartFrame,
		Node->ComboWindowEndFrame,
		Node->TotalFrames));
}

FText FGameplayAbilityComboGraphNodeDetails::GetNodeClassText() const
{
	const UGameplayAbilityComboGraphNode* Node = EditingNode.Get();
	const UClass* NodeClass = Node ? Node->GetClass() : nullptr;
	return NodeClass
		? FText::FromString(NodeClass->GetName())
		: LOCTEXT("NoNodeClass", "None");
}

FText FGameplayAbilityComboGraphNodeDetails::GetNodeClassTooltipText() const
{
	const UGameplayAbilityComboGraphNode* Node = EditingNode.Get();
	const UClass* NodeClass = Node ? Node->GetClass() : nullptr;
	return NodeClass
		? FText::FromString(NodeClass->GetPathName())
		: LOCTEXT("NoNodeClassTooltip", "No combo node selected.");
}

#undef LOCTEXT_NAMESPACE
