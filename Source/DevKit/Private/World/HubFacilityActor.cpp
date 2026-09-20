#include "World/HubFacilityActor.h"
#include "Character/PlayerCharacterBase.h"
#include "Components/BoxComponent.h"
#include "Components/WidgetComponent.h"
#include "CommonActivatableWidget.h"
#include "Blueprint/UserWidget.h"
#include "UI/InteractPromptWidget.h"
#include "UI/YogHUD.h"
#include "MetaProgression/YogMetaProgressionSubsystem.h"

AHubFacilityActor::AHubFacilityActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;

	InteractBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractBox"));
	InteractBox->SetBoxExtent(FVector(120.f, 120.f, 90.f));
	InteractBox->SetCollisionProfileName(TEXT("Trigger"));
	RootComponent = InteractBox;

	FacilityDisplayName = NSLOCTEXT("HubFacility", "UpgradeTerminal", "升级终端");

	InteractPromptWidgetComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("InteractPromptWidgetComp"));
	InteractPromptWidgetComp->SetupAttachment(RootComponent);
	InteractPromptWidgetComp->SetRelativeLocation(FVector(0.f, 0.f, 150.f));
	InteractPromptWidgetComp->SetWidgetSpace(EWidgetSpace::Screen);
	InteractPromptWidgetComp->SetDrawAtDesiredSize(true);
	InteractPromptWidgetComp->SetVisibility(false);
	InteractPromptWidgetComp->SetWidgetClass(UInteractPromptWidget::StaticClass());
}

void AHubFacilityActor::ConfigureInteractPrompt()
{
	if (!InteractPromptWidgetComp)
	{
		return;
	}

	if (APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
	{
		InteractPromptWidgetComp->SetOwnerPlayer(PC->GetLocalPlayer());
	}

	InteractPromptWidgetComp->SetWidgetClass(UInteractPromptWidget::StaticClass());
	InteractPromptWidgetComp->InitWidget();
	if (UInteractPromptWidget* PromptWidget = Cast<UInteractPromptWidget>(InteractPromptWidgetComp->GetWidget()))
	{
		PromptWidget->SetPromptLabel(FacilityDisplayName);
	}
}

void AHubFacilityActor::SetInteractPromptVisible(bool bVisible)
{
	if (InteractPromptWidgetComp)
	{
		InteractPromptWidgetComp->SetVisibility(bVisible);
	}
}

void AHubFacilityActor::SetInteractHoldProgress(float Normalized)
{
	if (!InteractPromptWidgetComp) return;

	if (UInteractPromptWidget* PromptWidget = Cast<UInteractPromptWidget>(InteractPromptWidgetComp->GetWidget()))
	{
		PromptWidget->SetHoldProgress(Normalized);
	}
}

void AHubFacilityActor::BeginPlay()
{
	Super::BeginPlay();
	InteractBox->OnComponentBeginOverlap.AddDynamic(this, &AHubFacilityActor::HandleBeginOverlap);
	InteractBox->OnComponentEndOverlap.AddDynamic(this,   &AHubFacilityActor::HandleEndOverlap);
	ConfigureInteractPrompt();
	ApplyFeatureAvailability();

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UYogMetaProgressionSubsystem* Meta = GI->GetSubsystem<UYogMetaProgressionSubsystem>())
		{
			Meta->OnFeatureUnlocked.AddDynamic(this, &AHubFacilityActor::HandleFeatureUnlocked);
		}
	}
}

void AHubFacilityActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UYogMetaProgressionSubsystem* Meta = GI->GetSubsystem<UYogMetaProgressionSubsystem>())
		{
			Meta->OnFeatureUnlocked.RemoveDynamic(this, &AHubFacilityActor::HandleFeatureUnlocked);
		}
	}

	Super::EndPlay(EndPlayReason);
}

void AHubFacilityActor::Interact(APlayerCharacterBase* Player)
{
	if (!IsFeatureAvailable())
	{
		return;
	}

	BP_OnInteract(Player);

	if (!WidgetClass) return;

	APlayerController* PC = Player ? Player->GetController<APlayerController>() : nullptr;
	if (!PC) return;

	if (UCommonActivatableWidget* Widget = CreateWidget<UCommonActivatableWidget>(PC, WidgetClass))
	{
		Widget->AddToViewport();
		Widget->ActivateWidget();
	}
}

bool AHubFacilityActor::IsFeatureAvailable() const
{
	if (!RequiredFeatureTag.IsValid())
	{
		return true;
	}

	const UGameInstance* GI = GetGameInstance();
	const UYogMetaProgressionSubsystem* Meta = GI ? GI->GetSubsystem<UYogMetaProgressionSubsystem>() : nullptr;
	return Meta && Meta->IsFeatureUnlocked(RequiredFeatureTag);
}

void AHubFacilityActor::ApplyFeatureAvailability()
{
	const bool bAvailable = IsFeatureAvailable();
	SetActorHiddenInGame(!bAvailable);
	SetActorEnableCollision(bAvailable);
	if (InteractBox)
	{
		InteractBox->SetGenerateOverlapEvents(bAvailable);
	}

	if (RequiredFeatureTag.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[HubFacility] %s availability=%d required=%s hidden=%d"),
			*GetNameSafe(this),
			bAvailable ? 1 : 0,
			*RequiredFeatureTag.ToString(),
			IsHidden() ? 1 : 0);
	}
}

void AHubFacilityActor::HandleBeginOverlap(UPrimitiveComponent* OverlappedComponent,
                                            AActor* OtherActor, UPrimitiveComponent* OtherComp,
                                            int32 OtherBodyIndex, bool bFromSweep,
                                            const FHitResult& SweepResult)
{
	if (APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(OtherActor))
	{
		if (IsFeatureAvailable())
		{
			Player->PendingFacility = this;
			SetInteractPromptVisible(true);
		}
	}
}

void AHubFacilityActor::HandleEndOverlap(UPrimitiveComponent* OverlappedComponent,
                                          AActor* OtherActor, UPrimitiveComponent* OtherComp,
                                          int32 OtherBodyIndex)
{
	if (APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(OtherActor))
	{
		if (Player->PendingFacility == this)
		{
			Player->PendingFacility = nullptr;
		}
		SetInteractPromptVisible(false);
	}
}

void AHubFacilityActor::HandleFeatureUnlocked(FGameplayTag FeatureTag)
{
	if (!RequiredFeatureTag.IsValid() || FeatureTag == RequiredFeatureTag)
	{
		ApplyFeatureAvailability();
	}
}
