#include <Stepper.h>

const int stepsPerRevolution = 200;  // change this to fit the number of steps per revolution
                                    // for your motor

// initialize the stepper library on pins 8 through 11:
Stepper myStepper(stepsPerRevolution, 11, 10, 9, 8);
Stepper myStepper2(stepsPerRevolution, 7, 6, 5, 4);

void setup() {
  // set the speed at 60 rpm:
  myStepper.setSpeed(60);
  myStepper2.setSpeed(60);
  // initialize the serial port:
  Serial.begin(9600);
}

bool first_loop = true;

void loop() {
  
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    if (cmd == 's') {
      int step1 = Serial.parseInt();
      int step2 = Serial.parseInt();

      myStepper.step(step1);
      myStepper2.step(step2);
//      if (Serial.read() == '\n') {
        Serial.println(step1);
        Serial.println(step2);
//      }
    } else if (cmd == 'r') {
//      if (Serial.read() == '\n') {
        Serial.println("Relay");
//      }
    }
  }
  
//  if (first_loop) {
//    for (int i = 0; i < 100; i++) {
//      myStepper.step(2);
//      delay(3000);
//    }
//      myStepper.step(stepsPerRevolution);
//      myStepper2.step(stepsPerRevolution);
//  }
//  first_loop = false;
//  // step one revolution  in one direction:
//  myStepper.step(stepsPerRevolution);
//  delay(500);
//
//  // step one revolution in the other direction:
//  Serial.println("counterclockwise");
//  myStepper.step(-stepsPerRevolution);
//  delay(500);
}

