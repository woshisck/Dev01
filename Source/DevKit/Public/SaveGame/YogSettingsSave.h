#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "YogSettingsSave.generated.h"

// ============================================================
//  全局设置存档（跨槽位，独立文件 "Settings" 槽位）
//  不绑定任何槽位，所有槽位共享同一份设置。
// ============================================================
UCLASS(BlueprintType)
class DEVKIT_API UYogSettingsSave : public USaveGame
{
	GENERATED_BODY()

public:

	// ── 音频 ────────────────────────────────────────────────────
	UPROPERTY(BlueprintReadWrite, Category = "Settings|Audio")
	float MasterVolume = 1.f;

	UPROPERTY(BlueprintReadWrite, Category = "Settings|Audio")
	float MusicVolume = 0.8f;

	UPROPERTY(BlueprintReadWrite, Category = "Settings|Audio")
	float SFXVolume = 1.f;

	// ── 上次选择的槽位（启动时自动高亮）────────────────────────
	UPROPERTY(BlueprintReadWrite, Category = "Settings|Slot")
	int32 LastActiveSlot = 0;
};
