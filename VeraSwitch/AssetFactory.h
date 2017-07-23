#pragma once
#include <memory>
#include <string>
#include <vector>

namespace AARC {
    struct Asset {
        Asset()              = default;
        Asset(const Asset &) = default;

        Asset(int64_t id, const std::string &asset) : id_(id), asset_(asset) {}
        int64_t     id_ = -1;
        std::string asset_;
    };

    namespace AssetFactory {
        auto select(const std::string &db_path) -> std::vector<std::unique_ptr<Asset>>;
        auto select(const std::string &db_path, const int64_t id) -> std::unique_ptr<Asset>;
        auto select_by_name(const std::string &db_path, const std::string name) -> std::unique_ptr<Asset>;
        auto create(const std::string &db_path, std::unique_ptr<Asset> &asset) -> void;
        auto create(const std::string &db_path, const std::vector<std::unique_ptr<Asset>> &assets) -> decltype(assets);
        auto remove(const std::string &db_path, const std::vector<int64_t> &ids) -> void;
        auto remove(const std::string &db_path, const std::string &name) -> void;
    } // namespace AssetFactory
} // namespace AARC