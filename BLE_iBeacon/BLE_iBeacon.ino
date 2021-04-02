#include "sys/time.h"
#include "BLEDevice.h"
#include "BLEUtils.h"
#include "BLEServer.h"
#include "BLEBeacon.h"
#include "esp_sleep.h"

#define GPIO_DEEP_SLEEP_DURATION     1  // sleep x seconds and then wake up
RTC_DATA_ATTR static time_t last;        // remember last boot in RTC Memory
RTC_DATA_ATTR static uint32_t bootcount; // remember number of boots in RTC Memory

#define Mode_Button 25  // Continuous Mode - Trigger Mode
#define Trigger 4 // Trigger Button 

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/
BLEAdvertising *pAdvertising;   // BLE Advertisement type
struct timeval now;

#define BEACON_UUID "87b99b2c-90fd-11e9-bc42-526af7764f64" // UUID 1 128-Bit (may use linux tool uuidgen or random numbers via https://www.uuidgenerator.net/)

void setBeacon()
{

  BLEBeacon oBeacon = BLEBeacon();
  oBeacon.setManufacturerId(0x4C00); // fake Apple 0x004C LSB (ENDIAN_CHANGE_U16!)
  oBeacon.setProximityUUID(BLEUUID(BEACON_UUID));
  oBeacon.setMajor((bootcount & 0xFFFF0000) >> 16);
  oBeacon.setMinor(bootcount & 0xFFFF);
  BLEAdvertisementData oAdvertisementData = BLEAdvertisementData();
  BLEAdvertisementData oScanResponseData = BLEAdvertisementData();

  oAdvertisementData.setFlags(0x04); // BR_EDR_NOT_SUPPORTED 0x04

  std::string strServiceData = "";

  strServiceData += (char)26;     // Len
  strServiceData += (char)0xFF;   // Type
  strServiceData += oBeacon.getData();
  oAdvertisementData.addData(strServiceData);

  pAdvertising->setAdvertisementData(oAdvertisementData);
  pAdvertising->setScanResponseData(oScanResponseData);
}

void setup() {

  Serial.begin(115200);
  pinMode(Mode_Button, INPUT);
  pinMode(Trigger, OUTPUT);
  gettimeofday(&now, NULL);
  Serial.printf("start ESP32 %d\n", bootcount++);
  Serial.printf("deep sleep (%lds since last reset, %lds since last boot)\n", now.tv_sec, now.tv_sec - last);
  last = now.tv_sec;

}

void loop() {

  if (digitalRead(Mode_Button)) // Continuous Transmitting Mode (Consumes more battery)
  {
    Serial.println("Continuous mode");
    // Create the BLE Device
    BLEDevice::init("techiesms Beacon Keychain"); // Name of your Beacon Device 
    // Create the BLE Server
    BLEServer *pServer = BLEDevice::createServer(); // <-- no longer required to instantiate BLEServer, less flash and ram usage
    pAdvertising = BLEDevice::getAdvertising();
    BLEDevice::startAdvertising();
    setBeacon();
    // Start advertising
    pAdvertising->start();
    Serial.println("Advertizing started...");
    delay(1000);
    pAdvertising->stop();
    Serial.printf("enter deep sleep\n");
    esp_deep_sleep(5000000LL * GPIO_DEEP_SLEEP_DURATION);
    Serial.printf("in deep sleep\n");
  }

  else  // Transmit on Trigger Mode
  {
    Serial.println("Trigger mode");
    // Create the BLE Device
    BLEDevice::init("techiesms Beacon Keychain");
    // Create the BLE Server
    BLEServer *pServer = BLEDevice::createServer(); // <-- no longer required to instantiate BLEServer, less flash and ram usage
    pAdvertising = BLEDevice::getAdvertising();
    BLEDevice::startAdvertising();
    setBeacon();
    // Start advertising
    pAdvertising->start();
    Serial.println("Advertizing started...");
    delay(1000);
    pAdvertising->stop();


    //Configure GPIO2 as ext0 wake up source for HIGH logic level
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_4, 0);

    //Go to sleep now
    esp_deep_sleep_start();
  }
}
