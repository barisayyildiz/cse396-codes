#include <wiringPi.h>
#include <stdio.h>

int main() {

    int pinNumber = 18; // Replace with the GPIO pin number you want to use
     
    wiringPiSetupGpio();
    
    pinMode(pinNumber, OUTPUT);
    
    printf("test...");
    
    while(1){
        digitalWrite(pinNumber, LOW);
        delay(75);
        digitalWrite(pinNumber, HIGH);
        delay(75);
    }
    
    
    return 0;
}