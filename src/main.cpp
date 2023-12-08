// TODO: KOmmentare an aktuelle Funktion anpassen. Auch im js und evrl css filegit
/**************************************************************************
 * ESP32 Webserver example
 * Using:
 *  - AZ-Delivery Devkit-v4 like board
 *  - Monochrome OLEDs based on SSD1306 drivers (128x64, I2C)
 *  - 8 LED ring (WS2812)
 **************************************************************************/
// #include <string>
// using namespace std;

// Avoid conflicts between OLED library color definitions and own color definitions
#define NO_ADAFRUIT_SSD1306_COLOR_COMPATIBILITY

// #include <Arduino.h> //lt. Webserver Ebook nötig, war aber vorher nicht da und lief bis dahin trotzdem - beobachten!
#include "../include/colors.h"
#include <SPI.h>
#include <FastLED.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>

// WIFI network credentials
const char *ssid = "Villanetz";
const char *password = "Kellerbad100%";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Create a WebSocket object
AsyncWebSocket ws("/ws");

// Stores LED state
String ledState;

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define LED_STRIPE_DATA_PIN 2
#define LED_STRIPE_CLOCK_PIN 4
#define LED_STRIPE_COUNT 60
#define BRIGHTNESS 255

#define DELAY 100

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library.
#define OLED_RESET -1		// Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C // original comment: See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
							// in fac 0x3C works
// create object for the display
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

CRGB leds[LED_STRIPE_COUNT];

void initWiFi();
void initSPIFFS();
void drawCycleText(const char *test_char);
void drawChar(const char character);
void drawIPAdr(IPAddress ipAdr);
String processor(const String &var);

void notifyClients(String state)
{
	ws.textAll(state);
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len)
{
	AwsFrameInfo *info = (AwsFrameInfo *)arg;
	if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
	{
		data[len] = 0;
		if (strcmp((char *)data, "bON") == 0)
		{
			ledState = "ON";
			notifyClients("ON");
		}
		if (strcmp((char *)data, "bOFF") == 0)
		{
			ledState = "OFF";
			notifyClients("OFF");
		}
	}
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
			 void *arg, uint8_t *data, size_t len)
{
	switch (type)
	{
	case WS_EVT_CONNECT:
		Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
		break;
	case WS_EVT_DISCONNECT:
		Serial.printf("WebSocket client #%u disconnected\n", client->id());
		break;
	case WS_EVT_DATA:
		handleWebSocketMessage(arg, data, len);
		break;
	case WS_EVT_PONG:
	case WS_EVT_ERROR:
		break;
	}
}

void initWebSocket()
{
	ws.onEvent(onEvent);
	server.addHandler(&ws);
}

/*
 Set one LED in the LED ring to the given color

 @param noLED ID of LED (0 ... 7)
 @param color LED color (RGB hex-code)
*/

void indicatorLight(bool state)
{
	;
}

void setup()
{
	Serial.begin(115200);

	// SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
	if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
	{
		Serial.println(F("SSD1306 allocation failed"));
		for (;;)
			; // Don't proceed, loop forever
	}

	// Clear the buffer
	display.clearDisplay();
	display.display();

	LEDS.addLeds<APA102, LED_STRIPE_DATA_PIN, LED_STRIPE_CLOCK_PIN, RGB>(leds, LED_STRIPE_COUNT);
	FastLED.setBrightness(BRIGHTNESS);

	initWiFi();
	initWebSocket();
	initSPIFFS();

	// when server is requested on root adress the main html file is delivered
	// TODO: Kommentar überprüfen und anpassen
	server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
			  { request->send(SPIFFS, "/index.html", "text/html", false, processor); });
	// serve requested files from root adress from file system root
	server.serveStatic("/", SPIFFS, "/");

	// Start server
	server.begin();
}

void loop()
{
	// ws.cleanupClients();
	// // TODO: warum darf Operanden Reihenfolge hier nicht vertauscht werden?
	// if (ledState == "ON")
	// {
	// 	indicatorLight(true);
	// }
	// else
	// {
	// 	indicatorLight(false);
	// }
	leds[0] = CRGB(255, 0, 0);
	leds[1] = CRGB(0, 255, 0);
	leds[2] = CRGB(0, 255, 0);
	leds[3] = CRGB(0, 0, 255);
	leds[4] = CRGB(0, 0, 255);
	leds[5] = CRGB(0, 0, 255);
	FastLED.show();
	delay(1000);
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

void drawChar(const char character)
{
	// display.clearDisplay();
	display.setTextColor(SSD1306_WHITE); // Draw white text
	display.setTextSize(2);
	// display.setCursor(2, 0);
	display.write(character);
	display.display();
}

/*
 Print IP adress on connected OLED display

 @param ipAdr IP adress to print
*/
void drawIPAdr(IPAddress ipAdr)
{
	display.setTextColor(SSD1306_WHITE); // Draw white text
	display.setTextSize(1);
	display.setCursor(0, 0);
	display.print(F("IP: "));
	display.println(ipAdr);
	display.display();
}

/*
 Init WiFi system
*/
void initWiFi()
{
	// set WiFi status indication
	WiFi.mode(WIFI_STA);
	WiFi.begin(ssid, password);
	Serial.print("Connecting to WiFi ..");

	// wait for succesful connection to WiFi
	while (WiFi.status() != WL_CONNECTED)
	{
		Serial.print('.');
		drawChar('.');

		delay(1000);
	}
	// set WiFi status indication
	Serial.println(WiFi.localIP());
	drawIPAdr(WiFi.localIP());
}

/*
Init SPIFFS file system
*/
void initSPIFFS()
{
	if (!SPIFFS.begin(true))
	{
		Serial.println("An error has occurred while mounting SPIFFS");
	}
	else
	{
		Serial.println("SPIFFS mounted successfully");
	}
}

String processor(const String &var)
{
	if (var == "STATE")
	{
		return ledState;
	}
	return String();
}
