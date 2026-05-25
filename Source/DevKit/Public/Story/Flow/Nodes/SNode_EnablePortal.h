#pragma once

#include "CoreMinimal.h"
#include "Story/Flow/Nodes/SNode_Base.h"
#include "SNode_EnablePortal.generated.h"

class URoomDataAsset;

/**
 * 按 Tag 或 Index 找到场景中的 APortal，调用 EnablePortal()（表现）再调用 Open()（功能）。
 * SelectedLevel / SelectedRoom 留空时仅执行 EnablePortal，跳过 Open。
 * Tag 不为空时优先按 Tag 匹配；两者都为空/默认时遍历所有门。
 */
UCLASS(meta = (DisplayName = "Enable Portal"))
class DEVKIT_API USNode_EnablePortal : public USNode_Base
{
	GENERATED_UCLASS_BODY()

public:
	/** Actor Tag，优先匹配。为空时使用 PortalIndex 或匹配所有门。 */
	UPROPERTY(EditAnywhere, Category = "Portal")
	FName PortalActorTag;

	/** APortal::Index 匹配，-1 表示忽略（仅在 PortalActorTag 为空时生效）。 */
	UPROPERTY(EditAnywhere, Category = "Portal", meta = (ClampMin = "-1"))
	int32 PortalIndex = -1;

	/** 传给 Open() 的目标关卡名。留空则跳过 Open()。 */
	UPROPERTY(EditAnywhere, Category = "Portal")
	FName SelectedLevel;

	/** 传给 Open() 的目标房间 DA。留空则跳过 Open()。 */
	UPROPERTY(EditAnywhere, Category = "Portal")
	TObjectPtr<URoomDataAsset> SelectedRoom;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
};
