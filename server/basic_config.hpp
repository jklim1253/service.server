#pragma once

#include <iostream>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

namespace config
{

namespace pt = boost::property_tree;

template<class DataFormat>
struct basic_loader
{
    static basic_loader<DataFormat>& get()
    {
        static basic_loader<DataFormat> instance;
        return instance;
    }
    void open(std::string const& filename)
    {
        m_filename = filename;
        pt::read_xml(m_filename, m_tree);
    }
    void close()
    {
        pt::write_xml(m_filename, m_tree);
    }
    void flush(std::string const& filename)
    {
        pt::write_xml(filename, m_tree, std::locale(), pt::xml_writer_make_settings<std::string>(' ', 4));
    }
    std::string get(std::string const& node)
    {
        std::string result;
        try {
            result = m_tree.get<std::string>(node);
        }
        catch (std::exception& e)
        {
            std::cout << "exception: " << e.what() << std::endl;
            return std::string();
        }

        return result;
    }
    void set(std::string const& node, std::string const& value)
    {
        m_tree.put(node, value);
    }
    std::string operator[](std::string const& node)
    {
        return get(node);
    }

private :
    basic_loader()
    {
    }
    ~basic_loader()
    {
    }
private :
    pt::ptree m_tree;
    std::string m_filename;
};

} // namespace config
