#include "UI/DialogContentDA.h"

const TArray<FTutorialPage>* UDialogContentDA::FindPages(FName EventID) const
{
	for (const FDialogContent& Entry : Contents)
	{
		if (Entry.EventID == EventID)
		{
			return &Entry.Pages;
		}
	}
	return nullptr;
}
