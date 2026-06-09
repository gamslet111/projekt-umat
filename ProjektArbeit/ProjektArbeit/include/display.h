#pragma once

#include <Arduino.h>

enum ButtonEvent
{
    BUTTON_NONE,
    BUTTON_PLAY,
    BUTTON_RIGHT,
    BUTTON_PLUS,
    BUTTON_MINUS,
    BUTTON_HOME
};

void displayInit();
ButtonEvent readButtonEvent();
void displayBootUI(const char *line1, const char *line2);
void displayMainUI(int players, int cards, int selectedOption);
void displayHomingUI();
void displayDealingUI(int currentCard, int totalCards, int currentPlayer, int totalPlayers);
void displayDoneUI(int players, int cards);
void displayGame(const char *stateText, int activePlayer, const int lives[], int playerCount, int cardsDealt);
void displayTestPattern();
void scanPossibleI2CPins();
void displayPowerOn();
void displayPowerOff();
