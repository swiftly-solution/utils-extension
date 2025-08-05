using SwiftlyS2.Internal_API;

namespace SwiftlyS2.API.Extensions
{
    public class PlayerUtils
    {
        private static IntPtr _ctx = IntPtr.Zero;

        private static void InitializeContext()
        {
            if (_ctx != IntPtr.Zero) return;
            _ctx = Invoker.CallNative<IntPtr>("PlayerUtils", "PlayerUtils", CallKind.ClassFunction);
        }

        public static void SetBunnyhop(int playerid, bool state)
        {
            InitializeContext();
            Invoker.CallNative("PlayerUtils", "SetBunnyhop", CallKind.ClassFunction, _ctx, playerid, state);
        }

        public static bool IsListeningToGameEvent(int playerid, string game_event)
        {
            InitializeContext();
            return Invoker.CallNative<bool>("PlayerUtils", "IsListeningToGameEvent", CallKind.ClassFunction, _ctx, playerid, game_event);
        }

        public static bool GetBunnyhop(int playerid)
        {
            InitializeContext();
            return Invoker.CallNative<bool>("PlayerUtils", "GetBunnyhop", CallKind.ClassFunction, _ctx, playerid);
        }
    }
}