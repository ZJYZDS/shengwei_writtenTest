#pragma once

#include <cstdint>

struct GpsCoordinate {
    double latitude;
    double longitude;
    double altitude;
    GpsCoordinate(
        double j_ = 0.0,
        double w_ = 0.0,
        double h_ = 0.0
    ): latitude(j_), longitude(w_), altitude(h_){}
};

struct Resolution {
    uint32_t height = 0;
    uint32_t width  = 0;

    bool operator<(const Resolution& o) const {
        if (height != o.height) return height < o.height;
        return width < o.width;
    }
};

struct Tile {
    GpsCoordinate top_left;
    GpsCoordinate bottom_right;
};
