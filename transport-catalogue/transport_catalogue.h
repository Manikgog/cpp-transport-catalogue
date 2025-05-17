#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <deque>
#include <vector>
#include <optional>
#include <set>
#include <unordered_set>

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

class TransportCatalogue {
public:
    void AddStop(std::string name, Coordinates coords);

    void AddBus(std::string name, const std::vector<std::string_view>& stops, bool is_roundtrip);

    const Stop* FindStop(std::string_view name) const;

    const Bus* FindBus(std::string_view name) const;

    const std::optional<std::set<std::string> > GetBusesByStopName(std::string_view name) const;
	
    struct BusInfo {
        size_t stops_count = 0;
        size_t unique_stops = 0;
        double route_length = 0.0;
    };
	
    std::optional<BusInfo> GetBusInfo(std::string_view name) const;

private:
    std::deque<Stop> stops_;
    std::deque<Bus> buses_;
    std::unordered_map<std::string_view, const Stop*> stop_name_to_stop_;
    std::unordered_map<std::string_view, const Bus*> bus_name_to_bus_;
    std::unordered_map<std::string_view, std::unordered_set<const Bus*> > stop_name_to_buses_;
};

}
