#include <SPI.h>
#include <Ethernet.h>
#include <Adafruit_NeoPixel.h>
#include <avr/power.h>

#define POT_PIN A0
#define PIXEL_PIN 6 //data pin for the leds
#define NUM_LEDS 10 //number of neo pixels
#define PORT 10002 //port number

byte server[] = {192, 168, 3, 173}; //PLEASE CHECK ipconfig of server for the IP address
EthernetClient client;

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, PIXEL_PIN, NEO_GRB + NEO_KHZ800);
byte brightness = 125;

byte mac[] = {
  0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x02
};

String input_string = "";
boolean is_string_complete = false;

uint32_t red = strip.Color(255, 0, 0);
uint32_t orange = strip.Color(255, 127, 0);
uint32_t yellow = strip.Color(255, 255, 0);
uint32_t green = strip.Color(0, 255, 0);
uint32_t blue = strip.Color(0, 0, 255);
uint32_t violet = strip.Color(255, 0, 255);
uint32_t black = strip.Color(0, 0, 0);
uint32_t the_color = green;

boolean is_connected; //flag whether to poll or not
long disconnected_mark_time;
const int POLL_INTERVAL = 10000;

int pot_value, prev_pot_value;
const int POT_SENSITIVITY = 3; //for the pot reading sensitivity
String led_status = "off";
char status_high[] = {'h', 'i', 'g', 'h','\n'};
char status_average[] = {'a','v','e','r','a','g','e','\n'};
char status_low[] = {'l','o','w','\n'};
char status_off[] = {'o','f','f','\n'};

void setup() {
  Serial.begin(9600);

  pinMode(PIXEL_PIN, OUTPUT);
  pinMode(POT_PIN, INPUT);

  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // no point in carrying on, so do nothing forevermore:
    for (;;)
      ;
  }

  delay(1000);
  Serial.println("connecting...");

  // if you get a connection, report back via serial:
  if (client.connect(server, PORT)) {
    Serial.println("connected");
    is_connected = true;
  }
  else {
    // if you didn't get a connection to the server:
    Serial.println("connection failed");
    is_connected = false;
  }

  strip.begin();

  //give visual indication whether connected
  if (is_connected) {
    the_color = green;
  } else {
    the_color = black;
  }
  strip.setBrightness(brightness);
  strip.show();
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, the_color);
    strip.show();
  }
}


void loop() {

  if (is_connected) {
    //read from knob
    pot_value = analogRead(POT_PIN);

    if ( abs(pot_value - prev_pot_value) > POT_SENSITIVITY ) {
      brightness = map(pot_value, 0, 1024, 0, 255);

      for (int i = 0; i < strip.numPixels(); i++) { //it will forget the colour if brightness is 0, so keep reminding
        strip.setPixelColor(i, the_color);
        strip.show();
      }

      strip.setBrightness(brightness);
      strip.show();

      Serial.print("knob brightness: ");
      Serial.println(brightness);

      if (brightness > 125 && brightness <= 255 && !led_status.equals("high") ) {
        led_status = "high";
        client.write(status_high, sizeof(status_high));
      }
      else if (brightness > 65  && brightness <= 125 && !led_status.equals("average") ) {
        led_status = "average";
        client.write(status_average, sizeof(status_average));
      }
      else if (brightness > 0 && brightness <= 65 && !led_status.equals("low") ) {
        led_status = "low";
        client.write(status_low, sizeof(status_low));
      }
      else if (brightness == 0 && !led_status.equals("off") ) {
        led_status = "off";
        client.write(status_off, sizeof(status_off));
      }

      client.write(brightness); //0-255
      client.write('\n');

      prev_pot_value = pot_value;
    }
  }

  //read from server
  if (is_string_complete) {
    if (input_string.length() == 1) {
      byte brightness = byte(input_string[0]);

      strip.setBrightness(brightness);
      strip.show(); //enable the changes

      Serial.print("brightness: ");
      Serial.println(brightness);
      Serial.println();
    } else { //it is a text string
      //Serial.println(input_string);

      //do your NeoPixel stuff here
      if (input_string.equals("red")) the_color = red;
      else if (input_string.equals("orange")) the_color = orange;
      else if (input_string.equals("yellow")) the_color = yellow;
      else if (input_string.equals("green")) the_color = green;
      else if (input_string.equals("blue")) the_color = blue;
      else if (input_string.equals("violet")) the_color = violet;

      Serial.print("color: ");
      Serial.println(input_string);
      Serial.println();

      for (int i = 0; i < strip.numPixels(); i++) {
        strip.setPixelColor(i, the_color);
        strip.show();
      }
    }
    input_string = "";
    is_string_complete = false;
  }

  // if there are incoming bytes available
  // from the server, read them and print them:
  if (client.available()) {
    char inChar = client.read();

    //Serial.print("received: ");
    //Serial.println(inChar);

    if (inChar != '\n') {
      input_string += inChar;
    }
    if (inChar == '\n') {
      is_string_complete = true;
      Serial.println("newline char received");
    }
  }

  // if the server's disconnected, stop the client:
  if (!client.connected() && is_connected) {
    //visual indication
    strip.setBrightness(0);
    strip.show();

    Serial.println();
    Serial.println("disconnecting.");
    client.stop();
    is_connected = false;
    disconnected_mark_time = millis();
    // do nothing:
    //while (true);
  }

  //try to auto-reconnect at every poll interval
  if (!is_connected) {
    if (millis() - disconnected_mark_time > POLL_INTERVAL) {

      Serial.println("trying to re-connect...");

      // if you get a connection, report back via serial:
      if (client.connect(server, PORT)) {
        Serial.println("re-connected");
        is_connected = true;
        //visual indication
        strip.setBrightness(125);
        strip.show();
        for (int i = 0; i < strip.numPixels(); i++) {
          strip.setPixelColor(i, green);
          strip.show();
        }

      }
      else {
        // if you didn't get a connection to the server:
        Serial.println("re-connection failed");
        is_connected = false;
        disconnected_mark_time = millis();
      }
    }
  }
}
