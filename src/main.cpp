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
#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <Arduino_JSON.h>

// WIFI network credentials
const char *ssid = "Villanetz";
const char *password = "Kellerbad100%";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

const char *PARAM_INPUT_OUTPUT = "output";
const char *PARAM_INPUT_STATE = "state";

#define NUM_OUTPUTS 4

bool outputStates[NUM_OUTPUTS] = {false, false, false, false};

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define LED_RING_KL_PIN 2
#define LED_RING_KL_COUNT 8

#define DELAY 100

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library.
#define OLED_RESET -1		// Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C // original comment: See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
							// in fac 0x3C works
// create object for the display
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
// create object for the LED-ring
Adafruit_NeoPixel ringKlein(LED_RING_KL_COUNT, LED_RING_KL_PIN, NEO_GRB + NEO_KHZ800);

void initWiFi();
void initSPIFFS();
void drawCycleText(const char *test_char);
void drawChar(const char character);
void drawIPAdr(IPAddress ipAdr);
void runningCircleSingle(uint32_t color, uint32_t backgroundColor, int wait);
void setOutput(int output, int state);

/*
 Retrieves the stae of the used outputs.

 @return json-formatted status of outputs
*/
String getOutputStates()
{
	JSONVar outputStatesJSON;
	for (int i = 0; i < NUM_OUTPUTS; i++)
	{
		outputStatesJSON["outputs"][i]["output"] = String(i + 1);
		outputStatesJSON["outputs"][i]["state"] = (outputStates[i]) ? "1" : "0";
		Serial.print(outputStates[i]);
	}
	String jsonString = JSON.stringify(outputStatesJSON);
	return jsonString;
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

	ringKlein.begin();
	ringKlein.show();
	ringKlein.setBrightness(8);

	initWiFi();
	initSPIFFS();

	// when server is requested on root adress the main html file is delivered
	server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
			  { request->send(SPIFFS, "/index.html", "text/html"); });
	// serve requested files from root adress from file system root
	server.serveStatic("/", SPIFFS, "/");

	// when server is requested on "/states" adress json-formatted states of outputs are delivered
	server.on("/states", HTTP_GET, [](AsyncWebServerRequest *request)
			  {
		String json = getOutputStates();
		request->send(200, "application/json", json);
		json = String(); });

	// when server is requested on "/update" outputs are set according to request parameters
	server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request)
			  {
		String output;
		String state;

		//if request has correct format
		if (request->hasParam(PARAM_INPUT_OUTPUT) && request->hasParam(PARAM_INPUT_STATE)) {
			output = request->getParam(PARAM_INPUT_OUTPUT)->value();
			state = request->getParam(PARAM_INPUT_STATE)->value();
			//set outputs according to transmitted parameters
			setOutput(output.toInt(), state.toInt());
		} else {
			output = "No message sent";
			state = "No message sent";
		} 
		Serial.print("GPIO: ");
		Serial.print(output);
		Serial.print(" - Set to: ");
		Serial.println(state);
		request->send(200, "text/plain", "OK"); });

	// Start server
	server.begin();
}

void loop()
{
	;
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

/*
 Set one LED in the LED ring to the given color

 @param noLED ID of LED (0 ... 7)
 @param color LED color (RGB hex-code)
*/
void setCircleLED(int noLED, uint32_t color)
{
	ringKlein.setPixelColor(noLED, color);
	ringKlein.show();
}

/*
 Init WiFi system
*/
void initWiFi()
{
	// set WiFi status indication
	setCircleLED(4, RED);
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
	setCircleLED(4, GREEN);
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

/*
Sets output indicator LEDs colors

@param output
	1: left indicator LEDs - blue
	2: left indicator LEDs - red
	3: right indicator LEDs - blue
	4: right indicator LEDs - red
@param state
	0: LED color OFF
	0: LED color ON
*/
void setOutput(int output, int state)
{
	static int ind1Color = 0; // color code for left indicator LEDs
	static int ind2Color = 0; // color code for right indicator LEDs

	switch (output)
	{
	case 1: // left indicator blue LED
		if (1 == state)
		{
			ind1Color = ind1Color | 0x0000FF;
		}
		else
		{
			ind1Color = ind1Color & 0xFFFF00;
		}
		break;
	case 2: // left indicator red LED
		if (1 == state)
		{
			ind1Color = ind1Color | 0xFF0000;
		}
		else
		{
			ind1Color = ind1Color & 0x00FFFF;
		}
		break;
	case 3: // right indicator blue LED
		if (1 == state)
		{
			ind2Color = ind2Color | 0x0000FF;
		}
		else
		{
			ind2Color = ind2Color & 0xFFFF00;
		}
		break;
	case 4: // right indicator red LED
		if (1 == state)
		{
			ind2Color = ind2Color | 0xFF0000;
		}
		else
		{
			ind2Color = ind2Color & 0x00FFFF;
		}
		break;

	default:
		break;
	}

	// store output states
	if (state)
	{
		outputStates[output - 1] = true;
	}
	else
	{
		outputStates[output - 1] = false;
	}

	// set left indicator LEDS
	setCircleLED(1, ind1Color);
	setCircleLED(2, ind1Color);
	setCircleLED(3, ind1Color);
	// set right indicator LEDS
	setCircleLED(5, ind2Color);
	setCircleLED(6, ind2Color);
	setCircleLED(7, ind2Color);
}
