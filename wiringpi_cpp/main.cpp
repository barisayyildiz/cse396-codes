#include <wiringPi.h>
#include <iostream>
#include <chrono>


#define STEPPER_PIN_1 1
#define STEPPER_PIN_2 7
#define STEPPER_PIN_3 8
#define STEPPER_PIN_4 25

#define DELAY_ONE_STEP 30
#define DELAY_MOVE 300

void oneStep(bool direction); 
void move(bool direction, int stepPrecision);

int step_number = 0;
int counter = 0;

int main() {
    wiringPiSetupGpio();
    
    pinMode(STEPPER_PIN_1, OUTPUT);
    pinMode(STEPPER_PIN_2, OUTPUT);
    pinMode(STEPPER_PIN_3, OUTPUT);
    pinMode(STEPPER_PIN_4, OUTPUT);

    auto t_start = std::chrono::high_resolution_clock::now();    
    while(counter < 2048){
        move(true, 16);
    }
    auto t_end = std::chrono::high_resolution_clock::now();

    double elapsed_time_ms = std::chrono::duration<double, std::milli>(t_end-t_start).count();
    std::cout << "elapsed time : " << elapsed_time_ms << std::endl;
    return 0;
}

void move(bool direction, int stepPrecision){
    int precisionCounter = 0;
    
    while(precisionCounter < stepPrecision){
        
        oneStep(direction);
        precisionCounter++;

        delay(DELAY_ONE_STEP);
    }

    delay(DELAY_MOVE);
}

void oneStep(bool direction){
    std::cout << "counter : " << ++counter << std::endl;
    if(direction){
    switch(step_number){
      case 0:
      digitalWrite(STEPPER_PIN_1, HIGH);
      digitalWrite(STEPPER_PIN_2, LOW);
      digitalWrite(STEPPER_PIN_3, LOW);
      digitalWrite(STEPPER_PIN_4, LOW);
      break;
      case 1:
      digitalWrite(STEPPER_PIN_1, LOW);
      digitalWrite(STEPPER_PIN_2, HIGH);
      digitalWrite(STEPPER_PIN_3, LOW);
      digitalWrite(STEPPER_PIN_4, LOW);
      break;
      case 2:
      digitalWrite(STEPPER_PIN_1, LOW);
      digitalWrite(STEPPER_PIN_2, LOW);
      digitalWrite(STEPPER_PIN_3, HIGH);
      digitalWrite(STEPPER_PIN_4, LOW);
      break;
      case 3:
      digitalWrite(STEPPER_PIN_1, LOW);
      digitalWrite(STEPPER_PIN_2, LOW);
      digitalWrite(STEPPER_PIN_3, LOW);
      digitalWrite(STEPPER_PIN_4, HIGH);
      break;
    }
  }
  else{
    switch(step_number){
      case 0:
      digitalWrite(STEPPER_PIN_1, LOW);
      digitalWrite(STEPPER_PIN_2, LOW);
      digitalWrite(STEPPER_PIN_3, LOW);
      digitalWrite(STEPPER_PIN_4, HIGH);
      break;
      case 1:
      digitalWrite(STEPPER_PIN_1, LOW);
      digitalWrite(STEPPER_PIN_2, LOW);
      digitalWrite(STEPPER_PIN_3, HIGH);
      digitalWrite(STEPPER_PIN_4, LOW);
      break;
      case 2:
      digitalWrite(STEPPER_PIN_1, LOW);
      digitalWrite(STEPPER_PIN_2, HIGH);
      digitalWrite(STEPPER_PIN_3, LOW);
      digitalWrite(STEPPER_PIN_4, LOW);
      break;
      case 3:
      digitalWrite(STEPPER_PIN_1, HIGH);
      digitalWrite(STEPPER_PIN_2, LOW);
      digitalWrite(STEPPER_PIN_3, LOW);
      digitalWrite(STEPPER_PIN_4, LOW);
      break;
    }
  }
  step_number++;
  if(step_number > 3){
    step_number = 0;
  }
}

// int main() {

//     int pinNumber = 18; // Replace with the GPIO pin number you want to use
     
//     wiringPiSetupGpio();
    
//     pinMode(pinNumber, OUTPUT);
    
//     printf("test...");
    
//     while(1){
//         digitalWrite(pinNumber, LOW);
//         delay(75);
//         digitalWrite(pinNumber, HIGH);
//         delay(75);
//     }
    
    
//     return 0;
// }