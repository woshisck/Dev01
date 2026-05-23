#include "Water/WaterCausticsDecalActor.h"

#include "Components/DecalComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

AWaterCausticsDecalActor::AWaterCausticsDecalActor()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	CausticsDecal = CreateDefaultSubobject<UDecalComponent>(TEXT("CausticsDecal"));
	CausticsDecal->SetupAttachment(SceneRoot);
	CausticsDecal->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
	CausticsDecal->DecalSize = FVector(64.0f, 600.0f, 600.0f);
}

void AWaterCausticsDecalActor::BeginPlay()
{
	Super::BeginPlay();

	if (CausticsMaterial)
	{
		CausticsMID = UMaterialInstanceDynamic::Create(CausticsMaterial, this);
		CausticsDecal->SetDecalMaterial(CausticsMID);
	}
	else
	{
		CausticsMID = CausticsDecal->CreateDynamicMaterialInstance();
	}

	SetCausticsIntensity(CausticsIntensity);
}

void AWaterCausticsDecalActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	ElapsedTime += FMath::Max(0.0f, DeltaSeconds);
	if (CausticsMID)
	{
		CausticsMID->SetScalarParameterValue(TEXT("CausticsTime"), ElapsedTime);
		CausticsMID->SetScalarParameterValue(TEXT("CausticsScale"), CausticsScale);
		CausticsMID->SetVectorParameterValue(TEXT("CausticsScrollSpeed"), FLinearColor(CausticsScrollSpeed.X, CausticsScrollSpeed.Y, 0.0f, 0.0f));
	}
}

void AWaterCausticsDecalActor::SetCausticsIntensity(float NewIntensity)
{
	CausticsIntensity = FMath::Max(0.0f, NewIntensity);
	if (CausticsMID)
	{
		CausticsMID->SetScalarParameterValue(TEXT("CausticsIntensity"), CausticsIntensity);
	}
}
