/*
    Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleServer.cpp
    Ported to Arduino ESP32 by Evandro Copercini
    updates by chegewara
*/

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <Wire.h>                                 // * Подключаем библиотеку для работы с аппаратной шиной I2C.
#include <iarduino_I2C_SHT.h>                     //   Подключаем библиотеку для работы с датчиком температуры и влажности I2C-flash (Sensor Humidity and Temperature).
iarduino_I2C_SHT sht;  

#define bleServerName "SERVER_ESP32"
#define SERVICE_UUID "91bad492-b950-4226-aa2b-4ede9fa42f59"

BLECharacteristic shtTemperatureCelsiusCharacteristics("cba1d466-344c-4be3-ab3f-189f80dd7518", BLECharacteristic::PROPERTY_NOTIFY);
BLEDescriptor shtTemperatureCelsiusDescriptor(BLEUUID((uint16_t)0x2902));
BLECharacteristic shtHumidityCharacteristics("ca73b3ba-39f6-4ab3-91ae-186dc9577d99", BLECharacteristic::PROPERTY_NOTIFY);
BLEDescriptor shtHumidityDescriptor(BLEUUID((uint16_t)0x2903));
float temp;
float hum;

unsigned long lastTime = 0;
unsigned long timerDelay = 30000;
bool deviceConnected = false;


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
  Serial.begin(115200);
  Serial.println("Starting BLE work!");
  while(!Serial){;}                             // * Ждём завершения инициализации шины UART.
  sht.begin();
  BLEDevice::init(bleServerName);
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *shtService = pServer->createService(SERVICE_UUID);
  shtService->addCharacteristic(&shtTemperatureCelsiusCharacteristics);
  shtTemperatureCelsiusDescriptor.setValue("SHT temperature Celsius");
  shtTemperatureCelsiusCharacteristics.addDescriptor(new BLE2902());

  // Humidity
  shtService->addCharacteristic(&shtHumidityCharacteristics);
  shtHumidityDescriptor.setValue("SHT humidity");
  shtHumidityCharacteristics.addDescriptor(new BLE2902());


  //pCharacteristic->setValue("Hello World!");
  shtService->start();
  // BLEAdvertising *pAdvertising = pServer->getAdvertising();  // this still is working for backward compatibility
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  //pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);  
  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify...");
}

void loop() {
  if (deviceConnected) {
  if ((millis() - lastTime) > timerDelay) {
    // Read temperature as Celsius (the default)
    temp = sht.getTem();
    // Read humidity
    hum = sht.getHum();
    static char temperatureCTemp[6];
    dtostrf(temp, 6, 2, temperatureCTemp);
    //Set temperature Characteristic value and notify connected client
    shtTemperatureCelsiusCharacteristics.setValue(temperatureCTemp);
    shtTemperatureCelsiusCharacteristics.notify();
    Serial.print(" - Temperature: ");
    Serial.print(temp);
    Serial.println(" C");
    static char humidityTemp[6];
    dtostrf(hum, 6, 2, humidityTemp);
    //Set humidity Characteristic value and notify connected client
    shtHumidityCharacteristics.setValue(humidityTemp);
    shtHumidityCharacteristics.notify();   
    Serial.print(" - Humidity: ");
    Serial.print(hum);
    Serial.println(" %");
    lastTime = millis();
  }
  }
}
