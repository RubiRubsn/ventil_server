#include <Arduino.h>
#include <ESPUI.h>

struct data_vent_serv
{
    int anz_relays;
    bool pumpe;
    bool fuellstand_sensor;
    int pumpe_pin;
    bool fuelle_zwischenspeicher;
    bool restart;
    const char *version = "1.2";
};

class server_ui
{
private:

public:
    void init_server();
};