#include <windows.h>
#include <iostream>
#include <string>
#include <sstream>

using namespace std;

const wstring nullStr(L"");
const long int CXL_VERSION = 68;
const string build("2017-9-21");

// Convert to wstring type
template <typename T>
wstring to_wstring(T const & value)
{
   wstringstream ss;
   ss << value;
   return ss.str();
}

// Set a registry key value
bool SetKey(HKEY root, const wstring &subkey, const wstring &name, const wstring &value)
{
    HKEY hKey;
    if(RegCreateKeyExW(root, subkey.c_str(), 0, 0, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, 0, &hKey, NULL) == 0)
    {
        const wchar_t *p = value.c_str();
        size_t sz = wcslen(p)*2+2;

        if(RegSetValueExW(hKey, name.c_str(), 0, REG_SZ, (LPBYTE)(p), sz) == 0)
        {
            RegCloseKey(hKey);
            return true;
        }
        RegCloseKey(hKey);
    }
    return false;
}

// Get a registry value
wstring GetKey(HKEY root, const wstring &subkey, const wstring &name)
{
    HKEY hKey;
    if(RegOpenKeyExW(root, subkey.c_str(), REG_OPTION_NON_VOLATILE, KEY_QUERY_VALUE, &hKey) == 0)
    {
        wchar_t buffer[MAX_PATH*2];
        DWORD bufferSize = 0;
        DWORD dataType = 0;
        memset(buffer, 0, sizeof(buffer));
        bufferSize = sizeof(buffer);

        if(RegQueryValueExW(hKey, name.c_str(), 0, &dataType, (LPBYTE)(buffer), &bufferSize) == 0)
        {
            if(dataType == REG_SZ)
            {
                RegCloseKey(hKey);
                buffer[MAX_PATH - 1] = 0;
                return buffer;
            }
        }
        RegCloseKey(hKey);
    }
    return nullStr;
}

// Main program entry point
int main(int argc, char* argv[])
{
    printf( "Cerberus Launcher %s\n", build.c_str() );

    // Get current launcher location
    wstring app = to_wstring(argv[0]);

    // Put all data into the Current user registry area
    HKEY root = HKEY_CURRENT_USER;

    // File types supported and storage location
    wstring type_cerberusPath (L"Software\\Classes\\.cxs");
    wstring type_monkeyPath (L"Software\\Classes\\.monkey");
    wstring tool_path (L"Software\\Classes\\cerberus-x.com");

    /* Check that the version hasn't changed. Instead of checking for a greater version number,
       check to see if the version numbers differ. Doing it this way makes it so that the end
       user can downgrade as well as upgrade. Also check to see if the Cerberus directory has been moved
    */
    if((CXL_VERSION != _wtoi(GetKey(root, tool_path+wstring (L"\\Version"), nullStr).c_str())) || (app != GetKey(root, tool_path+wstring (L"\\Location"), nullStr).c_str()))
    {
        wstring shell(L"\"");
        shell.append(app);
        shell.append(L"\" \"%1\"");

        SetKey(root, tool_path, nullStr, wstring(L"Simple Cerberus IDE"));
        SetKey(root, tool_path+wstring (L"\\Version"), nullStr, to_wstring(CXL_VERSION));
        SetKey(root, tool_path+wstring (L"\\DefaultIcon"), nullStr, app);
        SetKey(root, tool_path+wstring (L"\\Shell\\Open\\Command"), nullStr, shell);
        SetKey(root, tool_path+wstring (L"\\Location"), nullStr, to_wstring(app));  // Used if the Cerberus directory is relocated

         // Saves having to rebuild this application launcher if a new IDE is added. The new IDe checks this and updates it for its own use.
        SetKey(root, tool_path+wstring (L"\\CurrentIDE"), nullStr, wstring(L"bin\\Ted.exe"));
    }

    // Set application file associations
    if((GetKey(root, type_cerberusPath, nullStr) == nullStr)||(GetKey(root, type_monkeyPath, nullStr) == nullStr))
    {
        SetKey(root, type_cerberusPath, nullStr, wstring (L"cerberus-x.com"));
        SetKey(root, type_monkeyPath, nullStr, wstring (L"cerberus-x.com"));
    }

    // Construct the command line
    wstring args(GetKey(root, tool_path+wstring (L"\\CurrentIDE"), nullStr));
    for(int i = 1; i < argc; i++)
    {
        args.append(L" \"");
        args.append(to_wstring(argv[i]));
        args.append(L"\"");
    }

    // Use create process to start the IDE with command line arguments.
    STARTUPINFOW         siStartupInfo;
    PROCESS_INFORMATION piProcessInfo;

    memset(&siStartupInfo, 0, sizeof(siStartupInfo));
    memset(&piProcessInfo, 0, sizeof(piProcessInfo));

    siStartupInfo.cb = sizeof(siStartupInfo);

    if(CreateProcessW(NULL,
                    (LPWSTR)args.c_str(),
                    NULL,
                    NULL,
                    FALSE,
                    CREATE_DEFAULT_ERROR_MODE,
                    NULL,
                    NULL,
                    &siStartupInfo,
                    &piProcessInfo) == FALSE)
                    {
                        // Issue some sort of error message
                        printf("Cerberus Launcher %s\n", build.c_str());
                        wprintf(L"Failed on command:\n%ls\n", args.c_str());
                        printf("Press Return to exit\n");
                        std::cin.ignore();
                    }

    CloseHandle(piProcessInfo.hThread);
    CloseHandle(piProcessInfo.hProcess);
    return 0;
}
