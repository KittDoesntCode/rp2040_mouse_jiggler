#include <Adafruit_NeoPixel.h>
#include <Adafruit_TinyUSB.h>
#include <FreeRTOS.h>  // Included automatically with Arduino-Pico core
#include <task.h>

#define PIN 16
#define NUMPIXELS 1
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

// Variables to store the RGB values
float redValue = 255.0;
float greenValue = 0.0;
float blueValue = 0.0;

// Variable to track the direction of color changes
int redStep = -1;
int greenStep = 1;
int blueStep = 0;

// Mouse movement control
int moveSequence = 0;                     // 0: up, 1: right, 2: down, 3: left
const int deltaMove = 1;                  // Move by 1 pixels each time
const unsigned long moveInterval = 60000; // move every 1 min

// HID report descriptor using TinyUSB's template
enum {
  RID_MOUSE = 1
};

// USB HID object
Adafruit_USBD_HID usb_hid;

// Standard HID Mouse Report Descriptor (Driverless)
uint8_t const desc_hid_report[] = {
  TUD_HID_REPORT_DESC_MOUSE(HID_REPORT_ID(RID_MOUSE))
};

// Function to constrain the color values
float constrainColor(float color) {
  if (color < 0) return 0;
  if (color > 255) return 255;
  return color;
}

void ledTask() {
  redValue += redStep;
  greenValue += greenStep;
  blueValue += blueStep;

  redValue = constrain(redValue, 0, 255);
  greenValue = constrain(greenValue, 0, 255);
  blueValue = constrain(blueValue, 0, 255);

  pixels.setPixelColor(0, pixels.Color((int)redValue, (int)greenValue, (int)blueValue));
  pixels.show();

  if (redValue <= 0 && greenValue >= 255) {
    redStep = 0;
    greenStep = -1;
    blueStep = 1;
  } else if (greenValue <= 0 && blueValue >= 255) {
    redStep = 1;
    greenStep = 0;
    blueStep = -1;
  } else if (blueValue <= 0 && redValue >= 255) {
    redStep = -1;
    greenStep = 1;
    blueStep = 0;
  }
}

void process_hid() {
  if (usb_hid.ready()) {
    int8_t x = 0, y = 0;
    switch (moveSequence) {
      case 0: y = -deltaMove; break;  // Move up
      case 1: x = deltaMove; break;   // Move right
      case 2: y = deltaMove; break;   // Move down
      case 3: x = -deltaMove; break;  // Move left
    }

    usb_hid.mouseMove(RID_MOUSE, x, y);
    
    // Wait for HID to be ready and reset the mouse position
    while (!usb_hid.ready()) {
      vTaskDelay(2 / portTICK_PERIOD_MS);
    }

    // Reset the mouse back to the original position
    usb_hid.mouseMove(RID_MOUSE, -x, -y);

    // Update sequence and time
    moveSequence = (moveSequence + 1) % 4;
  }
}

void setup() {
  pixels.begin();
  TinyUSBDevice.begin();
  
  // Use generic HID VID/PID for driverless operation
  TinyUSBDevice.setID(0x046D, 0xC077);  // Generic VID/PID for HID mouse

  // Set up HID
  usb_hid.setPollInterval(1);
  usb_hid.setReportDescriptor(desc_hid_report, sizeof(desc_hid_report));
  usb_hid.setStringDescriptor("Logitech USB Optical Mouse");
  usb_hid.begin();
}

void loop() {
  ledTask();
  vTaskDelay(5 / portTICK_PERIOD_MS); // LED update every 5ms
}

void loop1() {
  process_hid();
  vTaskDelay(moveInterval / portTICK_PERIOD_MS);  // Mouse movement update
}
