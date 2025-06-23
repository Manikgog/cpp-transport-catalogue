#include "map_renderer.h"

#include <algorithm>
#include <cmath>

namespace renderer {

inline const double EPSILON = 1e-6;
bool IsZero(double value) {
    return std::abs(value) < EPSILON;
}

class SphereProjector {
public:
    // points_begin и points_end задают начало и конец интервала элементов geo::Coordinates
    template <typename PointInputIt>
    SphereProjector(PointInputIt points_begin, PointInputIt points_end,
                    double max_width, double max_height, double padding)
        : padding_(padding) //
    {
        // Если точки поверхности сферы не заданы, вычислять нечего
        if (points_begin == points_end) {
            return;
        }

        // Находим точки с минимальной и максимальной долготой
        const auto [left_it, right_it] = std::minmax_element(
            points_begin, points_end,
            [](auto lhs, auto rhs) { return lhs.lng < rhs.lng; });
        min_lon_ = left_it->lng;
        const double max_lon = right_it->lng;

        // Находим точки с минимальной и максимальной широтой
        const auto [bottom_it, top_it] = std::minmax_element(
            points_begin, points_end,
            [](auto lhs, auto rhs) { return lhs.lat < rhs.lat; });
        const double min_lat = bottom_it->lat;
        max_lat_ = top_it->lat;

        // Вычисляем коэффициент масштабирования вдоль координаты x
        std::optional<double> width_zoom;
        if (!IsZero(max_lon - min_lon_)) {
            width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
        }

        // Вычисляем коэффициент масштабирования вдоль координаты y
        std::optional<double> height_zoom;
        if (!IsZero(max_lat_ - min_lat)) {
            height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
        }

        if (width_zoom && height_zoom) {
            // Коэффициенты масштабирования по ширине и высоте ненулевые,
            // берём минимальный из них
            zoom_coeff_ = std::min(*width_zoom, *height_zoom);
        } else if (width_zoom) {
            // Коэффициент масштабирования по ширине ненулевой, используем его
            zoom_coeff_ = *width_zoom;
        } else if (height_zoom) {
            // Коэффициент масштабирования по высоте ненулевой, используем его
            zoom_coeff_ = *height_zoom;
        }
    }

    // Проецирует широту и долготу в координаты внутри SVG-изображения
    svg::Point operator()(const geo::Coordinates coords) const {
        return {
            (coords.lng - min_lon_) * zoom_coeff_ + padding_,
            (max_lat_ - coords.lat) * zoom_coeff_ + padding_
        };
    }

private:
    double padding_;
    double min_lon_ = 0;
    double max_lat_ = 0;
    double zoom_coeff_ = 0;
};

void MapRenderer::SetSettings(const RenderSettings& settings) {
    settings_ = settings;
}

svg::Document renderer::MapRenderer::RenderMap(const transport::TransportCatalogue &catalogue) const {
    svg::Document doc;
    std::set<std::string_view> sorted_bus_names = catalogue.GetBusNames();
    std::vector<geo::Coordinates> all_coords;
    for (const auto& name : sorted_bus_names) {
        const auto* bus = catalogue.FindBus(name);
        if (!bus || bus->stops.empty()) {
            continue;
        }
        for (const auto* stop : bus->stops) {
            all_coords.push_back(stop->coordinates);
        }
    }

    SphereProjector projector(
        all_coords.begin(), all_coords.end(),
        settings_.width_, settings_.height_, settings_.padding_
    );
    RenderBusesRoute(catalogue, doc, sorted_bus_names, projector);
    RenderBusesNames(catalogue, doc, sorted_bus_names, projector);
    RenderStopsCircles(catalogue, doc, sorted_bus_names, projector);
    RenderStopsNames(catalogue, doc, sorted_bus_names, projector);
    return doc;
}

void MapRenderer::RenderBusesRoute(const transport::TransportCatalogue &catalogue
                                    , svg::Document &doc
                                    , const std::set<std::string_view> &sorted_bus_names
                                    , const SphereProjector& projector) const {
    size_t bus_index = 0;
    size_t color_index = 0;
    for (const auto& name : sorted_bus_names) {
        const auto* bus = catalogue.FindBus(name);
        if (!bus || bus->stops.empty()) {
            continue;
        }

        svg::Polyline polyline;

        for (const auto* stop : bus->stops) {
            polyline.AddPoint(projector(stop->coordinates));
        }

        if (!bus->is_roundtrip) {
            for (auto it = bus->stops.rbegin() + 1; it != bus->stops.rend(); ++it) {
                polyline.AddPoint(projector((*it)->coordinates));
            }
        }


        if (color_index == settings_.color_palette_.size()) {
            color_index = 0;
        }
        const svg::Color color = settings_.color_palette_[color_index];
        polyline.SetFillColor("none")
               .SetStrokeColor(color)
               .SetStrokeWidth(settings_.line_width_)
               .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
               .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

        doc.Add(polyline);
        bus_index++;
        color_index++;
    }
}

void MapRenderer::RenderBusesNames(const transport::TransportCatalogue &catalogue
                                    , svg::Document &doc
                                    , const std::set<std::string_view> &sorted_bus_names
                                    , const SphereProjector& projector) const {
    size_t bus_index = 0;
    size_t color_index = 0;
    for (const auto& name : sorted_bus_names) {
        const auto* bus = catalogue.FindBus(name);
        if (!bus || bus->stops.empty()) {
            continue;
        }

        if (color_index == settings_.color_palette_.size()) {
            color_index = 0;
        }
        const svg::Color color = settings_.color_palette_[color_index];

        svg::Text bus_title_start = svg::Text()
            .SetFontFamily("Verdana")
            .SetFontSize(settings_.bus_label_font_size_)
            .SetPosition(projector({bus->stops.front()->coordinates.lat, bus->stops.front()->coordinates.lng}))
            .SetOffset({settings_.bus_label_offset_.x, settings_.bus_label_offset_.y})
            .SetData(bus->name)
            .SetFontWeight("bold");
        doc.Add(svg::Text{bus_title_start}
            .SetStrokeColor(settings_.underlayer_color_)
            .SetFillColor(settings_.underlayer_color_)
            .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND)
            .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
            .SetStrokeWidth(settings_.underlayer_width_)
            .SetFontWeight("bold"));
        doc.Add(svg::Text{bus_title_start}.SetFillColor(color));

        if (!bus->is_roundtrip) {
            if (bus->stops.front() == bus->stops.back()) {
                bus_index++;
                color_index++;
                continue;
            }
            svg::Text bus_title_end = svg::Text()
            .SetFontFamily("Verdana")
            .SetFontSize(settings_.bus_label_font_size_)
            .SetPosition(projector({bus->stops.back()->coordinates.lat, bus->stops.back()->coordinates.lng}))
            .SetOffset({settings_.bus_label_offset_.x, settings_.bus_label_offset_.y})
            .SetData(bus->name)
            .SetFontWeight("bold");
            doc.Add(svg::Text{bus_title_end}
                .SetStrokeColor(settings_.underlayer_color_)
                .SetFillColor(settings_.underlayer_color_)
                .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND)
                .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
                .SetStrokeWidth(settings_.underlayer_width_)
                .SetFontWeight("bold"));
            doc.Add(svg::Text{bus_title_end}.SetFillColor(color));
        }
        bus_index++;
        color_index++;
    }
}

void MapRenderer::RenderStopsCircles(const transport::TransportCatalogue &catalogue, svg::Document &doc, const std::set<std::string_view> &sorted_bus_names, const SphereProjector& projector) const {
    std::set<std::string_view> stops_names;
    for (const auto& name : sorted_bus_names) {
        const auto* bus = catalogue.FindBus(name);
        for (const auto* stop : bus->stops) {
            stops_names.insert(stop->name);
        }
    }
    for (const auto& stop_name : stops_names) {
        const transport::Stop *stop = catalogue.FindStop(stop_name);
        svg::Circle stop_circle;
        stop_circle.SetCenter(projector({stop->coordinates}))
                    .SetRadius(settings_.stop_radius_)
                    .SetFillColor("white");
        doc.Add(stop_circle);
    }
}

void MapRenderer::RenderStopsNames(const transport::TransportCatalogue &catalogue, svg::Document &doc, const std::set<std::string_view> &sorted_bus_names, const SphereProjector& projector) const {
    std::set<std::string_view> stops_names;
    for (const auto& name : sorted_bus_names) {
        const auto* bus = catalogue.FindBus(name);
        for (const auto* stop : bus->stops) {
            stops_names.insert(stop->name);
        }
    }
    for (const auto& stop_name : stops_names) {
        const transport::Stop *stop = catalogue.FindStop(stop_name);
        if (!stop) {
            continue;
        }

         const svg::Text stop_title = svg::Text()
                     .SetFillColor("black")
                     .SetFontFamily("Verdana")
                     .SetFontSize(settings_.stop_label_font_size_)
                     .SetPosition(projector({stop->coordinates}))
                     .SetOffset(svg::Point{settings_.stop_label_offset_.x, settings_.stop_label_offset_.y})
                     .SetData(stop->name);
         doc.Add(svg::Text{stop_title}
             .SetStrokeColor(settings_.underlayer_color_)
             .SetFillColor(settings_.underlayer_color_)
             .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND)
             .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
             .SetStrokeWidth(settings_.underlayer_width_));

        doc.Add(stop_title);
    }
}
} // namespace renderer