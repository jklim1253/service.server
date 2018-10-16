#pragma once

#include <string>
#include <functional>

#include <boost/signals2.hpp>

namespace util
{

template<class KeyType = std::string>
struct event_handler
{
    void append_handler(std::string name, std::string hid, std::function<int(std::string)> handler)
    {

    }
    void remove_handler(std::string name, std::string hid)
    {

    }
    void invoke_handler(std::string name, std::string args)
    {

    }

private :
};


} // namespace util
