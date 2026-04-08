#include "BuffFlow/Nodes/BFNode_Literals.h"

// ── LiteralFloat ────────────────────────────────────────────────────────────

UBFNode_LiteralFloat::UBFNode_LiteralFloat(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("BuffFlow|Utility");
#endif
	// 纯数据节点：移除所有执行引脚，只暴露 Value 数据输出引脚
	InputPins  = {};
	OutputPins = {};
}

// ── LiteralInt ──────────────────────────────────────────────────────────────

UBFNode_LiteralInt::UBFNode_LiteralInt(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("BuffFlow|Utility");
#endif
	InputPins  = {};
	OutputPins = {};
}

// ── LiteralBool ─────────────────────────────────────────────────────────────

UBFNode_LiteralBool::UBFNode_LiteralBool(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("BuffFlow|Utility");
#endif
	InputPins  = {};
	OutputPins = {};
}
