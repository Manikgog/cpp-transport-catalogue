#include "request_handler.h"

RequestHandler::RequestHandler(const transport::TransportCatalogue& db, const renderer::MapRenderer& renderer)
    : db_(db)
    , renderer_(renderer) {}

std::optional<transport::TransportCatalogue::BusInfo> RequestHandler::GetBusStat(const std::string_view& bus_name) const {
    return db_.GetBusInfo(bus_name);
}

transport::set_names RequestHandler::GetBusesByStop(const std::string_view& stop_name) const {
    return db_.GetBusesByStopName(stop_name).value();
}

svg::Document RequestHandler::RenderMap() const {
    return svg::Document();
}