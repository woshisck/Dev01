#include "Components/StylizedCharacterShadowPolicyComponent.h"

#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
	uint32 ReadNativeShadowFlags(const USkeletalMeshComponent* Mesh)
	{
		return (Mesh->CastShadow ? 1u : 0u)
			| (Mesh->bCastShadowOnOtherObjectsOnly ? 2u : 0u)
			| (Mesh->bCastContactShadow ? 4u : 0u)
			| (Mesh->bCastCapsuleDirectShadow ? 8u : 0u)
			| (Mesh->bCastInsetShadow ? 16u : 0u)
			| (Mesh->bSelfShadowOnly ? 32u : 0u)
			| (Mesh->bCastDynamicShadow ? 64u : 0u)
			| (Mesh->bCastStaticShadow ? 128u : 0u);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYogCloudShadowPolicyExplicitTest,
	"CelesLight.CharacterLighting.CloudShadowPolicyExplicitTargets", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FYogCloudShadowPolicyExplicitTest::RunTest(const FString&)
{
	UStylizedCharacterShadowPolicyComponent* Policy = NewObject<UStylizedCharacterShadowPolicyComponent>();
	USkeletalMeshComponent* Mesh = NewObject<USkeletalMeshComponent>();
	TestTrue(TEXT("Cloud policy enables the environment-only native shadow split by default"), Policy->bEnvironmentOnlyNativeShadow);
	Mesh->SetCastShadow(false);
	Mesh->SetCastContactShadow(true);
	Mesh->SetCastCapsuleDirectShadow(true);
	const uint32 UnrelatedFlags = ReadNativeShadowFlags(Mesh) & ~15u;
	Policy->ExplicitTargetComponents.Add(Mesh);
	Policy->ExplicitTargetComponents.Add(Mesh);
	Policy->ExplicitTargetComponents.Add(nullptr);
	Policy->ApplyShadowPolicy();
	TestTrue(TEXT("Native environment casting remains enabled"), Mesh->CastShadow);
	TestTrue(TEXT("Native projection is suppressed only on the same character mesh"), Mesh->bCastShadowOnOtherObjectsOnly);
	TestFalse(TEXT("Contact self-shadows are disabled by the cloud default"), Mesh->bCastContactShadow);
	TestFalse(TEXT("Capsule direct self-shadows are disabled by the cloud default"), Mesh->bCastCapsuleDirectShadow);
	TestEqual(TEXT("Unrelated authored shadow flags remain unchanged"), ReadNativeShadowFlags(Mesh) & ~15u, UnrelatedFlags);
	const uint32 AppliedFlags = ReadNativeShadowFlags(Mesh);
	Policy->ApplyShadowPolicy();
	TestEqual(TEXT("Applying the cloud policy again is idempotent"), ReadNativeShadowFlags(Mesh), AppliedFlags);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYogCloudShadowPolicyRegisteredTest,
	"CelesLight.CharacterLighting.CloudShadowPolicyRegisteredAutoTargets", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FYogCloudShadowPolicyRegisteredTest::RunTest(const FString&)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Editor, false);
	if (!TestNotNull(TEXT("Transient test world"), World)) return false;
	struct FDestroyTestWorld { UWorld* World; ~FDestroyTestWorld() { World->DestroyWorld(false); } } Destroy{World};
	AActor* Owner = World->SpawnActor<AActor>();
	if (!TestNotNull(TEXT("Transient mesh owner"), Owner)) return false;
	USkeletalMeshComponent* Mesh = NewObject<USkeletalMeshComponent>(Owner);
	Owner->AddInstanceComponent(Mesh);
	Mesh->SetCastShadow(false);
	Mesh->SetCastContactShadow(true);
	Mesh->SetCastCapsuleDirectShadow(true);
	Mesh->RegisterComponent();
	const uint32 UnrelatedFlags = ReadNativeShadowFlags(Mesh) & ~15u;
	UStylizedCharacterShadowPolicyComponent* Policy = NewObject<UStylizedCharacterShadowPolicyComponent>(Owner);
	Owner->AddInstanceComponent(Policy);
	Policy->bEnvironmentOnlyNativeShadow = true;
	Policy->bDisableContactShadows = true;
	Policy->bDisableCapsuleDirectShadows = true;
	Policy->RegisterComponent();
	TestTrue(TEXT("Registration finds the owner's skinned mesh and enables native casting"), Mesh->CastShadow);
	TestTrue(TEXT("Registration applies the published caster/receiver split"), Mesh->bCastShadowOnOtherObjectsOnly);
	TestFalse(TEXT("Registration disables contact self-shadows"), Mesh->bCastContactShadow);
	TestFalse(TEXT("Registration disables capsule direct self-shadows"), Mesh->bCastCapsuleDirectShadow);
	TestEqual(TEXT("Registration preserves unrelated native flags"), ReadNativeShadowFlags(Mesh) & ~15u, UnrelatedFlags);
	USkeletalMeshComponent* ModularPart = NewObject<USkeletalMeshComponent>(Owner);
	Owner->AddInstanceComponent(ModularPart);
	ModularPart->SetCastContactShadow(true);
	ModularPart->RegisterComponent();
	Policy->ApplyShadowPolicy();
	TestTrue(TEXT("Reapplying discovers new modular character parts"), ModularPart->bCastShadowOnOtherObjectsOnly);
	TestFalse(TEXT("New modular parts receive the contact-shadow policy"), ModularPart->bCastContactShadow);
	Policy->bEnvironmentOnlyNativeShadow = false;
	Policy->ApplyShadowPolicy();
	TestFalse(TEXT("Disabling the split clears the other-objects-only flag"), Mesh->bCastShadowOnOtherObjectsOnly);
	TestFalse(TEXT("The published policy does not invent a previous contact-shadow value"), Mesh->bCastContactShadow);
	TestFalse(TEXT("The published policy does not invent a previous capsule-shadow value"), Mesh->bCastCapsuleDirectShadow);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYogCloudShadowPolicyOptionalPathsTest,
	"CelesLight.CharacterLighting.CloudShadowPolicyOptionalShadowPaths", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FYogCloudShadowPolicyOptionalPathsTest::RunTest(const FString&)
{
	UStylizedCharacterShadowPolicyComponent* Policy = NewObject<UStylizedCharacterShadowPolicyComponent>();
	USkeletalMeshComponent* Mesh = NewObject<USkeletalMeshComponent>();
	Mesh->SetCastContactShadow(true);
	Mesh->SetCastCapsuleDirectShadow(true);
	Policy->ExplicitTargetComponents.Add(Mesh);
	Policy->bDisableContactShadows = false;
	Policy->bDisableCapsuleDirectShadows = false;
	Policy->ApplyShadowPolicy();
	TestTrue(TEXT("Contact shadows can be retained explicitly"), Mesh->bCastContactShadow);
	TestTrue(TEXT("Capsule direct shadows can be retained explicitly"), Mesh->bCastCapsuleDirectShadow);
	Policy->bEnvironmentOnlyNativeShadow = false;
	Mesh->SetCastShadow(false);
	const uint32 AuthoredFlags = ReadNativeShadowFlags(Mesh) & ~2u;
	Policy->bDisableContactShadows = true;
	Policy->bDisableCapsuleDirectShadows = true;
	Policy->ApplyShadowPolicy();
	TestEqual(TEXT("Without the split, casting/contact/capsule and unrelated authored flags stay untouched"), ReadNativeShadowFlags(Mesh), AuthoredFlags);
	return true;
}
#endif
