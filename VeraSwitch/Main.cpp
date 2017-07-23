// Initialise IMGUI using DirectX11
#include "D3DImgui.h"
#include <imgui.h>
#include <spdlog\spdlog.h>
#include <tchar.h>
#define IMGUI_INCLUDE_IMGUI_USER_H
#define IMGUI_INCLUDE_IMGUI_USER_INL
#include "MainWindow.h"
#include "TimeSeries.h"
#include <cstdlib>
#include <imgui_user.h>

#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest\doctest.h>

#pragma comment(lib, "nfd.lib")
#pragma comment(lib, "sqlite3.lib")

namespace spd = spdlog;

int CALLBACK WinMain(_In_ HINSTANCE hInstance, _In_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow) {
    const auto logger = spd::rotating_logger_mt("logger", "sim.log", 1048576, 3);
    logger->flush_on(spd::level::debug);
#ifdef NDEBUG
    spd::set_async_mode(2048);
#endif
#ifdef _DEBUG
    spd::set_level(spd::level::trace);
#else
    spd::set_level(spd::level::info);
#endif
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    logger->critical_if(hr != S_OK, "Failed to initialise com");
    D3DImgui   imgui("AARCWindow");
    MainWindow window(imgui);
    SPDLOG_DEBUG(logger, "Initalised everything ok");
    SPDLOG_DEBUG(logger, "IMGUI Version {}", ImGui::GetVersion());
    imgui.run([&window](const auto ms) { window.run(ms); });
    return 0;
}

#ifdef _TEST
int main(int argc, char **argv) {
    const auto logger = spd::rotating_logger_mt("logger", "test.log", 1048576, 3);
    logger->flush_on(spd::level::debug);
    spd::set_level(spd::level::trace);
    logger->debug("Starting tests");
    doctest::Context ctx;
    ctx.setOption("abort-after", 5); // default - stop after 5 failed asserts
    ctx.applyCommandLine(argc, argv);
    int res = ctx.run(); // run test cases unless with --no-run
    logger->flush();
    return 0;
}
#endif