#include <Key.h>
#include <Keypad.h>
#include <Wire.h>  
#include <LiquidCrystal_I2C.h>
#include <Arduino_FreeRTOS.h>
#include <Queue.h>
#include <Servo.h>

#define KEY_ROWS 4
#define KEY_COLS 4

#define photo_pin A0
#define pressure_pin A1
#define servo_pin 5

LiquidCrystal_I2C lcd(0x27,16,2);

char keymap[KEY_ROWS][KEY_COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

// Column pin 1~4
byte colPins[KEY_COLS] = {9, 8, 7, 6};
// Column pin 1~4
byte rowPins[KEY_ROWS] = {13, 12, 11, 10}; 

Keypad myKeypad = Keypad(makeKeymap(keymap), rowPins, colPins, KEY_ROWS, KEY_COLS);

Servo servo;

void Display(void *pvParameters);
void Save(void *pvParameters);
void Drop(void *pvParameters);
void Enter(void *pvParameters);
void Reset(void *pvParameters);

int pressure_value;
int servo_pos = 0;

int drop = 0;
boolean dropping = 0;

TaskHandle_t xHandle;

void setup()
{
  Serial.begin(9600);

  lcd.init();    // initialize LCD
  lcd.backlight();    // open LCD backlight
  pinMode(pressure_pin, INPUT);
  servo.attach(servo_pin);

  xTaskCreate(Display, "displayTask", 50, NULL, 1, NULL);
  xTaskCreate(Save, "saveTask", 80, NULL, 1, NULL);
  xTaskCreate(Enter, "enterTask", 60, NULL, 1, NULL);
  xTaskCreate(Reset, "resetTask", 60, NULL, 1, NULL);
  xTaskCreate(Drop, "dropTask", 70, NULL, 1, &xHandle);

  vTaskStartScheduler();
}

void loop()
{
}

void Display(void *pvParameters){
  (void) pvParameters;
  vTaskDelay(15);
}

void Save(void *pvParameters){
  (void) pvParameters;
  
  pressure_value = analogRead(pressure_pin);
  Serial.println(pressure_value);
  Serial.println("-------------");

  if(pressure_value > 50 && !dropping){
    /*Serial.println("in");
    servo1.write(0);
    vTaskDelay(1000);
    servo1.write(180);
    vTaskDelay(1000);*/
    Serial.println("Task1 raises Task2’s priority");
    vTaskPrioritySet(xHandle, 3);
    drop = 1;
    pressure_value = 0;
    vTaskDelay(5500);
  }
  else{
    drop = 0;
    vTaskDelay(15);
  }

  
}

void Drop(void *pvParameters){
  (void) pvParameters;

  Serial.print("drop: ");
  Serial.print(drop);
  Serial.print(",   dropping:");
  Serial.print(dropping);
  Serial.print(",   pos:");
  Serial.println(servo_pos);
  Serial.println("-------------");

  if(drop == 1){
    dropping = 1;
    if(servo_pos <= 180){
      servo.write(servo_pos);
      servo_pos+=1;
      //Serial.println(servo_pos);
      if(servo_pos == 181){
        Serial.println("in");
        drop = 2;
        servo_pos = 179;
      }
    }
  }
  else if(drop == 2){
    if(servo_pos >= 0){
      servo.write(servo_pos);
      servo_pos-=1;
      //Serial.println("in2");
      if(servo_pos == -1){
        dropping = 0;
        drop = 0;
        servo_pos = 0;
        Serial.println("Task2 lower its priority");
        vTaskPrioritySet(xHandle, 1);
      }
    }
  }
  /*else if(drop == 0){
    
  }*/
  
  vTaskDelay(15);
}

void Enter(void *pvParameters){
  (void) pvParameters;
  vTaskDelay(15);
}

void Reset(void *pvParameters){
  (void) pvParameters;
  vTaskDelay(15);
}
