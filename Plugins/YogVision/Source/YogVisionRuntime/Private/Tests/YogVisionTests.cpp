#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "YogVisionObserverComponent.h"
#include "YogVisionTargetComponent.h"
#include "YogVisionSourceComponent.h"
#include "YogVisionSubsystem.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/Actor.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYogVisionModifierTest, "YogVision.Runtime.ModifierStack", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FYogVisionModifierTest::RunTest(const FString& Parameters)
{
    UYogVisionObserverComponent* Observer = NewObject<UYogVisionObserverComponent>();
    const FYogVisionParameters Base;
    Observer->SetVisionModifier(TEXT("Torch"), 250.f, 0.f, 0.f);
    Observer->SetVisionModifier(TEXT("Charm"), 0.f, 700.f, .2f);
    TestEqual(TEXT("Independent items stack sight"), Observer->ResolveParameters(Base).BaseSightRadius, 670.f);
    TestEqual(TEXT("Independent items stack spirit range"), Observer->ResolveParameters(Base).SpiritRadius, 2500.f);
    Observer->SetVisionModifier(TEXT("Torch"), 100.f, 0.f, 0.f);
    TestEqual(TEXT("Same token replaces rather than leaking bonuses"), Observer->ResolveParameters(Base).BaseSightRadius, 520.f);
    Observer->RemoveVisionModifier(TEXT("Torch"));
    TestEqual(TEXT("Unequipping restores base sight"), Observer->ResolveParameters(Base).BaseSightRadius, Base.BaseSightRadius);
    TestEqual(TEXT("Removing torch preserves charm"), Observer->ResolveParameters(Base).SpiritRadius, 2500.f);
    Observer->RemoveVisionModifier(TEXT("Charm"));
    TestEqual(TEXT("Unequipping both restores strength"), Observer->ResolveParameters(Base).SpiritStrength, Base.SpiritStrength);
    Observer->SetVisionModifier(NAME_None, 1000.f, 1000.f, 1.f);
    TestEqual(TEXT("Unnamed item is ignored"), Observer->ResolveParameters(Base).BaseSightRadius, Base.BaseSightRadius);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYogVisionSanitizeTest, "YogVision.Runtime.Sanitize", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FYogVisionSanitizeTest::RunTest(const FString& Parameters)
{
    FYogVisionParameters Values;
    Values.BaseSightRadius = std::numeric_limits<float>::quiet_NaN();
    Values.SpiritRadius = -600.f;
    Values.SpiritStrength = std::numeric_limits<float>::infinity();
    Values.StrokeScale = -1.f;
    Values.FogDensity = 9.f;
    Values.DistanceFogStart = 5000.f;
    Values.DistanceFogEnd = 50.f;
    Values.SpiritVolumeHeight = 0.f;
    Values.SpiritTrailLength = std::numeric_limits<float>::infinity();
    Values.FogColor.R = std::numeric_limits<float>::quiet_NaN();
    Values.Sanitize();
    TestEqual(TEXT("NaN radius restored to safe default"), Values.BaseSightRadius, 420.f);
    TestEqual(TEXT("Negative spirit radius clamped"), Values.SpiritRadius, 0.f);
    TestEqual(TEXT("Infinite strength restored"), Values.SpiritStrength, .65f);
    TestTrue(TEXT("Stroke scale stays finite and nonzero"), FMath::IsFinite(Values.StrokeScale) && Values.StrokeScale > 0.f);
    TestEqual(TEXT("Density is bounded"), Values.FogDensity, 1.f);
    TestTrue(TEXT("Color is safe for shader"), FMath::IsFinite(Values.FogColor.R));
    TestTrue(TEXT("Distance fog remains ordered when artist reverses its range"), Values.DistanceFogEnd > Values.DistanceFogStart);
    TestTrue(TEXT("Spirit volume cannot collapse into a flat plane"), Values.SpiritVolumeHeight >= 10.f);
    TestTrue(TEXT("Spirit trail length is finite for ray-volume math"), FMath::IsFinite(Values.SpiritTrailLength));
    return true;
}

namespace
{
AActor* SpawnVisionTestActor(UWorld* World, const FVector& Position)
{
    FActorSpawnParameters Spawn;
    Spawn.ObjectFlags = RF_Transient;
    AActor* Actor = World->SpawnActor<AActor>(Spawn);
    USceneComponent* Root = NewObject<USceneComponent>(Actor);
    Actor->SetRootComponent(Root);
    Actor->AddInstanceComponent(Root);
    Root->RegisterComponent();
    Actor->SetActorLocation(Position);
    return Actor;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYogVisionWallTest, "YogVision.Runtime.WallAndLifecycle", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FYogVisionWallTest::RunTest(const FString& Parameters)
{
    const UWorld::InitializationValues Init = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, MakeUniqueObjectName(GetTransientPackage(), UWorld::StaticClass(), TEXT("YogVisionRuntimeTest")), nullptr, true, ERHIFeatureLevel::Num, &Init);
    if (!TestNotNull(TEXT("Test world"), World)) return false;
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    UYogVisionSubsystem* System = World->GetSubsystem<UYogVisionSubsystem>();
    if (!TestNotNull(TEXT("Vision subsystem"), System)) { GEngine->DestroyWorldContext(World); World->DestroyWorld(false); return false; }
    AActor* Observer = SpawnVisionTestActor(World, FVector::ZeroVector);
    AActor* Enemy = SpawnVisionTestActor(World, FVector(600, 0, 0));
    UYogVisionTargetComponent* Target = NewObject<UYogVisionTargetComponent>(Enemy);
    Enemy->AddInstanceComponent(Target);
    Target->RegisterComponent();
    Target->Activate(true);
    UStaticMeshComponent* Body = NewObject<UStaticMeshComponent>(Enemy);
    Enemy->AddInstanceComponent(Body);
    Body->SetupAttachment(Enemy->GetRootComponent());
    Body->RegisterComponent();
    UStaticMeshComponent* AuthoredHidden = NewObject<UStaticMeshComponent>(Enemy);
    Enemy->AddInstanceComponent(AuthoredHidden);
    AuthoredHidden->SetupAttachment(Enemy->GetRootComponent());
    AuthoredHidden->SetVisibility(false, false);
    AuthoredHidden->RegisterComponent();
    AActor* Wall = SpawnVisionTestActor(World, FVector(300, 0, 70));
    UBoxComponent* Blocker = NewObject<UBoxComponent>(Wall);
    Wall->AddInstanceComponent(Blocker);
    Blocker->SetupAttachment(Wall->GetRootComponent());
    Blocker->SetBoxExtent(FVector(30, 400, 200));
    Blocker->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Blocker->SetCollisionResponseToAllChannels(ECR_Ignore);
    Blocker->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    Blocker->RegisterComponent();
    FYogVisionParameters Values;
    Values.BaseSightRadius = 1000.f;
    System->SetEnabled(false);
    const FYogVisionParameters Before = System->GetParameters();
    System->SetPreviewObserver(Observer);
    System->ApplyParameters(Values);
    System->SetEnabled(true);
    System->Tick(.1f);
    System->Tick(.1f);
    TestFalse(TEXT("Wall blocks direct visibility"), Target->IsDirectlyVisible());
    TestTrue(TEXT("Spirit perception survives wall"), Target->IsSpiritSensed());
    // Test the component presentation contract even if an optional artist material has not been generated.
    Target->UpdatePresentation(true, Target->IsDirectlyVisible());
    TestFalse(TEXT("Only echo remains outside direct sight"), Body->GetVisibleFlag());
    TestFalse(TEXT("Hiding render body does not hide Actor or suppress perception"), Enemy->IsHidden());
    AActor* Weapon = SpawnVisionTestActor(World, Enemy->GetActorLocation());
    Weapon->SetOwner(Enemy); Weapon->AttachToActor(Enemy,FAttachmentTransformRules::KeepWorldTransform);
    UStaticMeshComponent* WeaponBody = NewObject<UStaticMeshComponent>(Weapon);
    Weapon->AddInstanceComponent(WeaponBody);WeaponBody->SetupAttachment(Weapon->GetRootComponent());WeaponBody->RegisterComponent();
    Target->UpdatePresentation(true,false);
    TestFalse(TEXT("Owned attached weapon cannot reveal enemy identity"),WeaponBody->GetVisibleFlag());
    Weapon->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);Weapon->SetOwner(nullptr);
    Target->UpdatePresentation(true,false);
    TestTrue(TEXT("Dropped weapon restores its prior presentation independently"),WeaponBody->GetVisibleFlag());
    Target->ApplyRequestedVisibility(AuthoredHidden, true);
    TestFalse(TEXT("Ticking widget request cannot leak through fog"), AuthoredHidden->GetVisibleFlag());
    Blocker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    System->Tick(.1f);
    TestTrue(TEXT("Removing wall restores direct visibility"), Target->IsDirectlyVisible());
    TestFalse(TEXT("Directly visible enemy does not duplicate spirit echo"), Target->IsSpiritSensed());
    TestTrue(TEXT("Direct sight restores original body visibility"), Body->GetVisibleFlag());
    TestTrue(TEXT("Direct sight restores updated widget intent"), AuthoredHidden->GetVisibleFlag());
    AuthoredHidden->SetVisibility(false, false);
    Enemy->SetActorLocation(FVector(950, 0, 0));
    System->Tick(.1f);
    TestFalse(TEXT("Retreat transition boundary does not authorize direct sight"), Target->IsDirectlyVisible());
    TestTrue(TEXT("Retreat transition retains spirit perception"), Target->IsSpiritSensed());
    Enemy->SetActorLocation(FVector(600, 0, 0));
    Blocker->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    AActor* Torch = SpawnVisionTestActor(World, FVector(600, 0, 0));
    UPointLightComponent* Light = NewObject<UPointLightComponent>(Torch);
    Torch->AddInstanceComponent(Light);
    Light->SetupAttachment(Torch->GetRootComponent());
    Light->SetIntensity(500.f);
    Light->SetAttenuationRadius(400.f);
    Light->RegisterComponent();
    UYogVisionSourceComponent* Source = NewObject<UYogVisionSourceComponent>(Torch);
    Torch->AddInstanceComponent(Source);
    Source->SetupAttachment(Torch->GetRootComponent());
    Source->RegisterComponent();
    Source->Activate(true);
    Source->BindLight(Light);
    TestEqual(TEXT("Bound torch limits reveal to actual light reach"), Source->GetEffectiveRadius(), 400.f);
    System->Tick(.1f);
    TestTrue(TEXT("Explicit local torch grants direct sight behind observer wall"), Target->IsDirectlyVisible());
    Light->SetIntensity(0.f);
    System->Tick(.1f);
    TestFalse(TEXT("Extinguishing torch removes direct sight"), Target->IsDirectlyVisible());
    TestTrue(TEXT("Extinguishing torch preserves spirit perception"), Target->IsSpiritSensed());
    TestFalse(TEXT("Originally hidden component remains hidden in fog"), AuthoredHidden->GetVisibleFlag());
    Source->bBindOwnerLight = true;
    Source->BindLight(nullptr);
    Light->SetIntensity(500.f);
    TestEqual(TEXT("Details owner binding is resolved lazily"), Source->GetEffectiveRadius(), 400.f);
    Light->SetIntensity(0.f);
    USpotLightComponent* Spot = NewObject<USpotLightComponent>(Torch);
    Torch->AddInstanceComponent(Spot);
    Spot->SetupAttachment(Torch->GetRootComponent());
    Spot->SetIntensity(500.f);
    Spot->RegisterComponent();
    Source->BindLight(Spot);
    TestEqual(TEXT("Spot cone is never treated as circular direct sight"), Source->GetEffectiveRadius(), 0.f);
    Source->BindLight(Light);
    Blocker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    UBoxComponent* HolderCollision = NewObject<UBoxComponent>(Observer);
    Observer->AddInstanceComponent(HolderCollision);HolderCollision->SetupAttachment(Observer->GetRootComponent());
    HolderCollision->SetBoxExtent(FVector(100,100,150));HolderCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    HolderCollision->SetCollisionResponseToAllChannels(ECR_Block);HolderCollision->RegisterComponent();
    Torch->SetOwner(Observer);Torch->AttachToActor(Observer,FAttachmentTransformRules::KeepWorldTransform);
    Torch->SetActorRelativeLocation(FVector(10,0,20));Source->Radius=1000;Light->SetAttenuationRadius(1000);Light->SetIntensity(500);
    Values.BaseSightRadius=0;System->ApplyParameters(Values);
    for(int32 I=0;I<6;++I)System->Tick(.1f);
    TestTrue(TEXT("Carried torch reveals beyond base sight without hitting its holder"),Target->IsDirectlyVisible());
    Light->SetIntensity(0);System->Tick(.1f);
    TestFalse(TEXT("Extinguishing carried torch restores zero base direct sight"),Target->IsDirectlyVisible());
    Enemy->SetActorHiddenInGame(true);
    System->Tick(.1f);
    TestFalse(TEXT("Hidden actor cannot be sensed"), Target->IsSpiritSensed());
    TestFalse(TEXT("Hidden actor clears cached direct sight"), Target->IsDirectlyVisible());
    Enemy->SetActorHiddenInGame(false);
    Target->SetTargetEnabled(false);
    TestTrue(TEXT("Disabling target restores body immediately"), Body->GetVisibleFlag());
    TestFalse(TEXT("Disabling target preserves authored hidden state"), AuthoredHidden->GetVisibleFlag());
    System->Tick(.1f);
    TestFalse(TEXT("Disabled target cannot be directly visible"), Target->IsDirectlyVisible());
    TestFalse(TEXT("Disabled target cannot be spirit sensed"), Target->IsSpiritSensed());
    Enemy->Destroy();
    System->Tick(.1f);
    System->ClearPreview();
    TestFalse(TEXT("Preview cleanup restores prior disabled state"), System->IsEnabled());
    TestEqual(TEXT("Preview cleanup restores prior art values"), System->GetParameters().BaseSightRadius, Before.BaseSightRadius);
    TestNull(TEXT("Preview cleanup releases material instance"), System->GetMaterialInstance());
    GEngine->DestroyWorldContext(World);
    World->DestroyWorld(false);
    return true;
}

#endif
