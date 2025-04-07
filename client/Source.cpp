#include <boost/asio.hpp>
#include <iostream>

using boost::asio::ip::tcp;

int main() {
    try {
        boost::asio::io_context io_context;
        tcp::socket socket(io_context);

        // Підключення до сервера (127.0.0.1:1234)
        socket.connect(tcp::endpoint(boost::asio::ip::make_address("127.0.0.1"), 1234));

        std::string message = "Hello noob\n";
        boost::asio::write(socket, boost::asio::buffer(message));

        // Читаємо відповідь сервера
        boost::asio::streambuf response;
        boost::asio::read_until(socket, response, '\n');

        std::istream stream(&response);
        std::string reply;
        std::getline(stream, reply);

        std::cout << "Server response: " << reply << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Помилка: " << e.what() << std::endl;
    }
    std::getchar();
    return 0;
}