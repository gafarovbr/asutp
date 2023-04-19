// Передает значения по BLE первое время без ошибок.

//библиотеки
#include <BLEDevice.h>//бибилиотека для связи плат по bluetooth
#include <BLEServer.h>//бибилиотека для связи плат по bluetooth
#include <BLEUtils.h>//бибилиотека для связи плат по bluetooth
#include <BLE2902.h>//бибилиотека для связи плат по bluetooth
#include <Wire.h>   //библиотека для i2c датчика
#include <iarduino_I2C_SHT.h> //   Подключаем библиотеку для работы с датчиком температуры и влажности I2C-flash.
//#include <SoftwareSerial.h>//библиотека для работы esp32 с uart

//SoftwareSerial mySerial(16, 17); // задаем обращение к 16 и 17 пину
// MySerial(RX,TX) - RX-TX платы
//byte cmd[9] = {0xFF,0x01,0x86,0x00,0x00,0x00,0x00,0x00,0x79};//массив обращения к датчику uart
//char response[9]; // здесь будет ответ

int pinD0 = 0; // Пин к которому подключен D0


#define temperatureCelsius//температура в цельсиях//скорее всего не нужна
#define pwmPin 5///рабочая версия 

//BLE server name
#define bleServerName "PODOK_ESP32"
//сокращаем обращение к библиотеке
iarduino_I2C_SHT sht;
//задаем переменные
float temp;
float hum;
float osv;
float osv1;
float ppm;

int prevVal = LOW;
long th, tl, h, l; //ppm;

// Переменные таймера
unsigned long lastTime = 0;
unsigned long timerDelay = 30000;

bool deviceConnected = false;//булевая переменная подключение девайса

// на сайте ниже можно создать новый uuid
// https://www.uuidgenerator.net/
#define SERVICE_UUID "91bad492-b950-4226-aa2b-4ede9fa42f59"//адрес сервиса
//переменные передающиеся по блютуз
// Temperature Characteristic and Descriptor
  BLECharacteristic bmeTemperatureCelsiusCharacteristics("cba1d466-344c-4be3-ab3f-189f80dd7518", BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);//тут будет передаваться информация о значениях
  BLEDescriptor bmeTemperatureCelsiusDescriptor(BLEUUID((uint16_t)0x2901));//тут передается более подробная информация про характеристику(формат и т.д.)


// Humidity Characteristic and Descriptor
BLECharacteristic bmeHumidityCharacteristics("ca73b3ba-39f6-4ab3-91ae-186dc9577d99", BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);//тут будет передаваться информация о значениях
BLEDescriptor bmeHumidityDescriptor(BLEUUID((uint16_t)0x2902));//тут передается более подробная информация про характеристику(формат и т.д.)


//Osvesh
BLECharacteristic OSVCharacteristics("aeefac6e-3f16-4f69-8941-b8238b949687", BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);//тут будет передаваться информация о значениях
BLEDescriptor OSVDescriptor(BLEUUID((uint16_t)0x2903));//тут передается более подробная информация про характеристику(формат и т.д.)


//CO2
BLECharacteristic PPMCharacteristics("3372b173-d90a-4a85-876d-c5f1e20de24a", BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);//тут будет передаваться информация о значениях
BLEDescriptor PPMDescriptor(BLEUUID((uint16_t)0x2904));//тут передается более подробная информация про характеристику(формат и т.д.)


//проверка подключения к сервису других плат
//Setup callbacks onConnect and onDisconnect
class MyServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
  };
  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
  }
};


void setup() {


 
  pinMode (pinD0, INPUT); // Установим вывод A1 как вход
  // Start serial communication 
  Serial.begin(115200);

  pinMode(pwmPin, INPUT);

  // инициируем датчик температуры и влажности 
  sht.begin();
  //CO2
  //mySerial.begin(9600); //а это датчик MH-Z19
  

  // Create the BLE Device
  BLEDevice::init(bleServerName);

  // Create the BLE Server
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *bmeService = pServer->createService(SERVICE_UUID);

  // Create BLE Characteristics and Create a BLE Descriptor
  // Temperature
  bmeService->addCharacteristic(&bmeTemperatureCelsiusCharacteristics);//добавляем значение
  bmeTemperatureCelsiusDescriptor.setValue("BME temperature Celsius");//добавляем описание
  bmeTemperatureCelsiusCharacteristics.addDescriptor(new BLE2902());//добавляем тех описание(формат и т.д.)

  // Humidity
  bmeService->addCharacteristic(&bmeHumidityCharacteristics);//добавляем значение
  bmeHumidityDescriptor.setValue("BME humidity");//добавляем описание
  bmeHumidityCharacteristics.addDescriptor(new BLE2902());//добавляем тех описание(формат и т.д.)
  // Osveshenie
  bmeService->addCharacteristic(&OSVCharacteristics);//добавляем значение
  OSVDescriptor.setValue("Osveshennost");//добавляем описание
  OSVCharacteristics.addDescriptor(new BLE2902());//добавляем тех описание(формат и т.д.)

  // CO2
  bmeService->addCharacteristic(&PPMCharacteristics);//добавляем значение
  PPMDescriptor.setValue("CO2");//добавляем описание
  PPMCharacteristics.addDescriptor(new BLE2902());//добавляем тех описание(формат и т.д.)
  
  // Start the service
  bmeService->start();

  // даем возможность подлкючиться к плате по bluetooth другим устройствам
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify...");
  
}

void loop() {
  //проверка что есть подключение других плат
  if (deviceConnected) {
     //проверка что прошло достаточно времени
    if ((millis() - lastTime) > timerDelay) {
      //диагностика подключения i2c
      if( sht.begin() ){}
      else             { Serial.print( "Модуль не найден на шине I2C" ); }
      
      // Read temperature as Celsius (the default)
      temp = sht.getTem();//значение температуры
      // Read humidity
      hum = sht.getHum();//значение влажности
  
      //считываем и передаем значение температуры в монитор порта и по bluetooth
      //Notify temperature reading from BME sensor
        static char temperatureCTemp[6];//массив ответа
        dtostrf(temp, 6, 2, temperatureCTemp);//переводим значение в нужный вид
        //Set temperature Characteristic value and notify connected client
        bmeTemperatureCelsiusCharacteristics.setValue(temperatureCTemp);//передаем значение в блютуз
        bmeTemperatureCelsiusCharacteristics.notify();//передаем подробности про значение
        Serial.print("Temperature Celsius: ");//вывод в монитор порта
        Serial.print(temp);
        Serial.print(" ºC");
      //считываем и передаем значение влажности в монитор порта и по bluetooth
      //Notify humidity reading from BME
      static char humidityTemp[6];//массив ответа
      dtostrf(hum, 6, 2, humidityTemp);//переводим значение в нужный вид
      //Set humidity Characteristic value and notify connected client
      bmeHumidityCharacteristics.setValue(humidityTemp);//передаем значение по блютуз
      bmeHumidityCharacteristics.notify();//передаем подробности про значение   
      Serial.print(" - Humidity: ");//вывод в монитор порта
      Serial.print(hum);
      Serial.println(" %");

      osv=analogRead(pinD0);//получаем значение освещения
      osv1=(100-((osv/4095)*100))*21.05;//значение в люксах
      //Serial.println(osv1); // пишем значение с пина pinD0 в порт
        //osveshenie
      static char OSVESH[6];//массив ответа
      dtostrf(osv1, 6, 2, OSVESH);//переводим значение в нужный вил
      //Set humidity Characteristic value and notify connected client
      OSVCharacteristics.setValue(OSVESH);//передаем значение по блютуз
      OSVCharacteristics.notify();//передаем подробности про значени   
      Serial.print(" Osveshenie: ");//вывод в монитор порта
      Serial.print(osv1);
      
      
     /* mySerial.write(cmd,9);//запрос PPM CO2
      memset(response, 0, 9);
        mySerial.readBytes(response, 9);
        int i;
        byte crc = 0;
        for (i = 1; i < 8; i++) crc+=response[i];
        crc = 255 - crc;
        crc++;
      
      
        if ( !(response[0] == 0xFF  && response[1] == 0x86 && response[8] == crc) ) {
          Serial.println("CRC error: " + String(crc) + " / "+ String(response[8]));
          ppm=1;
          
        } else {
          int responseHigh = (unsigned int) response[2];
          int responseLow = (unsigned int) response[3];
          ppm = (256*responseHigh) + responseLow;
          Serial.print(","); Serial.print(ppm); Serial.println(";");}*/
            // ну и по мануалу из ответа считаем PPM

        long tt = millis();
  int myVal = digitalRead(pwmPin);
/*if (isnan(myVal)) {
      Serial.println("Failed to read from DHT sensor!");
                 //  "Не удалось прочесть данные с датчика DHT!"
      return;}*/
  //Если обнаружили изменение
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
     
     // Serial.println("PPM = " + String(ppm));
      
    }
    
  }
       
       //PPM
      static char PPMChar[6];//массив ответа
      dtostrf(ppm, 6, 2, PPMChar);//переводим значение в нужный формат
      //Set humidity Characteristic value and notify connected client
      PPMCharacteristics.setValue(PPMChar);//передаем значение по блютуз
      PPMCharacteristics.notify();//передаем подробности про значение   
      Serial.print(" PPM: ");//вывод в монитор порта
      Serial.println(ppm);
      
      
      Serial.print("Sensor: ");
      
      lastTime = millis();
    }
  }
}
