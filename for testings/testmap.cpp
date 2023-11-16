#include<iostream>
#include<map>
using namespace std;

typedef std::pair<double,int>pair2;

struct compareOrder{
    bool operator()(const pair2& a,const pair2& b){
        if(a.first>b.first){
            return true;
        }else if(a.first==b.first){
            if(a.second<b.second){
                return true;
            }else return false;
        }else return false;
    }
};

class orderObj{
public:
    std::string clientOrderID;
    std::string inst;
    int side;
    double price;
    int qty;
    int priority;
    std::string status;
    orderObj(std::string& cliOrd, std::string& inst, int &side,int &qty,double &price){
        clientOrderID=cliOrd;
        this->inst=inst;
        this->side=side;
        this->qty=qty;
        this->price=price;
        priority=1;
        status="New";

        if(cliOrd.size()==0 || inst.size()==0 )status="Rejected";
        else if(inst!="Rose" &&  inst!="Lavender"&& inst!="Lotus"&& inst!="Tulip"&& inst!="Orchid")status="Rejected";
        else if(price<=0 || qty<10 || qty>1000 || qty%10!=0)status="Rejected";
        else if(side!=1 && side!=2)status="Rejected";

    }
    int executeQty(int qty){
        //doesnt need to check whether it becomes negative since i check that before calling this
        std::cout<<this->clientOrderID<<std::endl;
        this->qty-=qty;
        // this->status="Pfill";
        // if(this->qty==0){
        //     this->status="Fill";
        //     return 1;//returns 1 when order is fill
        // }
        return 0;//return 0 when pfill
    }
    
};
int main(){
    std::map<pair2,int,compareOrder> buy;
    buy.emplace(std::make_pair(55,2),3);
    buy.emplace(std::make_pair(69.1,2),0);
    buy.emplace(std::make_pair(55,1),1);
    buy.emplace(std::make_pair(69.1,1),2);

    for(auto it = buy.begin(); it != buy.end(); ++it)
{
    cout << it->first.first << " " << it->second <<"\n";
}

} 