#pragma once
#include "CoreMinimal.h"

class YOGVISIONEDITOR_API FYogVisionEditorService
{
public:
    static bool BuildAssets(FText& Message);
    static bool CreateValidationStage(FText& Message);
    static bool AuditScene(FText& Message);
    static bool CaptureCurrentView(const FString& Name);
};
