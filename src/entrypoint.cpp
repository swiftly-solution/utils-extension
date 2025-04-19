#include "entrypoint.h"
#include <embedder/src/embedder.h>
#include <ehandle.h>
#include "sdk/services.h"
#include "sdk/CRecipientFilters.h"
#include "networkbasetypes.pb.h"
#include <swiftly-ext/sdk.h>
#include <swiftly-ext/memory.h>

dyno::ReturnAction Hook_CCSPlayer_MovementServices_CheckJumpPre(dyno::CallbackType cbType, dyno::IHook& hook);

//////////////////////////////////////////////////////////////
/////////////////        Core Variables        //////////////
////////////////////////////////////////////////////////////

SH_DECL_HOOK5_void(IServerGameClients, ClientDisconnect, SH_NOATTRIB, 0, CPlayerSlot, ENetworkDisconnectionReason, const char*, uint64, const char*);
SH_DECL_HOOK2(IGameEventManager2, LoadEventsFromFile, SH_NOATTRIB, 0, int, const char*, bool);

int loadEventFromFileHookID = -1;
IServerGameClients* gameclients = nullptr;
IGameEventManager2* g_gameEventManager = nullptr;
IGameEventSystem* g_pGameEventSystem = nullptr;
IVEngineServer2* engine = nullptr;
ICvar* icvar = nullptr;

UtilsExtension g_Ext;
CREATE_GLOBALVARS();

FunctionHook CCSPlayer_MovementServices_CheckJumpPre("CCSPlayer_MovementServices_CheckJumpPre", dyno::CallbackType::Pre, Hook_CCSPlayer_MovementServices_CheckJumpPre, "pp", 'v');
FunctionHook CCSPlayer_MovementServices_CheckJumpPost("CCSPlayer_MovementServices_CheckJumpPre", dyno::CallbackType::Post, Hook_CCSPlayer_MovementServices_CheckJumpPre, "pp", 'v');

ConVar* autobunnyhoppingcvar = nullptr;

//////////////////////////////////////////////////////////////
/////////////////          Core Class          //////////////
////////////////////////////////////////////////////////////

EXT_EXPOSE(g_Ext);
bool UtilsExtension::Load(std::string& error, SourceHook::ISourceHook* SHPtr, ISmmAPI* ismm, bool late)
{
    g_SHPtr = SHPtr;
    g_SMAPI = ismm;

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
    if (!g_gameEventManager) {
        g_gameEventManager = META_IFACEPTR(IGameEventManager2);
    }

    RETURN_META_VALUE(MRES_IGNORED, 0);
}

uint64_t bhop_state = 0;

void UtilsExtension::Hook_ClientDisconnect(CPlayerSlot slot, ENetworkDisconnectionReason reason, const char* pszName, uint64 xuid, const char* pszNetworkID)
{
    if ((bhop_state & (1ULL << slot.Get())) != 0) bhop_state &= ~(1ULL << slot.Get());
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

dyno::ReturnAction Hook_CCSPlayer_MovementServices_CheckJumpPre(dyno::CallbackType cbType, dyno::IHook& hook)
{
    void* services = hook.getArgument<void*>(0);

    if (autobunnyhoppingcvar == nullptr)
        autobunnyhoppingcvar = FetchCVar("sv_autobunnyhopping");

    bool& autobunnyhopping = *reinterpret_cast<bool*>(&autobunnyhoppingcvar->values);

    if (!autobunnyhopping)
    {
        CHandle<CEntityInstance> controller = SDKGetProp<CHandle<CEntityInstance>>(((CPlayer_MovementServices*)services)->m_pPawn, "CBasePlayerPawn", "m_hController");
        int pid = controller.GetEntryIndex() - 1;
        if ((bhop_state & (1ULL << pid)) != 0) {
            autobunnyhopping = (cbType == dyno::CallbackType::Pre);
            return dyno::ReturnAction::Ignored;
        }
    }

    return dyno::ReturnAction::Ignored;
}

bool UtilsExtension::Unload(std::string& error)
{
    SH_REMOVE_HOOK_ID(loadEventFromFileHookID);
    SH_REMOVE_HOOK_MEMFUNC(IServerGameClients, ClientDisconnect, gameclients, this, &UtilsExtension::Hook_ClientDisconnect, true);

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
    EContext* ctx = (EContext*)pluginState;

    ADD_CLASS("PlayerUtils");

#ifndef _WIN32
    ADD_CLASS_FUNCTION("PlayerUtils", "SetBunnyhop", [](FunctionContext* context, ClassData* data) -> void {
        int playerid = context->GetArgumentOr<int>(0, -1);
        bool state = context->GetArgumentOr<bool>(1, false);

        if (state == true) bhop_state |= (1ULL << playerid);
        else bhop_state &= ~(1ULL << playerid);

        INetworkMessageInternal* netmsg = g_pNetworkMessages->FindNetworkMessagePartial("SetConVar");
        auto msg = netmsg->AllocateMessage()->ToPB<CNETMsg_SetConVar>();
        CMsg_CVars_CVar* cvar = msg->mutable_convars()->add_cvars();
        cvar->set_name("sv_autobunnyhopping");
        cvar->set_value(state == true ? "true" : "false");

        CSingleRecipientFilter filter(playerid);
        g_pGameEventSystem->PostEventAbstract(-1, false, &filter, netmsg, msg, 0);

        delete msg;
        });
#else
    ADD_CLASS_FUNCTION("PlayerUtils", "SetBunnyhop", [](FunctionContext* context, ClassData* data) -> void {
        int playerid = context->GetArgumentOr<int>(0, -1);
        bool state = context->GetArgumentOr<bool>(1, false);

        if (state == true) bhop_state |= (1ULL << playerid);
        else bhop_state &= ~(1ULL << playerid);

        INetworkMessageInternal* netmsg = g_pNetworkMessages->FindNetworkMessagePartial("SetConVar");
        auto msg = netmsg->AllocateMessage()->ToPB<CNETMsg_SetConVar>();
        CMsg_CVars_CVar* cvar = msg->mutable_convars()->add_cvars();
        cvar->set_name("sv_autobunnyhopping");
        cvar->set_value(state == true ? "true" : "false");

        CSingleRecipientFilter filter(playerid);
        g_pGameEventSystem->PostEventAbstract(-1, false, &filter, netmsg, msg, 0);
        });
#endif

    ADD_CLASS_FUNCTION("PlayerUtils", "IsListeningToGameEvent", [](FunctionContext* context, ClassData* data) -> void {
        int playerid = context->GetArgumentOr<int>(0, -1);
        std::string game_event = context->GetArgumentOr<std::string>(1, "");

        void* getListener = GetSignature("LegacyGameEventListener");
        if (!getListener) return context->SetReturn(false);

        IGameEventListener2* playerListener = reinterpret_cast<IGameEventListener2 * (*)(int)>(getListener)(playerid);
        if (!playerListener) return context->SetReturn(false);

        return context->SetReturn(g_gameEventManager->FindListener(playerListener, game_event.c_str()));
        });

    ADD_CLASS_FUNCTION("PlayerUtils", "GetBunnyhop", [](FunctionContext* context, ClassData* data) -> void {
        int playerid = context->GetArgumentOr<int>(0, -1);
        context->SetReturn(((bhop_state & (1ULL << playerid)) != 0));
        });

    ADD_VARIABLE("_G", "playerutils", MAKE_CLASS_INSTANCE_CTX(ctx, "PlayerUtils", {}));

    return true;
}

bool UtilsExtension::OnPluginUnload(std::string pluginName, void* pluginState, PluginKind_t kind, std::string& error)
{
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
