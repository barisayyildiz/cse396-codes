#include <wiringPi.h>
#include <iostream>

// GPIO pin numaralarını tanımla
const int buttonPin = 17;  // Buton pin
const int led1Pin = 27;    // İlk LED pin
const int led2Pin = 22;    // İkinci LED pin

int prevButtonState = LOW;

int main() {
    // WiringPi'yi başlat
    if (wiringPiSetupGpio() == -1) {
        return 1;
    }

    // Pin modlarını ayarla
    pinMode(buttonPin, INPUT);
    pinMode(led1Pin, OUTPUT);
    pinMode(led2Pin, OUTPUT);

    // Ana döngü
    while (true) {
        // Buton durumunu oku
        int buttonState = digitalRead(buttonPin);
        std::cout << buttonState << std::endl;

        // Buton basıldığında
        if (buttonState == HIGH) {
            // LED'leri yak
            digitalWrite(led1Pin, HIGH);
            digitalWrite(led2Pin, HIGH);
        } else {
            // LED'leri söndür
            // if(prevButtonState == HIGH) {
            //     std::cout << "start scanning" << std::endl;
            // }
            digitalWrite(led1Pin, LOW);
            digitalWrite(led2Pin, LOW);
        }
        prevButtonState = buttonState;
    }

    return 0;
}
