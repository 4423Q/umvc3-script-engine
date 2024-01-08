// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include "MemoryMgr.h"
#include <string>
#include <iostream>
#define SOL_ALL_SAFTIES_ON 1
#include <sol/sol.hpp>
#include "umvc3utils.h"
#include "UMvC3.h"

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

DWORD WINAPI Initialise(LPVOID lpreserved) {
    sol::state lua;

    AllocConsole();

    freopen("CONIN$", "r", stdin);
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);

    lua.open_libraries(sol::lib::base);

    typeLuaSBattleSetting(&lua);
    typeLuaSAction(&lua);

    try {
        lua.safe_script_file("config.lua");
    } catch (const sol::error& e) {
            std::cout << "Error loading config: " << e.what() << std::endl;
    }
    
    auto display = lua.get_or<bool>("_UmSE_DISPLAY", 0);

    std::cout << "Display:" << display;
    if (display == 1) {
        std::cout << "Display === 1" << std::endl;
    }

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

