#include "BleManager.h"
#include "BleConfig.h"
#include "Ws2811.h"
#include "esp_crc.h"

extern "C" {
  #include "esp_rom_crc.h"
}

/* ===============================
SERVICE_UUID
├── textRxChar   (iOS → ESP32)   WRITE / WRITE_NR   ← UTF-8 commands
├── dataRxChar   (iOS → ESP32)   WRITE              ← binary upload
├── textTxChar   (ESP32 → iOS)   NOTIFY / READ      ← status strings
├── dataTxChar   (ESP32 → iOS)   NOTIFY              ← binary packets
├── idChar       (ESP32 → iOS)   READ                ← device ID


   Globals (BLE owns these)
   =============================== 

   CRC Stuff */
/*struct __attribute__((packed)) BinaryHeader {
  uint16_t magic;
  uint8_t  version;
  uint32_t payloadLength;
  uint32_t crc32;
};
enum RxState {
  RX_HEADER,
  RX_PAYLOAD,
  RX_DONE
};*/
enum RxState {
  RX_IDLE,
  RX_HEADER,
  RX_PAYLOAD
};

RxState rxState = RX_IDLE;

static constexpr uint8_t  MAGIC[2] = {0xA5, 0xA5};
static constexpr uint8_t  PROTOCOL_VERSION = 1;
static constexpr size_t   HEADER_SIZE = 11;
static constexpr size_t   MAX_PAYLOAD = 4096;

uint8_t  headerBuf[HEADER_SIZE];
size_t   headerBytes = 0;

uint8_t  rxBuffer[MAX_PAYLOAD];
size_t   bytesReceived = 0;
uint32_t expectedBytes = 0;
uint32_t expectedCRC   = 0;

uint32_t crc32_esp(const uint8_t* data, size_t len) {
  return esp_rom_crc32_le(0, data, len);
}


#define MAGIC_0 0xA5
#define MAGIC_1 0xA5
#define HEADER_SIZE 11

//uint32_t expectedPayloadBytes = 0;


size_t headerBytesReceived = 0;
uint8_t headerBuffer[HEADER_SIZE];

bool receivingHeader = false;


//end crc stuff
bool deviceConnected = false;

BLECharacteristic *textRxChar;
BLECharacteristic *dataRxChar;
BLECharacteristic *textTxChar;
BLECharacteristic *dataTxChar;
BLECharacteristic *idChar;

#define STRUCTS_PER_PACKET 10
#define TOTAL_PACKETS ((NUM_LEDS + STRUCTS_PER_PACKET - 1) / STRUCTS_PER_PACKET)

bool receivingBinary = false;
size_t loadedCount;

constexpr size_t TOTAL_STRUCTS = NUM_LEDS;
constexpr size_t STRUCT_SIZE = sizeof(LedsInfo);
constexpr size_t EXPECTED_BYTES = NUM_LEDS * STRUCT_SIZE;

static_assert(sizeof(LedsInfo) == 18, "Size mismatch");

/* ==============================
   Test Timer
   =============================*/
   bool testActive = false;
uint32_t testStartMillis = 0;
const uint32_t TEST_DURATION_MS = 5 * 1000;

struct StoredLed {
  uint32_t candleBlue;
  int32_t  candleRed;
  int32_t  candleGreen;
};


StoredLed storedLED;

/* ===============================
   Callbacks
   =============================== */

class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer*) override {
    deviceConnected = true;
    Serial.println("iOS connected");
  }
  void onDisconnect(BLEServer*) override {
    deviceConnected = false;
    Serial.println("iOS disconnected");
    BLEDevice::startAdvertising();
  }
};

class DataCallbacks : public BLECharacteristicCallbacks {

  void onWrite(BLECharacteristic* pChar) override {

    String value = pChar->getValue();
    const uint8_t* data = (const uint8_t*)value.c_str();
    size_t len = value.length();

    if (len == 0) return;

    while (len > 0) {

      // ===== HEADER =====
      if (rxState == RX_IDLE || rxState == RX_HEADER) {

        rxState = RX_HEADER;

        size_t needed = HEADER_SIZE - headerBytes;
        size_t copyLen = min(needed, len);

        memcpy(headerBuf + headerBytes, data, copyLen);

        headerBytes += copyLen;
        data += copyLen;
        len  -= copyLen;

        if (headerBytes < HEADER_SIZE) return;

        if (headerBuf[0] != 0xA5 || headerBuf[1] != 0xA5) {
          Serial.println("❌ Bad magic");
          reset();
          return;
        }

        memcpy(&expectedBytes, headerBuf + 3, 4);
        memcpy(&expectedCRC,   headerBuf + 7, 4);

        if (expectedBytes == 0 || expectedBytes > MAX_PAYLOAD) {
          Serial.println("❌ Invalid size");
          reset();
          return;
        }

        Serial.print("🟢 Header OK, expecting ");
        Serial.println(expectedBytes);

        rxState = RX_PAYLOAD;
        bytesReceived = 0;
      }

      // ===== PAYLOAD =====
      if (rxState == RX_PAYLOAD) {

        size_t remaining = expectedBytes - bytesReceived;
        size_t copyLen = min(remaining, len);

        memcpy(rxBuffer + bytesReceived, data, copyLen);

        bytesReceived += copyLen;
        data += copyLen;
        len  -= copyLen;

        Serial.print("📦 bytesReceived ");
        Serial.println(bytesReceived);

        if (bytesReceived < expectedBytes) return;

        uint32_t crc = esp_rom_crc32_le(0, rxBuffer, expectedBytes);

        if (crc != expectedCRC) {
          Serial.println("❌ CRC mismatch");
          reset();
          return;
        }
//printArray();
        Serial.println("✅ Binary transfer from IOS complete");
        LedsInfo* infos = (LedsInfo*)rxBuffer;

        Serial.print("Applying ");
Serial.print(expectedBytes);
Serial.println(" bytes to leds_Info");


        memcpy(leds_Info, infos, expectedBytes);
        saveLedsInfoArray(leds_Info, NUM_LEDS);
        Serial.println("After memcpy:");

printArray();

        reset();
        return;
      }
    }
  }

private:
  void reset() {
    rxState = RX_IDLE;
    headerBytes = 0;
    bytesReceived = 0;
    expectedBytes = 0;
    expectedCRC = 0;
  }
};

class TextCallbacks : public BLECharacteristicCallbacks {

  void onWrite(BLECharacteristic *pChar) override {
    Serial.println("TextCallbacks");

    String cmd = pChar->getValue();
    cmd.trim();

    Serial.print("Text cmd: ");
    Serial.println(cmd);


   if (cmd == "send data to esp32") {
  startBinaryReception();
}
else if (cmd == "send array to ios") {
  sendLedsInfoChunked();
}else if (cmd.startsWith("test ")) {
      Serial.println("test command");
      String numStr = cmd.substring(5); // after "test "
    int value = numStr.toInt();

    if (value < 0 || value > 255) {
      Serial.println("❌ LED index out of range");
      return;
    }

    uint8_t led = (uint8_t)value;

    // Start / restart test
    testLED(led);
    testStartMillis = millis();
    testActive = true;

    Serial.print("⏱ Test started for LED ");
    Serial.println(led);
    }

  }
};

void startBinaryReception() {

  bytesReceived = 0;
  headerBytesReceived = 0;
  expectedBytes = 0;
  expectedCRC = 0;

  receivingBinary = true;
  receivingHeader = true;

  Serial.println("🟢 Ready for binary header");
};


/* ===============================
    Stuff
   ===============================*/

void printArray() {

 for (int i = 0; i < NUM_LEDS; i++) {
    Serial.print("LED ");
    Serial.println(i);
    Serial.print(" interval=");
    Serial.println(leds_Info[i].interval);
    Serial.print(" base=");
    Serial.println(leds_Info[i].baseBrightness);
    Serial.print(" candleBlue=");
    Serial.println(leds_Info[i].candleBlue);
    Serial.print(" house=");
    Serial.println(leds_Info[i].house);
    Serial.print(" candleRed=");
    Serial.println(leds_Info[i].candleRed);
    Serial.print(" candleGreen=");
    Serial.println(leds_Info[i].candleGreen);
  }

}


void sendLedsInfoChunked() {

if (!deviceConnected) return;

  for (uint8_t p = 0; p < TOTAL_PACKETS; p++) {

    uint8_t buffer[2 + STRUCTS_PER_PACKET * sizeof(LedsInfo)];
    buffer[0] = p;
    buffer[1] = TOTAL_PACKETS;

    int start = p * STRUCTS_PER_PACKET;
    Serial.print("start = ");Serial.println(start);
    int count = min(STRUCTS_PER_PACKET, NUM_LEDS - start);
    Serial.print("count = ");Serial.println(count);

    memcpy(&buffer[2],
           &leds_Info[start],
           count * sizeof(LedsInfo));

    dataTxChar->setValue(buffer, 2 + count * sizeof(LedsInfo));
    dataTxChar->notify();

    delay(15);
     Serial.print("send data packet : ");Serial.print(p);Serial.print(" of ");Serial.println(TOTAL_PACKETS);
  }
}
/* ===============================
   BLE Init
   =============================== */

void bleInit() {
    BLEDevice::init(GAP_NAME);
    BLEDevice::setMTU(BLE_MTU_SIZE);

    BLEServer* server = BLEDevice::createServer();
    server->setCallbacks(new ServerCallbacks());


    BLEService *service = server->createService(SERVICE_UUID);

    // iOS → ESP32 (TEXT)
    textRxChar = service->createCharacteristic(
        TEXT_RX_UUID,
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_WRITE_NR
    );
    textRxChar->setCallbacks(new TextCallbacks());

    // iOS → ESP32 (BINARY)
    dataRxChar = service->createCharacteristic(
        DATA_RX_UUID,
        BLECharacteristic::PROPERTY_WRITE
    );
    dataRxChar->setCallbacks(new DataCallbacks());

    // ESP32 → iOS (TEXT)
    textTxChar = service->createCharacteristic(
        TEXT_TX_UUID,
        BLECharacteristic::PROPERTY_NOTIFY |
        BLECharacteristic::PROPERTY_READ
    );
    textTxChar->addDescriptor(new BLE2902());

    // ESP32 → iOS (BINARY)
    dataTxChar = service->createCharacteristic(
        DATA_TX_UUID,
        BLECharacteristic::PROPERTY_NOTIFY
    );
    dataTxChar->addDescriptor(new BLE2902());

    // ESP32 → iOS (ID)
    idChar = service->createCharacteristic(
        ID_CHAR_UUID,
        BLECharacteristic::PROPERTY_READ
    );
    idChar->setValue("XMASCITY_1");

    BLEAdvertisementData adv;
    adv.setName(BLE_ADVERTISING_NAME);

    uint16_t companyID = 0xFFFF; // test ID

    String mfg;
    mfg.reserve(2 + strlen("XMASCITY_1"));

    // Company ID (little-endian per BLE spec)
    mfg += char(companyID & 0xFF);
    mfg += char((companyID >> 8) & 0xFF);

    // Payload
    mfg += "XMASCITY_1";

    // ✅ CORRECT for Arduino-ESP32
    adv.setManufacturerData(mfg);


    // 🔑 MUST be last
    service->start();

    BLEAdvertising *advertising = BLEDevice::getAdvertising();
    advertising->setAdvertisementData(adv);
    advertising->setScanResponse(true);
    advertising->start();




    if (!loadLedsInfoArray(leds_Info, NUM_LEDS, loadedCount)) {
          Serial.println("No saved data — writing defaults");
          loadDefault_LedsInfo();
        }else {
          Serial.println("Loading saved array");
        }

        printArray();


}

void testLED(uint8_t led) {
  Serial.print("🧪 Testing LED ");
  Serial.println(led);
  
  if (led != 99) {
      storedLED.candleRed =  leds[led].r;
      storedLED.candleGreen =  leds[led].g;
      storedLED.candleBlue =  leds[led].b;

      leds[led].r = 255;
      leds[led].g = 0;
      leds[led].b = 0;
      Serial.println("set led");
  }else {
      leds[led].r = storedLED.candleRed;
      leds[led].g = storedLED.candleGreen;
      leds[led].b = storedLED.candleBlue;
      Serial.println("reset led");
      testActive = false;
  }

}

void endTest() {
  Serial.println("🛑 Test ended");
  testActive = false;
  // restore normal LED state here
}



