#include <Wire.h>                                 // * Подключаем библиотеку для работы с аппаратной шиной I2C.
#include <iarduino_I2C_SHT.h>                     //   Подключаем библиотеку для работы с датчиком температуры и влажности I2C-flash (Sensor Humidity and Temperature).
iarduino_I2C_SHT sht;                             //   Объявляем объект sht для работы с функциями и методами библиотеки iarduino_I2C_SHT.



const int pinD27 = 27;                            // Пин подключения аналогового датчика освещенности
const int pinD5 = 5;                              // Пин подключение ШИМ для CO2

int prevVal = LOW;
long th, tl, h, l, ppm;

void setup(){                                     //
    delay(500);                                   // * Ждём завершение переходных процессов связанных с подачей питания.
    Serial.begin(9600);                           //
    while(!Serial){;}                             // * Ждём завершения инициализации шины UART.
    sht.begin();                                  //   Инициируем работу с датчиком температуры и влажности.
    pinMode(pinD5, INPUT);

}                                                 
                                                  
void loop(){ 
    long tt = millis();  
    int myVal = digitalRead(pinD5);
    if (myVal == HIGH) {
      if (myVal != prevVal) {
        h = tt;
        tl = h - l;
        prevVal = myVal;
      }
  }  else {
    if (myVal != prevVal) {
      l = tt;
      th = l - h;
      prevVal = myVal;
      ppm = 5000 * (th - 2) / (th + tl - 4);
      //Serial.println("PPM = " + String(ppm));
    }
  }
    Serial.print("PPM= ") ;
    Serial.print(String(ppm));    
    Serial.print("Освещенность = ");
    Serial.print(analogRead(pinD27));                     
    Serial.print("Температура = ");               
    Serial.print( sht.getTem() );                 //   Выводим текущую температуру, от -40.0 до +125 °C.
    Serial.print(" °C, влажность = ");            
    Serial.print( sht.getHum() );                 //   Выводим текущую влажность воздуха, от 0 до 100%.
    Serial.print(" %.\r\n");                      
    delay(500);                                   // * Задержка позволяет медленнее заполнять монитор последовательного порта.
} 