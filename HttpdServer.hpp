#ifndef __HTTPD_SERVER_HPP__
#define __HTTPD_SERVER_HPP__

#include<pthread.h>
#include"ThreadPool.hpp"
#include"ProtocolUtil.hpp"

class HttpdServer{
public:
    HttpdServer(int port_)
        :port(port_)
         ,listen_sock(-1)
         ,tp(NULL)
    {}//构造函数
    void InitServer()//初始化函数绑定端口监听指定端口
    {
        listen_sock=socket(AF_INET,SOCK_STREAM,0);
        //初始化监听socket
        if(listen_sock<0)
        {
            LOG(ERROR,"Create Socket Error!");
            //如果创建失败，打印日志
            exit(2);
        }
        int opt_=1;
        setsockopt(listen_sock,SOL_SOCKET,SO_REUSEADDR,&opt_,sizeof(opt_));//设置服务器即使挂掉也可以迅速重启
        struct sockaddr_in local_;//初始化socket
        local_.sin_family=AF_INET;
        local_.sin_port=htons(port);
        local_.sin_addr.s_addr=INADDR_ANY;
        //INADDR_ANY绑定该机器任意一个IP地址
        if(bind(listen_sock,(struct sockaddr*)&local_,sizeof(local_))<0)
        {//绑定端 如果失败打印日志
            LOG(ERROR,"Bind Socket Error!");
            exit(3);
        }
        if(listen(listen_sock,5)<0)
        {
            //如果监听失败打印日志信息
            LOG(ERROR,"Listen Socket Error");
            exit(4);
        }
        //代表一切正常打印成功日志
        tp=new ThreadPool();
        tp->initThreadPool();
        LOG(INFO,"Initserver Success!");
    }
    void Start()
    {
        LOG(INFO,"Start Server Begin!");
        for(;;)
        {
            struct sockaddr_in peer_;
            socklen_t len_=sizeof(peer_);
            int sock_=accept(listen_sock,(struct sockaddr*)&peer_,&len_);
            if(sock_<0)
            {
                LOG(WARNING,"Accept Error!");
                continue;
            }
            LOG(INFO,"Get New Client, Create Thread Handler Request");
             pthread_t tid_;
             Task t;
             t.SetTask(sock_,Entry::HandlerRequest);
             tp->PushTask(t);
            // pthread_create(&tid_,NULL,Entry::HandlerRequest,(void*)sockp_);
             //将接受到的socket通过多线程的方式去调用handlerquest函数
             //这里要注意线程创建函数后两个参数类型要是void*
        }
    } 
    ~HttpdServer()
    {
        if(listen_sock!=-1)
        {
            close(listen_sock);
        }
        port=-1;
    }

private: 
    int listen_sock;
    int port;
    ThreadPool *tp;
};
#endif 
