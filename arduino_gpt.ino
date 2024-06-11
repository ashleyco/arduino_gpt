#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>
#include <textparser.h>
TextParser parser(", ");  // Delimiter is a comma

// Replace with your network credentials
const char* ssid = "";
const char* password = "";

// Replace with your OpenAI API key
const char* apiKey = "";

String apiUrl = "https://api.openai.com/v1/chat/completions";
String finalPayload = "";

bool initialPrompt = true;
bool gettingResponse = true;

// Define the sensors and output
// Neopixels
#define PIN 13
#define NUMPIXELS 6
int r, g, b = 0;

// for distance sensor
const int trigPin = 16;
const int echoPin = 17;
long duration;
int distance;


HTTPClient http;
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  Serial.begin(9600);   // Initialize Serial
  // Define the IO
  pixels.begin();

  // distance sensor
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // Connect to Wi-Fi network
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println("Connected");
  http.begin(apiUrl);

  // initialise chatGPT
  String prompt = "Make up an way to produce an rgb value based on a distance value. Don't tell me the algorithm. The distance values are between 0 and 1000 and I will send you a distance every few seconds. use green for close distances changing to red for log distances. Send me only the rgb values as 3 numbers separated by commas";
  Serial.print("USER:");
  Serial.println(prompt);
  gettingResponse = true;
  char* answer = chatGptCall(prompt); // send the prompt to chatGpt and get the response

}

void loop() {
  pixels.clear();

  // for testing by typing prompt in
  if (Serial.available() > 0)
  {
    String prompt = Serial.readStringUntil('\n');
    prompt.trim();
    Serial.print("USER:");
    Serial.println(prompt);
    gettingResponse = true;
 //   char* answer = chatGptCall(prompt); // send the prompt to chatGpt and get the response
 //   parser.parseLine(answer, r, g, b);
  }

  // read value from sensor
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  // Set the trigPin HIGH for 10 microseconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  // Read the echoPin, and return the sound wave travel time in microseconds
  duration = pulseIn(echoPin, HIGH);
  // Calculate the distance
  int sensorValue  = duration * 0.034 / 2;

  // print out the value you read:
  gettingResponse = true;
  String prompt = "Distance = " + (String)sensorValue;
  Serial.print("USER:");
  Serial.println(prompt);
  char* answer = chatGptCall(prompt); // send the prompt to chatGpt and get the response
  parser.parseLine(answer, r, g, b);

  for (int i = 0; i < NUMPIXELS; i++) { // For each pixel...
    pixels.setPixelColor(i, pixels.Color(r, g, b));
  }
  pixels.show();

  delay(1000);
}

char* chatGptCall(String payload) {
  static char outputText[256]; // Fixed size char array. Static to preserve content after function returns.
  memset(outputText, 0, sizeof(outputText)); // Clear previous response

  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", "Bearer " + String(apiKey));

  if (initialPrompt) {
    finalPayload = "{\"model\": \"gpt-3.5-turbo\",\"messages\": [{\"role\": \"user\", \"content\": \"" + payload + "\"}]}";
    initialPrompt = false;
  } else {
    finalPayload = finalPayload + ",{\"role\": \"user\", \"content\": \"" + payload + "\"}]}";
  }

  while (gettingResponse) {
    int httpResponseCode = http.POST(finalPayload);

    if (httpResponseCode == 200) {
      String response = http.getString();

      // Parse JSON response
      DynamicJsonDocument jsonDoc(1024);
      deserializeJson(jsonDoc, response);
      const char* tempText = jsonDoc["choices"][0]["message"]["content"];

      // Ensure not to overflow the outputText buffer
      strncpy(outputText, tempText, sizeof(outputText) - 1);

      // Optionally remove newline if present in the response
      char* newlinePos = strchr(outputText, '\n');
      if (newlinePos) *newlinePos = '\0'; // Replace newline with null terminator

      Serial.print("CHATGPT: ");
      Serial.println(outputText);

      String returnResponse = "{\"role\": \"assistant\", \"content\": \"" + String(outputText) + "\"}";
      Serial.println(returnResponse); // AC ADDED
      finalPayload = removeEndOfString(finalPayload);
      finalPayload = finalPayload + "," + returnResponse;
      gettingResponse = false;

      break; // Exit loop once response is processed
    } else {
      // Optional: Handle error or retry logic
    }
    getDelay();
  }

  return outputText; // Return the pointer to the char array
}

String removeEndOfString(String originalString)
{
  int stringLength = originalString.length();
  String newString = originalString.substring(0, stringLength - 2);
  return (newString);
}

void getDelay() {
  unsigned long initialMillis = millis();
  while ((initialMillis + 5000) >= millis()) {
  }
}
