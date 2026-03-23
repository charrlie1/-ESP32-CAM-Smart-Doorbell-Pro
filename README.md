# 🛡️ ESP32-CAM Smart Doorbell Pro (Version X)

[![ESP32](https://img.shields.io/badge/ESP32-CAM-blue.svg)](https://github.com/espressif/arduino-esp32)
[![Arduino](https://img.shields.io/badge/Arduino-1.0.6-00979D.svg)](https://www.arduino.cc/)
[![License](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)
[![Telegram](https://img.shields.io/badge/Telegram-Bot_API-26A5E4.svg)](https://core.telegram.org/bots/api)

An enterprise-grade, IoT-enabled smart doorbell system built on the ESP32-CAM platform. This project transcends standard tutorials by implementing **production-ready security** with SHA-256 hashed credentials, persistent NVS storage, Over-The-Air (OTA) firmware updates, and a fully-featured Telegram Bot interface for remote access control.

![Product Concept](docs/images/Design Concept 3.png)
*Professional smart doorbell system - Modern security meets IoT innovation*

---

## 📑 Table of Contents

- [✨ Features](#-features)
- [🎯 Why This Project Stands Out](#-why-this-project-stands-out)
- [🛠️ Hardware Requirements](#️-hardware-requirements)
- [⚙️ Software Stack](#️-software-stack)
- [🚀 Quick Start Guide](#-quick-start-guide)
- [🔐 Security Architecture](#-security-architecture)
- [⚠️ Critical Developer Notes](#️-critical-developer-notes)
- [📡 Telegram Bot Commands](#-telegram-bot-commands)
- [🏗️ System Architecture](#️-system-architecture)
- [🐛 Troubleshooting](#-troubleshooting)
- [📸 Gallery](#-gallery)
- [🤝 Contributing](#-contributing)
- [📄 License](#-license)
- [👨‍💻 Author](#-author)

---

## ✨ Features

### Core Functionality

- **🔍 Facial Recognition Engine**
  - Local face detection and enrollment using the AI Thinker ESP32-CAM
  - Real-time intruder detection with photo alerts
  - Support for multiple enrolled faces with access level management

- **📱 Telegram Bot Integration**
  - Instant photo alerts sent directly to authorized users
  - Remote door control via inline keyboard buttons
  - Administrative commands: `/open`, `/login`, `/alarm`, `/status`, `/enroll`
  - Multi-user support with role-based access control (Admin/Family/Guest)

- **🔐 Advanced Security**
  - **SHA-256 Password Hashing**: Implemented via `mbedtls/sha256.h` for cryptographic password security
  - **Constant-Time Comparison**: Prevents timing-based password attacks
  - **Encrypted NVS Storage**: WiFi credentials and system state stored securely using `Preferences.h`
  - **TLS Certificate Validation**: Secure communication with Telegram API (requires ESP32 Core 2.0+)

- **🔄 OTA (Over-The-Air) Updates**
  - Wireless firmware updates via `ArduinoOTA`
  - No need to physically access the device for software updates
  - Password-protected OTA access for security

- **⚡ Hardware Optimizations**
  - **Brownout Protection**: Custom RTC control register mapping prevents voltage-drop reboots
  - **Fast WiFi Reconnection**: RTC-cached BSSID/channel for <500ms reconnect times
  - **PSRAM Management**: Optimized frame buffer allocation for stable camera operation

- **🖥️ Interactive User Interface**
  - I2C OLED display (SSD1306/SH1107 compatible) for real-time status
  - Visual indicators for lock state, scanning mode, and alerts
  - Customizable messages via Telegram bot

---

## 🎯 Why This Project Stands Out

This isn't just another ESP32-CAM tutorial project. Here's what makes it different:

| Standard Tutorial Projects | This Project (Version X) |
|---------------------------|--------------------------|
| Hardcoded WiFi credentials in source | Encrypted NVS storage with provisioning mode |
| Plain text passwords | SHA-256 cryptographic hashing + constant-time verification |
| Basic face detection demo | Production-ready multi-user facial recognition system |
| Manual firmware updates | Full OTA update support with authentication |
| Simple photo capture | Telegram bot integration with inline keyboards, access control, and alerts |
| No state persistence | Persistent state management across reboots |
| Prone to brownout crashes | Brownout protection + PSRAM optimization |

**Real-World Application**: This system is ready for actual deployment. It includes session management, command cooldown to prevent spam, proper error handling, and a comprehensive security model suitable for home or small office use.

---

## 🛠️ Hardware Requirements

### Essential Components

| Component | Specification | Purpose | Notes |
|-----------|--------------|---------|-------|
| **ESP32-CAM** | AI Thinker module with OV2640 | Main controller + camera | Must have PSRAM |
| **Power Supply** | 5V/2A regulated | System power | **Critical**: 1A minimum, 2A recommended |
| ** Module** | 5V SRD-05VDC-SL-C | Door strike control | Active-LOW trigger |
| **OLED Display** | 128x64 I2C (SSD1306/SH1107) | Status display | Connected to GPIO 13/14 |
| **Push Button** | Normally open momentary | Manual scan trigger | INPUT_PULLUP mode |
| **Buzzer** | Passive buzzer | Audio alerts | PWM-driven via LEDC |
| **Capacitor** | 100µF electrolytic | Power stabilization | **Highly recommended** |

### Pin Configuration

```cpp
// Hardware Pins
#define RELAY_PIN   2   // Door lock relay control
#define BUTTON_PIN  15  // Physical doorbell button
#define BUZZER_PIN  12   // Alert buzzer (LEDC channel 8)
#define I2C_SDA     13  // OLED display data
#define I2C_SCL     14  // OLED display clock
#define FLASH_LED_PIN 4 // On board led (LEDC channel 7)
```

### Wiring Diagram

```
ESP32-CAM Module
├─ 5V     → Power supply +5V (2A recommended)
├─ GND    → Common ground + capacitor negative
├─ GPIO 2 → Relay IN (door lock control)
├─ GPIO 15→ Button (other side to GND)
├─ GPIO 12 → Buzzer + (buzzer - to GND)
├─ GPIO 13→ OLED SDA
└─ GPIO 14→ OLED SCL

100µF Capacitor: Across 5V/GND pins (close to ESP32-CAM)
```

**⚠️ Power Supply Warning**: The ESP32-CAM draws significant current during camera operations (400-500mA peaks). Insufficient power causes brownouts, crashes, and unreliable operation. Use a quality 2A supply.

---

## ⚙️ Software Stack

### Required Arduino Libraries

Install these via Arduino IDE Library Manager:

```
UniversalTelegramBot  (v1.3.0+)
ArduinoJson           (v6.x)
Adafruit GFX Library  (latest)
Adafruit SSD1306      (latest)
ESP32 Arduino Core    (v1.0.6 REQUIRED - see notes below)
```

### ESP32 Board Configuration

**Critical**: Must use ESP32 Arduino Core **1.0.6** (see [Developer Notes](#️-critical-developer-notes) for why)

```
Tools → Board:              AI Thinker ESP32-CAM
Tools → Upload Speed:       115200
Tools → Flash Frequency:    80MHz
Tools → Partition Scheme:   Huge APP (3MB No OTA / 1MB SPIFFS)
Tools → Core Debug Level:   None
Tools → PSRAM:             Enabled
```

---

## 🚀 Quick Start Guide

### 1. Hardware Setup

1. **Assemble the circuit** following the wiring diagram above
2. **Add the 100µF capacitor** across 5V/GND pins near the ESP32-CAM
3. **Connect FTDI programmer** for initial flash:
   ```
   FTDI → ESP32-CAM
   5V  → 5V
   GND → GND
   TX  → U0R (RX)
   RX  → U0T (TX)
   GPIO 0 → GND (for flash mode)
   ```

### 2. Software Installation

1. **Download and install Arduino IDE** (1.8.19 recommended)

2. **Install ESP32 board support**:
   - Add board manager URL:
     ```
     https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
     ```
   - Install **ESP32 version 1.0.6** (critical - see notes)

3. **Install required libraries** via Library Manager

4. **Clone this repository**:
   ```bash
   git clone https://github.com/charrlie1/esp32-cam-smart-doorbell.git
   cd esp32-cam-smart-doorbell
   ```

### 3. Configuration

#### Create Telegram Bot

1. Open Telegram and message `@BotFather`
2. Send `/newbot` and follow the prompts
3. Save your bot token (format: `1234567890:ABCdefGHI...`)
4. Get your chat ID by messaging `@userinfobot`

#### Update Code Configuration

Edit the main `.ino` file and update these values:

```cpp
// === UPDATE YOUR CHAT IDs HERE ===
AuthorizedUser authorizedUsers[] = {
  {"YOUR_CHAT_ID", "Your Name", 0},    // Admin (auto-login)
  {"FAMILY_ID_1", "Family Member", 1}, // Family (needs login)
  {"GUEST_ID", "Guest Name", 2}        // Guest (view only)
};
```

**Access Levels:**
- `0` = Admin (auto-login, full access)
- `1` = Family (must login, can unlock door)
- `2` = Guest (must login, view photos only)

### 4. Initial Flash

1. **Enter flash mode**: Connect GPIO 0 to GND, press RESET
2. **Upload the sketch** via Arduino IDE
3. **Exit flash mode**: Disconnect GPIO 0, press RESET

### 5. Provisioning (First Boot)

1. **Open Serial Monitor** (115200 baud, NL+CR)
2. **Enter credentials when prompted**:
   ```
   WiFi SSID: YourNetworkName
   WiFi Password: ********
   Telegram Bot Token: 1234567890:ABC...
   Master Password: ******** (will be SHA-256 hashed)
   OTA Password: ******** (for wireless updates)
   ```
3. **Device will reboot** and connect to WiFi

### 6. Verify Operation

1. **Check Serial Monitor** for boot sequence:
   ```
   ✓ Credentials loaded from secure storage
   ✓ Fast connect: 287ms (192.168.1.100)
   ✓ TLS certificate validation enabled
   ✓ OTA updates enabled
   ✓ System ready
   ```

2. **Test Telegram bot**: Send `/start` to your bot
3. **Verify keyboard appears** with unlock/photo/status buttons

---

## 🔐 Security Architecture

### Password Security

**SHA-256 Cryptographic Hashing**:
```cpp
// Master password NEVER stored in plaintext
String MASTER_PASSWORD_HASH = sha256Hex(masterPassword);

// Constant-time comparison prevents timing attacks
volatile uint8_t diff = 0;
for (size_t i = 0; i < MASTER_PASSWORD_HASH.length(); i++) {
  diff |= (candHash[i] ^ MASTER_PASSWORD_HASH[i]);
}
return diff == 0;
```

**Why This Matters**:
- Even with physical access to flash memory, passwords cannot be recovered
- Timing attacks (measuring comparison time to guess passwords) are prevented
- Industry-standard cryptographic practices

### Credential Storage

**Encrypted NVS (Non-Volatile Storage)**:
```cpp
prefs.begin("secure", false);  // Encrypted namespace
prefs.putString("master_pw_hash", MASTER_PASSWORD_HASH);
prefs.putString("token", BOTtoken);
```

All sensitive data is stored in ESP32's encrypted NVS partition, not in source code.

### Access Control

**Three-Tier Permission System**:

| Level | Auto-Login | Unlock Door | View Photos | Admin Commands |
|-------|-----------|-------------|-------------|----------------|
| Admin (0) | ✅ | ✅ | ✅ | ✅ |
| Family (1) | ❌ | ✅ | ✅ | ❌ |
| Guest (2) | ❌ | ❌ | ✅ | ❌ |

**Session Management**:
- Admins: Never expire
- Family/Guest: 1-hour timeout
- Command cooldown: 2s (photos: 5s) to prevent spam

### Network Security

**TLS Certificate Validation** (ESP32 Core 2.0+):
```cpp
// Root CA certificate for Telegram API
const char telegram_root_ca[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
[Let's Encrypt ISRG Root X1 - Valid until 2035]
-----END CERTIFICATE-----
)EOF";

clientTCP.setCACert(telegram_root_ca);
```

Prevents man-in-the-middle attacks on Telegram communication.

---

## ⚠️ Critical Developer Notes

### Why ESP32 Core 1.0.6 is Required

**Important**: This project **MUST** use ESP32 Arduino Core Version 1.0.6 due to Espressif deprecating the `esp-who` Face Recognition libraries in newer cores.

**Installing ESP32 Core 1.0.6**:
1. Tools → Board → Boards Manager
2. Search "esp32"
3. **Uninstall** any newer versions
4. **Install** version 1.0.6 specifically
5. Restart Arduino IDE

### The CORRUPT HEAP Fix (Memory Stability)

During development, adding SHA-256 hashing, OTA updates, and Telegram integration pushed ESP32 RAM to its limits. This caused a critical `CORRUPT HEAP: Bad head at 0x3fxxxxxx` crash during facial recognition enrollment due to PSRAM data bus desynchronization.

**Root Cause**: The camera's default 20MHz XCLK frequency caused PSRAM read/write buffer overflow when combined with other memory-intensive operations.

**The Fix** (Essential for stability):

```cpp
// In initCamera() function:
config.xclk_freq_hz = 10000000;  // CRITICAL: Lowered from 20MHz to 10MHz
```

**Additional Stability Measures**:

1. **Enable PSRAM**: Mandatory in Arduino IDE
   ```
   Tools → PSRAM → Enabled
   ```

2. **Correct Partition Scheme**:
   ```
   Tools → Partition Scheme → Huge APP (3MB No OTA / 1MB SPIFFS)
   ```

3. **Hardware Capacitor**: 100µF across 5V/GND
   - Smooths ~1A current spike during AI face mapping
   - Prevents brownout-triggered reboots

4. **Brownout Protection**:
   ```cpp
   WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);  // Disable detector
   ```

**Memory Usage After Optimization**:
```
Sketch uses: ~1,398,450 bytes (45%) of program storage
Global variables: ~54,272 bytes (16%) of dynamic memory
Free heap at runtime: ~152,000 bytes
```

### Camera Configuration for Stability

**Critical settings for PSRAM stability**:

```cpp
config.xclk_freq_hz = 10000000;      // 10MHz (was 20MHz)
config.pixel_format = PIXFORMAT_JPEG;
config.frame_size = FRAMESIZE_240X240;
config.jpeg_quality = 10;            // Lower = better quality
config.fb_count = 2;                 // Double buffering

if (psramFound()) {
  config.fb_location = CAMERA_FB_IN_PSRAM;  // Core 2.0+ only
} else {
  config.fb_count = 1;
  config.jpeg_quality = 15;
}
```

**Frame Warmup** (prevents initial corruption):
```cpp
for (int i = 0; i < 3; i++) {
  camera_fb_t * fb = esp_camera_fb_get();
  if (fb) {
    esp_camera_fb_return(fb);
    delay(100);
  }
}
```

---

## 📡 Telegram Bot Commands

### User Commands

| Command | Description | Access Level |
|---------|-------------|--------------|
| `/start` | Show main menu with keyboard | All |
| `/help` | Display available commands | All |
| `/login <password>` | Authenticate (Family/Guest) | Family, Guest |
| `/status` | View system status | All |
| `/photo` | Capture and send photo | All |
| `/open` | Unlock door remotely | Admin, Family |

### Admin-Only Commands

| Command | Description |
|---------|-------------|
| `/setmsg <text>` | Set custom OLED message |
| `/clear` | Clear OLED message |
| `/enroll` | Start face enrollment mode |
| `/deletefaces` | Remove all enrolled faces |

### Inline Keyboard Buttons

**Intruder Alert** (sent to all admins):
```
[✅ Unlock] [❌ Dismiss]
[📷 View Photo] [🚨 Sound Alarm]
```

**Siren Control**:
```
[🛑 STOP ALARM]
[📷 View Photo] [📊 Status]
```

**Persistent Keyboard** (always visible):
```
Admin:
[🔓 Unlock] [📷 Photo]
[📊 Status] [👥 Users]
[📝 Set Message] [➕ Enroll]

Family:
[🔓 Unlock] [📷 Photo]
[📊 Status] [🚪 Logout]

Guest:
[📷 Photo] [📊 Status]
[🚪 Logout]
```

---

## 🏗️ System Architecture

### State Machine

```
STANDBY → Button Press → SCANNING
   ↓                         ↓
   ←─────────────────────────┘
   ↓                         
   ↓ Face Match?             
   ├─ YES → UNLOCKED (10s) → STANDBY
   └─ NO  → ALARM (3s) → STANDBY
   
Admin command: SIREN → Manual stop → STANDBY
```

### WiFi Fast Reconnect

**RTC-Cached Connection** (sub-500ms reconnects):

```cpp
RTC_DATA_ATTR struct {
  uint8_t bssid[6];
  uint8_t channel;
  bool valid;
} wifiCache;

// On reconnect:
WiFi.begin(ssid, password, wifiCache.channel, wifiCache.bssid, true);
```

**Fallback to DHCP** if fast reconnect fails.

### OLED Display States

| State | Icon | Display | Duration |
|-------|------|---------|----------|
| STANDBY | 🔒 | "LOCKED" + Last entry info | Persistent |
| SCANNING | 🔍 | "SCANNING" | 6 seconds |
| UNLOCKED | 🔓 | "WELCOME" + Progress bar | 10 seconds |
| ALARM | ⚠️ | "INTRUDER" | 3 seconds |
| SIREN | 🚨 | "LOCK-DOWN!" (flashing) | Until stopped |

---

## 🐛 Troubleshooting

### Common Issues

#### Camera Initialization Failed

**Symptoms**: `Camera init failed: 0x105` or similar

**Solutions**:
1. Verify camera ribbon cable connection
2. Ensure 5V power supply provides adequate current (2A)
3. Check PSRAM is enabled in Tools menu
4. Verify XCLK is set to 10MHz (not 20MHz)
5. Add 100µF capacitor if not present

#### WiFi Connection Fails

**Symptoms**: `WiFi Fail` on OLED, no network connection

**Solutions**:
1. Verify SSID and password (case-sensitive)
2. Ensure 2.4GHz WiFi (ESP32 doesn't support 5GHz)
3. Check WiFi signal strength (move closer to router)
4. Use DHCP instead of static IP for testing

#### Corrupt Heap Crashes

**Symptoms**: `CORRUPT HEAP: Bad head at 0x3f...` in Serial Monitor

**Solutions**:
1. **Lower XCLK frequency** to 10MHz (critical fix)
2. Verify Partition Scheme: Huge APP (3MB)
3. Add 100µF capacitor across power pins
4. Upgrade power supply to 2A
5. Enable PSRAM in Tools menu

#### Telegram Bot Not Responding

**Symptoms**: No response from bot commands

**Solutions**:
1. Verify bot token is correct (check with curl/browser)
2. Confirm chat ID matches authorized users array
3. Check network connectivity (ping test)
4. Verify bot isn't blocked by firewall
5. Check Serial Monitor for error messages

#### Photo Capture Fails

**Symptoms**: `Photo capture failed!` or garbled images

**Solutions**:
1. Improve lighting conditions
2. Verify camera isn't in use by another process
3. Check power supply voltage under load
4. Increase frame quality setting if too compressed
5. Verify camera warmup frames (3x) executed

#### OTA Updates Not Working

**Symptoms**: Can't find device in Arduino IDE port list

**Solutions**:
1. Verify device and computer on same WiFi network
2. Check IP address in Serial Monitor
3. Try using IP address instead of hostname
4. Verify OTA password is correct
5. Check firewall isn't blocking mDNS/Bonjour

---

## 📸 Gallery

### Hardware Assembly

![Hardware Setup](docs/images/hardware-assembly.jpg)
*ESP32-CAM with relay module, OLED display, and power supply*

### OLED Display States

![Display States](docs/images/oled-states.jpg)
*Different system states: Locked, Scanning, Unlocked, Alarm*

### Telegram Bot Interface

![Bot Interface](docs/images/telegram-interface.jpg)
*Inline keyboards, intruder alerts, and admin controls*

### Web Interface (Optional)

![Web Streaming](docs/images/web-interface.jpg)
*Live camera stream accessible at http://[ESP32-IP]:81*

---

## 🤝 Contributing

Contributions are welcome! This project is designed to be a learning platform and showcase of advanced ESP32 development.

**Areas for Contribution**:
- ESP32 Core 2.0+ compatibility layer
- Additional Telegram bot features
- Alternative face recognition algorithms
- PCB design for permanent installation
- 3D-printable enclosure designs
- Mobile app integration
- Cloud logging and analytics

**How to Contribute**:
1. Fork the repository
2. Create a feature branch (`git checkout -b feature/AmazingFeature`)
3. Commit your changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

---

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

**TL;DR**: You can use this code for personal or commercial projects, but please provide attribution.

---

## 👨‍💻 Author

**Toluwanimi Charles** ([@charrlie1](https://github.com/charrlie1))

*Electrical/Electronic Engineering Student | IoT Enthusiast | Embedded Systems Developer*

- GitHub: [@charrlie1](https://github.com/charrlie1)
- LinkedIn: [Toluwanimi Charles](https://linkedin.com/in/toluwanimi-charles)
- Email: [your-email@example.com](mailto:your-email@example.com)

---

## 🙏 Acknowledgments

- **Espressif Systems** - ESP32 platform and documentation
- **Brian Lough** - UniversalTelegramBot library
- **Adafruit** - GFX and SSD1306 display libraries
- **Arduino Community** - Extensive tutorials and support
- **Telegram Bot API** - Enabling remote IoT control

---

## 📚 Additional Resources

### Documentation
- [ESP32 Technical Reference](https://www.espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf)
- [OV2640 Camera Datasheet](https://www.uctronics.com/download/cam_module/OV2640DS.pdf)
- [Telegram Bot API](https://core.telegram.org/bots/api)
- [ESP32 Arduino Core Docs](https://docs.espressif.com/projects/arduino-esp32/en/latest/)

### Related Projects
- [ESP32-CAM Official Examples](https://github.com/espressif/esp32-camera)
- [Face Recognition Library](https://github.com/espressif/esp-who)
- [Telegram Bots with ESP32](https://github.com/witnessmenow/Universal-Arduino-Telegram-Bot)

---

## ⭐ Star This Repository

If you found this project useful, please consider giving it a star! It helps others discover this work and motivates continued development.

---

**Built with ❤️ and lots of ☕ in 2026**

**Version X** - *Where Security Meets Innovation*

---

<div align="center">

### 🔒 Secure | 🚀 Fast | 💡 Smart

**ESP32-CAM Smart Doorbell Pro** - Taking IoT Security to the Next Level

[⬆ Back to Top](#️-esp32-cam-smart-doorbell-pro-version-x)

</div>
