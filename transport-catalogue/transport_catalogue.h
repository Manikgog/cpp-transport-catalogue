#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <deque>
#include <vector>
#include <optional>
#include <set>

#include "geo.h"

namespace transport {

struct Stop {
    std::string name;
    Coordinates coordinates;
};

struct Bus {
    std::string name;
    std::vector<const Stop*> stops;
    bool is_roundtrip = false;
};

class StopPairHasher {
public:
    size_t operator()(const std::pair<const Stop*, const Stop*>& pair_of_stops) const {
        auto hash1 = std::hash<const Stop*>{}(pair_of_stops.first);
        auto hash2 = std::hash<const Stop*>{}(pair_of_stops.second);
        return hash1 ^ (hash2 + 0x9e3779b9 + (hash1 << 6) + (hash1 >> 2));
    }
};

using stops_map = std::unordered_map<std::string_view, const Stop*>;
using buses_map = std::unordered_map<std::string_view, const Bus*>;
using buses_names_on_stop_map = std::unordered_map<std::string_view, std::set<std::string_view> >;
using set_names = const std::set<std::string_view>*;
using map_distances = std::unordered_map<std::pair<const Stop*, const Stop*>, int, StopPairHasher>;

class TransportCatalogue {
public:

    struct BusInfo {
        size_t stops_count = 0;
        size_t unique_stops = 0;
        double route_length = 0.0;
        double curvature = 1.0;
    };

    void AddStop(std::string, Coordinates);

    void AddBus(std::string, const std::vector<std::string_view>&, bool);

    [[nodiscard]] const Stop* FindStop(std::string_view) const;

    [[nodiscard]] const Bus* FindBus(std::string_view) const;

    [[nodiscard]] std::optional<set_names> GetBusesByStopName(std::string_view) const;

    void AddDistance(const Stop*, const Stop*, int);
	
    [[nodiscard]] std::optional<BusInfo> GetBusInfo(std::string_view) const;

private:
    std::deque<Stop> stops_;
    std::deque<Bus> buses_;
    stops_map stop_name_to_stop_;
    buses_map bus_name_to_bus_;
    buses_names_on_stop_map stop_name_to_buses_;
    std::set<std::string_view> empty_buses_set_;
    map_distances distances_between_stops_;
};

}
