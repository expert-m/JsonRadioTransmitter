#include <ArduinoJson.h>
#include <RF24.h>


#define START_BYTE 'S'
#define DATA_BYTE 'D'


template<typename RadioJsonDocument>
class RadioTransmitter {
public:
    bool debugMode = false;
    bool isOn = false;
    unsigned int msgSendDelay = 50;
    unsigned int msgReceiveDelay = 5;
    unsigned int msgSize = 64;
    unsigned int readingMaxWaitingTime = 500;
    const unsigned int blockSize = 32;
    const unsigned int dataBlockSize = blockSize - 1 - 4 - 2;
    const unsigned short addressWidth = 5;
    char *addressToWrite;
    char *addressToRead;
    unsigned short channel;

    RadioTransmitter(RF24 &radio, char *addressToRead, char *addressToWrite, unsigned short channel);
    void init();
    bool write(RadioJsonDocument &jsonDocument);
    bool read(RadioJsonDocument &jsonDocument);
    bool hasInputData();
    void powerUp();
    void powerDown();

private:
    RF24 *radio;

    void serializeJsonDoc(RadioJsonDocument &jsonDocument, char* buffer);
    bool writeBlock(const void* data, int size);
    void startListening();
};
