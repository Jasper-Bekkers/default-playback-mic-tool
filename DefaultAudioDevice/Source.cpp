#include <stdio.h>
#include <wchar.h>
#include <tchar.h>
#include "windows.h"
#include "Mmdeviceapi.h"
#include "PolicyConfig.h"
#include "Propidl.h"
#include "Functiondiscoverykeys_devpkey.h"

HRESULT SetDefaultAudioPlaybackDevice(LPCWSTR devID)
{
	IPolicyConfigVista *pPolicyConfig;
	ERole reserved = eConsole;

	HRESULT hr = CoCreateInstance(__uuidof(CPolicyConfigVistaClient),
		NULL, CLSCTX_ALL, __uuidof(IPolicyConfigVista), (LPVOID *)&pPolicyConfig);
	if (SUCCEEDED(hr))
	{
		hr = pPolicyConfig->SetDefaultEndpoint(devID, reserved);
		pPolicyConfig->Release();
	}
	return hr;
}

void changeDefault(IMMDeviceEnumerator *pEnum, EDataFlow dataFlow, const wchar_t* defaultName)
{
	IMMDeviceCollection *pDevices;
	// Enumerate the output devices.
	HRESULT hr = pEnum->EnumAudioEndpoints(dataFlow, DEVICE_STATE_ACTIVE, &pDevices);

	IMMDevice *pDefaultDevice = NULL;
	hr = pEnum->GetDefaultAudioEndpoint(dataFlow, eConsole, &pDefaultDevice);

	LPWSTR defaultDeviceWstrID = NULL;
	hr = pDefaultDevice->GetId(&defaultDeviceWstrID);

	UINT count;
	pDevices->GetCount(&count);

	bool changed = false;
	for (int i = 0; i < count; i++)
	{
		IMMDevice *pDevice;
		hr = pDevices->Item(i, &pDevice);

		LPWSTR wstrID = NULL;
		hr = pDevice->GetId(&wstrID);

		IPropertyStore *pStore;
		hr = pDevice->OpenPropertyStore(STGM_READ, &pStore);

		PROPVARIANT friendlyName;
		PropVariantInit(&friendlyName);
		hr = pStore->GetValue(PKEY_Device_FriendlyName, &friendlyName);

		printf("%ws Device %d: %ws\n", dataFlow == eRender ? L"Playback" : L"Microphone", i, friendlyName.pwszVal);

		if (!!wcscmp(wstrID, defaultDeviceWstrID))
		{
			if (!wcscmp(friendlyName.pwszVal, defaultName))
			{
				if (!changed)
				{
					SetDefaultAudioPlaybackDevice(wstrID);
					changed = true;
				}
			}
		}

		PropVariantClear(&friendlyName);
		pStore->Release();
		pDevice->Release();
	}

	pDevices->Release();
	pDefaultDevice->Release();
}

// EndPointController.exe [NewDefaultDeviceID]
int _tmain(int argc, _TCHAR* argv[])
{
	auto defaultPlaybackDeviceName = L"Speakers / Headphones (Realtek Audio)";
	auto defaultMicrophoneDeviceName = L"Microphone (Samson C01U Pro Mic)";

	HRESULT hr = CoInitialize(NULL);
	if (SUCCEEDED(hr))
	{
		IMMDeviceEnumerator *pEnum = NULL;
		// Create a multimedia device enumerator.
		hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL,
			CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnum);
		if (SUCCEEDED(hr))
		{
			changeDefault(pEnum, eRender, defaultPlaybackDeviceName);
			changeDefault(pEnum, eCapture, defaultMicrophoneDeviceName);

			pEnum->Release();
		}
	}
	return hr;
}