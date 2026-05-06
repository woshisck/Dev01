#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "UI/CombatDeckEditWidget.h"

namespace
{
	FCombatCardInstance MakeDeckEditHintTestCard(
		ECombatCardType CardType,
		ECombatCardLinkOrientation LinkOrientation = ECombatCardLinkOrientation::Forward)
	{
		FCombatCardInstance Card;
		Card.Config.CardType = CardType;
		Card.LinkOrientation = LinkOrientation;
		return Card;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckEditLinkHintStatesPairForwardAndReversedTest,
	"DevKit.UI.CombatDeckEdit.LinkHintStatesPairForwardAndReversed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckEditLinkHintStatesPairForwardAndReversedTest::RunTest(const FString& Parameters)
{
	const TArray<FCombatCardInstance> Cards = {
		MakeDeckEditHintTestCard(ECombatCardType::Attack),
		MakeDeckEditHintTestCard(ECombatCardType::Link, ECombatCardLinkOrientation::Forward),
		MakeDeckEditHintTestCard(ECombatCardType::Link, ECombatCardLinkOrientation::Reversed),
		MakeDeckEditHintTestCard(ECombatCardType::Attack),
	};

	const TArray<ECombatDeckEditCardLinkHintState> States =
		UCombatDeckEditWidget::BuildLinkHintStatesForDeck(Cards);

	TestEqual(TEXT("Hint state count follows deck count"), States.Num(), Cards.Num());
	TestEqual(TEXT("Forward link marks previous card as target"),
		States[0], ECombatDeckEditCardLinkHintState::ForwardTarget);
	TestEqual(TEXT("Forward link marks link card"),
		States[1], ECombatDeckEditCardLinkHintState::ForwardLink);
	TestEqual(TEXT("Reversed link marks link card"),
		States[2], ECombatDeckEditCardLinkHintState::ReversedLink);
	TestEqual(TEXT("Reversed link marks next card as target"),
		States[3], ECombatDeckEditCardLinkHintState::ReversedTarget);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatDeckEditLinkHintStatesIgnoreEdgeLinksTest,
	"DevKit.UI.CombatDeckEdit.LinkHintStatesIgnoreEdgeLinks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatDeckEditLinkHintStatesIgnoreEdgeLinksTest::RunTest(const FString& Parameters)
{
	const TArray<FCombatCardInstance> Cards = {
		MakeDeckEditHintTestCard(ECombatCardType::Link, ECombatCardLinkOrientation::Forward),
		MakeDeckEditHintTestCard(ECombatCardType::Attack),
		MakeDeckEditHintTestCard(ECombatCardType::Link, ECombatCardLinkOrientation::Reversed),
	};

	const TArray<ECombatDeckEditCardLinkHintState> States =
		UCombatDeckEditWidget::BuildLinkHintStatesForDeck(Cards);

	TestEqual(TEXT("Hint state count follows deck count"), States.Num(), Cards.Num());
	TestEqual(TEXT("Forward link at deck start has no target"),
		States[0], ECombatDeckEditCardLinkHintState::None);
	TestEqual(TEXT("Middle attack is not linked by invalid edge links"),
		States[1], ECombatDeckEditCardLinkHintState::None);
	TestEqual(TEXT("Reversed link at deck end has no target"),
		States[2], ECombatDeckEditCardLinkHintState::None);

	return true;
}

#endif
