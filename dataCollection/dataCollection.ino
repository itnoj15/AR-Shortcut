#include <Arduino_LSM9DS1.h>
#include <Arduino_BMI270_BMM150.h>

BoschSensorClass IMU(Wire1);
LSM9DS1Class IMUExternal(Wire);

void setup() {
  Serial.begin(9600);
  while (!Serial); 
  Serial.println("Started");

  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU 1!");
    while (1);
  }

  if (!IMUExternal.begin()) {
    Serial.println("Failed to initialize IMU2!");
    while (1);
  }
}

void loop() {
  float Ax, Ay, Az, Gx, Gy, Gz, AEx, AEy, AEz, GEx, GEy, GEz;

  // 내부 IMU 데이터 읽기
  if (IMU.accelerationAvailable()) {
    IMU.readAcceleration(Ax, Ay, Az);
    Serial.print(Ax);
    Serial.print(",");
    Serial.print(Ay);
    Serial.print(",");
    Serial.print(Az);
    Serial.print(",");
  }
  if (IMU.gyroscopeAvailable()) {
    IMU.readGyroscope(Gx, Gy, Gz);
    Serial.print(Gx);
    Serial.print(",");
    Serial.print(Gy);
    Serial.print(",");
    Serial.print(Gz);
    Serial.print(",");
  }

  // 외부 IMU 데이터 읽기
  if (IMUExternal.accelerationAvailable()) {
    IMUExternal.readAcceleration(AEx, AEy, AEz);
    Serial.print(AEx);
    Serial.print(",");
    Serial.print(AEy);
    Serial.print(",");
    Serial.print(AEz);
    Serial.print(",");
  }
  if (IMUExternal.gyroscopeAvailable()) {
    IMUExternal.readGyroscope(GEx, GEy, GEz);
    Serial.print(GEx);
    Serial.print(",");
    Serial.print(GEy);
    Serial.print(",");
    Serial.println(GEz);
  }  
}
