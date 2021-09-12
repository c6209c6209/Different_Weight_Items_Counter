#include <Key.h>
#include <Keypad.h>
#include <Wire.h>  
#include <LiquidCrystal_I2C.h>
#include <Arduino_FreeRTOS.h>
#include <Servo.h>
#include <semphr.h>

#define KEY_ROWS 4
#define KEY_COLS 4

#define photo_pin A0
#define pressure_pin A1
#define servo_pin 5
#define red_pin 10
#define green_pin 9
#define blue_pin 3

LiquidCrystal_I2C lcd(0x27,16,2);

char keymap[KEY_ROWS][KEY_COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

// Column pin 1~4
byte colPins[KEY_COLS] = {2 , 8, 7, 6};
// Column pin 1~4
byte rowPins[KEY_ROWS] = {13, 12, 11, 4}; 

Keypad myKeypad = Keypad(makeKeymap(keymap), rowPins, colPins, KEY_ROWS, KEY_COLS);

Servo servo;

void Display(void *pvParameters);
void Save(void *pvParameters);
void Drop(void *pvParameters);
void Enter(void *pvParameters);

int pressure_value;
int photo_value;
boolean drop = 0;
boolean reset = 0;

int a = 0;
int b = 0;
int c = 0;

int enter_a1 = 0;
int enter_a2 = 0;
int enter_b1 = 0;
int enter_b2 = 0;
int enter_c1 = 0;
int enter_c2 = 0;

int enter_digit = 0;
int enter_type = 0;

int weight_count = 0;

boolean door = 0;

SemaphoreHandle_t binary_sem; //Global handler
TaskHandle_t xHandle;

void setup()
{
  Serial.begin(9600);
  
  pinMode(photo_pin, INPUT);

  lcd.init();    // initialize LCD
  lcd.backlight();    // open LCD backlight
  
  pinMode(red_pin, OUTPUT);
  pinMode(green_pin, OUTPUT);
  pinMode(blue_pin, OUTPUT); 
  analogWrite(red_pin, 0);
  analogWrite(green_pin, 0);
  analogWrite(blue_pin, 0);

  binary_sem = xSemaphoreCreateBinary();
  xSemaphoreGive(binary_sem);

  xTaskCreate(Display, "displayTask", 160, NULL, 1, NULL);
  xTaskCreate(Save, "saveTask", 75, NULL, 1, NULL);
  xTaskCreate(Enter, "enterTask", 80, NULL, 1, NULL);
  xTaskCreate(Drop, "dropTask", 75, NULL, 1, NULL);

  noInterrupts();
  TCCR1A = 0;    TCCR1B = 0;    TCNT1 = 0;
  TCCR1B |= (1 << WGM12); // turn on CTC mode
  TCCR1B |= (1<<CS12) | (1<<CS10); // 1024 prescaler
  OCR1A = 1562;  // give 0.1 sec at 16 MHz/1024
  interrupts();

  vTaskStartScheduler();
}

void loop()
{
  if (TIFR1 & (1 << OCF1A)) {
    photo_value = analogRead(photo_pin);
    //Serial.println(photo_value);
    if (photo_value >= 500 && !door){
       reset = 1;
       door = 1;
    }
    else if(photo_value < 400){
      door = 0;
    }
    TIFR1 = (1<<OCF1A); // clear overflow flag
  }

}

void Display(void *pvParameters){
  (void) pvParameters;
  
  lcd.clear();
  lcd.setCursor(0, 0);  // setting cursor
  lcd.print(F("A:"));
  lcd.setCursor(2, 0);  // setting cursor
  lcd.print(a);
  lcd.setCursor(5, 0);  // setting cursor
  lcd.print(F("B:"));
  lcd.setCursor(7, 0);  // setting cursor
  lcd.print(b);
  lcd.setCursor(10, 0);  // setting cursor
  lcd.print(F("C:"));
  lcd.setCursor(12, 0);  // setting cursor
  lcd.print(c);

  for(;;){
    if(reset){
      lcd.clear();
      lcd.setCursor(0, 0);  // setting cursor
      lcd.print(F("Reset"));
      lcd.setCursor(0, 1);  // setting cursor
      lcd.print(F("A:"));
      lcd.setCursor(5, 1);  // setting cursor
      lcd.print(F("B:"));
      lcd.setCursor(10, 1);  // setting cursor
      lcd.print(F("C:"));
      if(enter_type == 0){
        if(enter_digit == 1){
          lcd.setCursor(2, 1);  // setting cursor
          lcd.print(enter_a1);
        }
        else if(enter_digit == 2){
          lcd.setCursor(2, 1);  // setting cursor
          lcd.print(enter_a1);
          lcd.setCursor(3, 1);  // setting cursor
          lcd.print(enter_a2);
        }
      }
      else if(enter_type == 1){
        lcd.setCursor(2, 1);  // setting cursor
        lcd.print(a);
        if(enter_digit == 1){
          lcd.setCursor(7, 1);  // setting cursor
          lcd.print(enter_b1);
        }
        else if(enter_digit == 2){
          lcd.setCursor(7, 1);  // setting cursor
          lcd.print(enter_b1);
          lcd.setCursor(8, 1);  // setting cursor
          lcd.print(enter_b2);
        }
      }
      else if(enter_type == 2){
        lcd.setCursor(2, 1);  // setting cursor
        lcd.print(a);
        lcd.setCursor(7, 1);  // setting cursor
        lcd.print(b);
        if(enter_digit == 1){
          lcd.setCursor(12, 1);  // setting cursor
          lcd.print(enter_c1);
        }
        else if(enter_digit == 2){
          lcd.setCursor(12, 1);  // setting cursor
          lcd.print(enter_c1);
          lcd.setCursor(13, 1);  // setting cursor
          lcd.print(enter_c2);
        }
      }
    }
    else{
      lcd.clear();
      lcd.setCursor(0, 0);  // setting cursor
      lcd.print(F("A:"));
      lcd.setCursor(2, 0);  // setting cursor
      lcd.print(a);
      lcd.setCursor(5, 0);  // setting cursor
      lcd.print(F("B:"));
      lcd.setCursor(7, 0);  // setting cursor
      lcd.print(b);
      lcd.setCursor(10, 0);  // setting cursor
      lcd.print(F("C:"));
      lcd.setCursor(12, 0);  // setting cursor
      lcd.print(c);
    }
    vTaskDelay(15);
  }
}

void Save(void *pvParameters){
  (void) pvParameters;
  
  pinMode(pressure_pin, INPUT);
   

  for(;;){
    if(xSemaphoreTake(binary_sem, portMAX_DELAY)){
      pressure_value = analogRead(pressure_pin);
      /*Serial.println(pressure_value);
      Serial.println("-------------");*/

      if(drop || reset){
        vTaskDelay(15);
      }
      else{
        if(pressure_value > 50){
          if(weight_count >= 3){
            if(pressure_value > 50 && pressure_value <= 400){
              analogWrite(red_pin, 255);
              analogWrite(green_pin, 0);
              analogWrite(blue_pin, 0);
              xSemaphoreGive(binary_sem);
              a+=1;
              drop = 1;
              weight_count = 0;
              vTaskDelay(15);
            }
            else if(pressure_value > 400 && pressure_value <= 610){
              analogWrite(red_pin, 0);
              analogWrite(green_pin, 255);
              analogWrite(blue_pin, 0);
              xSemaphoreGive(binary_sem);
              b+=1;
              drop = 1;
              weight_count = 0;
              vTaskDelay(15);
            }
            else if(pressure_value > 610){
              analogWrite(red_pin, 0);
              analogWrite(green_pin, 0);
              analogWrite(blue_pin, 255);
              xSemaphoreGive(binary_sem);
              c+=1;
              drop = 1;
              weight_count = 0;
              vTaskDelay(15);
            }
          }
          else{
            xSemaphoreGive(binary_sem);
            weight_count += 1;
            vTaskDelay(15);
          }
        }
        else{
          analogWrite(red_pin, 0);
          analogWrite(green_pin, 0);
          analogWrite(blue_pin, 0);
          drop = 0;
          weight_count = 0;
          vTaskDelay(15);
        }
      }
      xSemaphoreGive(binary_sem);
    }
  }
}

void Drop(void *pvParameters){
  (void) pvParameters;
  
  servo.attach(servo_pin);
  servo.write(0);
  for(;;){
    if(xSemaphoreTake(binary_sem, portMAX_DELAY)){
      if(drop==1){
        servo.write(180);
        vTaskDelay(150);
        servo.write(0);
        vTaskDelay(80);
        drop = 0;
      }
      else{
        vTaskDelay(15);
      }
      xSemaphoreGive(binary_sem);
    }
  }
}

void Enter(void *pvParameters){
  (void) pvParameters;
  
  for(;;){
    if(reset){
      char key = myKeypad.getKey();
      /*Serial.print("x: ");
      Serial.print(enter_x);
      Serial.print(",     y: ");
      Serial.print(enter_y);
      Serial.print(",     digit: ");
      Serial.println(enter_digit);*/
  
      if(key >= '0' && key <= '9'){
        if(enter_type == 0){
          if(enter_digit == 0){
            enter_a1 = key - '0';
            enter_digit += 1;
          }
          else if(enter_digit == 1){
            enter_a2 = key - '0';
            enter_digit += 1;
          }
        }
        else if(enter_type == 1){
          if(enter_digit == 0){
            enter_b1 = key - '0';
            enter_digit += 1;
          }
          else if(enter_digit == 1){
            enter_b2 = key - '0';
            enter_digit += 1;
          }
        }
        else if(enter_type == 2){
          if(enter_digit == 0){
            enter_c1 = key - '0';
            enter_digit += 1;
          }
          else if(enter_digit == 1){
            enter_c2 = key - '0';
            enter_digit += 1;
          }
        }
      }
      else if(key == '#'){
        if(enter_type == 0){
          if(enter_digit == 1){
            a = enter_a1;
          }
          else if(enter_digit == 2){
            a = enter_a1*10 + enter_a2;
          }
        }
        else if(enter_type == 1){
          if(enter_digit == 1){
            b = enter_b1;
          }
          else if(enter_digit == 2){
            b = enter_b1*10 + enter_b2;
          }
        }
        else if(enter_type == 2){
          if(enter_digit == 1){
            c = enter_c1;
          }
          else if(enter_digit == 2){
            c = enter_c1*10 + enter_c2;
          }

        }
        enter_type += 1;
        enter_digit = 0;
        if(enter_type == 3){
          reset = 0;
          enter_type = 0;
          enter_a1 = 0;
          enter_a2 = 0;
          enter_b1 = 0;
          enter_b2 = 0;
          enter_c1 = 0;
          enter_c2 = 0;
        }
      }
    }
    vTaskDelay(10);
  }
}
