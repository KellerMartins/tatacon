
#include "switch_controller.h"

#include <Keypad.h>
#include <limits.h>

bool on = false;
bool debug_mode = false;

unsigned long int lastTime;

#define CHANNELS 4
int channelSample[CHANNELS];
int lastChannelSample[CHANNELS];

long int diff[CHANNELS];
bool triggered[CHANNELS];
unsigned int releaseTick[CHANNELS];
unsigned int debounced[CHANNELS];

// ---- Drum pins and settings ----

const int RXLED = 17;

const int DRUM_PINS[] = {A0, A1, A2, A3}; // L kat, L don, R don, R kat
const int DRUM_SENSITIVITY[] = {2, 1, 1, 2};

const int TRIGGER_THRESHOLD = 50;
const int DEBOUNCE_THRESHOLD = 20;
const int HOLD_TICKS = 120;

// ---- Keypad pins and mapping ----
const byte ROWS = 3;
const byte COLS = 4;

// Dummy buttons for special keypad functions
const byte ON_OFF_BUTTON = '@';
const byte KEY_A = 'a';
const byte KEY_B = 'b';
const byte KEY_P = 'p';
const byte KEY_M = 'm';
const byte KEY_RIGHT_ARROW = 'r';
const byte KEY_LEFT_ARROW = 'l';
const byte KEY_UP_ARROW = 'u';
const byte KEY_DOWN_ARROW = 'd';

// Keypad button mapping
const char KEYS[ROWS][COLS] = {
    {KEY_A, KEY_RIGHT_ARROW, KEY_UP_ARROW, KEY_P},
    {KEY_B, KEY_LEFT_ARROW, KEY_DOWN_ARROW, KEY_M},
    {ON_OFF_BUTTON, NO_KEY, NO_KEY, NO_KEY},
};

const byte ROW_PINS[ROWS] = {7, 6, 4};    // connect to the row pinouts of the keypad
const byte COL_PINS[COLS] = {3, 2, 1, 0}; // connect to the column pinouts of the keypad

Keypad keypad = Keypad(makeKeymap(KEYS), ROW_PINS, COL_PINS, ROWS, COLS);
// ----------------

void setup() {
    analogReference(DEFAULT);

    // Change ADC prescaler to speed up analogRead
    // https://gammon.com.au/adc
    ADCSRA &= ~(bit(ADPS0) | bit(ADPS1) | bit(ADPS2)); // clear prescaler bits
    ADCSRA |= bit(ADPS1) | bit(ADPS2);                 //  64

    for (short int i = 0; i < CHANNELS; i++) {
        lastChannelSample[i] = 0;
        releaseTick[i] = HOLD_TICKS;
        triggered[i] = false;
        debounced[i] = true;
    }
    lastTime = 0;

    pinMode(RXLED, OUTPUT);
    digitalWrite(RXLED, LOW);
    SwitchController().releaseHatButton();
}

void loop() {

    for (short int i = 0; i < CHANNELS; i++) {
        lastChannelSample[i] = channelSample[i];
        channelSample[i] = analogRead(DRUM_PINS[i]) / DRUM_SENSITIVITY[i];
        diff[i] = abs(channelSample[i] - lastChannelSample[i]);
    }

    for (short int i = 0; i < CHANNELS; i++) {
        if (!on)
            continue;

        if (!triggered[i]) {
            if (diff[i] >= TRIGGER_THRESHOLD && releaseTick[i] == HOLD_TICKS && debounced[i]) {
                triggered[i] = true;
                debounced[i] = false;
                releaseTick[i] = 0;
                pressDrumKey(i);
            }
        } else {
            if (releaseTick[i] >= HOLD_TICKS) {
                triggered[i] = false;
                releaseDrumKey(i);
            }
        }

        if (diff[i] < DEBOUNCE_THRESHOLD) {
            debounced[i] = true;
        }

        releaseTick[i] = min(releaseTick[i] + 1, HOLD_TICKS);

        if (debug_mode) {
            Serial.print(diff[i]);
            Serial.print("\t");
            Serial.print(triggered[i] ? 100 : 0);
            Serial.print("\t");
        }
        // End of each channel
    }

    if (debug_mode) {
        Serial.print(TRIGGER_THRESHOLD);
        Serial.print("\t");
        Serial.print(DEBOUNCE_THRESHOLD);
        Serial.print("\t");
    }

    handleKeypad();

    unsigned int frameTime = micros() - lastTime;
    lastTime = micros();
    if (debug_mode) {
        Serial.print("(");
        Serial.print(frameTime / 1000.0f);
        Serial.print(")");
    }

    if (debug_mode) {
        Serial.println("");
    }
}

void pressDrumKey(int channel) {
    switch(channel) {
        case 0:
            SwitchController().pressButton(Button::L);
            break;
        case 1:
            SwitchController().pressHatButton(Hat::RIGHT);
            break;
        case 2:
            SwitchController().pressButton(Button::Y);
            break;
        case 3:
            SwitchController().pressButton(Button::X);
            break;
    }
}

void releaseDrumKey(int channel) {
    switch(channel) {
        case 0:
            SwitchController().releaseButton(Button::L);
            break;
        case 1:
            SwitchController().releaseHatButton();
            break;
        case 2:
            SwitchController().releaseButton(Button::Y);
            break;
        case 3:
            SwitchController().releaseButton(Button::X);
            break;
    }
}

void handleKeypad() {
    char pressing = keypad.getKey() != NO_KEY;
    Key key = keypad.key[0];

    switch (key.kchar) {
    case NO_KEY:
        break;
    case (char)ON_OFF_BUTTON:
        if (pressing) {
            on = !on;
            digitalWrite(RXLED, on ? LOW : HIGH);
        } else if (key.kstate == HOLD && !debug_mode) {
            debug_mode = true;
            Serial.begin(115200);
        }
        break;
    case KEY_A:
        if (pressing) {
            SwitchController().pressButton(Button::A);
        } else if (key.kstate == RELEASED) {
            SwitchController().releaseButton(Button::A);
        }
        break;
    case KEY_B:
        if (pressing) {
            SwitchController().pressButton(Button::B);
        } else if (key.kstate == RELEASED) {
            SwitchController().releaseButton(Button::B);
        }
        break;
    case KEY_P:
        if (pressing) {
            SwitchController().pressButton(Button::PLUS);
        } else if (key.kstate == RELEASED) {
            SwitchController().releaseButton(Button::PLUS);
        }
        break;
    case KEY_M:
        if (pressing) {
            SwitchController().pressButton(Button::MINUS);
        } else if (key.kstate == RELEASED) {
            SwitchController().releaseButton(Button::MINUS);
        }
        break;
    case KEY_LEFT_ARROW:
        if (pressing) {
            SwitchController().pressHatButton(Hat::LEFT);
        } else if (key.kstate == RELEASED) {
            SwitchController().releaseHatButton();
        }
        break;
    case KEY_RIGHT_ARROW:
        if (pressing) {
            SwitchController().pressHatButton(Hat::RIGHT);
        } else if (key.kstate == RELEASED) {
            SwitchController().releaseHatButton();
        }
        break;
    case KEY_UP_ARROW:
        if (pressing) {
            SwitchController().pressHatButton(Hat::UP);
        } else if (key.kstate == RELEASED) {
            SwitchController().releaseHatButton();
        }
        break;
    case KEY_DOWN_ARROW:
        if (pressing) {
            SwitchController().pressHatButton(Hat::DOWN);
        } else if (key.kstate == RELEASED) {
            SwitchController().releaseHatButton();
        }
        break;
    }
}
