#include "Cpu_info.h"
#include "Memory_info.h"
using boost::asio::ip::tcp;

// Используем стандартное определение из winternl.h

json handle_command(std::string s, DWORD processID = 0);

// Функция для получения информации о памяти


json GetProcessListJson() {
    json processList = json::array(); // Створюємо JSON масив

    HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE) {
        throw std::runtime_error("Error creating process snapshot (Error: " + std::to_string(GetLastError()) + ")");
    }

    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (!Process32First(hProcessSnap, &pe32)) {
        CloseHandle(hProcessSnap);
        throw std::runtime_error("Error getting process info (Error: " + std::to_string(GetLastError()) + ")");
    }

    do {
        // Конвертуємо ім'я процесу
        std::string processName;
#ifdef UNICODE
        int size = WideCharToMultiByte(CP_UTF8, 0, pe32.szExeFile, -1, NULL, 0, NULL, NULL);
        if (size > 0) {
            char* buffer = new char[size];
            WideCharToMultiByte(CP_UTF8, 0, pe32.szExeFile, -1, buffer, size, NULL, NULL);
            processName = buffer;
            delete[] buffer;
        }
#else
        processName = pe32.szExeFile;
#endif

        // Отримуємо інформацію про пам'ять
        DWORD memoryUsageMB = 0;
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe32.th32ProcessID);
        if (hProcess != NULL) {
            PROCESS_MEMORY_COUNTERS pmc;
            if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {
                memoryUsageMB = pmc.WorkingSetSize / (1024 * 1024);
            }
            CloseHandle(hProcess);
        }

        // Додаємо інформацію про процес до JSON
        json processInfo = {
            {"pid", pe32.th32ProcessID},
            {"name", processName},
            {"parent_pid", pe32.th32ParentProcessID},
            {"threads", pe32.cntThreads},
            {"priority", pe32.pcPriClassBase},
            {"memory_mb", memoryUsageMB}
        };

        processList.push_back(processInfo);

    } while (Process32Next(hProcessSnap, &pe32));

    CloseHandle(hProcessSnap);
    return { {"processes", processList} }; // Повертаємо об'єкт з масивом процесів
}


bool KillProcessByID(DWORD processID) {
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, processID);
    if (hProcess == NULL) {
        std::cerr << "Unable to open process (Error: " << GetLastError() << ")" << std::endl;
        return false;
    }

    bool result = TerminateProcess(hProcess, 1);
    CloseHandle(hProcess);

    if (!result) {
        std::cerr << "Unable to kill process (Error: " << GetLastError() << ")" << std::endl;
    }

    return result;
}

void handle_client(tcp::socket socket) {
    try {
        while (true) {
            // Буфер для читання даних
            boost::asio::streambuf buffer;

            // Читаємо дані до символу '\n'
            boost::asio::read_until(socket, buffer, '\n');

            // Конвертуємо дані в std::string
            std::istream stream(&buffer);
            std::string message;
            std::getline(stream, message); // Читаємо рядок

            std::cout << "Received message: " << message << std::endl;

            if (!InitCpuMonitor()) {
                std::cerr << "Failed to initialize CPU monitor!" << std::endl;

            }


            //handle_command("CPU usage");

            json response = handle_command(message);
            //json response = GetProcessListJson();

            // Преобразуем JSON в строку и добавляем \n
            std::string response_str = response.dump() + "\n";

            KillProcessByID(18648);

            boost::asio::write(socket, boost::asio::buffer(response_str));
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

json handle_command(std::string s, DWORD processID = 0) {
    if (s == "CPU usage") {
        return GetCpuUsageJson();
    }
    if (s == "RAM usage") {
        return  GetMemoryInfo();
    }
    if (s == "Disk usage") {
        return get_disk_info_json();
    }
    if (s == "PID list") {
        return GetProcessListJson();
    }
    if (s.find("kill process") == 0) {
        // Извлекаем PID из сообщения (формат: "kill process 1234")
        size_t pos = s.find_last_of(' ');
        if (pos != std::string::npos) {
            try {
                DWORD pid = std::stoul(s.substr(pos + 1));
                if (KillProcessByID(pid)) {
                    boost::asio::write(socket, boost::asio::buffer("Process " + std::to_string(pid) + " killed\n"));
                }
                else {
                    boost::asio::write(socket, boost::asio::buffer("Failed to kill process " + std::to_string(pid) + "\n"));
                }
            }
            catch (...) {
                boost::asio::write(socket, boost::asio::buffer("Invalid PID format\n"));
            }
        }
        else {
            boost::asio::write(socket, boost::asio::buffer("Usage: kill process <PID>\n"));
        }
    }
    else return { "Unknown command" };
}

int main() {
    try {
        boost::asio::io_context io_context;
        tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 1234));

        std::cout << "Server started" << std::endl;

        while (true) {
            tcp::socket socket(io_context);
            acceptor.accept(socket);
            std::thread(handle_client, std::move(socket)).detach();
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Помилка сервера: " << e.what() << std::endl;
    }

    return 0;
}