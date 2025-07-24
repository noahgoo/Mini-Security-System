#include <IRremote.hpp>
#include <Servo.h>
#include <LiquidCrystal.h>

#define GREEN_LED 3
#define RED_LED 2
#define SERVO 10

#define TRIGGER_PIN A1
#define ECHO_PIN A2
#define BUZZER_PIN A0

#define IR_PIN 11
IRrecv irrecv(IR_PIN);
decode_results results;

LiquidCrystal lcd(4, 5, 6, 7, 8, 9);

int position = 90;
static Servo myServo;
static unsigned long acceptedCode[4] = {
  // code is 0000
  0xE916FF00,
  0xE916FF00,
  0xE916FF00,
  0xE916FF00
};
static int codeCounter = 0;
static int inputCounter = 0;
int securityState = 0;


// booleans
bool armed = false;
bool lcdShowingArmed = false;
bool lcdShowingUnarmed = false;

void setup() {
  Serial.begin(9600);
  // set up lcd
  lcd.begin(16, 2);
  lcd.print("Mini Security");
  lcd.setCursor(0,1);
  lcd.print("Activated");
  // set up IR receiver
  irrecv.begin(IR_PIN, ENABLE_LED_FEEDBACK);
  Serial.println("IR receiver ready");
  pinMode(TRIGGER_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  myServo.attach(SERVO);
  myServo.write(position);
  delay(2000);
  lcd.clear();
}

void loop() {
  fsmSecurityState();
  
}

void fsmSecurityState() {
  unsigned long input = readIR();

  switch (securityState) {
    // unarmed
    case 0:
      if (!lcdShowingUnarmed) {
        printUnarmed();
        lcdShowingArmed = false;
        lcdShowingUnarmed = true;
        unlockServo();
      }
      
      // if input is '1' switch to armed
      if (input != 0 && input == 0xF30CFF00) {
        securityState = 1;
      }
      break;

    // armed
    case 1:
      float distance = getDistanceSmoothed();
      if (!lcdShowingArmed) {
        printArmed();
        lcdShowingArmed = true;
        lcdShowingUnarmed = false;
        lockServo();
      }
      checkIntruder(distance);
      checkInputCode(input);
      checkCorrectCode();
      break;

  }
}

void checkIntruder(float distance) {
  if (distance < 10) {
    digitalWrite(BUZZER_PIN, HIGH);
  } else {
    digitalWrite(BUZZER_PIN, LOW);
  }
}

void checkInputCode(unsigned long input) {
  if (input != 0 && input == acceptedCode[codeCounter]) {
    codeCounter++;
    inputCounter++;
    // Serial.println(codeCounter);
  } else if (input != 0) {
    codeCounter = 0;
    inputCounter++;
    // Serial.print("INPUT ++  ");
    // Serial.println(inputCounter);
  }
}

void checkCorrectCode() {
  // determine if input is valid
  if (codeCounter == 4) {
    // if valid, turn on green light, turn Servo
    codeCounter = 0;
    inputCounter = 0;
    unlockServo();
    turnOnGreenLight();
    securityState = 0;
  } else if (inputCounter == 4) {
  // if not valid, turn on red light
    turnOnRedLight();
    inputCounter = 0;
  }
}

unsigned long readIR() {
  if (irrecv.decode()) {
    // Serial.println("Decoded");
    if (irrecv.decodedIRData.protocol != UNKNOWN) {
      unsigned long result = irrecv.decodedIRData.decodedRawData;
      Serial.print("Received: ");
      Serial.println(result, HEX);
      irrecv.resume();
      return result;
    }
    irrecv.resume();
  }
  return 0;
}

void turnOnGreenLight() {
  digitalWrite(GREEN_LED, HIGH);
  digitalWrite(RED_LED, LOW);
  delay(3000);
  digitalWrite(GREEN_LED, LOW);
}

void turnOnRedLight() {
  digitalWrite(RED_LED, HIGH);
  digitalWrite(GREEN_LED, LOW);
  delay(3000);
  digitalWrite(RED_LED, LOW);
}

void unlockServo() {
  myServo.write(180);
}

void lockServo() {
  myServo.write(90);
}

void printUnarmed() {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Security UNARMED");
  lcd.setCursor(0, 1);
  lcd.print("Enter '1' to arm");
}

void printArmed() {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Security ARMED");
  lcd.setCursor(0,1);
  lcd.print("Enter code:");
}

float getDistanceSmoothed() {
    static float distanceSmoothed = getDistanceRaw();
    float distance = getDistanceRaw();
    float alpha = 0.3; // alpha-filter constant
    if (distance != 0) {
  // this is an example of a measurement gate:
  // sensor returns a 0 when it times out 
  // (i.e., no measurement) ignore those measurements
    
  // alpha filter all good measurements     
      distanceSmoothed = alpha*distanceSmoothed +(1-alpha)*distance;
    }
    // Serial.println(distanceSmoothed);
    delay(100);
    return(distanceSmoothed);
}

float getDistanceRaw() {
    float duration = (float)getDurationRaw();
    // duration is time for sonar to travel to 
    // object and back
    duration = duration / 2;  
    // divide by 2 for travel time to object 
    float c = 343;  // speed of sound in m/s
    c = c * 100 / 1e6;  
// speed of sound in cm/microseconds
    // Calculate the distance in centimeters
    float distance = duration * c;
    // Serial.println(distance);
    // delay(100);
    return(distance);
}

long getDurationRaw() {
  long duration;
  // Clear the TRIGGER_PIN
  digitalWrite(TRIGGER_PIN, LOW);
  delayMicroseconds(2);

  // Set the TRIGGER_PIN on HIGH state 
  // for 10 micro seconds
  digitalWrite(TRIGGER_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER_PIN, LOW);
  
  // Reads the ECHO_PIN, returns the sound 
  //wave travel time in microseconds
  duration = pulseIn(ECHO_PIN, HIGH, 10000);
  // Serial.println(duration);
  // delay(250);
  return(duration);
}