#pragma once

#include "common_types.h"
#include <vector>
#include <optional>
#include <unordered_map>
#include <map>
#include <memory>
#include <utility>

class MapManager {
public:
    MapManager() = default;

    void set_bounds(double min_lat, double max_lat,
                    double min_lon, double max_lon);

    bool insert(const Tile& tile, const Resolution& res);

    bool remove(const Tile& tile, const Resolution& res);

    std::vector<Tile> get_all(const Resolution& res) const;

    std::optional<Tile> query_point(double latitude, double longitude,
                                    const Resolution& res) const;

    std::vector<Tile> query_range(double latitude, double longitude,
                                  double range,
                                  const Resolution& res) const;

    std::vector<Tile> query_range(double latitude, double longitude,
                                  double lon_range, double lat_range,
                                  const Resolution& res) const;

    bool contains(double latitude, double longitude,
                  const Resolution& res) const;

    std::vector<Resolution> supported_resolutions() const;

private:
    using Key     = uint64_t;
    using TilePtr = std::shared_ptr<const Tile>;
    using Grid    = std::unordered_map<Key, std::vector<TilePtr>>;

    double min_lat_ = -90.0, max_lat_ = 90.0;
    double min_lon_ = -180.0, max_lon_ = 180.0;

    std::map<Resolution, Grid> grids_;

    std::pair<uint32_t, uint32_t> Jw2Coord(double lat, double lon,
                                           const Resolution& res) const;

    static Key coordEncoder(uint32_t row, uint32_t col);

    static bool in_tile(const Tile& tile, double lat, double lon);

    static bool overlaps_rect(const Tile& tile,
                              double q_min_lat, double q_max_lat,
                              double q_min_lon, double q_max_lon);

    static std::pair<double, double> resolve_range(double degrees);

    std::vector<Tile> rect_query_impl(const Grid& grid, const Resolution& res,
                                      double q_min_lat, double q_max_lat,
                                      double q_min_lon, double q_max_lon) const;
};
