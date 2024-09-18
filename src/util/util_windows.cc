#include "src/util/util_windows.h"

#include <mntent.h>
#include <sys/stat.h>
#include <unistd.h>

#include <string>

namespace oceandoc {
namespace util {

bool UtilWindows::SetFileInvisible(std::string_view path) {
  DWORD attributes = GetFileAttributesA(path.data());
  if (attributes == INVALID_FILE_ATTRIBUTES) {
    return false;
  }
  return SetFileAttributesA(path.data(), attributes | FILE_ATTRIBUTE_HIDDEN);
}

std::string UtilWindows::Partition(std::string_view path) {
  HRESULT hres;
  IWbemLocator* pLoc = nullptr;
  IWbemServices* pSvc = nullptr;
  IEnumWbemClassObject* pEnumerator = nullptr;
  IWbemClassObject* pclsObj = nullptr;
  ULONG uReturn = 0;
  std::string result;

  hres = CoInitializeEx(0, COINIT_MULTITHREADED);
  if (FAILED(hres)) return "Error";

  hres =
      CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT,
                           RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
  if (FAILED(hres)) {
    CoUninitialize();
    return "Error";
  }

  hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER,
                          IID_IWbemLocator, (LPVOID*)&pLoc);
  if (FAILED(hres)) {
    CoUninitialize();
    return "Error";
  }

  hres = pLoc->ConnectServer(_bstr_t(L"ROOT\\CIMV2"), NULL, NULL, 0, NULL, 0, 0,
                             &pSvc);
  if (FAILED(hres)) {
    pLoc->Release();
    CoUninitialize();
    return "Error";
  }

  hres = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
                           RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE,
                           NULL, EOAC_NONE);
  if (FAILED(hres)) {
    pSvc->Release();
    pLoc->Release();
    CoUninitialize();
    return "Error";
  }

  hres = pSvc->ExecQuery(bstr_t("WQL"),
                         bstr_t("SELECT DeviceID FROM Win32_LogicalDisk"),
                         WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                         NULL, &pEnumerator);
  if (FAILED(hres)) {
    pSvc->Release();
    pLoc->Release();
    CoUninitialize();
    return "Error";
  }

  while (pEnumerator) {
    HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
    if (0 == uReturn) break;

    VARIANT vtProp;
    hr = pclsObj->Get(L"DeviceID", 0, &vtProp, 0, 0);
    result = _bstr_t(vtProp.bstrVal);
    VariantClear(&vtProp);

    pclsObj->Release();
  }

  pSvc->Release();
  pLoc->Release();
  pEnumerator->Release();
  CoUninitialize();

  return result;
}

std::string UtilWindows::PartitionUUID(std::string_view path) {
  // std::string partition = Partition(path);
  // struct udev* udev = udev_new();
  // if (!udev) return "Error";

  // struct udev_device* dev = udev_device_new_from_syspath(udev,
  // device.c_str()); if (!dev) { udev_unref(udev); return "Error";
  //}

  // const char* uuid = udev_device_get_property_value(dev,
  // "ID_PART_ENTRY_UUID"); std::string result = uuid ? uuid : "Error";

  // udev_device_unref(dev);
  // udev_unref(udev);

  // return result;
  return "";
}

}  // namespace util
}  // namespace oceandoc
