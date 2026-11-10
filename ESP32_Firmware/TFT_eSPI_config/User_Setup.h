// =====================================================================
//  TFT_eSPI configuration for the DC motor teaching platform
//  ESP32 DevKit V1 (38 pin) + ILI9341 2.8" touch display (XPT2046)
//  Pins taken from the platform schematic (Fig. 3 of the paper)
//
//  Copy this file to:
//     Documents/Arduino/libraries/TFT_eSPI/User_Setup.h
//  (replace the original file; keep a backup copy of it)
// =====================================================================

#define USER_SETUP_INFO "DCmotor_Platform_ILI9341"

// ---------- Display driver ----------
#define ILI9341_DRIVER

// ---------- SPI pins (VSPI) ----------
#define TFT_MISO 19   // shared with the touch controller (T_MISO)
#define TFT_MOSI 23   // shared with the touch controller (T_MOSI)
#define TFT_SCLK 18   // shared with the touch controller (T_CLK)
#define TFT_CS   15   // display chip select
#define TFT_DC    2   // data / command
#define TFT_RST   4   // reset
// LED (backlight) is wired directly to 3.3 V

// ---------- Touch controller (XPT2046) ----------
#define TOUCH_CS 33   // touch chip select (T_CS)

// ---------- Fonts ----------
#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF
#define SMOOTH_FONT

// ---------- SPI speeds ----------
#define SPI_FREQUENCY        40000000
#define SPI_READ_FREQUENCY   20000000
#define SPI_TOUCH_FREQUENCY   2500000
