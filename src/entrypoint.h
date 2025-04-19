#ifndef _entrypoint_h
#define _entrypoint_h

#include <string>

#include <public/igameevents.h>
#include <public/engine/igameeventsystem.h>
#include <networksystem/inetworkserializer.h>
#include <networksystem/inetworkmessages.h>
#include <networksystem/inetworksystem.h>

#include <swiftly-ext/core.h>
#include <swiftly-ext/extension.h>
#include <swiftly-ext/hooks/function.h>
#include <swiftly-ext/hooks/vfunction.h>

class UtilsExtension : public SwiftlyExt
{
public:
    bool Load(std::string& error, SourceHook::ISourceHook* SHPtr, ISmmAPI* ismm, bool late);
    bool Unload(std::string& error);

    void AllExtensionsLoaded();
    void AllPluginsLoaded();

    bool OnPluginLoad(std::string pluginName, void* pluginState, PluginKind_t kind, std::string& error);
    bool OnPluginUnload(std::string pluginName, void* pluginState, PluginKind_t kind, std::string& error);

    void Hook_ClientDisconnect(CPlayerSlot slot, ENetworkDisconnectionReason reason, const char* pszName, uint64 xuid, const char* pszNetworkID);
    int LoadEventsFromFile(const char* filePath, bool searchAll);

public:
    const char* GetAuthor();
    const char* GetName();
    const char* GetVersion();
    const char* GetWebsite();
};

extern UtilsExtension g_Ext;
extern IServerGameClients* gameclients;
extern IGameEventManager2* g_gameEventManager;
extern IGameEventSystem* g_pGameEventSystem;
extern IVEngineServer2* engine;
extern ICvar* icvar;
DECLARE_GLOBALVARS();

#endif