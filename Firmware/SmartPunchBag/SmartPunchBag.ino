// Libraries
#include <Arduino.h>
#include "LCD_Test.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <BLEBeacon.h>

// Definitions (Constants)
#define DEVICE_NAME            "SensAi Punch Bag"
#define SERVICE_UUID           "7A0247E7-8E88-409B-A959-AB5092DDB03E"
#define BEACON_UUID            "2D7A9F0C-E0E8-4CC9-A71B-A21DB2D034A1"
#define BEACON_UUID_REV        "A134D0B2-1DA2-1BA7-C94C-E8E00C9F7A2D"
#define CHARACTERISTIC_UUID    "82258BAA-DF72-47E8-99BC-B73D7ECD08A5"

// Variables
UWORD Imagesize = LCD_1IN28_HEIGHT * LCD_1IN28_WIDTH * 2;
UWORD *BlackImage = NULL;
float acc[3], gyro[3], accMin[3], accMax[3], accDelta[3], fResult;
uint16_t uiCount[3], uiGeneralCount;
unsigned int tim_count = 0;
uint16_t result, i;
const float conversion_factor = 3.3f / (1 << 12) * 3;
// Bluetooth variables
BLEServer *pServer;
BLECharacteristic *pCharacteristic;
bool deviceConnected = false;
uint8_t value = 0;

// Bluetooth actions
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("deviceConnected = true");
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("deviceConnected = false");

      // Restart advertising to be visible and connectable again
      BLEAdvertising* pAdvertising;
      pAdvertising = pServer->getAdvertising();
      pAdvertising->start();
      Serial.println("iBeacon advertising restarted");
    }
};

// Bluetooth actions
class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();

      if (rxValue.length() > 0) {
        Serial.print("Received Value: ");
        for (int i = 0; i < rxValue.length(); i++) {
          Serial.print(rxValue[i]);
        }
        Serial.println();
      }
    }
};

// Bluetooth initializations
void init_service() {
  BLEAdvertising* pAdvertising;
  pAdvertising = pServer->getAdvertising();
  pAdvertising->stop();

  // Create the BLE Service
  BLEService *pService = pServer->createService(BLEUUID(SERVICE_UUID));

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
  pCharacteristic->setCallbacks(new MyCallbacks());
  pCharacteristic->addDescriptor(new BLE2902());

  pAdvertising->addServiceUUID(BLEUUID(SERVICE_UUID));

  // Start the service
  pService->start();

  pAdvertising->start();
}

// Bluetooth initializations
void init_beacon() {
  BLEAdvertising* pAdvertising;
  pAdvertising = pServer->getAdvertising();
  pAdvertising->stop();
  // iBeacon
  BLEBeacon myBeacon;
  myBeacon.setManufacturerId(0x4c00);
  myBeacon.setMajor(5);
  myBeacon.setMinor(88);
  myBeacon.setSignalPower(0xc5);
  myBeacon.setProximityUUID(BLEUUID(BEACON_UUID_REV));

  BLEAdvertisementData advertisementData;
  advertisementData.setFlags(0x1A);
  advertisementData.setManufacturerData(myBeacon.getData());
  pAdvertising->setAdvertisementData(advertisementData);

  pAdvertising->start();
}

// Setup of program working
void setup()
{
  Serial.begin(115200);

  //PSRAM Initialize
  if(psramInit()){
    Serial.println("\nPSRAM is correctly initialized");
  }else{
    Serial.println("PSRAM not available");
  }
  if ((BlackImage = (UWORD *)ps_malloc(Imagesize)) == NULL){
    Serial.println("Failed to apply for black memory...");
    exit(0);
  }

  if (DEV_Module_Init() != 0)
    Serial.println("GPIO Init Fail!");
  else
    Serial.println("GPIO Init successful!");

  // Configure display
  LCD_1IN28_Init(HORIZONTAL);
  LCD_1IN28_Clear(WHITE);  
  /*1.Create a new image cache named IMAGE_RGB and fill it with white*/
  Paint_NewImage((UBYTE *)BlackImage, LCD_1IN28.WIDTH, LCD_1IN28.HEIGHT, 0, WHITE);
  Paint_SetScale(65);
  Paint_SetRotate(ROTATE_0);
  Paint_Clear(WHITE);

  // Configure gyroscope
  QMI8658_init();

  // Configure Bluetooth
  BLEDevice::init(DEVICE_NAME);
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  init_service();
  init_beacon();
  
  // First image on display
  Paint_Clear(WHITE);
  // France flag
  Paint_DrawRectangle(80, 20, 110, 70, RED, DOT_PIXEL_2X2, DRAW_FILL_FULL);
  Paint_DrawRectangle(110, 20, 140, 70, WHITE, DOT_PIXEL_2X2, DRAW_FILL_FULL);
  Paint_DrawRectangle(140, 20, 170, 70, BLUE, DOT_PIXEL_2X2, DRAW_FILL_FULL);
  // Informations
  Paint_DrawString_EN(70, 80, "SensAi", &Font24, WHITE, BLACK);
  Paint_DrawString_EN(20, 110, "Technologies", &Font24, WHITE, BLACK);
  LCD_1IN28_Display(BlackImage);

  // Initialize variables
  for (i = 0; i < 3; i++)
  {
    accMin[i] = 99999.9;
    accMax[i] = -99999.9;
    uiCount[i] = 0;
  }
  uiGeneralCount = 0;
}

void loop()
{
  result = DEC_ADC_Read();
  QMI8658_read_xyz(acc, gyro, &tim_count);
  
  // Analyse maximum and minimum acceleration (delta)
  for (i = 0; i < 3; i++)
  {
    // Analisys and clean after timout
    if (uiCount[i] >= 25) // Impact of 250ms maximum
    {
      accDelta[i] = accMax[i] - accMin[i];
      accMin[i] = 99999.9;
      accMax[i] = -99999.9;
      uiCount[i] = 0;
    }
    else
    {
      uiCount[i]++;
    }
    
    // Find maximum and minimum
    if (acc[i] < accMin[i])
    {
      accMin[i] = acc[i];
      uiCount[i] = 0;
    } 
    if (acc[i] > accMax[i])
    {
      accMax[i] = acc[i];
      uiCount[i] = 0;
    }
  }

  // Verify better delta result
  fResult = 0.0;
  for (i = 0; i < 3; i++)
  {
    if (accDelta[i] > fResult)
    {
      fResult = accDelta[i];
    }
  }

  // We got a punch!
  if (fResult > 1000.0)
  {
    // Write on display
    uiGeneralCount = 0; 
    Paint_Clear(RED);
    Paint_DrawString_EN(70, 80, "Punch!", &Font24, WHITE, BLACK);
    Paint_DrawRectangle(20, 140, 220, 180, 0X0000, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);
    Paint_DrawString_EN(35, 150, "Force=", &Font20, WHITE, BLACK);
    Paint_DrawNum(115, 150, fResult, &Font20, 2, BLACK, WHITE);
    LCD_1IN28_Display(BlackImage);
    // Send to Bluetooth
    String sForce = "Force=";
    char stringFloat[20];
    dtostrf(fResult, 6, 2, stringFloat);
    sForce += String(stringFloat);
    pCharacteristic->setValue(sForce.c_str());
    pCharacteristic->notify();
    value++;

    // Clean for next
    for (i = 0; i < 3; i++)
    {
      accDelta[i] = 0.0;
      accMin[i] = 99999.9;
      accMax[i] = -99999.9;
      uiCount[i] = 0;
    }
  }
  else
  {
    // Time of 5s enter save screen
    if (uiGeneralCount < 500)
    {
      uiGeneralCount++;
      if (uiGeneralCount == 500)
      {
        LCD_1IN28_Clear(BLACK); // Black screen to save energy
      }
    }
  }

  // Time to each loop
  delay(10);
}


