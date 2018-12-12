#ifndef __LOG_HPP__
#define __LOG_HPP__

#include<iostream>
#include<string>
#include<sys/time.h>
#include<time.h>
using namespace std;

#define INFO 0
#define DEBUG 1
#define WARNING 2
#define ERROR 3

uint64_t  GetTimeStamp()
{
    struct timeval time_;
    gettimeofday(&time_,NULL);
    return time_.tv_sec;
}
void GetSystemTime()
{
     time_t t;
     struct tm * lt;
     time (&t);//获取Unix时间戳。
     lt = localtime (&t);//转为时间结构。
     printf ( "%d/%d/%d %d:%d:%d\n",lt->tm_year+1900, lt->tm_mon, lt->tm_mday, lt->tm_hour, lt->tm_min, lt->tm_sec);//输出结果
}
string GetLogLevel(int level_)
{
    //简单划分日志等级
    switch(level_)
    {
      case 0:
          return "INFO";
      case 1:
          return "DEBUG";
      case 2:
          return "WARNING";
      case 3:
          return "ERROR";
      default:
          return "UNKNOW";
    }
}
void Log(int level_,string message_,string file_,int line_)
{
    GetSystemTime();
   cout<<" ["<<GetLogLevel(level_)<<" ]"<<" [ "<<file_<<" : "<<line_<<" ] "<<message_<<endl; 
}

#define LOG(level_,message_) Log(level_,message_, __FILE__,__LINE__)

#endif
