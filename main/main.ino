#include <TimerOne.h>
#include <Keypad.h>
#include <stdbool.h>

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

byte rowPins[4] = {9, 8, 7, 6};		// Keypad row pinouts, provided by ELEGOO
byte colPins[4] = {5, 4, 3, 2};		// Keypad col pinouts, provided by ELEGOO
char keypadCode[4] = {'1','2','3','4'}; 	// Code to lock/unlock bike (read right to left)
Keypad bikeKeypad = Keypad(makeKeymap(keypadKeys), rowPins, colPins, 4, 4); 	// Instantiate Keypad object
bool bikeLocked = false; 			// Flag for lock status
char inputKey;						// Storing keypad button

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

static task task1, task2;
task* tasks[] = { &task1, &task2 };
const unsigned char numTasks = sizeof(tasks) / sizeof(tasks[0]);
const char startState = 0;    // Refers to first state enum
unsigned long GCD = 0;        // For timer period

// Task 1 (Store the key being pressed on keypad)
enum GK_States { GK_SMStart };
int TickFct_GetKey(int state) {
  char tempKey = bikeKeypad.getKey();
  if (tempKey) {
	inputKey = tempKey;
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
			Serial.println("I'm in state Wait.");
			break;
		
		case KP_Digit1:
			Serial.println("I'm in state 1.");
			break;
			
		case KP_Digit2:
			Serial.println("I'm in state 2.");
			break;
			
		case KP_Digit3:
			Serial.println("I'm in state 3.");
			break;
			
		case KP_Digit4:
			bikeLocked = !bikeLocked;
			Serial.println("I'm in state 4.");
			break;
			
		default:
			break;
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

  // Find GCD for timer's period
  GCD = tasks[0]->period;
  for (unsigned char i = 1; i < numTasks; ++i) {
    GCD = gcd(GCD, tasks[i]->period); 
  }
  
  Serial.begin(9600);                 // Baud rate is 9600 (serial output)
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
*/