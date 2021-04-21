#include "ui_server.h"
#include <ESPUI.h>

void server_ui::init_server()
{
    ESPUI.setVerbosity(Verbosity::VerboseJSON);
    ESPUI.begin("test");
}
