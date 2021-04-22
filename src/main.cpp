#include <Arduino.h>
#include <ESPUI.h>
#include <DNSServer.h>
#include "ui_server.h"
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WiFiClient.h>
#include <ESP8266mDNS.h>
#include <string.h>
#include <Updater.h>

const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 1, 1);
DNSServer dnsServer;

const int anz_relays = 4;
bool pumpe = true;
bool fuellstand_sensor = true;
const int pumpe_pin = D5;
const uint8_t Relay[anz_relays] = {D0, D1, D2, D4};
char *SSID = "";
char *PSW = "";
const char *version = "2.0";
//hier die nr. des Ventil servers eintragen um eine einfachere handhabung bei der ip eingabe zu haben

bool fuelle_zwischenspeicher = false;
bool fuelle_zw_ui = false;
const char *OTA_INDEX PROGMEM = R"=====(<!DOCTYPE html><html><head><meta charset=utf-8><title>OTA</title></head><body><div class="upload"><form method="POST" action="/ota" enctype="multipart/form-data"><input type="file" name="data" /><input type="submit" name="upload" value="Upload" title="Upload Files"></form></div></body></html>)=====";
bool relay[anz_relays] = {false};
bool relay_zwischenspeicher[anz_relays] = {false};
uint32_t relay_anzeit[anz_relays] = {0};

bool restart = false;

server_ui server_ui;

void handle_version(String &message)
{
  message = String(version);
}

void handleOTAUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
{
  if (!index)
  {
    Serial.printf("UploadStart: %s\n", filename.c_str());
    // calculate sketch space required for the update, for ESP32 use the max constant
#if defined(ESP32)
    if (!Update.begin(UPDATE_SIZE_UNKNOWN))
#else
    const uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
    if (!Update.begin(maxSketchSpace))
#endif
    {
      // start with max available size
      Update.printError(Serial);
    }
#if defined(ESP8266)
    Update.runAsync(true);
#endif
  }

  if (len)
  {
    Update.write(data, len);
  }

  // if the final flag is set then this is the last frame of data
  if (final)
  {
    if (Update.end(true))
    {
      // true to set the size to the current progress
      Serial.printf("Update Success: %ub written\nRebooting...\n", index + len);
      ESP.restart();
    }
    else
    {
      Update.printError(Serial);
    }
  }
}

bool fuelle_messen()
{

  int messung = analogRead(A0);

  bool help = false;
  delay(40);
  if (messung > 800)
  {

    help = false;
  }
  else
  {

    help = true;
  }

  return help;
}

void handle_root(String &message)
{
  message = "zum schalten: /set_relay?relay_nr=<realy nr angeben>&bew_zeit=<zeit in minuten   zum ausschalten: /set_relay?relay_nr=<realy nr angeben>&schalten=<zufaellige positive zahl angeben> zum abfragen: /abfrage_relay?relay_nr=<relay nr angeben>  zum neustart /restart";
}

void handle_restart(String &message)
{
  message = "neustart nachdem alle bewaesserungen ausgefuehrt wurden";
  restart = true;
}

void handle_relay_schaltung(String &message, String relay_nr, String bew_zeit, String schalten)
{
  if (bew_zeit == "null")
  {
    relay[relay_nr.toInt() - 1] = false;
    message = "OK";
  }
  if (schalten == "null")
  {
    if (relay[relay_nr.toInt() - 1] == false)
    {
      Serial.print("relay :");
      Serial.println(relay[relay_nr.toInt() - 1]);

      Serial.println(relay_nr.toInt() - 1);
      relay[relay_nr.toInt() - 1] = true;

      relay_anzeit[relay_nr.toInt() - 1] = millis() + bew_zeit.toInt() * 60000;
      Serial.println(relay_anzeit[relay_nr.toInt() - 1]);
      message = "OK";
    }
    else
    {
      message = "bereits an!";
    }
  }
}

void handle_abfrage_relay(String &message, String relay_nr)
{
  if (relay[relay_nr.toInt() - 1])
  {
    message = String("offen fuer die naechsten ");
    if ((relay_anzeit[relay_nr.toInt() - 1] - millis()) / 1000 > 60)
    {
      message += String(((relay_anzeit[relay_nr.toInt() - 1] - millis()) / 1000) / 60);
      message += String(" Minuten ");
      if (fuellstand_sensor)
      {
        if (!fuelle_zwischenspeicher)
        {
          message = String("Abgebrochen da nicht genug wasser");
        }
      }
    }
    else
    {
      message += String(((relay_anzeit[relay_nr.toInt() - 1] - millis()) / 1000));
      message += String(" Sekunden ");
      if (fuellstand_sensor)
      {
        if (!fuelle_zwischenspeicher)
        {
          message = String("Abgebrochen da nicht genug wasser");
        }
      }
    }
  }
  else
  {
    message = String("geschlossen");
  }
}

void notFound(AsyncWebServerRequest *request)
{
  request->send(404, "text/plain", "Not found");
}

void handle_relays()
{

  if (fuellstand_sensor)
  {
    if (!fuelle_zwischenspeicher)
    {
      digitalWrite(pumpe_pin, HIGH);
    }
  }
  for (int i = 0; i < anz_relays; i++)
  {
    if (relay[i] == true)
    {
      if (millis() > relay_anzeit[i])
      {
        relay[i] = false;
      }
    }
  }

  for (int i = 0; i < anz_relays; i++)
  {
    if (relay[i] && digitalRead(Relay[i]) == HIGH)
    {
      digitalWrite(Relay[i], LOW);
    }
    if (!relay[i] && digitalRead(Relay[i]) == LOW)
    {
      digitalWrite(Relay[i], HIGH);
    }
  }
  if (pumpe)
  {

    bool help = false;
    for (int i = 0; i < anz_relays; i++)
    {
      if (digitalRead(Relay[i]) == LOW)
      {
        help = true;
      }
    }
    if (help == false && digitalRead(pumpe_pin) == LOW)
    {
      if (fuellstand_sensor)
      {
        if (fuelle_zwischenspeicher == true)
        {
          digitalWrite(pumpe_pin, HIGH);
        }
      }
      else
      {
        digitalWrite(pumpe_pin, HIGH);
      }
    }
    if (help == true && digitalRead(pumpe_pin) == HIGH)
    {
      if (fuellstand_sensor)
      {
        if (fuelle_zwischenspeicher == true)
        {
          digitalWrite(pumpe_pin, LOW);
        }
      }
      else
      {
        digitalWrite(pumpe_pin, LOW);
      }
    }
  }
}

void setup()
{

  Serial.begin(115200);
  if (!pumpe)
  {
    fuellstand_sensor = false;
  }
  server_ui.load();
  strcpy(server_ui.dat.version, version);
  pumpe = server_ui.dat.pumpe;
  fuellstand_sensor = server_ui.dat.fuellstand_sensor;
  WiFi.mode(WIFI_STA);
  WiFi.begin(server_ui.dat.SSID, server_ui.dat.PSW);
  Serial.print("Waiting to connect");
  int timeout = 0;
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    timeout++;
    if (timeout >= 20)
    {
      if (server_ui.dat.reset != 1)
      {
        Serial.print("\n\nCreating hotspot");

        WiFi.mode(WIFI_AP);
        WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
        WiFi.softAP("feuchte_server");

        int timeoutAP = 5;

        do
        {
          delay(500);
          Serial.print(".");
          timeoutAP--;
        } while (timeoutAP > 0);
        strcpy(server_ui.dat.SSID, "SSID EINGEBEN");
        strcpy(server_ui.dat.PSW, "PSW EINGEBEN");
        break;
      }
      else
      {
        ESP.restart();
      }
    }
  }
  Serial.println(" ");

  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  dnsServer.start(DNS_PORT, "*", apIP);
  server_ui.init_server(anz_relays, pumpe, fuellstand_sensor);

  ESPUI.server->on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    String message;
    handle_root(message);
    request->send(200, "text/plain", message);
  });

  ESPUI.server->on("/set_relay", HTTP_GET, [](AsyncWebServerRequest *request) {
    String message;
    AsyncWebParameter *relay_nummer = request->getParam(0);
    AsyncWebParameter *secondary_p = request->getParam(1);
    if (relay_nummer->name() == "relay_nr")
    {
      if (secondary_p->name() == "bew_zeit")
      {
        String relay_nr = relay_nummer->value();
        String bew_zeit = secondary_p->value();
        Serial.println(relay_nr);
        Serial.println(bew_zeit);
        handle_relay_schaltung(message, relay_nr, bew_zeit, "null");
        Serial.println("message");
        Serial.println(message);
      }
      if (secondary_p->name() == "schalten")
      {
        String relay_nr = relay_nummer->value();
        String schalten = secondary_p->value();
        handle_relay_schaltung(message, relay_nr, "null", schalten);
        Serial.println(message);
      }
    }
    else
    {
      message = "No message sent";
    }

    request->send(200, "text/plain", message);
  });

  ESPUI.server->on("/abfrage_relay", HTTP_GET, [](AsyncWebServerRequest *request) {
    String message;
    if (request->hasParam("relay_nr"))
    {
      String relay_nr = request->getParam("relay_nr")->value();
      handle_abfrage_relay(message, relay_nr);
    }
    else
    {
      message = "No message sent";
    }
    request->send(200, "text/plain", message);
  });

  ESPUI.server->on("/version", HTTP_GET, [](AsyncWebServerRequest *request) {
    String message;
    handle_version(message);
    request->send(200, "text/plain", message);
  });
  ESPUI.server->on("/restart", HTTP_GET, [](AsyncWebServerRequest *request) {
    String message;
    handle_restart(message);
    request->send(200, "text/plain", message);
  });
  ESPUI.server->on(
      "/ota",
      HTTP_POST,
      [](AsyncWebServerRequest *request) {
        request->send(200);
      },
      handleOTAUpload);

  ESPUI.server->on("/ota",
                   HTTP_GET,
                   [](AsyncWebServerRequest *request) {
                     AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", OTA_INDEX);
                     request->send(response);
                   });
  ESPUI.server->onNotFound(notFound);
  Serial.println("Server listening");
  for (int i = 0; i < anz_relays; i++)
  {
    pinMode(Relay[i], OUTPUT);
    digitalWrite(Relay[i], HIGH);
    Serial.println("pin auf output");
    delay(100);
  }
  if (pumpe)
  {
    pinMode(pumpe_pin, OUTPUT);
    digitalWrite(pumpe_pin, HIGH);
  }
  fuelle_zwischenspeicher = fuelle_messen();
  fuelle_zw_ui = !fuelle_zwischenspeicher;
}

void loop()
{
  fuelle_zwischenspeicher = fuelle_messen();
  handle_relays();
  for (int i = 0; i < 4; i++)
  {
    if (relay[i] && !relay_zwischenspeicher[i])
    {
      ESPUI.print(server_ui.ids[i], "geöffnet");
      relay_zwischenspeicher[i] = true;
    }
    else if (!relay[i] && relay_zwischenspeicher[i])
    {
      ESPUI.print(server_ui.ids[i], "geschlossen");
      relay_zwischenspeicher[i] = false;
    }
  }
  if (fuellstand_sensor)
  {
    if (fuelle_zwischenspeicher && !fuelle_zw_ui)
    {
      ESPUI.print(server_ui.ids[4], "sensor im nassen");
      fuelle_zw_ui = true;
    }
    else if (!fuelle_zwischenspeicher && fuelle_zw_ui)
    {
      ESPUI.print(server_ui.ids[4], "sensor im trocknen");
      fuelle_zw_ui = false;
    }
  }

  pumpe = server_ui.dat.pumpe;
  fuellstand_sensor = server_ui.dat.fuellstand_sensor;
  restart = server_ui.dat.restart;
  if (restart)
  {
    int help = 0;
    for (int i = 0; i < anz_relays; i++)
    {
      if (!relay[i])
      {
        help++;
      }
    }
    if (help == anz_relays)
    {
      server_ui.save();
      delay(100);
      ESP.restart();
    }
  }
}