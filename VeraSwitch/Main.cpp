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

TEST_CASE("Zion") {
    int         width    = 5; // the number of cells on the X axis
    int         height   = 1; // the number of cells on the Y axis
    std::string lines[1] = {"0.0.0"};
    for (int j = 0; j < height; j++) {
        std::string line = lines[j];
        for (int i = 0; i < line.size(); i++) {
            char ch = line[i];
            if (ch == '0') {
                const bool has_neighbour_x = i + 1 < width; // && j + 1 < height;
                const bool has_neighbour_y = j + 1 < height;
                const auto x1              = i;
                const auto y1              = j;
                const auto x2              = has_neighbour_x ? i + 1 : -1;
                const auto y2              = has_neighbour_x ? j : -1;
                const auto x3              = has_neighbour_y ? i : -1;
                const auto y3              = has_neighbour_y ? j + 1 : -1;
                printf("%d %d %d %d %d %d\n", x1, y1, x2, y2, x3, y3);
            }
        }
    }
}
#endif