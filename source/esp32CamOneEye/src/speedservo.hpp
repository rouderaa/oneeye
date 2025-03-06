// esp32servo based class with speed control
#include <ESP32Servo.h>

class SpeedServo {
private:
    Servo servo;
    int pin = -1;
    int min = 0;
    int max = 255;
    const char *name;

public:
    SpeedServo(const char *name, int min, int max, int pin) {
        this->name = name;
        this->min = min;
        this->max = max;

        setPeriodHertz(50);
        attach(pin);
    }
    const char *getName() {
        return name;
    }
    int getPin() {
        return pin;
    }
    int attach(int pin) {
        this->pin = pin;
        return servo.attach(pin);
    }
    bool set(int servoValue) {
        bool result = false;
        if (pin > 0) {
            servo.write(servoValue);
            result = true;
        }
        return result;
    }
    int move(int value) {
        int mappedValue = 0;
        if (pin > 0) {
            mappedValue = (value * (max - min) / 180) + min;
            servo.write(mappedValue);
        }
        return mappedValue;
    }
    void setPeriodHertz(int hz) {
        servo.setPeriodHertz(hz);
    }
};