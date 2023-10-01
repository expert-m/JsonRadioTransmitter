# Arduino JSON Radio Transmitter

The Arduino JSON Radio Transmitter is a library that provides a simple and efficient way to transmit and receive JSON messages over a wireless connection using the NRF24L01 radio module. It's designed to be easy to use and flexible, allowing you to focus on your application logic.

## Features

- Easy-to-use API for sending and receiving JSON messages.
- Uses the ArduinoJson library for efficient JSON handling.
- Supports message fragmentation and reassembly.
- Configurable message size and transmission delay.
- Debug mode for easier troubleshooting.
- Power management functions (power up/down).

## Installation

1. Download the library as a ZIP file from the GitHub repository.
2. In the Arduino IDE, go to `Sketch` > `Include Library` > `Add .ZIP Library...`.
3. Select the downloaded ZIP file and click `Open`.

## Usage

First, include the necessary header files in your Arduino sketch:

```cpp
#include <ArduinoJson.h>
#include <RF24.h>
#include "JsonRadioTransmitter.hh"
```

Next, create an instance of the `RF24` and `RadioTransmitter` classes, passing the CE and CSN pins of your NRF24L01 module to the `RF24` constructor:

```cpp
#define CE_PIN 9
#define CSN_PIN 10

RF24 radio(CE_PIN, CSN_PIN);
RadioTransmitter transmitter(radio, "12345", 90); // 12345 is the radio address, 90 is the radio channel
```

In your `setup()` function, initialize the radio transmitter:

```cpp
void setup() {
  transmitter.init();
}
```

To send a JSON message, create a `StaticJsonDocument`, populate it with data, and pass it to the `send()` method of the `RadioTransmitter`:

```cpp
void loop() {
  StaticJsonDocument<JSON_RADIO_MSG_SIZE> jsonDoc;
  jsonDoc["temperature"] = 25.6;
  jsonDoc["humidity"] = 65.3;
  
  if(transmitter.send(jsonDoc)) {
    Serial.println("Message sent successfully");
  } else {
    Serial.println("Message sending failed");
  }
  
  delay(1000);
}
```

To receive a JSON message, create a `StaticJsonDocument` and pass it to the `read()` method of the `RadioTransmitter`. If the method returns `true`, the document contains the received JSON data:

```cpp
void loop() {
  StaticJsonDocument<JSON_RADIO_MSG_SIZE> jsonDoc;
  
  if(transmitter.read(jsonDoc)) {
    Serial.print("Temperature: ");
    Serial.println(jsonDoc["temperature"].as<float>());
    Serial.print("Humidity: ");
    Serial.println(jsonDoc["humidity"].as<float>());
  }
  
  delay(1000);
}
```

## Configuration

You can configure the library by editing the `JsonRadioTransmitter.hh` file. This file allows you to set the radio channel, address, data rate, and other parameters.

## License

This library is released under the MIT License. See the `LICENSE` file for more information.
