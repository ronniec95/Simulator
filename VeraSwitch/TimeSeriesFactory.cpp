#include "TimeSeriesFactory.h"
#include "AARCDateTime.h"
#include "Registry.h"
#include "Utilities.h"
#include <doctest\doctest.h>
#include <spdlog\spdlog.h>
#include <sqlite3pp.h>
namespace spp = sqlite3pp;

auto AARC::TimeSeriesFactory::create(const std::string &db_path, const AARC::TSData &data) {
    MethodLogger mlog("TimeSeriesFactory::create");
    if (db_path.empty()) {
        mlog.logger()->error("SQLite path not set while trying to load asset");
        return;
    }
    static auto sql = "INSERT INTO TSDATA(ts,close,open,high,low,asset) values (:ts,:close,:open,:high,:low,:asset)";
    SPDLOG_DEBUG(mlog.logger(), "Running query {} for asset :asset {}", sql, data.asset_);

    auto  db  = spp::database(db_path.c_str(), SQLITE_OPEN_READWRITE);
    auto &tx  = spp::transaction(db);
    auto &cmd = spp::command(db, sql);
    for (auto i = 0UL; i < data.ts_.size(); i++) {
        cmd.bind(":ts", static_cast<int64_t>(data.ts_[i]));
        cmd.bind(":close", data.close_[i]);
        cmd.bind(":open", data.open_[i]);
        cmd.bind(":high", data.high_[i]);
        cmd.bind(":low", data.low_[i]);
        cmd.bind(":asset", static_cast<long long>(data.asset_));
        cmd.step();
        cmd.reset();
    }
    tx.commit();
}

auto AARC::TimeSeriesFactory::remove(const std::string &db_path, const uint64_t asset_id, const uint64_t start,
                                     const uint64_t end) {
    MethodLogger mlog("TimeSeriesFactory::create");
    if (db_path.empty()) {
        mlog.logger()->error("SQLite path not set while trying to load asset");
        return;
    }
    static auto sql = "DELETE FROM TSDATA WHERE ASSET=:asset AND TS>=:start AND TS<=:end";
    SPDLOG_DEBUG(mlog.logger(), "Running query {} for asset :asset {}", sql, asset_id);

    auto  db  = spp::database(db_path.c_str(), SQLITE_OPEN_READWRITE);
    auto &tx  = spp::transaction(db);
    auto &cmd = spp::command(db, sql);
    cmd.bind(":asset", static_cast<long long>(asset_id));
    cmd.bind(":start", static_cast<long long>(start));
    cmd.bind(":end", static_cast<long long>(end));
    cmd.execute();
    tx.commit();
}

auto AARC::TimeSeriesFactory::select(const std::string &db_path, const uint64_t asset_id, const uint64_t start,
                                     const uint64_t end) -> std::unique_ptr<TSData> {
    MethodLogger mlog("TimeSeriesFactory::select");
    if (db_path.empty()) {
        mlog.logger()->error("SQLite path not set while trying to load timeseries");
        return std::make_unique<TSData>();
    }
    static auto sql = "SELECT TS,OPEN,HIGH,LOW,CLOSE FROM TSDATA WHERE ASSET=:asset AND TS>=:start AND TS<=:end";
    SPDLOG_DEBUG(mlog.logger(), "Running query {} for asset :asset {}", sql, asset_id);
    auto  db  = spp::database(db_path.c_str(), SQLITE_OPEN_READONLY);
    auto &tx  = spp::transaction(db);
    auto &qry = spp::query(db, sql);
    qry.bind(":asset", static_cast<long long>(asset_id));
    qry.bind(":start", static_cast<long long>(start));
    qry.bind(":end", static_cast<long long>(end));
    auto data = std::make_unique<TSData>();
    for (const auto &row : qry) {
        data->ts_.emplace_back(row.get<long long>(0));
        data->open_.emplace_back(row.get<float>(1));
        data->high_.emplace_back(row.get<float>(2));
        data->low_.emplace_back(row.get<float>(3));
        data->close_.emplace_back(row.get<float>(4));
    }
    tx.commit();
    data->asset_ = asset_id;
    return data;
}

#ifdef _TEST
#include "AssetFactory.h"
#endif
TEST_SUITE("TimeseriesFactory") {
    TEST_CASE("Timeseries database access") {
        const auto db = AARC::Registry::read_string("AARCSim\\Database", "SQLiteLocation");
        CHECK(!db.empty());

        auto asset = std::make_unique<AARC::Asset>(-1, "TEST1");
        AARC::AssetFactory::create(db, asset);
        AARC::TSData data;
        data.asset_      = asset->id_;
        const auto start = AARC::AARCDateTime(2001, 1, 1);
        for (auto i = 0UL; i < 10; i++) {
            data.ts_.emplace_back(start.minutes + i);
            data.open_.emplace_back(static_cast<float>(std::rand() % 100 + 1));
            data.high_.emplace_back(static_cast<float>(std::rand() % 100 + 1));
            data.low_.emplace_back(static_cast<float>(std::rand() % 100 + 1));
            data.close_.emplace_back(static_cast<float>(std::rand() % 100 + 1));
        }
        // Save
        AARC::TimeSeriesFactory::create(db, data);

        // Retrieve
        SUBCASE("Retrieve all for asset") {
            const auto tsdata = AARC::TimeSeriesFactory::select(db, asset->id_, data.ts_.front(), data.ts_.back());
            CHECK(tsdata->asset_ == asset->id_);
            CHECK(tsdata->ts_.size() == 10);
        }
        SUBCASE("Retrieve between 2 timestamps") {
            const auto tsdata =
                AARC::TimeSeriesFactory::select(db, asset->id_, data.ts_.front() + 1, data.ts_.back() - 1);
            CHECK(tsdata->asset_ == asset->id_);
            CHECK(tsdata->ts_.size() == 8);
        }

        // Delete
        AARC::TimeSeriesFactory::remove(db, asset->id_, data.ts_.front(), data.ts_.back());
        const auto tsdata = AARC::TimeSeriesFactory::select(db, asset->id_, data.ts_.front(), data.ts_.back());
        CHECK(tsdata->asset_ == asset->id_);
        CHECK(tsdata->ts_.size() == 0);
        AARC::AssetFactory::remove(db, std::vector<int64_t>{asset->id_});
    }
}
