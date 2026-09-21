#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

const char* WIFI_SSID = "YOUR_WIFI_NAME";
const char* WIFI_PASS = "YOUR_WIFI_PASSWORD";

const float HOME_LAT = 0.0; // your latitude
const float HOME_LON = 0.0; // your longitude

#define RES_X 64
#define RES_Y 32
#define MARGIN 2

MatrixPanel_I2S_DMA *display = nullptr;
uint16_t COLOR_WHITE;
uint16_t COLOR_RED;

const float LAMIN = 0.0;   // south edge of search box
const float LAMAX = 0.0;   // north edge of search box
const float LOMIN = 0.0;   // west edge of search box
const float LOMAX = 0.0;   // east edge of search box

const int REFRESH_RATE = 10000; 

struct FlightData {
  String callsign;
  String airline;
  String airportCode;
  String model;
  int speedKnots = 0;
  int altFeet = 0;
  bool isClimbing = false;
  bool isMilitary = false;
  bool hasData = false;
};

bool checkIfMilitary(const String& callsign) {
  return false;
}

String getAirlineName(const String& callsign, bool isMil) {
  if (isMil) return callsign;
  if (callsign.startsWith("WZZ") || callsign.startsWith("WMT") || callsign.startsWith("WUK")) return "WIZZ AIR";
  if (callsign.startsWith("VLG")) return "VUELING";
  if (callsign.startsWith("VOE")) return "VOLOTEA";
  if (callsign.startsWith("UAL")) return "UNITED";
  if (callsign.startsWith("THY")) return "TURKISH";
  if (callsign.startsWith("TVF") || callsign.startsWith("TRA")) return "TRANSAVIA";
  if (callsign.startsWith("TAP")) return "TAP";
  if (callsign.startsWith("SWR")) return "SWISS";
  if (callsign.startsWith("TVS")) return "SMARTWINGS";
  if (callsign.startsWith("SAS")) return "SCANDINAVIAN";
  if (callsign.startsWith("RYR") || callsign.startsWith("MAY") || callsign.startsWith("RUK")) return "RYANAIR";
  if (callsign.startsWith("RAM")) return "ROYAL AIR MAROC";
  if (callsign.startsWith("NOZ") || callsign.startsWith("NAX")) return "NORWEGIAN";
  if (callsign.startsWith("LGL")) return "LUXAIR";
  if (callsign.startsWith("DLH")) return "LUFTHANSA";
  if (callsign.startsWith("LOT")) return "LOT";
  if (callsign.startsWith("KLM")) return "KLM";
  if (callsign.startsWith("IBE") || callsign.startsWith("ANE")) return "IBERIA";
  if (callsign.startsWith("EWG") || callsign.startsWith("EWE")) return "EUROWINGS";
  if (callsign.startsWith("EJU") || callsign.startsWith("EZY") || callsign.startsWith("EZS")) return "EASYJET";
  if (callsign.startsWith("DAL")) return "DELTA";
  if (callsign.startsWith("CSW")) return "CHAIR";
  if (callsign.startsWith("LZB")) return "BULGARIA AIR";
  if (callsign.startsWith("BEL")) return "BRUSSELS";
  if (callsign.startsWith("BAW")) return "BRITISH AIR";
  if (callsign.startsWith("AZU")) return "AZUL";
  if (callsign.startsWith("AUA")) return "AUSTRIAN";
  if (callsign.startsWith("TSC")) return "AIR TRANSAT";
  if (callsign.startsWith("AFR") || callsign.startsWith("HOP")) return "AIR FRANCE";
  if (callsign.startsWith("AEA")) return "AIR EUROPA";
  if (callsign.startsWith("ACA")) return "AIR CANADA";
  if (callsign.startsWith("BTI")) return "AIR BALTIC";
  if (callsign.startsWith("AEE")) return "AEGEAN";
  if (callsign.startsWith("RZO")) return "AZORES AIR";
  if (callsign.startsWith("PGA")) return "PORTUGALIA";
  if (callsign.startsWith("HFY")) return "HI FLY";
  if (callsign.startsWith("MMZ")) return "EUROATLANTIC";
  if (callsign.startsWith("FPY")) return "PLAY";
  if (callsign.startsWith("FIN")) return "FINNAIR";
  return callsign;
}

FlightData fetchOverheadFlight() {
  FlightData flight;
  if (WiFi.status() != WL_CONNECTED) return flight;

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.setTimeout(5000);
  String url = "https://data-cloud.flightradar24.com/zones/fcgi/feed.js?bounds=" +
               String(LAMAX, 4) + "," + String(LAMIN, 4) + "," +
               String(LOMIN, 4) + "," + String(LOMAX, 4) + "&gnd=1&air=1";

  http.begin(client, url);
  http.setUserAgent("Mozilla/5.0 (Windows NT 10.0; Win64; x64)");
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    JsonDocument doc;
    if (deserializeJson(doc, http.getStream()) == DeserializationError::Ok) {
      JsonObject obj = doc.as<JsonObject>();

      JsonArray nearest;
      float bestDistSq = -1;

      for (JsonPair kv : obj) {
        String key = kv.key().c_str();
        if (key == "full_count" || key == "version") continue;

        JsonArray ac = kv.value().as<JsonArray>();
        if (ac.size() < 17) continue;
        if (ac[14].as<int>() == 1) continue;
        if (ac[4].as<int>() > 10000) continue;
        float lat = ac[1].as<float>();
        float lon = ac[2].as<float>();
        float dLat = lat - HOME_LAT;
        float dLon = lon - HOME_LON;
        float distSq = dLat * dLat + dLon * dLon;

        if (bestDistSq < 0 || distSq < bestDistSq) {
          bestDistSq = distSq;
          nearest = ac;
        }
      }

      if (!nearest.isNull()) {
        String rawCallsign = nearest[16].as<String>();
        if (rawCallsign.length() == 0) rawCallsign = nearest[13].as<String>();
        rawCallsign.trim();

        flight.callsign   = rawCallsign;
        flight.isMilitary = checkIfMilitary(rawCallsign);
        flight.airline    = getAirlineName(rawCallsign, flight.isMilitary);
        flight.altFeet    = nearest[4].as<int>();
        flight.speedKnots = nearest[5].as<int>();
        flight.isClimbing = nearest[15].as<float>() > 0;

        String modelStr = nearest[8].is<const char*>() ? nearest[8].as<String>() : "";
        modelStr.trim();
        flight.model = modelStr.length() ? modelStr : "UNK";

        String origin = nearest[11].is<const char*>() ? nearest[11].as<String>() : "";
        String dest   = nearest[12].is<const char*>() ? nearest[12].as<String>() : "";
        origin.trim();
        dest.trim();
        String airport = flight.isClimbing ? dest : origin;
        flight.airportCode = airport.length() ? airport : "UNK";

        flight.hasData = true;
      }
    }
  }
  http.end();
  return flight;
}

void setupMatrix() {
  HUB75_I2S_CONFIG config(RES_X, RES_Y, 1);
  config.gpio.r1 = 25; config.gpio.g1 = 26; config.gpio.b1 = 27;
  config.gpio.r2 = 14; config.gpio.g2 = 32; config.gpio.b2 = 13;
  config.gpio.a  = 23; config.gpio.b  = 19; config.gpio.c  = 5; config.gpio.d = 17;
  config.gpio.lat = 4; config.gpio.oe = 15; config.gpio.clk = 16;

  display = new MatrixPanel_I2S_DMA(config);
  display->begin();
  display->setBrightness8(50); // max 255

  COLOR_WHITE = display->color565(255, 255, 255);
  COLOR_RED   = display->color565(255, 0, 0);
}

void drawMilitaryWarning(int x, int y) {
  display->drawRect(x, y, 7, 7, COLOR_RED);
  display->drawLine(x + 3, y + 1, x + 3, y + 3, COLOR_WHITE);
  display->drawPixel(x + 3, y + 5, COLOR_WHITE);
}

void drawVerticalArrow(int x, int y, bool pointingUp) {
  display->drawLine(x + 2, y, x + 2, y + 6, COLOR_WHITE);
  int tipY        = pointingUp ? y     : y + 6;
  int innerWingY  = pointingUp ? y + 1 : y + 5;
  int outerWingY  = pointingUp ? y + 2 : y + 4;
  display->drawPixel(x + 2, tipY, COLOR_WHITE);
  display->drawPixel(x + 1, innerWingY, COLOR_WHITE);
  display->drawPixel(x + 3, innerWingY, COLOR_WHITE);
  display->drawPixel(x,     outerWingY, COLOR_WHITE);
  display->drawPixel(x + 4, outerWingY, COLOR_WHITE);
}

struct SmallGlyph { char c; uint8_t rows[5]; };
const SmallGlyph SMALL_FONT[] = {
  {'0', {0b010,0b101,0b101,0b101,0b010}},
  {'1', {0b010,0b110,0b010,0b010,0b111}},
  {'2', {0b110,0b001,0b010,0b100,0b111}},
  {'3', {0b110,0b001,0b010,0b001,0b110}},
  {'4', {0b101,0b101,0b111,0b001,0b001}},
  {'5', {0b111,0b100,0b110,0b001,0b110}},
  {'6', {0b011,0b100,0b110,0b101,0b010}},
  {'7', {0b111,0b001,0b010,0b010,0b010}},
  {'8', {0b010,0b101,0b010,0b101,0b010}},
  {'9', {0b010,0b101,0b011,0b001,0b110}},
  {'k', {0b100,0b101,0b110,0b101,0b101}},
  {'t', {0b010,0b111,0b010,0b010,0b011}},
  {'s', {0b011,0b100,0b010,0b001,0b110}},
  {'f', {0b011,0b100,0b111,0b100,0b100}},
};
const int SMALL_FONT_COUNT = sizeof(SMALL_FONT) / sizeof(SMALL_FONT[0]);

void drawSmallChar(char c, int x, int y, uint16_t color) {
  for (int i = 0; i < SMALL_FONT_COUNT; i++) {
    if (SMALL_FONT[i].c != c) continue;
    for (int row = 0; row < 5; row++) {
      uint8_t bits = SMALL_FONT[i].rows[row];
      for (int col = 0; col < 3; col++) {
        if (bits & (1 << (2 - col))) display->drawPixel(x + col, y + row, color);
      }
    }
    return;
  }
}

void drawSmallText(const String& str, int x, int y, uint16_t color) {
  for (unsigned int i = 0; i < str.length(); i++) {
    drawSmallChar(str.charAt(i), x + i * 4, y, color);
  }
}

const int ARROW_X = 55 + MARGIN;

void renderFlightUI(const FlightData& flight) {
  display->clearScreen();

  if (!flight.hasData) {
    display->setTextColor(COLOR_WHITE);
    display->setTextSize(1);
    display->setCursor(2 + MARGIN, 12);
    display->print("NO FLIGHTS");
    return;
  }

  int textStartX = MARGIN;
  if (flight.isMilitary) {
    drawMilitaryWarning(MARGIN, MARGIN);
    textStartX = 9 + MARGIN;
  }

  int maxAirlineChars = (ARROW_X - textStartX) / 6;
  if (maxAirlineChars < 0) maxAirlineChars = 0;

  display->setTextColor(COLOR_WHITE);
  display->setTextSize(1);
  display->setCursor(textStartX, MARGIN);
  display->print(flight.airline.substring(0, maxAirlineChars));

  drawVerticalArrow(ARROW_X, MARGIN, flight.isClimbing);

  display->setCursor(MARGIN, 10 + MARGIN);
  display->print(flight.model.substring(0, 5));

  display->setCursor( 42 + MARGIN, 10 + MARGIN);
  display->print(flight.airportCode.substring(0, 3));

  display->drawFastHLine(MARGIN, 19 + MARGIN, RES_X - (2 * MARGIN), COLOR_WHITE);

  drawSmallText(String(flight.speedKnots) + "kts", MARGIN, 23 + MARGIN, COLOR_WHITE);
  drawSmallText(String(flight.altFeet) + "ft", 34 + MARGIN, 23 + MARGIN, COLOR_WHITE);
}

void setup() {
  Serial.begin(115200);
  setupMatrix();

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  display->setTextColor(COLOR_WHITE);
  display->setTextSize(1);
  display->setCursor(2, 12);
  display->print("CONNECTING");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
}

void loop()
