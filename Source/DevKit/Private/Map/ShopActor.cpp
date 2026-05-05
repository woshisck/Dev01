#include "Map/ShopActor.h"

#include "Character/PlayerCharacterBase.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
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
}

void AShopActor::BeginPlay()
{
	Super::BeginPlay();
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

void AShopActor::OnPlayerBeginOverlap(APlayerCharacterBase* Player)
{
	NearbyPlayer = Player;
	if (Player)
	{
		Player->PendingShop = this;
	}
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
	OnPlayerNearby(Player, false);
	if (ShopWidget && ShopWidget->IsActivated())
	{
		ShopWidget->DeactivateWidget();
	}
}
