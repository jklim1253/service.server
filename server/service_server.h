#pragma once

#include "basic_server.hpp"
#include "service_session.h"

namespace network
{

using service_server = basic_server<service_session>;

} // namespace network
