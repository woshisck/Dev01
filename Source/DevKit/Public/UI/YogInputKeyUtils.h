#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"

namespace YogInputKeys
{
	inline bool IsAcceptKey(const FKey& Key)
	{
		return Key == EKeys::Enter
			|| Key == EKeys::Virtual_Accept
			|| Key == EKeys::Gamepad_FaceButton_Bottom;
	}

	inline bool IsBackKey(const FKey& Key)
	{
		return Key == EKeys::Escape
			|| Key == EKeys::Virtual_Back
			|| Key == EKeys::Gamepad_FaceButton_Right;
	}

	inline bool IsMenuKey(const FKey& Key)
	{
		return Key == EKeys::Gamepad_Special_Right;
	}

	inline bool IsSecondaryKey(const FKey& Key)
	{
		return Key == EKeys::Gamepad_FaceButton_Left;
	}

	inline bool IsDetailsKey(const FKey& Key)
	{
		return Key == EKeys::Gamepad_FaceButton_Top;
	}

	inline int32 GetHorizontalNavigationDirection(const FKey& Key)
	{
		if (Key == EKeys::Left
			|| Key == EKeys::Gamepad_DPad_Left
			|| Key == EKeys::Gamepad_LeftStick_Left)
		{
			return -1;
		}
		if (Key == EKeys::Right
			|| Key == EKeys::Gamepad_DPad_Right
			|| Key == EKeys::Gamepad_LeftStick_Right)
		{
			return 1;
		}
		return 0;
	}

	inline int32 GetVerticalNavigationDirection(const FKey& Key)
	{
		if (Key == EKeys::Up
			|| Key == EKeys::Gamepad_DPad_Up
			|| Key == EKeys::Gamepad_LeftStick_Up)
		{
			return -1;
		}
		if (Key == EKeys::Down
			|| Key == EKeys::Gamepad_DPad_Down
			|| Key == EKeys::Gamepad_LeftStick_Down)
		{
			return 1;
		}
		return 0;
	}
}
