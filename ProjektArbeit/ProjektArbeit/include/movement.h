#pragma once

#include <Arduino.h>

void movementInit();
bool home();
unsigned long measureFullTurnMs();
void dealCards(int numPlayers, int numCards, unsigned long fullTurnDurationMs);
void shootCard();
void rotateToNextPlayer();
void rotateToPreviousPlayer();
void rotateFor(unsigned long durationMs, bool clockwise);
void stopAllMotors();
void motorSelfTest();
void runBothMotorsForward();
void runRotationMotorForward();
void runCardMotorForward();
void startRotationMotorForward();
void startCardMotorForward();
