#include "Story/Flow/Nodes/SNode_SetRoomRewardOverride.h"

#include "Engine/Texture2D.h"
#include "GameModes/YogGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "System/YogGameInstanceBase.h"

namespace
{
FString DescribeEnumValueForRewardDebug(const UEnum* Enum, int64 Value)
{
	return Enum ? Enum->GetNameStringByValue(Value) : FString::Printf(TEXT("%lld"), Value);
}

FString DescribeLootOptionsForRewardDebug(const TArray<FLootOption>& Options)
{
	if (Options.IsEmpty())
	{
		return TEXT("Count=0 []");
	}

	TArray<FString> Parts;
	Parts.Reserve(Options.Num());
	for (int32 Index = 0; Index < Options.Num(); ++Index)
	{
		const FLootOption& Option = Options[Index];
		Parts.Add(FString::Printf(
			TEXT("#%d{Type=%s,Amount=%d,Display=%s,Rune=%s,Icon=%s,Meta=%s}"),
			Index,
			*DescribeEnumValueForRewardDebug(StaticEnum<ELootType>(), static_cast<int64>(Option.LootType)),
			Option.Amount,
			*Option.DisplayName.ToString(),
			*GetNameSafe(Option.RuneAsset.Get()),
			*GetNameSafe(Option.Icon.Get()),
			*Option.MetaCurrencyTag.ToString()));
	}

	return FString::Printf(TEXT("Count=%d [%s]"), Options.Num(), *FString::Join(Parts, TEXT("; ")));
}
}

USNode_SetRoomRewardOverride::USNode_SetRoomRewardOverride(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("StoryDirector|Level");
#endif
	InputPins = { FFlowPin(TEXT("In")) };
	OutputPins = { FFlowPin(TEXT("Out")) };
}

void USNode_SetRoomRewardOverride::ExecuteInput(const FName& PinName)
{
	UWorld* World = GetWorld();
	AYogGameMode* GM = World ? Cast<AYogGameMode>(UGameplayStatics::GetGameMode(World)) : nullptr;
	UYogGameInstanceBase* GI = World ? Cast<UYogGameInstanceBase>(World->GetGameInstance()) : nullptr;

	UE_LOG(LogTemp, Log,
		TEXT("[StoryRewardDebug] FlowNode SetRoomRewardOverride Execute Node=%s Pin=%s World=%s GM=%s GI=%s Target=%s Clear=%d Options=%s"),
		*GetName(),
		*PinName.ToString(),
		*GetNameSafe(World),
		*GetNameSafe(GM),
		*GetNameSafe(GI),
		*DescribeEnumValueForRewardDebug(StaticEnum<EStoryRewardOverrideTarget>(), static_cast<int64>(OverrideTarget)),
		bClearOverride ? 1 : 0,
		*DescribeLootOptionsForRewardDebug(LootOptions));

	ApplyRewardOverride(GM, GI);

	TriggerOutput(TEXT("Out"), true);
}

bool USNode_SetRoomRewardOverride::ApplyRewardOverride(AYogGameMode* GameMode, UYogGameInstanceBase* GameInstance) const
{
	UE_LOG(LogTemp, Log,
		TEXT("[StoryRewardDebug] FlowNode SetRoomRewardOverride Apply Node=%s GM=%s GI=%s Target=%s Clear=%d Options=%s"),
		*GetName(),
		*GetNameSafe(GameMode),
		*GetNameSafe(GameInstance),
		*DescribeEnumValueForRewardDebug(StaticEnum<EStoryRewardOverrideTarget>(), static_cast<int64>(OverrideTarget)),
		bClearOverride ? 1 : 0,
		*DescribeLootOptionsForRewardDebug(LootOptions));

	if (!bClearOverride && LootOptions.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[StoryRewardDebug] FlowNode SetRoomRewardOverride skipped: LootOptions is empty and bClearOverride is false."));
		return false;
	}

	if (OverrideTarget == EStoryRewardOverrideTarget::NextRoom)
	{
		if (!GameInstance)
		{
			UE_LOG(LogTemp, Warning, TEXT("[StoryRewardDebug] FlowNode SetRoomRewardOverride skipped: GameInstance not found for NextRoom reward override."));
			return false;
		}

		if (bClearOverride)
		{
			UE_LOG(LogTemp, Log, TEXT("[StoryRewardDebug] FlowNode clearing NextRoom pending reward override."));
			GameInstance->ClearPendingRoomRewardOptionsOverride();
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("[StoryRewardDebug] FlowNode writing NextRoom pending reward override."));
			GameInstance->SetPendingRoomRewardOptionsOverride(LootOptions);
		}

		return true;
	}

	if (!GameMode)
	{
		UE_LOG(LogTemp, Warning, TEXT("[StoryRewardDebug] FlowNode SetRoomRewardOverride skipped: AYogGameMode not found for CurrentRoom reward override."));
		return false;
	}

	if (bClearOverride)
	{
		UE_LOG(LogTemp, Log, TEXT("[StoryRewardDebug] FlowNode clearing CurrentRoom reward override."));
		GameMode->ClearRoomRewardOptionsOverride();
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("[StoryRewardDebug] FlowNode writing CurrentRoom reward override."));
		GameMode->SetRoomRewardOptionsOverride(LootOptions);
	}

	return true;
}
