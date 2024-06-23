#include <Windows.h>
#include <string>
#include <algorithm>
#include <iostream>
#include <filesystem>

struct Patch {
    DWORD64 relAddr;
    DWORD size;
    char patch[50];
    char orig[50];
};

enum GAME {
    DS3,
    SEKIRO,
    ELDENRING,
    UNKNOWN
};

GAME Game;

typedef DWORD64(__stdcall *DIRECTINPUT8CREATE)(HINSTANCE, DWORD, REFIID, LPVOID *, LPUNKNOWN);
DIRECTINPUT8CREATE fpDirectInput8Create;
//TODO: fix type mismatch between DWORD64 and HRESULT?
extern "C" __declspec(dllexport)  HRESULT __stdcall DirectInput8Create(
    HINSTANCE hinst,
    DWORD dwVersion,
    REFIID riidltf,
    LPVOID * ppvOut,
    LPUNKNOWN punkOuter
)
{
    return fpDirectInput8Create(hinst, dwVersion, riidltf, ppvOut, punkOuter);
}
struct MatchPathSeparator
{
    bool operator()(char ch) const
    {
        return ch == '\\' || ch == '/';
    }
};
std::string
basename(std::string const& pathname)
{
    return std::string(
        std::find_if(pathname.rbegin(), pathname.rend(),
            MatchPathSeparator()).base(),
        pathname.end());
}

GAME DetermineGame() {
    const int fnLenMax = 200;
    char fnPtr[fnLenMax];
    auto fnLen = GetModuleFileNameA(0, fnPtr, fnLenMax);

    auto fileName = basename(std::string(fnPtr, fnLen));
    std::transform(fileName.begin(), fileName.end(), fileName.begin(), ::tolower);
    if (fileName == "darksoulsiii.exe") {
        return GAME::DS3;
    }
    else if (fileName == "sekiro.exe") {
        return GAME::SEKIRO;
    }
    else if (fileName == "eldenring.exe" || fileName == "start_protected_game.exe") {
        return GAME::ELDENRING;
    }
    else {
        return GAME::UNKNOWN;
    }
}

int applyPatches(Patch * patches, int patchCount)
{
    auto baseAddr = GetModuleHandle(NULL);
    int matches = 0;
    for (auto i = 0; i < patchCount; i++) {
        auto patch = patches[i];
        auto addr = (void*)((DWORD64)baseAddr + patch.relAddr);
        auto size = patch.size;

        if (memcmp(addr, patch.orig, size) == 0) {
            DWORD old;
            VirtualProtect(addr, size, PAGE_EXECUTE_READWRITE, &old);
            memcpy(addr, patch.patch, size);
            VirtualProtect(addr, size, old, &old);
            matches++; //some games require multiple patches, so continue
        }
    }
    return matches;
}

void SetupD8Proxy() {
    char syspath[320];
    GetSystemDirectoryA(syspath, 320);
    strcat_s(syspath, "\\dinput8.dll");
    auto hMod = LoadLibraryA(syspath);
    fpDirectInput8Create = (DIRECTINPUT8CREATE)GetProcAddress(hMod, "DirectInput8Create");
}

DWORD WINAPI doPatching(LPVOID lpParam)
{
    //patch succeeded, wait a moment before applying stutter fix; the target class needs to have initialised.
    Sleep(5000);
    
    int usrInputOffset = 0;
    int flagOffset = 0;
    auto baseAddr = GetModuleHandle(NULL);
    if (Game == GAME::DS3) {
        //for 1.15.2. TODO: support or at least recognise other patches
        usrInputOffset = 0x49644D0;
        flagOffset = 0x24b;
    }
    else if (Game == GAME::SEKIRO) {
        //for 1.06. TODO: support or at least recognise other patches
        usrInputOffset = 0x3F42B28;
        flagOffset = 0x23b;
    }
    else if (Game == GAME::ELDENRING) {
        //for 1.12. TODO: support or at least recognise other patches
        usrInputOffset = 0x485DB68;
        flagOffset = 0x88b;
    }

    if (usrInputOffset == 0)
    {
        MessageBoxA(0, "FromStutterFix failed. You may be running an unsupported version.", "", 0);
        return 1;
    }

    auto usrInputPtr = (uint8_t**)((DWORD64)baseAddr + usrInputOffset);
    int i = 0;
    while ((DWORD64)*usrInputPtr < (DWORD64)baseAddr || (DWORD64)*usrInputPtr > 0x800000000000LL)
    {//less than base address is possible but is unlikely
        i++;
        if (i > 60)
        {
            MessageBoxA(0, "FromStutterFix failed. You may be running an unsupported version.", "", 0);
            return 1;
        }
        Sleep(500);
    }
    
    auto ptrFlag = *usrInputPtr + flagOffset;
    if (*ptrFlag == 0)
    {
        *ptrFlag = 1;

        if (Game == GAME::ELDENRING && std::filesystem::exists("mods/achievement"))
        {
            auto trophyImpPtr = (uint8_t***)((DWORD64)baseAddr + 0x4589478); //CS::CSTrophyImp, 1.12.
            i = 0;
            while ((DWORD64)*trophyImpPtr < 0x700000000000LL || (DWORD64)*trophyImpPtr > 0x800000000000LL) //seems normal for this one to be below the base
            {
                i++;
                if (i > 60)
                {
                    MessageBoxA(0, "Achievement disable failed.", "", 0);
                    return 1;
                }
                Sleep(500);
            }
            auto ptrAchieve = (*(*trophyImpPtr + 1 /* +1 pointer ie. 8 bytes*/) + 0x4c);
            if (*ptrAchieve == 1)
            {
                *ptrAchieve = 0;
                PlaySound(TEXT("SystemStart"), NULL, SND_SYNC);
#if _DEBUG
                MessageBoxA(0, "Achievement disabled.", "", 0);
                printf("Achievements disabled, %d\r\n", i);
#endif
            }
        }

        //Beep(2000, 250); //annoying AF
#if _DEBUG
        printf("Patch success, i %d\r\n", i);
#endif
    }

    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
)
{
    HANDLE res = 0;
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        Game = DetermineGame();
        if (Game == GAME::UNKNOWN) {
            MessageBoxA(0, "Unable to determine game. Valid EXEs are darksoulsiii.exe, sekiro.exe, elden_ring.exe and start_protected_game.exe", "", 0);
            break; //game will likely crash without the real dinput8 being loaded, but that's okay.
        }

        SetupD8Proxy();
        
        res = CreateThread(NULL, 0, doPatching, NULL, 0, NULL);
        if (res == NULL)
        {
            MessageBoxA(0, "Could not start patching thread.", "", 0);
        }

        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
