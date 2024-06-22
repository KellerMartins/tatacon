#include <limits.h>
#include <HID-Project.h>
#include <HID-Settings.h>
#include <Keypad.h>

bool on = false;
bool debug_mode = false;

unsigned long int lastTime;

#define CHANNELS 4
int channelSample [CHANNELS];
int lastChannelSample [CHANNELS];

long int diff [CHANNELS];
bool triggered [CHANNELS];
unsigned int releaseTick [CHANNELS];

// ---- Drum pins and settings ----

const int RXLED = 17;

const int DRUM_PINS[] = {A0, A1, A2, A3};  // L don, R don, L kat, R kat
const char DRUM_KEYS[] = {'d', 'f', 'j', 'k'};
const int DRUM_SENSITIVITY[] = {2, 1, 1, 3};

const int THRESHOLD = 60;
const int DEBOUNCE_TICKS  = 60;

// ---- Keypad pins and mapping ----
const byte ROWS = 3;
const byte COLS = 4;

// Dummy buttons for special keypad functions
const byte ON_OFF_BUTTON = '@';
const byte VOLUME_UP_BUTTON = '[';
const byte VOLUME_DOWN_BUTTON = ']';

// Keypad button mapping
const char KEYS[ROWS][COLS] = {
  {KEY_RETURN,KEY_RIGHT_ARROW,KEY_UP_ARROW,KEY_LEFT_CTRL},
  {KEY_ESC,KEY_LEFT_ARROW,KEY_DOWN_ARROW,KEY_F2},
  {ON_OFF_BUTTON, NO_KEY, VOLUME_UP_BUTTON, VOLUME_DOWN_BUTTON},
};

const byte ROW_PINS[ROWS] = {7, 6, 4}; //connect to the row pinouts of the keypad
const byte COL_PINS[COLS] = {3, 2, 1, 0}; //connect to the column pinouts of the keypad

Keypad keypad = Keypad( makeKeymap(KEYS), ROW_PINS, COL_PINS, ROWS, COLS );
// ----------------

void setup() {
  Keyboard.begin ();
  analogReference (DEFAULT);
  
  // Change ADC prescaler to speed up analogRead
  // https://gammon.com.au/adc
  ADCSRA &= ~(bit (ADPS0) | bit (ADPS1) | bit (ADPS2)); // clear prescaler bits 
  ADCSRA |= bit (ADPS1) | bit (ADPS2);                 //  64 
  
  for (short int i = 0; i < CHANNELS; i++) {
    lastChannelSample[i] = 0;
    releaseTick[i] = DEBOUNCE_TICKS;
    triggered[i] = false;
  }
  lastTime = 0;

  pinMode(RXLED, OUTPUT);
  digitalWrite(RXLED, LOW);
  Consumer.begin();
}

void loop() {

  for (short int i = 0; i < CHANNELS; i++) {
    lastChannelSample[i] = channelSample[i];
    channelSample[i] = analogRead(DRUM_PINS[i]) / DRUM_SENSITIVITY[i];
    diff[i] = abs(channelSample[i] - lastChannelSample[i]);
  }

  bool don = releaseTick[1] != DEBOUNCE_TICKS || releaseTick[2] != DEBOUNCE_TICKS;
  
  for (short int i = 0; i < CHANNELS; i++) {
    if (debug_mode){
      Serial.print(diff[i]);
      Serial.print("\t");
    }
    
    if (!on) continue;
    
    if (!triggered [i]) {
      if (diff[i] >= THRESHOLD && releaseTick[i] == DEBOUNCE_TICKS && !(don && (i == 0 || i == 3))){
       
        triggered[i] = true;
        Keyboard.press(DRUM_KEYS[i]);
      } else {
        releaseTick[i] = min(releaseTick[i]+1, DEBOUNCE_TICKS);
      }
    } else {
      if (diff[i] < THRESHOLD && releaseTick[i] == DEBOUNCE_TICKS){
        triggered[i] = false;
        releaseTick[i] = 0;
        Keyboard.release(DRUM_KEYS[i]);
      }
    }
  // End of each channel
  }

  if (debug_mode) {
    Serial.print (THRESHOLD);
    Serial.print ("\t");
  }

  handleKeypad();

  unsigned int frameTime = micros () - lastTime;
  lastTime = micros ();
  if (debug_mode) {
    Serial.print ("(");
    Serial.print(frameTime / 1000.0f);
    Serial.print (")");
  }

  if (debug_mode) {
    Serial.println ("");
  }
}

void handleKeypad(){
  char pressing = keypad.getKey() != NO_KEY;
  Key key = keypad.key[0];
  
  switch(key.kchar) {
    case NO_KEY:
      break;
    case (char)ON_OFF_BUTTON:
      if (pressing){
        on = !on;
        digitalWrite(RXLED, on? LOW : HIGH);
      } else if (key.kstate == HOLD && !debug_mode) {
        debug_mode = true;
        Serial.begin (115200);
      }
      break;
    case (char)KEY_LEFT_ARROW:
    case (char)KEY_RIGHT_ARROW:
    case (char)KEY_UP_ARROW:
    case (char)KEY_DOWN_ARROW:
      if (pressing) {
        Keyboard.press(KeyboardKeycode(key.kchar));
      } else if (key.kstate == RELEASED) {
        Keyboard.release(KeyboardKeycode(key.kchar));
      }
      break;
    case (char)VOLUME_UP_BUTTON:
      if (pressing) {
        Consumer.write(MEDIA_VOLUME_UP);
      }
      break;
    case (char)VOLUME_DOWN_BUTTON:
      if (pressing) {
        Consumer.write(MEDIA_VOLUME_DOWN);
      }
      break;
    default:
      if (pressing) {
        Keyboard.write(KeyboardKeycode(key.kchar));
      }
      break;
  }
}