#include "transport_catalogue.h"
#include <unordered_set>

namespace transport {

void TransportCatalogue::AddStop(std::string name, Coordinates coords) {
    stops_.push_back({ std::move(name), coords });
    stop_name_to_stop_[stops_.back().name] = &stops_.back();
}

void TransportCatalogue::AddBus(std::string name, const std::vector<std::string_view>& stop_names, bool is_roundtrip) {
    buses_.push_back({ std::move(name), {}, is_roundtrip });
    Bus& bus = buses_.back();
    for (const auto& stop_name : stop_names) {
        if (auto it = stop_name_to_stop_.find(stop_name); it != stop_name_to_stop_.end()) {
            bus.stops.push_back(it->second);
            stop_name_to_buses_[it->first].insert(bus.name);
        }
    }
    bus_name_to_bus_[bus.name] = &bus;
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

const std::optional<std::set<std::string_view> > TransportCatalogue::GetBusesByStopName(std::string_view name) const {
    const Stop* stop = FindStop(name);
    if (!stop) {
        return std::nullopt;
    }
    if(!stop_name_to_buses_.count(name)) {
        return std::set<std::string_view>();
    }
    return { stop_name_to_buses_.at(name) };
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
        info.route_length += ComputeDistance(bus->stops[i - 1]->coordinates, bus->stops[i]->coordinates);
    }
    return info;
}

}
