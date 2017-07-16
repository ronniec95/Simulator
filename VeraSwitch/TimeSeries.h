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
        uint64_t        asset_ = 0;
        vector<int64_t> ts_;
        vector<float>   open_;
        vector<float>   high_;
        vector<float>   low_;
        vector<float>   close_;

        void clear() {
            ts_.clear();
            open_.clear();
            high_.clear();
            low_.clear();
            close_.clear();
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
        std::unique_ptr<TSData> live_data_;
        // Histogram
    };
} // namespace AARC