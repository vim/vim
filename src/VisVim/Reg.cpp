#include "stdafx.h"

// Returns key for HKEY_CURRENT_USER\"Software"\Company\AppName
// creating it if it doesn't exist
// responsibility of the caller to call RegCloseKey() on the returned HKEY
//
HKEY GetAppKey (char* AppName)
{
	HKEY hAppKey = NULL;
	HKEY hSoftKey = NULL;
	if (RegOpenKeyEx (HKEY_CURRENT_USER, "Software", 0, KEY_WRITE | KEY_READ,
		&hSoftKey) == ERROR_SUCCESS)
	{
		DWORD Dummy;
		RegCreateKeyEx (hSoftKey, AppName, 0, REG_NONE,
			REG_OPTION_NON_VOLATILE, KEY_WRITE | KEY_READ, NULL,
			&hAppKey, &Dummy);
	}
	if (hSoftKey)
		RegCloseKey (hSoftKey);

	return hAppKey;
}

// Returns key for
// HKEY_CURRENT_USER\"Software"\RegistryKey\AppName\Section
// creating it if it doesn't exist.
// responsibility of the caller to call RegCloseKey () on the returned HKEY
//
HKEY GetSectionKey (HKEY hAppKey, LPCTSTR Section)
{
	HKEY hSectionKey = NULL;
	DWORD Dummy;
	RegCreateKeyEx (hAppKey, Section, 0, REG_NONE,
		REG_OPTION_NON_VOLATILE, KEY_WRITE|KEY_READ, NULL,
		&hSectionKey, &Dummy);
	return hSectionKey;
}

int GetRegistryInt (HKEY hSectionKey, LPCTSTR Entry, int Default)
{
	DWORD Value;
	DWORD Type;
	DWORD Count = sizeof (DWORD);
	if (RegQueryValueEx (hSectionKey, (LPTSTR) Entry, NULL, &Type,
			     (LPBYTE) &Value, &Count) == ERROR_SUCCESS)
		return Value;
	return Default;
}

bool WriteRegistryInt (HKEY hSectionKey, char* Entry, int nValue)
{
	return RegSetValueEx (hSectionKey, Entry, NULL, REG_DWORD,
		(LPBYTE) &nValue, sizeof (nValue)) == ERROR_SUCCESS;
}

