#include <Arduino.h>
#include <Chrono.h>
#include <SerialTransfer.h>
#include <Servo.h>
#include <VL53L0X.h>
#include "Wire.h"
#include "teensy_config.h"

extern "C" {
#include "utility/twi.h"  // from Wire library, so we can do bus scanning
}

// #define DEBUG

Servo motorLeft;
Servo motorRight;

struct MotorSpeeds {
    float left;
    float right;
} motorSpeeds;

SerialTransfer myTransfer;

int8_t step = 1;

Chrono sendMessage;
VL53L0X sensor;

float distances[8];
bool activeToFSensors[8];

void tcaselect(uint8_t i) {
    if (i > 7) return;

    Wire.beginTransmission(TCAADDR);
    Wire.write(1 << i);
    Wire.endTransmission();
}

void setup() {
#ifdef DEBUG
    Serial.begin(115200);
    while (!Serial) {
    };
#endif

    Serial2.begin(1152000);
    // while (!Serial2) {
    // };

    motorLeft.attach(TEENSY_PIN_DRIVE_LEFT);
    motorRight.attach(TEENSY_PIN_DRIVE_RIGHT);

    myTransfer.begin(Serial2);
    motorSpeeds.left = 0;
    motorSpeeds.right = 0;

    Wire.begin();
    tcaselect(0);
    for (uint8_t t = 0; t < 8; t++) {
        tcaselect(t);
#ifdef DEBUG
        Serial.print("TCA Port #");
        Serial.println(t);
        sensor.setTimeout(500);
        Serial.print("init sensor: ");
        Serial.println(t);
#endif
        // Start continuous back-to-back mode (take readings as
        // fast as possible).  To use continuous timed mode
        // instead, provide a desired inter-measurement period in
        // ms (e.g. sensor.startContinuous(100)).

        activeToFSensors[t] = sensor.init();

        if (activeToFSensors[t]) {
            sensor.startContinuous();
#ifdef DEBUG
            Serial.printf("Sensor %d init success", t);
#endif

        } else {
            /*
            TODO What to do to indicate sensor init failure?
            don't attempt to read from failed sensor? show in i2c oled?
            */
#ifdef DEBUG
            Serial.print("Failed to detect and initialize sensor: ");
            Serial.println(t);
            // while (1) {
            // }

#endif
        }
    }
}

void loop() {
    if (sendMessage.hasPassed(20)) {
        // restart the timeout
        sendMessage.restart();
        if (myTransfer.available()) {
            uint8_t recSize = 0;
            myTransfer.rxObj(motorSpeeds, sizeof(motorSpeeds), recSize);
#ifdef DEBUG
            Serial.print(motorSpeeds.left);
            Serial.print(' ');
            Serial.print(motorSpeeds.right);
            Serial.println();
#endif

            motorLeft.writeMicroseconds(map(motorSpeeds.left, -100, 100, 1000, 2000));
            motorRight.writeMicroseconds(map(motorSpeeds.right * -1, -100, 100, 1000, 2000));
        }
#ifdef DEBUG
        else if (myTransfer.status < 0) {
            Serial.print("ERROR: ");
            Serial.println(myTransfer.status);
        } else {
            Serial.print("waiting:");
            Serial.println(myTransfer.status);
        }
#endif
    }
    for (uint8_t t = 0; t < 8; t++) {
        //         tcaselect(t);
        //         if (activeToFSensors[t]) {
        //             distances[t] = sensor.readRangeContinuousMillimeters();
        //             if (sensor.timeoutOccurred()) {
        //                 distances[t] = 0;
        // #ifdef DEBUG
        //                 Serial.printf("TIMEOUT READING ToF %d", t);
        // #endif
        //             }
        //         } else {
        //             distances[t] = 0;
        //         }

        distances[t] = 100.0;
    }
    // #ifdef DEBUG
    //     Serial.printf(
    //         "distances: %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f",
    //         distances[0],
    //         distances[1],
    //         distances[2],
    //         distances[3],
    //         distances[4],
    //         distances[5],
    //         distances[6],
    //         distances[7]);
    //     Serial.println();
    // #endif
    myTransfer.txObj(distances, sizeof(distances), 0);
    myTransfer.sendData(sizeof(distances));
    // #ifdef DEBUG
    //     Serial.printf("%d", sizeof(distances));
    // #endif
}