#ifndef __PROTOCOL_UTIL_HPP__
#define __PROTOCOL_UTIL_HPP__

#include<iostream>
#include<string>
#include<strings.h>
#include<sstream>
#include<unistd.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include"Log.hpp"
using namespace std;
#define OK 200
#define NOT_FOUND 404
#define HOME_PAGE "index.html"//
class Request{
    public:
        Request()
            :blank("\n")
             ,cgi(false)
             ,path(WEB_ROOT)
    {}
        void RequestLineParse()//把读到的一行数据拆分成三部分
        {
            //分别是请求方法、请求资源uri、HTTP版本
            stringstream ss(rq_line);
            ss>>method>>uri>>version;
        }
        void UriParse()//解析uri判断是否是cgi以及分割uri uri可能包含两部分一部分是资源路径一部分是参数
        {
            size_t pos_=uri.find('?');//？是判断标准因为资源路径和参数的分割线是以?分割的如果没有代表uri中没有参数
            if(string::npos!=pos_)//找到了
            {
                cgi=true;
                path+=uri.substr(0,pos_);
                param=uri.substr(pos_+1);
            }
            else
            {
                path+=uri;//如果没找到uri中没有参数所有都是路径
            }
            if(path[path.size()-1]=='/')
            {//这里处理请求的是文件夹的情况如果请求文件夹直接返回默认的主页
                path+=HOME_PAGE;
            }
        }
        bool IsMethodLegal()
        {
            if(strcasecmp(method.c_str(),"GET")==0||cgi=(strcasecmp(method.c_str(),"POST")==0))
            {//这里我们比较字符串忽略大小写所以无论是大写的get还是小写的都算正确
                return true;
            //并且这里处理cgi如果是post请求方法cgi肯定是true
            }
            //如果不是get或者post返回错误
            return false;
        }
        bool IsPathLegal()
        {//通过stat命令查看文件是否存在
            struct stat st;
            if(stat(path.c_str(),&st)<0)
            {
                LOG(WARNING,"path not found!");
                return false;
            }
            if(S_ISDIR(st.st_mode))
            {//判断请求的文件是不是一个文件夹如果是返回主页
                path+="/";
                path+=HOME_PAGE;
            }
            else
            {
              if((st.st_mode&S_IXUSR)||\
                  (st.st_mode &S_IXGRP)||\
                  (st.st_mode &S_IXOTH))
              {
                  cgi=true;
                  //这里处理的是post请求如果请求的文件是一个可执行文件那么需要执行cgi
              }
            }
            resource_size=st.st_size;
            //顺便把返回的文件大小求出来不过他不是post类型的数据post类型返回的是数据还不知道多大
            size_t pos=path.rfind(".");//找文件后缀
            if(string::npos!=pos)
            {
                resource_suffix=path.substr(pos);
            }
            return true;
            
            
        }
        ~Request()
        {}

    private:
        string method;//请求方法
        string uri;//请求资源uri
        string version;//请求版本
        bool cgi;//是否需要cgi处理
        string path;//请求资源的路径
        string param;//cgi所需参数
        int resource_size;
        string resource_suffix;
     public:
        string rq_line;//请求行
        string rq_head;//请求头部
        string blank;//空行
        string rq_text;//请求正文
};
class Response{
    public:
        Response()
            :blank("\n")
             ,code(OK)
    {}
        ~Response()
        {}
    private:
        string rsp_line;//响应首行
        string rsp_head;//响应头部
        string blank;//响应空行
        string rsp_text;//响应正文
    public:
        int code;//响应状态码
};
class Connect{
    public:
        Connect(int sock_)//构造函数初始化
            :sock(sock_)
        {}
        int RecvOneLine(string& line_)
        {//从请求报头中读取一行数组
            char c='J';//对c进行初始化
            while(c!='\n')
            {
                ssize_t s=recv(sock,&c,1,0);
                //每次读取一个字节直到读取到换行符代表这一行读完了
                if(s>0)//大于0代表去读到字符了
                {//这里要考虑多种情况浏览器中的换行符有的是以/r/n当做换行符 有的是以/n有的是/r这里要统一处理
                    if(c=='\r')
                    {
                        recv(sock,&c,1,MSG_PEEK);
                        //MSG_PEEK窥探选项，只看不拿， 不然可能把下一行的数据拿了
                        if(c=='\n')
                        {
                            recv(sock,&c,1,0);
                        }
                        else
                        {
                            //窥探到/r下一个不是\n说明读到下一行了那就不拿这个数据把/r变成/n这样方便后序统一处理
                            c='\n';
                        }
                    }
                    line_.push_back(c);//拿到的数据写入到传进来的string字符串中
                }
                else
                {
                   //recv返回值大于0代表读取到的字符等于0代表对方关闭了，小于0代表出错了
                    break;
                }
            }
            return line_.size();
        }
        void RecvRequestHead(string &head_)
        {//读取请求报头的头部n行kv值
            head_="";
            string line_;
            while(line_!="\n")//如果读到\n说明读到了空行后边就是正文了
            {
               line_="";
               RecvOneLine(line_);//继续用recv读取一行数据
               head_+=line_;
            }
        }
        ~Connect()
        {
            if(sock>=0)
            {
                close(sock);
            }
        }
    private:
        int sock;
};
class Entry{
    public:
        static void *HandlerRequest(void *arg_)
        {//arg_是监听到的socket
          int sock_=*(int*)arg_;
          delete (int*)arg_;
          Connect *conn_=new Connect(sock_);
          Request *rq_=new Request();
          Response *rsp_=new Response();
          
          int &code_=rsp_->code;
 
          conn_->RecvOneLine(rq_->rq_line);
          //读取一行数据第一行自然给了请求类中的请求头
          rq_->RequestLineParse();
          if(!rq_->IsMethodLegal())//如果请求数据的方式合法
          {
              code_=NOT_FOUND;
              goto end;
          }
          rq_->UriParse();
          if(rq_->IsPathLegal())
          {
              code_=NOT_FOUND;
              goto end;
          }
          LOG(INFO,"request path is OK!");
          conn_->RecvRequestHead(rq_->rq_head);//读取请求报头首部数据之后放到rq_head里边
          if(rq_->RequestHeadParse())
          {
              LOG(INFO,"parse head done");
          }
          else
          {
              code_=NOT_FOUND;
              goto end;
          }
end:
          if(code_!=OK)
          {
          }
          delete conn_;
          delete rq_;
          delete rsp_;
        }
};
