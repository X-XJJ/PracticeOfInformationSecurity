// ���� ifdef ���Ǵ���ʹ�� DLL �������򵥵�
// ��ı�׼�������� DLL �е������ļ��������������϶���� DLL_INJECT_EXPORTS
// ���ű���ġ���ʹ�ô� DLL ��
// �κ�������Ŀ�ϲ�Ӧ����˷��š�������Դ�ļ��а������ļ����κ�������Ŀ���Ὣ
// DLL_INJECT_API ������Ϊ�Ǵ� DLL ����ģ����� DLL ���ô˺궨���
// ������Ϊ�Ǳ������ġ�
#ifdef DLL_INJECT_EXPORTS
#define DLL_INJECT_API __declspec(dllexport)
#else
#define DLL_INJECT_API __declspec(dllimport)
#endif

// �����Ǵ� Dll_Inject.dll ������
class DLL_INJECT_API CDll_Inject {
public:
	CDll_Inject(void);

	// TODO:  �ڴ�������ķ�����
};

extern DLL_INJECT_API int nDll_Inject;

DLL_INJECT_API int fnDll_Inject(void);

//DLL_INJECT_API BOOL HookFindNextFileW(
//	HANDLE hFindFile,
//	LPWIN32_FIND_DATAW lpFindFileData
//	);


