//主函数
#include "HttpdServer.hpp"
#include<signal.h>
static void Usage(string _proc)
{
    //如果启动服务器没有指定参数提示用户
    cout<<"Usage "<<_proc<<"port"<<endl;
}
int main(int argc,char*argv[])
{
   if(argc!=2)
	{
      Usage(argv[0]);
	  exit(1);
	}
   signal(SIGPIPE,SIG_IGN);
    HttpdServer *serp=new HttpdServer(atoi(argv[1]));
    //构造服务器类对象把要监听的端口传进去
    serp->InitServer();//初始化服务器
    serp->Start();//启动服务器
    delete serp;
    return 0;	
}
