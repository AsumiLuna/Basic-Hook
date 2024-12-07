using System;
using System.Threading;
using System.Runtime.InteropServices;
using Microsoft.Win32;

class Program
{
    const string ThemeRegistryPath = @"Software\Microsoft\Windows\CurrentVersion\Themes";
    const string ThemeRegistryValue = "CurrentTheme";

    const string BasicThemerRegistryPath = @"HKEY_LOCAL_MACHINE\SOFTWARE\Windhawk\Engine\Mods\basic-themer";
    const string DwmUnextendRegistryPath = @"HKEY_LOCAL_MACHINE\SOFTWARE\Windhawk\Engine\Mods\dwm-unextend-frames";
    const string DisabledValueName = "Disabled";

    // Windows API для мониторинга реестра
    [DllImport("advapi32.dll", SetLastError = true)]
    static extern int RegOpenKeyEx(IntPtr hKey, string subKey, uint options, int samDesired, out IntPtr phkResult);

    [DllImport("advapi32.dll", SetLastError = true)]
    static extern int RegNotifyChangeKeyValue(IntPtr hKey, bool bWatchSubtree, uint dwNotifyFilter, IntPtr hEvent, bool fAsynchronous);

    [DllImport("advapi32.dll", SetLastError = true)]
    static extern int RegCloseKey(IntPtr hKey);

    private static readonly IntPtr HKEY_CURRENT_USER = new IntPtr(unchecked((int)0x80000001));
    private const int KEY_READ = 0x20019;
    private const uint REG_NOTIFY_CHANGE_LAST_SET = 0x00000004;

    static bool isBasicThemeActive;

    static void Main()
    {
        UpdateThemeStates();
        ApplyRegistryChanges();
        MonitorThemeChanges();
        Thread.Sleep(Timeout.Infinite); // Держим приложение активным в фоне
    }

    static void UpdateThemeStates()
    {
        try
        {
            string currentTheme = (string)Registry.GetValue($@"HKEY_CURRENT_USER\{ThemeRegistryPath}", ThemeRegistryValue, string.Empty);
            isBasicThemeActive = currentTheme?.Contains("basic.theme") ?? false;
        }
        catch
        {
            isBasicThemeActive = false;
        }
    }

    static void ApplyRegistryChanges()
    {
        SetRegistryValue(BasicThemerRegistryPath, isBasicThemeActive ? 0 : 1);
        SetRegistryValue(DwmUnextendRegistryPath, isBasicThemeActive ? 0 : 1);
    }

    static void SetRegistryValue(string registryPath, int value)
    {
        try
        {
            Registry.SetValue(registryPath, DisabledValueName, value, RegistryValueKind.DWord);
        }
        catch
        {
            // Логи удалены
        }
    }

    static void MonitorThemeChanges()
    {
        if (RegOpenKeyEx(HKEY_CURRENT_USER, ThemeRegistryPath, 0, KEY_READ, out IntPtr hKey) != 0)
        {
            return;
        }

        ThreadPool.QueueUserWorkItem(_ =>
        {
            try
            {
                while (true)
                {
                    if (RegNotifyChangeKeyValue(hKey, false, REG_NOTIFY_CHANGE_LAST_SET, IntPtr.Zero, false) == 0)
                    {
                        bool previousBasic = isBasicThemeActive;

                        UpdateThemeStates();

                        if (previousBasic != isBasicThemeActive)
                        {
                            SetRegistryValue(BasicThemerRegistryPath, isBasicThemeActive ? 0 : 1);
                            SetRegistryValue(DwmUnextendRegistryPath, isBasicThemeActive ? 0 : 1);
                        }
                    }
                    else
                    {
                        break;
                    }
                }
            }
            finally
            {
                RegCloseKey(hKey);
            }
        });
    }
}
