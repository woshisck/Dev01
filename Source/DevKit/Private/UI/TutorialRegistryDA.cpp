#include "UI/TutorialRegistryDA.h"
#include "UI/DialogContentDA.h"

const TArray<FTutorialPage>* UTutorialRegistryDA::FindPages(FName EventID) const
{
	const TObjectPtr<UDialogContentDA>* Found = Entries.Find(EventID);
	if (!Found || !*Found) return nullptr;
	return &(*Found)->Pages;
}
