#include <iostream>
#include <cstdio>
#include <unistd.h>
#include <string>
#include <stdlib.h>
#include <sstream>

int main()
{
    if(getenv("Content-Length")){
        int size_ = atoi(getenv("Content-Length"));
        std::string param_;
        char c_;
        while(size_){
            read(0, &c_, 1);
            param_.push_back(c_);
            size_--;
        }

        //a=100&b=200
        double a, b, c;
        int n=0;
        std::ostringstream oss;
        sscanf(param_.c_str(), "a=%lf&b=%lf&c=%lf", &a, &b,&c);
        double sum=a;
        double minamount=b;
        double maxamount=c;
        for(double i=minamount;i<=maxamount;i+=0.01){
        double num=sum/i;
         if(num==(int)num){
             n++;
            oss<<"发票张数为"<<num<<" "<<"每张发票单价为"<<i<<"<br>";
         }
         }
         if(n==0){
           oss<<"当前金额和区间没有开票结果 <br>开票总金额为 "<<sum<<"<br>单张票最小金额为 "<<minamount<<"<br>单张票最小金额为 "<<maxamount;
         }else{
           oss<<"共有"<<n<<"种开票结果!"<<"<br>";
         }
        std::cout << "<html>" << std::endl;
        std::cout <<"<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" />"<<std::endl;
        std::cout << "<head>发票计算器</head>" <<std::endl;
        std::cout << "<body>" << std::endl;
        std::cout << "<h1>" << oss.str() << "</h1>" <<std::endl;
        std::cout << "</body>" << std::endl;
        std::cout << "</html>" << std::endl;
    }
    return 0;
}


















