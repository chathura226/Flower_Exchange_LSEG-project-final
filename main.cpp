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
void writeToFile(orderObj*,int,double);

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
    char cliOrd[7],inst[8];
    int side,qty;
    double price;
	if (FILE* filePointer = fopen("orders.csv", "r")) {
        std::cout<<"hi"<<std::endl;
		while (fscanf(filePointer, "%[^,],%[^,],%d,%d,%lf", cliOrd, inst, &side,&qty,&price)==5) {
            std::cout<<cliOrd<<inst<<side<<qty<<price<<std::endl;
            orderObj* o1=new orderObj;
            o1->clientOrderID=cliOrd;
            o1->inst=inst;
            o1->side=side;
            o1->price=price;
            o1->qty=qty;
            o1->priority=1;
            o1->status="New";
            rose.newOrder(o1);
            delete o1;
		}
        fclose(filePointer);
        
	    file.close();
	}

    
    
}
void writeToFile(orderObj* x,int qty,double price){
	std::cout<<"edh\n"<<x->clientOrderID<<","
        <<x->inst<<","
        <<x->side<<","
        <<x->status<<","
        <<qty<<","
        <<price<<"\n";
    file<<x->clientOrderID<<","
        <<x->inst<<","
        <<x->side<<","
        <<x->status<<","
        <<qty<<","
        <<price;
}