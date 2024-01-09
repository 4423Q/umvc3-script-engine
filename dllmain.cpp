// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include "MemoryMgr.h"
#include <string>
#include <string_view>
#include <iostream>
#define SOL_ALL_SAFTIES_ON 1
#include <sol/sol.hpp>
#include "umvc3utils.h"
#include "UMvC3.h"
#include <mutex>
#include "Trampoline.h"
#include "MemoryMgr.h"
#include <WinUser.h>
#include <map>
#include <filesystem>
namespace fs = std::filesystem;

sol::state lua;
std::mutex lua_mutex;
std::map<std::string, sol::table> modules;

sAction* getSAction()
{
    sAction* action = *reinterpret_cast<sAction**>(_addr(0x140d47e68));
    return action;
}

sBattleSetting* getBattleSetting()
{
    sBattleSetting* battleSetting = *reinterpret_cast<sBattleSetting**>(_addr(0x140d50e58));
    return battleSetting;
}


void typeLuaSBattleSetting(sol::state* lua) {

    lua->set_function("getBattleSetting", &getBattleSetting);
    lua->new_usertype<sBattleSetting>("sBattleSetting",
        "battle_type", &sBattleSetting::battle_type,
        "battle_ui_disp", &sBattleSetting::battle_ui_disp,
        "arcade_dif", &sBattleSetting::arcade_dif,
        "battle_flag", &sBattleSetting::battle_flag,
        "stage_id", &sBattleSetting::stage_id,
        "mTimeLimit", &sBattleSetting::mTimeLimit,
        "input_key_disp", &sBattleSetting::input_key_disp,
        "damage_disp", &sBattleSetting::damage_disp,
        "replay_use", &sBattleSetting::replay_use,
        "finish_picture_save", &sBattleSetting::finish_picture_save,
        "characters", sol::property([](sBattleSetting& c) { return std::ref(c.characters); })
        );

    lua->new_usertype<sBattleSetting::character>("sBattleSetting::character",
        "mTeam", &sBattleSetting::character::mTeam,
        "mType", &sBattleSetting::character::mType,
        "mCpu", &sBattleSetting::character::mCpu,
        "mBody", &sBattleSetting::character::mBody,
        "assist", &sBattleSetting::character::assist,
        "vtable", &sBattleSetting::character::vtable
        );

}


void typeLuaSAction(sol::state* lua) {
    lua->set_function("getSAction", &getSAction);
    lua->new_usertype<sAction>("sAction",
        "mStep", &sAction::mStep,
        "mStepOrder", &sAction::mStepOrder,
    "mSubStep", &sAction::mSubStep,
    "mStepTime", &sAction::mStepTime,
        "mStepTime2", &sAction::mStepTime2,
    "mStepTimeWin", &sAction::mStepTimeWin,
    "mStepFrame", &sAction::mStepFrame,
        "mRound", &sAction::mRound,

        "mRTimeLimit", &sAction::mRTimeLimit,
    "mRTimePass", &sAction::mRTimePass,
    "mRTimePass2", &sAction::mRTimePass2,
    "mTimePass",&sAction::mTimePass,
    "mHitAfter", &sAction::mHitAfter,
    "mNoInputRestart", &sAction::mNoInputRestart,
    "mFinishWatch", &sAction::mFinishWatch,
    "mWinTeam", &sAction::mWinTeam,
        "mRestartMode", &sAction::mRestartMode,
        "mBtnRestart", &sAction::mBtnRestart
       );

}

BYTE oldKeyState[256];

void OnPostInput(uCharacter* character) {
    
    BYTE keyState[256];
    int temp;
    int temp2;

    if (character->mTeamId != 0) return; // This is a lazy way to do keyboard input

    memset(keyState, 0, sizeof(256));
    GetKeyState(0);
    if (GetKeyboardState(keyState)) {
        for (int i = 0; i < 256; i++)
        {
            temp = (int)keyState[i];
            temp >>= 7;
            temp2 = (int)oldKeyState[i];
            temp2 >>= 7;
            if (temp != 0 && temp2 == 0) {
                lua_mutex.lock();
                for (const auto& [name, table] : modules) {

                    sol::optional<sol::function> onkeydown = table["unstable_onkeyboarddown"];
                    if (onkeydown != sol::nullopt) {
                        onkeydown.value()(i);
                    }
                }
                lua_mutex.unlock();
            }
            oldKeyState[i] = keyState[i];
        }
    }
    else {
        printf("Error getting keyboard state");
    }
    
}

int HookPostInputUpdate(uCharacter* character) {
    OnPostInput(character);

    return character->mTeamId;
}

void InstallInputHook()
{
    Trampoline* tramp = Trampoline::MakeTrampoline(GetModuleHandle(nullptr));
    Memory::VP::InjectHook(_addr(0x14002e280), tramp->Jump(HookPostInputUpdate), PATCH_CALL);
}

void registerModule(std::string name, sol::table mod) {
    printf("Loading module %s\n", name.c_str());
    modules[name] = mod;
}

DWORD WINAPI Initialise(LPVOID lpreserved) {
    memset(oldKeyState, 0, sizeof(256));

    lua_mutex.lock();
    lua.open_libraries(sol::lib::base);

    lua["umse_register_module"] = registerModule;
    typeLuaSBattleSetting(&lua);
    typeLuaSAction(&lua);

    try {
        lua.safe_script_file("umse/config.lua");
    }
    catch (const sol::error& e) {
        std::cout << "Error loading config: " << e.what() << std::endl;
    }

    auto display = lua.get_or<bool>("_UmSE_DISPLAY", 0);

    std::cout << "Display:" << display;
    if (display == 1) {
        std::cout << "Display === 1" << std::endl;

        AllocConsole();

        freopen("CONIN$", "r", stdin);
        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);
    }


    std::string path = "umse/scripts";
    for (const auto& entry : fs::directory_iterator(path))
    {
        std::string filename = entry.path().filename().string();
        std::string::size_type n = filename.find(".lua", filename.length() - 4);
        if (n != std::string::npos) {
           
           printf("Loading script, %s\n",filename.c_str());
           lua.safe_script_file(entry.path().string());
        }
    }

    lua_mutex.unlock();

    InstallInputHook();

    printf("UMvC3 Script Engine Activated! :)\n");
    for (std::string line; std::getline(std::cin, line);) {
        try {
            lua_mutex.lock();
            auto result1 = lua.safe_script(line);
        }
        catch (const sol::error& e) {
            std::cout << "an expected error has occurred: " << e.what() << std::endl;
        }

        lua_mutex.unlock();
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

