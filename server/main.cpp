#include <iostream>
#include <string>

#include "basic_config.hpp"
#include "service_server.h"

struct global_setting
{
    std::string port;
};
using global_config = config::basic_loader<global_setting>;

int main(int argc, char** argv)
{
    try {

        global_config::get().open("server.xml");
        unsigned short port = std::stoi(global_config::get().get("configuration.server.port"));

        network::service_server serv;

        serv.open(port);

        serv.close();

        global_config::get().close();
    }
    catch (std::exception& e)
    {
        std::cout << "exception: " << e.what() << std::endl;
    }

    std::cin.get();
    return 0;
}