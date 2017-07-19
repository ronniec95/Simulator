#pragma once
#include "Utilities.h"
#include <array>
#include <memory>
#include <string>
#include <vector>

struct TSState;

namespace AARC {
    using namespace std;

    struct TSData {
        TSData() = default;
        TSData(const vector<size_t> &ts, const vector<float> &open, const vector<float> &high, const vector<float> &low,
               const vector<float> &close)
            : ts_(ts), open_(open), high_(high), low_(low), close_(close) {}
        uint64_t       asset_ = 0;
        vector<size_t> ts_;
        vector<float>  open_;
        vector<float>  high_;
        vector<float>  low_;
        vector<float>  close_;

        void clear() {
            ts_.clear();
            open_.clear();
            high_.clear();
            low_.clear();
            close_.clear();
        }

        void reserve(const size_t sz) {
            ts_.reserve(sz);
            open_.reserve(sz);
            high_.reserve(sz);
            low_.reserve(sz);
            close_.reserve(sz);
        }
    };

    class TimeSeriesMgr {
      public:
        TimeSeriesMgr();
        ~TimeSeriesMgr() { free(items); }
        void run(bool &show);

      private:
        void init();
        // GUI State is stored here
        string sqlite_path;

        // Window title
        char wnd_title[128] = {0};

        // Asset name/choice
        string asset;
        string asset_list_str;
        int    asset_choice = -1;

        // List box state
        vector<string> filenames;
        char **        items        = nullptr;
        int            current_item = 0;
        string         textboxfilename;

        // File loading specific data
        void add_item(const string &name) noexcept;
        void remove_item(const int &idx) noexcept;

        // Grid
        TSData live_data_;
        // Histogram
    };
} // namespace AARC