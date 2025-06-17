#include "Process_info.h"

json GetProcessListJson() {
    json processList = json::array();

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


        DWORD memoryUsageMB = 0;
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe32.th32ProcessID);
        if (hProcess != NULL) {
            PROCESS_MEMORY_COUNTERS pmc;
            if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {
                memoryUsageMB = pmc.WorkingSetSize / (1024 * 1024);
            }
            CloseHandle(hProcess);
        }


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
    return { {"processes", processList} };
}

json start_process(const std::string& program_path) {
    json result;
    result["status"] = "success";
    result["message"] = "Process start attempt";

    HINSTANCE hInstance = ShellExecuteA(
        NULL,
        "open",
        program_path.c_str(),
        NULL,
        NULL,
        SW_SHOW
    );

    if ((INT_PTR)hInstance > 32) {
        std::string full_message = "Process start attempt - File opened successfully";
        result["message"] = full_message;
    }
    else {
        std::string error_message = "Process start attempt - Failed to open file (Error: " +
            std::to_string((int)(INT_PTR)hInstance) + ")";
        result["message"] = error_message;
    }

    return result;
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