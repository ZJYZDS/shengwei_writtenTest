#include "map_manager.h"
#include <algorithm>
#include <cmath>
#include <iostream>

using namespace std;

pair<double, double> MapManager::resolve_range(double r) {
    if (r < 0) {
        cerr << "[warn] range = " << r << " < 0, taking abs" << endl;
        return resolve_range(-r);
    }
    if (r >= 180) {
        cerr << "[warn] range = " << r << " >= 180, clamped to 180" << endl;
        r = 180;
    }
    if (r > 90) {
        cerr << "[warn] range = " << r << " > 90 (lat max=90), lon_range=" << r
             << ", lat_range=0" << endl;
        return {r, 0.0};
    }
    cout << "[info] range = " << r << " <= 90, set as both lon_range and lat_range" << endl;
    return {r, r};
}

void MapManager::set_bounds(double min_lat, double max_lat,
                            double min_lon, double max_lon) {
    min_lat_ = min_lat;
    max_lat_ = max_lat;
    min_lon_ = min_lon;
    max_lon_ = max_lon;
}

pair<uint32_t, uint32_t> MapManager::Jw2Coord(double lat, double lon,
                                               const Resolution& res) const {
    double lat_range = max_lat_ - min_lat_;
    double lon_range = max_lon_ - min_lon_;
    uint32_t row = static_cast<uint32_t>((lat - min_lat_) / lat_range * res.height);
    uint32_t col = static_cast<uint32_t>((lon - min_lon_) / lon_range * res.width);
    row = min(row, res.height - 1);
    col = min(col, res.width  - 1);
    return {row, col};
}

MapManager::Key MapManager::coordEncoder(uint32_t row, uint32_t col) {
    return (static_cast<uint64_t>(row) << 32) | col;
}

bool MapManager::in_tile(const Tile& tile, double lat, double lon) {
    bool lat_ok = lat >= tile.bottom_right.latitude &&
                  lat <= tile.top_left.latitude;

    bool lon_ok;
    if (tile.top_left.longitude <= tile.bottom_right.longitude) {
        lon_ok = lon >= tile.top_left.longitude &&
                 lon <= tile.bottom_right.longitude;
    } else {
        lon_ok = lon >= tile.top_left.longitude ||
                 lon <= tile.bottom_right.longitude;
    }
    return lat_ok && lon_ok;
}

bool MapManager::overlaps_rect(const Tile& tile,
                                double q_min_lat, double q_max_lat,
                                double q_min_lon, double q_max_lon) {
    double tile_min_lat = tile.bottom_right.latitude;
    double tile_max_lat = tile.top_left.latitude;
    if (q_max_lat < tile_min_lat || q_min_lat > tile_max_lat)
        return false;

    double tl_lon = tile.top_left.longitude;
    double br_lon = tile.bottom_right.longitude;
    if (tl_lon > br_lon) br_lon += 360.0;

    for (double shift : { -360.0, 0.0, 360.0 }) {
        double a = tl_lon + shift;
        double b = br_lon + shift;
        if (max(a, q_min_lon) <= min(b, q_max_lon))
            return true;
    }
    return false;
}

bool MapManager::insert(const Tile& tile, const Resolution& res) {
    auto& grid = grids_[res];
    auto ptr = make_shared<const Tile>(tile);

    auto [r1, c1] = Jw2Coord(tile.top_left.latitude,     tile.top_left.longitude,     res);
    auto [r2, c2] = Jw2Coord(tile.bottom_right.latitude, tile.bottom_right.longitude, res);
    uint32_t r_begin = min(r1, r2), r_end = max(r1, r2);

    auto insert_columns = [&](uint32_t r, uint32_t c_begin, uint32_t c_end) {
        for (uint32_t c = c_begin; c <= c_end; ++c)
            grid[coordEncoder(r, c)].push_back(ptr);
    };

    for (uint32_t r = r_begin; r <= r_end; ++r) {
        if (c1 <= c2) {
            insert_columns(r, c1, c2);
        } else {
            insert_columns(r, c1, res.width - 1);
            insert_columns(r, 0, c2);
        }
    }
    return true;
}

bool MapManager::remove(const Tile& tile, const Resolution& res) {
    auto it = grids_.find(res);
    if (it == grids_.end()) return false;
    auto& grid = it->second;

    auto [r1, c1] = Jw2Coord(tile.top_left.latitude,     tile.top_left.longitude,     res);
    auto [r2, c2] = Jw2Coord(tile.bottom_right.latitude, tile.bottom_right.longitude, res);
    uint32_t r_begin = min(r1, r2), r_end = max(r1, r2);

    auto remove_from_cell = [&](uint32_t r, uint32_t c) -> bool {
        auto& list = grid[coordEncoder(r, c)];
        auto erase_it = find_if(list.begin(), list.end(),
            [&](const TilePtr& p) {
                return p->top_left.latitude     == tile.top_left.latitude     &&
                       p->top_left.longitude    == tile.top_left.longitude    &&
                       p->top_left.altitude     == tile.top_left.altitude     &&
                       p->bottom_right.latitude == tile.bottom_right.latitude &&
                       p->bottom_right.longitude == tile.bottom_right.longitude &&
                       p->bottom_right.altitude == tile.bottom_right.altitude;
            });
        if (erase_it == list.end()) return false;
        list.erase(erase_it);
        return true;
    };

    bool removed = false;
    for (uint32_t r = r_begin; r <= r_end; ++r) {
        if (c1 <= c2) {
            for (uint32_t c = c1; c <= c2; ++c)
                removed |= remove_from_cell(r, c);
        } else {
            for (uint32_t c = c1; c < res.width; ++c)
                removed |= remove_from_cell(r, c);
            for (uint32_t c = 0; c <= c2; ++c)
                removed |= remove_from_cell(r, c);
        }
    }
    return removed;
}

vector<Tile> MapManager::get_all(const Resolution& res) const {
    auto it = grids_.find(res);
    if (it == grids_.end()) return {};
    vector<Tile> result;
    vector<const Tile*> seen;
    for (const auto& [key, tiles] : it->second) {
        for (const auto& ptr : tiles) {
            if (find(seen.begin(), seen.end(), ptr.get()) == seen.end()) {
                seen.push_back(ptr.get());
                result.push_back(*ptr);
            }
        }
    }
    return result;
}

optional<Tile> MapManager::query_point(double latitude, double longitude,
                                       const Resolution& res) const {
    auto it = grids_.find(res);
    if (it == grids_.end()) return nullopt;
    auto [r, c] = Jw2Coord(latitude, longitude, res);
    auto cell_it = it->second.find(coordEncoder(r, c));
    if (cell_it == it->second.end()) return nullopt;
    for (const auto& ptr : cell_it->second) {
        if (in_tile(*ptr, latitude, longitude)) return *ptr;
    }
    return nullopt;
}

vector<Tile> MapManager::rect_query_impl(
    const Grid& grid, const Resolution& res,
    double q_min_lat, double q_max_lat,
    double q_min_lon, double q_max_lon) const
{
    auto [r1, c1] = Jw2Coord(q_min_lat, q_min_lon, res);
    auto [r2, c2] = Jw2Coord(q_max_lat, q_max_lon, res);
    uint32_t r_begin = min(r1, r2), r_end = max(r1, r2);
    uint32_t c_begin = min(c1, c2), c_end = max(c1, c2);

    vector<Tile> result;
    vector<const Tile*> seen;

    for (uint32_t r = r_begin; r <= r_end; ++r) {
        for (uint32_t c = c_begin; c <= c_end; ++c) {
            auto cell_it = grid.find(coordEncoder(r, c));
            if (cell_it == grid.end()) continue;
            for (const auto& ptr : cell_it->second) {
                if (overlaps_rect(*ptr, q_min_lat, q_max_lat, q_min_lon, q_max_lon)) {
                    if (find(seen.begin(), seen.end(), ptr.get()) == seen.end()) {
                        seen.push_back(ptr.get());
                        result.push_back(*ptr);
                    }
                }
            }
        }
    }
    return result;
}

vector<Tile> MapManager::query_range(double latitude, double longitude,
                                     double range,
                                     const Resolution& res) const {
    auto it = grids_.find(res);
    if (it == grids_.end()) return {};

    auto [lon_range, lat_range] = resolve_range(range);

    double q_min_lat = latitude - lat_range;
    double q_max_lat = latitude + lat_range;
    double q_min_lon = longitude - lon_range;
    double q_max_lon = longitude + lon_range;

    return rect_query_impl(it->second, res,
                           q_min_lat, q_max_lat, q_min_lon, q_max_lon);
}

vector<Tile> MapManager::query_range(double latitude, double longitude,
                                     double lon_range, double lat_range,
                                     const Resolution& res) const {
    auto it = grids_.find(res);
    if (it == grids_.end()) return {};

    if (lat_range == -numeric_limits<double>::infinity()) {
        cerr << "[warn] lat_range not provided, "
                "falling back to single-number degree-aware logic" << endl;
        return query_range(latitude, longitude, lon_range, res);
    }

    double q_min_lat = latitude - lat_range;
    double q_max_lat = latitude + lat_range;
    double q_min_lon = longitude - lon_range;
    double q_max_lon = longitude + lon_range;

    return rect_query_impl(it->second, res,
                           q_min_lat, q_max_lat, q_min_lon, q_max_lon);
}

bool MapManager::contains(double latitude, double longitude,
                          const Resolution& res) const {
    return query_point(latitude, longitude, res).has_value();
}

vector<Resolution> MapManager::supported_resolutions() const {
    vector<Resolution> result;
    for (const auto& [res, grid] : grids_) {
        result.push_back(res);
    }
    return result;
}
