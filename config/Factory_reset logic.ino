#include <nvs_flash.h>

void performFactoryReset() {
    Serial.println("Erasing NVS Partition... Factory Reset in progress.");
    
    // Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    
    // Wipe all stored credentials and settings
    nvs_flash_erase(); 
    Serial.println("NVS Erased. Restarting ESP32...");
    delay(1000);
    esp_restart();
}