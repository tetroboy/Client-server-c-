#include "NetConnection.h"
using boost::asio::ip::tcp;
json get_open_ports() {
    json result;
    result["status"] = "success";
    result["message"] = "Open ports list";

    boost::asio::io_context io_context;
    std::vector<std::pair<int, int>> port_ranges;
    const int START_PORT = 1;
    const int END_PORT = 65535;

    int current_start = -1;
    for (int port = START_PORT; port <= END_PORT; ++port) {
        boost::system::error_code ec;
        tcp::acceptor acceptor(io_context);
        acceptor.open(tcp::v4(), ec);
        if (ec) continue;

        acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
        boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address_v4::any(), port);
        acceptor.bind(endpoint, ec);
        if (!ec) {
            if (current_start == -1) {
                current_start = port;
            }
        }
        else {
            if (current_start != -1) {
                port_ranges.emplace_back(current_start, port - 1);
                current_start = -1;
            }
        }
        acceptor.close(ec);
    }

    if (current_start != -1) {
        port_ranges.emplace_back(current_start, END_PORT);
    }


    json ranges_json = json::array();
    for (const auto& range : port_ranges) {
        if (range.first == range.second) {
            ranges_json.push_back(range.first);
        }
        else {
            ranges_json.push_back(std::to_string(range.first) + "-" + std::to_string(range.second));
        }
    }
    result["port_ranges"] = ranges_json;

    return result;
}


json block_ip(const std::string& address) {
    json result;
    result["status"] = "success";
    if (blocked_ips.insert(address).second) {
        result["blocked"] = true;
        result["message"] = "IP " + address + " blocked successfully";
    }
    else {
        result["blocked"] = false;
        result["message"] = "IP " + address + " was already blocked";
    }
    return result;
}


json ping_address(const std::string& address) {
    json result;
    result["status"] = "success";
    result["message"] = "Ping result";
    result["reachable"] = json();

    boost::asio::io_context io_context;
    boost::asio::ip::tcp::resolver resolver(io_context);
    boost::system::error_code ec;

    std::cout << "Resolving address: " << address << std::endl;

    try {
        auto endpoints = resolver.resolve(address, "80", ec);
        if (!ec) {
            result["reachable"] = true;
            result["message"] += " - Host is reachable";
        }
        else {
            result["reachable"] = false;
            result["message"] += " - Host is unreachable";
            std::cout << "Resolve error: " << ec.message() << std::endl;
        }
    }
    catch (const std::exception& e) {
        result["status"] = "error";
        std::string error_msg = "Ping failed: " + std::string(e.what());
        result["message"] = error_msg;
        result["reachable"] = false;
        std::cout << "Exception: " << e.what() << std::endl;
    }

    std::cout << "Ping result: " << result.dump() << std::endl;
    return result;
}

