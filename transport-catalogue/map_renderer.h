#pragma once

#include "svg.h"
#include "transport_catalogue.h"
#include <vector>

namespace renderer {
    class SphereProjector;

    struct RenderSettings {
        double width_ = 0;
        double height_ = 0;
        double padding_ = 0;
        double line_width_ = 0;
        double stop_radius_ = 0;
        int bus_label_font_size_ = 0;
        //std::vector<double> bus_label_offset_{};
        svg::Point bus_label_offset_{};
        int stop_label_font_size_ = 0;
        //std::vector<double> stop_label_offset_{};
        svg::Point stop_label_offset_{};
        svg::Color underlayer_color_{};
        double underlayer_width_ = 0;
        std::vector<svg::Color> color_palette_{};
    };


    class MapRenderer {
    public:
        MapRenderer() = default;

        void SetSettings(const RenderSettings& settings);
        [[nodiscard]] svg::Document RenderMap(const transport::TransportCatalogue& catalogue) const;

    private:
        void RenderBusesRoute(const transport::TransportCatalogue &catalogue, svg::Document &doc, const std::set<std::string_view> &sorted_bus_names, const SphereProjector& projector) const;
        void RenderBusesNames(const transport::TransportCatalogue &catalogue, svg::Document& doc, const std::set<std::string_view> &sorted_bus_names, const SphereProjector& projector) const;
        void RenderStopsCircles(const transport::TransportCatalogue &catalogue, svg::Document& doc, const std::set<std::string_view> &sorted_bus_names, const SphereProjector& projector) const;
        void RenderStopsNames(const transport::TransportCatalogue &catalogue, svg::Document& doc, const std::set<std::string_view> &sorted_bus_names, const SphereProjector& projector) const;
        RenderSettings settings_;
    };

} // namespace renderer