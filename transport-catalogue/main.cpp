#include "transport_catalogue.h"
#include "json_reader.h"
#include <fstream>
#include <iostream>

int main() {
    //LOG_DURATION("ALL PROGRAM");
    // std::ifstream file("/home/maksim/CLionProjects/transport_catalogue_by_deepseek/cmake-build-debug/s10_final_opentest_3.json");
    // if (!file) {
    //     std::cerr << "Failed to open file!" << std::endl;
    //     return 1;
    // }

    transport::TransportCatalogue transportCatalogue;
    transport::JsonReader jsonReader;

    jsonReader.Input(std::cin);

    jsonReader.BaseRequestsProcessing(transportCatalogue);
    jsonReader.ParseRoutingSettings(transportCatalogue);
    renderer::RenderSettings render_settings = jsonReader.ParseRenderSettings();
    renderer::MapRenderer renderer;
    renderer.SetSettings(render_settings);
    svg::Document doc = renderer.RenderMap(transportCatalogue);
    transportCatalogue.BuildRouter();
    jsonReader.StatRequestsProcessing(transportCatalogue);
}