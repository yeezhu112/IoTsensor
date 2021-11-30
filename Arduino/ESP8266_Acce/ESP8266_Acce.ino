#include "Wire.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
 
const uint8_t MPU_addr=0x68; // I2C address of the MPU-6050
 
const float MPU_GYRO_250_SCALE = 131.0;
const float MPU_GYRO_500_SCALE = 65.5;
const float MPU_GYRO_1000_SCALE = 32.8;
const float MPU_GYRO_2000_SCALE = 16.4;
const float MPU_ACCL_2_SCALE = 16384.0;
const float MPU_ACCL_4_SCALE = 8192.0;
const float MPU_ACCL_8_SCALE = 4096.0;
const float MPU_ACCL_16_SCALE = 2048.0;
 
const char* ssid = "haha";
const char* password = "ppcc51123";
const char* mqtt_server = "192.168.0.162";

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE  (50)
char msg[MSG_BUFFER_SIZE];
 
struct rawdata {
int16_t AcX;
int16_t AcY;
int16_t AcZ;
int16_t Tmp;
int16_t GyX;
int16_t GyY;
int16_t GyZ;
};
 
struct scaleddata{
float AcX;
float AcY;
float AcZ;
float Tmp;
float GyX;
float GyY;
float GyZ;
};
 
bool checkI2c(byte addr);
void mpu6050Begin(byte addr);
rawdata mpu6050Read(byte addr, bool Debug);
void setMPU6050scales(byte addr,uint8_t Gyro,uint8_t Accl);
void getMPU6050scales(byte addr,uint8_t &Gyro,uint8_t &Accl);
scaleddata convertRawToScaled(byte addr, rawdata data_in,bool Debug);

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, HIGH);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is active low on the ESP-01)

  } else {
    digitalWrite(BUILTIN_LED, LOW);  // Turn the LED off by making the voltage HIGH

  }

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("toESP", "hello world");
      // ... and resubscribe
      client.subscribe("fromESP");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
 
void setup() {
pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
Wire.begin();
Serial.begin(115200);

setup_wifi();
client.setServer(mqtt_server, 1883);
client.setCallback(callback);
 
mpu6050Begin(MPU_addr);
}
 
void loop() {
rawdata next_sample;
setMPU6050scales(MPU_addr,0b00000000,0b00010000);

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  
next_sample = mpu6050Read(MPU_addr, true);
convertRawToScaled(MPU_addr, next_sample,true);
 
delay(5000); // Wait 5 seconds and scan again
}
 
  
 
void mpu6050Begin(byte addr){
// This function initializes the MPU-6050 IMU Sensor
// It verifys the address is correct and wakes up the
// MPU.
if (checkI2c(addr)){
Wire.beginTransmission(MPU_addr);
Wire.write(0x6B); // PWR_MGMT_1 register
Wire.write(0); // set to zero (wakes up the MPU-6050)
Wire.endTransmission(true);
 
delay(30); // Ensure gyro has enough time to power up
}
}
 
bool checkI2c(byte addr){
// We are using the return value of
// the Write.endTransmisstion to see if
// a device did acknowledge to the address.
Serial.println(" ");
Wire.beginTransmission(addr);
 
if (Wire.endTransmission() == 0)
{
Serial.print(" Device Found at 0x");
Serial.println(addr,HEX);
return true;
}
else
{
Serial.print(" No Device Found at 0x");
Serial.println(addr,HEX);
return false;
}
}
 
  
 
rawdata mpu6050Read(byte addr, bool Debug){
// This function reads the raw 16-bit data values from
// the MPU-6050
 
rawdata values;
 
Wire.beginTransmission(addr);
Wire.write(0x3B); // starting with register 0x3B (ACCEL_XOUT_H)
Wire.endTransmission(false);
Wire.requestFrom(addr,14,true); // request a total of 14 registers
values.AcX=Wire.read()<<8|Wire.read(); // 0x3B (ACCEL_XOUT_H) & 0x3C (ACCEL_XOUT_L)
values.AcY=Wire.read()<<8|Wire.read(); // 0x3D (ACCEL_YOUT_H) & 0x3E (ACCEL_YOUT_L)
values.AcZ=Wire.read()<<8|Wire.read(); // 0x3F (ACCEL_ZOUT_H) & 0x40 (ACCEL_ZOUT_L)
values.Tmp=Wire.read()<<8|Wire.read(); // 0x41 (TEMP_OUT_H) & 0x42 (TEMP_OUT_L)
values.GyX=Wire.read()<<8|Wire.read(); // 0x43 (GYRO_XOUT_H) & 0x44 (GYRO_XOUT_L)
values.GyY=Wire.read()<<8|Wire.read(); // 0x45 (GYRO_YOUT_H) & 0x46 (GYRO_YOUT_L)
values.GyZ=Wire.read()<<8|Wire.read(); // 0x47 (GYRO_ZOUT_H) & 0x48 (GYRO_ZOUT_L)
 
 
if(Debug){
Serial.print(" GyX = "); Serial.print(values.GyX);
Serial.print(" | GyY = "); Serial.print(values.GyY);
Serial.print(" | GyZ = "); Serial.print(values.GyZ);
Serial.print(" | Tmp = "); Serial.print(values.Tmp);
Serial.print(" | AcX = "); Serial.print(values.AcX);
Serial.print(" | AcY = "); Serial.print(values.AcY);
Serial.print(" | AcZ = "); Serial.println(values.AcZ);


}
 
return values;
}
 
void setMPU6050scales(byte addr,uint8_t Gyro,uint8_t Accl){
Wire.beginTransmission(addr);
Wire.write(0x1B); // write to register starting at 0x1B
Wire.write(Gyro); // Self Tests Off and set Gyro FS to 250
Wire.write(Accl); // Self Tests Off and set Accl FS to 8g
Wire.endTransmission(true);
}
 
void getMPU6050scales(byte addr,uint8_t &Gyro,uint8_t &Accl){
Wire.beginTransmission(addr);
Wire.write(0x1B); // starting with register 0x3B (ACCEL_XOUT_H)
Wire.endTransmission(false);
Wire.requestFrom(addr,2,true); // request a total of 14 registers
Gyro = (Wire.read()&(bit(3)|bit(4)))>>3;
Accl = (Wire.read()&(bit(3)|bit(4)))>>3;
}
 
  
 
scaleddata convertRawToScaled(byte addr, rawdata data_in, bool Debug){
 
scaleddata values;
float scale_value = 0.0;
byte Gyro, Accl;
 
getMPU6050scales(MPU_addr, Gyro, Accl);
 
if(Debug){
Serial.print("Gyro Full-Scale = ");
}
 
switch (Gyro){
case 0:
scale_value = MPU_GYRO_250_SCALE;
if(Debug){
Serial.println("±250 °/s");
}
break;
case 1:
scale_value = MPU_GYRO_500_SCALE;
if(Debug){
Serial.println("±500 °/s");
}
break;
case 2:
scale_value = MPU_GYRO_1000_SCALE;
if(Debug){
Serial.println("±1000 °/s");
}
break;
case 3:
scale_value = MPU_GYRO_2000_SCALE;
if(Debug){
Serial.println("±2000 °/s");
}
break;
default:
break;
}
 
values.GyX = (float) data_in.GyX / scale_value;
values.GyY = (float) data_in.GyY / scale_value;
values.GyZ = (float) data_in.GyZ / scale_value;
 
scale_value = 0.0;
if(Debug){
Serial.print("Accl Full-Scale = ");
}
switch (Accl){
case 0:
scale_value = MPU_ACCL_2_SCALE;
if(Debug){
Serial.println("±2 g");
}
break;
case 1:
scale_value = MPU_ACCL_4_SCALE;
if(Debug){
Serial.println("±4 g");
}
break;
case 2:
scale_value = MPU_ACCL_8_SCALE;
if(Debug){
Serial.println("±8 g");
}
break;
case 3:
scale_value = MPU_ACCL_16_SCALE;
if(Debug){
Serial.println("±16 g");
}
break;
default:
break;
}
values.AcX = (float) data_in.AcX / scale_value;
values.AcY = (float) data_in.AcY / scale_value;
values.AcZ = (float) data_in.AcZ / scale_value;
 
  
 
values.Tmp = (float) data_in.Tmp / 340.0 + 36.53;
 
if(Debug){
Serial.print(" GyX = "); Serial.print(values.GyX-2.13);
Serial.print(" °/s| GyY = "); Serial.print(values.GyY-2.20);
Serial.print(" °/s| GyZ = "); Serial.print(values.GyZ+0.17);
Serial.print(" °/s| Tmp = "); Serial.print(values.Tmp);
Serial.print(" °C| AcX = "); Serial.print(values.AcX-0.11);
Serial.print(" g| AcY = "); Serial.print(values.AcY+0.13);
Serial.print(" g| AcZ = "); Serial.print(values.AcZ-0.93);Serial.println(" g");

snprintf (msg, MSG_BUFFER_SIZE, "%.2f", values.GyX-2.13);
client.publish("toESP_GyX", msg);
snprintf (msg, MSG_BUFFER_SIZE, "%.2f", values.GyY-2.20);
client.publish("toESP_GyY", msg);
snprintf (msg, MSG_BUFFER_SIZE, "%.2f", values.GyZ+0.17);
client.publish("toESP_GyZ", msg);
snprintf (msg, MSG_BUFFER_SIZE, "%.2f", values.Tmp);
client.publish("toESP_GyTmp", msg);
snprintf (msg, MSG_BUFFER_SIZE, "%.2f", values.AcX-0.11);
client.publish("toESP_ACX", msg);
snprintf (msg, MSG_BUFFER_SIZE, "%.2f", values.AcY+0.13);
client.publish("toESP_ACY", msg);
snprintf (msg, MSG_BUFFER_SIZE, "%.2f", values.AcZ-0.93);
client.publish("toESP_ACZ", msg);



}
 
return values;
}
