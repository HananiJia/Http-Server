#ifndef __PROTOCOL_UTIL_HPP__
#define __PROTOCOL_UTIL_HPP__

#include<iostream>
#include<string>
#include<strings.h>
#include<string.h>
#include<fcntl.h>
#include<sys/sendfile.h>
#include<sys/wait.h>
#include<sstream>
#include<unistd.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<unordered_map>
#include"Log.hpp"
using namespace std;

#define OK 200
#define NOT_FOUND 404

#define HOME_PAGE "index.html"//主页文件
#define WEB_ROOT "wwwroot"//根目录
#define HTTP_VERSION "http/1.0"

unordered_map<string,string> stuffix_map{
    {".html","text/html"},
    {".htm","text/html"},
    {".css","text/css"},
    {".js","application/x-javascript"}
};

class ProtocolUtil
{
    public:
    static void MakeKV(unordered_map<string,string>& kv_,string &str_)
    {
        size_t pos=str_.find(": ");//键值对都是以这个区分键和值
        if(string::npos==pos)
        {
            return ;
        }
        string k_=str_.substr(0,pos);
        string v_=str_.substr(pos+2);
        kv_.insert(make_pair(k_,v_));
    }
    static string IntToString(int code)
    {
        stringstream ss;
        ss<<code;
        return ss.str();
    }
    static string CodeToDesc(int code)
    {
        switch(code)
        {
            case 200:
                return "OK";
            case 404:
                return "NOT FOUND";
            default:
                return "UNKNOW";
        }
    }
    static string SuffixToType(const string& suffix_)
    {
        return stuffix_map[suffix_];
    }

};
class Request{
    public:
        Request()
            :blank("\n")
             ,cgi(false)
             ,path(WEB_ROOT)
             ,resource_size(0)
             ,content_length(-1)
             ,resource_suffix(".html")
                 {}
        void RequestLineParse()//把读到的一行数据拆分成三部分
        {
            LOG(INFO,"RequestLineParse Success");
            //分别是请求方法、请求资源uri、HTTP版本
            stringstream ss(rq_line);
            ss>>method>>uri>>version;
        }
        void UriParse()//解析uri判断是否是cgi以及分割uri uri可能包含两部分一部分是资源路径一部分是参数
        {
            LOG(INFO,"UriParse Success");
            if(strcasecmp(method.c_str(),"GET")==0)
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
            }
            else
            {
                path+=uri;
            }
            if(path[path.size()-1]=='/')
            {//这里处理请求的是文件夹的情况如果请求文件夹直接返回默认的主页
                path+=HOME_PAGE;
            }
        }
        bool IsMethodLegal()
        {
            LOG(INFO,"IsMethodLegal Success");
            if(strcasecmp(method.c_str(),"GET")==0||(cgi=(strcasecmp(method.c_str(),"POST")==0)))
            {//这里我们比较字符串忽略大小写所以无论是大写的get还是小写的都算正确
                return true;
            //并且这里处理cgi如果是post请求方法cgi肯定是true
            }
            //如果不是get或者post返回错误
            return false;
        }
        bool IsPathLegal()
        {//通过stat命令查看文件是否存在
            LOG(INFO,"IsPathLegal Success");
            struct stat st;
            if(stat(path.c_str(),&st)<0)
            {
                LOG(WARNING,"Path Not Found!");
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
        bool RequestHeadParse()//拆分请求首部
        {
            LOG(INFO,"RequestHeadParse Success");
            int start=0;
            cout<<rq_head<<endl;
            while(start<rq_head.size())
            {
                size_t pos=rq_head.find('\n',start);
                //首部是一堆kv键值对以\n分隔
                if(string::npos==pos)
                {
                    break;
                }
                string sub_string_=rq_head.substr(start,pos-start);
                if(!sub_string_.empty())
                {
                    LOG(INFO,"substr is not empty");
                    ProtocolUtil::MakeKV(head_kv,sub_string_);
                }//可能存在字符串为空的情况
                else
                {
                    LOG(INFO,"substr is empty");
                    break;
                }
                start=pos+1;
            }
            return true;
        }
        bool IsNeedRecvText()
        {
            LOG(INFO,"IsNeedRecvText Success");
            cout<<method<<endl;
          if(strcasecmp(method.c_str(),"POST")==0)
          {
              return true;
          }
          return false;
        }
        bool IsCgi()
        {
            return cgi;
        }
        string& GetSuffix()
        {
            return  resource_suffix;
        }
        int GetResoureSize()
        {
            return resource_size;
        }
        void SetResourceSize(int rs_)
        {
            resource_size=rs_;
        }
        string& GetPath()
        {
            return path;
        }
        string& GetParam()
        {
            return param;
        }
        int GetContentLength()
        {
            string cl_=head_kv["Content-Length"];
            if(!cl_.empty())
            {
                stringstream ss(cl_);
                ss >>content_length;
            }
            return content_length;
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
        unordered_map<string,string> head_kv;
        int content_length;
     public:
        string rq_line;//请求行
        string rq_head;//请求头部
        string blank;//空行
        string rq_text;//请求正文
};
class Response{
    public:
        Response()
            :rsp_blank("\n")
             ,code(OK)
             ,fd(-1)
    {}
        void MakeStatusLine()
        {
            LOG(INFO,"MakeStatusLine Success");
            rsp_line=HTTP_VERSION;
            rsp_line+=" ";
            rsp_line+=ProtocolUtil::IntToString(code);
            rsp_line+=" ";
            rsp_line+=ProtocolUtil::CodeToDesc(code);
            rsp_line+="\n";
            cout<<rsp_line<<endl;
        }
        void MakeResponseHead(Request *&rq_)
        {
            LOG(INFO,"MakeResponseHead Success");
            rsp_head="Content-Length: ";
            rsp_head+=ProtocolUtil::IntToString(rq_->GetResoureSize());
            rsp_head+="\n";
            rsp_head+="Content-Type: ";
            string suffix_=rq_->GetSuffix();
            rsp_head+=ProtocolUtil::SuffixToType(suffix_);
            rsp_head+="\n";
            cout<<rsp_head<<endl;
        }
        void OpenResource(Request *&rq_)
        {
           LOG(INFO,"OpenResource Success");
            string path_=rq_->GetPath();
            fd=open(path_.c_str(),O_RDONLY);
            cout<<"文件描述符"<<fd<<endl;
        }
        ~Response()
        {
            if(fd>=0)
            {
                close(fd);
            }
        }
    public:
        string rsp_line;//响应首行
        string rsp_head;//响应头部
        string rsp_blank;//响应空行
        string rsp_text;//响应正文
        int code;//响应状态码
        int fd;//非cgi下返回文件的文件描述符

};
class Connect{
    public:
        Connect(int sock_)//构造函数初始化
            :sock(sock_)
        {}
        int RecvOneLine(string& line_)
        {//从请求报头中读取一行数组
            LOG(INFO,"RecvOneLine Success");
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
            cout<<line_;
            return line_.size();
        }
        void RecvRequestHead(string &head_)
        {//读取请求报头的头部n行kv值;
            LOG(INFO,"RecvRequestHead Success");
            head_="";
            string line_;
            while(line_!="\n")
            {
               line_="";
               RecvOneLine(line_);//继续用recv读取一行数据
               head_+=line_;
               cout<<line_;
            }
        }
        void RecvRequestText(string& text_,int len_,string& param_)
        {
           LOG(INFO,"RecvRequestText Success");
          cout<<"请求长度"<<len_;
           char c_;
          int i_=0;
          while(i_<len_)
          {
            recv(sock,&c_,1,0);
            cout<<c_;
            text_.push_back(c_);
          }
          param_=text_;
          
          cout<<"text_"<<text_<<endl;
          cout<<"param_"<<param_<<endl;
        }
        void SendResponse(Response *&rsp_,Request *&rq_,bool cgi_)
        {
            LOG(INFO,"SendResponse Success");
            string &rsp_line_=rsp_->rsp_line;
            string &rsp_head_=rsp_->rsp_head;
            string &rsp_blank_=rsp_->rsp_blank;
            send(sock,rsp_line_.c_str(),rsp_line_.size(),0);
            cout<<rsp_line_;
            send(sock,rsp_head_.c_str(),rsp_head_.size(),0);
            cout<<rsp_head_;
            send(sock,rsp_blank_.c_str(),rsp_blank_.size(),0);
            cout<<rsp_blank_;
            if(cgi_)//如果是cgi发送的是处理后的数据
            {
               string &rsp_text_=rsp_->rsp_text;
               send(sock,rsp_text_.c_str(),rsp_text_.size(),0);
            }
            else
            {
                //不是cgi返回的是请求的网页文件
                int &fd=rsp_->fd;
                cout<<"&"<<fd;
                size_t s=sendfile(sock,fd,NULL,rq_->GetResoureSize());
                cout<<"文件大小:"<<s<<endl;
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
       static void ProcessNonCgi(Connect *&conn_,Request *&rq_,Response *&rsp_) 
        {
            LOG(INFO,"ProcessNonCgi");
            int &code_=rsp_->code;
            rsp_->MakeStatusLine();
            rsp_->MakeResponseHead(rq_);
            rsp_->OpenResource(rq_);
            conn_->SendResponse(rsp_,rq_,false);
        }
       static void ProcessCgi(Connect *&conn_,Request *&rq_,Response *&rsp_)
       {
           LOG(INFO,"ProcessCgi");
           int &code_=rsp_->code;
           int input[2];
           int output[2];
           string &param_=rq_->GetParam();
           string &rsp_text_=rsp_->rsp_text;

           pipe(input);
           pipe(output);
           pid_t id=fork();
           if(id<0)
           {
               code_=NOT_FOUND;
               return ;
           }
           else
           {
               if(id==0)
               {
                   close(input[1]);
                   close(output[0]);
                   const string &path_=rq_->GetPath();
                   string cl_env_="Content-Length=";
                   cl_env_+=ProtocolUtil::IntToString(param_.size());
                   cout<<"env: "<<cl_env_<<endl;
                   putenv((char*)cl_env_.c_str());
                   dup2(input[0],0);
                   dup2(output[1],1);
                   execl(path_.c_str(),path_.c_str(),NULL);
                   exit(1);
               }
               else
               {
                   close(input[0]);
                   close(output[1]);

                   size_t size_=param_.size();
                   size_t total_=0;
                   size_t curr_=0;
                   const char *p_=param_.c_str();
                   while(total_<size_&&\
                           (curr_=write(input[1],p_+total_,size_-total_))>0)
                   {
                       total_+=curr_;
                   }
                   cout<<total_<<endl;
                   char c;
                   while(read(output[0],&c,1)>0)
                   {
                       rsp_text_.push_back(c);
                   }
                   cout<<"rsp_text:"<<rsp_text_<<endl;
                   waitpid(id,NULL,0);

                   close(input[1]);
                   close(output[0]);
                   
                   rsp_->MakeStatusLine();
                   rq_->SetResourceSize(rsp_text_.size());
                   rsp_->MakeResponseHead(rq_);
                   conn_->SendResponse(rsp_,rq_,true);
               }
           }
       }
       static int ProcessResponse(Connect *&conn_,Request *&rq_,Response *&rsp_)
        {
            if(rq_->IsCgi())
            {
                ProcessCgi(conn_,rq_,rsp_);
            }
            else
            {
                LOG(INFO,"ProcessNonCgi");
                ProcessNonCgi(conn_,rq_,rsp_); 
            }
        }
        static void *HandlerRequest(void *arg_)
        {//arg_是监听到的socket
           LOG(INFO,"HandlerRequest Success");
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
          if(!rq_->IsPathLegal())
          {
              code_=NOT_FOUND;
              goto end;
          }
          LOG(INFO,"Request Path is OK!");
          conn_->RecvRequestHead(rq_->rq_head);//读取请求报头首部数据之后放到rq_head里边
          if(rq_->RequestHeadParse())
          {
              LOG(INFO,"Parse Head Done");
          }
          else
          {
              code_=NOT_FOUND;
              goto end;
          }
          if(rq_->IsNeedRecvText())//判断是不是需要读取正文部分，get方法不需要读取
          {
             conn_->RecvRequestText(rq_->rq_text,rq_->GetContentLength(),rq_->GetParam());
          }
          LOG(INFO,"ProcessResponse");
          ProcessResponse(conn_,rq_,rsp_);
          LOG(INFO,"end");
end:
          if(code_!=OK)
          {
          }
          delete conn_;
          delete rq_;
          delete rsp_;
        }
};
#endif
