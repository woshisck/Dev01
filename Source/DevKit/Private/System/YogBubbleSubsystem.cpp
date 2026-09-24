#include "System/YogBubbleSubsystem.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Character/YogPlayerControllerBase.h"
#include "Component/BubbleMessageComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "HAL/IConsoleManager.h"
#include "System/YogSettings.h"
#include "UI/YogHUD.h"

DEFINE_LOG_CATEGORY(LogYogBubble);

namespace
{
	/** How long a dedup tag suppresses a repeat of the same bubble. */
	constexpr float YogBubble_DedupCooldownSeconds = 5.f;

	/** Keeps a bubble from flickering between anchors right at the viewport edge. */
	constexpr float YogBubble_ScreenEdgeMargin = 48.f;
}

void UYogBubbleSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	// A blocking bubble advances on the interact key, but EnterConversationMode sets
	// bBlockGameInput, and InteractPressed returns early on IsGameplayInputBlocked(). The raw
	// delegate fires ahead of that guard, so this is the only path that still reaches us.
	if (AYogPlayerControllerBase* PC = GetYogPlayerController())
	{
		BoundController = PC;
		InteractPressedHandle = PC->OnInteractPressedRaw.AddUObject(this, &UYogBubbleSubsystem::AdvanceBlockingBubble);
	}
}

void UYogBubbleSubsystem::Deinitialize()
{
	DismissAll();

	if (AYogPlayerControllerBase* PC = BoundController.Get())
	{
		PC->OnInteractPressedRaw.Remove(InteractPressedHandle);
	}
	InteractPressedHandle.Reset();
	BoundController.Reset();

	Super::Deinitialize();
}

TStatId UYogBubbleSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UYogBubbleSubsystem, STATGROUP_Tickables);
}

bool UYogBubbleSubsystem::DoesSupportWorldType(EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

// ─── Public API ──────────────────────────────────────────────────────────────

void UYogBubbleSubsystem::RequestBubbleFromRow(AActor* Speaker, const FDataTableRowHandle& RowHandle, const FGameplayTag& DedupTag)
{
	if (RowHandle.IsNull())
	{
		UE_LOG(LogYogBubble, Warning, TEXT("RequestBubbleFromRow: empty row handle."));
		return;
	}

	const FBubbleMessageRow* Row = RowHandle.GetRow<FBubbleMessageRow>(TEXT("YogBubbleSubsystem"));
	if (!Row)
	{
		UE_LOG(LogYogBubble, Warning, TEXT("RequestBubbleFromRow: row '%s' not found in %s."),
			*RowHandle.RowName.ToString(), *GetNameSafe(RowHandle.DataTable));
		return;
	}

	FBubbleRequest Request;
	Request.Speaker = Speaker;
	Request.SpeakerName = Row->SpeakerName;
	Request.Lines = Row->Lines;
	Request.AnchorMode = Row->AnchorMode;
	Request.ScreenCorner = Row->ScreenCorner;
	Request.Priority = Row->Priority;
	Request.bBlocking = Row->bBlocking;
	Request.DedupTag = DedupTag;

	SubmitRequest(MoveTemp(Request));
}

void UYogBubbleSubsystem::RequestBubbleByName(AActor* Speaker, FName RowName, const FGameplayTag& DedupTag)
{
	const UYogSettings* Settings = UYogSettings::Get();
	UDataTable* Table = Settings ? Settings->BubbleMessageTable.LoadSynchronous() : nullptr;
	if (!Table)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Bubble] No BubbleMessageTable configured; cannot resolve row '%s'."), *RowName.ToString());
		return;
	}

	FDataTableRowHandle Handle;
	Handle.DataTable = Table;
	Handle.RowName = RowName;
	RequestBubbleFromRow(Speaker, Handle, DedupTag);
}

void UYogBubbleSubsystem::RequestBubbleText(
	AActor* Speaker,
	const FText& SpeakerName,
	const FText& Line,
	float Duration,
	EBubbleAnchorMode AnchorMode,
	int32 Priority,
	const FGameplayTag& DedupTag)
{
	if (Line.IsEmpty())
	{
		return;
	}

	FBubbleRequest Request;
	Request.Speaker = Speaker;
	Request.SpeakerName = SpeakerName;
	Request.Lines.Add(FBubbleLine{ Line, FMath::Max(Duration, 0.1f) });
	Request.AnchorMode = IsValid(Speaker) ? AnchorMode : EBubbleAnchorMode::ScreenOnly;
	Request.Priority = Priority;
	Request.DedupTag = DedupTag;

	SubmitRequest(MoveTemp(Request));
}

void UYogBubbleSubsystem::DismissBubbleFor(AActor* Speaker)
{
	const TObjectKey<AActor> Key(Speaker);
	if (FActiveBubble* Bubble = ActiveAmbient.Find(Key))
	{
		ClearPresentation(*Bubble);
		ActiveAmbient.Remove(Key);
	}

	if (ActiveBlocking.IsSet() && ActiveBlocking->Request.Speaker.Get() == Speaker)
	{
		FinishBlocking();
	}
}

void UYogBubbleSubsystem::DismissAll()
{
	for (TPair<TObjectKey<AActor>, FActiveBubble>& Pair : ActiveAmbient)
	{
		ClearPresentation(Pair.Value);
	}
	ActiveAmbient.Reset();

	BlockingQueue.Reset();
	if (ActiveBlocking.IsSet())
	{
		FinishBlocking();
	}

	ComponentPool.Reset();
	DedupCooldowns.Reset();
}

// ─── Submission / routing ────────────────────────────────────────────────────

void UYogBubbleSubsystem::SubmitRequest(FBubbleRequest&& Request)
{
	if (!Request.IsValidRequest())
	{
		UE_LOG(LogYogBubble, Warning, TEXT("SubmitRequest: dropped, the request has no lines."));
		return;
	}

	// A bubble with no speaker has nothing to float above, whatever the row asked for.
	if (!Request.Speaker.IsValid())
	{
		Request.AnchorMode = EBubbleAnchorMode::ScreenOnly;
	}

	if (!ConsumeDedup(Request.DedupTag))
	{
		UE_LOG(LogYogBubble, Verbose, TEXT("SubmitRequest: suppressed by dedup tag %s."),
			*Request.DedupTag.ToString());
		return;
	}

	UE_LOG(LogYogBubble, Log, TEXT("SubmitRequest: speaker=%s lines=%d anchor=%d blocking=%d"),
		*GetNameSafe(Request.Speaker.Get()), Request.Lines.Num(),
		static_cast<int32>(Request.AnchorMode), Request.bBlocking ? 1 : 0);

	if (Request.bBlocking)
	{
		if (ActiveBlocking.IsSet())
		{
			BlockingQueue.Add(MoveTemp(Request));
			SortBlockingQueue(BlockingQueue);
			return;
		}

		StartBlocking(MoveTemp(Request));
		return;
	}

	StartAmbient(MoveTemp(Request));
}

void UYogBubbleSubsystem::StartAmbient(FBubbleRequest&& Request)
{
	const TObjectKey<AActor> Key(Request.Speaker.Get());

	// Ambient bubbles are lossy on purpose: a speaker mid-line drops the newcomer unless it
	// outranks the current one, so chatter never builds a backlog on one actor.
	if (FActiveBubble* Existing = ActiveAmbient.Find(Key))
	{
		if (Request.Priority < Existing->Request.Priority)
		{
			return;
		}
		ClearPresentation(*Existing);
	}

	FActiveBubble Bubble;
	Bubble.Request = MoveTemp(Request);
	Bubble.LineIndex = 0;
	Bubble.TimeRemaining = Bubble.Request.Lines[0].Duration;

	FActiveBubble& Stored = ActiveAmbient.Add(Key, MoveTemp(Bubble));
	RefreshAnchor(Stored);
	PresentLine(Stored);
}

void UYogBubbleSubsystem::StartBlocking(FBubbleRequest&& Request)
{
	FActiveBubble Bubble;
	Bubble.Request = MoveTemp(Request);
	Bubble.LineIndex = 0;

	ActiveBlocking = MoveTemp(Bubble);

	// Entered once for the whole sequence, not per line.
	if (AYogPlayerControllerBase* PC = GetYogPlayerController())
	{
		PC->EnterConversationMode();
	}

	RefreshAnchor(*ActiveBlocking);
	PresentLine(*ActiveBlocking);
}

void UYogBubbleSubsystem::AdvanceBlockingBubble()
{
	if (!ActiveBlocking.IsSet())
	{
		return;
	}

	++ActiveBlocking->LineIndex;
	if (!ActiveBlocking->Request.Lines.IsValidIndex(ActiveBlocking->LineIndex))
	{
		FinishBlocking();
		return;
	}

	RefreshAnchor(*ActiveBlocking);
	PresentLine(*ActiveBlocking);
}

void UYogBubbleSubsystem::FinishBlocking()
{
	if (!ActiveBlocking.IsSet())
	{
		return;
	}

	ClearPresentation(*ActiveBlocking);
	ActiveBlocking.Reset();

	if (AYogPlayerControllerBase* PC = GetYogPlayerController())
	{
		PC->ExitConversationMode();
	}

	if (BlockingQueue.Num() > 0)
	{
		FBubbleRequest Next = MoveTemp(BlockingQueue[0]);
		BlockingQueue.RemoveAt(0);
		StartBlocking(MoveTemp(Next));
	}
}

// ─── Tick ────────────────────────────────────────────────────────────────────

void UYogBubbleSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	TickDedup(DeltaTime);
	TickAmbient(DeltaTime);

	// Blocking bubbles never time out, but they still need to follow the speaker on and off screen.
	if (ActiveBlocking.IsSet())
	{
		const bool bWasScreen = ActiveBlocking->bUsingScreenFallback;
		RefreshAnchor(*ActiveBlocking);
		if (bWasScreen != ActiveBlocking->bUsingScreenFallback)
		{
			PresentLine(*ActiveBlocking);
		}
	}
}

void UYogBubbleSubsystem::TickAmbient(float DeltaTime)
{
	TArray<TObjectKey<AActor>, TInlineAllocator<4>> Finished;

	for (TPair<TObjectKey<AActor>, FActiveBubble>& Pair : ActiveAmbient)
	{
		FActiveBubble& Bubble = Pair.Value;

		// A screen-anchored bubble legitimately has no speaker; a world one that lost its actor
		// (killed mid-line) has nothing left to float above.
		const bool bNeedsSpeaker = Bubble.Request.AnchorMode != EBubbleAnchorMode::ScreenOnly;
		if (bNeedsSpeaker && !Bubble.Request.Speaker.IsValid())
		{
			Finished.Add(Pair.Key);
			continue;
		}

		const bool bWasScreen = Bubble.bUsingScreenFallback;
		RefreshAnchor(Bubble);
		if (bWasScreen != Bubble.bUsingScreenFallback)
		{
			PresentLine(Bubble);
		}

		Bubble.TimeRemaining -= DeltaTime;
		if (Bubble.TimeRemaining > 0.f)
		{
			continue;
		}

		++Bubble.LineIndex;
		const FBubbleLine* NextLine = Bubble.GetCurrentLine();
		if (!NextLine)
		{
			Finished.Add(Pair.Key);
			continue;
		}

		Bubble.TimeRemaining = NextLine->Duration;
		PresentLine(Bubble);
	}

	for (const TObjectKey<AActor>& Key : Finished)
	{
		if (FActiveBubble* Bubble = ActiveAmbient.Find(Key))
		{
			ClearPresentation(*Bubble);
		}
		ActiveAmbient.Remove(Key);

		// The component died with its actor; drop the pool entry so the map does not grow
		// one stale key per dead speaker over a long run.
		if (const TWeakObjectPtr<UBubbleMessageComponent>* Pooled = ComponentPool.Find(Key))
		{
			if (!Pooled->IsValid())
			{
				ComponentPool.Remove(Key);
			}
		}
	}
}

void UYogBubbleSubsystem::TickDedup(float DeltaTime)
{
	for (auto It = DedupCooldowns.CreateIterator(); It; ++It)
	{
		It.Value() -= DeltaTime;
		if (It.Value() <= 0.f)
		{
			It.RemoveCurrent();
		}
	}
}

// ─── Anchoring / presentation ────────────────────────────────────────────────

bool UYogBubbleSubsystem::ResolveShouldFallbackToScreen(
	const FVector2D& ScreenPos,
	const FVector2D& ViewportSize,
	bool bProjectionValid,
	float EdgeMargin)
{
	// A point behind the camera still projects to finite coordinates that can land inside the
	// viewport, so the bounds test below is not enough on its own.
	if (!bProjectionValid)
	{
		return true;
	}

	if (ViewportSize.X <= KINDA_SMALL_NUMBER || ViewportSize.Y <= KINDA_SMALL_NUMBER)
	{
		return true;
	}

	return ScreenPos.X < EdgeMargin
		|| ScreenPos.Y < EdgeMargin
		|| ScreenPos.X > ViewportSize.X - EdgeMargin
		|| ScreenPos.Y > ViewportSize.Y - EdgeMargin;
}

void UYogBubbleSubsystem::RefreshAnchor(FActiveBubble& Bubble)
{
	switch (Bubble.Request.AnchorMode)
	{
	case EBubbleAnchorMode::ScreenOnly:
		Bubble.bUsingScreenFallback = true;
		return;

	case EBubbleAnchorMode::WorldOnly:
		Bubble.bUsingScreenFallback = false;
		return;

	default:
		break;
	}

	AActor* Speaker = Bubble.Request.Speaker.Get();
	APlayerController* PC = GetYogPlayerController();
	if (!Speaker || !PC)
	{
		Bubble.bUsingScreenFallback = true;
		return;
	}

	FVector2D ScreenPos = FVector2D::ZeroVector;
	const bool bProjected = PC->ProjectWorldLocationToScreen(Speaker->GetActorLocation(), ScreenPos, false);

	FVector2D ViewportSize(1920.f, 1080.f);
	if (UGameViewportClient* GVC = GetWorld() ? GetWorld()->GetGameViewport() : nullptr)
	{
		GVC->GetViewportSize(ViewportSize);
	}

	Bubble.bUsingScreenFallback =
		ResolveShouldFallbackToScreen(ScreenPos, ViewportSize, bProjected, YogBubble_ScreenEdgeMargin);
}

void UYogBubbleSubsystem::PresentLine(FActiveBubble& Bubble)
{
	const FBubbleLine* Line = Bubble.GetCurrentLine();
	if (!Line)
	{
		return;
	}

	AActor* Speaker = Bubble.Request.Speaker.Get();
	const TObjectKey<AActor> Key(Speaker);

	if (Bubble.bUsingScreenFallback)
	{
		if (UBubbleMessageComponent* Component = ComponentPool.FindRef(Key).Get())
		{
			Component->Hide();
		}

		// One screen slot exists, so the first claimant keeps it and later bubbles simply wait
		// rather than fighting over the corner.
		if (bScreenSlotInUse && ScreenSlotOwner != Key)
		{
			UE_LOG(LogYogBubble, Log, TEXT("PresentLine: corner slot busy, waiting."));
			return;
		}

		AYogHUD* HUD = GetYogHUD();
		if (!HUD)
		{
			UE_LOG(LogYogBubble, Warning,
				TEXT("PresentLine: no AYogHUD on the player controller, so a screen-anchored bubble cannot draw. ")
				TEXT("Check the GameMode's HUDClass for this level."));
			return;
		}

		HUD->ShowBubbleAtCorner(Bubble.Request.SpeakerName, Line->Text, Bubble.Request.ScreenCorner);
		ScreenSlotOwner = Key;
		bScreenSlotInUse = true;
		UE_LOG(LogYogBubble, Log, TEXT("PresentLine: corner bubble shown."));
		return;
	}

	UBubbleMessageComponent* Component = GetOrCreateComponent(Speaker);
	if (!Component)
	{
		UE_LOG(LogYogBubble, Warning, TEXT("PresentLine: could not create a bubble component on '%s'."),
			*GetNameSafe(Speaker));
		return;
	}

	Component->ShowLine(Bubble.Request.SpeakerName, Line->Text);
	UE_LOG(LogYogBubble, Log, TEXT("PresentLine: world bubble shown above '%s'."), *GetNameSafe(Speaker));
}

void UYogBubbleSubsystem::ClearPresentation(FActiveBubble& Bubble)
{
	const TObjectKey<AActor> Key(Bubble.Request.Speaker.Get());

	if (UBubbleMessageComponent* Component = ComponentPool.FindRef(Key).Get())
	{
		Component->Hide();
	}

	if (bScreenSlotInUse && ScreenSlotOwner == Key)
	{
		if (AYogHUD* HUD = GetYogHUD())
		{
			HUD->HideBubble();
		}
		bScreenSlotInUse = false;
		ScreenSlotOwner = TObjectKey<AActor>();
	}
}

// ─── Helpers ─────────────────────────────────────────────────────────────────

bool UYogBubbleSubsystem::ConsumeDedup(const FGameplayTag& DedupTag)
{
	if (!DedupTag.IsValid())
	{
		return true;
	}

	if (DedupCooldowns.Contains(DedupTag))
	{
		return false;
	}

	DedupCooldowns.Add(DedupTag, YogBubble_DedupCooldownSeconds);
	return true;
}

void UYogBubbleSubsystem::SortBlockingQueue(TArray<FBubbleRequest>& Queue)
{
	// Stable so equal priorities keep the order they were requested in.
	Queue.StableSort([](const FBubbleRequest& A, const FBubbleRequest& B)
	{
		return A.Priority > B.Priority;
	});
}

UBubbleMessageComponent* UYogBubbleSubsystem::GetOrCreateComponent(AActor* Speaker)
{
	if (!IsValid(Speaker))
	{
		return nullptr;
	}

	const TObjectKey<AActor> Key(Speaker);
	if (UBubbleMessageComponent* Pooled = ComponentPool.FindRef(Key).Get())
	{
		return Pooled;
	}

	// An actor may already carry one from the editor; reuse it before adding another.
	UBubbleMessageComponent* Component = Speaker->FindComponentByClass<UBubbleMessageComponent>();
	if (!Component)
	{
		Component = NewObject<UBubbleMessageComponent>(Speaker);
		Component->RegisterComponent();
	}

	ComponentPool.Add(Key, Component);
	return Component;
}

AYogPlayerControllerBase* UYogBubbleSubsystem::GetYogPlayerController() const
{
	const UWorld* World = GetWorld();
	return World ? Cast<AYogPlayerControllerBase>(World->GetFirstPlayerController()) : nullptr;
}

AYogHUD* UYogBubbleSubsystem::GetYogHUD() const
{
	APlayerController* PC = GetYogPlayerController();
	return PC ? Cast<AYogHUD>(PC->GetHUD()) : nullptr;
}

// ─── Console commands ────────────────────────────────────────────────────────
//
// Registered with the console manager rather than declared UFUNCTION(Exec) on the cheat manager:
// exec functions need a PlayerController that has already built its UCheatManager, which silently
// does not happen in every console context. These resolve the world themselves and work from any
// console, so "nothing happened" is always accompanied by a reason in the log.

#if !UE_BUILD_SHIPPING

namespace
{
	UYogBubbleSubsystem* YogBubble_ResolveSubsystem(UWorld* World, const TCHAR* CommandName)
	{
		if (!World)
		{
			UE_LOG(LogYogBubble, Warning, TEXT("%s: no world context."), CommandName);
			return nullptr;
		}

		UYogBubbleSubsystem* Subsystem = World->GetSubsystem<UYogBubbleSubsystem>();
		if (!Subsystem)
		{
			UE_LOG(LogYogBubble, Warning,
				TEXT("%s: no bubble subsystem in world '%s'. The subsystem only exists during Play, ")
				TEXT("so start PIE before running this."),
				CommandName, *World->GetName());
			return nullptr;
		}

		return Subsystem;
	}

	AActor* YogBubble_ResolveSpeaker(UWorld* World)
	{
		const APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		return PC ? PC->GetPawn() : nullptr;
	}

	void YogBubble_ExecText(const TArray<FString>& Args, UWorld* World)
	{
		UYogBubbleSubsystem* Subsystem = YogBubble_ResolveSubsystem(World, TEXT("Yog.Bubble"));
		if (!Subsystem)
		{
			return;
		}

		const FString Text = Args.Num() > 0 ? FString::Join(Args, TEXT(" ")) : TEXT("Bubble test");
		AActor* Speaker = YogBubble_ResolveSpeaker(World);

		UE_LOG(LogYogBubble, Log, TEXT("Yog.Bubble: text='%s' speaker='%s'"), *Text, *GetNameSafe(Speaker));

		Subsystem->RequestBubbleText(
			Speaker,
			NSLOCTEXT("YogBubble", "DebugSpeaker", "GM"),
			FText::FromString(Text),
			5.f,
			EBubbleAnchorMode::Auto,
			YogBubblePriority::Story,
			FGameplayTag());
	}

	void YogBubble_ExecRow(const TArray<FString>& Args, UWorld* World)
	{
		UYogBubbleSubsystem* Subsystem = YogBubble_ResolveSubsystem(World, TEXT("Yog.BubbleRow"));
		if (!Subsystem)
		{
			return;
		}

		if (Args.Num() == 0)
		{
			UE_LOG(LogYogBubble, Warning, TEXT("Yog.BubbleRow: usage: Yog.BubbleRow <RowName>"));
			return;
		}

		AActor* Speaker = YogBubble_ResolveSpeaker(World);
		UE_LOG(LogYogBubble, Log, TEXT("Yog.BubbleRow: row='%s' speaker='%s'"), *Args[0], *GetNameSafe(Speaker));

		Subsystem->RequestBubbleByName(Speaker, FName(*Args[0]), FGameplayTag());
	}

	FAutoConsoleCommandWithWorldAndArgs GYogBubbleTextCommand(
		TEXT("Yog.Bubble"),
		TEXT("Show a test speech bubble above the player pawn. Usage: Yog.Bubble <text>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&YogBubble_ExecText));

	FAutoConsoleCommandWithWorldAndArgs GYogBubbleRowCommand(
		TEXT("Yog.BubbleRow"),
		TEXT("Show a bubble from DT_BubbleMessages. Usage: Yog.BubbleRow <RowName>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&YogBubble_ExecRow));
}

#endif // !UE_BUILD_SHIPPING
