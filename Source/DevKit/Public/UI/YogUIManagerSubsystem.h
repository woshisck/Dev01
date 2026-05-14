#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "UI/YogUIRegistry.h"
#include "YogUIManagerSubsystem.generated.h"

class UUserWidget;
class UCommonActivatableWidget;
class UYogUIRegistry;

USTRUCT(BlueprintType)
struct DEVKIT_API FYogUIScreenInputPolicy
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	bool bShowMouseCursor = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	bool bPauseGame = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	bool bDisablePawnInput = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	bool bAffectsMajorUI = false;
};

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

	UFUNCTION(BlueprintCallable, Category = "UI")
	void SetWidgetClassOverride(EYogUIScreenId ScreenId, TSubclassOf<UUserWidget> WidgetClass);

	UFUNCTION(BlueprintCallable, Category = "UI|Input")
	void SetInputPolicyOverride(EYogUIScreenId ScreenId, const FYogUIScreenInputPolicy& Policy);

	UFUNCTION(BlueprintCallable, Category = "UI|Input")
	void ClearInputPolicyOverride(EYogUIScreenId ScreenId);

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

	template<typename WidgetT>
	WidgetT* PushTypedScreen(EYogUIScreenId ScreenId)
	{
		return Cast<WidgetT>(PushScreen(ScreenId));
	}

	UFUNCTION(BlueprintCallable, Category = "UI", meta = (AutoCreateRefTerm = "OnReady"))
	void PushScreenAsync(EYogUIScreenId ScreenId, const FOnAsyncScreenReady& OnReady);

	UFUNCTION(BlueprintCallable, Category = "UI")
	void PopScreen(EYogUIScreenId ScreenId);

	static bool PopManagedScreen(UUserWidget* Widget, EYogUIScreenId ScreenId);

	// ─── One-shot popups ─────────────────────────────────────
	//
	// 用法：用 FGameplayTag 标识一次性弹窗（教程提示、首拾道具说明、首次解锁机制…），
	// 用 EPopupScope 控制去重周期。
	//
	//   if (UI->PushScreenOnce(EYogUIScreenId::TutorialPopup, Tag_Popup_FirstRune, EPopupScope::Save))
	//   { /* 真正弹了；可以做附加埋点 */ }
	//
	// 已经展示过的同一 PopupKey 会直接跳过（返回 nullptr），不再触发 Push / 异步加载 / InputMode 切换。
	// Save scope 会立即写入 UYogSaveGame.ShownPopupKeys 并调用 WriteSaveGame()。
	// 异步路径见 PushScreenOnceAsync —— 命中已展示时回调当帧带 nullptr 触发。

	UFUNCTION(BlueprintCallable, Category = "UI|Popup")
	UCommonActivatableWidget* PushScreenOnce(EYogUIScreenId ScreenId, FGameplayTag PopupKey, EPopupScope Scope);

	UFUNCTION(BlueprintCallable, Category = "UI|Popup", meta = (AutoCreateRefTerm = "OnReady"))
	void PushScreenOnceAsync(EYogUIScreenId ScreenId, FGameplayTag PopupKey, EPopupScope Scope, const FOnAsyncScreenReady& OnReady);

	UFUNCTION(BlueprintCallable, Category = "UI|Popup")
	bool IsPopupShown(FGameplayTag PopupKey, EPopupScope Scope) const;

	/** 手动标记（用于不走 PushScreenOnce 但仍需算"已展示"的场景，例如玩家从设置里跳过）。 */
	UFUNCTION(BlueprintCallable, Category = "UI|Popup")
	void MarkPopupShown(FGameplayTag PopupKey, EPopupScope Scope);

	/** 清空指定 scope 的已展示集合。Run scope 应在 Run 开始时调用；Save scope 需要回看时调用。 */
	UFUNCTION(BlueprintCallable, Category = "UI|Popup")
	void ResetPopupsForScope(EPopupScope Scope);

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
	FYogUIScreenInputPolicy GetInputPolicyForScreen(EYogUIScreenId ScreenId) const;
	EYogUIScreenId FindTopActiveScreen(EYogUILayer NewLayer) const;
	void BindActivationDelegates(EYogUIScreenId ScreenId, UCommonActivatableWidget* Activatable);
	void ApplyPauseAndMajorUIState(const FYogUIScreenInputPolicy& Policy);

	UPROPERTY(Transient)
	TObjectPtr<UYogUIRegistry> CachedRegistry;

	mutable TMap<EYogUIScreenId, TSubclassOf<UUserWidget>> LoadedWidgetClasses;

	mutable TMap<EYogUIScreenId, TSubclassOf<UUserWidget>> WidgetClassOverrides;

	TMap<EYogUIScreenId, FYogUIScreenInputPolicy> InputPolicyOverrides;

	UPROPERTY(Transient)
	TMap<EYogUIScreenId, TObjectPtr<UUserWidget>> Instances;

	/** 已绑定过 OnActivated/OnDeactivated 的 ScreenId，避免重复 AddRaw。 */
	TSet<EYogUIScreenId> BoundActivationScreens;

	/** 当前处于 Activated 状态的 ScreenId 集合（仅追踪 UCommonActivatableWidget）。 */
	TSet<EYogUIScreenId> ActivatedScreens;

	TArray<EYogUIScreenId> ActivationStack;

	EYogUILayer TopActivatedLayer = EYogUILayer::Game;
	bool bManagedPauseActive = false;
	bool bManagedMajorUIActive = false;
	bool bManagedPawnInputDisabled = false;

	/** 正在异步加载中的 ScreenId → callback，避免重复发起。 */
	TMap<EYogUIScreenId, FOnAsyncScreenReady> PendingAsyncLoads;

	/** Session / Run scope 的已展示弹窗集合（Save scope 走 UYogSaveGame.ShownPopupKeys）。 */
	TSet<FGameplayTag> ShownPopups_Session;
	TSet<FGameplayTag> ShownPopups_Run;

	struct FPendingPopupOnce
	{
		FGameplayTag Key;
		EPopupScope Scope = EPopupScope::Session;
		FOnAsyncScreenReady UserCallback;
	};

	/** PushScreenOnceAsync 的待处理 metadata。回调命中后用于 MarkPopupShown + 转发给 caller。 */
	TMap<EYogUIScreenId, FPendingPopupOnce> PendingPopupOnce;

	UFUNCTION()
	void HandlePushScreenOnceAsyncReady(EYogUIScreenId ScreenId, UCommonActivatableWidget* Widget);

	class UYogSaveGame* GetCurrentSaveGame() const;
};
