#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CelesLightEditorLibrary.generated.h"

class ACelesLightCaptureBox;
class ACelesPointLight;
class AStylizedEmissiveLight;
class AStylizedCharacterLookVolume;
class UStylizedEmissiveModelLibrary;

UCLASS()
class CELESLIGHTEDITOR_API UCelesLightEditorLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Celes Light|Tools")
	static int32 ManualUpdateCelesLights(UWorld* World = nullptr);

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Celes Light|Tools")
	static ACelesLightCaptureBox* CreateCelesLightCaptureBox(UWorld* World = nullptr);

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Celes Light|Tools")
	static ACelesPointLight* CreateCelesPointLight(UWorld* World = nullptr);

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Celes Light|Tools")
	static AStylizedEmissiveLight* CreateStylizedEmissiveLight(UWorld* World = nullptr);

	/** Returns the shared preset-library backend, creating and seeding it when needed. */
	static UStylizedEmissiveModelLibrary* GetOrCreateStylizedEmissiveModelLibrary();

	/** Legacy/debug entry that opens the raw backend asset. Artists use the YogTool panel. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Celes Light|Tools")
	static UStylizedEmissiveModelLibrary* OpenOrCreateStylizedEmissiveModelLibrary();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Celes Light|Tools")
	static AStylizedCharacterLookVolume* CreateStylizedCharacterLookVolume(UWorld* World = nullptr);
};
