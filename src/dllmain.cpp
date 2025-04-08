#include "stdafx.h"
#include "helper.hpp"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <inipp/inipp.h>
#include <safetyhook.hpp>

#define spdlog_confparse(var) spdlog::info("Config Parse: {}: {}", #var, var)

HMODULE exeModule = GetModuleHandle(NULL);
HMODULE thisModule;

// Fix details
std::string sFixName = "ACShadowsFix";
std::string sFixVersion = "0.0.2";
std::filesystem::path sFixPath;

// Ini
inipp::Ini<char> ini;
std::string sConfigFile = sFixName + ".ini";

// Logger
std::shared_ptr<spdlog::logger> logger;
std::string sLogFile = sFixName + ".log";
std::filesystem::path sExePath;
std::string sExeName;

// Ini variables
bool bIntroSkip;
bool bExtendedFOV;
bool bDisableFPSLimit;
bool bAdjustClothPhysics;
float fClothPhysicsFramerate;
bool bCutsceneFrameGen;
bool bDisableCutscenePillarboxing;
bool bDisablePhotoModePillarboxing;

// Variables
int iCurrentResX;
int iCurrentResY;
bool bIntroSkipped = false;

void Logging()
{
    // Get path to DLL
    WCHAR dllPath[_MAX_PATH] = {0};
    GetModuleFileNameW(thisModule, dllPath, MAX_PATH);
    sFixPath = dllPath;
    sFixPath = sFixPath.remove_filename();

    // Get game name and exe path
    WCHAR exePath[_MAX_PATH] = {0};
    GetModuleFileNameW(exeModule, exePath, MAX_PATH);
    sExePath = exePath;
    sExeName = sExePath.filename().string();
    sExePath = sExePath.remove_filename();

    // Spdlog initialisation
    try
    {
        logger = spdlog::basic_logger_st(sFixName, sExePath.string() + sLogFile, true);
        spdlog::set_default_logger(logger);
        spdlog::flush_on(spdlog::level::debug);

        spdlog::info("----------");
        spdlog::info("{:s} v{:s} loaded.", sFixName, sFixVersion);
        spdlog::info("----------");
        spdlog::info("Log file: {}", sFixPath.string() + sLogFile);
        spdlog::info("----------");
        spdlog::info("Module Name: {:s}", sExeName);
        spdlog::info("Module Path: {:s}", sExePath.string());
        spdlog::info("Module Address: 0x{:x}", (uintptr_t)exeModule);
        spdlog::info("Module Timestamp: {:d}", Memory::ModuleTimestamp(exeModule));
        spdlog::info("----------");
    }
    catch (const spdlog::spdlog_ex &ex)
    {
        AllocConsole();
        FILE *dummy;
        freopen_s(&dummy, "CONOUT$", "w", stdout);
        std::cout << "Log initialisation failed: " << ex.what() << std::endl;
        FreeLibraryAndExitThread(thisModule, 1);
    }
}

void Configuration()
{
    // Inipp initialisation
    std::ifstream iniFile(sFixPath / sConfigFile);
    if (!iniFile)
    {
        AllocConsole();
        FILE *dummy;
        freopen_s(&dummy, "CONOUT$", "w", stdout);
        std::cout << "" << sFixName.c_str() << " v" << sFixVersion.c_str() << " loaded." << std::endl;
        std::cout << "ERROR: Could not locate config file." << std::endl;
        std::cout << "ERROR: Make sure " << sConfigFile.c_str() << " is located in " << sFixPath.string().c_str() << std::endl;
        spdlog::error("ERROR: Could not locate config file {}", sConfigFile);
        spdlog::shutdown();
        FreeLibraryAndExitThread(thisModule, 1);
    }
    else
    {
        spdlog::info("Config file: {}", sFixPath.string() + sConfigFile);
        ini.parse(iniFile);
    }

    // Parse config
    ini.strip_trailing_comments();
    spdlog::info("----------");

    // Load settings from ini
    inipp::get_value(ini.sections["Intro Skip"], "Enabled", bIntroSkip);
    inipp::get_value(ini.sections["Extended FOV Slider"], "Enabled", bExtendedFOV);
    inipp::get_value(ini.sections["Disable Framerate Limit"], "Enabled", bDisableFPSLimit);
    inipp::get_value(ini.sections["Cloth Physics Framerate"], "Enabled", bAdjustClothPhysics);
    inipp::get_value(ini.sections["Cloth Physics Framerate"], "Framerate", fClothPhysicsFramerate);
    inipp::get_value(ini.sections["Cutscene Frame Generation"], "Enabled", bCutsceneFrameGen);
    inipp::get_value(ini.sections["Disable Pillarboxing"], "Cutscenes", bDisableCutscenePillarboxing);
    inipp::get_value(ini.sections["Disable Pillarboxing"], "PhotoMode", bDisablePhotoModePillarboxing);

    // Clamp settings
    fClothPhysicsFramerate = std::clamp(fClothPhysicsFramerate, 0.00f, 500.00f);

    // Log ini parse
    spdlog_confparse(bIntroSkip);
    spdlog_confparse(bExtendedFOV);
    spdlog_confparse(bDisableFPSLimit);
    spdlog_confparse(bAdjustClothPhysics);
    spdlog_confparse(fClothPhysicsFramerate);
    spdlog_confparse(bCutsceneFrameGen);
    spdlog_confparse(bDisableCutscenePillarboxing);
    spdlog_confparse(bDisablePhotoModePillarboxing);

    spdlog::info("----------");
}

void IntroSkip()
{
    if (bIntroSkip) 
    {
        // Intro skip
        std::uint8_t* IntroSkipScanResult = Memory::PatternScan(exeModule,"48 ?? ?? 74 ?? 48 8B ?? 48 85 ?? 48 8D ?? ?? ?? ?? ?? 48 ?? ?? ?? 48 89 ?? E8 ?? ?? ?? ?? C6 ?? ?? 01 48 89 ??");
        if (IntroSkipScanResult) {
            spdlog::info("Intro Skip: Address is {:s}+{:x}", sExeName.c_str(), IntroSkipScanResult - (std::uint8_t*)exeModule);
            static SafetyHookMid IntroSkipMidHook{};
            IntroSkipMidHook = safetyhook::create_mid(IntroSkipScanResult + 0x3,
                [](SafetyHookContext& ctx) {
                    if (!ctx.rdx) return;

                    if (!bIntroSkipped) {
                        char* vidName = *(char**)ctx.rdx;
                        ctx.rflags |= (1ULL << 6); // Set ZF to skip playback
                        
                        // The ubisoft logo is last so we know the intro skip is done when that plays
                        if (strncmp(vidName, "UbisoftLogo.webm", sizeof("UbisoftLogo.webm") - 1) == 0) {
                            spdlog::info("Intro Skip: Skipped intro logos.");
                            bIntroSkipped = true;
                        }
                    }
                });
        }
        else {
            spdlog::error("Intro Skip: Pattern scan failed.");
        }
    }   
}

void FOV()
{
    if (bExtendedFOV) 
    {
        // FOV slider
        std::uint8_t* FOVSliderPercentageScanResult = Memory::PatternScan(exeModule, "E9 ?? ?? ?? ?? 48 ?? ?? 48 ?? ?? 48 ?? ?? FF ?? ?? ?? ?? ?? 48 ?? ?? ?? ?? C5 ?? ?? ??");
        std::uint8_t* FOVSliderMultiplierScanResult = Memory::PatternScan(exeModule, "77 ?? 48 8B ?? ?? ?? ?? ?? 48 85 ?? 74 ?? 48 8B ?? FF ?? ?? ?? ?? ?? C5 FA ?? ?? ?? C5 FA ?? ?? ?? ?? ?? ?? 48 8B ??");
        if (FOVSliderPercentageScanResult && FOVSliderMultiplierScanResult) {
            spdlog::info("FOV Slider: Percentage: Address is {:s}+{:x}", sExeName.c_str(), FOVSliderPercentageScanResult - (std::uint8_t*)exeModule);
            static SafetyHookMid FOVSliderPercentageMidHook{};
            FOVSliderPercentageMidHook = safetyhook::create_mid(FOVSliderPercentageScanResult + 0x8,
                [](SafetyHookContext& ctx) {
                    if (!ctx.rdi) return;

                    // Get slider min/max
                    float* fMin = reinterpret_cast<float*>(ctx.rdi + 0x38);
                    float* fMax = reinterpret_cast<float*>(ctx.rdi + 0x3C);

                    // FOV slider is 85-115
                    if (*fMin == 85.00f && *fMax == 115.00f) {
                        // Widen to 70-150
                        *fMin = 70.00f;
                        *fMax = 150.00f;
                    }
                });

            spdlog::info("FOV Slider: Multiplier: Address is {:s}+{:x}", sExeName.c_str(), FOVSliderMultiplierScanResult - (std::uint8_t*)exeModule);
            static SafetyHookMid FOVSliderMultiplierMidHook{};
            FOVSliderMultiplierMidHook = safetyhook::create_mid(FOVSliderMultiplierScanResult + 0x9,
                [](SafetyHookContext& ctx) {
                    if (!ctx.rcx) return; // RCX is loaded from RSI in the previous instruction

                    // Widen the FOV multiplier range to match
                    char* currentSetting = *reinterpret_cast<char**>(ctx.rsi + 0x18);
                    if (strncmp(currentSetting, "FieldOfViewMultiplier", sizeof("FieldOfViewMultiplier") - 1) == 0) {
                        *reinterpret_cast<float*>(ctx.rcx + 0x8) = 0.70f;
                        *reinterpret_cast<float*>(ctx.rcx + 0xC) = 1.50f;
                    }
                });
        }
        else {
            spdlog::error("FOV Slider: Pattern scan(s) failed.");
        }
    }
}

void Pillarboxing()
{
    if (bDisableCutscenePillarboxing) 
    {
        // Cutscene pillarboxing
        std::uint8_t* CutscenePillarboxingScanResult = Memory::PatternScan(exeModule,"84 C0 74 ?? C5 FA ?? ?? ?? ?? C5 FA ?? ?? ?? ?? C5 FA ?? ?? C5 ?? 57 ?? C5 FA ?? ??");
        if (CutscenePillarboxingScanResult) {
            spdlog::info("Cutscene Pillarboxing: Address is {:s}+{:x}", sExeName.c_str(), CutscenePillarboxingScanResult - (std::uint8_t*)exeModule);
            Memory::PatchBytes(CutscenePillarboxingScanResult + 0x2, "\xEB", 1);
            spdlog::info("Cutscene Pillarboxing: Patched instruction.");
        }
        else {
            spdlog::error("Cutscene Pillarboxing: Pattern scan failed.");
        }
    }  
}

void Framerate()
{
    if (bDisableFPSLimit) 
    {
        // Framerate cap
        std::uint8_t* FramerateCapScanResult = Memory::PatternScan(exeModule,"C5 F8 ?? ?? 0F 83 ?? ?? ?? ?? C5 FA ?? ?? ?? ?? ?? ?? 48 8B ?? ?? ?? ?? ?? 48 85 ?? 78 ??");
        if (FramerateCapScanResult) {
            spdlog::info("Framerate: Cap: Address is {:s}+{:x}", sExeName.c_str(), FramerateCapScanResult - (std::uint8_t*)exeModule);
            Memory::PatchBytes(FramerateCapScanResult + 0x3, "\xC9\x0F\x84", 3);
            spdlog::info("Framerate: Cap: Patched instruction.");
        }
        else {
            spdlog::error("Framerate: Cap: Pattern scan failed.");
        }
    }

    if (bAdjustClothPhysics)
    {
        // Cloth physics
        std::uint8_t* ClothPhysicsScanResult = Memory::PatternScan(exeModule, "4C 8B ?? ?? 49 8B ?? ?? 45 0F ?? ?? ?? ?? ?? ?? 45 0F ?? ?? ?? ?? ?? ?? 41 ?? ?? ?? 49 ?? ?? 01");
        if (ClothPhysicsScanResult) {
            spdlog::info("Framerate: Cloth Physics: Address is {:s}+{:x}", sExeName.c_str(), ClothPhysicsScanResult - (std::uint8_t*)exeModule);
            static SafetyHookMid ClothPhysicsMidHook{};
            ClothPhysicsMidHook = safetyhook::create_mid(ClothPhysicsScanResult,
                [](SafetyHookContext& ctx) {
                    // By default the game appears to use 60fps cloth physics during gameplay and 30fps cloth physics during cutscenes

                    if (ctx.rdx + 0x70 && fClothPhysicsFramerate == 0.00f) {
                        // Use current frametime for cloth physics instead of fixed 0.01666/0.03333 values
                        if (uintptr_t pFramerate = *reinterpret_cast<uintptr_t*>(ctx.rdx + 0x70))
                            ctx.xmm0.f32[0] = *reinterpret_cast<float*>(pFramerate + 0x78); // Current frametime
                    }
                    else {
                        // Set user defined cloth physics framerate
                        ctx.xmm0.f32[0] = 1.00f / fClothPhysicsFramerate;
                    }
                });             
        }
        else {
            spdlog::error("Framerate: Cloth Physics: Pattern scan failed.");
        }
    }
    
    if (bCutsceneFrameGen)
    {
        // Cutscene frame generation
        std::uint8_t* CutsceneFrameGenerationScanResult = Memory::PatternScan(exeModule,"80 ?? ?? 00 0F 84 ?? ?? ?? ?? 31 ?? 20 ?? 48 8B ?? 45 ?? ?? 80 ?? ?? ?? ?? ?? 00");
        if (CutsceneFrameGenerationScanResult) {
            spdlog::info("Framerate: Cutscene Frame Generation: Address is {:s}+{:x}", sExeName.c_str(), CutsceneFrameGenerationScanResult - (std::uint8_t*)exeModule);
            Memory::PatchBytes(CutsceneFrameGenerationScanResult + 0x3, "\xFF\x0F\x85", 3);
            spdlog::info("Framerate: Cutscene Frame Generation: Patched instruction.");
        }
        else {
            spdlog::error("Framerate: Cutscene Frame Generation: Pattern scan failed.");
        }
    }
}

void Misc()
{
    if (bDisablePhotoModePillarboxing) 
    {
        // Photo mode pillarboxing
        std::uint8_t* PhotoModePillarboxingScanResult = Memory::PatternScan(exeModule, "C4 ?? ?? ?? ?? 89 ?? ?? 73 ?? EB ?? C5 FA ?? ?? ?? ?? ?? ?? C5 ?? 59 ?? ?? ?? ??");
        std::uint8_t* PhotoModeOutputScanResult = Memory::PatternScan(exeModule,"C4 ?? ?? ?? ?? FF ?? 83 ?? ?? 89 ?? ?? EB ?? 8B ?? ?? C4 ?? ?? ?? ??");
        if (PhotoModePillarboxingScanResult && PhotoModeOutputScanResult) {
            spdlog::info("Photo Mode: Pillarboxing: Address is {:s}+{:x}", sExeName.c_str(), PhotoModePillarboxingScanResult - (std::uint8_t*)exeModule);
            static SafetyHookMid PhotoModePillarboxingMidHook{};
            PhotoModePillarboxingMidHook = safetyhook::create_mid(PhotoModePillarboxingScanResult,
                [](SafetyHookContext& ctx) {
                    ctx.xmm1.f32[0] = 1920.00f;
                }); 
            
            spdlog::info("Photo Mode: Output: Address is {:s}+{:x}", sExeName.c_str(), PhotoModeOutputScanResult - (std::uint8_t*)exeModule);
            Memory::PatchBytes(PhotoModeOutputScanResult + 0x04, "\xD2", 1); // vcvttss2si rdx,xmm0 -> vcvttss2si rdx,xmm2 for >16:9
            Memory::PatchBytes(PhotoModeOutputScanResult + 0x23, "\xC1", 1); // vcvttss2si r8,xmm0  -> vcvttss2si rdx,xmm1 for <16:9
            spdlog::info("Photo Mode: Output: Patched instructions.");
        }
        else {
            spdlog::error("Photo Mode: Pattern scan(s) failed.");
        }
    }
}

DWORD __stdcall Main(void*)
{
    Logging();
    Configuration();
    IntroSkip();
    FOV();
    Pillarboxing();
    Framerate();
    Misc();

    return true;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    {
        thisModule = hModule;

        HANDLE mainHandle = CreateThread(NULL, 0, Main, 0, NULL, 0);
        if (mainHandle)
        {
            SetThreadPriority(mainHandle, THREAD_PRIORITY_HIGHEST);
            CloseHandle(mainHandle);
        }
        break;
    }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}