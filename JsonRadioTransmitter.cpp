#include "JsonRadioTransmitter.hh"

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

    char buffer[msgSize];
    serializeJson(jsonDocument, buffer, msgSize);

    unsigned short msgEndByte = 0;
    for (unsigned int i = 0; i < msgSize; ++i) {
        if (buffer[i] == '\0') {
            msgEndByte = i;
            break;
        }
    }

    if (msgEndByte == 0) {
        startListening();
        return false;
    }

    char strMsgId[5];
    sprintf(strMsgId, "%04i", random(10000));

    const unsigned int countOfBlocks = msgEndByte / dataBlockSize + (msgEndByte % dataBlockSize != 0);
    char strCountOfBlocks[3];
    sprintf(strCountOfBlocks, "%02i", countOfBlocks);

    delay(msgSendDelay);

    char blockBuffer[blockSize];
    const char strBlockId[4];
    blockBuffer[1] = strMsgId[0];
    blockBuffer[2] = strMsgId[1];
    blockBuffer[3] = strMsgId[2];
    blockBuffer[4] = strMsgId[3];

    if (debugMode) {
        Serial.println("Start sending.");
    }

    for (unsigned short i = 0; i < countOfBlocks; ++i) {
        if (i == 0) {
            blockBuffer[0] = START_BYTE;
            blockBuffer[5] = strCountOfBlocks[0];
            blockBuffer[6] = strCountOfBlocks[1];
        } else {
            blockBuffer[0] = DATA_BYTE;
            sprintf(strBlockId, "%02i", i);
            blockBuffer[5] = strBlockId[0];
            blockBuffer[6] = strBlockId[1];
        }

        for (unsigned short j = 0; j < dataBlockSize; ++j) {
            blockBuffer[j + (blockSize - dataBlockSize)] = buffer[i * dataBlockSize + j];
        }

        if (!writeBlock(blockBuffer, blockSize)) {
            startListening();
            return false;
        }

        delay(msgSendDelay);
    }

    if (debugMode) {
        Serial.println("Data was sent.");
    }

    delay(msgSendDelay);
    startListening();

    return true;
}

template<typename RadioJsonDocument>
bool RadioTransmitter<RadioJsonDocument>::read(RadioJsonDocument &jsonDocument) {
    if (debugMode) {
        Serial.println("Start reading.");
    }

    char buffer[msgSize];
    char blockBuffer[blockSize];

    char strMsgId[5] = {'\0', '\0', '\0', '\0', '\0'};
    char strCountOfBlocks[3] = {'\0', '\0', '\0'};
    unsigned short countOfBlocks = 0;

    unsigned short expectedBlockId = 0;
    unsigned long lastReceivedDataAt = millis();

    while (millis() - lastReceivedDataAt < readingMaxWaitingTime) {
        if (!radio->available()) {
            delay(msgReceiveDelay);
            continue;
        }

        radio->read(&blockBuffer, blockSize);

        if (debugMode) {
            Serial.print("Read block: `");
            Serial.print(blockBuffer);
            Serial.println("`");
        }

        unsigned short currentBlockId = 0;

        if (blockBuffer[0] == START_BYTE) {
            strMsgId[0] = blockBuffer[1];
            strMsgId[1] = blockBuffer[2];
            strMsgId[2] = blockBuffer[3];
            strMsgId[3] = blockBuffer[4];
            strCountOfBlocks[0] = blockBuffer[5];
            strCountOfBlocks[1] = blockBuffer[6];
            countOfBlocks = atoi(strCountOfBlocks);
            expectedBlockId = 0;
        } else if (blockBuffer[0] == DATA_BYTE) {
            const char strBlockId[3] = {blockBuffer[5], blockBuffer[6], '\0'};
            currentBlockId = atoi(strBlockId);
        } else {
            if (debugMode) {
                Serial.println("Invalid data.");
            }

            continue;
        }

        if (debugMode) {
            Serial.println("Process data.");
        }

        if (blockBuffer[1] != strMsgId[0] || blockBuffer[2] != strMsgId[1] || blockBuffer[3] != strMsgId[2] || blockBuffer[4] != strMsgId[3]) {
            if (debugMode) {
                Serial.println("Skip block with invalid message ID.");
            }

            continue;
        }

        if (blockBuffer[0] == DATA_BYTE) {
            if (expectedBlockId > currentBlockId) {
                Serial.println("Skip processed block.");
            } else if (expectedBlockId < currentBlockId) {
                Serial.println("Invalid block ID");
                return false;
            }
        }

        strncpy(buffer + currentBlockId * dataBlockSize, &blockBuffer[blockSize - dataBlockSize], dataBlockSize);

        if (expectedBlockId == countOfBlocks - 1) {
            Serial.println("Get last block.");

            if (debugMode) {
                Serial.print("Raw buffer: `");
                Serial.print(buffer);
                Serial.println("`");
            }

            deserializeJson(jsonDocument, (const char*)buffer, countOfBlocks * dataBlockSize);

            return true;
        }

        ++expectedBlockId;

        lastReceivedDataAt = millis();
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
void RadioTransmitter<RadioJsonDocument>::serializeJsonDoc(RadioJsonDocument &jsonDocument, char* buffer) {
    if (debugMode) {
        Serial.println("Serialize JSON.");
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
        Serial.println("Write block.");
    }

    if (!radio->write(data, blockSize)) {
        if (debugMode) {
            Serial.println("Not delivered.");
        }

        return false;
    }

    return true;
}

template<typename RadioJsonDocument>
void RadioTransmitter<RadioJsonDocument>::startListening() {
    radio->startListening();

    if (debugMode) {
        Serial.println("Start listening.");
    }
}
