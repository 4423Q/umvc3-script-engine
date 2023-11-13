// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include "MemoryMgr.h"
#include <string>
#include <iostream>
#define SOL_ALL_SAFTIES_ON 1
#include <sol/sol.hpp>
#include "umvc3utils.h"

DWORD WINAPI Initialise(LPVOID lpreserved) {
    sol::state lua;

    AllocConsole();

    freopen("CONIN$", "r", stdin);
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);


    lua.open_libraries(sol::lib::base);

    lua.script("print('bark bark bark!')");


    printf("Initialise() | Begin!\n");
    for (std::string line; std::getline(std::cin, line);) {
        lua.script(line);
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
        MessageBoxA(0, "Invalid game version!\nmod_enable_damage_counter only supports latest Steam executable.", 0, MB_ICONINFORMATION);
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

