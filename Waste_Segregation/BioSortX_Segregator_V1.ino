#include <Servo.h>

int limitswitch = 13;
int state = 0;
int value;

// Moisture sensor and LED pins
int moisturePin = 8;  // Moisture sensor digital output pin
int ledPin = 7;       // LED pin
int moistureValue;

// IR sensor pin
int irPin = 6;  // IR sensor output pin
int irValue;

// Servo motor
Servo wasteServo;
int servoPin = 9;     // Servo control pin
int openAngle = 90;   // Angle to open
int closeAngle = 0;   // Angle to close

void setup() {
  Serial.begin(9600);

  pinMode(limitswitch, INPUT);
  pinMode(moisturePin, INPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(irPin, INPUT);

  // Attach servo
  wasteServo.attach(servoPin);
  wasteServo.write(closeAngle);  // Keep flap closed initially
}

void loop() {
  // ---------- IR Sensor Check ----------
  irValue = digitalRead(irPin);

  if (irValue == LOW) {  
    // Something detected by IR
    Serial.println("IR detected something...");
    delay(800);  // Small delay to stabilize readings

    // ---------- Limit switch check ----------
    value = digitalRead(limitswitch);
    state = value;  // Update state
    Serial.print("Limit switch value = ");
    Serial.println(state);

    bool wasteDetected = false; // flag to check if waste present

    if (state == HIGH) {
      Serial.println("Metal detected");
      wasteDetected = true;
    }

    // ---------- Moisture sensor check ----------
    moistureValue = digitalRead(moisturePin);

    if (moistureValue == LOW) {  
      // Water detected -> blink LED
      digitalWrite(ledPin, HIGH);
      delay(200);
      digitalWrite(ledPin, LOW);
      delay(200);
      Serial.println("Wet waste detected");
      wasteDetected = true;
    } 
    if(moistureValue == HIGH && state == LOW) { 
      // No water -> turn LED off 
      digitalWrite(ledPin, LOW);
      Serial.println("Dry waste detected"); 
      wasteDetected = true;
      }

    // ---------- Servo Action ----------
    if (wasteDetected) {
      Serial.println("Opening servo flap...");
      wasteServo.write(openAngle);  // Open flap
      delay(2000);                  // Keep open for 2 sec
      wasteServo.write(closeAngle); // Close flap
      Serial.println("Servo flap closed");
    }
  }
}
