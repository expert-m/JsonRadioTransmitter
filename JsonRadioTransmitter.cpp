#include "JsonRadioTransmitter.cpp"

#include <ArduinoJson.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>


template<typename RadioJsonDocument>
RadioTransmitter<RadioJsonDocument>::RadioTransmitter(RF24 &radio, char *addressToRead, char *addressToWrite, unsigned short channel) {
    this->radio = &radio;
    this->addressToRead = addressToRead;
    this->addressToWrite = addressToWrite;
    this->channel = channel;
}

template<typename RadioJsonDocument>
void RadioTransmitter<RadioJsonDocument>::init() {
    radio->begin();
    radio->setChannel(channel);
    radio->setAddressWidth(addressWidth);
    radio->setRetries(msgSendDelay, 3);
    radio->setPayloadSize(blockSize);
    radio->setDataRate(RF24_250KBPS);
    radio->setPALevel(RF24_PA_MIN);
    radio->openWritingPipe((const uint8_t *) addressToWrite);
    radio->openReadingPipe(addressToWrite != addressToRead, (const uint8_t *) addressToRead);
    radio->setAutoAck(true);
    radio->startListening();
    isOn = true;
}

template<typename RadioJsonDocument>
bool RadioTransmitter<RadioJsonDocument>::write(RadioJsonDocument &jsonDocument) {
    radio->stopListening();

    if (!startSending()) {
        delay(msgSendDelay);
        startListening();
        return false;
    }

    delay(msgSendDelay);

    char buffer[msgSize];
    serializeJson(jsonDocument, buffer, msgSize);

    for (unsigned int i = 0; i < msgSize; i += blockSize) {
        bool isLastBlock = false;

        for (unsigned int j = i; j < i + blockSize; ++j) {
            if (buffer[j] == '\0') {
                isLastBlock = true;
                break;
            }
        }

        if (!writeBlock(&buffer[i], blockSize)) {
            return false;
        }

        delay(msgSendDelay);

        if (isLastBlock) {
            if (debugMode) {
                Serial.println("It was the last block.");
            }

            break;
        }
    }

    delay(msgSendDelay);

    const bool isFinished = finishSending();

    delay(msgSendDelay);
    startListening();

    return isFinished;
}

template<typename RadioJsonDocument>
bool RadioTransmitter<RadioJsonDocument>::read(RadioJsonDocument &jsonDocument) {
    if (debugMode) {
        Serial.println("Start reading...");
    }

    char buffer[msgSize];
    memset(buffer, '\0', msgSize);

    char blockBuffer[blockSize];
    memset(blockBuffer, '\0', blockSize);

    radio->read(&blockBuffer, blockSize);

    if (strcmp(blockBuffer, startBytes) != 0) {
        if (debugMode) {
            Serial.print("\"");
            Serial.print(blockBuffer);
            Serial.println("\" - it is not started bytes.");
        }

        return false;
    }

    unsigned long startTime = millis();
    unsigned int shift = 0;
    bool hasData = false;

    while (millis() - startTime < readingMaxWaitingTime) {
        delay(msgReceiveDelay);

        if (!radio->available()) {
            continue;
        }

        memset(blockBuffer, '\0', blockSize);
        radio->read(&blockBuffer, blockSize);

        if (debugMode) {
            Serial.print("Read block: ");
            Serial.println(blockBuffer);
        }

        if (strcmp(blockBuffer, startBytes) == 0) {
            if (debugMode) {
                Serial.println("Got started bytes.");
            }

            memset(buffer, '\0', msgSize);

            shift = 0;
            startTime = millis();
            continue;
        }

        if (strcmp(blockBuffer, endBytes) == 0) {
            if (debugMode) {
                Serial.println("Got end bytes.");
            }

            if (hasData) {
                deserializeJson(jsonDocument, (const char*)buffer, shift);

                if (debugMode) {
                    Serial.print("Raw buffer: ");
                    Serial.println(buffer);
                    Serial.print("Parsed JSON: ");
                    serializeJson(jsonDocument, Serial);
                    Serial.println();
                }

                return true;
            } else {
                if (debugMode) {
                    Serial.println("Invalid data.");
                }

                return false;
            }
        }

        if (debugMode) {
            Serial.println("Saving block...");
        }

        if (shift + blockSize > msgSize) {
            if (debugMode) {
                Serial.println("Got more bytes than expected.");
            }

            return false;
        }

        strncpy(buffer + shift, blockBuffer, blockSize);
        shift += blockSize;
        hasData = true;

        if (debugMode) {
            Serial.print("Shift: ");
            Serial.println(shift);

            Serial.print("Raw buffer: ");
            Serial.println(buffer);
        }

        startTime = millis();
    }

    if (debugMode) {
        Serial.print("Time limit.");
    }

    return false;
}

template<typename RadioJsonDocument>
bool RadioTransmitter<RadioJsonDocument>::hasInputData() {
    return radio->available();
}

template<typename RadioJsonDocument>
void RadioTransmitter<RadioJsonDocument>::powerUp() {
    radio->powerUp();
    radio->startListening();
    isOn = true;
    delay(5);
}

template<typename RadioJsonDocument>
void RadioTransmitter<RadioJsonDocument>::powerDown() {
    radio->powerDown();
    isOn = false;
    delay(5);
}

template<typename RadioJsonDocument>
bool RadioTransmitter<RadioJsonDocument>::startSending() {
    if (debugMode) {
        Serial.println("Start sending...");
    }

    return writeBlock(static_cast<const void*>(startBytes), sizeof(startBytes));
}

template<typename RadioJsonDocument>
bool RadioTransmitter<RadioJsonDocument>::finishSending() {
    if (debugMode) {
        Serial.println("Finish sending...");
    }

    return writeBlock(static_cast<const void*>(endBytes), sizeof(endBytes));
}

template<typename RadioJsonDocument>
void RadioTransmitter<RadioJsonDocument>::serializeJsonDoc(RadioJsonDocument &jsonDocument, char* buffer) {
    if (debugMode) {
        Serial.println("Serializing JSON...");
    }

    serializeJson(jsonDocument, buffer);

    if (debugMode) {
        Serial.print("Buffer for sending: ");
        Serial.println(buffer);
    }
}

template<typename RadioJsonDocument>
bool RadioTransmitter<RadioJsonDocument>::writeBlock(const void* data, int size) {
    if (debugMode) {
        Serial.println("Writing the data...");
    }

    if (!radio->write(data, size)) {
        if (debugMode) {
            Serial.println("Not delivered.");
        }

        radio->txStandBy();
        return false;
    }

    return true;
}

template<typename RadioJsonDocument>
void RadioTransmitter<RadioJsonDocument>::startListening() {
    radio->startListening();

    if (debugMode) {
        Serial.println("Starting listening...");
    }
}
