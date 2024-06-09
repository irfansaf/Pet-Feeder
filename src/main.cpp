#include <Arduino.h>
#include <ThreeWire.h>
#include <RtcDS1302.h>
#include <WiFiManager.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <PubSubClient.h>

// Stepper Motor Configuration
#define IN1 D4
#define IN2 D3
#define IN3 D2
#define IN4 D1

// UltraSonic Configuration
#define TRIG D7
#define ECHO D6

// Steps to rotate the stepper motor for dispensing 10 grams of food (calibrate as needed)
#define DISPENSE_STEPS 128

// RTC Configuration
ThreeWire myWire(D0, D5, D8); // IO, SCLK, CE
RtcDS1302<ThreeWire> Rtc(myWire);

// Variable to track if the object is detected
bool objectDetected = false;

// Dispensing level (1 to 10, where 1 is 10 grams, 2 is 20 grams, etc.)
int dispensingLevel = 2;

// Feeding schedule structure
struct FeedingTime
{
  int hour;
  int minute;
};

// Array to store feeding times
FeedingTime feedingTimes[10];
int feedingTimesCount = 0;

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
void setRTCFromNTP();
void openFeeder();
void closeFeeder();
void stepperMotor(int numberOfSteps, bool direction);
void setDispensingLevel(int level);
void addFeedingTime(int hour, int minute);
void printDateTime(const RtcDateTime &dt);
bool isLeapYear(uint16_t year);

void setup()
{
  // Initialize the serial communication for debugging
  Serial.begin(9600);

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
  Rtc.Begin();
  setRTCFromNTP();

  // Set the initial dispensing level (can be set by the user)
  setDispensingLevel(1);

  // Set a feeding schedule (example)
  addFeedingTime(8, 0);  // Feed at 8:00 AM
  addFeedingTime(18, 0); // Feed at 6:00 PM
}

void loop()
{
  if (!client.connected())
  {
    reconnect();
  }
  client.loop();

  RtcDateTime now = Rtc.GetDateTime();
  printDateTime(now);
  Serial.println();

  // Check if it's time to feed
  for (int i = 0; i < feedingTimesCount; i++)
  {
    if (now.Hour() == feedingTimes[i].hour && now.Minute() == feedingTimes[i].minute)
    {
      for (int i = 0; i < dispensingLevel; i++)
      {
        openFeeder();
      }
      delay(60000); // Wait for one minute to avoid multiple triggers within the same minute
    }
  }

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

  // Check if the distance is less than a threshold (e.g., 10 cm)
  if (distance < 10)
  {
    // If the object was not previously detected, open the feeder
    if (!objectDetected)
    {
      for (int i = 0; i < dispensingLevel; i++)
      {
        openFeeder();
      }
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

void callback(char *topic, byte *payload, unsigned int length)
{
  payload[length] = '\0'; // Null-terminate the payload
  String message = String((char *)payload);
  Serial.print("Message received on topic ");
  Serial.print(topic);
  Serial.print(": ");
  Serial.println(message);

  if (String(topic) == "pet-feeder/feeding-time")
  {
    int hour = message.substring(0, 2).toInt();
    int minute = message.substring(3, 5).toInt();
    addFeedingTime(hour, minute);
  }
  else if (String(topic) == "pet-feeder/dispensing-level")
  {
    int level = message.toInt();
    setDispensingLevel(level);
  }
  else if (String(topic) == "pet-feeder/manual-feed")
  {
    // Switch case
    switch (message.toInt())
    {
    case 1:
      openFeeder();
      break;
    case 2:
      for (int i = 0; i < 2; i++)
      {
        openFeeder();
      }
      break;
    case 3:
      for (int i = 0; i < 3; i++)
      {
        openFeeder();
      }
      break;
    case 4:
      for (int i = 0; i < 4; i++)
      {
        openFeeder();
      }
      break;
    case 5:
      for (int i = 0; i < 5; i++)
      {
        openFeeder();
      }
      break;
    case 6:
      for (int i = 0; i < 6; i++)
      {
        openFeeder();
      }
      break;

    case 7:
      for (int i = 0; i < 7; i++)
      {
        openFeeder();
      }
      break;

    case 8:
      for (int i = 0; i < 8; i++)
      {
        openFeeder();
      }
      break;

    case 9:
      for (int i = 0; i < 9; i++)
      {
        openFeeder();
      }
      break;

    case 10:
      for (int i = 0; i < 10; i++)
      {
        openFeeder();
      }
      break;

    default:
      break;
    }
  }
}

void reconnect()
{
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("SAF_Pet_Feeder_Client"))
    {
      Serial.println("connected");
      // Once connected, subscribe to the topics
      client.subscribe("pet-feeder/feeding-time");
      client.subscribe("pet-feeder/dispensing-level");
      client.subscribe("pet-feeder/manual-feed");
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void stepperMotor(int numberOfSteps, bool direction)
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
    for (int j = 0; j < 8; j++)
    {
      int index = direction ? j : (7 - j); // Reverse the steps if direction is false
      digitalWrite(IN1, steps[index][0]);
      digitalWrite(IN2, steps[index][1]);
      digitalWrite(IN3, steps[index][2]);
      digitalWrite(IN4, steps[index][3]);
      delay(1); // Adjust delay for speed control
    }
  }
}

void openFeeder()
{
  stepperMotor(DISPENSE_STEPS, true);
  delay(100);    // Short delay to ensure food is dispensed
  closeFeeder(); // Immediately close the feeder
  delay(100);
}

void closeFeeder()
{
  // Rotate stepper motor to close the feeder
  stepperMotor(DISPENSE_STEPS, false);
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

void addFeedingTime(int hour, int minute)
{
  if (feedingTimesCount < 10)
  {
    feedingTimes[feedingTimesCount].hour = hour;
    feedingTimes[feedingTimesCount].minute = minute;
    feedingTimesCount++;
    Serial.print("Feeding time added: ");
    Serial.print(hour);
    Serial.print(":");
    Serial.println(minute);
  }
  else
  {
    Serial.println("Cannot add more feeding times. Maximum reached.");
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

void setRTCFromNTP()
{
  timeClient.update();
  unsigned long epochTime = timeClient.getEpochTime();

  // Convert epoch time to RtcDateTime
  uint16_t year = 1970;
  uint8_t month = 1, day = 1, hour = 0, minute = 0, second = 0;
  unsigned long days = epochTime / 86400L;
  unsigned long seconds = epochTime % 86400L;

  // Calculate current hour, minute and second
  hour = seconds / 3600;
  seconds %= 3600;
  minute = seconds / 60;
  second = seconds % 60;

  // Calculate current year, month and day
  while (days >= 365 + isLeapYear(year))
  {
    days -= 365 + isLeapYear(year);
    year++;
  }
  const uint8_t daysInMonth[] = {31, 28 + isLeapYear(year), 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  for (month = 0; month < 12; month++)
  {
    if (days < daysInMonth[month])
      break;
    days -= daysInMonth[month];
  }
  day += days;

  RtcDateTime ntpTime(year, month + 1, day, hour, minute, second);
  Rtc.SetDateTime(ntpTime);
}

bool isLeapYear(uint16_t year)
{
  return ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0));
}
