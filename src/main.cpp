#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

#define SERVICE_UUID        "4fa2c732-cc4a-e1bf-b349-41ef3b3501e8"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

BLECharacteristic *pCharacteristic = NULL; // Alustetaan varmuuden vuoksi NULLiksi
bool laiteYhdistetty = false;

class OmaServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      laiteYhdistetty = true;
      Serial.println("[BLE] Puhelin yhdistetty!");
    };

    void onDisconnect(BLEServer* pServer) {
      laiteYhdistetty = false;
      Serial.println("[BLE] Puhelin katkaisi yhteyden. Aloitetaan mainostus uudelleen...");
      pServer->startAdvertising();
    }
};

// Tehtävä 1: Pyörii Ytimellä 1 ja päivittää anturitietoa
void AnturiJaBLETehtava(void * pvParameters) {
  int simuloituAnturi = 20;

  for(;;) {
    simuloituAnturi++;
    if(simuloituAnturi > 40) simuloituAnturi = 20;

    Serial.print("[Ydin 1] Päivitetään BLE-arvo: ");
    Serial.println(simuloituAnturi);

    // Käytetään Arduinon String-luokkaa C-taulukoiden sijaan. 
    // BLE-kirjasto tukee tätä suoraan.
    String lahetettavaTeksti = String(simuloituAnturi);
    
    if (pCharacteristic != NULL) {
      // Asetetaan arvo String-muodossa
      pCharacteristic->setValue(lahetettavaTeksti.c_str());

      if (laiteYhdistetty) {
        pCharacteristic->notify();
        Serial.println("[Ydin 1] Ilmoitus lähetetty puhelimeen.");
      }
    }

    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}




void setup() {
  Serial.begin(115200);
  vTaskDelay(pdMS_TO_TICKS(1000));

  Serial.println("--- ALUSTETAAN JÄRJESTELMÄ ---");

  // Alustetaan BLE ja annetaan sille taas uusi nimi välimuistin nollaamiseksi!
  BLEDevice::init("ESP32_Toimii");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new OmaServerCallbacks());
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Luodaan ominaisuus
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );

  // AMMATTILAISKORJAUS: Lisätään BLE2902-deskriptori, joka sallii Notify-ilmoitukset puhelimeen
  pCharacteristic->addDescriptor(new BLE2902());

  pCharacteristic->setValue("0");
  pService->start();
  
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  
  BLEDevice::startAdvertising();
  Serial.println("--- BLE ALUSTETTU ONNISTUNEESTI ---");

  // Käynnistetään sovelluslogiikka Ytimellä 1
  xTaskCreatePinnedToCore(AnturiJaBLETehtava, "Anturi_BLE", 4000, NULL, 1, NULL, 1);
}


void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));
}
