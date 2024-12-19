#ifndef _utilsclass_h
#define _utilsclass_h

#include <string>

class PlayerUtils
{
private:
    std::string plugin_name;

public:
    PlayerUtils(std::string m_plugin_name);

    void SetBunnyhop(int playerid, bool state);
    bool GetBunnyhop(int playerid);
    bool IsListeningToGameEvent(int playerid, std::string game_event);

};

#endif