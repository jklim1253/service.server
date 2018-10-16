#pragma once

#include <string>
#include <memory>
#include <deque>
#include <map>

#ifndef _WIN32_WINNT
#   define _WIN32_WINNT 0x0A00
#endif
#include <boost/asio.hpp>

namespace network
{

namespace asio = boost::asio;
using tcp = asio::ip::tcp;

struct service_session; // forward

struct Action
{
    enum mode_type : int
    {
        read,
        write,
    };

    struct write_type
    {
        enum type : int
        {
            unknown_type,
            string_type,
            file_type,
        };
    };

    static const char sc_line_seperator = '\n';
    static const char sc_pair_seperator = '=';
    static const char* sc_protocol_identifier;

    std::size_t size() const { return m_size; }

    virtual int mode() const = 0;
    virtual int type() const = 0;
    virtual int operator()(std::istream&) { return -1; }
    virtual int operator()(std::ostream&) { return -1; }

    void parse_header(std::istream& is)
    {
        std::string line;
        std::getline(is, line, sc_line_seperator);

        if (line != sc_protocol_identifier)
        {
            throw std::exception("header parsing error");
        }

        while (is)
        {
            std::getline(is, line, sc_line_seperator);

            if (line.empty()) continue;

            auto pos = line.find(sc_pair_seperator);
            auto tag = line.substr(0, pos);
            auto value = line.substr(pos + 1);

            m_property[tag] = value;
        }
        is.clear(); // clear istream error bit
    }
    void put_property(std::string const& tag, std::string const& value)
    {
        m_property[tag] = value;
    }
    std::string get_property(std::string const& tag) const
    {
        auto target = m_property.find(tag);
        if (target == m_property.end())
            return std::string();

        return target->second;
    }
    std::string& operator[](std::string const& tag)
    {
        return m_property[tag];
    }
    void prepare_header(std::ostream& os)
    {
        os << sc_protocol_identifier << sc_line_seperator;
        for (auto const& e : m_property)
        {
            os << e.first << sc_pair_seperator << e.second << sc_line_seperator;
        }
        os << sc_line_seperator;
    }

protected :
    std::size_t m_size;
    std::map<std::string, std::string> m_property;
};

struct read : public Action
{
    int mode() const override final { return Action::read; }
    int type() const override final
    {
        if (get_property("type") == "string_type")
            return Action::write_type::string_type;
        else if (get_property("type") == "file_type")
            return Action::write_type::file_type;

        return Action::write_type::unknown_type;
    }
    int operator()(std::istream& is) override
    {

        return 0;
    }
};
struct write_string : public Action
{
    write_string(std::string const& value) : m_value(value) {}

    int mode() const override final { return Action::write; }
    int type() const override final { return Action::write_type::string_type; }
    int operator()(std::ostream& os) override
    {

        return 0;
    }
private :
    std::string m_value;
};
struct write_file : public Action
{
    write_file(std::string const& value) : m_value(value) {}

    int mode() const override final { return Action::write; }
    int type() const override final { return Action::write_type::file_type; }
    int operator()(std::ostream& os) override
    {

        return 0;
    }
private :
    std::string m_value;
};

struct service_session
{
    friend class std::_Ref_count_obj<service_session>;

    typedef service_session Me;
    typedef service_session session_type;
    typedef std::shared_ptr<session_type> session_ptr;

    typedef Action action_type;
    typedef std::shared_ptr<action_type> action_ptr;

    static session_ptr make(asio::io_context& m_ioc)
    {
        return std::make_shared<session_type>(m_ioc);
    }
    void execute()
    {
        m_action_depot.push_back(std::make_shared<read>());
        do_execute();
    }
    tcp::socket& socket()
    {
        return m_socket;
    }

    ~service_session()
    {
        if (m_socket.is_open())
        {
            m_socket.shutdown(tcp::socket::shutdown_both);
            m_socket.close();
        }
    }
private :
    service_session(asio::io_context& ioc)
        : m_ioc(ioc)
        , m_socket(ioc)
        , m_read_buffer()
        , m_write_buffer()
        , m_read_stream(&m_read_buffer)
        , m_write_stream(&m_write_buffer)
    {
    }

    void do_execute()
    {
        auto action = m_action_depot.front();
        if (action->mode() == Action::read)
        {
            do_read(action);
        }
    }
    void do_read(action_ptr action)
    {
        static const char neck[] = { Action::sc_line_seperator, Action::sc_line_seperator, '\0' };

        asio::async_read_until(
            m_socket,
            m_read_buffer,
            neck,
            std::bind(&Me::on_read_header, this, std::placeholders::_1, std::placeholders::_2, action)
        );
    }
    void on_read_header(const boost::system::error_code& ec, std::size_t bytes, action_ptr action)
    {
        if (ec)
        {
            on_finish(ec, "read header");
            return;
        }

        // parse header
        action->parse_header(m_read_stream);

        asio::async_read(
            m_socket,
            m_read_buffer,
            std::bind(&Me::on_read_completion, this, std::placeholders::_1, std::placeholders::_2),
            std::bind(&Me::on_read_body, this, std::placeholders::_1, std::placeholders::_2, action)
        );
    }
    std::size_t on_read_completion(const boost::system::error_code& ec, std::size_t bytes)
    {
        if (ec)
        {
            on_finish(ec, "read completion");
            return bytes; // TODO : on error, determine return value.
        }

        // TODO : calculate read completion size

        return bytes;
    }
    void on_read_body(const boost::system::error_code& ec, std::size_t bytes, action_ptr action)
    {
        if (ec)
        {
            on_finish(ec, "read body");
            return;
        }

        on_finish(ec, "read body");
    }
    void do_write(action_ptr action)
    {

    }

    void on_finish(const boost::system::error_code& ec, const char* phase)
    {
        if (!ec)
        {
            // TODO : successful operation done, execute next action
            m_action_depot.pop_front();

            do_execute();

            return;
        }
        else if (ec && ec.value() == asio::error::operation_aborted)
        {
            // TODO : cancel all enqueued actions
            m_action_depot.clear();

            return;
        }

        // TODO : error process,

        return;
    }

private :
    asio::io_context& m_ioc;
    tcp::socket m_socket;

    std::deque<action_ptr> m_action_depot;

    asio::streambuf m_read_buffer;
    asio::streambuf m_write_buffer;
    std::istream m_read_stream;
    std::istream m_write_stream;
};

} // namespace network
