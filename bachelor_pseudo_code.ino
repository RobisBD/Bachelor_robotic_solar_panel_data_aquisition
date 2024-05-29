#include <Arduino_JSON.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_INA260.h>
#include <ESP32Servo.h>
#include <Wire.h>
#include <EEPROM.h>
#include <TinyGPS++.h>

// MOTOR DEFINITIONS
// Motor back left
int motor_bck_r_m = 27;
int motor_bck_r_en = 14;

// Motor back right
int motor_bck_l_m = 13;
int motor_bck_l_en = 12;

// Motor front left
int motor_frnt_l_m = 19; 
int motor_frnt_l_en = 18; 

// Motor front right
int motor_frnt_r_m = 5; 
int motor_frnt_r_en = 17; 

// SOLAR TRACKER
int pos_x = 90;
int pos_y = 45;
int max_pos = 180;
int min_pos = 0;
int min_y_pos = 40;
int prc_to_pos_conv_koef = 1  ;
int x_axis_pin = 2;
int y_axis_pin = 4;

int photo_rez_bottom_right = 26;
int photo_rez_top_right = 25;
int photo_rez_bottom_left = 33;
int photo_rez_top_left = 32;

// GPS
static const int RXPin = 1, TXPin = 3;

// BATTERY
int batteryRead = 15;

// ULTRASONIC SENSOR
int trigPin = 34;
int echoPin = 35;

// WIFI DEFINITIONS (NOT SHOWN)
const char* ssid = "some network";
const char* password = "somepassword";

// MQTT Broker IP address
const char* mqtt_server = "10.10.10.175";

// MQQT COMMUNICATION VARIABLES
long lastMsg = 0;
char msg[50];
int value = 0;

// INSTANTIATING 
Adafruit_INA260 ina260 = Adafruit_INA260();
Servo x_axis;
Servo y_axis;
WiFiClient espClient;
PubSubClient client(espClient);
TinyGPSPlus gps;


// WEATHER & SUN DATA
int cloud_prcntg = 0;
int sun_lat = 0;
int sun_lng = 0;

bool readySystem;
bool day;

void setup() {
  Serial.begin(115200);
  EEPROM.begin(20);
  // WIFI AND MQTT SETUP
  bool connectedToNetowrk = setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // SETUP INA260
  bool foundPowerMeter = setupPowerMeter();

  // SERVO SETUP + ADD before position retrieval!!!!!!!!!!!!!!!!!!!!!!!!!!!
  setupServos();
  
  // MOTOR SETUP
  setupMotors();

  // Ultrasonic sensor
  bool ultrasonicReady = setupHCSR();

  // GPS
  bool readyGPS = setupGPS();

  if (!foundPowerMeter) {
    client.publish("esp32/error", "{\"robotId\": \"ROBOT0001\", \"error\": \"powerMeter\"}");
    readySystem = false;
  }
  if (!readyGPS) {
    client.publish("esp32/error", "{\"robotId\": \"ROBOT0001\", \"error\": \"GPS\"}");
    readySystem = false;
  }
  if (!ultrasonicReady) {
    client.publish("esp32/error", "{\"robotId\": \"ROBOT0001\", \"error\": \"HC-SR04\"}");
    readySystem = false;
  }
  readySystem = true;
}

void loop() {
  // If system not ready, don't start main script!
  if(!readySystem) {
    while(true) {
      Serial.println("System not ready!");
      delay(1000);
    }
  }

  // System ready, proceed
  if (!client.connected()) {
      reconnect();
    }
    client.loop();

  // get data from server
  if (sun_lat == 0 && sun_lng == 0) {
    // data not received from server yet! Delay for 5 minutes and repeat check
    delay(300000);
  } else {
    // check platform
    day = true;
    bool checkupComplete = platformCheckup();
    int ph1;
    int ph2;
    int ph3;
    int ph4;

    if(checkupComplete) {
      // check succeded, position tracker and platform
      ph1 = analogRead(photo_rez_bottom_right);
      ph2 = analogRead(photo_rez_top_right);
      ph3 = analogRead(photo_rez_bottom_left);
      ph4 = analogRead(photo_rez_top_left);

      mapToPercentage(ph1, ph3, ph2, ph4);

      // get data and send to server
      gatherAndSendToServer(ph1, ph2, ph3, ph4);

      // check if day has ended
      if (!day) {
        sun_lat = 0;
        sun_lng = 0;
        readySystem = false;
      }
    } else {
      readySystem = false;
    }
  }
}

bool setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  unsigned long wifiStart = millis();

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");

    unsigned long wifiElapsed = millis();

    if ((wifiElapsed - wifiStart) >= 10000) {
      return false;
    }
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  return true;
}

void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    messageTemp += (char)message[i];
  }
  String parsedMsg = JSON.stringify(messageTemp);
  Serial.print(parsedMsg);
  Serial.println();
  if("topic" == "server/startup") {
    cloud_prcntg = (parsedMsg.substring(16, 19)).toInt();
    sun_lat = (parsedMsg.substring(23, 32)).toInt();
    sun_lng = (parsedMsg.substring(26, 45)).toInt();
    return;
  }
  if("topic" == "server/dayEnd") {
    day = false;
  }
  if("topic" == "server/cloudiness") {
    cloud_prcntg = (parsedMsg.substring(16, 19)).toInt();
  }
}

bool reconnect() {
  int attempts = 0;
  bool connected = false;
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP32_Solar_Robot")) {
      Serial.println("connected");

      // Subscribe
      client.subscribe("server/api-data");
      connected = true;
    } else {
      if (attempts >= 5) {
        break;
      }
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
    attempts += 1;
  }
  return connected;
}

void mapToPercentage(int bottom_right, int bottom_left, int top_right, int top_left) {
  int bottom_r_prc = map(bottom_right, 0, 3800, 0, 100);
  int bottom_l_prc = map(bottom_left, 0, 3800, 0, 100);
  int top_r_prc = map(top_right, 0, 3800, 0, 100);
  int top_l_prc = map(top_left, 0, 3800, 0, 100);

  Serial.print("Bottom right illuminance: ");
  Serial.print(bottom_r_prc);
  Serial.println("%");
  Serial.print("Bottom left illuminance: ");
  Serial.print(bottom_l_prc);
  Serial.println("%");
  Serial.print("Top right illuminance: ");
  Serial.print(top_r_prc);
  Serial.println("%");
  Serial.print("Top left illuminance: ");
  Serial.print(top_l_prc);
  Serial.println("%");
  Serial.println("-------------------------");

  // calculate displacement for x & y axis 
  // x axis direction: negative value -> move left, positive value -> move right
  // y axis direction: negative value -> move up, positive value -> move down

  int x_displacement_prc = (bottom_r_prc + top_r_prc) - (bottom_l_prc + top_l_prc);
  int y_displacement_prc = (bottom_r_prc + bottom_l_prc) - (top_r_prc + top_l_prc);
  Serial.print("x displacement: ");
  Serial.print(x_displacement_prc);
  Serial.println("%");
  Serial.print("y displacement: ");
  Serial.print(y_displacement_prc);
  Serial.println("%");
  Serial.println("-------------------------");

  // 3% - -3% disallignment is allowed
  if (y_displacement_prc > 3 || y_displacement_prc < -3) {
    reAdjustPanelPositionY(y_displacement_prc);
  }
  if (x_displacement_prc > 3 || x_displacement_prc < -3) {
    reAdjustPanelPositionX(x_displacement_prc);
  }

}


void reAdjustPanelPositionY(int disp_y){
  // calculate new position & guard clauses
  int displacement_koef = map(disp_y, -100, 100, -10, 10);
  if (displacement_koef == 0) {
    displacement_koef = 1;
  }
  int next_pos_y;

  // calculate the next position depending on the displacement percentage
  if (disp_y > 0){
    next_pos_y = pos_y - 1 * abs(displacement_koef);
  } else {
    next_pos_y = pos_y + 1 * abs(displacement_koef);
  }

  // Max or min position reached for servos, set the position to max or min for Y axis!
  if (next_pos_y > max_pos) {
    Serial.println("Need to move robot, Y angles not available!");
    if (cloud_prcntg < 50){
      moveBack();
    }
    Serial.println("-------------------------");
    return;
  }
  if (next_pos_y < min_y_pos) {
	  y_axis.write(min_y_pos);
    Serial.println("Set Y position to minimum!");
    Serial.println("-------------------------");
    return;
  }

  // change position of panel to new positions
	y_axis.write(next_pos_y);
  // set the current position to the next position
  pos_y = next_pos_y;
  EEPROM.write(4, next_pos_y);
  EEPROM.commit();
  Serial.print("Servo Y axis new positions: ");
  Serial.println(next_pos_y);
  Serial.println("-------------------------");

}

void reAdjustPanelPositionX(int disp_x){
  // calculate new position & guard clauses
  int displacement_koef = map(disp_x, -100, 100, -10, 10);
  if (displacement_koef == 0) {
    displacement_koef = 1;
  }
  int next_pos_x;

  // calculate the next position depending on the displacement percentage
  if (disp_x > 0){
    next_pos_x = pos_x - 1 * abs(displacement_koef);
  } else {
    next_pos_x = pos_x + 1 * abs(displacement_koef);
  }

  // Max or min position reached for servos, need to reposition robor with drive!
  if (next_pos_x < min_pos) {
    Serial.println("Need to move robot (left?), angles not available!");
    Serial.println("-------------------------");
    if (cloud_prcntg < 50){
      turnSomeDegreesLeft();
    }
    return;
  }
  if (next_pos_x > max_pos) {
    Serial.println("Need to move robot (right?), angles not available!");
    Serial.println("-------------------------");
    if (cloud_prcntg < 50){
      turnSomeDegreesRight();
    }
    return;
  }

  // change position of panel to new positions
  x_axis.write(next_pos_x);
  // set the current position to the next position
  pos_x = next_pos_x;
  Serial.print("Servo X axis new positions: ");
  Serial.println(next_pos_x);
  Serial.println("-------------------------");
}

void motorDirectionControl(int m_bck_l, int m_bck_r, int m_frnt_l, int m_frnt_r) {
  digitalWrite(motor_bck_l_m, m_bck_l);
  digitalWrite(motor_bck_r_m, m_bck_r);
  digitalWrite(motor_frnt_l_m, m_frnt_l);
  digitalWrite(motor_frnt_r_m, m_frnt_r);
} 
void motorSpeedControl(int bck_l, int bck_r, int frnt_l, int frnt_r) {
  analogWrite(motor_bck_l_en, bck_l);
  analogWrite(motor_bck_r_en, bck_r);
  analogWrite(motor_frnt_l_en, frnt_l);
  analogWrite(motor_frnt_r_en, frnt_r);
}

void moveBack() {
  motorDirectionControl(LOW, HIGH, LOW, HIGH);

  for(value = 0 ; value <= 255; value+=5)
  {
    motorSpeedControl(value, value, value, value);
    delay(30);
  }
}
void moveForward() {
  motorDirectionControl(HIGH, LOW, HIGH, LOW);

  for(value = 0 ; value <= 255; value+=5)
  {
    motorSpeedControl(value, value, value, value);
    delay(30);
  }
}

void turnSomeDegreesLeft() {
  int value;
    
  // Set direction to forward
  motorDirectionControl(HIGH, LOW, HIGH, LOW);
  
  // Provide some momentum for turning
  for(value = 0 ; value <= 255; value+=5)
  {
    motorSpeedControl(value, value, value, value);
    delay(30);
  }

  // Turn off left motors to change their direction
  motorSpeedControl(0, 255, 0, 255);
  delay(500);

  // Change left motor directions & accelerate maximum
  motorDirectionControl(LOW, LOW, LOW, LOW);
  motorSpeedControl(255, 255, 255, 255);

  delay(1000);

  // Stop the motors
  motorSpeedControl(0, 0, 0, 0);

  delay(1000);

  // Set direction to backwards
  motorDirectionControl(LOW, HIGH, LOW, HIGH);

  // Provide some momentum for turning
  for(value = 0 ; value <= 255; value+=5)
  {
    motorSpeedControl(value, value, value, value);
    delay(30);
  }

  // Turn off right motors to change their direction
  motorSpeedControl(255, 0, 255, 0);
  delay(500);

  // Change right motor directions & accelerate maximum
  motorDirectionControl(LOW, LOW, LOW, LOW);
  motorSpeedControl(255, 255, 255, 255);

  delay(1000);

  motorSpeedControl(0, 0, 0, 0);

  delay(1000);
}
void turnSomeDegreesRight() {
  int value;
    
  // Set direction to forward
  motorDirectionControl(LOW, HIGH, LOW, HIGH);
  
  // Provide some momentum for turning
  for(value = 0 ; value <= 255; value+=5)
  {
    motorSpeedControl(value, value, value, value);
    delay(30);
  }

  // Turn off left motors to change their direction
  motorSpeedControl(0, 255, 0, 255);
  delay(500);

  // Change left motor directions & accelerate maximum
  motorDirectionControl(HIGH, LOW, HIGH, LOW);

  motorSpeedControl(255, 255, 255, 255);

  delay(1000);

  // Stop the motors
  motorSpeedControl(0, 0, 0, 0);

  delay(1000);

  // Set direction to backwards
  motorDirectionControl(LOW, HIGH, LOW, HIGH);

  // Provide some momentum for turning
  for(value = 0 ; value <= 255; value+=5)
  {
    motorSpeedControl(value, value, value, value);
    delay(30);
  }

  // Turn off right motors to change their direction
  motorSpeedControl(255, 0, 255, 0);
  delay(500);

  // Change right motor directions & accelerate maximum
  motorDirectionControl(HIGH, HIGH, HIGH, HIGH);
  motorSpeedControl(255, 255, 255, 255);

  delay(1000);

  motorSpeedControl(0, 0, 0, 0);

  delay(1000);
}

void setupServos() {
  x_axis.setPeriodHertz(300);
	y_axis.setPeriodHertz(300);
	x_axis.attach(x_axis_pin);
	y_axis.attach(y_axis_pin);

	x_axis.write(pos_x);
	y_axis.write(pos_y);
}
void setupMotors() {
  pinMode(motor_bck_l_m, OUTPUT);
  pinMode(motor_bck_r_m, OUTPUT);
  pinMode(motor_frnt_l_m, OUTPUT);
  pinMode(motor_frnt_r_m, OUTPUT);
  // speed at zero for all motors
  analogWrite(motor_bck_l_en, 0);
  analogWrite(motor_bck_r_en, 0);
  analogWrite(motor_frnt_l_en, 0);
  analogWrite(motor_frnt_r_en, 0);
  // forward direction
  digitalWrite(motor_bck_l_m, HIGH);
  digitalWrite(motor_bck_r_m, LOW);
  digitalWrite(motor_frnt_l_m, HIGH);
  digitalWrite(motor_frnt_r_m, LOW);
}
bool setupPowerMeter() {
  if (!ina260.begin()) {
    return false;
  }
  return true;
}
int batteryVoltage() {
  int reading = analogRead(batteryRead);
  int batteryPrcntg = map(reading, 3100, 2400, 100, 0);
  return batteryPrcntg;
}
bool setupGPS() {
  if (!gps.encode(Serial.read())) {
    return false;
  }
  return true;
}

bool setupHCSR() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  double duration = pulseIn(echoPin, HIGH);
  int distance = (duration * 0.0343) / 2;
  Serial.print("Distance: ");
  Serial.println(distance);
  
  if(distance > 400 || distance < 0) {
    return false;
  }
  return true;
}

bool checkPV() {
  int ph1 = analogRead(photo_rez_bottom_right);
  int ph2 = analogRead(photo_rez_top_right);
  int ph3 = analogRead(photo_rez_bottom_left);
  int ph4 = analogRead(photo_rez_top_left);

  if(ph1 > 3800 || ph1 < 100) {
    return false;
  }
  if(ph2 > 3800 || ph1 < 100) {
    return false;
  }
  if(ph3 > 3800 || ph1 < 100) {
    return false;
  }
  if(ph4 > 3800 || ph1 < 100) {
    return false;
  }
  return true;
}

bool platformCheckup() {
  bool readyGPS = setupGPS();
  bool photoresistorCheck = checkPV();
  int currentBatteryPrcntg = batteryVoltage();
  bool foundPowerMeter = setupPowerMeter();

  if (currentBatteryPrcntg < 50 || photoresistorCheck == false || foundPowerMeter == false || readyGPS == false) {
    client.publish("esp32/error", "{\"robotId\": \"ROBOT0001\", \"error\": \"startupError\"}");
    return false;
  }
  return true;
}
// void getGPSData() {
//   if (gps.location.isUpdated()) {
//     latitude = gps.location.lat();
//     longitude = gps.location.lng();
//   }
// }


String getExistingStoredData() {
  int length = EEPROM.read(3);
  
  // Check if there's already data stored
  if (length != 0xFF) {
    String data = EEPROM.read(3);
    return data;
  }
  return "none";
}

bool gatherAndSendToServer(int ph1, int ph2, int ph3, int ph4) {
  int current = ina260.readCurrent();
  int voltage = ina260.readBusVoltage();
  int power = ina260.readPower();
  double latitude = gps.location.lat();
  double longitude = gps.location.lng();

    // Check if values are valid
  if (!gps.location.isValid()) {
    Serial.println("Error: GPS data is not valid.");
    return false;
  }
  
  if (current == 0 || voltage == 0 || power == 0) {
    Serial.println("Error: Sensor data is not valid.");
    return false;
  }

  // Create a JSON object to hold the data
  StaticJsonDocument<200> doc;
  doc["ph1"] = ph1;
  doc["ph2"] = ph2;
  doc["ph3"] = ph3;
  doc["ph4"] = ph4;
  doc["current"] = current;
  doc["voltage"] = voltage;
  doc["power"] = power;
  doc["latitude"] = latitude;
  doc["longitude"] = longitude;

  String existingData = getExistingStoredData();
  if (existingData != "none") {
    doc["previous"] = existingData;
  }

  bool sent;
  for (int i = 0; i < 5; i++) {

    if(client.publish("esp32/data", "{\"robotId\": \"ROBOT0001\", \"data\": " + doc + "\"}")) {
      sent = true;
      break;
    } else {
      sent = false;
    }
  }
  if (sent == false) {
    String jsonStr = JSON.stringify(doc);
    EEPROM.write(3, jsonStr);
    EEPROM.commit();
  }
  return sent;
}






















