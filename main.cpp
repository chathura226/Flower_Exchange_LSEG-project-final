#include<iostream>
#include<cstdio>
#include<string>
#include<map>
#include<fstream>
static int orderNum=0;
std::ofstream file;

typedef std::pair<double,int>pair;

class orderObj{
public:
    std::string clientOrderID;
    std::string inst;
    int side;
    double price;
    int qty;
    int priority;
    std::string status;

};

void writeToFile(orderObj* x,int qty,double price){

    file<<x->clientOrderID<<","
        <<x->inst<<","
        <<x->side<<","
        <<x->status<<","
        <<qty<<","
        <<price<<"\n";
}
class instrument{
public:
    std::map<pair,orderObj*> buy;
    std::map<pair,orderObj*> sell;
    void newOrder(orderObj* newObj){
        if(newObj->side==1&&sell.empty()){
            buy.emplace(std::make_pair(newObj->price,newObj->priority),newObj);
            writeToFile(newObj,newObj->qty,newObj->price);
            return;
        }else if(newObj->side==2&&buy.empty()){
        	
            sell.emplace(std::make_pair(newObj->price,newObj->priority),newObj);
            writeToFile(newObj,newObj->qty,newObj->price);
            return;
        }
    }
};




int main(){
    file.open("execution_rep.csv",std::ios_base::app);//to append
    
    instrument rose;
    orderObj* o1=new orderObj;
    o1->clientOrderID="hi31";
    o1->inst="Rose";
    o1->side=2;
    o1->price=523.00;
    o1->qty=100;
    o1->priority=1;
    o1->status="New";
    rose.newOrder(o1);
    delete o1;
	file.close();
}