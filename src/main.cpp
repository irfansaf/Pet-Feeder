#include <Arduino.h>
#include <RtcDS1302.h>
#include <WiFiManager.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <PubSubClient.h>

// Stepper Motor Configuration
#define IN1 D8
#define IN2 D7
#define IN3 D6
#define IN4 D5

// UltraSonic Configuration
#define TRIG D0
#define ECHO D1

// Steps to rotate the stepper motor for one full rotation
#define DISPENSE_STEPS 515

// Variable to track if the object is detected
bool objectDetected = false;

// Dispensing level (1 to 10, where 1 is 10 grams, 2 is 20 grams, etc.)
int dispensingLevel = 2;


// NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 7 * 3600);

// WiFi and MQTT configuration
const char *mqttBroker = "52.74.155.78";
const int mqttPort = 1883;
WiFiClient espClient;
PubSubClient client(espClient);

// Forward declarations of functions
void callback(char *topic, byte *payload, unsigned int length);
void reconnect();
void openFeeder(int times);
void stepperMotor(int numberOfSteps);
void setDispensingLevel(int level);
void printDateTime(const RtcDateTime &dt);
bool isLeapYear(uint16_t year);

void printCurrentTime()
{
  timeClient.update();
  Serial.print("Current time: ");
  Serial.println(timeClient.getFormattedTime());
}

String getDeviceId()
{
  // Get the MAC address of the WiFi module
  uint8_t mac[6];
  WiFi.macAddress(mac);

  // Convert the MAC address to a String
  String macStr = "";
  for (int i = 0; i < 6; i++)
  {
    if (mac[i] < 16)
    {
      macStr += "0";
    }
    macStr += String(mac[i], HEX);
    if (i < 5)
    {
      macStr += ":";
    }
  }

  return "AukeyPet-" + macStr;
}

void setup()
{
  // Initialize the serial communication for debugging
  Serial.begin(115200);

  // Initialize the pins for the stepper motor
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  // Initialize the pins for the ultrasonic sensor
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);

  // Initialize WiFi and connect to MQTT broker
  WiFiManager wifiManager;
  wifiManager.autoConnect("SAF_AP");

  client.setServer(mqttBroker, mqttPort);
  client.setCallback(callback);

  // Initialize NTP Client
  timeClient.begin();
  while (!timeClient.update())
  {
    timeClient.forceUpdate();
  }

  // Initialize RTC
  timeClient.update();

  // Set the initial dispensing level (can be set by the user)
  setDispensingLevel(1);
}

void loop()
{
  if (!client.connected())
  {
    reconnect();
  }

  client.loop();

  // Get the current time
  printCurrentTime();

  // Variable to store the distance
  long duration, distance;

  // Clear the TRIG pin
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);

  // Set the TRIG pin high for 10 microseconds
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);

  // Read the ECHO pin
  duration = pulseIn(ECHO, HIGH);

  // Calculate the distance
  distance = (duration / 2) / 29.1;

  // Print the distance to the serial monitor
  Serial.print("Distance: ");
  Serial.println(distance);

  // Publish the distance to the MQTT broker
  client.publish("pet-feeder/distance", String(distance).c_str());

  // Check if the distance is less than a threshold (e.g., 10 cm)
  if (distance < 10)
  {
    // If the object was not previously detected, open the feeder
    if (!objectDetected)
    {
      openFeeder(dispensingLevel);
    }
    objectDetected = true; // Set object detected flag
  }
  else
  {
    // If the object is not detected, reset the flag
    objectDetected = false;
  }

  // Small delay before the next loop
  delay(200);
}

void handleDispensingLevel(const String &message)
{
  int level = message.toInt();
  setDispensingLevel(level);
}

void handleManualFeed(int times)
{
  openFeeder(times);
}

void callback(char *topic, byte *payload, unsigned int length)
{
  payload[length] = '\0'; // Null-terminate the payload
  String message = String((char *)payload);
  Serial.print("Message received on topic ");
  Serial.print(topic);
  Serial.print(": ");
  Serial.println(message);

  if (String(topic) == "pet-feeder/dispensing-level")
  {
    handleDispensingLevel(message);
  }
  else if (String(topic) == "pet-feeder/manual-feed")
  {
    int feedTimes = message.toInt();
    handleManualFeed(feedTimes);
  }
}

void subscribeToTopics()
{
  client.subscribe("pet-feeder/dispensing-level");
  client.subscribe("pet-feeder/manual-feed");
}

void reconnect()
{
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("SAF_Pet_Feeder_Client"))
    {
      Serial.println("connected");
      subscribeToTopics();
      Serial.println("DeviceID: " + getDeviceId());
    }
    else
    {
      Serial.println("DeviceID: " + getDeviceId());
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void stepperMotor(int numberOfSteps)
{
  // Define the steps for the stepper motor
  int steps[8][4] = {
      {1, 0, 0, 0},
      {1, 1, 0, 0},
      {0, 1, 0, 0},
      {0, 1, 1, 0},
      {0, 0, 1, 0},
      {0, 0, 1, 1},
      {0, 0, 0, 1},
      {1, 0, 0, 1}};

  // Loop through the steps
  for (int i = 0; i < numberOfSteps; i++)
  {
    // Set the index based on the direction
    for (int j = 0; j < 8; j++)
    {
      digitalWrite(IN1, steps[j][0]);
      digitalWrite(IN2, steps[j][1]);
      digitalWrite(IN3, steps[j][2]);
      digitalWrite(IN4, steps[j][3]);
      delay(1);
    }

    // Publish status to MQTT
    client.publish("pet-feeder/status", "Moving...");
  }
}

void openFeeder(int times)
{
  int steps = DISPENSE_STEPS * times;
  stepperMotor(steps);
}

void setDispensingLevel(int level)
{
  // Ensure the level is within the valid range (1 to 10)
  if (level >= 1 && level <= 10)
  {
    dispensingLevel = level;
    Serial.print("Dispensing level set to: ");
    Serial.println(dispensingLevel);
  }
  else
  {
    Serial.println("Invalid level. Please set a level between 1 and 10.");
  }
}

void printDateTime(const RtcDateTime &dt)
{
  char datestring[20];

  snprintf_P(datestring,
             countof(datestring),
             PSTR("%02u/%02u/%04u %02u:%02u:%02u"),
             dt.Month(),
             dt.Day(),
             dt.Year(),
             dt.Hour(),
             dt.Minute(),
             dt.Second());
  Serial.print(datestring);
}

bool isLeapYear(uint16_t year)
{
  return ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0));
}