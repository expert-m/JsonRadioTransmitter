# Arduino JSON Radio Transmitter

The Arduino JSON Radio Transmitter is a library that provides a simple way to transmit and receive JSON messages over a wireless connection using the NRF24L01 radio module. It uses the ArduinoJson library for efficient JSON handling and provides message fragmentation and reassembly, power management functions, and a debug mode for easier troubleshooting.

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

Next, create an instance of the `RF24` and `RadioTransmitter` classes:

```cpp
RF24 radio(9, 10); // CE and CSN pins of your NRF24L01 module
RadioTransmitter<StaticJsonDocument<200>> transmitter(radio, "RADDR", "WADDR", 90); // Replace "RADDR" and "WADDR" with your own 5-character addresses
```

Initialize the radio transmitter in your `setup()` function:

```cpp
void setup() {
  transmitter.init();
}
```

To send a JSON message, create a `StaticJsonDocument`, fill it with data, and call the `write()` method:

```cpp
void loop() {
  StaticJsonDocument<200> doc;
  doc["temperature"] = 25.6;
  doc["humidity"] = 65.3;

  if(transmitter.write(doc)) {
    Serial.println("Message sent successfully");
  } else {
    Serial.println("Message sending failed");
  }

  delay(1000);
}
```

To receive a JSON message, create a `StaticJsonDocument` and call the `read()` method. If the method returns `true`, the document contains the received data:

```cpp
void loop() {
  StaticJsonDocument<200> doc;

  if(transmitter.read(doc)) {
    Serial.print("Temperature: ");
    Serial.println(doc["temperature"].as<float>());
    Serial.print("Humidity: ");
    Serial.println(doc["humidity"].as<float>());
  }

  delay(1000);
}
```

You can also check if there is incoming data available with the `hasInputData()` method, and control the power state of the radio with the `powerUp()` and `powerDown()` methods.

## Configuration

You can configure the library by modifying the public variables in the `RadioTransmitter` class. These variables include `msgSendDelay`, `msgReceiveDelay`, `msgSize`, `readingMaxWaitingTime`, and `debugMode`.

## License

This library is released under the MIT License. See the `LICENSE` file for more information.
