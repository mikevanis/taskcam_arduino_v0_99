/* Taskcam v2 software for IRS Taskcam v2 Camera Shield + IRS Taskcam Camera Module */
/* Written by Andy Sheen 2017 */

/*~~~~~~~~~~TO DO~~~~~~~~~~*/
/* - Make shutter button take photo on letting go of button
   - Add Button and switch names
   - Add power down function
   - Add animation to text
   - Add question display
   - Add photo capture animation
   - Remove Adafruit splash (sorry Lady Ada...)
   - Add Photo hash tally
   - Add Contrast/Light/FX menu (bonus)
*/
#include <SoftwareSerial.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define OLED_RESET 4
#define PWR_PIN 2
#define CAM_PWR 10
#define RIGHT_BUTTON 8
#define LEFT_BUTTON 9
#define CAM_RX  12
#define CAM_TX  13
#define SHUTTER 3
#define LED 11


Adafruit_SSD1306 display(OLED_RESET);

boolean LEFT_DEBOUNCE = false;
boolean RIGHT_DEBOUNCE = false;

long buttonCheck;
int buttonInterval = 20;

long sleepMillis = 0;
long sleepTime = 20000;
boolean newQuestion = false;

boolean buttonHeld = false;
boolean pwrdwn = false;
long buttonHeldCount;
long offDuration = 2000;

int currentQuestion = 0;
//TO FIX
int numQuestions = 16;
byte questionTicks = 0;


char inputBuffer[64];
bool flag = false;
SoftwareSerial mySerial(CAM_RX, CAM_TX); // RX, TX

void setup() {

  //PWR Pin
  pinMode(PWR_PIN, OUTPUT);
  digitalWrite(PWR_PIN, 1);

  //CAM PIN
  pinMode(CAM_PWR, OUTPUT);

  //Buttons
  pinMode(SHUTTER, INPUT_PULLUP);
  pinMode(RIGHT_BUTTON, INPUT_PULLUP);
  pinMode(LEFT_BUTTON, INPUT_PULLUP);

  //LED
  pinMode(LED, OUTPUT);

  Serial.begin(57600);
  while (!Serial) {
  }
  //Camera Module Interface
  mySerial.begin(38400);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3D (for the 128x64)
  display.clearDisplay();


  digitalWrite(CAM_PWR, 1);
  startUpAni();
  initCam();
  indexQs();
  getQuestion(currentQuestion);
  delay(500);
  digitalWrite(CAM_PWR, 0);

}

void loop() { // run over and over
  sleepCheck();
  checkButtons();
  checkQuestions();
  display.clearDisplay();
  //display.setTextWrap(false);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 10);
  display.println(inputBuffer);
  applyTicks();
  display.display();
  delay(1);

  checkPwr();
  if (digitalRead(3) == 0 && flag == false) {
    flag = true;
    // checkPwr();
  }
  if (digitalRead(3) == 1 && flag == true) {
    // checkPwr();
    analogWrite(LED, 60);
    flag = false;
    display.clearDisplay();
    display.setTextSize(3);
    display.setTextColor(WHITE);
    display.setCursor(10, 0);
    display.clearDisplay();
    display.println("taking photo");
    display.display();
    delay(1);
    display.display();
    digitalWrite(10, 1);
    delay(1000);
    initCam();
    indexQs();
    capturePic();
    for (int i = 60; i < 255; i++) {
      analogWrite(LED, i);
      delay(1);
    }
    analogWrite(LED, 0);
    // getQuestion(currentQuestion);
    digitalWrite(10, 0);
  }
}

void getQuestion(uint8_t question) {
  // q + question num return Qs
  mySerial.write(0x71);
  mySerial.write((byte)question);
  mySerial.write(0x0D);
  mySerial.write(0x0A);


  //prints out the Question.... TO FIX
  while (mySerial.available() < 63) {
    delay(1);
  }
  for (int i = 0; i < 64 ; i++) {
    inputBuffer[i] = 0;
  }
  questionTicks = 0;
  for (int i = 0; i < 64; i++) {
    //Add to question tick
    if ((char)mySerial.peek() == '#') {
      questionTicks++;
      mySerial.read();
    } else {
      inputBuffer[i - questionTicks] = (char)mySerial.read();
    }
  }
  Serial.println(inputBuffer);
  Serial.println();
  Serial.print("NUM OF TICKS: ");
  Serial.print(questionTicks);
  Serial.println();
  while (mySerial.available() > 0) {
    mySerial.read();
  }

}

void initCam() {

  //Init CAM
  mySerial.write('~');
  mySerial.write('i');
  mySerial.write(0x0D);
  mySerial.write(0x0A);
  delay(100);
  while (mySerial.available() < 2) {
    //WAIT
    delay(1);
  }
  //Wait for 'INI'
  for (int i = 0 ; i < 4; i++) {
    Serial.print((char)mySerial.read());
  }
  while (mySerial.available() > 0) {
    mySerial.read();
  }
  Serial.println();
}

void indexQs() {
  //Index Questions
  mySerial.write(0x7E);
  mySerial.write(0x2B);
  mySerial.write(0x0D);
  mySerial.write(0x0A);
  while (mySerial.available() < 2) {
    delay(1);
  }
  numQuestions = (int)mySerial.read();
  Serial.print("NUMBER OF QS: ");
  Serial.println(numQuestions);
  while (mySerial.available() > 0) {
    mySerial.read();
  }
  Serial.println();
  delay(300);
}


void capturePic() {
  //Take picture
  mySerial.write(0x21);
  mySerial.write((uint8_t)currentQuestion);
  mySerial.write(0x0D);
  mySerial.write(0x0A);
  delay(2000);
  while (mySerial.available() < 0) {
    delay(1);
  }
  //Ack for when picture finished saving... needs fixing as hang over from oversized Q in buffer
  byte in = mySerial.read();
  Serial.println((int)in);
  if (in == 0x06) {
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(10, 0);
    display.clearDisplay();
    display.println("ERROR");
    display.display();
    delay(1000);
    display.display();
  } else {
    display.clearDisplay();
    display.setTextSize(3);
    display.setTextColor(WHITE);
    display.setCursor(10, 0);
    display.clearDisplay();
    display.println("       SAVED");
    display.display();
    delay(1000);
    display.display();
  }
  while (mySerial.available() > 0) {
    mySerial.read();
  }
}

void checkPwr() {
  int butRead = digitalRead(SHUTTER);
  if (butRead == 0 && buttonHeld == false) {
    sleepMillis = millis();
    Serial.println("BUTTON ON");
    buttonHeldCount = millis();
    buttonHeld = true;
  }
  if (buttonHeld == true) {
    if (millis() - buttonHeldCount > offDuration) {
      Serial.println("SHUTDOWN");
      display.clearDisplay();
      display.setTextSize(2);
      display.setTextColor(WHITE);
      display.setCursor(10, 0);
      display.clearDisplay();
      display.println("Shutting down");
      display.display();
      display.display();
      delay(300);
      pwrdwn = true;
      while (1) {
        if (pwrdwn == true) {
          display.clearDisplay();
          display.display();
          digitalWrite(PWR_PIN, 0);
        }
      }
    }
  }
  if (buttonHeld == true && butRead == 1) {
    buttonHeld = false;
    Serial.println("OFF");
  }
}

void checkButtons() {

  if (millis() - buttonCheck > buttonInterval) {
    buttonCheck = millis();

    if (digitalRead(LEFT_BUTTON) == LOW && LEFT_DEBOUNCE == false) {
      sleepMillis = millis();
      LEFT_DEBOUNCE = true;
      currentQuestion--;
      newQuestion = true;
    }
    if (digitalRead(LEFT_BUTTON) == HIGH && LEFT_DEBOUNCE == true) {
      LEFT_DEBOUNCE = false;
    }

    if (digitalRead(RIGHT_BUTTON) == LOW && RIGHT_DEBOUNCE == false) {
      sleepMillis = millis();
      RIGHT_DEBOUNCE = true;
      currentQuestion++;
      newQuestion = true;
    }
    if (digitalRead(RIGHT_BUTTON) == HIGH && RIGHT_DEBOUNCE == true) {
      RIGHT_DEBOUNCE = false;
    }

    if (currentQuestion >= numQuestions) {
      currentQuestion = 0;
    }

    if (currentQuestion < 0) {
      currentQuestion = numQuestions;
    }

  }

}

void checkQuestions() {
  if (newQuestion) {
    newQuestion = false;
    digitalWrite(10, 1);
    delay(1000);
    initCam();
    indexQs();
    getQuestion(currentQuestion);
   // getNumTicks(currentQuestion);
    delay(500);
    digitalWrite(10, 0);
  }
}

void sleepCheck() {
  if (millis() - sleepMillis > sleepTime) {
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(0, 20);
    display.clearDisplay();
    display.println("Sleep mode");
    display.display();
    delay(300);
    display.clearDisplay();
    display.display();
    digitalWrite(CAM_PWR, 0);
    digitalWrite(PWR_PIN, 0);
  }
}

void startUpAni() {
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 20);
  display.clearDisplay();
  display.println("TaskCam   v0.99");
  display.display();
  delay(1);

  display.startscrollright(0x00, 0x0F);
  delay(1700);
  display.stopscroll();
}

void tick(int x, int y) {
  display.drawLine(x, y - 3, x + 3, y, WHITE);
  display.drawLine(x + 3, y, x + 3 + 6, y - 6, WHITE);
}

void applyTicks() {
 byte tickNum = questionTicks;
  if (tickNum) {
    for (int i = 0; i < tickNum; i++) {
      tick(5 + (i * 12), 8);
    }
  }
}

byte getNumTicks(byte question) {
   while (mySerial.available() > 0) {
    mySerial.read();
   }
  mySerial.write(0x22);
  mySerial.write((byte)question);
  mySerial.write(0x0D);
  mySerial.write(0x0A);
  while (mySerial.available() < 0) {
    delay(1);
  }
  //Ack for when picture finished saving... needs fixing as hang over from oversized Q in buffer
  byte in = mySerial.read();
  questionTicks = in;
}

