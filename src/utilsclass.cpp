#include "utilsclass.h"
#include "entrypoint.h"
#include "networkbasetypes.pb.h"
#include "sdk/CRecipientFilters.h"

uint64_t bhop_state = 0;

PlayerUtils::PlayerUtils(std::string m_plugin_name)
{
    this->plugin_name = m_plugin_name;
}

void PlayerUtils::SetBunnyhop(int playerid, bool state)
{
    if(state == true) bhop_state |= (1ULL << playerid);
    else bhop_state &= ~(1ULL << playerid);

    INetworkMessageInternal* netmsg = g_pNetworkMessages->FindNetworkMessagePartial("SetConVar");
    auto msg = netmsg->AllocateMessage()->ToPB<CNETMsg_SetConVar>();
    CMsg_CVars_CVar* cvar = msg->mutable_convars()->add_cvars();
    cvar->set_name("sv_autobunnyhopping");
    cvar->set_value(state == true ? "true" : "false");

    CSingleRecipientFilter filter(playerid);
    g_pGameEventSystem->PostEventAbstract(-1, false, &filter, netmsg, msg, 0);

    #ifndef _WIN32
    delete msg;
    #endif
}

bool PlayerUtils::IsListeningToGameEvent(int playerid, std::string game_event)
{
    void* getListener = GetSignature("LegacyGameEventListener");
    if(!getListener) return false;
    
    IGameEventListener2* playerListener = reinterpret_cast<IGameEventListener2*(*)(int)>(getListener)(playerid);
    if(!playerListener) return false;

    return g_gameEventManager->FindListener(playerListener, game_event.c_str());
}

bool PlayerUtils::GetBunnyhop(int playerid)
{
    return ((bhop_state & (1ULL << playerid)) != 0);
}