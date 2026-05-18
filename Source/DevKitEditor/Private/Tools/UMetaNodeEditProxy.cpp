#include "Tools/UMetaNodeEditProxy.h"

#include "Engine/DataTable.h"
#include "ScopedTransaction.h"

void UMetaNodeEditProxy::LoadFromRow(UDataTable* Table, FName RowName)
{
	SourceTable = Table;
	SourceRowName = RowName;

	if (Table)
	{
		if (const FMetaUpgradeNodeRow* Row = Table->FindRow<FMetaUpgradeNodeRow>(RowName, TEXT("LoadFromRow"), false))
		{
			Data = *Row;
		}
	}
}

#if WITH_EDITOR
void UMetaNodeEditProxy::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (!SourceTable || !SourceRowName.IsValid()) return;

	FScopedTransaction Transaction(NSLOCTEXT("MetaProgressionWorkbench", "EditNode", "Edit Meta Node"));
	SourceTable->Modify();
	SourceTable->AddRow(SourceRowName, Data);
	SourceTable->MarkPackageDirty();
}
#endif
