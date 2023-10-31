#include <ModbusRTUSlave.h>
#include <DHT.h>
#include <Wire.h>
#include <cm1106_i2c.h>

CM1106_I2C cm1106_i2c;

#define CM1107
#define DHTPIN 2     // AM2305B(DHT22) 센서의 데이터 핀 설정
#define DHTTYPE DHT22   // 사용하는 센서 유형
#define analogPin A0   // 암모니아 센서를 연결한 아날로그 핀

const word bufSize = 256;
byte buf[bufSize];


ModbusRTUSlave rtu(Serial, buf);
u16 _D0[400];
DHT dht(DHTPIN, DHTTYPE);

void FloatToHex(float f, byte* hex) {
  byte* f_byte = reinterpret_cast<byte*>(&f);
  memcpy(hex, f_byte, 4);
}

void setup() {
  Serial.begin(9600);
  dht.begin();
  cm1106_i2c.begin();
  Serial.begin(9600);
  delay(1000);

  rtu.begin(1, 9600, SERIAL_8N1);
  _D0[3] = 11;
  _D0[4] = 1001;
  _D0[5] = 10;
  _D0[6] = 30;
  _D0[101] = 1;
  _D0[104] = 1;
  _D0[107] = 1;
  _D0[108] = 1;
  for(int i = 1; i<=400; i++){
    if( i !=3 || i !=4 || i !=5 || i !=6 || i != 101 || i != 104 || i != 107 || i != 108){
      _D0[i] = 0;
    }
  }
  

  // Configure the holding registers
  rtu.configureHoldingRegisters(_D0, 400);

}

void loop() {

  uint8_t ret = cm1106_i2c.measure_result();


 
  // Read temperature from the DHT sensor
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  float ammonia = analogRead(analogPin);
  float co2 = (float) cm1106_i2c.co2;

  byte tempHex[4] = {0};
  FloatToHex(temperature, tempHex);
  word tempValue_h = (tempHex[1] << 8) | tempHex[0];
  word tempValue_l = (tempHex[3] << 8) | tempHex[2];

  byte humidityHex[4] = {0};
  FloatToHex(humidity, humidityHex);
  word humidityValue_h = (humidityHex[1] << 8) | humidityHex[0];
  word humidityValue_l = (humidityHex[3] << 8) | humidityHex[2];
  
  byte ammoniaHex[4] = {0};
  FloatToHex(ammonia, ammoniaHex);
  word ammoniaValue_h = (ammoniaHex[1] << 8) | ammoniaHex[0];
  word ammoniaValue_l = (ammoniaHex[3] << 8) | ammoniaHex[2];

  byte co2Hex[4] = {0};
  FloatToHex(co2, co2Hex);
  word co2Value_h = (co2Hex[1] << 8) | co2Hex[0];
  word co2Value_l = (co2Hex[3] << 8) | co2Hex[2];


  // Check if valid readings are obtained from the sensor
  if (!isnan(humidity) && !isnan(temperature)) {
    // Update _D0[203] and _D0[204] with the new temperature values
    _D0[203] = tempValue_h;
    _D0[204] = tempValue_l;
    _D0[212] = humidityValue_h;
    _D0[213] = humidityValue_l;
    _D0[202] = 0;
    _D0[205] = 0;
    _D0[214] = 0;
  }
  else{
    _D0[202] = 1;
    _D0[203] = 0;
    _D0[204] = 0;
    _D0[212] = 0;
    _D0[213] = 0;
    _D0[205] = 1;
    _D0[214] = 1;
  }
  if (ret == 0 && !isnan(co2))
  {
    _D0[221] = co2Value_h;
    _D0[222] = co2Value_l;
    _D0[223] = 0;
  }
  else
  {
    _D0[221] = 0;   
    _D0[222] = 0;
    _D0[223] = 1;
  }
  if (!isnan(ammonia) && ammonia >=0 && ammonia < 100){

    _D0[224] = ammoniaValue_h;
    _D0[225] = ammoniaValue_l;
    _D0[226] = 0;
  }
  else
  {
    _D0[224] = 0;
    _D0[225] = 0;
    _D0[226] = 1;
  }
  // Poll the Modbus RTU slave
  rtu.poll();

  // Add a delay here if needed to control the rate of sensor readings
}
