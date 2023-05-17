//Код для клиента, который принимает температуру, влажность и освещенность 
// Добавлена освещенность, но уходит в резет(((((
#include <WiFi.h>
#include <PubSubClient.h>
#include "BLEDevice.h"
#include <Wire.h>


// Задаем параметры WIFI, к которому подключается микроконтроллер
const char* ssid = "IpHoNeBuLaT";
const char* password = "12345567";

// IP-адрес MQTT-брокера
//const char *mqtt_server = "m4.wqtt.ru";                 //const char* mqtt_server = "ksia.ddwarf.ru";
//const char *mqtt_username = "u_0ZVHCT";
//const char *mqtt_password = "bs5yit2y";
//const int mqtt_port = 7990;                            //const int mqtt_port = 1883;
const char* mqtt_server = "172.20.10.2";
const int mqtt_port = 1883;
WiFiClient espClient; // Инициализация нашей платы как сетевого клиента WIFI
PubSubClient client(espClient); //Частично инициализированный экземпляр клиента MQTT.





//BLE Server name (the other ESP32 name running the server sketch)
#define bleServerName "BME280_ESP32"

/* UUID's of the service, characteristic that we want to read */
// BLE Service
static BLEUUID bmeServiceUUID("91bad492-b950-4226-aa2b-4ede9fa42f59");
static BLEUUID lightServiceUUID("570e256b-9f4a-4e4a-9a6f-6e5bda9cf4ea");
static BLEUUID ppmServiceUUID("089346ba-1b42-43ed-9d0f-76a6d903be96");

// BLE Characteristics
//Temperature Celsius Characteristic
static BLEUUID temperatureCharacteristicUUID("cba1d466-344c-4be3-ab3f-189f80dd7518");


// Humidity Characteristic
static BLEUUID humidityCharacteristicUUID("ca73b3ba-39f6-4ab3-91ae-186dc9577d99");

// Light Characteristic
static BLEUUID lightingCharacteristicUUID("a82aeff6-eb48-48a2-b6da-f9b5e5462c91");

// PPM Characteristic
static BLEUUID ppmCharacteristicUUID("1ecb9150-24cd-410e-8156-6f02c5b7ca9d");

//Flags stating if should begin connecting and if the connection is up
static boolean doConnect = false;
static boolean connected = false;

//Address of the peripheral device. Address will be found during scanning...
static BLEAddress *pServerAddress;
 
//Characteristicd that we want to read
static BLERemoteCharacteristic* temperatureCharacteristic;
static BLERemoteCharacteristic* humidityCharacteristic;
static BLERemoteCharacteristic* lightingCharacteristic;
static BLERemoteCharacteristic* ppmCharacteristic;

//Activate notify
const uint8_t notificationOn[] = {0x1, 0x0};
const uint8_t notificationOff[] = {0x0, 0x0};


//Variables to store temperature and humidity
char* temperatureChar;
char* humidityChar;
uint32_t temperature_uint;
uint32_t humidity_uint;
uint32_t light_uint;
uint32_t ppm_uint;

//Flags to check whether new temperature and humidity readings are available
boolean newTemperature = false;
boolean newHumidity = false;
boolean newLight = false;
boolean newppm = false;

void setup_wifi() {
  delay(10);
  // Начинаем подключаться к WiFi сети
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

// Функция, которая вызывается, когда приходит сообщение с верхнего уровня
//char *topic - тема, в которую пришло сообщение
//byte *message - полезная нагрузка сообщения
//unsigned int length - длина полезной нагрузки сообщения



void callback(char *topic, byte *message, unsigned int length) {
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
  //Serial.print("Message:");
  //for (int i = 0; i < length; i++) {
  //Serial.print((char) message[i]);
  //}
  Serial.println();
  Serial.println("-----------------------");
  /*if (String(topic) == "/commands/ab941888-c303-11ed-afa1-0242ac120002") {    //получение топика команды/насос
    flag = 1;
    if (message[0] == '0') Mode = 0;                                            //Ручной режим
    if (message[0] == '1') Mode = 1;                                            //По таймеру
    if (message[0] == '2') Mode = 2;                                            //По датчику
    if (message[0] == '3') Mode = 3;                                            //Ремонт
  }
  if (String(topic) == "/commands/708ebcc4-358d-4988-8b34-30cd708866d5"){
    if (message[0] == '0') button = 0;                                            //Выкл насос
    if (message[0] == '1') button = 1;                                             //Вкл насос
  }
  if (String(topic) == "/IoTmanager")
  {
    client.publish("/IoTmanager/lampochka/config", mes);
    Serial.println(mes);
  }*/
}
// функция, отвечающая за подключение к mqtt-брокеру
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    //if (client.connect("ESP32Client", mqtt_username, mqtt_password, "mytopic/status", 2, true,  "'{ \"clientId\": \"any\", \"state\": \"offline\" }' ")) {
    if (client.connect("ESP32Client", "mytopic/status", 2, true,  "'{ \"clientId\": \"any\", \"state\": \"offline\" }' ")) {      
      client.publish("/lwt", "test_signal");
      Serial.println("connected");
      // Subscribe
      client.subscribe("/commands/ab941888-c303-11ed-afa1-0242ac120002");
      client.subscribe("/commands/708ebcc4-358d-4988-8b34-30cd708866d5");
      client.subscribe("/IoTmanager");                     //!!! ПОДПИСЫВАЕМСЯ НА НУЖНЫЕ ТЕМЫ, ESP32 подписан на тему esp32/test_back  для получения сообщений, опубликованных в этой теме
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      //Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


//Connect to the BLE Server that has the name, Service, and Characteristics
bool connectToServer(BLEAddress pAddress) {
   BLEClient* pClient = BLEDevice::createClient();
 
  // Connect to the remove BLE Server.
  pClient->connect(pAddress);
  Serial.println(" - Connected to server");
 
  // Obtain a reference to the service we are after in the remote BLE server.
  BLERemoteService* pRemoteServiceONE = pClient->getService(bmeServiceUUID);
  BLERemoteService* pRemoteServiceTWO = pClient->getService(lightServiceUUID); 
  BLERemoteService* pRemoteServiceTHREE = pClient->getService(ppmServiceUUID);  
  if (pRemoteServiceONE == nullptr) {
    Serial.print("Failed to find our service UUID: ");
    Serial.println(bmeServiceUUID.toString().c_str());
    return (false);
  }
 
  // Obtain a reference to the characteristics in the service of the remote BLE server.
  temperatureCharacteristic = pRemoteServiceONE->getCharacteristic(temperatureCharacteristicUUID);
  humidityCharacteristic = pRemoteServiceONE->getCharacteristic(humidityCharacteristicUUID);
  lightingCharacteristic = pRemoteServiceTWO->getCharacteristic(lightingCharacteristicUUID);  
  ppmCharacteristic = pRemoteServiceTHREE->getCharacteristic(ppmCharacteristicUUID);
  if (temperatureCharacteristic == nullptr || humidityCharacteristic == nullptr || lightingCharacteristic == nullptr || ppmCharacteristic == nullptr) {
    Serial.print("Failed to find our characteristic UUID");
    return false;
  }
  Serial.println(" - Found our characteristics");
 
  //Assign callback functions for the Characteristics
  temperatureCharacteristic->registerForNotify(temperatureNotifyCallback);
  humidityCharacteristic->registerForNotify(humidityNotifyCallback);
  lightingCharacteristic->registerForNotify(lightNotifyCallback);
  ppmCharacteristic->registerForNotify(ppmNotifyCallback);
  return true;
}

//Callback function that gets called, when another device's advertisement has been received
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    if (advertisedDevice.getName() == bleServerName) { //Check if the name of the advertiser matches
      advertisedDevice.getScan()->stop(); //Scan can be stopped, we found what we are looking for
      pServerAddress = new BLEAddress(advertisedDevice.getAddress()); //Address of advertiser is the one we need
      doConnect = true; //Set indicator, stating that we are ready to connect
      Serial.println("Device found. Connecting!");
    }
  }
};
 
//When the BLE Server sends a new temperature reading with the notify property
static void temperatureNotifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, 
                                        uint8_t* pData, size_t length, bool isNotify) {                                          
  temperature_uint = pData[0];
    for(int i = 1; i < length; i++) {
      temperature_uint = temperature_uint | (pData[i] << i*8);
    }
  //Serial.print(temperature_uint);
newTemperature = true;  
}



//When the BLE Server sends a new humidity reading with the notify property
static void humidityNotifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, 
                                    uint8_t* pData, size_t length, bool isNotify) {
  //store humidity value
  humidity_uint = pData[0];
  for(int i = 1; i < length; i++) {
    humidity_uint = humidity_uint | (pData[i] << i*8);
  }
  //humidityChar = (char*)pData;
  newHumidity = true;
}

//When the BLE Server sends a new lighting reading with the notify property
static void lightNotifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, 
                                    uint8_t* pData, size_t length, bool isNotify) {
  //store humidity value
  light_uint = pData[0];
  for(int i = 1; i < length; i++) {
    light_uint = light_uint | (pData[i] << i*8);
  }
  //humidityChar = (char*)pData;
  newLight = true;
}

static void ppmNotifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, 
                                    uint8_t* pData, size_t length, bool isNotify) {
  //store humidity value
  ppm_uint = pData[0];
  for(int i = 1; i < length; i++) {
    ppm_uint = ppm_uint | (pData[i] << i*8);
  }
  //humidityChar = (char*)pData;
  newppm = true;
}

//function that prints the latest sensor readings
void printReadings(){
  Serial.print(" Humidity:");
  Serial.print(humidity_uint);
  Serial.println("%");
  String humidityString = (String)humidity_uint;
  String humidityStringjson = String("{ value: ")+humidityString+String(" }");   
  const char* humiditymessage = humidityStringjson.c_str();
  client.publish("/humidity", humiditymessage);
  Serial.print("Temperature:");
  Serial.print(temperature_uint);
  Serial.print("C");
  String temperatureString = (String)temperature_uint;
  String temperatureStringjson = String("{ value: ")+temperatureString+String(" }"); 
  const char* temperaturemessage = temperatureStringjson.c_str();  
  client.publish("/temperature", temperaturemessage);  
  Serial.print("Lighting:");
  Serial.print(light_uint);
  Serial.print("%");
  String lightString = (String)light_uint;
  String lightStringjson = String("{ value: ")+lightString+String(" }"); 
  const char* lightmessage = lightStringjson.c_str();  
  client.publish("/lighting", lightmessage);  
  Serial.print(" PPM:");
  Serial.print(ppm_uint);
  Serial.print(" ppm");
  String ppmString = (String)ppm_uint;
  String ppmStringjson = String("{ value: ")+ppmString+String(" }"); 
  const char* ppmmessage = ppmStringjson.c_str();  
  client.publish("/ppm", ppmmessage); 
}

void setup() {
  //OLED display setup
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
 
  
  //Start serial communication
  Serial.begin(115200);
  Serial.println("Starting Arduino BLE Client application...");

  //Init BLE device
  BLEDevice::init("");
 
  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 30 seconds.
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  pBLEScan->start(30);

  setup_wifi();// Функция установления соединения по WiFi
  client.setServer(mqtt_server, mqtt_port); // Инициализация экземпляра клиента MQTT
  client.setCallback(callback);
  
}

void loop() {
  // If the flag "doConnect" is true then we have scanned for and found the desired
  // BLE Server with which we wish to connect.  Now we connect to it.  Once we are
  // connected we set the connected flag to be true.
  if (doConnect == true) {
    if (connectToServer(*pServerAddress)) {
      Serial.println("We are now connected to the BLE Server.");
      //Activate the Notify property of each Characteristic
      temperatureCharacteristic->getDescriptor(BLEUUID((uint16_t)0x2902))->writeValue((uint8_t*)notificationOn, 2, true);
      humidityCharacteristic->getDescriptor(BLEUUID((uint16_t)0x2902))->writeValue((uint8_t*)notificationOn, 2, true);
      lightingCharacteristic->getDescriptor(BLEUUID((uint16_t)0x2902))->writeValue((uint8_t*)notificationOn, 2, true);
      ppmCharacteristic->getDescriptor(BLEUUID((uint16_t)0x2902))->writeValue((uint8_t*)notificationOn, 2, true);
      connected = true;
    } else {
      Serial.println("We have failed to connect to the server; Restart your device to scan for nearby BLE server again.");
    }
    doConnect = false;
  }
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  //if new temperature readings are available, print in the OLED
  if (newTemperature && newppm && newHumidity && newLight){
    newTemperature = false;
    newHumidity = false;
    newLight = false;
    newppm = false;
    printReadings();
  }
  delay(1000); // Delay a second between loops.
}
