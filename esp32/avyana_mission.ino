/*
  AVYANA AUV — Full Mission ESP32 Code
  RoboFest 5.0 | GEC Bhavnagar

  ARM SWITCH  : GPIO33 → GND
  KILL SWITCH : Hardware only
  GRIPPER     : GPIO32 (servo)
*/

#include <ESP32Servo.h>
#include <Wire.h>
#include <Adafruit_BNO08x.h>

// ===== PINS =====
#define FL_PIN  21
#define FR_PIN  22
#define RL_PIN  23
#define RR_PIN  19
#define VF_PIN  18
#define VR_PIN  5
#define ARM_PIN    33
#define SERVO_PIN  32
#define PI_RX   16
#define PI_TX   17
#define SDA_PIN 25
#define SCL_PIN 26

// ===== PWM =====
#define NEUTRAL 1500
#define FWD     1400
#define REV     1600

// ===== GRIPPER =====
#define GRIPPER_OPEN  90
#define GRIPPER_CLOSE 0

// ===== OBJECTS =====
Servo FL, FR, RL, RR, VF, VR;
Servo gripper;
Adafruit_BNO08x bno08x;
sh2_SensorValue_t sensorValue;

// ===== IMU =====
float yaw = 0, pitch = 0, roll = 0;
float pitchOffset = 0, rollOffset = 0;
uint8_t imuCal = 0;
bool armed = false;

// ===== CONTROL =====
float fwdSpeed  = 0;
float vertSpeed = 0;
float yawTarget = 0;

// ===== TIMING =====
unsigned long lastSend  = 0;
unsigned long lastPrint = 0;


// ═══════════════════════════════════════════════════
// SAFE DELAY — checks arm switch every 50ms
// ═══════════════════════════════════════════════════
void safeDelay(int ms) {
  int steps = ms / 50;
  for (int i = 0; i < steps; i++) {
    armed = (digitalRead(ARM_PIN) == LOW);
    if (!armed) {
      stopAll();
      Serial.println("!!! ARM OFF — STOPPED !!!");
      Serial2.println("ARM_OFF");
      while (digitalRead(ARM_PIN) == HIGH) delay(100);
      armed = true;
      Serial2.println("ARM_ON");
    }
    readIMU();
    sendToPi();
    handlePiCommand();
    delay(50);
  }
}


// ═══════════════════════════════════════════════════
// THRUSTERS
// ═══════════════════════════════════════════════════
void stopAll() {
  FL.writeMicroseconds(NEUTRAL);
  FR.writeMicroseconds(NEUTRAL);
  RL.writeMicroseconds(NEUTRAL);
  RR.writeMicroseconds(NEUTRAL);
  VF.writeMicroseconds(NEUTRAL);
  VR.writeMicroseconds(NEUTRAL);
}

void goForward() {
  FL.writeMicroseconds(FWD);
  FR.writeMicroseconds(FWD);
  RL.writeMicroseconds(FWD);
  RR.writeMicroseconds(FWD);
}

void doDive() {
  VF.writeMicroseconds(FWD);
  VR.writeMicroseconds(FWD);
}

void doSurface() {
  VF.writeMicroseconds(REV);
  VR.writeMicroseconds(REV);
}

void doUTurn() {
  FL.writeMicroseconds(FWD);
  RL.writeMicroseconds(FWD);
  FR.writeMicroseconds(REV);
  RR.writeMicroseconds(REV);
}

void setThrusters(float fwd, float yawCorr, float vert) {
  if (!armed) { stopAll(); return; }
  float fl = constrain(fwd - yawCorr, -100, 100);
  float fr = constrain(fwd + yawCorr, -100, 100);
  float bl = constrain(fwd - yawCorr, -100, 100);
  float br = constrain(fwd + yawCorr, -100, 100);
  float vl = constrain(vert, -100, 100);
  float vr = constrain(vert, -100, 100);
  FL.writeMicroseconds(map(fl, -100, 100, 1600, 1400));
  FR.writeMicroseconds(map(fr, -100, 100, 1600, 1400));
  RL.writeMicroseconds(map(bl, -100, 100, 1600, 1400));
  RR.writeMicroseconds(map(br, -100, 100, 1600, 1400));
  VF.writeMicroseconds(map(vl, -100, 100, 1600, 1400));
  VR.writeMicroseconds(map(vr, -100, 100, 1600, 1400));
}


// ═══════════════════════════════════════════════════
// GRIPPER
// ═══════════════════════════════════════════════════
void openGripper()  { gripper.write(GRIPPER_OPEN);  Serial.println("Gripper OPEN");  }
void closeGripper() { gripper.write(GRIPPER_CLOSE); Serial.println("Gripper CLOSE"); }
void dropBall() {
  Serial.println("Dropping ball...");
  openGripper();
  delay(2000);
  closeGripper();
  Serial.println("Ball dropped!");
  Serial2.println("DROP_DONE");
}


// ═══════════════════════════════════════════════════
// IMU
// ═══════════════════════════════════════════════════
void setReports() {
  bno08x.enableReport(SH2_ARVR_STABILIZED_RV, 10000);
}

void readIMU() {
  if (bno08x.wasReset()) setReports();
  if (bno08x.getSensorEvent(&sensorValue)) {
    if (sensorValue.sensorId == SH2_ARVR_STABILIZED_RV) {
      float qr=sensorValue.un.arvrStabilizedRV.real;
      float qi=sensorValue.un.arvrStabilizedRV.i;
      float qj=sensorValue.un.arvrStabilizedRV.j;
      float qk=sensorValue.un.arvrStabilizedRV.k;
      float sqr=sq(qr),sqi=sq(qi),sqj=sq(qj),sqk=sq(qk);
      yaw   = atan2(2.0*(qi*qj+qk*qr),(sqi-sqj-sqk+sqr))*RAD_TO_DEG;
      pitch = asin(-2.0*(qi*qk-qj*qr)/(sqi+sqj+sqk+sqr))*RAD_TO_DEG;
      roll  = atan2(2.0*(qj*qk+qi*qr),(-sqi-sqj+sqk+sqr))*RAD_TO_DEG;
      roll += 180.0;
      if (roll >  180) roll -= 360;
      if (roll < -180) roll += 360;
      pitch = -pitch - pitchOffset;
      roll  = roll   - rollOffset;
      imuCal = sensorValue.status & 0x03;
    }
  }
}

void autoZero() {
  Serial.println("Auto zero — keep flat...");
  pitchOffset = 0; rollOffset = 0;
  delay(3000);
  float ps=0,rs=0; int n=0;
  unsigned long t=millis();
  while (millis()-t<2000) {
    readIMU();
    ps+=pitch; rs+=roll; n++;
    delay(10);
  }
  if (n>0) { pitchOffset=ps/n; rollOffset=rs/n; }
  Serial.printf("Zero! P:%.2f R:%.2f\n",pitchOffset,rollOffset);
}


// ═══════════════════════════════════════════════════
// UART
// ═══════════════════════════════════════════════════
void sendToPi() {
  if (millis()-lastSend < 100) return;
  Serial2.printf("Y:%.2f,P:%.2f,R:%.2f,CA:%d,AR:%d\n",
    yaw,pitch,roll,imuCal,armed?1:0);
  lastSend=millis();
}

void handlePiCommand() {
  if (!Serial2.available()) return;
  String cmd=Serial2.readStringUntil('\n');
  cmd.trim();
  if (cmd.length()==0) return;
  Serial.println("CMD: "+cmd);

  if      (cmd=="PING")    Serial2.println("READY");
  else if (cmd=="STOP")  { stopAll(); fwdSpeed=0; vertSpeed=0; yawTarget=yaw; }
  else if (cmd=="REZERO"){ pitchOffset+=pitch; rollOffset+=roll; Serial2.println("ZERO_OK"); }
  else if (cmd=="DROP")    dropBall();
  else if (cmd=="OPEN")    openGripper();
  else if (cmd=="CLOSE")   closeGripper();
  else if (cmd.startsWith("FWD:"))  { fwdSpeed  = cmd.substring(4).toFloat(); setThrusters(fwdSpeed, 0, vertSpeed); }
  else if (cmd.startsWith("YAW:"))  { yawTarget = cmd.substring(4).toFloat(); }
  else if (cmd.startsWith("DEPTH:")){ vertSpeed = cmd.substring(6).toFloat(); setThrusters(fwdSpeed, 0, vertSpeed); }
}


// ═══════════════════════════════════════════════════
// SETUP
// ═══════════════════════════════════════════════════
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("=== AVYANA FULL MISSION BOOTING ===");

  pinMode(ARM_PIN, INPUT_PULLUP);
  Serial2.begin(115200, SERIAL_8N1, PI_RX, PI_TX);

  // Gripper
  gripper.setPeriodHertz(50);
  gripper.attach(SERVO_PIN, 500, 2400);
  closeGripper();
  Serial.println("Gripper closed (ball loaded)");

  // ESCs
  FL.attach(FL_PIN,1000,2000); FR.attach(FR_PIN,1000,2000);
  RL.attach(RL_PIN,1000,2000); RR.attach(RR_PIN,1000,2000);
  VF.attach(VF_PIN,1000,2000); VR.attach(VR_PIN,1000,2000);
  stopAll();
  Serial.println("Arming ESCs (5s)...");
  delay(5000);
  Serial.println("ESCs Armed!");

  // IMU
  Wire.begin(SDA_PIN, SCL_PIN);
  for (int i=1;i<=10;i++) {
    if (bno08x.begin_I2C()) { Serial.println("IMU OK!"); break; }
    delay(500);
  }
  setReports();
  autoZero();
  yawTarget = yaw;

  Serial2.println("READY");
  Serial.println("=== READY — Waiting for arm switch ===");

  // Wait for arm switch
  while (digitalRead(ARM_PIN)==HIGH) {
    readIMU(); sendToPi(); handlePiCommand();
    if (millis()-lastPrint>1000) {
      Serial.printf("Waiting... CAL:%d Y:%.1f\n",imuCal,yaw);
      lastPrint=millis();
    }
    delay(50);
  }

  armed=true;
  Serial.println("=== ARMED! Pi controls mission ===");
  Serial2.println("ARM_ON");
  // Pi takes over from here!
  // ESP32 just responds to Pi commands
}


// ═══════════════════════════════════════════════════
// LOOP — Pi sends commands, ESP32 executes
// ═══════════════════════════════════════════════════
void loop() {
  armed = (digitalRead(ARM_PIN)==LOW);
  if (!armed) stopAll();

  readIMU();

  // Yaw correction when Pi sends YAW command
  float yawError = yawTarget - yaw;
  if (yawError >  180) yawError -= 360;
  if (yawError < -180) yawError += 360;
  float yawCorr = constrain(yawError * 0.3, -40, 40);
  setThrusters(fwdSpeed, yawCorr, vertSpeed);

  sendToPi();
  handlePiCommand();

  if (millis()-lastPrint>500) {
    Serial.printf("ARM:%d CAL:%d Y:%.1f P:%.1f R:%.1f FWD:%.0f YAW:%.1f VERT:%.0f\n",
      armed,imuCal,yaw,pitch,roll,fwdSpeed,yawTarget,vertSpeed);
    lastPrint=millis();
  }
  delay(10);
}
