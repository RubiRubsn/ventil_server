#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiClient.h>
#include <ESP8266mDNS.h>
#include <string.h>

const int anz_relays = 4;
const bool pumpe = true;
bool fuellstand_sensor = true;
const int pumpe_pin = D5;
const uint8_t Relay[anz_relays] = {D0, D1, D2, D4};
const char *SSID = "****";
const char *PSW = "****";
//hier die nr. des Ventil servers eintragen um eine einfachere handhabung bei der ip eingabe zu haben
const char *Nummnerierung_der_server = "1";

bool relay[anz_relays] = {false};
uint32_t relay_anzeit[anz_relays] = {0};

bool restart = false;
ESP8266WebServer server(80);

bool fuelle_messen()
{
  int messung = analogRead(A0);
  bool help = false;
  delay(10);
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

void handle_root()
{
  server.send(200, "text / plain", "zum schalten: /set_relay?relay_nr=<realy nr angeben>&bew_zeit=<zeit in minuten   zum ausschalten: /set_relay?relay_nr=<realy nr angeben>&schalten=<zufaellige positive zahl angeben> zum abfragen: /abfrage_relay?relay_nr=<relay nr angeben>  zum neustart /restart");
}

void handle_restart()
{
  server.send(200, "text / plain", "neustart nachdem alle bewaesserungen ausgefuehrt wurden");
  restart = true;
}

void handle_relay_schaltung()
{
  String message = "";
  if (server.arg("relay_nr") == "" && server.arg("schalten") == "")
  {
    message = "ERROR";
  }
  else if (server.arg("bew_zeit") == "")
  {
    relay[server.arg("relay_nr").toInt() - 1] = false;
    message = "OK";
  }

  if (server.arg("relay_nr") == "" && server.arg("bew_zeit") == "")
  {
    message = "ERROR";
  }
  else if (relay[server.arg("relay_nr").toInt() - 1] == false && server.arg("schalten") == "")
  {
    Serial.print("relay :");
    Serial.println(relay[server.arg("relay_nr").toInt() - 1]);

    Serial.println(server.arg("relay_nr").toInt() - 1);
    relay[server.arg("relay_nr").toInt() - 1] = true;

    relay_anzeit[server.arg("relay_nr").toInt() - 1] = millis() + server.arg("bew_zeit").toInt() * 60000;
    Serial.println(relay_anzeit[server.arg("relay_nr").toInt() - 1]);
    message = "OK";
  }
  else if (server.arg("schalten") == "")
  {
    message = "bereits an!";
  }
  server.send(200, "text / plain", message);
}

void handle_abfrage_relay()
{
  String message = "";
  if (server.arg("relay_nr") == "")
  {
    message = "ERROR";
  }
  else
  {
    if (relay[server.arg("relay_nr").toInt() - 1])
    {
      message = String("offen fuer die naechsten ");
      if ((relay_anzeit[server.arg("relay_nr").toInt() - 1] - millis()) / 1000 > 60)
      {
        message += String(((relay_anzeit[server.arg("relay_nr").toInt() - 1] - millis()) / 1000) / 60);
        message += String(" Minuten ");
        if (fuellstand_sensor)
        {
          if (fuelle_messen() == false)
          {
            message = String("Abgebrochen da nicht genug wasser");
          }
        }
      }
      else
      {
        message += String(((relay_anzeit[server.arg("relay_nr").toInt() - 1] - millis()) / 1000));
        message += String(" Sekunden ");
        if (fuellstand_sensor)
        {
          if (fuelle_messen() == false)
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

  server.send(200, "text / plain", message);
}

void handle_relays()
{

  if (fuellstand_sensor)
  {
    if (!fuelle_messen())
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
        if (fuelle_messen() == true)
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
        if (fuelle_messen() == true)
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
  if (!pumpe)
  {
    fuellstand_sensor = false;
  }
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PSW);
  Serial.print("Waiting to connect");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" ");

  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  server.on("/set_relay", handle_relay_schaltung);
  server.on("/abfrage_relay", handle_abfrage_relay);
  server.on("/", handle_root);
  server.on("/restart", handle_restart);
  server.begin();
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
  char *name = new char[strlen("ventilserver") + strlen(Nummnerierung_der_server)];
  strcpy(name, "ventilserver");
  strcat(name, Nummnerierung_der_server);
  Serial.print("dns: ");
  Serial.print(name);
  Serial.println(".local");
  if (!MDNS.begin(name))
  { // Start the mDNS responder for esp8266.local
    Serial.println("Error setting up MDNS responder!");
  }
  Serial.println("mDNS responder started");
}

void loop()
{
  MDNS.update();
  server.handleClient();
  handle_relays();
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
      ESP.restart();
    }
  }
}