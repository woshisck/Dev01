#include "Map/ShopActor.h"

#include "Character/PlayerCharacterBase.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "UI/InteractPromptWidget.h"
#include "UI/ShopSelectionWidget.h"
#include "UObject/ConstructorHelpers.h"

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

	if (!ShopWidget || !ShopWidget->IsInViewport())
	{
		TSubclassOf<UShopSelectionWidget> WidgetClass = ShopWidgetClass;
		if (!WidgetClass)
		{
			WidgetClass = UShopSelectionWidget::StaticClass();
		}

		if (APlayerController* PC = Player->GetController<APlayerController>())
		{
			ShopWidget = CreateWidget<UShopSelectionWidget>(PC, WidgetClass);
			if (ShopWidget)
			{
				ShopWidget->AddToViewport(15);
			}
		}
	}

	if (!ShopWidget)
	{
		return;
	}

	ShopWidget->Setup(ShopData, Player, this);
	ShopWidget->ActivateWidget();
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
		ShopWidget->DeactivateWidget();
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
