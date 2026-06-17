#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CelesLightEditorLibrary.generated.h"

class ACelesLightCaptureBox;

UCLASS()
class CELESLIGHTEDITOR_API UCelesLightEditorLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Celes Light|赛璐璐灯光")
	static int32 ManualUpdateCelesLights(UWorld* World = nullptr);

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Celes Light|赛璐璐灯光")
	static ACelesLightCaptureBox* CreateCelesLightCaptureBox(UWorld* World = nullptr);
};
