#include "input_reader.h"
#include "geo.h"
#include "transport_catalogue.h"
#include <cassert>
#include <iterator>

Coordinates ParseCoordinates(std::string_view str) {
    static const double nan = std::nan("");

    auto not_space = str.find_first_not_of(' ');
    auto comma = str.find(',');

    if (comma == str.npos) {
        return {nan, nan};
    }

    auto not_space2 = str.find_first_not_of(' ', comma + 1);

    double lat = std::stod(std::string(str.substr(not_space, comma - not_space)));
    double lng = std::stod(std::string(str.substr(not_space2)));

    return {lat, lng};
}

std::string_view Trim(std::string_view string) {
    const auto start = string.find_first_not_of(' ');
    if (start == string.npos) {
        return {};
    }
    return string.substr(start, string.find_last_not_of(' ') + 1 - start);
}

void ParseDistances(const std::string_view from_stop_name, std::string_view str, transport::TransportCatalogue& catalogue) {
    size_t comma_pos = str.find(',');
    if (comma_pos == str.npos) {
        return;
    }
    comma_pos = str.find(',', comma_pos + 1);
    if (comma_pos == str.npos) {
        return;
    }
    size_t distances_start = str.find_first_not_of(' ', comma_pos + 1);
    if (distances_start == str.npos) {
        return;
    }
    size_t pos = distances_start;
    while (pos < str.size()) {
        size_t m_pos = str.find("m to", pos);
        if (m_pos == str.npos) {
            break;
        }
        std::string_view dist_str = str.substr(pos, m_pos - pos);
        dist_str = Trim(dist_str);
        int distance = std::stoi(std::string(dist_str));
        size_t stop_start = m_pos + 4;
        size_t stop_end = str.find(',', stop_start);
        if (stop_end == str.npos) {
            stop_end = str.size();
        }

        std::string_view to_stop_name = str.substr(stop_start, stop_end - stop_start);
        to_stop_name = Trim(to_stop_name);
        const auto* from_stop = catalogue.FindStop(from_stop_name);
        const auto* to_stop = catalogue.FindStop(to_stop_name);
        if (from_stop && to_stop) {
            catalogue.AddDistance(from_stop, to_stop, distance);
        }
        pos = stop_end;
        if (pos != str.npos && str[pos] == ',') {
            pos++;
        }
    }
}

std::vector<std::string_view> Split(std::string_view string, char delim) {
    std::vector<std::string_view> result;

    size_t pos = 0;
    while ((pos = string.find_first_not_of(' ', pos)) < string.length()) {
        auto delim_pos = string.find(delim, pos);
        if (delim_pos == string.npos) {
            delim_pos = string.size();
        }
        if (auto substr = Trim(string.substr(pos, delim_pos - pos)); !substr.empty()) {
            result.push_back(substr);
        }
        pos = delim_pos + 1;
    }

    return result;
}

std::vector<std::string_view> ParseRoute(std::string_view route) {
    if (route.find('>') != route.npos) {
        return Split(route, '>');
    }

    auto stops = Split(route, '-');
    std::vector<std::string_view> results(stops.begin(), stops.end());
    results.insert(results.end(), std::next(stops.rbegin()), stops.rend());

    return results;
}

CommandDescription ParseCommandDescription(std::string_view line) {
    auto colon_pos = line.find(':');
    if (colon_pos == line.npos) {
        return {};
    }

    auto space_pos = line.find(' ');
    if (space_pos >= colon_pos) {
        return {};
    }

    auto not_space = line.find_first_not_of(' ', space_pos);
    if (not_space >= colon_pos) {
        return {};
    }

    return {std::string(line.substr(0, space_pos)),
            std::string(line.substr(not_space, colon_pos - not_space)),
            std::string(line.substr(colon_pos + 1))};
}

void InputReader::ParseLine(std::string_view line) {
    auto command_description = ParseCommandDescription(line);
    if (command_description) {
        commands_.push_back(std::move(command_description));
    }
}

void InputReader::ApplyCommands(transport::TransportCatalogue& catalogue) const {
    for (const auto& cmd : commands_) {
        if (cmd.command == "Stop") {
            auto coords = ParseCoordinates(cmd.description);
            catalogue.AddStop(cmd.id, coords);
        }
    }

    for (const auto& cmd : commands_) {
        if (cmd.command == "Stop") {
            ParseDistances(cmd.id, cmd.description, catalogue);
        }
    }

    for (const auto& cmd : commands_) {
        if (cmd.command == "Bus") {
            bool is_roundtrip = cmd.description.find('>') != std::string_view::npos;
            auto stops = ParseRoute(cmd.description);
            catalogue.AddBus(cmd.id, stops, is_roundtrip);
        }
    }
}

void InputReader::Input(std::istream& in, transport::TransportCatalogue& catalogue) {
    int base_request_count;
    in >> base_request_count >> std::ws;
    InputReader reader;
    for (int i = 0; i < base_request_count; ++i) {
        std::string line;
        getline(in, line);
        reader.ParseLine(line);
    }
    reader.ApplyCommands(catalogue);
}
