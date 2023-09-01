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

// #include <Arduino.h> //lt. Webserver Ebook nötig, war aber vorher nicht da und lief bis dahin trotzdem - beobachten!
#include <SPI.h>
#include <Adafruit_NeoPixel.h>
#include "../include/colors.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

// Replace with your network credentials
const char *ssid = "Villanetz";
const char *password = "Kellerbad100%";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<title>ESP Web Server</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<link rel="icon" href="data:,">
<style>
html {
font-family: Arial;
text-align: center;
}
body {
max-width: 400px;
margin:0px auto;
}
</style>
</head>
<body>
<h1>Hello World!</h1>
<p>Congratulations!<br>This is your first Web Server with the ESP.</p>
</body>
</html>
)rawliteral";

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

void initWiFi();
void drawCycleText(const char *test_char);
void runningCircleSingle(uint32_t color, uint32_t backgroundColor, int wait);

void setup()
{
	Serial.begin(115200);
	initWiFi();
	server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
			  { request->send(200, "text/html", index_html); });
	// Start server
	server.begin();

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

void initWiFi()
{
	WiFi.mode(WIFI_STA);
	WiFi.begin(ssid, password);
	Serial.print("Connecting to WiFi ..");
	while (WiFi.status() != WL_CONNECTED)
	{
		Serial.print('.');
		delay(1000);
	}
	Serial.println(WiFi.localIP());
}
