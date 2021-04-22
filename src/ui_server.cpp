#include "ui_server.h"
#include <ESPUI.h>
#include <string.h>
#include <EEPROM.h>

int server_ui::ids[11] = {0};
data_vent_serv server_ui::dat;

void server_ui::buttonCall(Control *sender, int type)
{
    switch (type)
    {
    case B_DOWN:
        dat.reset = 1;
        dat.restart = true;
        break;

    case B_UP:
        Serial.println("Button UP");
        break;
    }
}

void server_ui::save()
{
    EEPROM.begin(84);
    EEPROM.write(0, dat.reset);
    if (dat.pumpe)
    {
        EEPROM.write(1, 1);
    }
    else
    {
        EEPROM.write(1, 0);
    }
    if (dat.fuellstand_sensor)
    {
        EEPROM.write(2, 1);
    }
    else
    {
        EEPROM.write(2, 0);
    }
    for (int i = 0; i < 40; i++)
    {
        EEPROM.write(3 + i, dat.SSID[i]);
    }
    for (int i = 0; i < 40; i++)
    {
        EEPROM.write(43 + i, dat.PSW[i]);
    }
    EEPROM.commit();
};

void server_ui::load()
{
    EEPROM.begin(84);
    dat.reset = EEPROM.read(0);
    if (EEPROM.read(1) == 1)
    {
        dat.pumpe = true;
    }
    else
    {
        dat.pumpe = false;
    }
    if (EEPROM.read(2) == 1)
    {
        dat.fuellstand_sensor = true;
    }
    else
    {
        dat.fuellstand_sensor = false;
    }
    for (int i = 0; i < 40; i++)
    {
        dat.SSID[i] = EEPROM.read(3 + i);
    }
    for (int i = 0; i < 40; i++)
    {
        dat.PSW[i] = EEPROM.read(43 + i);
    }
}

void server_ui::textCall(Control *sender, int type)
{
    if (sender->id == ids[7])
    {
        strcpy(dat.SSID, sender->value.c_str());
    }
    if (sender->id == ids[8])
    {
        strcpy(dat.PSW, sender->value.c_str());
    }
}

void server_ui::switchCall(Control *sender, int value)
{
    switch (value)
    {
    case S_ACTIVE:
        if (ids[5] == sender->id)
        {
            dat.pumpe = true;
        }
        if (ids[6] == sender->id)
        {
            dat.fuellstand_sensor = true;
        }
        break;

    case S_INACTIVE:
        if (ids[5] == sender->id)
        {
            dat.pumpe = false;
        }
        if (ids[6] == sender->id)
        {
            dat.fuellstand_sensor = false;
        }
        break;
    }

    Serial.print(" ");
    Serial.println(sender->id);
}

void server_ui::init_server(const int anz_relays, const bool pumpe, const bool fuellstand_sensor)
{
    ESPUI.setVerbosity(Verbosity::VerboseJSON);
    uint16_t tab1 = ESPUI.addControl(ControlType::Tab, "Sensor", "Sensor");
    for (int i = 0; i < anz_relays; i++)
    {
        char *name = new char[strlen("0") + strlen("zustand Ventil nr: ")];
        strcpy(name, "zustand Ventil ");
        ids[i] = ESPUI.addControl(ControlType::Label, name, "geschlossen", ControlColor::Wetasphalt, tab1);
    }
    if (pumpe)
    {
        ids[4] = ESPUI.addControl(ControlType::Label, "füllstand", "sensor im Trocknen", ControlColor::Wetasphalt, tab1);
    }
    ids[5] = ESPUI.addControl(ControlType::Switcher, "Pumpe vorhanden?", String(pumpe), ControlColor::Wetasphalt, tab1, &switchCall);
    ids[6] = ESPUI.addControl(ControlType::Switcher, "Sensor vorhanden?", String(fuellstand_sensor), ControlColor::Wetasphalt, tab1, &switchCall);
    ids[7] = ESPUI.addControl(ControlType::Text, "SSID", String(dat.SSID), ControlColor::Wetasphalt, tab1, &textCall);
    ids[8] = ESPUI.addControl(ControlType::Text, "Wifi PSW", String(dat.PSW), ControlColor::Wetasphalt, tab1, &textCall);
    ids[9] = ESPUI.addControl(ControlType::Label, "Version", "test", ControlColor::Wetasphalt, tab1);
    ids[10] = ESPUI.addControl(ControlType::Button, "Speichern", "DRÜCKEN", ControlColor::Wetasphalt, tab1, &buttonCall);

    ESPUI.begin("test");
}
