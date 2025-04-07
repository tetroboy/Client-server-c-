#include <boost/asio.hpp>
#include <iostream>

using boost::asio::ip::tcp;

int main() {
    try {
        boost::asio::io_context io_context;
        tcp::socket socket(io_context);

        // Устанавливаем соединение один раз
        socket.connect(tcp::endpoint(boost::asio::ip::make_address("127.0.0.1"), 1234));
        std::cout << "Connected to server. Type commands (1-CPU, 2-RAM, 3-Disk, 0-Exit)\n";

        while (true) {
            std::cout << "> ";

            int command;
            if (!(std::cin >> command)) {
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                std::cout << "Invalid input. Use 1, 2, 3 or 0.\n";
                continue;
            }

            if (command == 0) break;

            std::string message;
            switch (command) {
            case 1: message = "CPU usage\n"; break;
            case 2: message = "RAM usage\n"; break;
            case 3: message = "Disk usage\n"; break;
            case 4: message = "Processes list\n"; break;
            case 5: {
                
                DWORD PID;
                if (!(std::cin >> PID)) {
                    std::cin.clear();
                    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                    std::cout << "Invalid input. Write PID\n";
                    continue;
                }
                message = "Kill process " + std::to_string(PID) + "\n";
                break;
            }
            default:
                std::cout << "Unknown command\n";
                continue;
            }

            try {
                // Отправка запроса
                boost::asio::write(socket, boost::asio::buffer(message));

                // Чтение ответа
                boost::asio::streambuf response;
                boost::asio::read_until(socket, response, '\n');

                std::istream stream(&response);
                std::string reply;
                std::getline(stream, reply);

                std::cout << "Server response:\n" << reply << "\n";
            }
            catch (const boost::system::system_error& e) {
                if (e.code() == boost::asio::error::eof) {
                    std::cerr << "Server closed connection. Reconnecting...\n";
                    // Переподключение
                    socket.close();
                    socket.connect(tcp::endpoint(boost::asio::ip::make_address("127.0.0.1"), 1234));
                }
                else {
                    throw; // Перебрасываем другие ошибки
                }
            }
        }

        // Корректное закрытие соединения
        if (socket.is_open()) {
            socket.shutdown(tcp::socket::shutdown_both);
            socket.close();
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}