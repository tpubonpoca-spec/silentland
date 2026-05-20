#pragma once

#include "types.hpp"

namespace dppbot {

class SiteMetadataClient {
public:
    SiteCatalog FetchCatalog() const;
};

}  // namespace dppbot
