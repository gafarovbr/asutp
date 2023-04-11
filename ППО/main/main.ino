// ПЕРЕДАЕТ ТРИ ЗНАЧЕНИЯ БЕЗ ПРОБЛЕМ (ОСВЕЩЕНИЕ, ВЛАЖНОСТЬ, ТЕМПЕРАТУРУ)
#include "BLEDevice.h"
#include <WiFi.h>
#include <PubSubClient.h>
//#include <ArduinoJson.h>
//#include "time.h"
#include <NTPClient.h>


//NTP-сервер
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

// BLE
#define bleServerName "PODOK_ESP32" // название сервера, к которому мы подключаемся
// ШИМ
#define dutyCycle 255

// BLE Service
static BLEUUID bmeServiceUUID("91bad492-b950-4226-aa2b-4ede9fa42f59");


// задаем UUID характеристики температуры
static BLEUUID temperatureCharacteristicUUID("cba1d466-344c-4be3-ab3f-189f80dd7518"); 
// задаем UUID характеристики влажности
static BLEUUID humidityCharacteristicUUID("ca73b3ba-39f6-4ab3-91ae-186dc9577d99");

// задаем UUID характеристики освещенности
static BLEUUID OSVCharacteristicUUID("aeefac6e-3f16-4f69-8941-b8238b949687");



//Какие-то адреса без них не работает BLE дискриптор 
const uint8_t notificationOn[] = {0x1, 0x0};
const uint8_t notificationOff[] = {0x0, 0x0};


// Объявление переменных, которые будут использоваться позже с Bluetooth, чтобы проверить, подключены ли мы к серверу или нет.
static boolean doConnect = false;
static boolean connected = false;

//Переменная относится к адресу сервера, к которому мы хотим подключиться. Этот адрес будет найден при сканировании.
static BLEAddress *pServerAddress;
//Переменные характеристик, которые мы хотим считать
static BLERemoteCharacteristic* temperatureCharacteristic;
static BLERemoteCharacteristic* humidityCharacteristic;
static BLERemoteCharacteristic* OSVCharacteristic;

//переменные char для хранения значений(value) температуры и влажности, полученных от сервера.
char* TempChar;
char* humidityChar;
char* OSVChar;

//Переменные используются для проверки наличия новых показаний температуры
boolean newTemperature = false;
boolean newHumidity = false;
boolean newOSV = false;




  
// Задаем параметры WIFI, к которому подключается микроконтроллер
const char* ssid = "IpHoNeBuLaT";
const char* password = "12345567";

// IP-адрес MQTT-брокера
 const char* mqtt_server = "m7.wqtt.ru";                //const char* mqtt_server = "ksia.ddwarf.ru";
const char *mqtt_username = "u_XFB8NC";
const char *mqtt_password = "hkWbB2Nw";
const int mqtt_port = 14918;                                                        //const int mqtt_port = 1883;

WiFiClient espClient; // Инициализация нашей платы как сетевого клиента WIFI
PubSubClient client(espClient); //Частично инициализированный экземпляр клиента MQTT.


byte state; // Переменная отвечающая за состояния насоса
bool flag = 0;  // флаг, отвечающий за получение команды переключения режима
int Mode;
int ModeFin = 0; // Задаем изначально ручной режим.
bool button = 0; // Переменная, отвечающая за включение насоса
//bool test_variable;
//char* mes; // переменная, отвечающая за строку, преобразованную из json-объекта и формирующую кнопку, что подключение выполнено
volatile double WaterFlow; // Переменная считающая, расход воды в литрах
volatile double WaterFlowPrev;
uint32_t myTimer1; // Таймер, который отвечает за то, чтобы значения датчика влажности передавались раз в 5 сек.
int tempSum1 = 0, temp1; // датчики влажности
int tempSum2 = 0, temp2;
int tempSum3 = 0, temp3;
byte tempCounter;// датчик влажности



// номер порта
const int pinD13 = 13;  // Инициализация порта ШИМ
const int pinD34 = 34;  // Инициализация портов датчиков влажности почвы
const int pinD36 = 36;                    
const int pinD39 = 39;
const int pinD14  = 14; // Пин, к которому подключено реле клапана
const int pinD02 = 2;   // Пин, к которому подключено реле осветительного прибора 
const int pinD00 = 0;   // Пин, к которому подключено реле клапана
const int pinD16 = 16;  // Пин, к которому подключен датчик уровня воды

// задаём свойства ШИМ-сигнала
const int freq = 5000;
const int ledChannel = 0;
const int resolution = 8;







//Подключение к серверу
bool connectToServer(BLEAddress pAddress) {
   BLEClient* pClient = BLEDevice::createClient();
 
  // Подключение к серверу.
  pClient->connect(pAddress);
  Serial.println(" - Connected to server");
 
  // Получение ссылки на service который нам нужен из сервера.
  BLERemoteService* pRemoteService = pClient->getService(bmeServiceUUID);
  if (pRemoteService == nullptr) {
    Serial.print("Failed to find our service UUID: ");
    Serial.println(bmeServiceUUID.toString().c_str());
    return (false);
  }
 
  //  Получение ссылок на Characteristic которые нам нужны из сервера
  temperatureCharacteristic = pRemoteService->getCharacteristic(temperatureCharacteristicUUID);   
  humidityCharacteristic = pRemoteService->getCharacteristic(humidityCharacteristicUUID);
  OSVCharacteristic = pRemoteService->getCharacteristic(OSVCharacteristicUUID);
if (temperatureCharacteristic == nullptr || humidityCharacteristic == nullptr || OSVCharacteristic == nullptr) {
    Serial.print("Failed to find our characteristic UUID");
    return false;
  }
  Serial.println(" - Found our characteristics");
 
  //Назначение callback функций для характеристик
  temperatureCharacteristic->registerForNotify(temperatureNotifyCallback); 
  humidityCharacteristic->registerForNotify(humidityNotifyCallback);
  OSVCharacteristic->registerForNotify(OSVNotifyCallback);
  return true;
}
// Эта функция вызывается, когда сканер находит какое-либо устройство. 
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    if (advertisedDevice.getName() == bleServerName) { //Проверка на соответствие с именем сервера, который нам нужен
      advertisedDevice.getScan()->stop(); //Останавливаем сканирование
      pServerAddress = new BLEAddress(advertisedDevice.getAddress()); //Адрес устройства нужного нам
      doConnect = true; //Установка флага, что мы нашли устройство и готовы к подключению
      Serial.println("Device found. Connecting!");
    }
  }
};


//Функции, срабатыващие при получении значений
static void temperatureNotifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
  //store temperature value
  
  TempChar = (char*)pData;
  newTemperature = true;
}

static void humidityNotifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
  //store humidity value
  humidityChar = (char*)pData;
  newHumidity = true;
}


static void OSVNotifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, 
                                    uint8_t* pData, size_t length, bool isNotify) {
//  store humidity value
  OSVChar = (char*)pData;
  newOSV = true;
}


void setup() {
  Serial.begin(115200);
  //Инициализация устройства BLE.
  BLEDevice::init("");
  // Следующие методы выполняют поиск ближайших устройств
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  pBLEScan->start(30);

  setup_wifi();// Функция установления соединения по WiFi
  client.setServer(mqtt_server, mqtt_port); // Инициализация экземпляра клиента MQTT
  client.setCallback(callback); // Устанавливает функцию обратного вызова сообщения callback (Эта функция вызывается, когда клиенту приходят новые сообщения).

  timeClient.begin();// Инициализация NTP-клиента
  timeClient.setTimeOffset(10800);


  
  pinMode (pinD34, INPUT); // Установим вывод D34 как вход (Датчики влажности)
  pinMode (pinD36, INPUT); // Установим вывод D36 как вход
  pinMode (pinD39, INPUT); // Установим вывод D39 как вход
  pinMode (pinD14, OUTPUT); // Установим вывод D14 как выход (Клапана)
  pinMode (pinD00, OUTPUT); // Установим вывод D00 как выход
  pinMode(pinD16, INPUT_PULLUP); // Конфигурируем вывод как вход, подтягивая его до уровня логической «1» через внутренний подтягивающий резистор
  ledcSetup(ledChannel, freq, resolution);
  
  // привязываем канал к порту
  ledcAttachPin(pinD13, ledChannel);



  
  WaterFlow = 0; // Обнуляем расход
  attachInterrupt(27, pulse, RISING); //Настраиваем прерывания на 27 пин (Пин, к которому подключен датчик расхода жидкости)
 }

// функция отвечающая за печать значений в монитор порта и передачи их на MQTT-сервер.
void printReadings()
{
  Serial.print(" Temperature:");
  Serial.print(TempChar);
  Serial.println("C");
  client.publish("/temperature", TempChar);
  Serial.print(" Humidity:");
  Serial.print(humidityChar); 
  Serial.println("%");
  client.publish("/humidity", humidityChar);
  Serial.print(" OSV:");
  Serial.print(OSVChar); 
  Serial.println("%");
  client.publish("/OSV", OSVChar);
  //Serial.print(" soil1:");
 // Serial.print((char*)temp1); 
  //client.publish("/Soil1", (char*)temp1);
  //Serial.print(" soil2:");
  //Serial.print((char*)temp2);
  //client.publish("/Soil2", (char*)temp2);
  //Serial.print(" soil3:");
  //Serial.print((char*)temp3);
  //client.publish("/Soil3", (char*)temp3);
  
}


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
  if (String(topic) == "/commands/ab941888-c303-11ed-afa1-0242ac120002") {    //получение топика комманды/насос
    flag == 1;
    if (message[0] == '0') Mode = 0;                                            //Ручной режим
    if (message[0] == '1') Mode = 1;                                            //По таймеру
    if (message[0] == '2') Mode = 2;                                            //По датчику
    if (message[0] == '2') Mode = 3;                                            //Ремонт
  }
  if (String(topic) == "/commands/708ebcc4-358d-4988-8b34-30cd708866d5"){
    if (message[0] == '0') button = 0;                                            //Выкл насос
    if (message[0] == '1') button = 1;                                             //Вкл насос
  }
 /* if (String(topic) == "/IoTmanager")
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
    if (client.connect("ESP32Client", mqtt_username, mqtt_password)) {
      Serial.println("connected");
      // Subscribe
      client.subscribe("/commands/ab941888-c303-11ed-afa1-0242ac120002");
      client.subscribe("/commands/708ebcc4-358d-4988-8b34-30cd708866d5");
      client.subscribe("/IoTmanager");                     //!!! ПОДПИСЫВАЕМСЯ НА НУЖНЫЕ ТЕМЫ, ESP32 подписан на тему esp32/test_back  для получения сообщений, опубликованных в этой теме
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}





int stateIM();
void StateProcessing();
void ModeSwitching();
void BlockControl();
void WateringControl();
void watering()
{
  state = stateIM();
  StateProcessing();
  ModeSwitching();
  BlockControl();
  WateringControl();
}


int stateIM()
{

  volatile double error = 0.2; // Какое-то количество литров, которое может пройти жидкость после выключения
  switch(button){
    case 0: // если насос выключен
    if ((WaterFlow - WaterFlowPrev) > error)// Значения датчика не корректны 
    {
      return 0; // Активация состояния "ИМ отключен и не исправен"
    }
    else return 1; // Активация состояния "ИМ отключен и готов к работе"
    break;
    case 1: // Если насос включен
    if (WaterFlow > WaterFlowPrev)// Значения датчика корректны 
    {
      return 4; // Активация состояния "ИМ в работе"
    }
    else 
    {
      if (digitalRead(pinD16)) return 3; // Активация состояния "ИМ не исправен"
      else 
      return 2; // Активация состояния "ИМ исправен и не готов к работе" и активация неисправности "Отсутствие воды в резервуаре"
    }
    break; 
  }
}
void BlockControl()
{
  // Уровень воды учли в stateIM, ремонт учли ModeSwitching
}



void WateringControl()
{
uint32_t tmr1;
volatile double WaterFlowPrev;
  switch(ModeFin)
  {
    case 0: // ручной режим
    if (button) {
      digitalWrite(pinD14, HIGH);
      digitalWrite(pinD00, HIGH);
      ledcWrite(ledChannel, dutyCycle);
      if (millis() - tmr1 >= 6000000) {  // Если включено больше 10 минут, то выключаем и кнопку включения на выключения
      tmr1 = millis();   // Сбрасываем таймер
      digitalWrite(pinD14, LOW);
      digitalWrite(pinD00, LOW);
      WaterFlowPrev = WaterFlow;
      button = 0;
    }
    else {
      tmr1 = millis(); // Сбрасываем таймер
      digitalWrite(pinD14, LOW);
      digitalWrite(pinD00, LOW);
      WaterFlowPrev = WaterFlow;
      ledcWrite(ledChannel, 0);
    }
    break;
    case 1:// по таймеру
    if((timeClient.getHours() == 8) && (timeClient.getMinutes() > 0) && (timeClient.getMinutes() < 2)) { // Полив в период с 8:00 по 8:02
      digitalWrite(pinD14, HIGH);
      digitalWrite(pinD00, HIGH);
      ledcWrite(ledChannel, dutyCycle);
    }
    else {
      digitalWrite(pinD14, LOW);
      digitalWrite(pinD00, LOW);
      WaterFlowPrev = WaterFlow;
      ledcWrite(ledChannel, 0);
    }
    break;
    case 2: // по датчику
    if ((0 < temp1) && (temp1 <300)){  // Если значения датчика, подключеного к пину D34 низкие, то включить насос и клапан pinD14
      digitalWrite(pinD14, HIGH);
      ledcWrite(ledChannel, dutyCycle);
    }
    else {
    digitalWrite(pinD14, LOW);
    WaterFlowPrev = WaterFlow;
    ledcWrite(ledChannel, 0);
    }


    if (((0 < temp2) && (temp2 <300))||((0 < temp3) && (temp3 <300))){// Если значения датчика, подключеного к пину D36 или к пину D39 низкие, то включить насос и клапан pinD00
      digitalWrite(pinD00, HIGH);
      ledcWrite(ledChannel, dutyCycle);
    }
    else {
    digitalWrite(pinD00, LOW);
    WaterFlowPrev = WaterFlow;
    ledcWrite(ledChannel, 0);
    }
    break;
  }
  } 
}
void ModeSwitching(){
  bool block;
  if (flag == 1 && state == 1 && block == 0) // Можем менять режим только если пришла команда с верх уровня (флаг), насос исправен и НЕ в работе, и режим НЕ ремонт
  {
    switch (Mode){
    case 0:
    ModeFin = 0;
    // скорее всего клиент паблиш в статус.
    break;
    case 1:
    ModeFin = 1;
    break;
    case 2:
    ModeFin = 2;
    break;
    case 3:
    ModeFin = 3;
    block = 1;
    break;
    }
  }
  if (Mode == 4 && button == 0) 
  {
  block = 0;
  ModeFin = 4;
  }
}
void StateProcessing() // Думаю, будем передавать значения о состояниях насоса на верхний уровень
{
  switch(state){ 
    case 0:
    break;
    case 1:
    break;
    case 2:
    break;
    case 3:
    break;
    case 4:
    break;
  }
}
void pulse() //Счётчик прямоугольных имульсов и перевод их в литры.
 {
 WaterFlow += 1.0 / 5880.0;
 } 

void getTemp() {
  // суммируем влажность в общую переменную
  tempSum1 += analogRead(pinD34);
  tempSum2 += analogRead(pinD36);
  tempSum3 += analogRead(pinD39);
  // счётчик измерений
  tempCounter++;
  if (tempCounter >= 5) { // если больше 5
    tempCounter = 0;  // обнулить
    temp1 = tempSum1 / 5; // среднее арифметическое
    temp2 = tempSum2 / 5;
    temp3 = tempSum3 / 5;
    tempSum1 = 0;  // обнулить
    tempSum2 = 0;
    tempSum3 = 0;
  }
}
void loop() {
  timeClient.update();// Обновление времени
  if (!client.connected()) {
    reconnect();
  }
  client.loop();//Его следует вызывать регулярно, чтобы клиент мог обрабатывать входящие сообщения и поддерживать соединение с сервером.
if (doConnect == true) {
if (connectToServer(*pServerAddress)) {
  Serial.println("We are now connected to the BLE Server.");
  //Запускаем дескриптор после подсоединения (обр. связь)
  temperatureCharacteristic->getDescriptor(BLEUUID((uint16_t)0x2902))->writeValue((uint8_t*)notificationOn, 2, true);
  humidityCharacteristic->getDescriptor(BLEUUID((uint16_t)0x2902))->writeValue((uint8_t*)notificationOn, 2, true);
  //OSVCharacteristic->getDescriptor(BLEUUID((uint16_t)0x2902))->writeValue((uint8_t*)notificationOn, 2, true);
  connected = true;
    } else {
      Serial.println("We have failed to connect to the server; Restart your device to scan for nearby BLE server again.");
    }
    doConnect = false;
  }

  
  watering(); // Функция полива
    if (newTemperature && newHumidity && newOSV){
    newTemperature = false;
    newHumidity = false;
    newOSV = false;
    printReadings();
  }
  if (millis() - myTimer1 >= 1000) {
    myTimer1 = millis(); // сбросить таймер
    getTemp();
  }

}
