#pragma once

#include "domain.h"
#include "graph.h"
#include <string>
#include <string_view>
#include <unordered_map>
#include <set>
#include <deque>
#include <optional>
#include <vector>
#include <memory>
#include "router.h"
#include "transport_router.h"



namespace transport {
    class TransportRouter;
    struct RouteInfo;

    using stops_map = std::unordered_map<std::string_view, const Stop*>;
    using buses_map = std::unordered_map<std::string_view, const Bus *>;
    using buses_names_on_stop_map = std::unordered_map<std::string_view, std::set<std::string_view> >;
    using set_names = const std::set<std::string_view> *;
    using map_distances = std::unordered_map<std::pair<const Stop *, const Stop *>, int, StopPairHasher>;

    class TransportCatalogue {
    public:
        TransportCatalogue();
        ~TransportCatalogue();

        struct BusInfo {
            size_t stops_count = 0;
            size_t unique_stops = 0;
            double route_length = 0.0;
            double curvature = 1.0;
        };

        void AddStop(std::string, geo::Coordinates);
        void AddDistance(const Stop *, const Stop *, int);
        void AddBus(std::string, const std::vector<std::string_view> &, bool);

        [[nodiscard]] const Stop *FindStop(std::string_view) const;
        [[nodiscard]] const Bus *FindBus(std::string_view) const;
        [[nodiscard]] std::optional<set_names> GetBusesByStopName(std::string_view) const;
        [[nodiscard]] std::optional<BusInfo> GetBusInfo(std::string_view) const;
        [[nodiscard]] std::set<std::string_view> GetBusNames() const;
        const std::deque<Stop>& GetStops() const;
        const std::deque<Bus>& GetBuses() const;
        std::optional<int> GetDistance(const Stop *lhs, const Stop *rhs) const;
        void SetRoutingSettings(int bus_wait_time, double bus_velocity);
        void BuildRouter();
        std::optional<RouteInfo> FindRoute(const std::string& from, const std::string& to) const;

    private:
        std::deque<Stop> stops_;
        std::deque<Bus> buses_;
        stops_map stop_name_to_stop_;
        buses_map bus_name_to_bus_;
        buses_names_on_stop_map stop_name_to_buses_;
        std::set<std::string_view> empty_buses_set_;
        map_distances distances_between_stops_;
        TransportRouter* router_;
    };
}
