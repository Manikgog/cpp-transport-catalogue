#pragma once

#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "graph.h"
#include "router.h"
#include "transport_catalogue.h"

namespace transport {

    class TransportCatalogue;

    struct RouteInfo {
        double total_time;
        std::vector<std::variant<
            std::pair<std::string, double>, // Ожидание на остановке: название остановки, время
            std::tuple<std::string, std::string, int, double> // Маршрут: название маршрута, от остановки, до остановки, время
        >> items;

        RouteInfo() = default;
    };

    class TransportRouter {
    public:
        struct RoutingSettings {
            int bus_wait_time = 0;
            double bus_velocity = 0;
        };

        void SetRoutingSettings(int bus_wait_time, double bus_velocity);
        void BuildGraph(const TransportCatalogue& catalogue);
        std::optional<RouteInfo> FindRoute(const std::string& from, const std::string& to) const;

    private:
        RoutingSettings routing_settings_;
        std::unique_ptr<graph::DirectedWeightedGraph<double>> graph_;
        std::unique_ptr<graph::Router<double>> router_;
        std::unordered_map<std::string, graph::VertexId> stop_to_wait_vertex_;
        std::unordered_map<std::string, graph::VertexId> stop_to_bus_vertex_;
        std::unordered_map<graph::EdgeId, std::tuple<std::string, std::string, int>> edge_info_;
    };
}