#include "transport_catalogue.h"
#include <unordered_set>
#include <iostream>

namespace transport {

void TransportCatalogue::AddStop(std::string name, Coordinates coords) {
    stops_.push_back({std::move(name), coords});
    stop_name_to_stop_[stops_.back().name] = &stops_.back();
}

void TransportCatalogue::AddBus(std::string name, const std::vector<std::string_view>& stop_names, bool is_roundtrip) {
    Bus bus;
    bus.name = std::move(name);
    bus.is_roundtrip = is_roundtrip;

    for (const auto& stop_name : stop_names) {
        if (auto it = stop_name_to_stop_.find(stop_name); it != stop_name_to_stop_.end()) {
            bus.stops.push_back(it->second);
        }
    }

    buses_.push_back(std::move(bus));
    bus_name_to_bus_[buses_.back().name] = &buses_.back();
}

const Stop* TransportCatalogue::FindStop(std::string_view name) const {
    if (auto it = stop_name_to_stop_.find(name); it != stop_name_to_stop_.end()) {
        return it->second;
    }
    return nullptr;
}

const Bus* TransportCatalogue::FindBus(std::string_view name) const {
    if (auto it = bus_name_to_bus_.find(name); it != bus_name_to_bus_.end()) {
        return it->second;
    }
    return nullptr;
}

const std::optional<std::set<std::string> > TransportCatalogue::GetBusesByStopName(std::string_view name) const {
    const Stop* stop = FindStop(name);
    if(!stop) {
        return std::nullopt;
    }
    std::set<std::string> buses_on_stop;
    for(const auto& bus : buses_) {
        for(const auto& s : bus.stops) {
            if(s->name == name) {
                buses_on_stop.insert(bus.name);
                break;
            }
        }
    }
    return {buses_on_stop};
}

std::optional<TransportCatalogue::BusInfo> TransportCatalogue::GetBusInfo(std::string_view name) const {
    const Bus* bus = FindBus(name);
    if (!bus || bus->stops.empty()) {
        return std::nullopt;
    }
    BusInfo info;
    info.stops_count = bus->is_roundtrip ? bus->stops.size() : bus->stops.size();
    std::unordered_set<const Stop*> unique_stops(bus->stops.begin(), bus->stops.end());
    info.unique_stops = unique_stops.size();
    for (size_t i = 1; i < bus->stops.size(); ++i) {
        info.route_length += ComputeDistance(bus->stops[i-1]->coordinates, bus->stops[i]->coordinates);
    }
    return info;
}

}
