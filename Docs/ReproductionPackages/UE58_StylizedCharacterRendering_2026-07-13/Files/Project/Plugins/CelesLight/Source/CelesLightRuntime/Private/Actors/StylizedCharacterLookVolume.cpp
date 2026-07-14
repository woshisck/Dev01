#include "Actors/StylizedCharacterLookVolume.h"

AStylizedCharacterLookVolume::AStylizedCharacterLookVolume(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bEnabled = true;
	bUnbound = false;
	BlendRadius = 200.0f;
	BlendWeight = 1.0f;
}
