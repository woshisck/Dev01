#include "Map/AirWall.h"
#include "Components/BoxComponent.h"

// 与 GA_PlayerDash 保持一致
static const ECollisionChannel DashTraceChannel   = ECC_GameTraceChannel1;
static const ECollisionChannel DashThroughChannel = ECC_GameTraceChannel5;

AAirWall::AAirWall()
{
	PrimaryActorTick.bCanEverTick = false;

	BoxComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxComponent"));
	SetRootComponent(BoxComponent);

	BoxComponent->SetBoxExtent(FVector(50.f, 50.f, 100.f));
	BoxComponent->SetLineThickness(2.f);
}

void AAirWall::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyCollisionSettings();
}

#if WITH_EDITOR
void AAirWall::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	if (PropertyChangedEvent.GetPropertyName() ==
		GET_MEMBER_NAME_CHECKED(AAirWall, bAllowDashThrough))
	{
		ApplyCollisionSettings();
	}
}
#endif

void AAirWall::ApplyCollisionSettings()
{
	if (!BoxComponent) return;

	BoxComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	if (bAllowDashThrough)
	{
		// 物体类型 = DashThrough：
		//   正常状态 Capsule.response[DashThrough] = Block  → 阻挡
		//   冲刺状态 Capsule.response[DashThrough] = Overlap → 可穿越
		// DashTrace 仍 Block → 步进判断能检测到此空气墙，厚度超出步进范围则冲刺停住
		BoxComponent->SetCollisionObjectType(DashThroughChannel);
		BoxComponent->SetCollisionResponseToAllChannels(ECR_Block);
		BoxComponent->ShapeColor = FColor(50, 200, 50); // 绿色 = 可冲刺穿越
	}
	else
	{
		// 实心空气墙：阻挡一切，包含冲刺
		BoxComponent->SetCollisionObjectType(ECC_WorldStatic);
		BoxComponent->SetCollisionResponseToAllChannels(ECR_Block);
		BoxComponent->ShapeColor = FColor(220, 60, 60); // 红色 = 实心阻挡
	}
}
