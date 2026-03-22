
#include "esp_camera.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "time.h"
#include <Preferences.h>
#include <ArduinoOTA.h>
#include <mbedtls/sha256.h>

#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"

// Hardware Pins
#define RELAY_PIN   2
#define BUTTON_PIN  15
#define BUZZER_PIN  12
#define I2C_SDA     13
#define I2C_SCL     14
#define FLASH_LED_PIN 4

// Display Optimization
bool displayNeedsUpdate = true;
unsigned long lastClockUpdate = 0;
const unsigned long CLOCK_REFRESH_MS = 60000;
unsigned long lastSirenDisplayUpdate = 0;
const unsigned long SIREN_DISPLAY_INTERVAL = 1000; // OLED flashes at 1s, buzzer at its own rate

// === FAST WiFi RECONNECT ===
RTC_DATA_ATTR struct {
  uint8_t bssid[6];
  uint8_t channel;
  bool valid;
} wifiCache;

// === SECURITY: Credentials loaded from encrypted storage ===
String ssid;
String password;
String BOTtoken;
String MASTER_PASSWORD_HASH;
String MASTER_OTA_PASSWORD;

// Telegram root CA (not used in ESP32 1.0.6, but kept for future upgrades)
const char telegram_root_ca[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----
)EOF";

struct AuthorizedUser {
  String chat_id;
  String name;
  int accessLevel;
};

// === UPDATE YOUR CHAT IDs HERE ===
AuthorizedUser authorizedUsers[] = {
  {"7114322086", "charles", 0},       // Admin (auto-login)
  {"5665066332", "Mom", 0},         // Admin (auto-login)
  {"721369496", "Sister", 1},      // Family (needs login)
  {"777777777", "Grandma", 1},     // Family (needs login)
  {"999999999", "Neighbor", 2}     // Guest (view only)
};

const int NUM_USERS = 5;
const unsigned long SESSION_TIMEOUT = 3600000;

// Temporary login tracking
struct TempLoginState {
  String chat_id;
  bool awaitingPassword;
  unsigned long timestamp;
};

TempLoginState tempLogins[5];
const unsigned long TEMP_LOGIN_TIMEOUT = 300000;

struct UserSession {
  String chat_id;
  bool isLoggedIn;
  unsigned long loginTime;
  int accessLevel;
  String userName;
  unsigned long lastCommandTime;
  bool awaitingMessage;
};

const int MAX_SESSIONS = 10;
UserSession activeSessions[MAX_SESSIONS];

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 0;

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

WiFiClientSecure clientTCP;
UniversalTelegramBot* bot = nullptr;
Preferences prefs;

extern void startCameraServer();
extern void checkForFaces();
extern volatile bool matchFace;
extern void startEnrollment();
extern void deleteAllFaces();

enum State { STANDBY, SCANNING, UNLOCKED, ALARM, SIREN };
State currentState = STANDBY;

unsigned long scanStartTime = 0;
const unsigned long SCAN_TIMEOUT = 60000;
unsigned long unlockStartTime = 0;
const unsigned long UNLOCK_TIME = 15000;
unsigned long lastTelegramCheck = 0;
const unsigned long BOT_MTBS = 1000;
unsigned long alarmStartTime = 0;
const unsigned long ALARM_DISPLAY_TIME = 3000;
unsigned long lastSirenBeep = 0;
const unsigned long SIREN_BEEP_INTERVAL = 800;  // buzzer alternates every 800ms
bool sirenState = false;

unsigned long lastButtonPress = 0;
const unsigned long DEBOUNCE_DELAY = 200;
unsigned long lastPhotoTime = 0;

const unsigned long COMMAND_COOLDOWN = 2000;
const unsigned long PHOTO_COOLDOWN = 5000;

String cachedTimeStr = "--:--";  // cached clock string, updated only on clock refresh
String customMessage = "";
String lastEntryTime = "--:--";
String lastUnlockedBy = "Unknown";
int intruderCount = 0;

const unsigned char icon_lock[] PROGMEM = {
  0x03, 0xc0, 0x04, 0x20, 0x08, 0x10, 0x08, 0x10, 0x08, 0x10, 0x3f, 0xfc,
  0x3f, 0xfc, 0x30, 0x0c, 0x30, 0x0c, 0x30, 0x0c, 0x31, 0x8c, 0x31, 0x8c,
  0x30, 0x0c, 0x3f, 0xfc, 0x3f, 0xfc, 0x00, 0x00
};

const unsigned char icon_unlock[] PROGMEM = {
  0x03, 0xc0, 0x04, 0x20, 0x08, 0x10, 0x08, 0x10, 0x08, 0x00, 0x3f, 0xfc,
  0x3f, 0xfc, 0x30, 0x0c, 0x30, 0x0c, 0x30, 0x0c, 0x31, 0x8c, 0x31, 0x8c,
  0x30, 0x0c, 0x3f, 0xfc, 0x3f, 0xfc, 0x00, 0x00
};

const unsigned char icon_scan[] PROGMEM = {
  0xff, 0xff, 0x80, 0x01, 0x80, 0x01, 0x88, 0x11, 0x88, 0x11, 0x80, 0x01,
  0x9c, 0x39, 0x9c, 0x39, 0x9c, 0x39, 0x80, 0x01, 0x88, 0x11, 0x88, 0x11,
  0x80, 0x01, 0x80, 0x01, 0xff, 0xff, 0x00, 0x00
};

const unsigned char icon_alert[] PROGMEM = {
  0x01, 0x80, 0x03, 0xc0, 0x07, 0xe0, 0x0f, 0xf0, 0x1e, 0x78, 0x3c, 0x3c,
  0x78, 0x1e, 0x70, 0x0e, 0xe1, 0x87, 0xe1, 0x87, 0xe0, 0x07, 0x70, 0x0e,
  0x78, 0x1e, 0x3c, 0x3c, 0x1f, 0xf8, 0x00, 0x00
};


const unsigned char icon_clock[] PROGMEM = {
  0x07, 0xe0, 0x1f, 0xf8, 0x3c, 0x3c, 0x78, 0x1e, 0x70, 0x0e, 0xe0, 0x07,
  0xe0, 0xc7, 0xc1, 0xe3, 0xc1, 0xe3, 0xc0, 0x63, 0xe0, 0x07, 0x70, 0x0e,
  0x78, 0x1e, 0x3c, 0x3c, 0x1f, 0xf8, 0x07, 0xe0
};

// ── Face verification animation (32x32, 11 frames, 1408 bytes total) ──────────
// Source: 64x64 Wokwi animation frames 9-19, downsampled 2x2 majority vote
#define FACE_ANIM_FRAME_COUNT 11
#define FACE_ANIM_FRAME_DELAY 55
#define FACE_ANIM_W 32
#define FACE_ANIM_H 32
const uint8_t PROGMEM faceAnimFrames[][128] = {
  {0x00,0x00,0x00,0x00,0x0f,0x80,0x01,0xf0,0x1f,0x80,0x01,0xf8,0x20,0x00,0x00,0x04,0x60,0x00,0x00,0x06,0x60,0x00,0x00,0x06,0x60,0x00,0x00,0x06,0x60,0x00,0x00,0x06,0x60,0x00,0x00,0x06,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x60,0x00,0x00,0x06,0x60,0x00,0x00,0x06,0x60,0x00,0x00,0x06,0x60,0x00,0x00,0x06,0x60,0x00,0x00,0x06,0x20,0x00,0x00,0x04,0x1f,0x80,0x01,0xf8,0x0f,0x80,0x01,0xf0,0x00,0x00,0x00,0x00},
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0xe0,0x07,0xc0,0x07,0x00,0x00,0xe0,0x08,0x00,0x00,0x10,0x10,0x00,0x00,0x08,0x30,0x00,0x00,0x0c,0x30,0x00,0x00,0x0c,0x20,0x00,0x00,0x04,0x20,0x00,0x00,0x04,0x20,0x00,0x00,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x00,0x00,0x04,0x20,0x00,0x00,0x04,0x20,0x00,0x00,0x04,0x20,0x00,0x00,0x04,0x30,0x00,0x00,0x0c,0x30,0x00,0x00,0x0c,0x10,0x00,0x00,0x18,0x08,0x00,0x00,0x30,0x07,0x00,0x00,0xe0,0x03,0xe0,0x07,0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00,0x00,0xf8,0x1f,0x00,0x01,0xc0,0x01,0x80,0x03,0x00,0x00,0x40,0x04,0x00,0x00,0x20,0x08,0x00,0x00,0x10,0x18,0x00,0x00,0x18,0x10,0x00,0x00,0x08,0x10,0x00,0x00,0x0c,0x30,0x00,0x00,0x0c,0x30,0x00,0x00,0x0c,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x00,0x00,0x00,0x30,0x00,0x00,0x00,0x18,0x00,0x00,0x00,0x0c,0x00,0x08,0x30,0x04,0x00,0x0c,0x30,0x00,0x00,0x08,0x10,0x00,0x00,0x08,0x18,0x00,0x00,0x18,0x18,0x00,0x00,0x10,0x08,0x00,0x00,0x30,0x04,0x00,0x00,0x20,0x02,0x00,0x00,0xc0,0x01,0x80,0x03,0x80,0x00,0xf8,0x1f,0x00,0x00,0x18,0x18,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x80,0x00,0x00,0x1f,0xf8,0x00,0x00,0x78,0x0e,0x00,0x00,0xc0,0x03,0x00,0x01,0x00,0x00,0x80,0x02,0x00,0x00,0x40,0x04,0x00,0x00,0x60,0x0c,0x00,0x00,0x30,0x08,0x00,0x00,0x10,0x18,0x00,0x00,0x18,0x10,0x00,0x00,0x18,0x10,0x00,0x00,0x08,0x30,0x00,0x00,0x08,0x30,0x20,0x20,0x0c,0x30,0x30,0x40,0x08,0x30,0x18,0xc0,0x08,0x10,0x0d,0x80,0x08,0x10,0x07,0x00,0x18,0x18,0x02,0x00,0x18,0x08,0x00,0x00,0x10,0x0c,0x00,0x00,0x30,0x04,0x00,0x00,0x20,0x02,0x00,0x00,0x40,0x01,0x00,0x00,0x80,0x00,0xc0,0x03,0x00,0x00,0x70,0x1e,0x00,0x00,0x1f,0xf8,0x00,0x00,0x01,0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x80,0x00,0x00,0x1f,0xf8,0x00,0x00,0x78,0x0e,0x00,0x00,0xc0,0x03,0x00,0x01,0x00,0x00,0x80,0x02,0x00,0x00,0x40,0x04,0x00,0x00,0x60,0x0c,0x00,0x00,0x30,0x08,0x00,0x00,0x10,0x18,0x00,0x04,0x18,0x10,0x00,0x0c,0x18,0x10,0x00,0x18,0x08,0x30,0x00,0x10,0x08,0x30,0x20,0x20,0x0c,0x30,0x30,0x60,0x08,0x30,0x18,0xc0,0x08,0x10,0x0d,0x80,0x08,0x10,0x07,0x00,0x18,0x18,0x02,0x00,0x18,0x08,0x00,0x00,0x10,0x0c,0x00,0x00,0x30,0x04,0x00,0x00,0x20,0x02,0x00,0x00,0x40,0x01,0x00,0x00,0x80,0x00,0xc0,0x03,0x00,0x00,0x70,0x1e,0x00,0x00,0x1f,0xf8,0x00,0x00,0x01,0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x80,0x00,0x00,0x1f,0xf8,0x00,0x00,0x78,0x0e,0x00,0x00,0xc0,0x03,0x00,0x01,0x00,0x00,0x80,0x02,0x00,0x00,0x40,0x04,0x00,0x00,0x60,0x0c,0x00,0x00,0x30,0x08,0x00,0x00,0x10,0x18,0x00,0x04,0x18,0x10,0x00,0x0c,0x18,0x10,0x00,0x18,0x08,0x30,0x00,0x10,0x08,0x30,0x20,0x20,0x0c,0x30,0x30,0x60,0x08,0x30,0x18,0xc0,0x08,0x10,0x0d,0x80,0x08,0x10,0x07,0x00,0x18,0x18,0x02,0x00,0x18,0x08,0x00,0x00,0x10,0x0c,0x00,0x00,0x30,0x04,0x00,0x00,0x20,0x02,0x00,0x00,0x40,0x01,0x00,0x00,0x80,0x00,0xc0,0x03,0x00,0x00,0x70,0x1e,0x00,0x00,0x1f,0xf8,0x00,0x00,0x01,0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x80,0x00,0x00,0x1f,0xf8,0x00,0x00,0x78,0x0e,0x00,0x00,0xc0,0x03,0x00,0x01,0x00,0x00,0x80,0x02,0x00,0x00,0x40,0x04,0x00,0x00,0x60,0x0c,0x00,0x00,0x30,0x08,0x00,0x00,0x10,0x18,0x00,0x04,0x18,0x10,0x00,0x0c,0x18,0x10,0x00,0x18,0x08,0x30,0x00,0x10,0x08,0x30,0x20,0x20,0x0c,0x30,0x30,0x60,0x08,0x30,0x18,0xc0,0x08,0x10,0x0d,0x80,0x08,0x10,0x07,0x00,0x18,0x18,0x02,0x00,0x18,0x08,0x00,0x00,0x10,0x0c,0x00,0x00,0x30,0x04,0x00,0x00,0x20,0x02,0x00,0x00,0x40,0x01,0x00,0x00,0x80,0x00,0xc0,0x03,0x00,0x00,0x70,0x1e,0x00,0x00,0x1f,0xf8,0x00,0x00,0x01,0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x80,0x00,0x00,0x1f,0xf8,0x00,0x00,0x78,0x0e,0x00,0x00,0xc0,0x03,0x00,0x01,0x00,0x00,0x80,0x02,0x00,0x00,0x40,0x04,0x00,0x00,0x60,0x0c,0x00,0x00,0x30,0x08,0x00,0x00,0x10,0x18,0x00,0x04,0x18,0x10,0x00,0x0c,0x18,0x10,0x00,0x18,0x08,0x30,0x00,0x10,0x08,0x30,0x20,0x20,0x0c,0x30,0x30,0x60,0x08,0x30,0x18,0xc0,0x08,0x10,0x0d,0x80,0x08,0x10,0x07,0x00,0x18,0x18,0x02,0x00,0x18,0x08,0x00,0x00,0x10,0x0c,0x00,0x00,0x30,0x04,0x00,0x00,0x20,0x02,0x00,0x00,0x40,0x01,0x00,0x00,0x80,0x00,0xc0,0x03,0x00,0x00,0x70,0x1e,0x00,0x00,0x1f,0xf8,0x00,0x00,0x01,0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x80,0x00,0x00,0x1f,0xf8,0x00,0x00,0x78,0x0e,0x00,0x00,0xc0,0x03,0x00,0x01,0x00,0x00,0x80,0x02,0x00,0x00,0x40,0x04,0x00,0x00,0x60,0x0c,0x00,0x00,0x30,0x08,0x00,0x00,0x10,0x18,0x00,0x04,0x18,0x10,0x00,0x0c,0x18,0x10,0x00,0x18,0x08,0x30,0x00,0x10,0x08,0x30,0x20,0x20,0x0c,0x30,0x30,0x60,0x08,0x30,0x18,0xc0,0x08,0x10,0x0d,0x80,0x08,0x10,0x07,0x00,0x18,0x18,0x02,0x00,0x18,0x08,0x00,0x00,0x10,0x0c,0x00,0x00,0x30,0x04,0x00,0x00,0x20,0x02,0x00,0x00,0x40,0x01,0x00,0x00,0x80,0x00,0xc0,0x03,0x00,0x00,0x70,0x1e,0x00,0x00,0x1f,0xf8,0x00,0x00,0x01,0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x80,0x00,0x00,0x1f,0xf8,0x00,0x00,0x78,0x0e,0x00,0x00,0xc0,0x03,0x00,0x01,0x00,0x00,0x80,0x02,0x00,0x00,0x40,0x04,0x00,0x00,0x60,0x0c,0x00,0x00,0x30,0x08,0x00,0x00,0x10,0x18,0x00,0x04,0x18,0x10,0x00,0x0c,0x18,0x10,0x00,0x18,0x08,0x30,0x00,0x10,0x08,0x30,0x20,0x20,0x0c,0x30,0x10,0x40,0x08,0x30,0x18,0xc0,0x08,0x10,0x08,0x80,0x08,0x10,0x07,0x00,0x18,0x18,0x02,0x00,0x18,0x08,0x00,0x00,0x10,0x0c,0x00,0x00,0x30,0x04,0x00,0x00,0x20,0x02,0x00,0x00,0x40,0x01,0x00,0x00,0x80,0x00,0xc0,0x03,0x00,0x00,0x70,0x1e,0x00,0x00,0x1f,0xf8,0x00,0x00,0x01,0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}
};

// Animation playback state
int  faceAnimFrame     = 0;
bool faceAnimPlaying   = false;
unsigned long lastFaceAnimTime = 0;

// Function declarations
extern int perform_standalone_ai_scan();
extern void trigger_enrollment();
void updateDisplay();
void drawHeader();
void handleNewMessages(int numNewMessages);
void handleCallbackQuery(int numNewMessages);
void removeInlineKeyboard(String chat_id, String message_id);
String sendPhoto(const String &caption, String target_chat_id);
String sendPhotoWithKeyboard(const String &caption, String target_chat_id, String keyboardJson);
void tone(int pin, int freq, int duration);
void sendMainMenu(String chat_id, String message, int accessLevel);
void sendActionRequired(String chat_id, String message);
void sendSirenControl(String chat_id, String message);
void sendLoginPrompt(String chat_id);
void sendHelpMessage(String chat_id, int accessLevel);
void sendStatusMessage(String chat_id);
bool connectWiFi();
bool initCamera();
int getUserAccessLevel(String chat_id);
String getUserName(String chat_id);
bool isUserAuthorized(String chat_id);
UserSession* getSession(String chat_id);
void createSession(String chat_id, int accessLevel, String userName);
void clearSession(String chat_id);
void checkSessionTimeouts();
bool isLoggedIn(String chat_id);
void notifyAllAdmins(String message);
void sendTelegramStatusPin(String unlockedBy, String entryTime);
void wifiReconnectIfNeeded();
bool checkCommandCooldown(String chat_id, bool isPhotoCommand);
String getStateString();
bool loadCredentials();
void saveCredentials();
void provisionDevice();
TempLoginState* getTempLogin(String chat_id);
TempLoginState* createTempLogin(String chat_id);
void clearTempLogin(String chat_id);
void cleanupTempLogins();
void setupOTA();

// ═══════════════════════════════════════════════════════════
// PASSWORD SECURITY - SHA-256 WITH CONSTANT-TIME COMPARISON
// ═══════════════════════════════════════════════════════════

String sha256Hex(const String &input) {
  unsigned char output[32];
  mbedtls_sha256_context ctx;
  
  mbedtls_sha256_init(&ctx);
  mbedtls_sha256_starts_ret(&ctx, 0);
  mbedtls_sha256_update_ret(&ctx, (const unsigned char*)input.c_str(), input.length());
  mbedtls_sha256_finish_ret(&ctx, output);
  mbedtls_sha256_free(&ctx);

  char hex[65];
  for (int i = 0; i < 32; ++i) {
    sprintf(hex + i*2, "%02x", output[i]);
  }
  hex[64] = 0;
  
  return String(hex);
}

bool verifyMasterPassword(const String &candidate) {
  if (MASTER_PASSWORD_HASH.length() == 0) return false;
  
  String candHash = sha256Hex(candidate);
  
  if (candHash.length() != MASTER_PASSWORD_HASH.length()) return false;
  
  // Constant-time comparison to prevent timing attacks
  volatile uint8_t diff = 0;
  for (size_t i = 0; i < MASTER_PASSWORD_HASH.length(); i++) {
    diff |= (candHash[i] ^ MASTER_PASSWORD_HASH[i]);
  }
  
  return diff == 0;
}

bool loadCredentials() {
  prefs.begin("secure", false);

  ssid = prefs.getString("ssid", "");
  password = prefs.getString("password", "");
  BOTtoken = prefs.getString("token", "");
  MASTER_PASSWORD_HASH = prefs.getString("master_pw_hash", "");
  MASTER_OTA_PASSWORD = prefs.getString("ota_pw", "");

  bool hasCredentials = (ssid.length() > 0 && password.length() > 0 &&
                         BOTtoken.length() > 0 && MASTER_PASSWORD_HASH.length() > 0 &&
                         MASTER_OTA_PASSWORD.length() > 0);

  prefs.end();

  if (!hasCredentials) {
    Serial.println("⚠️  No credentials found");
    return false;
  }

  Serial.println("✓ Credentials loaded from secure storage");
  return true;
}

void saveCredentials() {
  prefs.begin("secure", false);
  prefs.putString("ssid", ssid);
  prefs.putString("password", password);
  prefs.putString("token", BOTtoken);
  prefs.putString("master_pw_hash", MASTER_PASSWORD_HASH);
  prefs.putString("ota_pw", MASTER_OTA_PASSWORD);
  prefs.end();
  
  Serial.println("✓ Credentials saved to secure storage");
}

void provisionDevice() {
  Serial.println("\n═══════════════════════════════════════");
  Serial.println("    DEVICE PROVISIONING MODE");
  Serial.println("═══════════════════════════════════════");
  Serial.println("Enter credentials via Serial Monitor (115200 baud):");
  Serial.println();

  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("PROVISIONING");
  display.println("MODE");
  display.println();
  display.println("Use Serial");
  display.println("Monitor to");
  display.println("configure");
  display.display();

  Serial.print("WiFi SSID: ");
  while(!Serial.available()) delay(100);
  ssid = Serial.readStringUntil('\n');
  ssid.trim();
  Serial.println(ssid);

  Serial.print("WiFi Password: ");
  while(!Serial.available()) delay(100);
  password = Serial.readStringUntil('\n');
  password.trim();
  Serial.println("********");

  Serial.print("Telegram Bot Token: ");
  while(!Serial.available()) delay(100);
  BOTtoken = Serial.readStringUntil('\n');
  BOTtoken.trim();
  Serial.println("TOKEN SET");

  Serial.print("Master Password (will be hashed): ");
  while(!Serial.available()) delay(100);
  String master = Serial.readStringUntil('\n');
  master.trim();
  MASTER_PASSWORD_HASH = sha256Hex(master);
  
  Serial.print("OTA Password (used for ArduinoOTA): ");
  while(!Serial.available()) delay(100);
  MASTER_OTA_PASSWORD = Serial.readStringUntil('\n');
  MASTER_OTA_PASSWORD.trim();
  Serial.println("********");

  saveCredentials();

  master = "";

  Serial.println();
  Serial.println("✓ Provisioning complete!");
  Serial.println("✓ Credentials stored (master password hashed)");
  Serial.println("✓ Rebooting in 3 seconds...");
  delay(3000);
  ESP.restart();
}

// FIND existing temp login (don't create new ones!)
TempLoginState* getTempLogin(String chat_id) {
  for (int i = 0; i < 5; i++) {
    if (tempLogins[i].chat_id == chat_id && tempLogins[i].awaitingPassword) {
      return &tempLogins[i];
    }
  }
  return nullptr;  // Not found - don't create!
}

// CREATE a new temp login (only called when user sends /login)
TempLoginState* createTempLogin(String chat_id) {
  for (int i = 0; i < 5; i++) {
    if (!tempLogins[i].awaitingPassword) {
      tempLogins[i].chat_id = chat_id;
      tempLogins[i].awaitingPassword = true;
      tempLogins[i].timestamp = millis();
      return &tempLogins[i];
    }
  }
  return nullptr;  // All slots full
}

void clearTempLogin(String chat_id) {
  for (int i = 0; i < 5; i++) {
    if (tempLogins[i].chat_id == chat_id) {
      tempLogins[i].awaitingPassword = false;
      tempLogins[i].chat_id = "";
      tempLogins[i].timestamp = 0;
    }
  }
}

void cleanupTempLogins() {
  static unsigned long lastCleanup = 0;
  if (millis() - lastCleanup < 60000) return;
  lastCleanup = millis();

  for (int i = 0; i < 5; i++) {
    if (tempLogins[i].awaitingPassword &&
        millis() - tempLogins[i].timestamp > TEMP_LOGIN_TIMEOUT) {
      Serial.printf("Temp login timeout: %s\n", tempLogins[i].chat_id.c_str());
      tempLogins[i].awaitingPassword = false;
      tempLogins[i].chat_id = "";
    }
  }
}

void setupOTA() {
  ArduinoOTA.setPassword(MASTER_OTA_PASSWORD.c_str());
  ArduinoOTA.setHostname("doorbell-cam");

  ArduinoOTA.onStart([]() {
    Serial.println("OTA Start");
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("OTA UPDATE");
    display.println("Please wait...");
    display.display();
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    unsigned int percent = progress / (total / 100);
    Serial.printf("Progress: %u%%\r", percent);

    static unsigned int lastPercent = 0;
    if (percent != lastPercent && percent % 10 == 0) {
      display.clearDisplay();
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.printf("OTA UPDATE\n\n%d%%", percent);
      display.display();
      lastPercent = percent;
    }
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("\nOTA Complete");
    display.clearDisplay();
    display.println("Update OK!");
    display.println("Rebooting...");
    display.display();
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("OTA Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });

  ArduinoOTA.begin();
  Serial.println("✓ OTA updates enabled");
}

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  Serial.begin(115200);
  Serial.println("\n═══════════════════════════════════════");
  Serial.println("  ESP32-CAM Smart Doorbell ");
  Serial.println("═══════════════════════════════════════\n");

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  

  ledcSetup(8, 2000, 8);
  ledcAttachPin(BUZZER_PIN, 8);

  ledcSetup(7, 5000, 8);
  ledcAttachPin(FLASH_LED_PIN, 7);
  ledcWrite(7, 0);


  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(400000);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3D)) {
      Serial.println(F("SSD1306 init failed"));
    }
  }

  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(34, 20);
  display.println("SMART");
  display.setCursor(20, 35);
  display.println("SECURE");
  display.display();
  delay(2000);

  if (!loadCredentials()) {
    provisionDevice();
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Booting System...");
  display.display();

  delay(500);

  if (!initCamera()) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Cam Fail!");
    display.println("Check:");
    display.println("-5V/1A power");
    display.println("-Camera cable");
    display.display();
    while (1) delay(1000);
  }

  display.println("WiFi...");
  display.display();
  if (!connectWiFi()) {
    display.println("WiFi Fail");
    display.display();
  }

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  // Seed the cached clock string immediately after NTP sync
  {
    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 5000)) {
      char buf[6];
      strftime(buf, sizeof(buf), "%H:%M", &timeinfo);
      cachedTimeStr = String(buf);
    }
  }

  // TLS for ESP32 core 1.0.6
  clientTCP.setInsecure();
  clientTCP.setTimeout(20000);
  Serial.println("✓ TLS configured (ESP32 1.0.6)");

  bot = new UniversalTelegramBot(BOTtoken, clientTCP);

  startCameraServer();

  prefs.begin("doorbell", false);
  intruderCount = prefs.getInt("intruders", 0);

  if (bot) {
    int numMessages = bot->getUpdates(bot->last_message_received + 1);
    if (numMessages > 0) {
      Serial.printf("Skipping %d offline messages\n", numMessages);
    }
  }

  setupOTA();

  tone(BUZZER_PIN, 1000, 200);
  updateDisplay();

  Serial.println("\n✓ System ready");
  Serial.printf("✓ Free heap: %d bytes\n", ESP.getFreeHeap());
  Serial.printf("✓ IP Address: %s\n", WiFi.localIP().toString().c_str());
}

bool connectWiFi() {
  WiFi.mode(WIFI_STA);

  // Using DHCP for phone hotspot
  if (wifiCache.valid) {
    Serial.println("Fast reconnect: cached BSSID/channel (DHCP)");
    
    WiFi.begin(ssid.c_str(), password.c_str(), wifiCache.channel, wifiCache.bssid, true);

    unsigned long startTime = millis();
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 6) {
      delay(500);
      attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.printf("✓ Fast connect: %lums (%s)\n", millis() - startTime, WiFi.localIP().toString().c_str());
      return true;
    }

    Serial.println("Fast reconnect failed, full scan...");
    WiFi.disconnect();
    wifiCache.valid = false;
  }

  // Full scan with DHCP
  Serial.println("Full WiFi scan (DHCP)...");
  WiFi.begin(ssid.c_str(), password.c_str());

  unsigned long startTime = millis();
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("✓ Full connect: %lums (%s)\n", millis() - startTime, WiFi.localIP().toString().c_str());

    uint8_t* bssid = WiFi.BSSID();
    memcpy(wifiCache.bssid, bssid, 6);
    wifiCache.channel = WiFi.channel();
    wifiCache.valid = true;

    Serial.printf("✓ Cached BSSID: %02X:%02X:%02X:%02X:%02X:%02X, Ch: %d\n",
                  wifiCache.bssid[0], wifiCache.bssid[1], wifiCache.bssid[2],
                  wifiCache.bssid[3], wifiCache.bssid[4], wifiCache.bssid[5],
                  wifiCache.channel);

    return true;
  }

  return false;
}

bool initCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 10000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if (psramFound()) {
    Serial.println("PSRAM found");
    config.frame_size = FRAMESIZE_240X240;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    Serial.println("PSRAM not found");
    config.frame_size = FRAMESIZE_240X240;
    config.jpeg_quality = 15;
    config.fb_count = 1;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed: 0x%x\n", err);
    return false;
  }

  sensor_t * s = esp_camera_sensor_get();
  if (s == NULL) {
    Serial.println("Failed to get sensor");
    return false;
  }

  s->set_brightness(s, 0);
  s->set_contrast(s, 0);
  s->set_saturation(s, 0);
  s->set_whitebal(s, 1);
  s->set_awb_gain(s, 1);
  s->set_exposure_ctrl(s, 1);
  s->set_aec2(s, 0);
  s->set_gain_ctrl(s, 1);
  s->set_bpc(s, 0);
  s->set_wpc(s, 1);
  s->set_raw_gma(s, 1);
  s->set_lenc(s, 1);
  s->set_hmirror(s, 0);
  s->set_vflip(s, 0);
  s->set_dcw(s, 1);

  Serial.println("Warming up camera...");
  for (int i = 0; i < 3; i++) {
    camera_fb_t * fb = esp_camera_fb_get();
    if (fb) {
      esp_camera_fb_return(fb);
      delay(100);
    }
  }

  Serial.println("✓ Camera ready");
  return true;
}

void loop() {
  ArduinoOTA.handle();

  if (!bot) return;

  if (millis() - lastTelegramCheck > BOT_MTBS) {
    int numNewMessages = bot->getUpdates(bot->last_message_received + 1);
    while (numNewMessages) {
      handleNewMessages(numNewMessages);
      handleCallbackQuery(numNewMessages);
      numNewMessages = bot->getUpdates(bot->last_message_received + 1);
    }
    lastTelegramCheck = millis();
  }

  checkSessionTimeouts();
  cleanupTempLogins();
  wifiReconnectIfNeeded();

  if (currentState == STANDBY && digitalRead(BUTTON_PIN) == LOW) {
    if (millis() - lastButtonPress > DEBOUNCE_DELAY) {
      currentState = SCANNING;
      ledcWrite(7, 200);
      scanStartTime = millis();
      matchFace = false;
      tone(BUZZER_PIN, 2000, 50);
      lastButtonPress = millis();
      displayNeedsUpdate = true;
    }
  }

  switch (currentState) {
    case SCANNING: { 
      
      // 1. Actively grab a frame and run the AI math
      int face_id = perform_standalone_ai_scan();

      if (face_id == -3){
        // enrollment in progress - a frame was captured here 
        tone(BUZZER_PIN, 2000, 100);
        delay(400); // change facial angle
        break; // keep scanning
      }
      else if (face_id == 2) {
        // enrollment completely finished !
        Serial.println("Face Enrollment Completed via Telegram !");
        currentState = STANDBY;
        displayNeedsUpdate = true;
        tone(BUZZER_PIN, 1000, 200); delay(200); tone(BUZZER_PIN, 1500, 300);

        notifyAllAdmins("✅ Face Enrollment Successful! New Face ID saved.", false);
        break;    
      }
      
      // 2. If it returns 0 or higher, it successfully recognized an enrolled face
      if (face_id >= 0) {
        matchFace = true; 
      }

      // 3. Unlocking logic
      if (matchFace) {
        ledcWrite(7, 0);
        Serial.printf("Face Matched! ID: %d. Unlocking...\n", face_id);
        currentState = UNLOCKED;
        unlockStartTime = millis();
        digitalWrite(RELAY_PIN, LOW); // (Note to change to LOW later if you use the NPN transistor not gate)
        tone(BUZZER_PIN, 1500, 100);
        displayNeedsUpdate = true;

        struct tm timeinfo;
        if (getLocalTime(&timeinfo)) {
          char timeStr[6];
          strftime(timeStr, sizeof(timeStr), "%H:%M", &timeinfo);
          lastEntryTime = String(timeStr);
        }
        lastUnlockedBy = "Face ID " + String(face_id); 
        notifyAllAdmins("🏠 Door unlocked by Face Recognition at " + lastEntryTime, false);
        sendTelegramStatusPin(lastUnlockedBy, lastEntryTime);
        // Trigger one-shot face→tick animation before WELCOME screen
        faceAnimFrame   = 0;
        faceAnimPlaying = true;
      } 
      
      
      else if (millis() - scanStartTime > SCAN_TIMEOUT) {
        ledcWrite(7, 0);
        Serial.println("Scan Timeout! Triggering Alarm.");
        currentState = ALARM;
        alarmStartTime = millis();
        intruderCount++;
        prefs.putInt("intruders", intruderCount);
        tone(BUZZER_PIN, 500, 1000);
        displayNeedsUpdate = true;

        String keyboardJson = "[[{\"text\":\"✅ Unlock\",\"callback_data\":\"unlock\"},{\"text\":\"❌ Dismiss\",\"callback_data\":\"dismiss\"}],[{\"text\":\"📷 View Photo\",\"callback_data\":\"photo\"},{\"text\":\"🚨 Sound Alarm\",\"callback_data\":\"alarm\"}]]";
        String intruderCaption = "🚨 [INTRUDER ALERT] 🚨\n\nUnknown person at door!\n\nSelect Action:";
        
        for (int i = 0; i < NUM_USERS; i++) {
          if (authorizedUsers[i].accessLevel == 0) { // 0 = Admin
            sendPhotoWithKeyboard(intruderCaption, authorizedUsers[i].chat_id, keyboardJson);
          }
        }
      }
      
      break; 
    } // <-- End of SCANNING case block

    case UNLOCKED:
      if (millis() - unlockStartTime > UNLOCK_TIME) {
        digitalWrite(RELAY_PIN, HIGH);
        currentState = STANDBY;
        matchFace = false;
        displayNeedsUpdate = true;
      }
      break;

    case ALARM:
      if (millis() - alarmStartTime > ALARM_DISPLAY_TIME) {
        ledcWrite(7, 0); // ensure flash LED is off
        currentState = STANDBY;
        displayNeedsUpdate = true;
      }
      break;

    case SIREN:
      if (millis() - lastSirenBeep > SIREN_BEEP_INTERVAL) {
        sirenState = !sirenState;
        ledcWriteTone(8, sirenState ? 800 : 400);
        lastSirenBeep = millis();
      }
      // Only refresh the OLED at SIREN_DISPLAY_INTERVAL, not every buzzer toggle
      if (millis() - lastSirenDisplayUpdate > SIREN_DISPLAY_INTERVAL) {
        displayNeedsUpdate = true;
        lastSirenDisplayUpdate = millis();
      }
      break;

    case STANDBY:
      break;
  }

  if (displayNeedsUpdate || (millis() - lastClockUpdate > CLOCK_REFRESH_MS)) {
    if (millis() - lastClockUpdate > CLOCK_REFRESH_MS) {
      // Only call getLocalTime on the slow clock tick
      struct tm timeinfo;
      if (getLocalTime(&timeinfo, 10)) { // 10ms timeout max
        char buf[6];
        strftime(buf, sizeof(buf), "%H:%M", &timeinfo);
        cachedTimeStr = String(buf);
      }
      lastClockUpdate = millis();
    }
    updateDisplay();
    displayNeedsUpdate = false;
  }
}

uint8_t* telegramImageBuffer = nullptr;
size_t telegramImageSize = 0;
size_t telegramImageIndex = 0;

bool isMoreDataAvailable() {
  return (telegramImageIndex < telegramImageSize);
}

byte getNextByte() {
  if (telegramImageIndex < telegramImageSize) return telegramImageBuffer[telegramImageIndex++];
  return 0;
}
//Update wrapper here !!!
String sendPhoto(const String &caption, String target_chat_id) {
  camera_fb_t * fb = NULL;

  // 1. Capture the photo (Keeping your great 3-retry logic)
  for (int attempt = 0; attempt < 3; attempt++) {
    fb = esp_camera_fb_get();

    if (fb) {
      if (fb->len > 100 && fb->format == PIXFORMAT_JPEG) {
        Serial.printf("Photo OK: %d bytes\n", fb->len);
        break;
      } else {
        Serial.printf("Invalid frame, retry %d\n", attempt + 1);
        esp_camera_fb_return(fb);
        fb = NULL;
        delay(200);
      }
    } else {
      Serial.printf("Capture fail, retry %d\n", attempt + 1);
      delay(200);
    }
  }

  if (!fb) {
    Serial.println("❌ Photo capture failed!");
    return "Failed";
  }

  // 2. Direct Upload bypassing UniversalTelegramBot
  Serial.println("Connecting to Telegram directly...");
  WiFiClientSecure client_tcp;
  client_tcp.setInsecure(); // Skip certificate check to save RAM and speed up connection

  String result = "";

  if (client_tcp.connect("api.telegram.org", 443)) {
    Serial.println("Connection successful, uploading chunked image...");
    
    // ⚠️ IMPORTANT: Ensure "BOT_TOKEN" matches the variable name at the top of your sketch!
    String url = "/bot" + String(BOTtoken) + "/sendPhoto"; 
    
    // Build the multipart form data payload
    String head = "--ESP32Boundary\r\nContent-Disposition: form-data; name=\"chat_id\"\r\n\r\n" + target_chat_id + "\r\n";
    
    // Add the caption if one was provided
    if (caption.length() > 0) {
      head += "--ESP32Boundary\r\nContent-Disposition: form-data; name=\"caption\"\r\n\r\n" + caption + "\r\n";
    }
    
    head += "--ESP32Boundary\r\nContent-Disposition: form-data; name=\"photo\"; filename=\"esp32-cam.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
    String tail = "\r\n--ESP32Boundary--\r\n";

    uint32_t imageLen = fb->len;
    uint32_t totalLen = head.length() + imageLen + tail.length();

    // Send HTTP POST Headers
    client_tcp.println("POST " + url + " HTTP/1.1");
    client_tcp.println("Host: api.telegram.org");
    client_tcp.println("Content-Length: " + String(totalLen));
    client_tcp.println("Content-Type: multipart/form-data; boundary=ESP32Boundary");
    client_tcp.println();
    client_tcp.print(head);

    // Write the image payload in 1024-byte chunks
    uint8_t *fbBuf = fb->buf;
    size_t fbLen = fb->len;
    size_t n = 0;
    while (n < fbLen) {
      size_t toWrite = ((n + 1024) <= fbLen) ? 1024 : (fbLen - n);
      client_tcp.write(fbBuf + n, toWrite);
      n += toWrite;
    }
    client_tcp.print(tail);
    
    Serial.println("✓ Photo chunks sent! Waiting for Telegram response...");
    
    // Read the response from Telegram to prevent broken pipes
    long startTimer = millis();
    while (client_tcp.connected() && millis() - startTimer < 5000) {
      while (client_tcp.available()) {
        char c = client_tcp.read();
        result += c;
        startTimer = millis(); 
      }
    }
    client_tcp.stop();
    Serial.printf("✓ Photo successfully delivered to %s\n", target_chat_id.c_str());
    
  } else {
    Serial.println("⚠️ Connection to Telegram failed!");
    result = "Failed";
  }
  
  // 3. Free camera memory
  esp_camera_fb_return(fb); 
  
  return result;
}
String sendPhotoWithKeyboard(const String &caption, String target_chat_id, String keyboardJson) {
  camera_fb_t * fb = NULL;

  // 1. Capture the photo (Keeping your great 3-retry logic)
  for (int attempt = 0; attempt < 3; attempt++) {
    fb = esp_camera_fb_get();

    if (fb) {
      if (fb->len > 100 && fb->format == PIXFORMAT_JPEG) {
        Serial.printf("Photo OK: %d bytes\n", fb->len);
        break;
      } else {
        Serial.printf("Invalid frame, retry %d\n", attempt + 1);
        esp_camera_fb_return(fb);
        fb = NULL;
        delay(200);
      }
    } else {
      Serial.printf("Capture fail, retry %d\n", attempt + 1);
      delay(200);
    }
  }

  if (!fb) {
    Serial.println("❌ Photo capture failed!");
    return "Failed";
  }

  // 2. Direct Upload bypassing UniversalTelegramBot
  Serial.println("Connecting to Telegram directly...");
  WiFiClientSecure client_tcp;
  client_tcp.setInsecure(); // Skip certificate check to save RAM and speed up connection

  String result = "";

  if (client_tcp.connect("api.telegram.org", 443)) {
    Serial.println("Connection successful, uploading chunked image with keyboard...");
    
    // ⚠️ IMPORTANT: Ensure "BOT_TOKEN" matches the variable name at the top of your sketch!
    String url = "/bot" + String(BOTtoken) + "/sendPhoto"; 
    
    // Build the multipart form data payload
    String head = "--ESP32Boundary\r\nContent-Disposition: form-data; name=\"chat_id\"\r\n\r\n" + target_chat_id + "\r\n";
    
    // Add the caption if one was provided
    if (caption.length() > 0) {
      head += "--ESP32Boundary\r\nContent-Disposition: form-data; name=\"caption\"\r\n\r\n" + caption + "\r\n";
    }

    // 👇 ADDED: Inject the Inline Keyboard into the Photo Payload
    if (keyboardJson.length() > 0) {
      // Telegram requires the array to be wrapped inside an "inline_keyboard" object
      String replyMarkup = "{\"inline_keyboard\":" + keyboardJson + "}";
      head += "--ESP32Boundary\r\nContent-Disposition: form-data; name=\"reply_markup\"\r\n\r\n" + replyMarkup + "\r\n";
    }
    
    head += "--ESP32Boundary\r\nContent-Disposition: form-data; name=\"photo\"; filename=\"esp32-cam.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
    String tail = "\r\n--ESP32Boundary--\r\n";

    uint32_t imageLen = fb->len;
    uint32_t totalLen = head.length() + imageLen + tail.length();

    // Send HTTP POST Headers
    client_tcp.println("POST " + url + " HTTP/1.1");
    client_tcp.println("Host: api.telegram.org");
    client_tcp.println("Content-Length: " + String(totalLen));
    client_tcp.println("Content-Type: multipart/form-data; boundary=ESP32Boundary");
    client_tcp.println();
    client_tcp.print(head);

    // Write the image payload in 1024-byte chunks
    uint8_t *fbBuf = fb->buf;
    size_t fbLen = fb->len;
    size_t n = 0;
    while (n < fbLen) {
      size_t toWrite = ((n + 1024) <= fbLen) ? 1024 : (fbLen - n);
      client_tcp.write(fbBuf + n, toWrite);
      n += toWrite;
    }
    client_tcp.print(tail);
    
    Serial.println("✓ Photo chunks sent! Waiting for Telegram response...");
    
    // Read the response from Telegram to prevent broken pipes
    long startTimer = millis();
    while (client_tcp.connected() && millis() - startTimer < 5000) {
      while (client_tcp.available()) {
        char c = client_tcp.read();
        result += c;
        startTimer = millis(); 
      }
    }
    client_tcp.stop();
    Serial.printf("✓ Photo successfully delivered to %s\n", target_chat_id.c_str());
    
  } else {
    Serial.println("⚠️ Connection to Telegram failed!");
    result = "Failed";
  }
  
  // 3. Free camera memory
  esp_camera_fb_return(fb); 
  
  return result;
}

void updateDisplay() {
  display.clearDisplay();
  display.setTextColor(WHITE);
  drawHeader();

  if (currentState == STANDBY) {
    if (customMessage != "") {
      display.setTextSize(1);
      display.setCursor(0, 22);
      display.println("MESSAGE:");
      display.println(customMessage);
    } else {
      display.drawBitmap(56, 24, icon_lock, 16, 16, WHITE);
      display.setTextSize(2);
      display.setCursor(28, 44);
      display.println("LOCKED");
    }
  }
  else if (currentState == SCANNING) {
    display.drawBitmap(56, 24, icon_scan, 16, 16, WHITE);
    display.setTextSize(2);
    display.setCursor(15, 44);
    display.println("SCANNING");
  }
  else if (currentState == UNLOCKED) {
    // Play face→tick animation once on face-match unlock, then show WELCOME
    if (faceAnimPlaying) {
      if (millis() - lastFaceAnimTime > FACE_ANIM_FRAME_DELAY) {
        display.clearDisplay();
        drawHeader();
        display.drawBitmap(48, 16, faceAnimFrames[faceAnimFrame], FACE_ANIM_W, FACE_ANIM_H, WHITE);
        display.display();
        lastFaceAnimTime = millis();
        faceAnimFrame++;
        if (faceAnimFrame >= FACE_ANIM_FRAME_COUNT) {
          faceAnimPlaying = false;  // animation done, fall through to WELCOME
          displayNeedsUpdate = true;
        }
      }
      return; // don't overwrite display while animating
    }
    display.drawBitmap(8, 24, icon_unlock, 16, 16, WHITE);
    display.setTextSize(2);
    display.setCursor(30, 28);
    display.println("WELCOME");

    int timeOpen = millis() - unlockStartTime;
    int remaining = UNLOCK_TIME - timeOpen;
    int barWidth = map(remaining, 0, UNLOCK_TIME, 0, 120);
    display.fillRect(4, 60, barWidth, 4, WHITE);
    display.drawRect(4, 60, 120, 4, WHITE);
  }
  else if (currentState == ALARM) {
    display.drawBitmap(56, 22, icon_alert, 16, 16, WHITE);
    display.setTextSize(2);
    display.setCursor(20, 42);
    display.println("DENIED");
  }
  else if (currentState == SIREN) {
    if (sirenState) {
      display.fillRect(0, 20, 128, 44, WHITE);
      display.setTextColor(BLACK);
    } else {
      display.setTextColor(WHITE);
    }
    display.drawBitmap(56, 22, icon_alert, 16, 16, sirenState ? BLACK : WHITE);
    display.setTextSize(2);
    display.setCursor(20, 42);
    display.println("LOCK-DOWN!");
  }

  display.display();
}

void drawHeader() {
  display.drawBitmap(0, 0, icon_clock, 16, 16, WHITE);
  display.setTextSize(1);
  display.setCursor(18, 4);
  display.print(cachedTimeStr); // use pre-fetched time, avoids blocking getLocalTime on every frame

  long rssi = WiFi.RSSI();
  int bars = 0;
  if (rssi > -50) bars = 4;
  else if (rssi > -60) bars = 3;
  else if (rssi > -70) bars = 2;
  else if (rssi > -85) bars = 1;

  for (int i = 0; i < bars; i++) {
    display.fillRect(108 + (i*4), 12 - (i*3), 3, 3 + (i*3), WHITE);
  }
  display.drawFastHLine(0, 17, 128, WHITE);
}

bool isUserAuthorized(String chat_id) {
  for (int i = 0; i < NUM_USERS; i++)
    if (authorizedUsers[i].chat_id == chat_id) return true;
  return false;
}

int getUserAccessLevel(String chat_id) {
  for (int i = 0; i < NUM_USERS; i++)
    if (authorizedUsers[i].chat_id == chat_id) return authorizedUsers[i].accessLevel;
  return -1;
}

String getUserName(String chat_id) {
  for (int i = 0; i < NUM_USERS; i++)
    if (authorizedUsers[i].chat_id == chat_id) return authorizedUsers[i].name;
  return "Unknown";
}

UserSession* getSession(String chat_id) {
  for (int i = 0; i < MAX_SESSIONS; i++) {
    if (activeSessions[i].chat_id == chat_id && activeSessions[i].isLoggedIn)
      return &activeSessions[i];
  }
  return nullptr;
}

bool isLoggedIn(String chat_id) {
  return (getSession(chat_id) != nullptr);
}

void createSession(String chat_id, int accessLevel, String userName) {
  int slot = -1;
  for (int i = 0; i < MAX_SESSIONS; i++) {
    if (!activeSessions[i].isLoggedIn) {
      slot = i;
      break;
    }
  }
  if (slot == -1) slot = 0;

  activeSessions[slot].chat_id = chat_id;
  activeSessions[slot].isLoggedIn = true;
  activeSessions[slot].loginTime = millis();
  activeSessions[slot].accessLevel = accessLevel;
  activeSessions[slot].userName = userName;
  activeSessions[slot].lastCommandTime = 0;
  activeSessions[slot].awaitingMessage = false;
}

void clearSession(String chat_id) {
  for (int i = 0; i < MAX_SESSIONS; i++) {
    if (activeSessions[i].chat_id == chat_id) {
      activeSessions[i].isLoggedIn = false;
      activeSessions[i].chat_id = "";
      activeSessions[i].awaitingMessage = false;
    }
  }
}

void checkSessionTimeouts() {
  static unsigned long lastCheck = 0;
  if (millis() - lastCheck < 60000) return;
  lastCheck = millis();

  for (int i = 0; i < MAX_SESSIONS; i++) {
    if (activeSessions[i].isLoggedIn &&
        activeSessions[i].accessLevel > 0 &&
        millis() - activeSessions[i].loginTime > SESSION_TIMEOUT) {
      bot->sendMessage(activeSessions[i].chat_id, "⏰ Session expired.", "");
      activeSessions[i].isLoggedIn = false;
    }
  }
}

bool checkCommandCooldown(String chat_id, bool isPhotoCommand) {
  UserSession* session = getSession(chat_id);
  if (!session) {
    Serial.printf("⚠️  No session for %s\n", chat_id.c_str());
    bot->sendMessage(chat_id, "🔐 Please login first: /login <password>", "");
    return false;
  }

  unsigned long cooldown = isPhotoCommand ? PHOTO_COOLDOWN : COMMAND_COOLDOWN;
  unsigned long timeSince = millis() - session->lastCommandTime;

  if (timeSince < cooldown) {
    unsigned long wait = (cooldown - timeSince) / 1000;
    bot->sendMessage(chat_id, "⏱️ Wait " + String(wait + 1) + "s", "");
    return false;
  }

  session->lastCommandTime = millis();
  return true;
}

String getStateString() {
  switch (currentState) {
    case STANDBY: return "Standby";
    case SCANNING: return "Scanning";
    case UNLOCKED: return "Unlocked";
    case ALARM: return "Alarm";
    case SIREN: return "Siren";
    default: return "Unknown";
  }
}

void sendStatusMessage(String chat_id) {
  long rssi = WiFi.RSSI();
  String status = "📊 System Status:\n";
  status += "━━━━━━━━━━━━━━\n";
  status += "WiFi: " + String(rssi) + " dBm\n";
  status += "State: " + getStateString() + "\n";
  status += "Alerts: " + String(intruderCount) + "\n";
  status += "Last Entry: " + lastEntryTime + "\n";

  bot->sendMessage(chat_id, status, "");
}

void sendHelpMessage(String chat_id, int accessLevel) {
  String help = "🤖 Available Commands:\n";
  help += "━━━━━━━━━━━━━━\n";
  help += "/start - Show main menu\n";
  help += "/open - Unlock door\n";
  help += "/photo - Take photo\n";
  help += "/status - System info\n";

  if (accessLevel == 0) {
    help += "/setmsg - Set OLED message\n";
    help += "/clear - Clear screen\n";
    help += "/enroll - Add new face\n";
    help += "/deletefaces - Delete all faces\n";
  }

  help += "/help - Show this help";

  bot->sendMessage(chat_id, help, "");
}

void sendMainMenu(String chat_id, String message, int accessLevel) {
  String keyboardJson;
  if (accessLevel == 0) {
    if (customMessage != "") {
      keyboardJson = "[[\"🔓 Unlock\",\"📷 Photo\"],[\"📊 Status\",\"👥 Users\"],[\"🧹 Clear Screen\",\"📝 Set Message\"],[\"🚨 Sound Alarm\"]]";
    } else {
      keyboardJson = "[[\"🔓 Unlock\",\"📷 Photo\"],[\"📊 Status\",\"👥 Users\"],[\"📝 Set Message\",\"➕ Enroll\"],[\"🚨 Sound Alarm\"]]";
    }
  }
  else if (accessLevel == 1) {
    keyboardJson = "[[\"🔓 Unlock\",\"📷 Photo\"],[\"📊 Status\",\"🚪 Logout\"]]";
  }
  else {
    keyboardJson = "[[\"📷 Photo\",\"📊 Status\"],[\"🚪 Logout\"]]";
  }
  bot->sendMessageWithReplyKeyboard(chat_id, message, "", keyboardJson, true);
}

void sendActionRequired(String chat_id, String message) {
  String keyboardJson = "[[{\"text\":\"✅ Unlock\",\"callback_data\":\"unlock\"},{\"text\":\"❌ Dismiss\",\"callback_data\":\"dismiss\"}],[{\"text\":\"📷 View Photo\",\"callback_data\":\"photo\"},{\"text\":\"🚨 Sound Alarm\",\"callback_data\":\"alarm\"}]]";
  bot->sendMessageWithInlineKeyboard(chat_id, message, "", keyboardJson);
}

void sendSirenControl(String chat_id, String message) {
  String keyboardJson = "[[{\"text\":\"🛑 STOP ALARM\",\"callback_data\":\"stop_alarm\"}],[{\"text\":\"📷 View Photo\",\"callback_data\":\"photo\"},{\"text\":\"📊 Status\",\"callback_data\":\"status\"}]]";
  bot->sendMessageWithInlineKeyboard(chat_id, message, "", keyboardJson);
}

void sendLoginPrompt(String chat_id) {
  bot->sendMessage(chat_id, "🔐 Login: /login <password>", "");
}

void removeInlineKeyboard(String chat_id, String message_id) {
  WiFiClientSecure client_tcp;
  client_tcp.setInsecure(); // Skip certificate validation for speed/RAM
  if (client_tcp.connect("api.telegram.org", 443)) {
    // We call the specific API endpoint that ONLY removes the keyboard
    String url = "/bot" + String(BOTtoken) + "/editMessageReplyMarkup";
    String payload = "{\"chat_id\":\"" + chat_id + "\",\"message_id\":" + message_id + ",\"reply_markup\":{\"inline_keyboard\":[]}}";
    
    client_tcp.println("POST " + url + " HTTP/1.1");
    client_tcp.println("Host: api.telegram.org");
    client_tcp.println("Content-Type: application/json");
    client_tcp.println("Content-Length: " + String(payload.length()));
    client_tcp.println();
    client_tcp.println(payload);
    
    long startTimer = millis();
    while (client_tcp.connected() && millis() - startTimer < 2000) {
      while (client_tcp.available()) client_tcp.read(); // Drain response
    }
    client_tcp.stop();
  }
}

void handleCallbackQuery(int numNewMessages) {
  for (int i = 0; i < numNewMessages; i++) {
    if (bot->messages[i].type != "callback_query") continue;

    String chat_id = bot->messages[i].chat_id;
    String callback_data = bot->messages[i].text;
    String query_id = bot->messages[i].query_id;
    int message_id = bot->messages[i].message_id; 
    String msgIdStr = String(message_id);

    if (!isUserAuthorized(chat_id)) {
      bot->answerCallbackQuery(query_id, "⛔ Unauthorized");
      continue;
    }

    String userName = getUserName(chat_id);
    int accessLevel = getUserAccessLevel(chat_id);

    // Admin auto-login for inline keyboard actions
    if (accessLevel == 0 && !isLoggedIn(chat_id)) {
      createSession(chat_id, 0, userName);
    }

    if (!isLoggedIn(chat_id) && accessLevel > 0) {
      bot->answerCallbackQuery(query_id, "🔐 Please login first");
      continue;
    }

    bool isPhoto = (callback_data == "photo");
    if (!checkCommandCooldown(chat_id, isPhoto)) {
      bot->answerCallbackQuery(query_id, "⏱️ Please wait");
      continue;
    }

    // === 🔓 UNLOCK ACTION ===
    if (callback_data == "unlock") {
      if (accessLevel > 1) {
        bot->answerCallbackQuery(query_id, "⛔ Access Denied");
        continue;
      }
      if (currentState == SIREN) {
        bot->answerCallbackQuery(query_id, "⚠️ Stop alarm first");
        continue;
      }

      currentState = UNLOCKED;
      matchFace = true;
      unlockStartTime = millis();
      digitalWrite(RELAY_PIN, LOW);
      lastUnlockedBy = userName;
      displayNeedsUpdate = true;

      struct tm timeinfo;
      if (getLocalTime(&timeinfo)) {
        char timeStr[6];
        strftime(timeStr, sizeof(timeStr), "%H:%M", &timeinfo);
        lastEntryTime = String(timeStr);
      }

      bot->answerCallbackQuery(query_id, "🔓 Door unlocked");
      
      // 👇 FIX: Removes keyboard from photo, sends clean text confirmation
      removeInlineKeyboard(chat_id, msgIdStr);
      bot->sendMessage(chat_id, "✅ Action Taken: Door Unlocked by " + userName, "");
    }

    // === ❌ DISMISS ACTION ===
    else if (callback_data == "dismiss") {
      bot->answerCallbackQuery(query_id, "✅ Dismissed");
      
      // 👇 FIX
      removeInlineKeyboard(chat_id, msgIdStr);
      bot->sendMessage(chat_id, "❌ Action Taken: Alert Dismissed by " + userName, "");
    }

    // === 📷 PHOTO ACTION ===
    else if (callback_data == "photo") {
      bot->answerCallbackQuery(query_id, "📷 Taking photo...");
      
      // 👇 FIX
      removeInlineKeyboard(chat_id, msgIdStr);
      bot->sendMessage(chat_id, "📸 Photo requested by " + userName + ". Capturing now...", "");
      sendPhoto("Requested by " + userName, chat_id);
    }

    // === 🚨 ALARM ACTION ===
    else if (callback_data == "alarm") {
      if (accessLevel > 0) {
        bot->answerCallbackQuery(query_id, "⛔ Admin only");
        continue;
      }
      currentState = SIREN;
      sirenState = false;
      displayNeedsUpdate = true;
      bot->answerCallbackQuery(query_id, "🚨 Alarm activated");
      
      // 👇 FIX
      removeInlineKeyboard(chat_id, msgIdStr);
      bot->sendMessage(chat_id, "🚨 Siren activated by " + userName, "");
      sendSirenControl(chat_id, "🚨 ALARM ACTIVE");
    }

    //
    else if (callback_data == "stop_alarm") {
      currentState = STANDBY;
      ledcWriteTone(8, 0);
      displayNeedsUpdate = true;
      bot->answerCallbackQuery(query_id, "✅ Alarm stopped");
      
      
      removeInlineKeyboard(chat_id, msgIdStr);
      bot->sendMessage(chat_id, "✅ Alarm stopped by " + userName, "");
    }

    // === 📊 STATUS ACTION ===
    else if (callback_data == "status") {
      bot->answerCallbackQuery(query_id, "📊 Status");
      
      // 👇 FIX
      removeInlineKeyboard(chat_id, msgIdStr);
      bot->sendMessage(chat_id, "📊 Status requested by " + userName, "");
      sendStatusMessage(chat_id);
    }

    else {
      bot->answerCallbackQuery(query_id, "❓ Unknown action");
    }
  }
}

void handleNewMessages(int numNewMessages) {
  for (int i = 0; i < numNewMessages; i++) {
    // Skip callback queries - they're handled by handleCallbackQuery
    if (bot->messages[i].type == "callback_query") continue;
    
    String chat_id = String(bot->messages[i].chat_id);
    String text = bot->messages[i].text;
    int messageId = bot->messages[i].message_id;

    if (!isUserAuthorized(chat_id)) {
      bot->sendMessage(chat_id, "⛔ Unauthorized ID: " + chat_id, "");
      continue;
    }

    String userName = getUserName(chat_id);
    int accessLevel = getUserAccessLevel(chat_id);

    // === ADMIN AUTO-LOGIN (accessLevel 0) ===
    // Admins NEVER need to login - auto-create session
    if (accessLevel == 0 && !isLoggedIn(chat_id)) {
      createSession(chat_id, 0, userName);
      clearTempLogin(chat_id);  // Clear any temp login state
    }

    // Handle temp login password entry (for Family/Guest users)
    // Only process if user is actively in login process
    TempLoginState* tempLogin = getTempLogin(chat_id);
    if (tempLogin && tempLogin->awaitingPassword) {
      if (verifyMasterPassword(text)) {
        createSession(chat_id, accessLevel, userName);
        clearTempLogin(chat_id);
        sendMainMenu(chat_id, "✅ Welcome " + userName, accessLevel);
      } else {
        clearTempLogin(chat_id);
        bot->sendMessage(chat_id, "❌ Incorrect password", "");
      }
      continue;
    }

    UserSession* session = getSession(chat_id);

    if (session && session->awaitingMessage) {
      session->awaitingMessage = false;
      customMessage = text;
      if (customMessage.length() > 50) customMessage = customMessage.substring(0, 50);
      displayNeedsUpdate = true;
      sendMainMenu(chat_id, "✅ Message set:\n" + customMessage, accessLevel);
      continue;
    }

    if (text.startsWith("/login")) {
      if (text.startsWith("/login ") && text.length() > 7) {
        // User sent: /login password
        String pwd = text.substring(7);
        pwd.trim(); // strip trailing whitespace/newline from Telegram

        if (verifyMasterPassword(pwd)) {
          createSession(chat_id, accessLevel, userName);
          sendMainMenu(chat_id, "✅ Welcome " + userName, accessLevel);
        } else {
          bot->sendMessage(chat_id, "❌ Incorrect password", "");
        }
      } else {
        // User sent just: /login (no password)
        // Create temp login state so next message will be treated as password
        TempLoginState* temp = createTempLogin(chat_id);
        if (temp) {
          bot->sendMessageWithReplyKeyboard(chat_id, "🔐 Please enter password:", "", "", true, true);
        } else {
          bot->sendMessage(chat_id, "⚠️  Too many login attempts. Try again later.", "");
        }
      }
      continue;
    }

    if (!isLoggedIn(chat_id) && text != "/start" && accessLevel > 0) {
      sendLoginPrompt(chat_id);
      continue;
    }

    if (text != "/start" && text != "/help" && text != "📊 Status" && text != "/status" && text != "/login") {
      bool isPhoto = (text == "/photo" || text == "📷 Photo" || text == "📷 View");
      if (!checkCommandCooldown(chat_id, isPhoto)) {
        continue;
      }
    }

    if (text == "/start") {
      if (!isLoggedIn(chat_id) && accessLevel > 0) {
        sendLoginPrompt(chat_id);
      } else {
        sendMainMenu(chat_id, "System Ready", accessLevel);
      }
    }

    else if (text == "/help") {
      sendHelpMessage(chat_id, accessLevel);
    }

    else if (text == "/status" || text == "📊 Status") {
      sendStatusMessage(chat_id);
    }

    else if (text == "/open" || text == "🔓 Unlock" || text == "✅ Unlock") {
      if (accessLevel > 1) {
        bot->sendMessage(chat_id, "⛔ Denied", "");
        continue;
      }
      if (currentState == SIREN) {
        bot->sendMessage(chat_id, "⚠️ Stop the alarm before unlocking.", "");
        continue;
      }
      currentState = UNLOCKED;
      matchFace = true;
      unlockStartTime = millis();
      digitalWrite(RELAY_PIN, LOW);
      lastUnlockedBy = userName;
      displayNeedsUpdate = true;

      struct tm timeinfo;
      if (getLocalTime(&timeinfo)) {
        char timeStr[6];
        strftime(timeStr, sizeof(timeStr), "%H:%M", &timeinfo);
        lastEntryTime = String(timeStr);
      }

      sendMainMenu(chat_id, "🔓 Unlocked by " + userName, accessLevel);
      sendTelegramStatusPin(lastUnlockedBy, lastEntryTime);
    }

    else if (text == "/photo" || text == "📷 Photo" || text == "📷 View") {
      bot->sendMessage(chat_id, "📷 Taking photo...", "");
      sendPhoto("Requested by " + userName, chat_id);
    }

    else if (text == "/setmsg" || text == "📝 Set Message") {
      if (accessLevel > 0) {
        bot->sendMessage(chat_id, "⛔ Admins only", "");
        continue;
      }
      UserSession* msgSession = getSession(chat_id);
      if (msgSession) {
        msgSession->awaitingMessage = true;
        bot->sendMessageWithReplyKeyboard(chat_id, "📝 Enter message for OLED display (max 50 chars):", "", "", true, true);
      }
    }

    else if (text == "/clear" || text == "🧹 Clear Screen") {
      if (accessLevel > 0) {
        bot->sendMessage(chat_id, "⛔ Admins only", "");
        continue;
      }
      customMessage = "";
      displayNeedsUpdate = true;
      sendMainMenu(chat_id, "🧹 Screen cleared", accessLevel);
    }

    
    else if (text == "/enroll" || text == "➕ Enroll") {
	    if (accessLevel > 0) {
	      bot -> sendMessage(chat_id, "⛔ Admins only","");
	      continue;
	    }
      trigger_enrollment(); // Tells the AI the next faces it sees should be saved!
      currentState = SCANNING; // Force the doorbell into scanning mode
      scanStartTime = millis(); // Reset the timer

      ledcWrite(7, 200);
      
      bot->sendMessage(chat_id, "📸 Enrollment Mode active \n1 Look directly at the camera for 20 seconds \n2 you will hear the beep as it captures each angle", "");
    }



    else if (text == "/deletefaces" || text == "🚮 Delete Faces") {
      if (accessLevel > 0) {
        bot->sendMessage(chat_id, "⛔ Admins only", "");
        continue;
      }
      deleteAllFaces();
      bot->sendMessage(chat_id, "🗑️ All faces deleted", "");
    }

    else if (text == "👥 Users") {
      if (accessLevel > 0) {
        bot->sendMessage(chat_id, "⛔ Admin only", "");
        continue;
      }

      String userList = "👥 Users:\n";
      for (int j = 0; j < NUM_USERS; j++) {
        userList += authorizedUsers[j].name + " ";
        if (authorizedUsers[j].accessLevel == 0) userList += "(Admin)";
        else if (authorizedUsers[j].accessLevel == 1) userList += "(Family)";
        else userList += "(Guest)";
        userList += isLoggedIn(authorizedUsers[j].chat_id) ? " ✅\n" : " ⭕\n";
      }
      sendMainMenu(chat_id, userList, accessLevel);
    }

    else if (text == "🚨 Sound Alarm") {
      if (accessLevel > 0) {
        bot->sendMessage(chat_id, "⛔ Admin only", "");
        continue;
      }
      currentState = SIREN;
      sirenState = false;
      displayNeedsUpdate = true;
      sendSirenControl(chat_id, "🚨 ALARM ON");
    }

    else if (text == "🚪 Logout") {
      clearSession(chat_id);
      bot->sendMessage(chat_id, "👋 Logged out", "");
    }

    else {
      bot->sendMessage(chat_id, "❓ Unknown command. Send /help for available commands.", "");
    }
  }
}

void tone(int pin, int freq, int duration) {
  ledcWriteTone(8, freq);
  delay(duration);
  ledcWriteTone(8, 0);
}

void notifyAllAdmins(String message) {
  notifyAllAdmins(message, true); // Defaults to showing the action keyboard
}

// Sends (or edits) a persistent status message to all admins.
// Uses a stored message_id to edit-in-place so it stays pinned at the top.
// Falls back to a new message if no prior message exists.
void sendTelegramStatusPin(String unlockedBy, String entryTime) {
  String text = "🔐 *Last Entry Log*\n"
                "━━━━━━━━━━━━━━\n"
                "👤 " + unlockedBy + "\n" +
                "🕐 " + entryTime + "\n" +
                "🚨 Alerts: " + String(intruderCount);

  for (int i = 0; i < NUM_USERS; i++) {
    if (authorizedUsers[i].accessLevel != 0) continue;

    String chat_id = authorizedUsers[i].chat_id;

    // Try to edit the existing pinned message first
    prefs.begin("doorbell", false);
    String keyName = "pin_" + chat_id;
    String storedMsgId = prefs.getString(keyName.c_str(), "");
    prefs.end();

    WiFiClientSecure client;
    client.setInsecure();
    if (!client.connect("api.telegram.org", 443)) continue;

    String payload, url;

    if (storedMsgId.length() > 0) {
      // Edit existing message
      url = "/bot" + BOTtoken + "/editMessageText";
      payload = "{\"chat_id\":\"" + chat_id + "\","
                "\"message_id\":" + storedMsgId + ","
                "\"text\":\"" + text + "\","
                "\"parse_mode\":\"Markdown\"}";
    } else {
      // Send new message then pin it
      url = "/bot" + BOTtoken + "/sendMessage";
      payload = "{\"chat_id\":\"" + chat_id + "\","
                "\"text\":\"" + text + "\","
                "\"parse_mode\":\"Markdown\"}";
    }

    client.println("POST " + url + " HTTP/1.1");
    client.println("Host: api.telegram.org");
    client.println("Content-Type: application/json");
    client.println("Content-Length: " + String(payload.length()));
    client.println();
    client.print(payload);

    // Read response to get message_id for new messages
    String response = "";
    long t = millis();
    while (client.connected() && millis() - t < 4000) {
      while (client.available()) { response += (char)client.read(); t = millis(); }
    }
    client.stop();

    // If it was a new message, extract message_id and pin it
    if (storedMsgId.length() == 0) {
      int idPos = response.indexOf("\"message_id\":");
      if (idPos >= 0) {
        int start = idPos + 13;
        int end = response.indexOf(',', start);
        String newMsgId = response.substring(start, end);
        newMsgId.trim();

        // Save the message_id
        prefs.begin("doorbell", false);
        prefs.putString(keyName.c_str(), newMsgId);
        prefs.end();

        // Pin it
        WiFiClientSecure pinClient;
        pinClient.setInsecure();
        if (pinClient.connect("api.telegram.org", 443)) {
          String pinUrl = "/bot" + BOTtoken + "/pinChatMessage";
          String pinPayload = "{\"chat_id\":\"" + chat_id + "\","
                              "\"message_id\":" + newMsgId + ","
                              "\"disable_notification\":true}";
          pinClient.println("POST " + pinUrl + " HTTP/1.1");
          pinClient.println("Host: api.telegram.org");
          pinClient.println("Content-Type: application/json");
          pinClient.println("Content-Length: " + String(pinPayload.length()));
          pinClient.println();
          pinClient.print(pinPayload);
          long pt = millis();
          while (pinClient.connected() && millis() - pt < 3000) {
            while (pinClient.available()) { pinClient.read(); pt = millis(); }
          }
          pinClient.stop();
        }
      }
    }
  }
}

void notifyAllAdmins(String message, bool requiresAction) {
  for (int i = 0; i < NUM_USERS; i++) {
    if (authorizedUsers[i].accessLevel == 0) {
      if (requiresAction) {
        sendActionRequired(authorizedUsers[i].chat_id, message);
      } else {
        bot->sendMessage(authorizedUsers[i].chat_id, message, "");
      }
    }
  }
}

void wifiReconnectIfNeeded() {
  static unsigned long lastCheck = 0;
  if (millis() - lastCheck < 5000) return;
  lastCheck = millis();

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected, reconnecting...");

    if (wifiCache.valid) {
      WiFi.disconnect();
      WiFi.begin(ssid.c_str(), password.c_str(), wifiCache.channel, wifiCache.bssid, true);

      unsigned long startTime = millis();
      int attempts = 0;
      while (WiFi.status() != WL_CONNECTED && attempts < 6) {
        delay(500);
        attempts++;
      }

      if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("✓ Fast reconnect: %lums\n", millis() - startTime);
        return;
      }

      Serial.println("Fast reconnect failed");
      wifiCache.valid = false;
    }

    connectWiFi();
  }
}
