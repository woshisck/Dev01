#include "BuffFlow/Nodes/BFNode_GrantGA.h"
#include "AbilitySystemComponent.h"
#include "Types/FlowDataPinResults.h"

UBFNode_GrantGA::UBFNode_GrantGA(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("BuffFlow|Effect");
#endif
	OutputPins = { FFlowPin(TEXT("Out")), FFlowPin(TEXT("Failed")) };
	AbilityLevel = FFlowDataPinInputProperty_Int32(1);
}

void UBFNode_GrantGA::ExecuteInput(const FName& PinName)
{
	auto FailOut = [this]()
	{
		bGAGranted = FFlowDataPinOutputProperty_Bool(false);
		GALevel    = FFlowDataPinOutputProperty_Int32(0);
		TriggerOutput(TEXT("Failed"), true);
	};

	if (!AbilityClass)     { FailOut(); return; }

	AActor* TargetActor = ResolveTarget(Target);
	if (!TargetActor)      { FailOut(); return; }

	UAbilitySystemComponent* ASC = TargetActor->FindComponentByClass<UAbilitySystemComponent>();
	if (!ASC)
	{
		FailOut();
		return;
	}

	// 优先从连接的数据引脚读取 AbilityLevel，无连接则使用节点上填写的值
	FFlowDataPinResult_Int LevelResult = TryResolveDataPinAsInt(GET_MEMBER_NAME_CHECKED(UBFNode_GrantGA, AbilityLevel));
	const int32 ResolvedLevel = (LevelResult.Result == EFlowDataPinResolveResult::Success) ? static_cast<int32>(LevelResult.Value) : AbilityLevel.Value;

	FGameplayAbilitySpec Spec(AbilityClass, ResolvedLevel);
	GrantedHandle = ASC->GiveAbility(Spec);

	if (!GrantedHandle.IsValid())
	{
		bGAGranted = FFlowDataPinOutputProperty_Bool(false);
		GALevel    = FFlowDataPinOutputProperty_Int32(0);
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	bGAGranted = FFlowDataPinOutputProperty_Bool(true);
	GALevel    = FFlowDataPinOutputProperty_Int32(ResolvedLevel);
	GrantedASC = ASC;
	TriggerOutput(TEXT("Out"), true);
}

void UBFNode_GrantGA::Cleanup()
{
	// FA 停止时自动撤销 GA（符文卸下、BuffFlow 终止、场景切换等均触发）
	if (GrantedHandle.IsValid() && GrantedASC.IsValid())
	{
		GrantedASC->ClearAbility(GrantedHandle);
	}
	GrantedHandle = FGameplayAbilitySpecHandle();
	GrantedASC.Reset();

	Super::Cleanup();
}
