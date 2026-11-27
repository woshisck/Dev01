#pragma once

#include "CoreMinimal.h"
#include "Containers/StaticArray.h"
#include "HAL/ThreadSafeCounter.h"
#include "RHIResources.h"

class FSceneInterface;
class FTextureRenderTargetResource;

/** Low-rate, scene-view-owned environment field. No UObject is read on RT. */
namespace YogNinjaFogMaskCache
{
struct FInput
{
    FSceneInterface* Scene = nullptr;
    FTextureRHIRef Target;
    // Same RGBA16F coefficients as Target, stored as R x R x S for one
    // hardware-linear material lookup. Target remains the compatible atlas.
    FTextureRHIRef VolumeTarget;
    FTextureRHIRef WakeDensity;
    FVector3f WakeMin = FVector3f::ZeroVector;
    FVector3f WakeInvSize = FVector3f::ZeroVector;
    float WakeStrength = 0.f;
    FVector3f Min = FVector3f::ZeroVector;
    FVector3f Size = FVector3f(1.f);
    uint32 Resolution = 128;
    uint32 Slices = 16;
    uint32 Columns = 4;
    float GroundHeight = 0.f;
    float BodyHeight = 180.f;
    float GroundAmount = 2.5f;
    float UpperAmount = .55f;
    float SurfaceFade = 40.f;
    float SurfaceBand = 250.f;
    float ContactBoost = .35f;
    uint32 HeightEnabled = 1;
    float NearHeight = 35.f;
    float FarHeight = 350.f;
    float WallHeight = 500.f;
    float HeightFeather = 120.f;
    float WallRange = 150.f;
    float PlayerStrength = .85f;
    float PlayerFeather = 400.f;
    float PlayerHeight = 350.f;
    float LightStrength = .8f;
    float LightFeather = 450.f;
    FVector4f Observer = FVector4f(0,0,0,0);
    TStaticArray<FVector4f,4> Lights;
    TStaticArray<FVector4f,4> Directions;
    uint64 Revision = 0;

    FInput()
    {
        for (int32 Index=0;Index<4;++Index)
        {
            Lights[Index]=FVector4f(0,0,0,0);
            Directions[Index]=FVector4f(0,0,0,-1);
        }
    }
};

struct FState
{
    FThreadSafeCounter Executed;
    FThreadSafeCounter ValidGDF;
    // Revision is intentionally small and monotonic per controller. Set when
    // its compute pass has been submitted; same-queue GPU barriers order reads.
    FThreadSafeCounter CompletedRevision;
};

void Startup();
void Shutdown();
void Submit(uint64 Key,const FInput& Input,TSharedRef<FState,ESPMode::ThreadSafe> State,
    FTextureRenderTargetResource* WakeResource = nullptr);
void Remove(uint64 Key);
}
