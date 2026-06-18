#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <sys/utsname.h>
#include <sys/sysinfo.h>
#include <iomanip>
#include <vector>
#include <utility>
#include <algorithm>

using namespace std;

#define RESET        "\033[0m"
#define BRIGHT_GREEN "\033[92m"
#define WHITE        "\033[97m"

// Палитра для вывода цветных блоков (как в настоящем neofetch)
const vector<string> COLORS = {
    "\033[40m", "\033[41m", "\033[42m", "\033[43m", 
    "\033[44m", "\033[45m", "\033[46m", "\033[47m"
};

// Логотип без пробелов в конце строк (чтобы избежать бага с заливкой фона)
vector<string> getLogoLines() {
    return {
        "          .--.",
        "         |o_o |",
        "         |:_/ |",
        "        //   \\ \\",
        "       (|     | )",
        "      /'\\_   _/`\\",
        "      \\___)=(___/",
        "            )   (",
        "           /     \\",
        "          /       \\",
        "         `-.....--`"
    };
}

string getRealOS() {
    ifstream file("/etc/os-release");
    string line, prettyName;
    while (getline(file, line)) {
        if (line.compare(0, 11, "PRETTY_NAME") == 0) {
            size_t start = line.find('"') + 1;
            size_t end = line.find('"', start);
            if (start != string::npos && end != string::npos) {
                prettyName = line.substr(start, end - start);
            }
        }
    }
    file.close();
    
    if (!prettyName.empty() && prettyName != "Android") return prettyName;
    
    FILE* fp = popen("getprop ro.build.version.release 2>/dev/null", "r");
    if (fp) {
        char buffer[128];
        string version;
        if (fgets(buffer, sizeof(buffer), fp) != NULL) {
            version = buffer;
            version.erase(version.find_last_not_of(" \n\r\t") + 1);
        }
        pclose(fp);
        if (!version.empty()) return "Android " + version;
    }
    return "Android (Termux)";
}

string getDeviceModel() {
    FILE* fp = popen("getprop ro.product.model 2>/dev/null", "r");
    if (!fp) return "Unknown Device";
    char buffer[256];
    string model;
    if (fgets(buffer, sizeof(buffer), fp) != NULL) {
        model = buffer;
        model.erase(model.find_last_not_of(" \n\r\t") + 1);
    }
    pclose(fp);
    return model.empty() ? "Unknown Device" : model;
}

string getDeviceManufacturer() {
    FILE* fp = popen("getprop ro.product.manufacturer 2>/dev/null", "r");
    if (!fp) return "Unknown";
    char buffer[128];
    string manufacturer;
    if (fgets(buffer, sizeof(buffer), fp) != NULL) {
        manufacturer = buffer;
        manufacturer.erase(manufacturer.find_last_not_of(" \n\r\t") + 1);
    }
    pclose(fp);
    return manufacturer.empty() ? "Unknown" : manufacturer;
}

string getHostname() {
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        return string(hostname);
    }
    return "localhost";
}

string getUser() {
    const char* user = getenv("USER");
    return user ? string(user) : "unknown";
}

string getShell() {
    const char* shellEnv = getenv("SHELL");
    if (!shellEnv) return "unknown";
    string shellPath(shellEnv);
    size_t lastSlash = shellPath.find_last_of('/');
    return (lastSlash != string::npos) ? shellPath.substr(lastSlash + 1) : shellPath;
}

string getKernel() {
    struct utsname unameData;
    if (uname(&unameData) == 0) {
        return string(unameData.release);
    }
    return "unknown";
}

string getUptime() {
    struct sysinfo info;
    if (sysinfo(&info) != 0) return "unknown";
    
    int days = info.uptime / 86400;
    int hours = (info.uptime % 86400) / 3600;
    int mins = (info.uptime % 3600) / 60;
    
    stringstream ss;
    if (days > 0) ss << days << "d ";
    if (hours > 0) ss << hours << "h ";
    ss << mins << "m";
    return ss.str();
}

string getCPU() {
    ifstream file("/proc/cpuinfo");
    string line, model;
    int cores = 0;
    
    while (getline(file, line)) {
        if (line.compare(0, 10, "model name") == 0 || line.compare(0, 8, "Hardware") == 0) {
            size_t start = line.find(':');
            if (start != string::npos) {
                string foundModel = line.substr(start + 1);
                // Убираем начальные пробелы
                foundModel.erase(0, foundModel.find_first_not_of(" \t"));
                if (model.empty() || line.compare(0, 10, "model name") == 0) {
                    model = foundModel;
                }
            }
        }
        if (line.compare(0, 9, "processor") == 0) cores++;
    }
    file.close();
    
    if (model.empty()) model = "Unknown CPU";
    if (model.length() > 45) model = model.substr(0, 42) + "...";
    
    stringstream ss;
    ss << cores << "x " << model;
    return ss.str();
}

string getRAM() {
    ifstream file("/proc/meminfo");
    string line;
    long total = 0, available = 0;
    while (getline(file, line)) {
        if (line.compare(0, 9, "MemTotal:") == 0) {
            sscanf(line.c_str(), "MemTotal: %ld kB", &total);
        } else if (line.compare(0, 13, "MemAvailable:") == 0) {
            sscanf(line.c_str(), "MemAvailable: %ld kB", &available);
        }
    }
    file.close();
    
    if (total == 0) return "Unknown";
    
    long used = total - available;
    double usedGB = (double)used / 1024.0 / 1024.0;
    double totalGB = (double)total / 1024.0 / 1024.0;
    
    stringstream ss;
    ss << fixed << setprecision(1) << usedGB << "GB / " << totalGB << "GB";
    return ss.str();
}

string getPackages() {
    FILE* fp = popen("pkg list-installed 2>/dev/null | wc -l", "r");
    if (!fp) return "unknown";
    
    char buffer[64];
    string result;
    if (fgets(buffer, sizeof(buffer), fp) != NULL) {
        result = buffer;
        result.erase(result.find_last_not_of(" \n\r\t") + 1);
    }
    pclose(fp);
    
    if (result == "0" || result.empty()) {
        fp = popen("dpkg -l 2>/dev/null | wc -l", "r");
        if (fp) {
            if (fgets(buffer, sizeof(buffer), fp) != NULL) {
                result = buffer;
                result.erase(result.find_last_not_of(" \n\r\t") + 1);
            }
            pclose(fp);
        }
    }
    return result.empty() ? "unknown" : result;
}

int main() {
    cout << "\n";
    
    string manufacturer = getDeviceManufacturer();
    string model = getDeviceModel();
    string device = (manufacturer == "Unknown" || model == "Unknown Device") ? "Unknown Device" : manufacturer + " " + model;
    
    vector<pair<string, string>> info = {
        {"User", getUser()},
        {"Host", getHostname()},
        {"OS", getRealOS()},
        {"Device", device},
        {"Kernel", getKernel()},
        {"Uptime", getUptime()},
        {"Packages", getPackages()},
        {"Shell", getShell()},
        {"CPU", getCPU()},
        {"RAM", getRAM()}
    };
    
    size_t maxKeyLen = 0;
    for (const auto &i : info) {
        maxKeyLen = max(maxKeyLen, i.first.length());
    }
    
    vector<string> logo = getLogoLines();
    
    size_t logoWidth = 0;
    for (const auto& l : logo) {
        logoWidth = max(logoWidth, l.length());
    }
    logoWidth += 3; // Отступ между логотипом и текстом
    
    size_t maxLines = max(logo.size(), info.size() + 2);
    
    for (size_t i = 0; i < maxLines; ++i) {
        size_t currentLogoLen = 0;
        
        // 1. Отрисовка графики
        if (i < logo.size()) {
            cout << BRIGHT_GREEN << logo[i] << RESET;
            currentLogoLen = logo[i].length();
        }
        
        // Добиваем пробелами до границы инфоблока (строго после RESET!)
        cout << string(logoWidth - currentLogoLen, ' ');
        
        // 2. Отрисовка системной информации
        if (i < info.size()) {
            cout << BRIGHT_GREEN << left << setw(maxKeyLen) << info[i].first << RESET 
                 << WHITE << " : " << RESET 
                 << info[i].second;
        } 
        // 3. Отрисовка цветных блоков
        else if (i == info.size() + 1) {
            for (const auto& color : COLORS) {
                cout << color << "   " << RESET; // Цветной фон + 3 пробела
            }
        }
        cout << "\n";
    }
    
    cout << "\n";
    return 0;
}

