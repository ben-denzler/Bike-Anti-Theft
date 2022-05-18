#include <TimerOne.h>
#include <Keypad.h>

// Computes GCD of two numbers
unsigned long gcd(unsigned long a, unsigned long b) {
  if (b == 0) return a;
  return gcd(b, a % b);
}

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

static task task1;
task* tasks[] = { &task1 };
const unsigned char numTasks = sizeof(tasks) / sizeof(tasks[0]);
const char startState = 0;    // Refers to first state enum
unsigned long GCD = 0;        // For timer period

// Task 1 (...)
enum T1_States { T1_SMStart };
int TickFct_T1(int state) {
  return state;
}

void setup() {
  unsigned char j = 0;

  // Task 1 (...)
  tasks[j]->state = startState;
  tasks[j]->period = 100000;
  tasks[j]->elapsedTime = tasks[j]->period;
  tasks[j]->TickFct = &TickFct_T1;
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