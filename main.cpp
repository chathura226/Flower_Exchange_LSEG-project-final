#include<iostream>
#include<cstdio>
#include<string>
#include<map>
#include<fstream>
#include<algorithm>
static int orderNum=0;
std::ofstream file;

typedef std::pair<double,int> pair;

struct compareOrder{
    bool operator()(const pair& a,const pair& b){
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
        std::cout<<"execute order called for "<<clientOrderID<<std::endl;
        this->qty-=qty;
        this->status="Pfill";
        if(this->qty==0){
            this->status="Fill";
            std::cout<<"Execute order finished!\n";
            return 1;//returns 1 when order is fill
        }
        std::cout<<"Execute order finished!\n";
        return 0;//return 0 when pfill
        
    }
    
};
void writeToFile(orderObj*,int,double);


void initializeInstrumentArray();
int initializeIns(int);
int validateAndCreate(std::string &cliOrd, std::string &inst, int &side,int &qty,double &price);
void printfDetails(orderObj* x){
    std::cout<<"print details of follwing\n"<<x->clientOrderID<<","
        <<x->inst<<","
        <<x->side<<","
        <<x->status<<","
        <<x->qty<<","
        <<x->price<<"\n";
}

int minQty(orderObj* x,orderObj*y){
    std::cout<<"minqty called for  "<<x->qty<<" "<<y->qty<<std::endl;
    if(x->qty>y->qty)return y->qty;
    else return x->qty;
}
class instrument{
public:
    std::string name;
    std::map<pair,orderObj*,compareOrder> buy;//descending order
    std::map<pair,orderObj*> sell;
    void newOrder(orderObj* newObj){
        std::cout<<"Accessing "<<name<<" newOrder method\n";
        auto relBegin = (newObj->side==2)  ? buy.begin() : sell.begin();
        if(newObj->side==1&&(sell.empty() || (relBegin->second->price)>newObj->price) ){
            auto x=buy.find(std::make_pair(newObj->price,newObj->priority));
            if(x!=buy.end()){
                    int i=1;
                    while((++x)->second->price==newObj->price)i++;
                    newObj->priority=++i;
            }
            buy.emplace(std::make_pair(newObj->price,newObj->priority),newObj);
            writeToFile(newObj,newObj->qty,newObj->price);
            return;
        }else if(newObj->side==2&&(buy.empty() || (relBegin->second->price)<newObj->price)){
            auto x=sell.find(std::make_pair(newObj->price,newObj->priority));
            if(x!=sell.end()){
                    int i=1;
                    while((++x)->second->price==newObj->price)i++;
                    newObj->priority=++i;
            }
            sell.emplace(std::make_pair(newObj->price,newObj->priority),newObj);
            writeToFile(newObj,newObj->qty,newObj->price);
            return;
        }else if(newObj->side==1){//this is a buy order and theres a sell order for equal or low price
            std::cout<<"inside side==1 but sell is not empty"<<std::endl;
            while(!sell.empty() && (relBegin->second->price)<=newObj->price){
                int execQty=minQty(newObj,relBegin->second);
                printfDetails(relBegin->second);
                std::cout<<"excec qty "<<execQty<<std::endl;
                int isFill1=newObj->executeQty(execQty);
                
                writeToFile(newObj,execQty,relBegin->second->price);
                int isFill2=relBegin->second->executeQty(execQty);
                writeToFile(relBegin->second,execQty,relBegin->second->price);
                
                if(isFill2){
                    orderObj* itr=relBegin->second;
                    sell.erase(std::make_pair(itr->price,itr->priority));
                    if(!sell.empty()){
                        auto temp=sell.begin();
                        while(temp->second->price==itr->price){
                            temp->second->priority--;
                            temp++;
                        }
                    }
                    std::cout<<"Deleting order "<<itr->clientOrderID<<std::endl;
                    delete itr;
                    std::cout<<"Deleted!\n";
                    if(!sell.empty()) relBegin=sell.begin();
                    else break;
                }
                if(isFill1){
                    std::cout<<"Deleting order "<<newObj->clientOrderID<<std::endl;
                    delete newObj;
                    std::cout<<"Deleted!\n";
                    break;
                }
                
            }
        }else if(newObj->side==2){//this is a sell order and theres a but order for equal or low price
            std::cout<<"inside side==2 but buy is not empty"<<std::endl;
            while(!buy.empty() && (relBegin->second->price)>=newObj->price){
            //while(!buy.empty() && (buy.begin()->second->price)>=newObj->price){    
                std::cout<<"inside while loop\n";
                int execQty=minQty(newObj,relBegin->second);
                int isFill1=newObj->executeQty(execQty);
                writeToFile(newObj,execQty,relBegin->second->price);
                int isFill2=relBegin->second->executeQty(execQty);
                writeToFile(relBegin->second,execQty,relBegin->second->price);
                
                if(isFill2){
                    orderObj* itr=relBegin->second;
                    buy.erase(std::make_pair(itr->price,itr->priority));
                    if(!buy.empty()){
                        auto temp=buy.begin();
                        while(temp->second->price==itr->price){
                            temp->second->priority--;
                            temp++;
                        }
                    }
                    std::cout<<"Deleting order "<<itr->clientOrderID<<std::endl;
                    delete itr;
                    std::cout<<"Deleted!\n";
                    if(!buy.empty()) relBegin=buy.begin();
                    else break;
                }

                if(isFill1){
                    std::cout<<"Deleting order "<<newObj->clientOrderID<<std::endl;
                    delete newObj;
                    std::cout<<"Deleted!\n";
                    break;
                }
                // if(!buy.empty())relBegin++;
                // std::cout<<"After increment\n";
            }
        }

        
    }
};

instrument* allInstruments[5];

int main(){
    initializeInstrumentArray();
    file.open("execution_rep.csv",std::ios_base::app);//to append
    char cliOrd[7],inst[8];
    int side,qty;
    double price;
	if (FILE* filePointer = fopen("orders.csv", "r")) {
        std::cout<<"hi"<<std::endl;
		while (fscanf(filePointer, "%[^,],%[^,],%d,%d,%lf\n", cliOrd, inst, &side,&qty,&price)==5) {
            std::cout<<cliOrd<<inst<<side<<qty<<price<<"boohoo"<<std::endl;
            std::string cliOrdString=std::string(cliOrd);
            std::string instString=std::string(inst);
            int temp=validateAndCreate(cliOrdString,instString,side,qty,price);
            if(temp==1)continue;

		}
        fclose(filePointer);
        
	    file.close();
	}

    
    
}
void writeToFile(orderObj* x,int qty,double price){
	std::cout<<"write to file called for following\n"<<x->clientOrderID<<","
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
        <<price<<"\n";

        //<<x->priority;//remove this later
    std::cout<<"write to file finished!\n";

}


void initializeInstrumentArray(){
    allInstruments[0]=NULL;
    allInstruments[1]=NULL;
    allInstruments[2]=NULL;
    allInstruments[3]=NULL;
    allInstruments[4]=NULL;
}

int initializeIns(int i){
    instrument *temp=new instrument;
    if(temp==NULL){
        std::cout<<"Error allocating memory for instrument book!\n";
        return 1;
    }
    allInstruments[i]=temp;
    switch(i){
        case 0: temp->name="Rose"; std::cout<<"Initialized the book for Rose\n"; break;
        case 1: temp->name="Lavender";std::cout<<"Initialized the book for Lavender\n"; break;
        case 2: temp->name="Lotus";std::cout<<"Initialized the book for Lotus\n"; break;
        case 3: temp->name="Tulip";std::cout<<"Initialized the book for Tulip\n"; break;
        case 4: temp->name="Orchid";std::cout<<"Initialized the book for Orchid\n"; break;
    }
    return 0;

}

int validateAndCreate(std::string& cliOrd, std::string& inst, int &side,int &qty,double &price){
    orderObj* temp=new orderObj(cliOrd, inst, side,qty,price);
    if(temp->status=="Rejected"){
        writeToFile(temp,temp->qty,temp->price);
        delete(temp);
        return 1;
    }
    int i;
    if(temp->inst==std::string("Rose"))i=0;
    else if(temp->inst==std::string("Lavender"))i=1;
    else if(temp->inst==std::string("Lotus"))i=2;
    else if(temp->inst==std::string("Tulip"))i=3;
    else if(temp->inst==std::string("Orchid"))i=4;
    
    if(allInstruments[i]==NULL)initializeIns(i);
    allInstruments[i]->newOrder(temp);
    return 0;

}