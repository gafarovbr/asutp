void watering() //Основная функция, вызывающая все остальные
{
  Serial.println("Я в вотеринг");
  state = StateIM();
  StateProcessing();
  ModeSwitching();
  BlockControl();
  WateringControl();
}

int StateIM() //Функция, возвращающая состояние системы (0 - ИМ отключен,  1 - ИМ в работе, 2 - нет воды в резервуаре)
{

  volatile double error = 0.2; // Какое-то количество литров, которое может пройти жидкость после выключения
  if (digitalRead(pinD16)){
  switch(stateIM){
    case 0: // если насос выключен
    return 0; // Активация состояния "ИМ отключен"
    break;
    case 1: // Если насос включен
      return 1; // Активация состояния "ИМ в работе"
    break; 
  }
  }
  else
  {
    return 2;//Активация состояния "Отсутствие воды в резервуаре"
  }
}


void StateProcessing() // Вывод переменной state(состояние насоса) в сериал принт
{
  switch(state){ 
    case 0:
    Serial.println("ИМ отключен");
    break;
    case 1:
    Serial.println("ИМ в работе");
    break;
    case 2:
    Serial.println("Отсутствие воды в резервуаре");
    break;
  }
}

void ModeSwitching() //Функция отвечает за смену режима
{
  if (flag == 1 && (state == 0 || state == 2) && block == 0) // Можем менять режим только если пришла команда с верх уровня (флаг), насос НЕ в работе, и режим НЕ ремонт
  {
    switch (Mode){
    case 0:
    ModeFin = 0;
    client.publish("/sensors/edd0c1b4-9f30-40f0-b31d-6bea2d618666", "Ручной режим");
    flag = 0;
    // скорее всего клиент паблиш в статус.
    break;
    case 1:
    ModeFin = 1;
    client.publish("/sensors/edd0c1b4-9f30-40f0-b31d-6bea2d618666", "Режим по таймеру");
    flag = 0;
    break;
    case 2:
    ModeFin = 2;
    client.publish("/sensors/edd0c1b4-9f30-40f0-b31d-6bea2d618666", "Режим по датчику");
    flag = 0;
    break;
    case 3:
    ModeFin = 3;
    client.publish("/sensors/edd0c1b4-9f30-40f0-b31d-6bea2d618666", "Режим ремонт");
    flag = 0;
    block = 1;
    break;
    }
  }
  if (flag == 1 && Mode == 4){
    if (button == 0){ //Можем отключить режим ремонт, если насос выключен
    block = 0;
    ModeFin = 4;
    client.publish("/status/edd0c1b4-9f30-40f0-b31d-6bea2d618666", "Выход из режима ремонт");
    }
    flag = 0; // Говорим, что обработали команду с верхнего уровня о смене режима
  }
  if (flag == 1)
  {
    client.publish("/status/edd0c1b4-9f30-40f0-b31d-6bea2d618666", "Нельзя поменять режим работы");
    flag = 0;
  }
}
void BlockControl() //Пока ненужная функция
{
  // Уровень воды учли в stateIM, ремонт учли ModeSwitching
}

void WateringControl() //Управляет поливом (включает, выключает ИМ в соответствии с режимом работы)
{
  Serial.print("Режим - ");
  Serial.println(ModeFin);
  uint32_t tmr1;
  volatile double WaterFlowPrev;
  switch(ModeFin)
  {
    case 0:
    
    Serial.println("Я в ручном режиме!!");
    if (button == 1){
     // digitalWrite(pinD02, LOW);
      digitalWrite(pinD00, LOW);
      digitalWrite(pinD14, LOW);
      stateIM = 1;
    //  Serial.println("Мы везде подаем лоу");
      ledcWrite(ledChannel, dutyCycle);
      if (millis() - tmr1 >= 6000000) {  // Если включено больше 10 минут, то выключаем и кнопку включения на выключения
      tmr1 = millis();   // Сбрасываем таймер
      digitalWrite(pinD14, HIGH);
      digitalWrite(pinD00, HIGH);
      stateIM = 0;
      ledcWrite(ledChannel, 0);
    //   Serial.println("Мы везде подаем high но после 10 минут");
      WaterFlowPrev = WaterFlow;
      button = 0;
    }
    }
    else {
      tmr1 = millis(); // Сбрасываем таймер
     // digitalWrite(pinD02, HIGH);
      digitalWrite(pinD14, HIGH);
      digitalWrite(pinD00, HIGH);
   //   Serial.println("Мы везде подаем high так как button == 0");
      WaterFlowPrev = WaterFlow;
      ledcWrite(ledChannel, 0);
      stateIM = 0;
    }
    break;
    case 1:// по таймеру
    Serial.println("Я в режиме таймер!!");
    if((timeClient.getHours() == 20) && (timeClient.getMinutes() > 0) && (timeClient.getMinutes() < 60)) { // Полив в период с 8:00 по 8:02
      Serial.print("Часы:");
      Serial.println(timeClient.getHours());
      Serial.print("Минуты");
      Serial.println(timeClient.getMinutes());
      digitalWrite(pinD14, LOW);
      digitalWrite(pinD00, LOW);
      ledcWrite(ledChannel, dutyCycle);
      stateIM = 1;
    }
    else {
      digitalWrite(pinD14, HIGH);
      digitalWrite(pinD00, HIGH);
      stateIM = 0;
      WaterFlowPrev = WaterFlow;
      ledcWrite(ledChannel, 0);
    }
    break;
    case 2: // по датчику
    //if ((0 < temp1) && (temp1 < 300)){  // Если значения датчика, подключеного к пину D34 низкие, то включить насос и клапан pinD14
    //  digitalWrite(pinD14, LOW);
    //  ledcWrite(ledChannel, dutyCycle);
    //  stateIM = 1;
    //}
    //else {
    //  digitalWrite(pinD14, HIGH);
    //  WaterFlowPrev = WaterFlow;
    //  ledcWrite(ledChannel, 0);
    //  stateIM = 0;
    //}


    //if (((0 < temp2) && (temp2 < 300))||((0 < temp3) && (temp3 < 300))){// Если значения датчика, подключеного к пину D36 или к пину D39 низкие, то включить насос и клапан pinD00
    //  digitalWrite(pinD00, LOW);
    //  stateIM = 1;
    //  ledcWrite(ledChannel, dutyCycle);
    //}
    //else {
    // digitalWrite(pinD00, HIGH);
    //  stateIM = 0;
    //  WaterFlowPrev = WaterFlow;
    //  ledcWrite(ledChannel, 0);
    //}
    break;
  }
} 

void pulse() //Счётчик прямоугольных имульсов и перевод их в литры.
 {
 WaterFlow += 1.0 / 5880.0;
 } 

void getTemp() { //Получение значений влажности почвы
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
