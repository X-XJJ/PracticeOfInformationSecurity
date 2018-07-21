#include "stdafx.h"
#include "hookFunc.h"

#pragma comment(lib, "Dbghelp.lib")

#include <io.h>
#include <conio.h>
#include <fstream>

using namespace std;

ofstream infile2;


char* filename2 = "C:\\hookfunction.txt";



//修改导入表
BOOL ModifyImportTable(
	IMAGE_IMPORT_DESCRIPTOR* iid, //导入表
	void* target, //目标
	void* replacement)	//代替
{
	IMAGE_THUNK_DATA* itd = (IMAGE_THUNK_DATA*)
		(((char*)GetModuleHandle(NULL)) + iid->FirstThunk);
	//GetModuleHandle()获取一个应用程序或动态链接库dll的模块句柄，NULL返回自身应用程序句柄

	infile2 << "\n\nModifyImportTable\n\n\t target\t" << target << "\n\n\t replacement\t" << &replacement << endl;


	while (itd->u1.Function)
	{

		if (((void*)itd->u1.Function) == target)
		{
			// Temporary change access to memory area to READWRITE
			MEMORY_BASIC_INFORMATION mbi;
			VirtualQuery(itd, &mbi, sizeof(MEMORY_BASIC_INFORMATION));//获取内存地址信息

			infile2 << "\nmbi:\n\tAllocationBase:\t " << mbi.AllocationBase << "  \n\tAllocationProtect\t" << mbi.AllocationProtect << "  \n\tBaseAddress\t" << mbi.BaseAddress << "  \n\tProtect\t" << mbi.Protect << "  \n\tRegionSize\t" << mbi.RegionSize << "  \n\tState\t" << mbi.State << "  \n\tType\t" << mbi.Type << endl;



			//VirtualProtect()修改内存属性为可读可写PAGE_READWRITE
			VirtualProtect(mbi.BaseAddress, mbi.RegionSize, PAGE_READWRITE, &mbi.Protect);	//&mbi.Protect保存老的属性

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

//安装钩子
BOOL InstallHook(LPCSTR module, LPCSTR function, void* hook, void** original)
{//LPCSTR字符串数据类型

	infile2.open(filename2);
	//infile << "\n"  << endl;

	HMODULE process = GetModuleHandle(NULL);

	infile2 << "\n当前模块句柄:" << process << endl;
	// Save original address to function
	*original = (void*)GetProcAddress(GetModuleHandleA(module), function);

	infile2 << "\n ntdll.dll中NtQuerySystemInformation的原始地址值：& " << &original << "  * " << *original << "  " << original << endl;

	//MessageBoxA(NULL, "GetProcAddress of NtQuerySystemInformation 1", "Success", MB_ICONINFORMATION);

	ULONG entrySize;	//8字节无符号整数

	//ImageDirectoryEntryToData()获取导入表地址
	IMAGE_IMPORT_DESCRIPTOR* iid =
		(IMAGE_IMPORT_DESCRIPTOR*)ImageDirectoryEntryToData(
		process,	//要获得的导入段所在模块的基地址
		1,			//为true时，系统将该模块以映像文件的形式映射，否则以数据文件的形式映射
		IMAGE_DIRECTORY_ENTRY_IMPORT,//要获得的段的索引
		&entrySize);	//返回表项的大小

	infile2 << "\n导入表：" << iid << endl;



	// Search for module
	while (iid->Name)
	{
		infile2 << "\n 名称：" << iid->Name << " \n\tCharacteristics:\t" << iid->Characteristics << "  \n\tFirstThunk\t" << iid->FirstThunk << "  \n\tForwarderChain\t" << iid->ForwarderChain << "  \n\tOriginalFirstThunk\t" << iid->OriginalFirstThunk << "  \n\tTimeDateStamp\t" << iid->TimeDateStamp << endl;

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
