#pragma once

#include <iosfwd>
#include <string_view>

#include "transport_catalogue.h"

void ParseAndPrintStat(const transport::TransportCatalogue& tansport_catalogue
                       , std::string_view request
                       , std::ostream& output);

void InputStat(std::istream& in
               , transport::TransportCatalogue& catalogue);
