/*********
  LEDEdit PRO
  How to Install ESP32 Boards in Arduino IDE 2.0
*********/

#include <Arduino.h>
#include "AiEsp32RotaryEncoder.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>




/*
connecting Rotary encoder

Rotary encoder side    MICROCONTROLLER side  
-------------------    ---------------------------------------------------------------------
CLK (A pin)            any microcontroler intput pin with interrupt -> in this example pin 32
DT (B pin)             any microcontroler intput pin with interrupt -> in this example pin 21
SW (button pin)        any microcontroler intput pin with interrupt -> in this example pin 25
GND - to microcontroler GND
VCC                    microcontroler VCC (then set ROTARY_ENCODER_VCC_PIN -1) 

***OR in case VCC pin is not free you can cheat and connect:***
VCC                    any microcontroler output pin - but set also ROTARY_ENCODER_VCC_PIN 25 
                        in this example pin 25

*/

#define ROTARY_ENCODER_A_PIN 32
#define ROTARY_ENCODER_B_PIN 35
#define ROTARY_ENCODER_BUTTON_PIN 33
#define ROTARY_ENCODER_VCC_PIN -1 /* 27 put -1 of Rotary encoder Vcc is connected directly to 3,3V; else you can use declared output pin for powering rotary encoder */
#define LED_PIN    25
#define LED_COUNT 1
//depending on your encoder - try 1,2 or 4 to get expected behaviour
//#define ROTARY_ENCODER_STEPS 1
//#define ROTARY_ENCODER_STEPS 2
#define ROTARY_ENCODER_STEPS 4

// Replace the next variables with your SSID/Password combination
const char* ssid = "";
const char* password = "";

// Add your MQTT Broker IP address, example:
//const char* mqtt_server = "192.168.1.144";
const char* mqtt_server = "edifier530d";

WiFiClient espClient;
PubSubClient client(espClient);
JsonDocument doc;
Adafruit_NeoPixel state_led(LED_COUNT, LED_PIN, NEO_RGBW + NEO_KHZ800);

long prev_volume = 0;
bool isMuted = false;
bool isRotaryInit = false;
long last_change = 0;
long last_state = 0;
//instead of changing here, rather change numbers above
AiEsp32RotaryEncoder rotaryEncoder = AiEsp32RotaryEncoder(ROTARY_ENCODER_A_PIN, ROTARY_ENCODER_B_PIN, ROTARY_ENCODER_BUTTON_PIN, ROTARY_ENCODER_VCC_PIN, ROTARY_ENCODER_STEPS);


//********** button handling
//********** button handling
//********** button handling
//********** button handling
//********** button handling
/*
	Note: try changing shortPressAfterMiliseconds and longPressAfterMiliseconds to fit your needs
	In case you dont need long press set longPressAfterMiliseconds=999999;  that should be enough.

	Then change what code soes on void on_button_short_click() and void on_button_long_click()
	to fit your needs

	To remove writing "+"  when button is down remove lines marked with //REMOVE THIS LINE IF YOU DONT WANT TO SEE

	Use the similar logic to implement double click or very long button press.

	There is a bit of code. But it is non-blocking.
	Try moving the rotary encoder while the button is down. You will see that it works.

	rotary_loop() is actually calling 	handle_rotary_button();
	If you prefer you can move that logic somewhere else but dont forget to call both methods frequently.
	So it is important that you have nonblocking code.

	button functions
		on_button_short_click - change function body to fit your needs
		on_button_long_click - change function body to fit your needs
		handle_rotary_button() - it already ahs logic for short and long press, but you can add double click or extra long press...
			if no need than leave it as it is (and remove lines marked with REMOVE THIS LINE IF YOU DONT WANT TO SEE)

	in case your button is reversed you can uncomment line looking like this (but not here -> do it in a handle_rotary_button):
		isEncoderButtonDown = !isEncoderButtonDown; 
*/

//paramaters for button
unsigned long shortPressAfterMiliseconds = 50;   //how long short press shoud be. Do not set too low to avoid bouncing (false press events).
unsigned long longPressAfterMiliseconds = 1000;  //how long čong press shoud be.


void on_button_short_click() {
  Serial.print("button SHORT press ");
  Serial.print(millis());
  Serial.println(" milliseconds after restart");
  mute();
}

void on_button_long_click() {
  Serial.print("button LONG press ");
  Serial.print(millis());
  Serial.println(" milliseconds after restart");
  char payload[16];
  if(last_state == 1){
    snprintf(payload, sizeof(payload), "%u", 0);
  } else {
    snprintf(payload, sizeof(payload), "%u", 1);
  }
  client.publish("/state", payload);
}

void handle_rotary_button() {
  static unsigned long lastTimeButtonDown = 0;
  static bool wasButtonDown = false;

  bool isEncoderButtonDown = rotaryEncoder.isEncoderButtonDown();
  //isEncoderButtonDown = !isEncoderButtonDown; //uncomment this line if your button is reversed

  if (isEncoderButtonDown) {
    Serial.print("+");  //REMOVE THIS LINE IF YOU DONT WANT TO SEE
    if (!wasButtonDown) {
      //start measuring
      lastTimeButtonDown = millis();
    }
    //else we wait since button is still down
    wasButtonDown = true;
    return;
  }

  //button is up
  if (wasButtonDown) {
    Serial.println("");  //REMOVE THIS LINE IF YOU DONT WANT TO SEE
    //click happened, lets see if it was short click, long click or just too short
    if (millis() - lastTimeButtonDown >= longPressAfterMiliseconds) {
      on_button_long_click();
    } else if (millis() - lastTimeButtonDown >= shortPressAfterMiliseconds) {
      on_button_short_click();
    }
  }
  wasButtonDown = false;
}
//********** button handling ----
//********** button handling ----
//********** button handling ----
//********** button handling ----

void rotary_loop() {
  //dont print anything unless value changed
  if (rotaryEncoder.encoderChanged()) {
    Serial.print("Setting volume to: ");
    Serial.println(rotaryEncoder.readEncoder());
    char payload[16];
    snprintf(payload, sizeof(payload), "%u", rotaryEncoder.readEncoder());
    client.publish("/volume", payload);
    last_change = millis();
    state_led.setPixelColor(0, rotaryEncoder.readEncoder() * 2, 0, 0, 0);
    state_led.show();
  }
  handle_rotary_button();
}

void IRAM_ATTR readEncoderISR() {
  rotaryEncoder.readEncoder_ISR();
}

void setup() {

  Serial.begin(115200);
  state_led.begin();           
  state_led.show();            
  state_led.setBrightness(100);
  //we must initialize rotary encoder
  rotaryEncoder.begin();
  rotaryEncoder.setup(readEncoderISR);
  //set boundaries and if values should cycle or not
  //in this example we will set possible values between 0 and 1000;
  bool circleValues = false;
  rotaryEncoder.setBoundaries(1, 56, circleValues);  //minValue, maxValue, circleValues true|false (when max go to min and vice versa)
  state_led.setPixelColor(0, 65,105,225, 0);
  state_led.show();
  /*Rotary acceleration introduced 25.2.2021.
   * in case range to select is huge, for example - select a value between 0 and 1000 and we want 785
   * without accelerateion you need long time to get to that number
   * Using acceleration, faster you turn, faster will the value raise.
   * For fine tuning slow down.
   */
  //rotaryEncoder.disableAcceleration(); //acceleration is now enabled by default - disable if you dont need it
  rotaryEncoder.setAcceleration(0);  //or set the value - larger number = more accelearation; 0 or 1 means disabled acceleration

  Serial.begin(115200);
  // default settings
  // (you can also pass in a Wire library object like &Wire2)
  //status = bme.begin();  

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {

  // Loop until we're reconnected
  while (!client.connected()) {
    state_led.setPixelColor(0, 255,0,0, 0);
    state_led.show();
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
      // Subscribe
      client.subscribe("/settings");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
  client.publish("/get", "0");

  delay(50);
  state_led.setPixelColor(0, 0,0,0, 0);
  state_led.show();
}

void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  // Feel free to add more if statements to control more GPIOs with MQTT

  // If a message is received on the topic esp32/output, you check if the message is either "on" or "off". 
  // Changes the output state according to the message
  if (String(topic) == "/settings") {
    deserializeJson(doc, message);
    long volume = doc["volume"];
    long state = doc["state"];
    last_state = state;
    if (state == 0){
      state_led.setPixelColor(0, 0, 100, 0, 0);
      state_led.show();
    } else if ((last_change + 1000 < millis() && !isMuted) || !isRotaryInit) {
      state_led.setPixelColor(0, volume * 2, 0, 0, 0);
      state_led.show();
    }
    if ((rotaryEncoder.readEncoder() != volume && last_change + 1000 < millis()) || !isRotaryInit){
      Serial.print("Rotary changed to ");
      Serial.println(volume);
      rotaryEncoder.setEncoderValue(volume);
      isRotaryInit = true;
    }
  }
}

void mute(){
  if(!isMuted){
    prev_volume = rotaryEncoder.readEncoder();
    isMuted = true;
    client.publish("/volume", "1");
    Serial.println("Muted");
    state_led.setPixelColor(0, 100, 100, 0, 0);
    state_led.show();
  } 
  else if (prev_volume != 0 and isMuted) {
    rotaryEncoder.setEncoderValue(prev_volume);
    char payload[16];
    snprintf(payload, sizeof(payload), "%u", rotaryEncoder.readEncoder());
    client.publish("/volume",payload);
    isMuted = false;
    Serial.println("Unmuted");
    Serial.print("Volume set to: ");
    Serial.println(payload);
    state_led.setPixelColor(0, prev_volume * 2, 0, 0, 0);
    state_led.show();
  }

}

void loop() {
    rotary_loop();

    if (!client.connected()) {
    reconnect();
  }
  client.loop();
  //in loop call your custom function which will process rotary encoder values
  delay(50);  //or do whatever you need to do...
}