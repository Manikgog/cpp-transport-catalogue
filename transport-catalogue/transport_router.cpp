#include "transport_router.h"
#include "transport_catalogue.h"

void transport::TransportRouter::SetRoutingSettings(int bus_wait_time, double bus_velocity) {
    routing_settings_ = {bus_wait_time, bus_velocity};
}

void transport::TransportRouter::BuildGraph(const transport::TransportCatalogue& catalogue) {
    // Очищаем предыдущие данные
    stop_to_wait_vertex_.clear();
    stop_to_bus_vertex_.clear();
    edge_info_.clear();

    // Создаем вершины для остановок
    graph::VertexId vertex_id = 0;
    for (const auto& stop : catalogue.GetStops()) {
        stop_to_wait_vertex_[stop.name] = vertex_id++;
        stop_to_bus_vertex_[stop.name] = vertex_id++;
    }

    // Создаем граф с удвоенным количеством вершин
    graph_ = std::make_unique<graph::DirectedWeightedGraph<double>>(vertex_id);

    // Добавляем ребра ожидания (от вершины ожидания к вершине посадки)
    for (const auto& [stop_name, _] : stop_to_wait_vertex_) {
        graph::VertexId from = stop_to_wait_vertex_.at(stop_name);
        graph::VertexId to = stop_to_bus_vertex_.at(stop_name);
        graph::EdgeId edge_id = graph_->AddEdge({from, to, static_cast<double>(routing_settings_.bus_wait_time)});
        edge_info_[edge_id] = {"WAIT", stop_name, 0}; // Метка для ребра ожидания
    }

    // Добавляем ребра автобусных переездов
    for (const auto& bus : catalogue.GetBuses()) {
        const auto& stops = bus.stops;
        if (stops.empty()) continue;

        // Обрабатываем все возможные пары остановок в маршруте
        for (size_t i = 0; i < stops.size(); ++i) {
            for (size_t j = i + 1; j < stops.size(); ++j) {
                const std::string& from_stop = stops[i]->name;
                const std::string& to_stop = stops[j]->name;

                // Вычисляем общее расстояние и время
                int total_distance = 0;
                for (size_t k = i; k < j; ++k) {
                    auto dist = catalogue.GetDistance(stops[k], stops[k+1]);
                    if (!dist) dist = catalogue.GetDistance(stops[k+1], stops[k]);
                    if (dist) total_distance += *dist;
                }
                const int meters_to_km = 1000;
                const int seconds_to_min = 60;
                double time = total_distance / (routing_settings_.bus_velocity * meters_to_km / seconds_to_min); // км/ч → м/мин

                // Добавляем ребро от вершины посадки 'from' до вершины ожидания 'to'
                graph::VertexId from = stop_to_bus_vertex_.at(from_stop);
                graph::VertexId to = stop_to_wait_vertex_.at(to_stop);
                graph::EdgeId edge_id = graph_->AddEdge({from, to, time});
                edge_info_[edge_id] = {bus.name, from_stop, static_cast<int>(j - i)};

                // Для некольцевых маршрутов добавляем обратные ребра
                if (!bus.is_roundtrip) {
                    int reverse_distance = 0;
                    for (size_t k = j; k > i; --k) {
                        auto dist = catalogue.GetDistance(stops[k], stops[k-1]);
                        if (!dist) dist = catalogue.GetDistance(stops[k-1], stops[k]);
                        if (dist) reverse_distance += *dist;
                    }

                    double reverse_time = reverse_distance / (routing_settings_.bus_velocity * meters_to_km / seconds_to_min);
                    graph::VertexId reverse_from = stop_to_bus_vertex_.at(to_stop);
                    graph::VertexId reverse_to = stop_to_wait_vertex_.at(from_stop);
                    graph::EdgeId reverse_edge_id = graph_->AddEdge({reverse_from, reverse_to, reverse_time});
                    edge_info_[reverse_edge_id] = {bus.name, to_stop, static_cast<int>(j - i)};
                }
            }
        }
    }

    // Строим маршрутизатор
    router_ = std::make_unique<graph::Router<double>>(*graph_);
}

std::optional<transport::RouteInfo> transport::TransportRouter::FindRoute(
    const std::string& from, const std::string& to) const
{
    if (!stop_to_wait_vertex_.count(from) || !stop_to_wait_vertex_.count(to)) {
        return std::nullopt;
    }

    graph::VertexId from_vertex = stop_to_wait_vertex_.at(from);
    graph::VertexId to_vertex = stop_to_wait_vertex_.at(to);

    auto route_info = router_->BuildRoute(from_vertex, to_vertex);
    if (!route_info) {
        return std::nullopt;
    }

    RouteInfo result;
    result.total_time = route_info->weight;

    for (graph::EdgeId edge_id : route_info->edges) {
        const auto& edge = graph_->GetEdge(edge_id);
        const auto& [bus_name, from_stop, span_count] = edge_info_.at(edge_id);

        if (bus_name == "WAIT") { // Ребро ожидания
            result.items.emplace_back(std::pair{from_stop, edge.weight});
        } else { // Ребро автобуса
            result.items.emplace_back(std::tuple{bus_name, from_stop, span_count, edge.weight});
        }
    }

    return result;
}