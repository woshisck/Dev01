#include "YogEntryCustomization.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "PropertyHandle.h"
#include "PropertyCustomizationHelpers.h"

void FYogEntryCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> InStructPropertyHandle,
	FDetailWidgetRow& HeaderRow,
	IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	StructPropertyHandle = InStructPropertyHandle;

	// Gets ItemDataAsset
	TSharedPtr<IPropertyHandle> Key = StructPropertyHandle->GetChildHandle(0);

	// Gets Count
	TSharedPtr<IPropertyHandle> Value = StructPropertyHandle->GetChildHandle(1);

	// Setup in the header row so that we still get the TArray dropdown
	HeaderRow
		.NameContent()
		[
			Key->CreatePropertyValueWidget()
		]
		.ValueContent()
		.MaxDesiredWidth(0.0f)
		[
			Value->CreatePropertyValueWidget()
		];

	// This avoids making duplicate reset boxes
	StructPropertyHandle->MarkResetToDefaultCustomized();
}

void FYogEntryCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> InStructPropertyHandle,
	IDetailChildrenBuilder& StructBuilder,
	IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
}
