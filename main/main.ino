#include <TimerOne.h>
#include <Keypad.h>
#include <SPI.h>
#include <MFRC522.h>

#define RST_PIN   10
#define SS_PIN    53
#define BUZZER    11
#define BLUE_LED  23
#define GREEN_LED 22
#define RED_LED_1 24
#define RED_LED_2 25
#define RED_LED_3 26
#define RED_LED_4 27
#define RED_LED_5 28

MFRC522 RFID(SS_PIN, RST_PIN);	// Create RFID instance

// Computes GCD of two numbers
unsigned long gcd(unsigned long a, unsigned long b) {
  if (b == 0) return a;
  return gcd(b, a % b);
}

// Keys on 4x4 keypad
char keypadKeys[4][4] {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

byte rowPins[4] = {9, 8, 7, 6};			// Keypad row pinouts, provided by ELEGOO
byte colPins[4] = {5, 4, 3, 2};			// Keypad col pinouts, provided by ELEGOO
char keypadCode[4] = {'1','2','3','4'}; // Code to lock/unlock bike (read right to left)
Keypad bikeKeypad = Keypad(makeKeymap(keypadKeys), rowPins, colPins, 4, 4);
bool bikeLocked = false; 				// Flag for lock status
bool bikeAlarm = false; 				// Flag for alarm sounding
int bikeSpeed = 5;						// Current speed in MPH
char inputKey;							// Storing keypad button


// Used for timer interrupts
volatile unsigned char timerFlag = 0;
void TimerISR() {
  timerFlag = 1;
}

typedef struct task {
  int state;                    // Task's current state
  unsigned long period;         // Task's period in ms
  unsigned long elapsedTime;    // Time elapsed since last tick
  int (*TickFct)(int);          // Address of tick function
} task;

static task task1, task2, task3, task4, task5, task6;
task* tasks[] = { &task1, &task2, &task3, &task4, &task5, &task6 };
const unsigned char numTasks = sizeof(tasks) / sizeof(tasks[0]);
const char startState = 0;    // Refers to first state enum
unsigned long GCD = 0;        // For timer period

// Task 1 (Store the key being pressed on keypad)
enum GK_States { GK_SMStart };
int TickFct_GetKey(int state) {
  char tempKey = bikeKeypad.getKey();
  if (tempKey) {
	inputKey = tempKey;
	tone(BUZZER, 400, 100);

	Serial.print("Input key = ");
	Serial.println(inputKey);
	Serial.print("Variable bikeLocked = ");
	Serial.println(bikeLocked);
  }
  return state;
}

// Task 2 (Locks/unlocks with keypad)
enum KP_States { KP_SMStart, KP_Wait, KP_Digit1, KP_Digit2, KP_Digit3, KP_Digit4 };
int TickFct_Keypad(int state) {
	switch (state) {	// State transitions
		case KP_SMStart:
			state = KP_Wait;
			break;

		case KP_Wait:
			if (inputKey == keypadCode[0]) {
				state = KP_Digit1;
			}
			else {
				state = KP_Wait;
			}
			break;

		case KP_Digit1:
			if (inputKey == keypadCode[1]) {
				state = KP_Digit2;
			}
			else if (inputKey == keypadCode[0]) {
				state = KP_Digit1;
			}
			else {
				state = KP_Wait;
			}
			break;

		case KP_Digit2:
			if (inputKey == keypadCode[2]) {
				state = KP_Digit3;
			}
			else if (inputKey == keypadCode[1]) {
				state = KP_Digit2;
			}
			else {
				state = KP_Wait;
			}
			break;

		case KP_Digit3:
			if (inputKey == keypadCode[3]) {
				state = KP_Digit4;
			}
			else if (inputKey == keypadCode[2]) {
				state = KP_Digit3;
			}
			else {
				state = KP_Wait;
			}
			break;

		case KP_Digit4:
			state = KP_Wait;
			break;

		default:
			state = KP_SMStart;
			break;
	}
	switch (state) {	// State actions
		case KP_Wait:
			break;

		case KP_Digit1:
			break;

		case KP_Digit2:
			break;

		case KP_Digit3:
			break;

		case KP_Digit4:
			bikeLocked = !bikeLocked;
			break;

		default:
			break;
	}
	return state;
}

// Task 3 (Locks/unlocks with RFID)
enum RFID_States { RFID_SMStart, RFID_Wait, RFID_Read, RFID_Cooldown };
int TickFct_RFID(int state) {

	static unsigned char i;

	switch (state) {	// State transitions
		case RFID_SMStart:
			state = RFID_Wait;
			break;

		case RFID_Wait:
			if (!RFID.PICC_IsNewCardPresent() || !RFID.PICC_ReadCardSerial()) {	  // Checks for RFID tag
				state = RFID_Wait;
			}
			else {
				state = RFID_Read;
			}
			break;

		case RFID_Read:
			i = 0;
			Serial.println("Waiting 2 seconds...\n");
			state = RFID_Cooldown;
			break;

		case RFID_Cooldown:
			if (i < 10) {   // Wait 2 sec
				state = RFID_Cooldown;
			}
			else {
				state = RFID_Wait;
			}
			break;

		default:
			state = RFID_SMStart;
			break;
	}
	switch (state) {	// State actions
		case RFID_Wait:
			break;

		case RFID_Read:
			// Check the UID of the tag (first 4 bytes)
			if (RFID.uid.uidByte[0] == 0xA3 &&
				RFID.uid.uidByte[1] == 0x26 &&
				RFID.uid.uidByte[2] == 0x25 &&
				RFID.uid.uidByte[3] == 0x0C) {

				tone(BUZZER, 400, 100);
				bikeLocked = !bikeLocked;

				Serial.println("UID read success!");
				Serial.print("Variable bikeLocked = ");
				Serial.println(bikeLocked);
			}
			else {
				tone(BUZZER, 200, 600);
				Serial.println("UID read fail!");
			}
			break;

		case RFID_Cooldown:
			++i;
			break;

		default:
			break;
	}

/* 	// Look for any RFID cards, select one if present
	if (!RFID.PICC_IsNewCardPresent() || !RFID.PICC_ReadCardSerial()) return state;

	// Check the UID of the tag (first 4 bytes)
	if (RFID.uid.uidByte[0] == 0xA3 &&
		RFID.uid.uidByte[1] == 0x26 &&
		RFID.uid.uidByte[2] == 0x25 &&
		RFID.uid.uidByte[3] == 0x0C) {
		bikeLocked = !bikeLocked;
		Serial.println("UID read success!");
		Serial.print("Variable bikeLocked = ");
		Serial.println(bikeLocked);
	}
	else {
		Serial.println("UID read fail!");
	} */

	return state;
}

// Task 4 (Lights LEDs to indicate lock status)
int TickFct_LED(int state) {
	if (!bikeLocked) {
		digitalWrite(GREEN_LED, HIGH);
		digitalWrite(BLUE_LED, LOW);
	}
	else {
		digitalWrite(GREEN_LED, LOW);
		digitalWrite(BLUE_LED, HIGH);
	}
	return state;
}

// Task 5 (Determines if bike is being moved while locked)
enum BS_States { BS_SMStart, BS_Wait, BS_Count, BS_Alarm };
int TickFct_BikeSteal(int state) {	// State transitions

	static unsigned char i;

	switch (state) {
		case BS_SMStart:
			state = BS_Wait;
			break;

		case BS_Wait:
			if (bikeLocked && (bikeSpeed >= 5)) {
				state = BS_Count;
				i = 0;
			}
			else {
				state = BS_Wait;
			}
			break;

		case BS_Count:
			if (!bikeLocked || (bikeSpeed < 5)) {
				state = BS_Wait;
			}
			else if ((i < 10) && bikeLocked && (bikeSpeed >= 5)) {
				state = BS_Count;
			}
			else if (!(i < 10) && bikeLocked && (bikeSpeed >= 5)) {
				state = BS_Alarm;
			}
			else {
				state = BS_Count;
				Serial.println("Something went wrong in BS_Count!");
			}
			break;

		case BS_Alarm:
			if (bikeLocked) {
				state = BS_Alarm;
			}
			else if (!bikeLocked) {
				state = BS_Wait;
			}
			break;

		default:
			state = BS_SMStart;
			break;
	}
	switch (state) {	// State actions
		case BS_Wait:
			bikeAlarm = false;
			break;

		case BS_Count:
			++i;
			bikeAlarm = false;
			break;

		case BS_Alarm:
			bikeAlarm = true;
			break;

		default:
			break;
	}

	// DEBUGGING
	Serial.print("i = ");
	Serial.println(i);
	Serial.print("bikeAlarm = ");
	Serial.println(bikeAlarm);
	Serial.println();

	return state;
}

// Task 6 (Flashes LEDs if bike is being moved while locked)
int TickFct_AlarmLED(int state) {
	if (bikeAlarm) {
		if (digitalRead(RED_LED_1) == HIGH) {
			digitalWrite(RED_LED_1, LOW);
			digitalWrite(RED_LED_2, LOW);
			digitalWrite(RED_LED_3, LOW);
			digitalWrite(RED_LED_4, LOW);
			digitalWrite(RED_LED_5, LOW);
		}
		else {
			digitalWrite(RED_LED_1, HIGH);
			digitalWrite(RED_LED_2, HIGH);
			digitalWrite(RED_LED_3, HIGH);
			digitalWrite(RED_LED_4, HIGH);
			digitalWrite(RED_LED_5, HIGH);
		}
	}
	else {
		digitalWrite(RED_LED_1, LOW);
		digitalWrite(RED_LED_2, LOW);
		digitalWrite(RED_LED_3, LOW);
		digitalWrite(RED_LED_4, LOW);
		digitalWrite(RED_LED_5, LOW);
	}
	return state;
}

void setup() {
  unsigned char j = 0;

  // Task 1 (Store the key being pressed on keypad)
  tasks[j]->state = startState;
  tasks[j]->period = 200000;
  tasks[j]->elapsedTime = tasks[j]->period;
  tasks[j]->TickFct = &TickFct_GetKey;
  ++j;

  // Task 2 (Locks/unlocks with keypad)
  tasks[j]->state = startState;
  tasks[j]->period = 100000;
  tasks[j]->elapsedTime = tasks[j]->period;
  tasks[j]->TickFct = &TickFct_Keypad;
  ++j;

  // Task 3 (Locks/unlocks with RFID)
  tasks[j]->state = startState;
  tasks[j]->period = 200000;
  tasks[j]->elapsedTime = tasks[j]->period;
  tasks[j]->TickFct = &TickFct_RFID;
  ++j;

  // Task 4 (Lights LEDs to indicate lock status)
  tasks[j]->state = startState;
  tasks[j]->period = 100000;
  tasks[j]->elapsedTime = tasks[j]->period;
  tasks[j]->TickFct = &TickFct_LED;
  ++j;

  // Task 5 (Determines if bike is being moved while locked)
  tasks[j]->state = startState;
  tasks[j]->period = 500000;
  tasks[j]->elapsedTime = tasks[j]->period;
  tasks[j]->TickFct = &TickFct_BikeSteal;
  ++j;

  // Task 6 (Flashes LEDs if bike is being moved while locked)
  tasks[j]->state = startState;
  tasks[j]->period = 50000;
  tasks[j]->elapsedTime = tasks[j]->period;
  tasks[j]->TickFct = &TickFct_AlarmLED;
  ++j;

  // Find GCD for timer's period
  GCD = tasks[0]->period;
  for (unsigned char i = 1; i < numTasks; ++i) {
    GCD = gcd(GCD, tasks[i]->period);
  }

  pinMode(GREEN_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);
  pinMode(RED_LED_1, OUTPUT);
  pinMode(RED_LED_2, OUTPUT);
  pinMode(RED_LED_3, OUTPUT);
  pinMode(RED_LED_4, OUTPUT);
  pinMode(RED_LED_5, OUTPUT);

  Serial.begin(9600);                 // Baud rate is 9600 (serial output)
  SPI.begin();						  // Initialize SPI bus for RFID
  RFID.PCD_Init();					  // Initialize RFID module
  Timer1.initialize(GCD);             // GCD is in microseconds
  Timer1.attachInterrupt(TimerISR);   // TimerISR runs on interrupt
}

void loop() {
  for (unsigned char i = 0; i < numTasks; ++i) {    // Task scheduler
    if (tasks[i]->elapsedTime >= tasks[i]->period) {
      tasks[i]->state = tasks[i]->TickFct(tasks[i]->state);
      tasks[i]->elapsedTime = 0;
    }
    tasks[i]->elapsedTime += GCD;
  }
  while (!timerFlag);
  timerFlag = 0;
}

/* WORKS CITED
-------------------------------------
Learned how to use sizeof():
https://en.cppreference.com/w/cpp/language/sizeof

Learned how to use gcd():
https://en.cppreference.com/w/cpp/numeric/gcd

Using the TimerOne library for interrupts:
https://www.arduino.cc/reference/en/libraries/timerone/

GCD function (Euclid's algorithm):
https://www.tutorialspoint.com/c-program-to-implement-euclid-s-algorithm

Using the Arduino Keypad library to interface with keypad:
https://www.arduino.cc/reference/en/libraries/keypad/

Using the MFRC522 library to interface with RFID module:
https://www.arduino.cc/reference/en/libraries/mfrc522/

Learned how to read the UID of the MFRC522 RFID tag:
https://randomnerdtutorials.com/security-access-using-mfrc522-rfid-reader-with-arduino/
https://stackoverflow.com/questions/32839396/how-to-get-the-uid-of-rfid-in-arduino
*/