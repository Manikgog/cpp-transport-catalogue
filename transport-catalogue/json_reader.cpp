#include "json_reader.h"
#include "json_builder.h"
#include "graph.h"

#include <algorithm>
#include <string>
#include <iterator>
#include <iostream>

#include "router.h"

namespace transport {

JsonReader::JsonReader()
    : document_(json::Node()) {
}

void JsonReader::Input(std::istream &in) {
    document_ = json::Load(in);
}

renderer::RenderSettings JsonReader::ParseRenderSettings() const {
    renderer::RenderSettings settings;
    const auto& root_map = document_.GetRoot().AsDict();

    if (const auto it = root_map.find("render_settings"); it != root_map.end()) {
        const auto& render_settings = it->second.AsDict();
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

void JsonReader::ParseRoutingSettings(transport::TransportCatalogue &catalogue) const {
    const auto& root_map = document_.GetRoot().AsDict();
    if (const auto it = root_map.find("routing_settings"); it != root_map.end()) {
        const auto& map = it->second.AsDict();
        catalogue.SetRoutingSettings(map.at("bus_wait_time").AsInt(), map.at("bus_velocity").AsDouble());
    }
}

void JsonReader::BaseRequestsProcessing(TransportCatalogue &catalogue) const {
    const auto& root_map = document_.GetRoot().AsDict();

    if (const auto it = root_map.find("base_requests"); it != root_map.end()) {
        for (const auto& item : it->second.AsArray()) {
            const auto& item_map = item.AsDict();
            if (item_map.at("type").AsString() == "Stop") {
                std::string stop_name(item_map.at("name").AsString());
                double latitude = item_map.at("latitude").AsDouble();
                double longitude = item_map.at("longitude").AsDouble();
                catalogue.AddStop(stop_name, {latitude, longitude});
            }
        }

        for (const auto& item : it->second.AsArray()) {
            const auto& item_map = item.AsDict();
            if (item_map.at("type").AsString() == "Stop") {
                std::string from_stop_name(item_map.at("name").AsString());
                const transport::Stop* from = catalogue.FindStop(from_stop_name);
                if (from && item_map.count("road_distances")) {
                    for (const auto& [to_name, distance_node] : item_map.at("road_distances").AsDict()) {
                        const transport::Stop* to = catalogue.FindStop(to_name);
                        if (to) {
                            catalogue.AddDistance(from, to, distance_node.AsInt());
                        }
                    }
                }
            }
        }

        for (const auto& item : it->second.AsArray()) {
            const auto& item_map = item.AsDict();
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

void JsonReader::StatRequestsProcessing(TransportCatalogue &catalogue) const {
    const auto& root_map = document_.GetRoot().AsDict();
    if (const auto it = root_map.find("stat_requests"); it != root_map.end()) {
        json::Builder builder;
        builder.StartArray();

        for (const auto& item : it->second.AsArray()) {
            const auto& item_map = item.AsDict();
            const int id = item_map.at("id").AsInt();
            const std::string type = item_map.at("type").AsString();

            if (type == "Bus") {
                const std::string name = item_map.at("name").AsString();
                const auto bus_info = catalogue.GetBusInfo(name);

                builder.StartDict().Key("request_id").Value(id);

                if (bus_info) {
                    builder.Key("curvature").Value(bus_info->curvature)
                          .Key("route_length").Value(bus_info->route_length)
                          .Key("stop_count").Value(static_cast<int>(bus_info->stops_count))
                          .Key("unique_stop_count").Value(static_cast<int>(bus_info->unique_stops));
                } else {
                    builder.Key("error_message").Value("not found");
                }

                builder.EndDict();
            }
            else if (type == "Stop") {
                const std::string name = item_map.at("name").AsString();
                const auto buses = catalogue.GetBusesByStopName(name);

                builder.StartDict().Key("request_id").Value(id);

                if (buses) {
                    builder.Key("buses").StartArray();
                    for (const auto& bus : *buses.value()) {
                        builder.Value(std::string(bus));
                    }
                    builder.EndArray();
                } else {
                    builder.Key("error_message").Value("not found");
                }

                builder.EndDict();
            }
            else if (type == "Map") {
                renderer::RenderSettings settings = ParseRenderSettings();
                renderer::MapRenderer mapRenderer;
                mapRenderer.SetSettings(settings);
                svg::Document doc = mapRenderer.RenderMap(catalogue);
                std::ostringstream oss;
                doc.Render(oss);
                builder.StartDict()
                .Key("request_id").Value(id)
                .Key("map").Value(oss.str())
                .EndDict();
            }
            else if (type == "Route") {
                const std::string from = item_map.at("from").AsString();
                const std::string to = item_map.at("to").AsString();

                builder.StartDict().Key("request_id").Value(id);

                if (auto route_info = catalogue.FindRoute(from, to)) {
                    builder.Key("total_time").Value(route_info->total_time)
                          .Key("items").StartArray();

                    for (const auto& item : route_info->items) {
                        if (std::holds_alternative<std::pair<std::string, double>>(item)) {
                            const auto& wait_item = std::get<std::pair<std::string, double>>(item);
                            builder.StartDict()
                                  .Key("type").Value("Wait")
                                  .Key("stop_name").Value(wait_item.first)
                                  .Key("time").Value(wait_item.second)
                                  .EndDict();
                        } else {
                            const auto& bus_item = std::get<std::tuple<std::string, std::string, int, double>>(item);
                            builder.StartDict()
                                  .Key("type").Value("Bus")
                                  .Key("bus").Value(std::get<0>(bus_item))
                                  .Key("span_count").Value(std::get<2>(bus_item))
                                  .Key("time").Value(std::get<3>(bus_item))
                                  .EndDict();
                        }
                    }

                    builder.EndArray();
                } else {
                    builder.Key("error_message").Value("not found");
                }

                builder.EndDict();
            }
        }

        builder.EndArray();
        json::Print(json::Document{builder.Build()}, std::cout);
    }
}
} // namespace transport