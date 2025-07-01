#include "Process_info.h"

json ProcessManager::get_process_list() {
    json process_list = json::array();

    HANDLE h_process_snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (h_process_snap == INVALID_HANDLE_VALUE) {
        throw std::runtime_error("Error creating process snapshot (Error: " +
            std::to_string(GetLastError()) + ")");
    }

    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (!Process32First(h_process_snap, &pe32)) {
        CloseHandle(h_process_snap);
        throw std::runtime_error("Error getting process info (Error: " +
            std::to_string(GetLastError()) + ")");
    }

    do {
        std::string process_name;
#ifdef UNICODE
        int size = WideCharToMultiByte(CP_UTF8, 0, pe32.szExeFile, -1, nullptr, 0, nullptr, nullptr);
        if (size > 0) {
            char* buffer = new char[size];
            WideCharToMultiByte(CP_UTF8, 0, pe32.szExeFile, -1, buffer, size, nullptr, nullptr);
            process_name = buffer;
            delete[] buffer;
        }
#else
        process_name = pe32.szExeFile;
#endif

        DWORD memory_usage_mb = 0;
        HANDLE h_process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe32.th32ProcessID);
        if (h_process != nullptr) {
            PROCESS_MEMORY_COUNTERS pmc;
            if (GetProcessMemoryInfo(h_process, &pmc, sizeof(pmc))) {
                memory_usage_mb = pmc.WorkingSetSize / (1024 * 1024);
            }
            CloseHandle(h_process);
        }

        json process_info = {
            {"pid", pe32.th32ProcessID},
            {"name", process_name},
            {"parent_pid", pe32.th32ParentProcessID},
            {"threads", pe32.cntThreads},
            {"priority", pe32.pcPriClassBase},
            {"memory_mb", memory_usage_mb}
        };

        process_list.push_back(process_info);
    } while (Process32Next(h_process_snap, &pe32));

    CloseHandle(h_process_snap);
    return { {"processes", process_list} };
}

json ProcessManager::start_process(const std::string& program_path) {
    json result;
    result["status"] = "success";
    result["message"] = "Process start attempt";

    HINSTANCE h_instance = ShellExecuteA(
        nullptr, "open", program_path.c_str(), nullptr, nullptr, SW_SHOW);

    if ((INT_PTR)h_instance > 32) {
        result["message"] = "Process start attempt - File opened successfully";
    }
    else {
        result["message"] = "Process start attempt - Failed to open file (Error: " +
            std::to_string((int)(INT_PTR)h_instance) + ")";
    }

    return result;
}

bool ProcessManager::kill_process_by_id(DWORD process_id) {
    HANDLE h_process = OpenProcess(PROCESS_TERMINATE, FALSE, process_id);
    if (h_process == nullptr) {
        std::cerr << "Unable to open process (Error: " << GetLastError() << ")" << std::endl;
        return false;
    }

    bool result = TerminateProcess(h_process, 1);
    CloseHandle(h_process);

    if (!result) {
        std::cerr << "Unable to kill process (Error: " << GetLastError() << ")" << std::endl;
    }

    return result;
}