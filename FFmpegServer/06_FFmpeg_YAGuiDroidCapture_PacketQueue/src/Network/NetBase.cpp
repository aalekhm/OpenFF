#include "Network/NetBase.h"

bool NetBase::netInitialize()
{
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		printf("Failed. Error Code : %d", WSAGetLastError());
		return false;
	}

	m_bIsConnected = false;

	return true;
}

bool NetBase::netClose()
{
	return !closesocket(m_Socket);
}

std::string NetBase::getDynamicIPAddress(OUT ULONG& ulIPAddress)
{
	std::string sIPAddress = "";

	/* Variables used by GetIpAddrTable */
	PMIB_IPADDRTABLE pIPAddrTable;
	DWORD dwSize = 0;
	DWORD dwRetVal = 0;
	IN_ADDR IPAddr;

	// Before calling AddIPAddress we use GetIpAddrTable to get
	// an adapter to which we can add the IP.
	pIPAddrTable = (MIB_IPADDRTABLE*)MALLOC(sizeof(MIB_IPADDRTABLE));

	if (pIPAddrTable)
	{
		// Make an initial call to GetIpAddrTable to get the
		// necessary size into the dwSize variable
		if (GetIpAddrTable(pIPAddrTable, &dwSize, 0) == ERROR_INSUFFICIENT_BUFFER)
		{
			FREE(pIPAddrTable);
			pIPAddrTable = (MIB_IPADDRTABLE*)MALLOC(dwSize);

			if (pIPAddrTable != NULL)
			{
				// Make a second call to GetIpAddrTable to get the actual data we want
				if ((dwRetVal = GetIpAddrTable(pIPAddrTable, &dwSize, 0)) == NO_ERROR)
				{
					for (int i = 0; i < (int)pIPAddrTable->dwNumEntries; i++)
					{
						if (pIPAddrTable->table[i].wType & MIB_IPADDR_DYNAMIC)
						{
							IPAddr.S_un.S_addr = (u_long)pIPAddrTable->table[i].dwAddr;
							ulIPAddress = IPAddr.S_un.S_addr;

							ulIPAddress = ((ulIPAddress >> 24) & 0xFF) | ((ulIPAddress >> 8) & 0xFF00) | ((ulIPAddress << 8) & 0xFF0000) | ((ulIPAddress << 24) & 0xFF000000);

							sIPAddress = inet_ntoa(IPAddr);
							break;
						}
					}
				}
			}
		}

		return sIPAddress;
	}
}