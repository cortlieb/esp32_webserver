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

CRGBPalette16 gPal;

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define LED_STRIPE_DATA_PIN 2
#define LED_STRIPE_CLOCK_PIN 4
#define LED_STRIPE_COUNT 60
#define BRIGHTNESS 100
#define FRAMES_PER_SECOND 45
#define COOLING 55
#define SPARKING 120

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

#define DELAY 100

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library.
#define OLED_RESET -1		// Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C // original comment: See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
							// in fac 0x3C works
// create object for the display
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

CRGB leds[LED_STRIPE_COUNT];
bool gReverseDirection = false;

uint8_t gCurrentPatternNumber = 0; // Index number of which pattern is current
uint8_t gHue = 0;				   // rotating "base color" used by many of the patterns

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

void Fire2012WithPalette()
{
	// Array of temperature readings at each simulation cell
	static uint8_t heat[LED_STRIPE_COUNT];

	// Step 1.  Cool down every cell a little
	for (int i = 0; i < LED_STRIPE_COUNT; i++)
	{
		heat[i] = qsub8(heat[i], random8(0, ((COOLING * 10) / LED_STRIPE_COUNT) + 2));
	}

	// Step 2.  Heat from each cell drifts 'up' and diffuses a little
	for (int k = LED_STRIPE_COUNT - 1; k >= 2; k--)
	{
		heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2]) / 3;
	}

	// Step 3.  Randomly ignite new 'sparks' of heat near the bottom
	if (random8() < SPARKING)
	{
		int y = random8(7);
		heat[y] = qadd8(heat[y], random8(160, 255));
	}

	// Step 4.  Map from heat cells to LED colors
	for (int j = 0; j < LED_STRIPE_COUNT; j++)
	{
		// Scale the heat value from 0-255 down to 0-240
		// for best results with color palettes.
		uint8_t colorindex = scale8(heat[j], 240);
		CRGB color = ColorFromPalette(gPal, colorindex);
		int pixelnumber;
		if (gReverseDirection)
		{
			pixelnumber = (LED_STRIPE_COUNT - 1) - j;
		}
		else
		{
			pixelnumber = j;
		}
		leds[pixelnumber] = color;
	}
}

void nextPattern();
void rainbow();
void rainbowWithGlitter();
void confetti();
void sinelon();
void bpm();
void juggle();

// List of patterns to cycle through.  Each is defined as a separate function below.
typedef void (*SimplePatternList[])();
SimplePatternList gPatterns = {rainbow, rainbowWithGlitter, confetti, sinelon, juggle, bpm};

void nextPattern()
{
	// add one to the current pattern number, and wrap around at the end
	gCurrentPatternNumber = (gCurrentPatternNumber + 1) % ARRAY_SIZE(gPatterns);
}

void rainbow()
{
	// FastLED's built-in rainbow generator
	fill_rainbow(leds, LED_STRIPE_COUNT, gHue, 7);
}

void addGlitter(fract8 chanceOfGlitter)
{
	if (random8() < chanceOfGlitter)
	{
		leds[random16(LED_STRIPE_COUNT)] += CRGB::White;
	}
}

void rainbowWithGlitter()
{
	// built-in FastLED rainbow, plus some random sparkly glitter
	rainbow();
	addGlitter(80);
}

void confetti()
{
	// random colored speckles that blink in and fade smoothly
	fadeToBlackBy(leds, LED_STRIPE_COUNT, 10);
	int pos = random16(LED_STRIPE_COUNT);
	leds[pos] += CHSV(gHue + random8(64), 200, 255);
}

void sinelon()
{
	// a colored dot sweeping back and forth, with fading trails
	fadeToBlackBy(leds, LED_STRIPE_COUNT, 20);
	int pos = beatsin16(13, 0, LED_STRIPE_COUNT - 1);
	leds[pos] += CHSV(gHue, 255, 192);
}

void bpm()
{
	// colored stripes pulsing at a defined Beats-Per-Minute (BPM)
	uint8_t BeatsPerMinute = 62;
	CRGBPalette16 palette = PartyColors_p;
	uint8_t beat = beatsin8(BeatsPerMinute, 64, 255);
	for (int i = 0; i < LED_STRIPE_COUNT; i++)
	{ // 9948
		leds[i] = ColorFromPalette(palette, gHue + (i * 2), beat - gHue + (i * 10));
	}
}

void juggle()
{
	// eight colored dots, weaving in and out of sync with each other
	fadeToBlackBy(leds, LED_STRIPE_COUNT, 20);
	uint8_t dothue = 0;
	for (int i = 0; i < 8; i++)
	{
		leds[beatsin16(i + 7, 0, LED_STRIPE_COUNT - 1)] |= CHSV(dothue, 200, 255);
		dothue += 32;
	}
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

	LEDS.addLeds<APA102, LED_STRIPE_DATA_PIN, LED_STRIPE_CLOCK_PIN, BGR>(leds, LED_STRIPE_COUNT);
	FastLED.setBrightness(BRIGHTNESS);
	gPal = HeatColors_p;
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

	random16_add_entropy(random(1));

	Fire2012WithPalette(); // run simulation frame, using palette colors

	// Call the current pattern function once, updating the 'leds' array
	// gPatterns[gCurrentPatternNumber]();
	// send the 'leds' array out to the actual LED strip
	// FastLED.show();
	// insert a delay to keep the framerate modest
	// FastLED.delay(1000 / FRAMES_PER_SECOND);

	// do some periodic updates
	// EVERY_N_MILLISECONDS(20) { gHue++; }   // slowly cycle the "base color" through the rainbow
	// EVERY_N_SECONDS(10) { nextPattern(); } // change patterns periodically

	// leds[0] = 0x000000;
	// leds[1] = 0x330000;
	// leds[2] = 0x660000;
	// leds[3] = 0x990000;
	// leds[4] = 0xCC0000;
	// leds[5] = 0xFF0000;
	// leds[6] = 0xFF3300;
	// leds[7] = 0xFF6600;
	// leds[8] = 0xFF9900;
	// leds[9] = 0xFFCC00;
	// leds[10] = 0xFFFF00;
	// leds[11] = 0xFFFF33;
	// leds[12] = 0xFFFF66;
	// leds[13] = 0xFFFF99;
	// leds[14] = 0xFFFFCC;
	// leds[15] = 0xFFFFFF;

	FastLED.show(); // display this frame
	FastLED.delay(1000 / FRAMES_PER_SECOND);
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
