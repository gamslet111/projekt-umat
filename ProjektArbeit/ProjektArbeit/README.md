Code for the Liar's-Bar card machine demonstrator.

Main functions:
- Four-player card dealing with one card per player and return to player 1.
- Hall-sensor homing for the rotating plate.
- Debounced PCF8574 input buttons.
- OLED status display with active player, dealt cards and lives.
- Challenge sequence with life loss and optional DFPlayer sounds.

Button mapping on the PCF8574 input board:
- P1: Start
- P5: Stop / Reset / Home
- P4: Back
- P0: Stay / Challenge
- P2: Next

DFPlayer tracks:
- 001.mp3: ready/reset
- 002.mp3: dealing
- 003.mp3: challenge
- 004.mp3: life lost
- 005.mp3: player out
