#include "Map/ShopActor.h"

#include "Character/PlayerCharacterBase.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/LocalPlayer.h"
#include "UI/InteractPromptWidget.h"
#include "UI/ShopSelectionWidget.h"
#include "UI/YogUIManagerSubsystem.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
UYogUIManagerSubsystem* GetShopUIManagerForPlayer(const APlayerCharacterBase* Player)
{
	const APlayerController* PC = Player ? Player->GetController<APlayerController>() : nullptr;
	ULocalPlayer* LocalPlayer = PC ? PC->GetLocalPlayer() : nullptr;
	return LocalPlayer ? LocalPlayer->GetSubsystem<UYogUIManagerSubsystem>() : nullptr;
}
}

AShopActor::AShopActor()
{
	InteractBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractBox"));
	RootComponent = InteractBox;
	InteractBox->InitBoxExtent(FVector(120.0f, 120.0f, 120.0f));
	InteractBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractBox->SetCollisionObjectType(ECC_WorldDynamic);
	InteractBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	InteractBox->SetGenerateOverlapEvents(true);

	ShopMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShopMesh"));
	ShopMesh->SetupAttachment(RootComponent);
	ShopMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ShopMesh->SetRelativeScale3D(FVector(1.3f, 0.75f, 0.55f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (DefaultMesh.Succeeded())
	{
		ShopMesh->SetStaticMesh(DefaultMesh.Object);
	}

	InteractPromptWidgetComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("InteractPromptWidgetComp"));
	InteractPromptWidgetComp->SetupAttachment(RootComponent);
	InteractPromptWidgetComp->SetRelativeLocation(FVector(0.f, 0.f, 150.f));
	InteractPromptWidgetComp->SetWidgetSpace(EWidgetSpace::Screen);
	InteractPromptWidgetComp->SetDrawAtDesiredSize(true);
	InteractPromptWidgetComp->SetVisibility(false);
	InteractPromptWidgetComp->SetWidgetClass(UInteractPromptWidget::StaticClass());
}

void AShopActor::BeginPlay()
{
	Super::BeginPlay();
	ConfigureInteractPrompt();
}

void AShopActor::TryInteract(APlayerCharacterBase* Player)
{
	if (!Player || !ShopData)
	{
		return;
	}

	if (APlayerController* PC = Player->GetController<APlayerController>())
	{
		TSubclassOf<UShopSelectionWidget> WidgetClass = ShopWidgetClass;
		if (!WidgetClass)
		{
			WidgetClass = UShopSelectionWidget::StaticClass();
		}

		if (ULocalPlayer* LocalPlayer = PC->GetLocalPlayer())
		{
			if (UYogUIManagerSubsystem* UIManager = LocalPlayer->GetSubsystem<UYogUIManagerSubsystem>())
			{
				UIManager->SetWidgetClassOverride(EYogUIScreenId::ShopSelection, WidgetClass);
				ShopWidget = Cast<UShopSelectionWidget>(UIManager->EnsureWidget(EYogUIScreenId::ShopSelection));
			}
		}
	}

	if (!ShopWidget)
	{
		return;
	}

	ShopWidget->Setup(ShopData, Player, this);
	if (APlayerController* PC = Player->GetController<APlayerController>())
	{
		if (ULocalPlayer* LocalPlayer = PC->GetLocalPlayer())
		{
			if (UYogUIManagerSubsystem* UIManager = LocalPlayer->GetSubsystem<UYogUIManagerSubsystem>())
			{
				UIManager->PushScreen(EYogUIScreenId::ShopSelection);
				return;
			}
		}
	}
}

void AShopActor::SetShopData(UShopDataAsset* InData)
{
	ShopData = InData;
	ConfigureInteractPrompt();
	SetInteractPromptVisible(NearbyPlayer.IsValid() && ShopData);
}

void AShopActor::OnPlayerBeginOverlap(APlayerCharacterBase* Player)
{
	NearbyPlayer = Player;
	if (Player)
	{
		Player->PendingShop = this;
	}
	SetInteractPromptVisible(Player && ShopData);
	OnPlayerNearby(Player, true);
}

void AShopActor::OnPlayerEndOverlap(APlayerCharacterBase* Player)
{
	if (Player && Player->PendingShop == this)
	{
		Player->PendingShop = nullptr;
	}
	if (NearbyPlayer.Get() == Player)
	{
		NearbyPlayer.Reset();
	}
	SetInteractPromptVisible(false);
	OnPlayerNearby(Player, false);
	if (ShopWidget && ShopWidget->IsActivated())
	{
		if (UYogUIManagerSubsystem* UIManager = GetShopUIManagerForPlayer(Player))
		{
			UIManager->PopScreen(EYogUIScreenId::ShopSelection);
		}
	}
}

void AShopActor::ConfigureInteractPrompt()
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
		PromptWidget->SetPromptLabel(NSLOCTEXT("InteractPrompt", "OpenShop", "打开商店"));
	}
}

void AShopActor::SetInteractPromptVisible(bool bVisible)
{
	if (InteractPromptWidgetComp)
	{
		InteractPromptWidgetComp->SetVisibility(bVisible && ShopData);
	}
}
