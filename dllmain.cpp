// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include "MemoryMgr.h"
#include <string>
#include <iostream>
#define SOL_ALL_SAFTIES_ON 1
#include <sol/sol.hpp>
#include "umvc3utils.h"

struct sBattleSetting {
    void* vtable;
    char pad[0x30];
    struct character {
        void* vtable;
        INT32 mTeam;
        INT32 mType;
        INT32 unknown;
        INT32 mBody;
        INT32 mCpu;
        INT32 assist;
        char pad[0x38];
    };
    character characters[6];
    char pad2[0x104];
    INT32 battle_type;
    char pad3[0x21];
    std::byte input_key_disp;
    std::byte finish_picture_save;
    std::byte replay_use;
    std::byte damage_disp;
};

sBattleSetting* getBattleSetting() {
 sBattleSetting* battleSetting = *reinterpret_cast<sBattleSetting**>(_addr(0x140d50e58));
    return battleSetting;
}



DWORD WINAPI Initialise(LPVOID lpreserved) {
    sol::state lua;

    AllocConsole();

    freopen("CONIN$", "r", stdin);
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);


    lua.open_libraries(sol::lib::base);


    lua.script("print('bark bark bark!')");
    lua.set_function("getBattleSetting", &getBattleSetting);
    /*
    lua.new_usertype<sBattleSetting>("sBattleSetting",
        "input_key_disp", &sBattleSetting::input_key_disp
        );
        */
    lua.new_usertype<sBattleSetting>("sBattleSetting",
        "battle_type", &sBattleSetting::battle_type,
        "input_key_disp", &sBattleSetting::input_key_disp,
        "damage_disp", &sBattleSetting::damage_disp,
        "characters", sol::property([](sBattleSetting& c) { return std::ref(c.characters); })
        );

    lua.new_usertype<sBattleSetting::character>("sBattleSetting::character",
        "mTeam", &sBattleSetting::character::mTeam,
        "mType", &sBattleSetting::character::mType,
        "mCpu", &sBattleSetting::character::mCpu,
        "mBody", &sBattleSetting::character::mBody,
        "assist", &sBattleSetting::character::assist,
        "vtable", &sBattleSetting::character::vtable
        );

    printf("UMvC3 Script Engine Activated! :)\n");
    for (std::string line; std::getline(std::cin, line);) {
        try {
            auto result1 = lua.safe_script(line);
        }
        catch (const sol::error& e) {
            std::cout << "an expected error has occurred: " << e.what() << std::endl;
        }
    }

    return TRUE;
}


bool CheckGame()
{
    char* gameName = (char*)(_addr(0x140B12D10));

    if (strcmp(gameName, "umvc3") == 0)
    {
        return true;
    }
    else
    {
        MessageBoxA(0, "Invalid game version!\numvc3 script engine only supports latest Steam executable.", 0, MB_ICONINFORMATION);
        return false;
    }
}


BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        if (CheckGame()) {
            CreateThread(nullptr, 0, Initialise, hModule, 0, nullptr);
        }
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

