#include "TimeSeries.h"
#define NOMINMAX
#include <Windows.h>
#include <spdlog\spdlog.h>
#define STB_DEFINE
#include "AssetFactory.h"
#include "Registry.h"
#include "TimeSeriesCSVFactory.h"
#include "TimeSeriesFactory.h"
#include "Utilities.h"
#include <array>
#include <chobo/small_vector.hpp>
#include <doctest\doctest.h>
#include <experimental\filesystem>
#include <fstream>
#include <imgui.h>
#include <imgui_user.h>
#include <iomanip>
#include <nfd.h>
#include <numeric>
#include <stb.h>
#include <stb_sprintf.h>
#include <sys/stat.h>

namespace spd = spdlog;

AARC::TimeSeriesMgr::TimeSeriesMgr() { init(); }

// Gui helper functions
void AARC::TimeSeriesMgr::add_item(const std::string &name) noexcept {
    MethodLogger mlog("TimeSeriesMgr::add_item");
    for (auto i = 0UL; i < filenames.size(); i++) { free(items[i]); }
    free(items);
    items = nullptr;
    filenames.emplace_back(name);
    auto tmp = (char **)realloc(items, sizeof(char *) * filenames.size());
    items    = tmp;
    for (auto i = 0UL; i < filenames.size(); i++) {
        items[i] = (char *)malloc(sizeof(char) * filenames[i].size() + 1);
        strcpy(items[i], filenames[i].c_str());
    }
}

void AARC::TimeSeriesMgr::remove_item(const int &idx) noexcept {
    MethodLogger mlog("TimeSeriesMgr::remove_item");

    free(items);
    items = nullptr;
    filenames.erase(filenames.begin() + idx);
    for (auto i = 0UL; i < filenames.size(); i++) {
        items[i] = (char *)malloc(sizeof(char) * filenames[i].size() + 1);
        strcpy(items[i], filenames[i].c_str());
    }
}

namespace {} // namespace

const auto reg_sqlite_location = "AARCSim\\Database";
const auto reg_sqlite_key      = "SQLiteLocation";

void AARC::TimeSeriesMgr::init() {
    MethodLogger mlog("TimeSeriesMgr::TimeSeriesMgr");
    sqlite_path           = AARC::Registry::read_string(reg_sqlite_location, reg_sqlite_key);
    const auto asset_list = AssetFactory::select(sqlite_path);
    for (const auto &asset : asset_list) { asset_list_str += asset->asset_ + '\0'; }
    strcpy(wnd_title, "Import CSV for: ");
}

// Input text for file name with +/- button
// List box with list of files
// DateTime format specifier
// Asset name
// Separator specifier
// Grid with output data
void AARC::TimeSeriesMgr::run(bool &show) {
    using namespace std;
    static const ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue;
    if (show) {
        auto file_to_load = array<char, MAX_PATH>();
        ImGui::Begin(wnd_title, &show, ImVec2(500, 500), 0.8f, ImGuiWindowFlags_NoResize);
        if (ImGui::InputText("Filename", file_to_load.data(), MAX_PATH, flags) == true) {
            spdlog::get("logger")->debug(" inputtext box changed {}", file_to_load.data());
            if (experimental::filesystem::exists(file_to_load.data())) {
                spdlog::get("logger")->debug(" file exists {}", file_to_load.data());
                textboxfilename = file_to_load.data();
                add_item(textboxfilename);
                live_data_ = TimeSeries_CSV::read_csv_partial_file(textboxfilename);
            }
        }
        if (!file_to_load.empty() && experimental::filesystem::exists(file_to_load.data()) == 0) {
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::Text("%s is an unknown file", file_to_load);
                ImGui::EndTooltip();
            }
        }
        ImGui::SameLine();
        // Add a file
        if (ImGui::Button("+", ImVec2(24, 24))) {
            auto path = std::array<nfdchar_t *, MAX_PATH>();
            if (NFD_OpenDialog("csv,txt", NULL, path.data()) == NFD_OKAY) {
                spdlog::get("logger")->debug(" open file chosen {}", path[0]);
                add_item(path[0]);
            }
        }
        ImGui::SameLine();
        // Subtract a file
        if (ImGui::Button("-", ImVec2(24, 24))) {
            if (filenames.size() > current_item) {
                spdlog::get("logger")->debug(" Removing item {} => {}", current_item, filenames[current_item]);
                remove_item(current_item);
            }
        }
        // List box
        if (ImGui::ListBox("Files", &current_item, items, filenames.size(), 5)) {
            live_data_ = TimeSeries_CSV::read_csv_partial_file(textboxfilename);
        }
        if (!filenames.empty() && ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text(filenames[current_item].c_str());
            ImGui::EndTooltip();
        }

        auto format = std::array<char, 128>();
        if (ImGui::InputText("Format:", format.data(), format.size(), ImGuiInputTextFlags_EnterReturnsTrue)) {
            // load_time_series_data(filenames[current_item]);
            stb_snprintf(wnd_title, 128, "Import CSV for: %s", asset);
        }
        // Grid
        if (!live_data_.ts_.empty()) {
            ImGui::Columns(5, "Timeseries");
            ImGui::Text("Timestamp");
            const auto sz = live_data_.ts_.size();
            for (int i = 0UL; i < sz; i++) {
                std::array<char, 16> row;
                stb_snprintf(row.data(), row.size(), "%d", live_data_.ts_[i]);
                ImGui::Selectable(row.data(), false, ImGuiSelectableFlags_SpanAllColumns);
            }
            ImGui::NextColumn();
            ImGui::Text("Open");
            for (int i = 0UL; i < sz; i++) {
                std::array<char, 16> row;
                stb_snprintf(row.data(), row.size(), "%.4f", live_data_.open_[i]);
                ImGui::Selectable(row.data(), false, ImGuiSelectableFlags_SpanAllColumns);
            }
            ImGui::NextColumn();
            ImGui::Text("High");
            for (int i = 0UL; i < sz; i++) {
                std::array<char, 16> row;
                stb_snprintf(row.data(), row.size(), "%.4f", live_data_.high_[i]);
                ImGui::Selectable(row.data(), false, ImGuiSelectableFlags_SpanAllColumns);
            }
            ImGui::NextColumn();
            ImGui::Text("Low");
            for (int i = 0UL; i < sz; i++) {
                std::array<char, 16> row;
                stb_snprintf(row.data(), row.size(), "%.4f", live_data_.low_[i]);
                ImGui::Selectable(row.data(), false, ImGuiSelectableFlags_SpanAllColumns);
            }
            ImGui::NextColumn();
            ImGui::Text("Close");
            for (int i = 0UL; i < sz; i++) {
                std::array<char, 16> row;
                stb_snprintf(row.data(), row.size(), "%.4f", live_data_.close_[i]);
                ImGui::Selectable(row.data(), false, ImGuiSelectableFlags_SpanAllColumns);
            }
            ImGui::NextColumn();
            ImGui::Separator();
            ImGui::Columns(1);

            const auto minmax = std::minmax_element(std::begin(live_data_.close_), std::end(live_data_.close_));
            ImGui::PlotLines("Close", live_data_.close_.data(), live_data_.close_.size() - 1, 0, 0,
                             *std::get<0>(minmax), *std::get<1>(minmax), ImVec2(300, 50));
        }

        ImGui::PushItemWidth(-100);
        if (ImGui::Button("OK", ImVec2(100, 32))) {
            // Kick off async threads per file to save to the database
            show = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(100, 32))) { show = false; }
        ImGui::End();
    } else {
    }
}
