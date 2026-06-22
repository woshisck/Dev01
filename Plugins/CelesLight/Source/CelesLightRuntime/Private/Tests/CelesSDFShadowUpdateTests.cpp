#include "Misc/AutomationTest.h"

#include "Components/CelesSDFShadowUpdateComponent.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCelesSDFShadowUpdateDirectionByAxisTest,
	"CelesLight.SDFShadow.DirectionByAxis",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCelesSDFShadowUpdateDirectionByAxisTest::RunTest(const FString& Parameters)
{
	const FRotator Rotation(0.0, 90.0, 0.0);

	TestTrue(TEXT("X axis uses forward vector"), UCelesSDFShadowUpdateComponent::GetDirectionByAxis(Rotation, EAxis::X, false).Equals(FVector::YAxisVector));
	TestTrue(TEXT("Y axis uses right vector"), UCelesSDFShadowUpdateComponent::GetDirectionByAxis(Rotation, EAxis::Y, false).Equals(-FVector::XAxisVector));
	TestTrue(TEXT("Z axis uses up vector"), UCelesSDFShadowUpdateComponent::GetDirectionByAxis(Rotation, EAxis::Z, false).Equals(FVector::ZAxisVector));
	TestTrue(TEXT("Invert flips direction"), UCelesSDFShadowUpdateComponent::GetDirectionByAxis(Rotation, EAxis::X, true).Equals(-FVector::YAxisVector));
	TestTrue(TEXT("None returns zero vector"), UCelesSDFShadowUpdateComponent::GetDirectionByAxis(Rotation, EAxis::None, false).IsNearlyZero());

	return true;
}

#endif
