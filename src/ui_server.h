#include <Arduino.h>
#include <ESPUI.h>

struct data_vent_serv
{
    int anz_relays;
    bool pumpe;
    bool fuellstand_sensor;
    int pumpe_pin;
    bool fuelle_zwischenspeicher;
    bool restart = false;
    char *version = "1.0";
    uint8_t reset;
    char SSID[40];
    char PSW[40];
};

class server_ui
{
private:
    static void switchCall(Control *sender, int value);
    static void textCall(Control *sender, int type);
    static void buttonCall(Control *sender, int type);

public:
    static int ids[11];
    static data_vent_serv dat;
    static void save();
    static void load();
    void init_server(const int anz_relays, const bool pumpe, const bool fuellstand_sensor);
};