#include "transport_catalogue.h"

#include <unordered_set>
#include <iostream>

namespace transport {

    void TransportCatalogue::AddStop(std::string name, geo::Coordinates coords) {
        stops_.push_back({ std::move(name), coords });
        stop_name_to_stop_[stops_.back().name] = &stops_.back();
    }

    void TransportCatalogue::AddBus(std::string name, const std::vector<std::string_view>& stop_names, bool is_roundtrip) {
        if (name.empty()) return;
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

    std::optional<set_names> TransportCatalogue::GetBusesByStopName(std::string_view name) const {
        const Stop* stop = FindStop(name);
        if (!stop) {
            return std::nullopt;
        }
        if (!stop_name_to_buses_.count(name)) {
            return &empty_buses_set_;
        }
        return { &stop_name_to_buses_.at(name) };
    }

    std::optional<TransportCatalogue::BusInfo> TransportCatalogue::GetBusInfo(std::string_view name) const {
        const Bus* bus = FindBus(name);
        if (!bus || bus->stops.empty()) {
            return std::nullopt;
        }

        BusInfo info;
        info.stops_count = bus->is_roundtrip ? bus->stops.size() : bus->stops.size() * 2 - 1;

        std::unordered_set<const Stop*> unique_stops(bus->stops.begin(), bus->stops.end());
        info.unique_stops = unique_stops.size();

        double road_distance = 0.0;
        double geo_distance = 0.0;

        for (size_t i = 1; i < bus->stops.size(); ++i) {
            const Stop* from = bus->stops[i-1];
            const Stop* to = bus->stops[i];

            if (auto it = distances_between_stops_.find({from, to}); it != distances_between_stops_.end()) {
                road_distance += it->second;
            } else if (auto it_rev = distances_between_stops_.find({to, from});
                     it_rev != distances_between_stops_.end()) {
                road_distance += it_rev->second;
            }

            geo_distance += geo::ComputeDistance(from->coordinates, to->coordinates);
        }

        if (!bus->is_roundtrip) {
            for (size_t i = bus->stops.size()-1; i > 0; --i) {
                const Stop* from = bus->stops[i];
                const Stop* to = bus->stops[i-1];

                if (auto it = distances_between_stops_.find({from, to}); it != distances_between_stops_.end()) {
                    road_distance += it->second;
                } else if (auto it_rev = distances_between_stops_.find({to, from});
                         it_rev != distances_between_stops_.end()) {
                    road_distance += it_rev->second;
                }

                geo_distance += geo::ComputeDistance(from->coordinates, to->coordinates);
            }
        }

        info.route_length = road_distance;
        info.curvature = road_distance / geo_distance;

        return info;
    }

    std::set<std::string_view> TransportCatalogue::GetBusNames() const {
        std::set<std::string_view> bus_names;
        for (const auto& name : buses_) {
            if (!name.name.empty()) {
                bus_names.insert(name.name);
            }
        }
        return bus_names;
    }

    void TransportCatalogue::AddDistance(const Stop* from, const Stop* to, int distance) {
        distances_between_stops_[std::make_pair(const_cast<Stop*>(from), const_cast<Stop*>(to))] = distance;
    }

}