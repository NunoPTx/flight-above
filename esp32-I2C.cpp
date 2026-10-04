#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

const char* WIFI_SSID = "WIFI_SSID"; // DEFINE THIS (2.4Ghz ONLY)
const char* WIFI_PASS = "WIFI_PASS"; // DEFINE THIS (2.4Ghz ONLY)

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1
#define OLED_I2C_ADDR 0x3C
#define I2C_SDA 8
#define I2C_SCL 9
#define MARGIN 2
#define REFRESH_MS 10000

const float HOME_LAT = NULL; // DEFINE THIS
const float HOME_LON = -NULL; // DEFINE THIS

const float LAMIN = NULL; // DEFINE THIS
const float LAMAX = NULL; // DEFINE THIS
const float LOMIN = -NULL; // DEFINE THIS 
const float LOMAX = -NULL; // DEFINE THIS

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

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

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  http.useHTTP10(true);
  http.setTimeout(5000);
  http.begin(client,
             "https://data-cloud.flightradar24.com/zones/fcgi/feed.js?bounds=" +
             String(LAMAX, 4) + "," + String(LAMIN, 4) + "," +
             String(LOMIN, 4) + "," + String(LOMAX, 4) + "&gnd=1&air=1");
  http.setUserAgent("Mozilla/5.0 (Windows NT 10.0; Win64; x64)");

  if (http.GET() == HTTP_CODE_OK) {
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

void drawMilitaryWarning(int x, int y) {
  display.drawRect(x, y, 7, 7, SSD1306_WHITE);
  display.drawLine(x + 3, y + 1, x + 3, y + 3, SSD1306_WHITE);
  display.drawPixel(x + 3, y + 5, SSD1306_WHITE);
}

void drawVerticalArrow(int x, int y, bool pointingUp) {
  display.drawLine(x + 2, y, x + 2, y + 6, SSD1306_WHITE);
  int tipY       = pointingUp ? y     : y + 6;
  int innerWingY = pointingUp ? y + 1 : y + 5;
  int outerWingY = pointingUp ? y + 2 : y + 4;
  display.drawPixel(x + 2, tipY, SSD1306_WHITE);
  display.drawPixel(x + 1, innerWingY, SSD1306_WHITE);
  display.drawPixel(x + 3, innerWingY, SSD1306_WHITE);
  display.drawPixel(x,     outerWingY, SSD1306_WHITE);
  display.drawPixel(x + 4, outerWingY, SSD1306_WHITE);
}

void printRight(const String& s, int y) {
  int16_t bx, by;
  uint16_t bw, bh;
  display.getTextBounds(s, 0, y, &bx, &by, &bw, &bh);
  display.setCursor(SCREEN_WIDTH - MARGIN - bw, y);
  display.print(s);
}

// Same layout as the original:
//  [airline ..............] [arrow]
//  [model ..............] [airport]
//  ------------------------------
//  [speed ................] [alt]
void renderFlightUI(const FlightData& flight) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  int textStartX = MARGIN;
  if (flight.isMilitary) {
    drawMilitaryWarning(MARGIN, MARGIN);
    textStartX = 9 + MARGIN;
  }

  const int arrowX = SCREEN_WIDTH - 5 - MARGIN;
  int maxAirlineChars = (arrowX - 4 - textStartX) / 6;
  if (maxAirlineChars < 0) maxAirlineChars = 0;

  display.setCursor(textStartX, MARGIN);
  display.print(flight.airline.substring(0, maxAirlineChars));
  drawVerticalArrow(arrowX, MARGIN, flight.isClimbing);

  display.setCursor(MARGIN, 11);
  display.print(flight.model.substring(0, 5));
  printRight(flight.airportCode.substring(0, 3), 11);

  display.drawFastHLine(MARGIN, 20, SCREEN_WIDTH - 2 * MARGIN, SSD1306_WHITE);

  display.setCursor(MARGIN, 23);
  display.print(String(flight.speedKnots) + "kts");
  printRight(String(flight.altFeet) + "ft", 23);

  display.display();
}

void showMessage(const char* msg) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(1, 12);
  display.print(msg);
  display.display();
}

void setup() {
  Wire.begin(I2C_SDA, I2C_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDR)) {
    for (;;);
  }

  showMessage("CONNECTING...");

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  for (int i = 0; i < 30 && WiFi.status() != WL_CONNECTED; i++) {
    delay(500);
  }

  if (WiFi.status() != WL_CONNECTED) {
    showMessage("WIFI FAILED");
  }
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.reconnect();
    showMessage("NO WIFI");
    delay(5000);
    return;
  }

  FlightData currentFlight = fetchOverheadFlight();
  if (currentFlight.hasData) {
    renderFlightUI(currentFlight);
  } else {
    showMessage("NO FLIGHTS");
  }

  delay(REFRESH_MS);
}
