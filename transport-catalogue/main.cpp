#include <iostream>
#include "input_reader.h"
#include "stat_reader.h"
#include "transport_catalogue.h"

using namespace std;

int main() {
    transport::TransportCatalogue catalogue;
    InputReader input_reader;
    input_reader.Input(cin, catalogue);
    InputStat(cin, catalogue);
}
