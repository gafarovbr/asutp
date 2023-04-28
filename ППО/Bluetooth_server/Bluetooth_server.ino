//Код для сервера, который передает температуру и влажность


#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Wire.h>
#include <iarduino_I2C_SHT.h>  //   Подключаем библиотеку для работы с датчиком температуры и влажности I2C-flash.


//Default Temperature is in Celsius
//Comment the next line for Temperature in Fahrenheit
#define temperatureCelsius

//BLE server name
#define bleServerName "BME280_ESP32"

int pinD27 = 27; // Пин, к которому подключен датчик освещенности

iarduino_I2C_SHT sht;
uint32_t temp;
uint32_t hum;
uint32_t light;

// Timer variables
unsigned long lastTime = 0;
unsigned long timerDelay = 5000;

bool deviceConnected = false;

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/
#define SERVICE_UUID_ONE "91bad492-b950-4226-aa2b-4ede9fa42f59" // Сервис для датчика температуры и влажности
#define SERVICE_UUID_TWO "570e256b-9f4a-4e4a-9a6f-6e5bda9cf4ea" // Сервис для датчика освещенности

// Temperature Characteristic and Descriptor
BLECharacteristic bmeTemperatureCelsiusCharacteristics("cba1d466-344c-4be3-ab3f-189f80dd7518", BLECharacteristic::PROPERTY_NOTIFY);
BLEDescriptor bmeTemperatureCelsiusDescriptor(BLEUUID((uint16_t)0x2902));


// Humidity Characteristic and Descriptor
BLECharacteristic bmeHumidityCharacteristics("ca73b3ba-39f6-4ab3-91ae-186dc9577d99", BLECharacteristic::PROPERTY_NOTIFY);
BLEDescriptor bmeHumidityDescriptor(BLEUUID((uint16_t)0x2902));


// Lighting Characteristic and Descriptor
BLECharacteristic lightingCharacteristics("a82aeff6-eb48-48a2-b6da-f9b5e5462c91", BLECharacteristic::PROPERTY_NOTIFY);
BLEDescriptor lightingDescriptor(BLEUUID((uint16_t)0x2902));

//Setup callbacks onConnect and onDisconnect
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) {
    deviceConnected = true;
  };
  void onDisconnect(BLEServer *pServer) {
    deviceConnected = false;
  }
};


void setup() {
  // Start serial communication
  Serial.begin(115200);
  pinMode (pinD27, INPUT);
  // Init BME Sensor
  sht.begin();

  // Create the BLE Device
  BLEDevice::init(bleServerName);

  // Create the BLE Server
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *bmeService = pServer->createService(SERVICE_UUID_ONE);
  BLEService *lightService = pServer->createService(SERVICE_UUID_TWO);
  // Create BLE Characteristics and Create a BLE Descriptor
  // Temperature
  bmeService->addCharacteristic(&bmeTemperatureCelsiusCharacteristics);
  bmeTemperatureCelsiusDescriptor.setValue("BME temperature Celsius");
  bmeTemperatureCelsiusCharacteristics.addDescriptor(&bmeTemperatureCelsiusDescriptor);


  // Humidity
  bmeService->addCharacteristic(&bmeHumidityCharacteristics);
  bmeHumidityDescriptor.setValue("BME humidity");
  bmeHumidityCharacteristics.addDescriptor(&bmeHumidityDescriptor);


  // Lighting
  lightService->addCharacteristic(&lightingCharacteristics);
  lightingDescriptor.setValue("Lighting");
  lightingCharacteristics.addDescriptor(&lightingDescriptor);
  
  // Start the service
  bmeService->start();
  lightService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID_ONE);
  pAdvertising->addServiceUUID(SERVICE_UUID_TWO);  
  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify...");
}

void loop() {
  if (deviceConnected) {
    if ((millis() - lastTime) > timerDelay) {
      // Read temperature as Celsius (the default)
      temp = (uint32_t)sht.getTem();
      // Read humidity
      hum = (uint32_t)sht.getHum();
      // Read lighting
      light = (uint32_t)(100 - (0.02442*analogRead(pinD27)));
      
      
      //Notify temperature reading from BME sensor
      static char temperatureCTemp[6];
      dtostrf(temp, 6, 2, temperatureCTemp);
      //Set temperature Characteristic value and notify connected client
      bmeTemperatureCelsiusCharacteristics.setValue(temp);
      bmeTemperatureCelsiusCharacteristics.notify();
      Serial.print("Temperature Celsius: ");
      Serial.print(temp);
      Serial.print(" ºC");


      //Notify humidity reading from BME
      static char humidityTemp[6];
      dtostrf(hum, 6, 2, humidityTemp);
      //Set humidity Characteristic value and notify connected client
      bmeHumidityCharacteristics.setValue(hum);
      bmeHumidityCharacteristics.notify();
      Serial.print(" - Humidity: ");
      Serial.print(hum);
      Serial.println(" %");

      //Notify light reading 
      lightingCharacteristics.setValue(light);
      lightingCharacteristics.notify();
      Serial.print(" - Lighting: ");
      Serial.print(light);
      Serial.println(" %");

      lastTime = millis();
    }
  }
}
