#include<windows.h>
#include "stdio.h"

#define Install_Run 1
#define Stop_Unload 2

void installDvr();//��װ����
void startDvr();//��������
void stopDvr();//ֹͣ����
void unloadDvr();//ж������
void setdriver(int op)
{
	if (op == 1)
	{
		installDvr();//��װ����
		startDvr();//�������� 
	}
	if (op == 2)
	{
		stopDvr();//ֹͣ����
		unloadDvr();//ж������
	}

}

void installDvr()//��װ
{
	SC_HANDLE schSCManager;
	schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (schSCManager)
	{
		SC_HANDLE schService = CreateService(schSCManager,

			"babaHelloDDKk",

			"babaMyHelloDDKk",

			SERVICE_ALL_ACCESS,

			SERVICE_KERNEL_DRIVER, //�����ķ�������1Ϊ��������

			SERVICE_DEMAND_START, //���ڵ��н��̵���StartService ����ʱ�ɷ�����ƹ�����(SCM)�����ķ��񡣲�ѯStarting Services on Demand�Ի�ȡ������Ϣ��

			SERVICE_ERROR_IGNORE,

			"C:\\WINDOWS\\system32\\drivers\\cccc.sys",//�����ļ����·��

			NULL,

			NULL,

			NULL,

			NULL,

			NULL);
		CloseServiceHandle(schService); //������ǵ��ͷž��
		if (schService)
		{
			printf("��װ����ɹ�\n");

		}
		else
		{
			printf("��װ����ʧ��\n");
		}
		CloseServiceHandle(schSCManager);
	}


}
void startDvr()//����
{
	SC_HANDLE schSCManager;
	SC_HANDLE hs;
	schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (schSCManager)
	{
		hs = OpenService(schSCManager, "babaHelloDDKk", SERVICE_ALL_ACCESS); //�򿪷���
		if (hs)
		{
			StartService(hs, 0, 0);
			printf("��������ɹ�\n");

			CloseServiceHandle(hs);
		}
		CloseServiceHandle(schSCManager);
	}
}
void stopDvr()//ֹͣ
{
	SC_HANDLE schSCManager;
	SC_HANDLE hs;
	schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (schSCManager)
	{
		hs = OpenService(schSCManager, "babaHelloDDKk", SERVICE_ALL_ACCESS); //�򿪷���
		if (hs)
		{
			SERVICE_STATUS status;
			int num = 0;

			QueryServiceStatus(hs, &status);
			if (status.dwCurrentState != SERVICE_STOPPED && status.dwCurrentState != SERVICE_STOP_PENDING)
			{
				ControlService(hs, SERVICE_CONTROL_STOP, &status);
				do
				{
					Sleep(50);
					num++;
					QueryServiceStatus(hs, &status);
				} while (status.dwCurrentState != SERVICE_STOPPED || num > 80);
			}

			if (num > 80)
			{
				printf("ֹͣ����ʧ��\n");
			}
			else
			{
				printf("ֹͣ����ɹ�\n");
			}
			CloseServiceHandle(hs);
		}
		CloseServiceHandle(schSCManager);
	}

}
void unloadDvr()//ж��
{
	SC_HANDLE schSCManager;
	SC_HANDLE hs;
	schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (schSCManager)
	{
		hs = OpenService(schSCManager, "babaHelloDDKk", SERVICE_ALL_ACCESS); //�򿪷���
		if (hs)
		{
			bool a = DeleteService(hs); //ɾ������
			if (!a)
			{
				printf("ɾ������ʧ��\n");
			}
			else
			{
				printf("��ɾ������\n");
			}

			CloseServiceHandle(hs);//�ͷ����������ɴӷ��������ʧ �ͷ�ǰ���ѽ�ֹ״̬
		}
		CloseServiceHandle(schSCManager);
	}

}