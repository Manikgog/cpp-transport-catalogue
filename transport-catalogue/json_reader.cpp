#include "json_reader.h"

#include <algorithm>
#include <string>
#include <iterator>
#include <iostream>
#include <sstream>

namespace transport {

JsonReader::JsonReader()
    : document_(json::Node()) {
}

void JsonReader::Input(std::istream &in) {
    document_ = json::Load(in);
}

renderer::RenderSettings JsonReader::ParseRenderSettings() const {
    renderer::RenderSettings settings;
    const auto& root_map = document_.GetRoot().AsMap();

    if (const auto it = root_map.find("render_settings"); it != root_map.end()) {
        const auto& render_settings = it->second.AsMap();
        settings.width_ = render_settings.at("width").AsDouble();
        settings.height_ = render_settings.at("height").AsDouble();
        settings.padding_ = render_settings.at("padding").AsDouble();
        settings.line_width_ = render_settings.at("line_width").AsDouble();
        settings.stop_radius_ = render_settings.at("stop_radius").AsDouble();
        settings.bus_label_font_size_ = render_settings.at("bus_label_font_size").AsInt();
        const auto& bus_label_offset = render_settings.at("bus_label_offset").AsArray();
        settings.bus_label_offset_ = {bus_label_offset[0].AsDouble(), bus_label_offset[1].AsDouble()};
        settings.stop_label_font_size_ = render_settings.at("stop_label_font_size").AsInt();
        const auto& stop_label_offset = render_settings.at("stop_label_offset").AsArray();
        settings.stop_label_offset_ = {stop_label_offset[0].AsDouble(), stop_label_offset[1].AsDouble()};
        const auto& underlayer_color = render_settings.at("underlayer_color");
        if (underlayer_color.IsString()) {
            settings.underlayer_color_ = underlayer_color.AsString();
        } else if (underlayer_color.IsArray()) {
            const auto& color_array = underlayer_color.AsArray();
            if (color_array.size() == 3) {
                settings.underlayer_color_ = svg::Rgb{
                    static_cast<uint8_t>(color_array[0].AsInt()),
                    static_cast<uint8_t>(color_array[1].AsInt()),
                    static_cast<uint8_t>(color_array[2].AsInt())
                };
            } else if (color_array.size() == 4) {
                settings.underlayer_color_ = svg::Rgba{
                    static_cast<uint8_t>(color_array[0].AsInt()),
                    static_cast<uint8_t>(color_array[1].AsInt()),
                    static_cast<uint8_t>(color_array[2].AsInt()),
                    color_array[3].AsDouble()
                };
            }
        }

        settings.underlayer_width_ = render_settings.at("underlayer_width").AsDouble();
        for (const auto& color_node : render_settings.at("color_palette").AsArray()) {
            if (color_node.IsString()) {
                settings.color_palette_.emplace_back(color_node.AsString());
            } else if (color_node.IsArray()) {
                const auto& color_array = color_node.AsArray();
                if (color_array.size() == 3) {
                    settings.color_palette_.emplace_back(svg::Rgb{
                        static_cast<uint8_t>(color_array[0].AsInt()),
                        static_cast<uint8_t>(color_array[1].AsInt()),
                        static_cast<uint8_t>(color_array[2].AsInt())
                    });
                } else if (color_array.size() == 4) {
                    settings.color_palette_.emplace_back(svg::Rgba{
                        static_cast<uint8_t>(color_array[0].AsInt()),
                        static_cast<uint8_t>(color_array[1].AsInt()),
                        static_cast<uint8_t>(color_array[2].AsInt()),
                        color_array[3].AsDouble()
                    });
                }
            }
        }
    }
    return settings;
}

void JsonReader::BaseRequestsProcessing(transport::TransportCatalogue &catalogue) const {
    const auto& root_map = document_.GetRoot().AsMap();

    if (const auto it = root_map.find("base_requests"); it != root_map.end()) {
        for (const auto& item : it->second.AsArray()) {
            const auto& item_map = item.AsMap();
            if (item_map.at("type").AsString() == "Stop") {
                std::string stop_name(item_map.at("name").AsString());
                double latitude = item_map.at("latitude").AsDouble();
                double longitude = item_map.at("longitude").AsDouble();
                catalogue.AddStop(stop_name, {latitude, longitude});
            }
        }

        for (const auto& item : it->second.AsArray()) {
            const auto& item_map = item.AsMap();
            if (item_map.at("type").AsString() == "Stop") {
                std::string from_stop_name(item_map.at("name").AsString());
                const transport::Stop* from = catalogue.FindStop(from_stop_name);
                if (from && item_map.count("road_distances")) {
                    for (const auto& [to_name, distance_node] : item_map.at("road_distances").AsMap()) {
                        const transport::Stop* to = catalogue.FindStop(to_name);
                        if (to) {
                            catalogue.AddDistance(from, to, distance_node.AsInt());
                        }
                    }
                }
            }
        }

        for (const auto& item : it->second.AsArray()) {
            const auto& item_map = item.AsMap();
            if (item_map.at("type").AsString() == "Bus") {
                std::string name = item_map.at("name").AsString();
                bool is_roundtrip = item_map.at("is_roundtrip").AsBool();
                std::vector<std::string_view> stop_names;
                for (const auto& stop_node : item_map.at("stops").AsArray()) {
                    stop_names.push_back(stop_node.AsString());
                }
                catalogue.AddBus(name, stop_names, is_roundtrip);
            }
        }
    }
}

void JsonReader::StatRequestsProcessing(transport::TransportCatalogue &catalogue) const {
    const auto& root_map = document_.GetRoot().AsMap();
    if (const auto it = root_map.find("stat_requests"); it != root_map.end()) {
        json::Array responses;

        for (const auto& item : it->second.AsArray()) {
            const auto& item_map = item.AsMap();
            int request_id = item_map.at("id").AsInt();
            std::string type = item_map.at("type").AsString();
            json::Dict response;
            response.emplace("request_id", request_id);

            if (type == "Stop") {
                std::string stop_name = item_map.at("name").AsString();
                const transport::Stop* stop = catalogue.FindStop(stop_name);

                if (stop) {
                    auto buses_opt = catalogue.GetBusesByStopName(stop_name);
                    json::Array buses_array;
                    if (buses_opt) {
                        for (const auto& bus_name : *buses_opt.value()) {
                            buses_array.emplace_back(std::string(bus_name));
                        }
                    }
                    response.emplace("buses", buses_array);
                } else {
                    response.emplace("error_message", std::string("not found"));
                }
            }
            else if (type == "Bus") {
                std::string bus_name = item_map.at("name").AsString();
                auto bus_info_opt = catalogue.GetBusInfo(bus_name);

                if (bus_info_opt) {
                    const auto& bus_info = bus_info_opt.value();
                    response.emplace("curvature", bus_info.curvature);
                    response.emplace("route_length", bus_info.route_length);
                    response.emplace("stop_count", static_cast<int>(bus_info.stops_count));
                    response.emplace("unique_stop_count", static_cast<int>(bus_info.unique_stops));
                } else {
                    response.emplace("error_message", std::string("not found"));
                }
            }
            else if (type == "Map") {
                renderer::RenderSettings settings = ParseRenderSettings();
                renderer::MapRenderer mapRenderer;
                mapRenderer.SetSettings(settings);
                svg::Document doc = mapRenderer.RenderMap(catalogue);
                std::ostringstream oss;
                doc.Render(oss);
                response.emplace("map", oss.str());
            }

            responses.emplace_back(response);
        }

        json::Print(json::Document(responses), std::cout);
    }
}
} // namespace transport