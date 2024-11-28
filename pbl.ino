#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

// Wi-Fi credentials
const char* ssid = "iBUS@MUJ";  // Replace with your Wi-Fi SSID
const char* password = "";      // No password for your Wi-Fi

// Telegram bot credentials
const String TELEGRAM_TOKEN = "7980908593:AAHA3ggDKIfFaV8V9r3wkQs6V9nCzVpkk9c";  // Replace with your bot token
const String CHAT_ID = "1562163286";  // Replace with your chat ID

// IR Sensor Pin (Mailbox open/close detection)
int irSensorPin = D1;  // GPIO pin connected to IR sensor
int irSensorState = 0;  // Variable to hold sensor state

// HC-SR04 Ultrasonic Sensor Pins (Distance measurement)
int trigPin = D2;  // Trig pin connected to D2
int echoPin = D5;  // Echo pin connected to D5

// Variables for ultrasonic sensor and mail detection
long duration, distance;
bool mailDetected = false;
unsigned long mailDetectedTime = 0;  // Stores the time when mail was first detected
const unsigned long oneDay = 86400000;  // One day in milliseconds

WiFiClient wifiClient;  // Required for HTTPClient

void setup() {
  Serial.begin(115200);

  // Initialize sensor pins
  pinMode(irSensorPin, INPUT);  // IR sensor as input
  pinMode(trigPin, OUTPUT);     // HC-SR04 trig pin as output
  pinMode(echoPin, INPUT);      // HC-SR04 echo pin as input

  // Connect to Wi-Fi
  Serial.print("Connecting to Wi-Fi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to Wi-Fi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  // Check IR sensor state for mailbox door activity
  irSensorState = digitalRead(irSensorPin); 
  if (irSensorState == LOW) {  // Triggered when LOW (door opened)
    Serial.println("Mailbox door opened (IR sensor)!");
    sendTelegramMessage("Mailbox door opened!");
    mailDetected = false;  // Reset mail detection when the door is opened
    delay(1000);  // Prevent multiple triggers
  }

  // Measure distance using ultrasonic sensor to check if mail is present
  distance = measureDistance();

  // Check if mail is detected and set the timer
  if (distance < 15) {  // Assuming 15 cm is the threshold for mail detection
    if (!mailDetected) {
      sendTelegramMessage("Mail detected inside the mailbox!");
      mailDetected = true;
      mailDetectedTime = millis();  // Record the time when mail was detected
    }
  } else {
    mailDetected = false;  // Reset when mail is removed
  }

  // Check if mail has been in the mailbox for over a day without being collected
  if (mailDetected && (millis() - mailDetectedTime >= oneDay)) {
    sendTelegramMessage("Reminder: Mail has not been collected for 24 hours.");
    mailDetectedTime = millis();  // Reset timer for daily reminders if needed
  }

  delay(2000);  // Delay before the next loop iteration
}

long measureDistance() {
  // Send a pulse to trigger the ultrasonic sensor
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  // Measure the pulse duration from the Echo pin
  duration = pulseIn(echoPin, HIGH);

  // Calculate distance in cm
  distance = duration * 0.0344 / 2;

  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  return distance;
}

void sendTelegramMessage(String message) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = "https://api.telegram.org/bot" + TELEGRAM_TOKEN + "/sendMessage?chat_id=" + CHAT_ID + "&text=" + message;

    Serial.print("Sending message: ");
    Serial.println(url);  // Print the full URL to check for issues

    http.begin(wifiClient, url);  // Pass WiFiClient and URL for the HTTP request
    int httpResponseCode = http.GET();
    
    Serial.print("HTTP Response Code: ");
    Serial.println(httpResponseCode);  // Print HTTP response code for debugging

    if (httpResponseCode > 0) {
      Serial.print("Message sent, response code: ");
      Serial.println(httpResponseCode);
    } else {
      Serial.print("Error sending message: ");
      Serial.println(httpResponseCode);  // Print error code for troubleshooting
    }
    http.end();
  } else {
    Serial.println("WiFi not connected");
  }
}
