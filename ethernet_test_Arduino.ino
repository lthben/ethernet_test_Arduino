#include <SPI.h>
#include <Ethernet.h>

byte mac[] = {
  0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x02
};

EthernetClient client;
byte server[] = {192, 168, 3, 64};

String input_string = "";
boolean is_string_complete = false;

void setup() {
  Serial.begin(9600);

  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // no point in carrying on, so do nothing forevermore:
    for (;;)
      ;
  }

  delay(1000);
  Serial.println("connecting...");

  // if you get a connection, report back via serial:
  if (client.connect(server, 10002)) {
    Serial.println("connected");
  }
  else {
    // if you didn't get a connection to the server:
    Serial.println("connection failed");
  }
}

void loop() {

  if (is_string_complete) {
    if (input_string.length() == 1) {
      byte brightness = byte(input_string[0]) ;
      //Serial.println(brightness);
    } else { //it is a text string
      Serial.println(input_string);
    }

    //do your NeoPixel stuff here

    input_string = "";
    is_string_complete = false;
  }
  // if there are incoming bytes available
  // from the server, read them and print them:
  if (client.available()) {

    char inChar = client.read();

    if (inChar != '\n') {
      input_string += inChar;
    } else {
      is_string_complete = true;
    }

  }

  // if the server's disconnected, stop the client:
  if (!client.connected()) {
    Serial.println();
    Serial.println("disconnecting.");
    client.stop();
    // do nothing:
    while (true);
  }

}
