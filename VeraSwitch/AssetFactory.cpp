#include "AssetFactory.h"
#include <doctest\doctest.h>
#include <sqlite3pp.h>
#include <spdlog\spdlog.h>
#include "Utilities.h"
#include "Registry.h"

namespace spp = sqlite3pp;

auto AARC::AssetFactory::select(const std::string & db_path) -> std::vector<std::unique_ptr<Asset>> {
	MethodLogger mlog("AssetFactory::select");
	if (db_path.empty()) {
		mlog.logger()->error("SQLite path not set while trying to load asset");
		return std::vector<std::unique_ptr<Asset>>();
	}

	static auto sql = "SELECT ID,ASSET FROM ASSETS";
	SPDLOG_DEBUG(mlog.logger(), "Running query {}", sql);

	auto db = spp::database(db_path.c_str(), SQLITE_OPEN_READONLY);
	const auto& tx = spp::transaction(db);
	auto& qry = spp::query(db, sql);

	std::vector<std::unique_ptr<Asset>> assets;
	for (const auto& i : qry) {
		auto const id = i.get<int>(0);
		auto const str = i.get<std::string>(1);
		assets.emplace_back(std::make_unique<Asset>(id, str));
	}
	return assets;
}

auto AARC::AssetFactory::select(const std::string & db_path, const int64_t id) ->std::unique_ptr<Asset> {
	MethodLogger mlog("AssetFactory::select");
	if (db_path.empty()) {
		mlog.logger()->error("SQLite path not set while trying to load asset");
		return std::make_unique<Asset>();
	}
	static auto sql = "SELECT ID,ASSET FROM ASSETS WHERE ID=:id";
	SPDLOG_DEBUG(mlog.logger(), "Running query {} bound :id to {}", sql, id);

	auto db = spp::database(db_path.c_str(), SQLITE_OPEN_READONLY);
	const auto& tx = spp::transaction(db);
	auto& qry = spp::query(db, sql);
	qry.bind(":id", id);

	for (const auto& i : qry) {
		auto const id = i.get<int>(0);
		auto const str = i.get<std::string>(1);
		return std::make_unique<Asset>(id, str);
	}
	return std::make_unique<Asset>();
}

auto AARC::AssetFactory::create(const std::string & db_path, std::unique_ptr<Asset>& asset) -> void {
	MethodLogger mlog("AssetFactory::create");
	if (db_path.empty()) {
		mlog.logger()->error("SQLite path not set while trying to load asset");
		return;
	}
	static auto sql = "INSERT INTO ASSETS(asset) values (:name)";
	SPDLOG_DEBUG(mlog.logger(), "Running query {} bound :name to {}", sql, asset->asset_);

	auto db = spp::database(db_path.c_str(), SQLITE_OPEN_READWRITE);
	auto& tx = spp::transaction(db);
	auto& cmd = spp::command(db, sql);
	cmd.bind(":name", asset->asset_, sqlite3pp::nocopy);
	if (cmd.execute() != SQLITE_OK)
		tx.rollback();
	tx.commit();
	asset->id_ = db.last_insert_rowid();
}

auto AARC::AssetFactory::create(const std::string & db_path, const std::vector<std::unique_ptr<Asset>>& assets) -> decltype(assets) {
	MethodLogger mlog("AssetFactory::create");
	if (db_path.empty()) {
		mlog.logger()->error("SQLite path not set while trying to load asset");
		return assets;
	}
	static auto sql = "INSERT INTO ASSETS(asset) values (:name)";
	auto db = spp::database(db_path.c_str(), SQLITE_OPEN_READWRITE);
	auto& tx = spp::transaction(db);
	auto& cmd = spp::command(db);
	cmd.prepare(sql);
	for (auto& asset : assets) {
		cmd.bind(":name", asset->asset_, sqlite3pp::nocopy);
		if (cmd.execute() != SQLITE_OK) {
			tx.rollback();
			break;
		}
		asset->id_ = db.last_insert_rowid();
		cmd.reset();
	}
	tx.commit();
	return assets;
}

auto AARC::AssetFactory::remove(const std::string & db_path, const std::vector<int64_t>& ids) -> void{
	MethodLogger mlog("AssetFactory::remove");
	if (db_path.empty()) {
		mlog.logger()->error("SQLite path not set while trying to load asset");
		return;
	}
	static auto sql = "DELETE FROM ASSETS WHERE ID=:id";

	auto db = spp::database(db_path.c_str(), SQLITE_OPEN_READWRITE);
	auto& tx = spp::transaction(db);
	auto& cmd = spp::command(db, sql);
	for (auto id : ids) {
		SPDLOG_DEBUG(mlog.logger(), "Running query {} bound :id to {}", sql, id);
		cmd.bind(":id", id);
		cmd.execute();
		cmd.reset();
	}
	tx.commit();
}

auto AARC::AssetFactory::remove(const std::string & db_path, const std::string & name) -> void {
	MethodLogger mlog("AssetFactory::remove");
	if (db_path.empty()) {
		mlog.logger()->error("SQLite path not set while trying to load asset");
		return;
	}
	static auto sql = "DELETE FROM ASSETS WHERE ASSET=:asset";

	auto db = spp::database(db_path.c_str(), SQLITE_OPEN_READWRITE);
	auto& tx = spp::transaction(db);
	auto& cmd = spp::command(db, sql);
	SPDLOG_DEBUG(mlog.logger(), "Running query {} bound :name to {}", sql, name);
	cmd.bind(":name", name, sqlite3pp::nocopy);
	cmd.execute();
	tx.commit();
}

TEST_SUITE("Registry") {
	TEST_CASE("Create And read keys") {
		AARC::Registry::create_folder_with_key("AARCSim\\Database", "SQLiteLocation", "H:\\Users\\Mushfaque.Cradle\\Documents\\Repos\\PortfolioSim\\Simulator\\Database\\PortfolioSim.db3");
		std::array<uint8_t, 128> buf{};
		AARC::Registry::create_folder_with_key("AARCSim\\Database", "TestBinaryKey", buf);
		const auto sqlite_path = AARC::Registry::read_string("AARCSim\\Database", "SQLiteLocation");
		CHECK(!sqlite_path.empty());
	}
}

TEST_SUITE("AssetFactory") {
	TEST_CASE("SQL") {
		const auto sqlite_path = AARC::Registry::read_string("AARCSim\\Database", "SQLiteLocation");
		CHECK(!sqlite_path.empty());
		auto asset = std::make_unique<AARC::Asset>(-1, "TEST1");
		auto asset2 = std::make_unique<AARC::Asset>(-1, "TEST2");
		auto id = int64_t(0);
		auto id2 = int64_t(0);

		// Create
		AARC::AssetFactory::create(sqlite_path, asset);
		AARC::AssetFactory::create(sqlite_path, asset2);
		id = asset->id_;
		id2 = asset2->id_;
		CHECK(id > 0);
		CHECK(id2 > 0);
		CHECK(id2 != id);
		CHECK(id2 > id);

		// Read
		SUBCASE("Select") {
			const auto asset = AARC::AssetFactory::select(sqlite_path, id);
			CHECK(asset->id_ == id);
		}
		SUBCASE("Select") {
			const auto assets = AARC::AssetFactory::select(sqlite_path);
			CHECK(assets.size() >= 2);
		}
		// Delete
		AARC::AssetFactory::remove(sqlite_path, std::vector<int64_t>{ id, id2 });
		const auto asset3 = AARC::AssetFactory::select(sqlite_path, id);
		const auto asset4 = AARC::AssetFactory::select(sqlite_path, id);
		CHECK(asset3->id_ == -1);
		CHECK(asset4->id_ == -1);
	}
}
