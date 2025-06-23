#include "domain.h"

static constexpr size_t tiny_encryption_algorithm_number = 0x9e3779b9;

size_t transport::StopPairHasher::operator()(const std::pair<const Stop*, const Stop*>& pair_of_stops) const {
    auto hash1 = std::hash<const Stop*>{}(pair_of_stops.first);
    auto hash2 = std::hash<const Stop*>{}(pair_of_stops.second);
    return hash1 ^ (hash2 + tiny_encryption_algorithm_number + (hash1 << 6) + (hash1 >> 2));
}