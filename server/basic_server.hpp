#pragma once

#include <thread>
#include <list>
#include <memory>

#ifndef _WIN32_WINNT
#   define _WIN32_WINNT 0x0A00
#endif

#include <boost/asio.hpp>

namespace network
{

namespace asio = boost::asio;
using tcp = asio::ip::tcp;

template<class SessionLayer>
struct basic_server
{
    typedef SessionLayer session_type;
    typedef std::shared_ptr<session_type> session_ptr;
    basic_server()
        : m_ioc()
        , m_work_guard(asio::make_work_guard(m_ioc))
        , m_server_thread([&] { m_ioc.run(); })
        , m_acceptor(m_ioc)
    {
    }
    ~basic_server()
    {
        close();

        m_work_guard.reset();
        m_server_thread.join();
    }
    void open(unsigned short port)
    {
        m_port = port;

        tcp::endpoint ep(asio::ip::address(), m_port);
        m_acceptor.open(ep.protocol());
        m_acceptor.set_option(tcp::acceptor::reuse_address(true));
        m_acceptor.bind(ep);
        m_acceptor.listen();

        do_accept();
    }
    void close()
    {
        if (m_acceptor.is_open())
        {
            m_acceptor.close();
        }
    }
private :
    void do_accept()
    {
        auto session = SessionLayer::make(m_ioc);
        m_session_depot.push_back(session);
        m_acceptor.async_accept(
            session->socket(),
            [session, this](const boost::system::error_code& ec) { on_accepted(ec, session); }
        );
    }
    void on_accepted(const boost::system::error_code& ec, session_ptr session)
    {
        if (ec)
        {
            on_finish(ec, "accept");
            return;
        }

        do_accept();

        session->execute();
    }
    void on_finish(const boost::system::error_code& ec, const char* phase)
    {
        if (!ec)
        {
            // done
            return;
        }
        else if (ec && ec.value() == asio::error::operation_aborted)
        {
            // abort
            return;
        }

        // error
    }
private :
    asio::io_context m_ioc;
    asio::executor_work_guard<asio::io_context::executor_type> m_work_guard;
    std::thread m_server_thread;

    unsigned short m_port;
    tcp::acceptor m_acceptor;

    std::list<session_ptr> m_session_depot;
};

} // namespace network
