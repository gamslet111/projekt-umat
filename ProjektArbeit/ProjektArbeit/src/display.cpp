#include <Arduino.h>
#include <PCF8574.h>
#include <Wire.h>

#include "board_pins.h"
#include "display.h"

constexpr uint8_t OLED = BoardPins::OLED_I2C_ADDRESS;
uint8_t pcf8574Address = BoardPins::PCF8574_I2C_ADDRESS;

// PCF8574 button wiring from the reference game, adapted to this board address:
// OPTIONS (P0): select Players/Cards option
// PLAY  (P1): start dealing
// RIGHT (P2): select Players/Cards option
// PLUS  (P6): increase selected value
// MINUS (P4): decrease selected value
// HOME  (P5): run timed home rotation
// P3/P7 are not used by the reference menu.
constexpr uint8_t BTN_OPTIONS = 0;
constexpr uint8_t BTN_PLAY  = 1;
constexpr uint8_t BTN_RIGHT = 2;
constexpr uint8_t BTN_PLUS  = 6;
constexpr uint8_t BTN_MINUS = 4;
constexpr uint8_t BTN_HOME  = 5;
constexpr unsigned long BUTTON_LOCKOUT_MS = 140;

PCF8574 buttons(BoardPins::PCF8574_I2C_ADDRESS);

const uint8_t buttonPins[] = {BTN_OPTIONS, BTN_PLAY, BTN_RIGHT, BTN_PLUS, BTN_MINUS, BTN_HOME};
const ButtonEvent buttonEvents[] = {BUTTON_RIGHT, BUTTON_PLAY, BUTTON_RIGHT, BUTTON_PLUS, BUTTON_MINUS, BUTTON_HOME};
constexpr uint8_t BUTTON_COUNT = sizeof(buttonPins) / sizeof(buttonPins[0]);

bool buttonsReady = false;
bool oledReady = false;
uint8_t lastButtonByte = 0xFF;
unsigned long lastButtonEventMs = 0;
bool lastStable[BUTTON_COUNT];
bool lastRaw[BUTTON_COUNT];
unsigned long lastChange[BUTTON_COUNT];
uint8_t displayBuffer[8][128];

const uint8_t *glyphFor(char c)
{
    static const uint8_t space[5] = {0x00, 0x00, 0x00, 0x00, 0x00};
    static const uint8_t colon[5] = {0x00, 0x36, 0x36, 0x00, 0x00};
    static const uint8_t slash[5] = {0x20, 0x10, 0x08, 0x04, 0x02};
    static const uint8_t dash[5]  = {0x08, 0x08, 0x08, 0x08, 0x08};
    static const uint8_t gt[5]    = {0x41, 0x22, 0x14, 0x08, 0x00};
    static const uint8_t digits[10][5] = {
        {0x3E, 0x51, 0x49, 0x45, 0x3E}, {0x00, 0x42, 0x7F, 0x40, 0x00},
        {0x42, 0x61, 0x51, 0x49, 0x46}, {0x21, 0x41, 0x45, 0x4B, 0x31},
        {0x18, 0x14, 0x12, 0x7F, 0x10}, {0x27, 0x45, 0x45, 0x45, 0x39},
        {0x3C, 0x4A, 0x49, 0x49, 0x30}, {0x01, 0x71, 0x09, 0x05, 0x03},
        {0x36, 0x49, 0x49, 0x49, 0x36}, {0x06, 0x49, 0x49, 0x29, 0x1E},
    };
    static const uint8_t letters[26][5] = {
        {0x7E, 0x11, 0x11, 0x11, 0x7E}, {0x7F, 0x49, 0x49, 0x49, 0x36},
        {0x3E, 0x41, 0x41, 0x41, 0x22}, {0x7F, 0x41, 0x41, 0x22, 0x1C},
        {0x7F, 0x49, 0x49, 0x49, 0x41}, {0x7F, 0x09, 0x09, 0x09, 0x01},
        {0x3E, 0x41, 0x49, 0x49, 0x7A}, {0x7F, 0x08, 0x08, 0x08, 0x7F},
        {0x00, 0x41, 0x7F, 0x41, 0x00}, {0x20, 0x40, 0x41, 0x3F, 0x01},
        {0x7F, 0x08, 0x14, 0x22, 0x41}, {0x7F, 0x40, 0x40, 0x40, 0x40},
        {0x7F, 0x02, 0x0C, 0x02, 0x7F}, {0x7F, 0x04, 0x08, 0x10, 0x7F},
        {0x3E, 0x41, 0x41, 0x41, 0x3E}, {0x7F, 0x09, 0x09, 0x09, 0x06},
        {0x3E, 0x41, 0x51, 0x21, 0x5E}, {0x7F, 0x09, 0x19, 0x29, 0x46},
        {0x46, 0x49, 0x49, 0x49, 0x31}, {0x01, 0x01, 0x7F, 0x01, 0x01},
        {0x3F, 0x40, 0x40, 0x40, 0x3F}, {0x1F, 0x20, 0x40, 0x20, 0x1F},
        {0x3F, 0x40, 0x38, 0x40, 0x3F}, {0x63, 0x14, 0x08, 0x14, 0x63},
        {0x07, 0x08, 0x70, 0x08, 0x07}, {0x61, 0x51, 0x49, 0x45, 0x43},
    };

    if (c >= 'a' && c <= 'z')
    {
        c -= 32;
    }
    if (c >= '0' && c <= '9')
    {
        return digits[c - '0'];
    }
    if (c >= 'A' && c <= 'Z')
    {
        return letters[c - 'A'];
    }
    if (c == ':')
    {
        return colon;
    }
    if (c == '/')
    {
        return slash;
    }
    if (c == '-')
    {
        return dash;
    }
    if (c == '>')
    {
        return gt;
    }
    return space;
}

bool i2cAddressFound(uint8_t address)
{
    Wire.beginTransmission(address);
    return Wire.endTransmission() == 0;
}

uint8_t findPCF8574Address()
{
    if (i2cAddressFound(BoardPins::PCF8574_I2C_ADDRESS))
    {
        return BoardPins::PCF8574_I2C_ADDRESS;
    }

    // Fallback for boards populated with a different PCF8574 address variant.
    for (uint8_t address = 0x20; address <= 0x27; address++)
    {
        if (i2cAddressFound(address))
        {
            return address;
        }
    }

    for (uint8_t address = 0x38; address <= 0x3F; address++)
    {
        if (i2cAddressFound(address))
        {
            return address;
        }
    }

    return BoardPins::PCF8574_I2C_ADDRESS;
}

bool sendByte(uint8_t control, uint8_t value)
{
    Wire.beginTransmission(OLED);
    Wire.write(control);
    Wire.write(value);
    return Wire.endTransmission() == 0;
}

void oledCommand(uint8_t value)
{
    sendByte(0x00, value);
    delay(2);
}

void oledData(uint8_t value)
{
    sendByte(0x40, value);
}

void setPage(uint8_t page)
{
    oledCommand(0xB0 | (page & 0x07));
    oledCommand(0x02);
    oledCommand(0x10);
}

void clearDisplayBuffer()
{
    memset(displayBuffer, 0x00, sizeof(displayBuffer));
}

void flushDisplayBuffer()
{
    if (!oledReady)
    {
        return;
    }

    for (uint8_t page = 0; page < 8; page++)
    {
        setPage(page);
        for (uint8_t col = 0; col < 128; col++)
        {
            oledData(displayBuffer[page][col]);
        }
    }
}

void drawText(uint8_t page, uint8_t x, const char *text)
{
    if (page >= 8)
    {
        return;
    }

    while (*text != '\0' && x < 123)
    {
        const uint8_t *glyph = glyphFor(*text);
        for (uint8_t col = 0; col < 5 && x < 128; col++)
        {
            displayBuffer[page][x++] = glyph[col];
        }
        if (x < 128)
        {
            displayBuffer[page][x++] = 0x00;
        }
        text++;
    }
}

void fillDisplay(uint8_t pattern)
{
    if (!oledReady)
    {
        return;
    }

    for (uint8_t page = 0; page < 8; page++)
    {
        setPage(page);
        for (uint8_t col = 0; col < 128; col++)
        {
            oledData(pattern);
        }
    }
}

void initSh1106()
{
    oledCommand(0xAE);
    oledCommand(0xD5);
    oledCommand(0x80);
    oledCommand(0xA8);
    oledCommand(0x3F);
    oledCommand(0xD3);
    oledCommand(0x00);
    oledCommand(0x40);
    oledCommand(0xAD);
    oledCommand(0x8B);
    oledCommand(0xA1);
    oledCommand(0xC8);
    oledCommand(0xDA);
    oledCommand(0x12);
    oledCommand(0x81);
    oledCommand(0xFF);
    oledCommand(0xD9);
    oledCommand(0x1F);
    oledCommand(0xDB);
    oledCommand(0x40);
    oledCommand(0xA4);
    oledCommand(0xA6);
    oledCommand(0xAF);
}

void displayInit()
{
    oledReady = i2cAddressFound(OLED);
    pcf8574Address = findPCF8574Address();
    buttons.setAddress(pcf8574Address);
    buttonsReady = buttons.begin(0xFF);
    if (buttonsReady)
    {
        lastButtonByte = buttons.read8();
    }

    Serial.printf("OLED 0x%02X: %s\r\n", OLED, oledReady ? "ok" : "not found");
    Serial.printf("PCF8574 configured 0x%02X, using 0x%02X: %s\r\n",
                  BoardPins::PCF8574_I2C_ADDRESS,
                  pcf8574Address,
                  buttonsReady ? "ok" : "not found");

    if (oledReady)
    {
        initSh1106();
        fillDisplay(0x00);
    }

    for (uint8_t i = 0; i < BUTTON_COUNT; i++)
    {
        lastRaw[i] = buttonsReady ? buttons.readButton(buttonPins[i]) : true;
        lastStable[i] = lastRaw[i];
        lastChange[i] = millis();
    }
}

void displayPowerOn()
{
    if (oledReady)
    {
        oledCommand(0xAF);
    }
}

void displayPowerOff()
{
    if (oledReady)
    {
        oledCommand(0xAE);
    }
}

ButtonEvent readButtonEvent()
{
    if (!buttonsReady)
    {
        return BUTTON_NONE;
    }

    unsigned long currentMillis = millis();
    uint8_t previousButtonByte = lastButtonByte;
    uint8_t currentButtonByte = buttons.read8();
    if (currentButtonByte != lastButtonByte)
    {
        Serial.printf("PCF8574 raw=0b");
        for (int8_t bit = 7; bit >= 0; bit--)
        {
            Serial.print((currentButtonByte & (1 << bit)) ? '1' : '0');
        }
        Serial.printf(" hex=0x%02X, changed P", currentButtonByte);
        for (uint8_t bit = 0; bit < 8; bit++)
        {
            if ((currentButtonByte ^ lastButtonByte) & (1 << bit))
            {
                Serial.print(bit);
                Serial.print(' ');
            }
        }
        Serial.println();
        lastButtonByte = currentButtonByte;
    }

    if (currentMillis - lastButtonEventMs < BUTTON_LOCKOUT_MS)
    {
        return BUTTON_NONE;
    }

    for (uint8_t i = 0; i < BUTTON_COUNT; i++)
    {
        uint8_t mask = 1 << buttonPins[i];
        bool wasReleased = (previousButtonByte & mask) != 0;
        bool isPressed = (currentButtonByte & mask) == 0;

        if (wasReleased && isPressed)
        {
            lastButtonEventMs = currentMillis;
            Serial.printf("Button event P%u\r\n", buttonPins[i]);
            return buttonEvents[i];
        }
    }

    return BUTTON_NONE;
}

void displayBootUI(const char *line1, const char *line2)
{
    clearDisplayBuffer();
    drawText(1, 4, line1);
    drawText(3, 4, line2);
    flushDisplayBuffer();
}

void displayMainUI(int players, int cards, int selectedOption)
{
    char line[22];

    clearDisplayBuffer();
    drawText(0, 18, "CARD DEALER");

    snprintf(line, sizeof(line), "%c PLAYERS: %d", selectedOption == 0 ? '>' : ' ', players);
    drawText(2, 4, line);

    snprintf(line, sizeof(line), "%c CARDS: %d", selectedOption == 1 ? '>' : ' ', cards);
    drawText(3, 4, line);

    drawText(5, 4, "PLAY: DEAL");
    drawText(6, 4, "RIGHT: SELECT");
    drawText(7, 4, "PLUS MINUS HOME");
    flushDisplayBuffer();
}

void displayHomingUI()
{
    clearDisplayBuffer();
    drawText(3, 28, "HOMING");
    flushDisplayBuffer();
}

void displayDealingUI(int currentCard, int totalCards, int currentPlayer, int totalPlayers)
{
    char line[22];

    clearDisplayBuffer();
    drawText(0, 28, "DEALING");

    snprintf(line, sizeof(line), "CARD %d/%d", currentCard, totalCards);
    drawText(3, 4, line);

    snprintf(line, sizeof(line), "PLAYER %d/%d", currentPlayer, totalPlayers);
    drawText(5, 4, line);
    flushDisplayBuffer();
}

void displayDoneUI(int players, int cards)
{
    char line[22];

    clearDisplayBuffer();
    drawText(1, 42, "DONE");
    snprintf(line, sizeof(line), "%d PLAYERS", players);
    drawText(3, 4, line);
    snprintf(line, sizeof(line), "%d CARDS", cards);
    drawText(4, 4, line);
    flushDisplayBuffer();
}

void displayGame(const char *stateText, int activePlayer, const int lives[], int playerCount, int cardsDealt)
{
    char line[22];

    clearDisplayBuffer();
    drawText(0, 4, stateText);

    snprintf(line, sizeof(line), "PLAYER %d/%d", activePlayer + 1, playerCount);
    drawText(2, 4, line);

    snprintf(line, sizeof(line), "CARDS %d", cardsDealt);
    drawText(3, 4, line);

    for (int i = 0; i < playerCount && i < 4; i++)
    {
        snprintf(line, sizeof(line), "P%d LIVES %d", i + 1, lives[i]);
        drawText(4 + i, 4, line);
    }

    flushDisplayBuffer();
}

void displayTestPattern()
{
    fillDisplay(0xFF);
    delay(300);
    fillDisplay(0x00);
    displayBootUI("OLED OK", "RAW SH1106");
}

void scanPossibleI2CPins()
{
    Serial.printf("Configured I2C: SDA=GPIO%d SCL=GPIO%d\r\n", BoardPins::I2C_SDA, BoardPins::I2C_SCL);
    for (uint8_t address = 1; address < 127; address++)
    {
        if (i2cAddressFound(address))
        {
            Serial.printf("  found 0x%02X\r\n", address);
        }
    }
}
