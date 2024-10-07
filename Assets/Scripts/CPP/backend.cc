#include "backend.h"
#include <windows.h>
#include <string>
#include <sstream>
#include <vector>
#include <iostream>
#include <comdef.h>
#include <Wbemidl.h>
#pragma comment(lib, "wbemuuid.lib")

std::string GetCPUName() {
    std::string cpuName;
    char cpuString[0x40];
    DWORD bufferSize = sizeof(cpuString);
    HKEY hKey;
    
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        if (RegQueryValueEx(hKey, "ProcessorNameString", nullptr, nullptr, (LPBYTE)cpuString, &bufferSize) == ERROR_SUCCESS) {
            cpuName = cpuString;
        }
        RegCloseKey(hKey);
    }
    return cpuName;
}

std::string GetRAMSize() {
    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof(statex);
    GlobalMemoryStatusEx(&statex);
    std::ostringstream oss;
    oss << statex.ullTotalPhys / (1024 * 1024) << " MB"; 
    return oss.str();
}

std::string GetGPUInfo() {
    std::string gpuInfo;
    HRESULT hr;
    IWbemLocator *pLoc = NULL;
    IWbemServices *pSvc = NULL;

    hr = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hr)) return "Failed to initialize COM";

    hr = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
    if (FAILED(hr)) {
        CoUninitialize();
        return "Failed to initialize security";
    }

    hr = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID *)&pLoc);
    if (FAILED(hr)) {
        CoUninitialize();
        return "Failed to create IWbemLocator object";
    }

    hr = pLoc->ConnectServer(_bstr_t(L"ROOT\\CIMV2"), NULL, NULL, 0, NULL, 0, 0, &pSvc);
    if (FAILED(hr)) {
        pLoc->Release();
        CoUninitialize();
        return "Could not connect to WMI";
    }

    hr = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
    if (FAILED(hr)) {
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return "Could not set proxy blanket";
    }

    IEnumWbemClassObject* pEnumerator = NULL;
    hr = pSvc->ExecQuery(bstr_t("WQL"), bstr_t("SELECT * FROM Win32_VideoController"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnumerator);
    if (FAILED(hr)) {
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return "Query for video controllers failed";
    }

    IWbemClassObject *pclsObj = NULL;
    ULONG uReturn = 0;
    while (pEnumerator) {
        HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
        if (0 == uReturn) break;

        VARIANT vtProp;
        hr = pclsObj->Get(L"Name", 0, &vtProp, 0, 0);
        gpuInfo = _bstr_t(vtProp.bstrVal);
        VariantClear(&vtProp);
        pclsObj->Release();
    }

    pSvc->Release();
    pLoc->Release();
    pEnumerator->Release();
    CoUninitialize();
    
    return gpuInfo;
}

extern "C" {
    const char* DLL_EXPORT GetPCSpecs() {
        static std::string specs;
        specs = "CPU: " + GetCPUName() + "\n" + "RAM: " + GetRAMSize() + "\n" + "GPU: " + GetGPUInfo();
        return specs.c_str();
    }
}
