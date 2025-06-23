#pragma once
#include "transport_catalogue.h"
#include "svg.h"
#include "map_renderer.h"

 class RequestHandler {
 public:

     RequestHandler(const transport::TransportCatalogue& db, const renderer::MapRenderer& renderer);

     [[nodiscard]] std::optional<transport::TransportCatalogue::BusInfo> GetBusStat(const std::string_view& bus_name) const;

     [[nodiscard]] transport::set_names GetBusesByStop(const std::string_view& stop_name) const;

     [[nodiscard]] svg::Document RenderMap() const;

 private:

     const transport::TransportCatalogue& db_;
     const renderer::MapRenderer& renderer_;
 };