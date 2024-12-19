#include "entrypoint.h"
#include <lua/lua.h>
#include <lua/lauxlib.h>
#include <lua/lualib.h>
#include <LuaBridge/LuaBridge.h>
#include "utilsclass.h"
#include <ehandle.h>
#include "sdk/services.h"

#include <swiftly-ext/sdk.h>

void Hook_CCSPlayer_MovementServices_CheckJumpPre(void* services, void* movementData);

//////////////////////////////////////////////////////////////
/////////////////        Core Variables        //////////////
////////////////////////////////////////////////////////////

SH_DECL_HOOK5_void(IServerGameClients, ClientDisconnect, SH_NOATTRIB, 0, CPlayerSlot, ENetworkDisconnectionReason, const char *, uint64, const char *);
SH_DECL_HOOK2(IGameEventManager2, LoadEventsFromFile, SH_NOATTRIB, 0, int, const char*, bool);

int loadEventFromFileHookID = -1;
IServerGameClients *gameclients = nullptr;
IGameEventManager2* g_gameEventManager = nullptr;
IGameEventSystem* g_pGameEventSystem = nullptr;
IVEngineServer2* engine = nullptr;
ICvar* icvar = nullptr;

UtilsExtension g_Ext;
CUtlVector<FuncHookBase*> g_vecHooks;
CREATE_GLOBALVARS();

FuncHook<decltype(Hook_CCSPlayer_MovementServices_CheckJumpPre)> TCCSPlayer_MovementServices_CheckJumpPre(Hook_CCSPlayer_MovementServices_CheckJumpPre, "CCSPlayer_MovementServices_CheckJumpPre");

ConVar* autobunnyhoppingcvar = nullptr;

//////////////////////////////////////////////////////////////
/////////////////          Core Class          //////////////
////////////////////////////////////////////////////////////

EXT_EXPOSE(g_Ext);
bool UtilsExtension::Load(std::string& error, SourceHook::ISourceHook* SHPtr, ISmmAPI* ismm, bool late)
{
    SAVE_GLOBALVARS();
    if (!InitializeHooks()) {
        error = "Failed to initialize hooks.";
        return false;
    }

    GET_IFACE_CURRENT(GetEngineFactory, engine, IVEngineServer, INTERFACEVERSION_VENGINESERVER);
    GET_IFACE_CURRENT(GetEngineFactory, icvar, ICvar, CVAR_INTERFACE_VERSION);
    GET_IFACE_ANY(GetServerFactory, gameclients, IServerGameClients, INTERFACEVERSION_SERVERGAMECLIENTS);
    GET_IFACE_ANY(GetEngineFactory, g_pNetworkMessages, INetworkMessages, NETWORKMESSAGES_INTERFACE_VERSION);
    GET_IFACE_CURRENT(GetEngineFactory, g_pGameEventSystem, IGameEventSystem, GAMEEVENTSYSTEM_INTERFACE_VERSION);

    void* eventManagerVTable = AccessVTable("server", "CGameEventManager");

    loadEventFromFileHookID = SH_ADD_DVPHOOK(IGameEventManager2, LoadEventsFromFile, (IGameEventManager2*)((void*)eventManagerVTable), SH_MEMBER(this, &UtilsExtension::LoadEventsFromFile), false);
    SH_ADD_HOOK_MEMFUNC(IServerGameClients, ClientDisconnect, gameclients, this, &UtilsExtension::Hook_ClientDisconnect, true);

    return true;
}

int UtilsExtension::LoadEventsFromFile(const char* filePath, bool searchAll)
{
    if(!g_gameEventManager) {
        g_gameEventManager = META_IFACEPTR(IGameEventManager2);
    }

    RETURN_META_VALUE(MRES_IGNORED, 0);
}

extern uint64_t bhop_state;

void UtilsExtension::Hook_ClientDisconnect( CPlayerSlot slot, ENetworkDisconnectionReason reason, const char *pszName, uint64 xuid, const char *pszNetworkID )
{
    if((bhop_state & (1ULL << slot.Get())) != 0) bhop_state &= ~(1ULL << slot.Get());
}

ConVar* FetchCVar(std::string cvarname)
{
    if (!icvar)
        return nullptr;

    ConVarHandle cvarHandle = icvar->FindConVar(cvarname.c_str());
    if (!cvarHandle.IsValid())
        return nullptr;

    return icvar->GetConVar(cvarHandle);
}

void Hook_CCSPlayer_MovementServices_CheckJumpPre(void* services, void* movementData)
{
    if (autobunnyhoppingcvar == nullptr)
        autobunnyhoppingcvar = FetchCVar("sv_autobunnyhopping");

    bool& autobunnyhopping = *reinterpret_cast<bool*>(&autobunnyhoppingcvar->values);

    if (!autobunnyhopping)
    {
        CHandle<CEntityInstance> controller = SDKGetProp<CHandle<CEntityInstance>>(((CPlayer_MovementServices*)services)->m_pPawn, "CBasePlayerPawn", "m_hController");
        int pid = controller.GetEntryIndex() - 1;
        if((bhop_state & (1ULL << pid)) != 0) {
            autobunnyhopping = true;

            TCCSPlayer_MovementServices_CheckJumpPre(services, movementData);

            autobunnyhopping = false;
            return;
        }
    }

    TCCSPlayer_MovementServices_CheckJumpPre(services, movementData);
}

bool UtilsExtension::Unload(std::string& error)
{
    SH_REMOVE_HOOK_ID(loadEventFromFileHookID);
    SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, ClientDisconnect, gameclients, this, &UtilsExtension::Hook_ClientDisconnect, true);

    UnloadHooks();
    return true;
}

void UtilsExtension::AllExtensionsLoaded()
{

}

void UtilsExtension::AllPluginsLoaded()
{

}

bool UtilsExtension::OnPluginLoad(std::string pluginName, void* pluginState, PluginKind_t kind, std::string& error)
{

    if (kind == PluginKind_t::Lua) {
        lua_State* state = (lua_State*)pluginState;

        luabridge::getGlobalNamespace(state)
            .beginClass<PlayerUtils>("PlayerUtils")
            .addConstructor<void (*)(std::string)>()
            .addFunction("SetBunnyhop", &PlayerUtils::SetBunnyhop)
            .addFunction("GetBunnyhop", &PlayerUtils::GetBunnyhop)
            .addFunction("IsListeningToGameEvent", &PlayerUtils::IsListeningToGameEvent)
            .endClass();

        luaL_dostring(state, "playerutils = PlayerUtils(GetCurrentPluginName())");
    }

    return true;
}

bool UtilsExtension::OnPluginUnload(std::string pluginName, void* pluginState, PluginKind_t kind, std::string& error)
{
    if (kind == PluginKind_t::Lua) {
        lua_State* state = (lua_State*)pluginState;

        luaL_dostring(state, "playerutils = nil");
    }
    return true;
}

const char* UtilsExtension::GetAuthor()
{
    return "Swiftly Development Team";
}

const char* UtilsExtension::GetName()
{
    return "Utils Extension API";
}

const char* UtilsExtension::GetVersion()
{
#ifndef VERSION
    return "Local";
#else
    return VERSION;
#endif
}

const char* UtilsExtension::GetWebsite()
{
    return "https://swiftlycs2.net/";
}
