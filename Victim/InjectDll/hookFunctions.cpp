#include "stdafx.h"
#include "hookFunc.h"

#pragma comment(lib, "Dbghelp.lib")

#include <io.h>
#include <conio.h>
#include <fstream>

using namespace std;

ofstream infile2;


char* filename2 = "C:\\hookfunction.txt";



//�޸ĵ����
BOOL ModifyImportTable(
	IMAGE_IMPORT_DESCRIPTOR* iid, //�����
	void* target, //Ŀ��
	void* replacement)	//����
{
	IMAGE_THUNK_DATA* itd = (IMAGE_THUNK_DATA*)
		(((char*)GetModuleHandle(NULL)) + iid->FirstThunk);
	//GetModuleHandle()��ȡһ��Ӧ�ó����̬���ӿ�dll��ģ������NULL��������Ӧ�ó�����

	infile2 << "\n\nModifyImportTable\n\n\t target\t" << target << "\n\n\t replacement\t" << &replacement << endl;


	while (itd->u1.Function)
	{

		if (((void*)itd->u1.Function) == target)
		{
			// Temporary change access to memory area to READWRITE
			MEMORY_BASIC_INFORMATION mbi;
			VirtualQuery(itd, &mbi, sizeof(MEMORY_BASIC_INFORMATION));//��ȡ�ڴ��ַ��Ϣ

			infile2 << "\nmbi:\n\tAllocationBase:\t " << mbi.AllocationBase << "  \n\tAllocationProtect\t" << mbi.AllocationProtect << "  \n\tBaseAddress\t" << mbi.BaseAddress << "  \n\tProtect\t" << mbi.Protect << "  \n\tRegionSize\t" << mbi.RegionSize << "  \n\tState\t" << mbi.State << "  \n\tType\t" << mbi.Type << endl;



			//VirtualProtect()�޸��ڴ�����Ϊ�ɶ���дPAGE_READWRITE
			VirtualProtect(mbi.BaseAddress, mbi.RegionSize, PAGE_READWRITE, &mbi.Protect);	//&mbi.Protect�����ϵ�����

			// Replace entry!!
			*((void**)itd) = replacement;

			// Restore memory permissions
			VirtualProtect(mbi.BaseAddress, mbi.RegionSize, mbi.Protect, &mbi.Protect);
			infile2.close();
			return TRUE;
		}

		itd += 1;
	}
	return FALSE;
}

//��װ����
BOOL InstallHook(LPCSTR module, LPCSTR function, void* hook, void** original)
{//LPCSTR�ַ�����������

	infile2.open(filename2);
	//infile << "\n"  << endl;

	HMODULE process = GetModuleHandle(NULL);

	infile2 << "\n��ǰģ����:" << process << endl;
	// Save original address to function
	*original = (void*)GetProcAddress(GetModuleHandleA(module), function);

	infile2 << "\n ntdll.dll��NtQuerySystemInformation��ԭʼ��ֵַ��& " << &original << "  * " << *original << "  " << original << endl;

	//MessageBoxA(NULL, "GetProcAddress of NtQuerySystemInformation 1", "Success", MB_ICONINFORMATION);

	ULONG entrySize;	//8�ֽ��޷�������

	//ImageDirectoryEntryToData()��ȡ������ַ
	IMAGE_IMPORT_DESCRIPTOR* iid =
		(IMAGE_IMPORT_DESCRIPTOR*)ImageDirectoryEntryToData(
		process,	//Ҫ��õĵ��������ģ��Ļ���ַ
		1,			//Ϊtrueʱ��ϵͳ����ģ����ӳ���ļ�����ʽӳ�䣬�����������ļ�����ʽӳ��
		IMAGE_DIRECTORY_ENTRY_IMPORT,//Ҫ��õĶε�����
		&entrySize);	//���ر���Ĵ�С

	infile2 << "\n�����" << iid << endl;



	// Search for module
	while (iid->Name)
	{
		infile2 << "\n ���ƣ�" << iid->Name << " \n\tCharacteristics:\t" << iid->Characteristics << "  \n\tFirstThunk\t" << iid->FirstThunk << "  \n\tForwarderChain\t" << iid->ForwarderChain << "  \n\tOriginalFirstThunk\t" << iid->OriginalFirstThunk << "  \n\tTimeDateStamp\t" << iid->TimeDateStamp << endl;

		const char* name = ((char*)process) + iid->Name;

		if (_stricmp(name, module) == 0)
		{
			//MessageBoxA(NULL, "ModifyImportTable 2", "Success", MB_ICONINFORMATION);

			return ModifyImportTable(iid, *original, hook);
		}
		iid += 1;
	}
	//MessageBoxA(NULL, "not find 3", "Success", MB_ICONINFORMATION);

	return FALSE;
}
