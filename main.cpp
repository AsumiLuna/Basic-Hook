#include <windows.h>
#include <thread>
#include <string>
#include <iostream>
#include <stdexcept>

// Constants for registry paths and values
const std::wstring ThemeRegistryPath = L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes";
const std::wstring ThemeRegistryValue = L"CurrentTheme";

const std::wstring BasicThemerRegistryPath = L"SOFTWARE\\Windhawk\\Engine\\Mods\\basic-themer";
const std::wstring DwmUnextendRegistryPath = L"SOFTWARE\\Windhawk\\Engine\\Mods\\dwm-unextend-frames";
const std::wstring DisabledValueName = L"Disabled";

bool isBasicThemeActive = false;

// Function to update the theme states by querying the registry
void UpdateThemeStates() {
    HKEY hKey;
    if (RegOpenKeyEx(HKEY_CURRENT_USER, ThemeRegistryPath.c_str(), 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        WCHAR currentTheme[MAX_PATH];
        DWORD bufferSize = sizeof(currentTheme);

        if (RegQueryValueEx(hKey, ThemeRegistryValue.c_str(), nullptr, nullptr, reinterpret_cast<LPBYTE>(currentTheme), &bufferSize) == ERROR_SUCCESS) {
            std::wstring themeValue(currentTheme);
            isBasicThemeActive = themeValue.find(L"basic.theme") != std::wstring::npos;
        } else {
            isBasicThemeActive = false;
        }

        RegCloseKey(hKey);
    } else {
        isBasicThemeActive = false;
    }
}

// Function to set a registry value
void SetRegistryValue(const std::wstring& registryPath, DWORD value) {
    HKEY hKey;
    if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, registryPath.c_str(), 0, nullptr, 0, KEY_WRITE, nullptr, &hKey, nullptr) == ERROR_SUCCESS) {
        RegSetValueEx(hKey, DisabledValueName.c_str(), 0, REG_DWORD, reinterpret_cast<const BYTE*>(&value), sizeof(value));
        RegCloseKey(hKey);
    }
}

// Function to apply registry changes based on the current theme state
void ApplyRegistryChanges() {
    SetRegistryValue(BasicThemerRegistryPath, isBasicThemeActive ? 0 : 1);
    SetRegistryValue(DwmUnextendRegistryPath, isBasicThemeActive ? 0 : 1);
}

// Function to monitor registry changes for theme updates
void MonitorThemeChanges() {
    HKEY hKey;
    if (RegOpenKeyEx(HKEY_CURRENT_USER, ThemeRegistryPath.c_str(), 0, KEY_NOTIFY, &hKey) != ERROR_SUCCESS) {
        return;
    }

    std::thread([hKey]() {
        try {
            while (true) {
                if (RegNotifyChangeKeyValue(hKey, FALSE, REG_NOTIFY_CHANGE_LAST_SET, nullptr, FALSE) == ERROR_SUCCESS) {
                    bool previousBasic = isBasicThemeActive;

                    UpdateThemeStates();

                    if (previousBasic != isBasicThemeActive) {
                        ApplyRegistryChanges();
                    }
                } else {
                    break; // Exit loop on error
                }
            }
        } catch (...) {
            // Log or handle unexpected errors here if necessary
        }

        RegCloseKey(hKey); // Ensure resource cleanup
    }).detach();
}

// Entry point of the program
int main() {
    UpdateThemeStates();
    ApplyRegistryChanges();
    MonitorThemeChanges();

    // Keep the application running indefinitely
    while (true) {
        std::this_thread::sleep_for(std::chrono::hours(24));
    }

    return 0;
}
