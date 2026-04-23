#include "BuffFlow/Nodes/BFNode_GrantGA.h"
#include "AbilitySystemComponent.h"
#include "Types/FlowDataPinResults.h"

UBFNode_GrantGA::UBFNode_GrantGA(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("BuffFlow|Effect");
#endif
	InputPins  = { FFlowPin(TEXT("Grant")), FFlowPin(TEXT("Revoke")) };
	OutputPins = { FFlowPin(TEXT("Out")), FFlowPin(TEXT("Failed")), FFlowPin(TEXT("Revoked")) };
	AbilityLevel = FFlowDataPinInputProperty_Int32(1);
}

void UBFNode_GrantGA::ExecuteInput(const FName& PinName)
{
	// ── Revoke 引脚：撤销已授予的 GA ─────────────────────────────────
	if (PinName == TEXT("Revoke"))
	{
		if (GrantedHandle.IsValid() && GrantedASC.IsValid())
		{
			GrantedASC->ClearAbility(GrantedHandle);
			UE_LOG(LogTemp, Verbose, TEXT("[BFNode_GrantGA] Revoked %s"), *GetNameSafe(AbilityClass));
		}
		GrantedHandle = FGameplayAbilitySpecHandle();
		GrantedASC.Reset();
		TriggerOutput(TEXT("Revoked"), true);
		return;
	}

	// ── Grant 引脚（同时兼容旧版 "In" 引脚名）────────────────────────
	auto FailOut = [this]()
	{
		bGAGranted = FFlowDataPinOutputProperty_Bool(false);
		GALevel    = FFlowDataPinOutputProperty_Int32(0);
		TriggerOutput(TEXT("Failed"), true);
	};

	UE_LOG(LogTemp, Warning, TEXT("[BFNode_GrantGA] ExecuteInput | AbilityClass=%s | TargetActor=%s"),
		*GetNameSafe(AbilityClass), *GetNameSafe(ResolveTarget(Target)));

	if (!AbilityClass)     { UE_LOG(LogTemp, Warning, TEXT("[BFNode_GrantGA] SKIP: AbilityClass is null")); FailOut(); return; }

	AActor* TargetActor = ResolveTarget(Target);
	if (!TargetActor)      { UE_LOG(LogTemp, Warning, TEXT("[BFNode_GrantGA] SKIP: TargetActor is null")); FailOut(); return; }

	UAbilitySystemComponent* ASC = TargetActor->FindComponentByClass<UAbilitySystemComponent>();
	if (!ASC)
	{
		FailOut();
		return;
	}

	// 优先从连接的数据引脚读取 AbilityLevel，无连接则使用节点上填写的值
	FFlowDataPinResult_Int LevelResult = TryResolveDataPinAsInt(GET_MEMBER_NAME_CHECKED(UBFNode_GrantGA, AbilityLevel));
	const int32 ResolvedLevel = (LevelResult.Result == EFlowDataPinResolveResult::Success) ? static_cast<int32>(LevelResult.Value) : AbilityLevel.Value;

	// 幂等检查：若已授予同类 GA，直接复用，避免循环 FA 里重复 GiveAbility 导致 Spec 泄漏
	if (GrantedHandle.IsValid() && GrantedASC == ASC)
	{
		if (ASC->FindAbilitySpecFromHandle(GrantedHandle))
		{
			UE_LOG(LogTemp, Verbose, TEXT("[BFNode_GrantGA] Already granted %s → %s, reusing"),
				*GetNameSafe(AbilityClass), *GetNameSafe(TargetActor));
			bGAGranted = FFlowDataPinOutputProperty_Bool(true);
			GALevel    = FFlowDataPinOutputProperty_Int32(ResolvedLevel);
			TriggerOutput(TEXT("Out"), true);
			return;
		}
		// Handle 失效（目标死亡重生等），重新授予
		GrantedHandle = FGameplayAbilitySpecHandle();
	}

	FGameplayAbilitySpec Spec(AbilityClass, ResolvedLevel);
	GrantedHandle = ASC->GiveAbility(Spec);

	if (!GrantedHandle.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[BFNode_GrantGA] FAILED to grant %s → %s"),
			*GetNameSafe(AbilityClass), *GetNameSafe(TargetActor));
		bGAGranted = FFlowDataPinOutputProperty_Bool(false);
		GALevel    = FFlowDataPinOutputProperty_Int32(0);
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[BFNode_GrantGA] Granted %s → %s"),
		*GetNameSafe(AbilityClass), *GetNameSafe(TargetActor));
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
