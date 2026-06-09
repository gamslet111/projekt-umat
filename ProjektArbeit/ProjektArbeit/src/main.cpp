#include <Arduino.h>
#include <Wire.h>

#include "board_pins.h"
#include "display.h"
#include "movement.h"

constexpr int MIN_PLAYERS = 2;
constexpr int MAX_PLAYERS = 6;
constexpr int MIN_CARDS = 1;
constexpr int MAX_CARDS = 20;

int selectedOption = 0; // 0 = players, 1 = cards
int numPlayers = 2;
int numCards = 3;
unsigned long fullTurnDurationMs = 0;
bool uiDirty = true;

// Called first in setup so DRV8833 inputs do not float during boot.
static void forceMotorPinsLow()
{
    pinMode(BoardPins::ROTATE_MOTOR_1, OUTPUT);
    pinMode(BoardPins::ROTATE_MOTOR_2, OUTPUT);
    pinMode(BoardPins::CARD_MOTOR_1, OUTPUT);
    pinMode(BoardPins::CARD_MOTOR_2, OUTPUT);
    digitalWrite(BoardPins::ROTATE_MOTOR_1, LOW);
    digitalWrite(BoardPins::ROTATE_MOTOR_2, LOW);
    digitalWrite(BoardPins::CARD_MOTOR_1, LOW);
    digitalWrite(BoardPins::CARD_MOTOR_2, LOW);

}

static void changeSelectedValue(int delta)
{
    if (selectedOption == 0)
    {
        numPlayers = constrain(numPlayers + delta, MIN_PLAYERS, MAX_PLAYERS);
        Serial.printf("Players set to %d\r\n", numPlayers);
    }
    else
    {
        numCards = constrain(numCards + delta, MIN_CARDS, MAX_CARDS);
        Serial.printf("Cards set to %d\r\n", numCards);
    }

    uiDirty = true;
}

static void handleButton(ButtonEvent event)
{
    switch (event)
    {
        case BUTTON_PLAY:
            Serial.printf("PLAY: dealing %d cards to %d players\r\n", numCards, numPlayers);
            dealCards(numPlayers, numCards, fullTurnDurationMs);
            uiDirty = true;
            break;

        case BUTTON_RIGHT:
            selectedOption = selectedOption == 0 ? 1 : 0;
            Serial.printf("SELECT: editing %s\r\n", selectedOption == 0 ? "players" : "cards");
            uiDirty = true;
            break;

        case BUTTON_PLUS:
            Serial.println("PLUS");
            changeSelectedValue(1);
            break;

        case BUTTON_MINUS:
            Serial.println("MINUS");
            changeSelectedValue(-1);
            break;

        case BUTTON_HOME:
            Serial.println("HOME: timed home rotation");
            home();
            delay(300);
            uiDirty = true;
            break;

        case BUTTON_NONE:
        default:
            break;
    }
}

void setup()
{
    forceMotorPinsLow();

    Serial.begin(115200);
    delay(100);

    Serial.println();
    Serial.println("Automatic Card Dealer boot");

    Wire.begin(BoardPins::I2C_SDA, BoardPins::I2C_SCL);
    Wire.setClock(10000);

    displayInit();
    movementInit();
    stopAllMotors();

    displayBootUI("CARD DEALER", "STARTING");
    delay(700);

    home();
    delay(300);
    fullTurnDurationMs = measureFullTurnMs();

    Serial.println("Button map: PLAY=start, RIGHT=select, PLUS/MINUS=change, HOME=timed home");
    uiDirty = true;
}

void loop()
{
    ButtonEvent event = readButtonEvent();
    if (event != BUTTON_NONE)
    {
        handleButton(event);
    }

    static unsigned long lastRefresh = 0;
    if (uiDirty || millis() - lastRefresh >= 1000)
    {
        lastRefresh = millis();
        uiDirty = false;
        displayMainUI(numPlayers, numCards, selectedOption);
    }
}
