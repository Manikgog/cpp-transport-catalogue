#include <iostream>
#include <string>

#include "input_reader.h"
#include "stat_reader.h"
#include "transport_catalogue.h"
using namespace std;

//int main() {
//    transport::TransportCatalogue catalogue;

//    std::vector<std::string> input;
//    input.push_back("Stop Tolstopaltsevo: 55.611087, 37.20829");
//    input.push_back("Stop Marushkino: 55.595884, 37.209755");
//    input.push_back("Bus 256: Biryulyovo Zapadnoye > Biryusinka > Universam > Biryulyovo Tovarnaya > Biryulyovo Passazhirskaya > Biryulyovo Zapadnoye");
//    input.push_back("Bus 750: Tolstopaltsevo - Marushkino - Rasskazovka");
//    input.push_back("Stop Rasskazovka: 55.632761, 37.333324");
//    input.push_back("Stop Biryulyovo Zapadnoye: 55.574371, 37.6517");
//    input.push_back("Stop Biryusinka: 55.581065, 37.64839");
//    input.push_back("Stop Universam: 55.587655, 37.645687");
//    input.push_back("Stop Biryulyovo Tovarnaya: 55.592028, 37.653656");
//    input.push_back("Stop Biryulyovo Passazhirskaya: 55.580999, 37.659164");
//    input.push_back("Bus 828: Biryulyovo Zapadnoye > Universam > Rossoshanskaya ulitsa > Biryulyovo Zapadnoye");
//    input.push_back("Stop Rossoshanskaya ulitsa: 55.595579, 37.605757");
//    input.push_back("Stop Prazhskaya: 55.611678, 37.603831");

//    {
//        InputReader reader;
//        for (size_t i = 0; i < input.size(); ++i) {
//            reader.ParseLine(input.at(i));
//        }
//        reader.ApplyCommands(catalogue);
//    }

//    input.clear();
//    input.push_back("Bus 256");
//    input.push_back("Bus 750");
//    input.push_back("Bus 751");
//    input.push_back("Stop Samara");
//    input.push_back("Stop Prazhskaya");
//    input.push_back("Stop Biryulyovo Zapadnoye");
//    for (size_t i = 0; i < input.size(); ++i) {
//        ParseAndPrintStat(catalogue, input.at(i), cout);
//    }
//}

int main() {
    transport::TransportCatalogue catalogue;

    int base_request_count;
    cin >> base_request_count >> ws;

    {
        InputReader reader;
        for (int i = 0; i < base_request_count; ++i) {
            string line;
            getline(cin, line);
            reader.ParseLine(line);
        }
        reader.ApplyCommands(catalogue);
    }

    int stat_request_count;
    cin >> stat_request_count >> ws;
    for (int i = 0; i < stat_request_count; ++i) {
        string line;
        getline(cin, line);
        ParseAndPrintStat(catalogue, line, cout);
    }
}
