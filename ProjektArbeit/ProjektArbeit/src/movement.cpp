#include <Arduino.h>

#include "board_pins.h"
#include "display.h"
#include "movement.h"

constexpr unsigned long DEFAULT_FULL_TURN_MS = 3200; // ADJUST: rotation duration in ms for one full table turn
constexpr unsigned long CARD_FORWARD_MS      = 210;  // ADJUST: card feed forward duration in ms
constexpr unsigned long CARD_REVERSE_MS      = 210;  // ADJUST: card feed reverse duration in ms
constexpr unsigned long PLAYER_PAUSE_MS      = 100;  // ADJUST: pause after each player rotation in ms
constexpr unsigned long HOME_TURN_MS         = 500;  // ADJUST: rotation duration in ms for manual home action
constexpr unsigned long TEST_MOTOR_MS        = 700;  // ADJUST: motor self-test duration in ms

unsigned long fullTurnMs = DEFAULT_FULL_TURN_MS;

static unsigned long playerRotationMs(int numPlayers, unsigned long fullTurnDurationMs)
{
    const int offsetsByPlayerCount[7] = {70, 80, 95, 100, 100, 85, 75};
    int offset = 0;

    if (numPlayers >= 2 && numPlayers <= 8)
    {
        offset = offsetsByPlayerCount[numPlayers - 2];
    }

    long duration = static_cast<long>(fullTurnDurationMs / numPlayers) - offset;
    return duration > 0 ? static_cast<unsigned long>(duration) : 1;
}

void stopAllMotors()
{
    digitalWrite(BoardPins::ROTATE_MOTOR_1, LOW);
    digitalWrite(BoardPins::ROTATE_MOTOR_2, LOW);
    digitalWrite(BoardPins::CARD_MOTOR_1,   LOW);
    digitalWrite(BoardPins::CARD_MOTOR_2,   LOW);
}

void movementInit()
{
    pinMode(BoardPins::ROTATE_MOTOR_1, OUTPUT);
    pinMode(BoardPins::ROTATE_MOTOR_2, OUTPUT);
    pinMode(BoardPins::CARD_MOTOR_1,   OUTPUT);
    pinMode(BoardPins::CARD_MOTOR_2,   OUTPUT);
    stopAllMotors();
}

void stopRotation()
{
    digitalWrite(BoardPins::ROTATE_MOTOR_1, LOW);
    digitalWrite(BoardPins::ROTATE_MOTOR_2, LOW);
}

void rotateClockwise()
{
    digitalWrite(BoardPins::ROTATE_MOTOR_1, HIGH);
    digitalWrite(BoardPins::ROTATE_MOTOR_2, LOW);
}

void rotateCounterClockwise()
{
    digitalWrite(BoardPins::ROTATE_MOTOR_1, LOW);
    digitalWrite(BoardPins::ROTATE_MOTOR_2, HIGH);
}

bool home()
{
    displayHomingUI();

    // No hall effect sensor is installed on this hardware.
    rotateFor(HOME_TURN_MS, true); // ADJUST: rotation duration in ms
    stopAllMotors();

    Serial.println("Timed home action done. Set Player 1 mechanically if needed.");
    return true;
}

unsigned long measureFullTurnMs()
{
    Serial.printf("Using timed full turn=%lu ms\r\n", fullTurnMs);
    return fullTurnMs;
}

void shootCard()
{
    digitalWrite(BoardPins::CARD_MOTOR_1, HIGH);
    digitalWrite(BoardPins::CARD_MOTOR_2, LOW);
    delay(CARD_FORWARD_MS);

    digitalWrite(BoardPins::CARD_MOTOR_1, LOW);
    digitalWrite(BoardPins::CARD_MOTOR_2, HIGH);
    delay(CARD_REVERSE_MS);

    digitalWrite(BoardPins::CARD_MOTOR_1, LOW);
    digitalWrite(BoardPins::CARD_MOTOR_2, LOW);
}

void rotateFor(unsigned long durationMs, bool clockwise)
{
    if (clockwise)
    {
        rotateClockwise();
    }
    else
    {
        rotateCounterClockwise();
    }

    delay(durationMs); // ADJUST: rotation duration in ms
    stopRotation();
    delay(PLAYER_PAUSE_MS);
}

void rotateToNextPlayer()
{
    rotateFor(fullTurnMs / 4, true); // ADJUST: rotation duration in ms
}

void rotateToPreviousPlayer()
{
    rotateFor(fullTurnMs / 4, false); // ADJUST: rotation duration in ms
}

void dealCards(int numPlayers, int numCards, unsigned long fullTurnDurationMs)
{
    const unsigned long cardIntervalMs = playerRotationMs(numPlayers, fullTurnDurationMs); // ADJUST: rotation duration in ms

    Serial.printf("Deal start: players=%d cards=%d player interval=%lu ms\r\n",
                  numPlayers,
                  numCards,
                  cardIntervalMs);

    for (int card = 0; card < numCards; card++)
    {
        for (int player = 0; player < numPlayers; player++)
        {
            displayDealingUI(card + 1, numCards, player + 1, numPlayers);
            shootCard();
            rotateFor(cardIntervalMs, true); // ADJUST: rotation duration in ms
        }
    }

    home();
    delay(300);
    displayDoneUI(numPlayers, numCards);
}

void motorSelfTest()
{
    Serial.println("Motor self-test: rotation clockwise");
    rotateFor(TEST_MOTOR_MS, true);
    delay(250);

    Serial.println("Motor self-test: rotation counter-clockwise");
    rotateFor(TEST_MOTOR_MS, false);
    delay(250);

    Serial.println("Motor self-test: card motor");
    shootCard();
    delay(250);

    stopAllMotors();
    Serial.println("Motor self-test done");
}

void runBothMotorsForward()
{
    digitalWrite(BoardPins::ROTATE_MOTOR_1, HIGH);
    digitalWrite(BoardPins::ROTATE_MOTOR_2, LOW);
    digitalWrite(BoardPins::CARD_MOTOR_1,   HIGH);
    digitalWrite(BoardPins::CARD_MOTOR_2,   LOW);
}

void runRotationMotorForward()
{
    digitalWrite(BoardPins::ROTATE_MOTOR_1, HIGH);
    digitalWrite(BoardPins::ROTATE_MOTOR_2, LOW);
    digitalWrite(BoardPins::CARD_MOTOR_1,   LOW);
    digitalWrite(BoardPins::CARD_MOTOR_2,   LOW);
}

void runCardMotorForward()
{
    digitalWrite(BoardPins::ROTATE_MOTOR_1, LOW);
    digitalWrite(BoardPins::ROTATE_MOTOR_2, LOW);
    digitalWrite(BoardPins::CARD_MOTOR_1,   HIGH);
    digitalWrite(BoardPins::CARD_MOTOR_2,   LOW);
}

void startRotationMotorForward()
{
    digitalWrite(BoardPins::ROTATE_MOTOR_1, HIGH);
    digitalWrite(BoardPins::ROTATE_MOTOR_2, LOW);
}

void startCardMotorForward()
{
    digitalWrite(BoardPins::CARD_MOTOR_1, HIGH);
    digitalWrite(BoardPins::CARD_MOTOR_2, LOW);
}
