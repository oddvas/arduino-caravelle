#include <OneWire.h>
#include <DallasTemperature.h>
#include <DHT.h>

OneWire oneWire(4);
DallasTemperature sensors(&oneWire);
DHT dht;

static DeviceAddress REFRIDGERATOR_THERMOMETER = {0x28, 0x5e, 0x6b, 0x5c, 0x04, 0x00, 0x00, 0x16};
static DeviceAddress WATER_TANK_THERMOMETER = {0x28, 0x56, 0xa0, 0x5d, 0x04, 0x00, 0x00, 0x6a};
static DeviceAddress ELECTRONICS_THERMOMETER = {0x28, 0x70, 0xaf, 0x82, 0x04, 0x00, 0x00, 0x9a};
static const int PROGMEM AVERAGE_MEASUREMENTS = 50;
static const int PROGMEM TEMPERATURE_DELAY = 30000;

float vcc;
long lastTempUpdate = 0;

static int voltage;
static int refridgeratorCurrent;

static boolean refridgeratorOpen;
static boolean refridgeratorRunning;
static int batteryVoltage;

volatile int NbTopsFan = 0;
long lastFlowMeasured = 0;
float totalWaterFlow = 0.0;

void setup() {
  voltage = 10 * (5 * analogRead(A1) / 1023.0 * 200 * 0.06 - 30);
  refridgeratorCurrent = 10 * abs(vcc * (analogRead(A0) - 512) / 102.4 + 0.04);
  refridgeratorRunning = false;
  refridgeratorOpen = false;
  batteryVoltage = 12;
  Serial.begin(9600);
  sensors.begin();
  
  for (int i = 5; i <=12; i++) {
    pinMode(i, OUTPUT);
    digitalWrite(i, HIGH);
  }
  
  pinMode(2, INPUT);
  attachInterrupt(0, rpm, RISING);
  
  dht.setup(3);
  
  updateTemperature();
}

void loop(void) {
  //digitalWrite(12, LOW);
  if (millis() - lastTempUpdate > TEMPERATURE_DELAY) {
    updateTemperature();
  }
  if(Serial.available() > 0) {
    switch (Serial.read()) {
      case 'r':
        switch (Serial.read()) {
          case 'z':
            if (Serial.read() == '1') {
              digitalWrite(12, LOW);
              sendCommand("rz1");
            } else {
              digitalWrite(12, HIGH);
              sendCommand("rz0");
            }
            break;
          case 'f':
            if (Serial.read() == '1') {
              digitalWrite(5, LOW);
              sendCommand("rf1");
            } else {
              digitalWrite(5, HIGH);
              sendCommand("rf0");
            }
            break;
        }
        break;
      case 'w':
        switch (Serial.read()) {
          case 'z':
            if (Serial.read() == '1') {
              digitalWrite(11, LOW);
              sendCommand("wz1");
            } else {
              digitalWrite(11, HIGH);
              sendCommand("wz0");
            }
            break;
          case 'v':
            if (Serial.read() == '1') {
              digitalWrite(7, LOW);
              sendCommand("wv1");
            } else {
              digitalWrite(7, HIGH);
              sendCommand("wv0");
            }
        }
    }
  }
  
  vcc = readVcc()/1000.0;
  voltage += (vcc * analogRead(A1) / 1023.0 * 200 * 0.06 - 30) * 10;
  voltage /= 2;
  refridgeratorCurrent += 10 * abs(vcc * (analogRead(A0) - 512) / 102.4 + 0.04);
  refridgeratorCurrent /= 2;
  if (refridgeratorCurrent > 2) {
    if (!refridgeratorRunning) sendCommand("rr1");
    refridgeratorRunning = true;
  } else if (refridgeratorRunning) {
    sendCommand("rr0");
    refridgeratorRunning = false;
  }
  if (voltage != batteryVoltage) {
    batteryVoltage = voltage;
    //sendCommand("bv" + String(batteryVoltage));
    Serial.println(batteryVoltage);
  }
  if (analogRead(A2) > 1) {
    if (!refridgeratorOpen) sendCommand("rd1");
    refridgeratorOpen = true;
  } else if (refridgeratorOpen) {
    sendCommand("rd0");
    refridgeratorOpen = false;
  }
  
  if (millis() - lastFlowMeasured > 1000) {
    float flow = NbTopsFan * 60 / 7.5;
    Serial.print("Flow: ");
    Serial.println(flow);
    totalWaterFlow += flow / 3600;
    Serial.print("Total: ");
    Serial.println(totalWaterFlow);
    NbTopsFan = 0;
    lastFlowMeasured = millis();
  }
  
  delay(100);
}

String addressToString(DeviceAddress deviceAddress) {
  String address = "";
  for (uint8_t i = 0; i < 8; i++)
  {
    if (deviceAddress[i] < 16) address = address + "0";
    address = address + String(deviceAddress[i], HEX); 
    //Serial.println(String(deviceAddress[i], HEX));
  }
  return address;
}

long readVcc() {
  long result;
  // Read 1.1V reference against AVcc
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Convert
  while (bit_is_set(ADCSRA,ADSC));
  result = ADCL;
  result |= ADCH<<8;
  result = 1125300L / result; // Back-calculate AVcc in mV
  return result;
}

void updateTemperature() {
  float coolTemp;
  DeviceAddress currentThermometer;
  sensors.requestTemperatures();
  oneWire.reset_search();
  for (uint8_t i = 0; i < sensors.getDeviceCount(); i++) {
    oneWire.search(currentThermometer);
    Serial.print(addressToString(currentThermometer));
    Serial.print(" - ");
    Serial.println(sensors.getTempC(currentThermometer));
  }
  Serial.print("Humidity = ");
  Serial.println(dht.getHumidity());
  Serial.print("Temperature = ");
  Serial.println(dht.getTemperature());
  coolTemp = sensors.getTempC(REFRIDGERATOR_THERMOMETER);
  sendCommand("rt" + String((int)(coolTemp * 100)));
  if (coolTemp < 1.0) {
    digitalWrite(5, HIGH);
  }
  else if (coolTemp > 3.0) {
    //digitalWrite(5, LOW);
  }
  sendCommand("wt" + String((int)(sensors.getTempC(WATER_TANK_THERMOMETER) * 100)));
  lastTempUpdate = millis();
}

void sendCommand(String command) {
  Serial.println("<" + command + ">");
}

void rpm() {
  NbTopsFan++;
}
