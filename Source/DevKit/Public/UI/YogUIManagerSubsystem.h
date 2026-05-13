#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "UI/YogUIRegistry.h"
#include "YogUIManagerSubsystem.generated.h"

class UUserWidget;
class UCommonActivatableWidget;
class UYogUIRegistry;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTopLayerChanged, EYogUILayer, NewTopLayer);
DECLARE_DYNAMIC_DELEGATE_TwoParams(FOnAsyncScreenReady, EYogUIScreenId, ScreenId, UCommonActivatableWidget*, Widget);

UCLASS()
class DEVKIT_API UYogUIManagerSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintPure, Category = "UI")
	UYogUIRegistry* GetRegistry() const;

	UFUNCTION(BlueprintCallable, Category = "UI")
	TSubclassOf<UUserWidget> GetWidgetClass(EYogUIScreenId ScreenId) const;

	UFUNCTION(BlueprintCallable, Category = "UI")
	int32 GetZOrder(EYogUIScreenId ScreenId, int32 FallbackZOrder = 0) const;

	template<typename WidgetT>
	TSubclassOf<WidgetT> GetTypedWidgetClass(EYogUIScreenId ScreenId) const
	{
		TSubclassOf<UUserWidget> WidgetClass = GetWidgetClass(ScreenId);
		UClass* ResolvedClass = WidgetClass.Get();
		if (ResolvedClass && ResolvedClass->IsChildOf(WidgetT::StaticClass()))
		{
			return TSubclassOf<WidgetT>(ResolvedClass);
		}

		return TSubclassOf<WidgetT>();
	}

	// ─── Lifecycle ───────────────────────────────────────────
	//
	// EnsureWidget       : create + AddToViewport if not yet present. Idempotent. Works for any registered ScreenId.
	// PushScreen         : EnsureWidget + ActivateWidget. Returns nullptr if the resolved class is not a UCommonActivatableWidget.
	// PushScreenAsync    : RequestAsyncLoad on the soft class, then PushScreen + callback. For screens not preloaded by bCreateOnHUDStart.
	// PopScreen          : DeactivateWidget on the cached instance (no-op if not active).
	// GetWidget          : the cached instance (or nullptr).
	// IsScreenActive     : true iff cached instance is a UCommonActivatableWidget currently activated.
	//
	// All methods route widget Outer/AddToViewport through the LocalPlayer's PlayerController.

	UFUNCTION(BlueprintCallable, Category = "UI")
	UUserWidget* EnsureWidget(EYogUIScreenId ScreenId);

	UCommonActivatableWidget* PushScreen(EYogUIScreenId ScreenId);

	UFUNCTION(BlueprintCallable, Category = "UI", meta = (AutoCreateRefTerm = "OnReady"))
	void PushScreenAsync(EYogUIScreenId ScreenId, const FOnAsyncScreenReady& OnReady);

	UFUNCTION(BlueprintCallable, Category = "UI")
	void PopScreen(EYogUIScreenId ScreenId);

	UFUNCTION(BlueprintCallable, Category = "UI")
	UUserWidget* GetWidget(EYogUIScreenId ScreenId) const;

	UFUNCTION(BlueprintCallable, Category = "UI")
	bool IsScreenActive(EYogUIScreenId ScreenId) const;

	template<typename WidgetT>
	WidgetT* GetTypedWidget(EYogUIScreenId ScreenId) const
	{
		return Cast<WidgetT>(GetWidget(ScreenId));
	}

	/** Iterate registry; for every entry with bCreateOnHUDStart=true, EnsureWidget(ScreenId). */
	UFUNCTION(BlueprintCallable, Category = "UI")
	void CreateAutoStartWidgets();

	// ─── Layer / Input ───────────────────────────────────────
	//
	// 当 ActivatedScreens 集合发生变化时，Subsystem 会重新计算当前的 Top Layer：
	//   - Game   : 没有任何 Menu/Modal 处于激活态
	//   - Menu   : 至少有一个 Menu 激活，但没有 Modal
	//   - Modal  : 至少有一个 Modal 激活（最优先）
	//
	// 若 bAutoManageInputMode = true：
	//   - Top == Game           → SetInputMode(GameOnly)
	//   - Top == Menu / Modal   → SetInputMode(GameAndUI)，并把焦点设到顶层 widget 的 GetDesiredFocusTarget()
	//
	// 这条链就是用来修死亡复活后 D-pad 没焦点的：复活时 GA_Dead 结束，
	// 顶层 Menu 取消激活，Subsystem 自动切回 GameOnly 并把焦点清掉。

	UFUNCTION(BlueprintPure, Category = "UI|Layer")
	EYogUILayer GetTopActivatedLayer() const { return TopActivatedLayer; }

	UPROPERTY(BlueprintAssignable, Category = "UI|Layer")
	FOnTopLayerChanged OnTopLayerChanged;

	/** 由 Subsystem 自动接管 InputMode / Focus 切换。关掉它表示项目自己管。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Layer")
	bool bAutoManageInputMode = true;

private:
	APlayerController* GetOwningPlayerController() const;

	void HandleScreenActivated(EYogUIScreenId ScreenId);
	void HandleScreenDeactivated(EYogUIScreenId ScreenId);
	void RecomputeTopLayer();
	void ApplyInputModeForLayer(EYogUILayer NewLayer);
	EYogUILayer GetLayerForScreen(EYogUIScreenId ScreenId) const;
	void BindActivationDelegates(EYogUIScreenId ScreenId, UCommonActivatableWidget* Activatable);

	UPROPERTY(Transient)
	TObjectPtr<UYogUIRegistry> CachedRegistry;

	mutable TMap<EYogUIScreenId, TSubclassOf<UUserWidget>> LoadedWidgetClasses;

	UPROPERTY(Transient)
	TMap<EYogUIScreenId, TObjectPtr<UUserWidget>> Instances;

	/** 已绑定过 OnActivated/OnDeactivated 的 ScreenId，避免重复 AddRaw。 */
	TSet<EYogUIScreenId> BoundActivationScreens;

	/** 当前处于 Activated 状态的 ScreenId 集合（仅追踪 UCommonActivatableWidget）。 */
	TSet<EYogUIScreenId> ActivatedScreens;

	EYogUILayer TopActivatedLayer = EYogUILayer::Game;

	/** 正在异步加载中的 ScreenId → callback，避免重复发起。 */
	TMap<EYogUIScreenId, FOnAsyncScreenReady> PendingAsyncLoads;
};
