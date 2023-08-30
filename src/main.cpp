/**************************************************************************
 This is an example for our Monochrome OLEDs based on SSD1306 drivers

 Pick one up today in the adafruit shop!
 ------> http://www.adafruit.com/category/63_98

 This example is for a 128x64 pixel display using I2C to communicate
 3 pins are required to interface (two I2C and one reset).

 Adafruit invests time and resources providing this open
 source code, please support Adafruit and open-source
 hardware by purchasing products from Adafruit!

 Written by Limor Fried/Ladyada for Adafruit Industries,
 with contributions from the open source community.
 BSD license, check license.txt for more information
 All text above, and the splash screen below must be
 included in any redistribution.
 **************************************************************************/
// #include <string>
// using namespace std;

#include <SPI.h>
#include <Adafruit_NeoPixel.h>
#include "../include/colors.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define LED_RING_KL_PIN 2
#define LED_RING_KL_COUNT 8

#define DELAY 100

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library.
// On an arduino UNO:       A4(SDA), A5(SCL)
// On an arduino MEGA 2560: 20(SDA), 21(SCL)
// On an arduino LEONARDO:   2(SDA),  3(SCL), ...
#define OLED_RESET -1		// Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

Adafruit_NeoPixel ringKlein(LED_RING_KL_COUNT, LED_RING_KL_PIN, NEO_GRB + NEO_KHZ800);

#define NUMFLAKES 10 // Number of snowflakes in the animation example

#define LOGO_HEIGHT 16
#define LOGO_WIDTH 16
static const unsigned char PROGMEM logo_bmp[] =
	{0b00000000, 0b11000000,
	 0b00000001, 0b11000000,
	 0b00000001, 0b11000000,
	 0b00000011, 0b11100000,
	 0b11110011, 0b11100000,
	 0b11111110, 0b11111000,
	 0b01111110, 0b11111111,
	 0b00110011, 0b10011111,
	 0b00011111, 0b11111100,
	 0b00001101, 0b01110000,
	 0b00011011, 0b10100000,
	 0b00111111, 0b11100000,
	 0b00111111, 0b11110000,
	 0b01111100, 0b11110000,
	 0b01110000, 0b01110000,
	 0b00000000, 0b00110000};


void drawCycleText(const char *test_char);
void runningCircleSingle(uint32_t color, uint32_t backgroundColor, int wait);

void setup()
{
	Serial.begin(9600);

	// SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
	if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
	{
		Serial.println(F("SSD1306 allocation failed"));
		for (;;)
			; // Don't proceed, loop forever
	}

	// Clear the buffer
	display.clearDisplay();

	ringKlein.begin();
	ringKlein.show();
	ringKlein.setBrightness(64);
}

void loop()
{
	drawCycleText("   Rot    ");
	runningCircleSingle(RED, BLACK, DELAY);
	drawCycleText("  Gruen   ");
	runningCircleSingle(GREEN, BLACK, DELAY);
	drawCycleText("   Blau   ");
	runningCircleSingle(BLUE, BLACK, DELAY);
	drawCycleText("  Weiss   ");
	runningCircleSingle(WHITE, BLACK, DELAY);
}

void testdrawchar(void)
{
	display.clearDisplay();

	display.setTextSize(1);				 // Normal 1:1 pixel scale
	display.setTextColor(SSD1306_WHITE); // Draw white text
	display.setCursor(0, 0);			 // Start at top-left corner
	display.cp437(true);				 // Use full 256 char 'Code Page 437' font

	// Not all the characters will fit on the display. This is normal.
	// Library will draw what it can and the rest will be clipped.
	for (int16_t i = 0; i < 256; i++)
	{
		if (i == '\n')
			display.write(' ');
		else
			display.write(i);
	}

	display.display();
	delay(2000);
}

void drawCycleText(const char *text)
{
	display.clearDisplay();
	display.setTextColor(SSD1306_WHITE); // Draw white text
	display.setTextSize(2);
	display.setCursor(2, 0);
	display.println(F("  Zyklus"));
	display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
	display.println(text);
	display.display();
}

void runningCircleSingle(uint32_t color, uint32_t backgroundColor, int wait)
{
	for (int i = 0; i < LED_RING_KL_COUNT; i++)
	{
		ringKlein.fill(backgroundColor);
		ringKlein.setPixelColor(i, color);
		ringKlein.show();
		delay(wait);
	}
}
