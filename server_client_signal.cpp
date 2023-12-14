#include <boost/asio/io_context.hpp>
#include <boost/asio/spawn.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/asio/sd_event.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/exception.hpp>
#include <sdbusplus/server.hpp>
#include <sdbusplus/timer.hpp>
#include <nlohmann/json.hpp>
#include <chrono>
#include <ctime>
#include <iostream>
#include <variant>

using namespace std;
 
shared_ptr <sdbusplus::asio::connection> bus;
 
int addInt(int d1, int d2)
{
    return d1 + d2;
}
 
string addString(string s1, string s2)
{
    string res = s1 + s2;
    sdbusplus::message_t m = bus->new_signal("/calculate_obj","calculate_infterface.data", "MethodCallAddString");
    m.append(s1, s2); 
    m.append(res);
    m.signal_send();
 
    return res;
}

int server()
{
    boost::asio::io_context io;
    auto conn = std::make_shared<sdbusplus::asio::connection>(io);
    bus = conn;
    conn->request_name("calculate.service");
    auto server = sdbusplus::asio::object_server(conn);
    shared_ptr<sdbusplus::asio::dbus_interface> iface = server.add_interface("/calculate_obj","calculate_infterface.data");
    iface->register_method("AddInt", addInt);
    iface->register_method("AddString", addString);
    iface->register_property("data", 18, sdbusplus::asio::PropertyPermission::readWrite);
    iface->register_property("name", "calculate"s, sdbusplus::asio::PropertyPermission::readWrite);
    iface->register_signal<string, string, string>("MethodCallAddString");
    iface->initialize();
    io.run();
    return 0;

}
namespace rules = sdbusplus::bus::match::rules;
auto recvCalculatMethodSignal(sdbusplus::asio::connection &bus)
{
    auto propertyMatch = std::make_shared<sdbusplus::bus::match_t>(
        bus,
        rules::type::signal().append(rules::member("MethodCallAddString")).append(rules::sender("calculate.service")),
        [](sdbusplus::message::message& msg) {
            string s1, s2, s3;
            msg.read(s1, s2, s3);
            cout<<s1<<"+"<<s2<<"="<<s3<<endl;
        });
 
    return propertyMatch;
}
 
int client()
{
    boost::asio::io_context io;
    auto bus = std::make_shared<sdbusplus::asio::connection>(io);
    vector<std::shared_ptr<sdbusplus::bus::match_t>> rules;
    rules.push_back(recvCalculatMethodSignal(*bus));
    io.run();
    return 0;
}

int main(int argc,const char* argv[])
{
    if(std::string(argv[1]) == "--server")
        server();
    else if (std::string(argv[1]) == "--client")
        client();
    else    
        std::cout<<"error input\n";

}

// busctl call calculate.service /calculate_obj calculate_infterface.data AddString ss 123 1000
// client: 123+1000=1231000
//
