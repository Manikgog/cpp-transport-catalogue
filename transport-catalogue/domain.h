#pragma once

#include "geo.h"

#include <string>
#include <vector>

namespace transport {

    struct Stop {
        std::string name;
        geo::Coordinates coordinates;
    };

    struct Bus {
        std::string name;
        std::vector<const Stop*> stops;
        bool is_roundtrip = false;
    };

    class StopPairHasher {
    public:
        size_t operator()(const std::pair<const Stop*, const Stop*>& pair_of_stops) const;
    };

}