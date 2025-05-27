#include "stat_reader.h"
#include "transport_catalogue.h"
#include <iomanip>
#include <iostream>

void ParseAndPrintStat(const transport::TransportCatalogue& catalogue, std::string_view request, std::ostream& output) {
    if (request.substr(0, 4) == "Bus ") {
        std::string_view bus_name = request.substr(4);
        auto info = catalogue.GetBusInfo(bus_name);

        output << "Bus " << bus_name << ": ";
        if (!info) {
            output << "not found\n";
            return;
        }

        output << info->stops_count << " stops on route, "
               << info->unique_stops << " unique stops, "
               << std::setprecision(6) << info->route_length << " route length, "
               << std::setprecision(6) << info->curvature << " curvature\n";
        return;
    }

    if(request.substr(0, 5) == "Stop ") {
        std::string_view stop_name = request.substr(5);
        auto info = catalogue.GetBusesByStopName(stop_name);

        output << "Stop " << stop_name << ": ";
        if (!info) {
            output << "not found\n";
            return;
        }
        if(info.has_value() && info.value()->empty()) {
            output << "no buses\n";
            return;
        }
        output << "buses ";
        for(const auto & stop : *info.value()) {
            output << stop << " ";
        }
        output << "\n";
        return;
    }
}

void InputStat(std::istream& in, transport::TransportCatalogue& catalogue) {
    int stat_request_count;
    in >> stat_request_count >> std::ws;
    for (int i = 0; i < stat_request_count; ++i) {
        std::string line;
        getline(in, line);
        ParseAndPrintStat(catalogue, line, std::cout);
    }
}
