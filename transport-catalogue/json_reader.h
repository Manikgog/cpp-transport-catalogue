#pragma once

#include "transport_catalogue.h"
#include "json.h"
#include "map_renderer.h"

namespace transport {

    class JsonReader {
    public:
        JsonReader();
        void Input(std::istream& in);
        void BaseRequestsProcessing(transport::TransportCatalogue& catalogue) const;
        void StatRequestsProcessing(transport::TransportCatalogue& catalogue) const;
        [[nodiscard]] renderer::RenderSettings ParseRenderSettings() const;
        void ParseRoutingSettings(const transport::TransportCatalogue& catalogue) const;

    private:
        json::Document document_;
    };

} // namespace transport