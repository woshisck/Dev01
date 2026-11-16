#pragma once

class AYogAirFogVolume;
class AYogPlanarFogActor;

// Game-thread lifetime API. The bridge never owns or changes authored PP volumes.
namespace YogAirFogRenderBridge
{
void Startup();
void Shutdown();
void RegisterVolume(AYogAirFogVolume* Volume);
void UnregisterVolume(AYogAirFogVolume* Volume);
void RegisterPlanarVolume(AYogPlanarFogActor* Volume);
void UnregisterPlanarVolume(AYogPlanarFogActor* Volume);
}
