#include<iostream>
#include<cstdio>
#include <stdio.h>
#include<string>
#include<map>
#include<fstream>
#include<algorithm>
#include<iomanip>
#include <string.h>
#include<regex>
#include <sstream>  
#include<vector>
#include<memory>
#include <chrono>
#include<unordered_map>
#define LOGS_Enabled 0//to enable my test logs
static int orderNum=0;
std::ofstream file;
typedef std::pair<double,int> pair;
double epsilon=0.001;

struct compareOrder{
    bool operator()(const pair& a,const pair& b) const {

        
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
    std::string orderId;
    std::string clientOrderID;
    std::string inst;
    int side;
    double price;
    int qty;
    int priority;
    std::string status;
    orderObj();//define later
    orderObj(std::string& cliOrd, std::string& inst, int &side,int &qty,double &price);
    int executeQty(int qty);
    ~orderObj(){
        if(LOGS_Enabled)std::cout<<"OrderObj destructor called for ordNum: "<<orderId<<"\n";
    }
    
};


class instrument{
public:
    std::string name;
    std::map<const pair,std::shared_ptr<orderObj>,compareOrder> buy;//descending order
    std::unordered_map<double,std::map<const pair,std::shared_ptr<orderObj>>::iterator> buyHighestPrio;
    std::map<const pair,std::shared_ptr<orderObj>> sell;
    std::unordered_map<double,std::map<const pair,std::shared_ptr<orderObj>>::iterator> sellHighestPrio;
    std::map<const pair,std::shared_ptr<orderObj>>::iterator buyBegin;
    std::map<const pair,std::shared_ptr<orderObj>>::iterator sellBegin;
    instrument();
    void newOrder(std::shared_ptr<orderObj> newObj);
};

void printOrderBook(std::shared_ptr<orderObj>);
void writeToFile(std::shared_ptr<orderObj>,int,double);
void writeHeader();
std::string transac_time();
void initializeInstrumentArray();
int initializeIns(int);
int validateAndCreate(std::string &cliOrd, std::string &inst, int &side,int &qty,double &price);
void printfDetails(std::shared_ptr<orderObj> x);//for test logs
int minQty(std::shared_ptr<orderObj> x,std::shared_ptr<orderObj>y);//compare qtys and find the correct qty


instrument* allInstruments[5];//pointer array for order books

int main(){
    initializeInstrumentArray();
    file.open("execution_rep.csv",std::ios_base::app);//to append
    file<<std::fixed<<std::setprecision(2);
    writeHeader();
    std::string cliOrd,inst;
    int side,qty;
    double price;
	if (FILE* filePointer = fopen("orders.csv", "r")) {
        char linechar[1024];

        int fieldOrder[]={0,1,2,3,4};
        int nLines=0;
		while (fgets(linechar, 1024, filePointer)) {
            std::string line=std::string(linechar);
            if (!line.empty() && line[line.size() - 1] == '\n') {
                line.erase(line.size() - 1);
            }
            std::regex field_regex(R"(([^,]*),?)");
            std::vector<std::string> fields;
            auto field_begin = std::sregex_iterator(line.begin(), line.end(), field_regex);
            auto field_end = std::sregex_iterator();
            for (auto i = field_begin; i != field_end; ++i) {
                fields.push_back(i->str(1));
            }
            if(nLines==0){//first line of the file-headers
                for(int i=0;i<5;i++){
                    std::cout<<"dd"<<fields[i]<<"fff\n";
                    if(fields[i]=="Client Order ID"||fields[i]=="Client Order ID\n") fieldOrder[0]=i;
                    else if(fields[i]=="Instrument"||fields[i]=="Instrument\n") fieldOrder[1]=i;
                    else if(fields[i]=="Side"||fields[i]=="Side\n") fieldOrder[2]=i;
                    else if(fields[i]=="Price"||fields[i]=="Price\n") fieldOrder[3]=i;
                    else if(fields[i]=="Quantity"||fields[i]=="Quantity\n") fieldOrder[4]=i;
                    else {
                        std::cout<<i<<"Invalid header name found!\n";
                        return 1;
                    }
                }
                nLines++;
                continue;               
            }
            cliOrd=fields[fieldOrder[0]];
            inst=fields[fieldOrder[1]];
            side=stoi(fields[fieldOrder[2]]);
            qty=stoi(fields[fieldOrder[3]]);
            price=stod(fields[fieldOrder[4]]);

            nLines++;

            int temp=validateAndCreate(cliOrd,inst,side,qty,price);
            if(temp==1)continue;
        }
        fclose(filePointer);
        
	    file.close();
	}

    
    
}


void writeToFile(std::shared_ptr<orderObj> x,int qty,double price){
	if(LOGS_Enabled)std::cout<<"write to file called for following\n"
        <<x->orderId<<","
        <<x->clientOrderID<<","
        <<x->inst<<","
        <<x->side<<","
        <<x->status<<","
        <<qty<<","
        <<price<<" priority "<<x->priority<<"\n";
    
    file<<x->clientOrderID<<","
        <<x->orderId<<","
        <<x->inst<<","
        <<x->side<<","
        <<price<<","
        <<qty<<","
        <<x->status<<","
        <<transac_time();
    if(LOGS_Enabled)file<<" "<<x->priority;
    file<<"\n";
    if(LOGS_Enabled)printOrderBook(x);
    if(LOGS_Enabled)std::cout<<"write to file finished!\n";

}

void writeHeader(){
    file<<"Client Order ID,"
        <<"Order ID,"
        <<"Instrument,"
        <<"Side,"
        <<"Price,"
        <<"Quantity,"
        <<"Status,"
        <<"Transaction Time\n";
    if(LOGS_Enabled)std::cout<<"Writing header finished!\n";
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
        if(LOGS_Enabled)std::cout<<"Error allocating memory for instrument book!\n";
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
    std::shared_ptr<orderObj> temp=std::make_shared<orderObj>(cliOrd, inst, side,qty,price);
    if(temp->status=="Rejected"){
        writeToFile(temp,temp->qty,temp->price);
        //delete(temp);
        if(LOGS_Enabled)std::cout<<"About to get deletd!\n";
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


std::string transac_time() {
  // milliseconds since the epoch
  auto now = std::chrono::system_clock::now();
  std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);
  std::stringstream ss;
  ss << std::put_time(std::localtime(&now_time_t), "%Y%m%d-%H%M%S");
  ss << "." << std::setfill('0') << std::setw(3) << std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count()%1000;
  return ss.str();
}

int minQty(std::shared_ptr<orderObj> x,std::shared_ptr<orderObj>y){
    if(LOGS_Enabled)std::cout<<"minqty called for  "<<x->qty<<" "<<y->qty<<std::endl;
    if(x->qty>y->qty)return y->qty;
    else return x->qty;
}

//OrderObj member functions

orderObj::orderObj(std::string& cliOrd, std::string& inst, int &side,int &qty,double &price){
        if(LOGS_Enabled)std::cout<<"orderObj constructor called for cliorder num "<<cliOrd<<"\n";
        orderId="ord"+std::to_string(++orderNum);
        clientOrderID=cliOrd;
        this->inst=inst;
        this->side=side;
        this->qty=qty;
        this->price=price;
        priority=1;
        status="New";
        if(LOGS_Enabled)std::cout<<"New order object created!\n";
        if(cliOrd.size()==0 || inst.size()==0 )status="Rejected";
        else if(inst!="Rose" &&  inst!="Lavender"&& inst!="Lotus"&& inst!="Tulip"&& inst!="Orchid")status="Rejected";
        else if(price<=0 || qty<10 || qty>1000 || qty%10!=0)status="Rejected";
        else if(side!=1 && side!=2)status="Rejected";
}

int orderObj::executeQty(int qty){
        //doesnt need to check whether it becomes negative since i check that before calling this
        if(LOGS_Enabled)std::cout<<"execute order called for "<<clientOrderID<<std::endl;
        this->qty-=qty;
        this->status="Pfill";
        if(this->qty==0){
            this->status="Fill";
            if(LOGS_Enabled)std::cout<<"Execute order finished!\n";
            return 1;//returns 1 when order is fill
        }
        if(LOGS_Enabled)std::cout<<"Execute order finished!\n";
        return 0;//return 0 when pfill
        
}


// For test logs.................

void printOrderBook(std::shared_ptr<orderObj> temp){
    int i;
    if(temp->inst==std::string("Rose"))i=0;
    else if(temp->inst==std::string("Lavender"))i=1;
    else if(temp->inst==std::string("Lotus"))i=2;
    else if(temp->inst==std::string("Tulip"))i=3;
    else if(temp->inst==std::string("Orchid"))i=4;
    file<<"Buy orders: \n";
    for(auto k=allInstruments[i]->buy.begin();k!=allInstruments[i]->buy.end();k++){
        file<<"\t"
        <<k->second->orderId<<","
        <<k->second->clientOrderID<<","
        <<k->second->inst<<","
        <<k->second->side<<","
        <<k->second->status<<","
        <<k->second->qty<<","
        <<k->second->price<<" priority "<<k->second->priority<<" "<<k->first.second<<"\n";
       
    }
    file<<"Sell orders: \n";
    for(auto k=allInstruments[i]->sell.begin();k!=allInstruments[i]->sell.end();k++){
        file<<"\t"
        <<k->second->orderId<<","
        <<k->second->clientOrderID<<","
        <<k->second->inst<<","
        <<k->second->side<<","
        <<k->second->status<<","
        <<k->second->qty<<","
        <<k->second->price<<" priority "<<k->second->priority<<" "<<k->first.second<<"\n";
        
    }
}

void printfDetails(std::shared_ptr<orderObj> x){
    if(LOGS_Enabled)std::cout<<"print details of follwing\n"<<x->clientOrderID<<","
        <<x->inst<<","
        <<x->side<<","
        <<x->status<<","
        <<x->qty<<","
        <<x->price<<"\n";
}


//Instrument member functions..........


instrument::instrument(){
    buyBegin=buy.begin();
    sellBegin=sell.begin();
}

void instrument::newOrder(std::shared_ptr<orderObj> newObj){
    if(LOGS_Enabled)std::cout<<"Accessing "<<name<<" newOrder method\n";
    auto relBegin = (newObj->side==2)  ? buyBegin : sellBegin;
    if(newObj->side==1&&(sell.empty() || (relBegin->second->price)>newObj->price) ){//this is a buy order and sell is empty or sell orders are higher
        if(LOGS_Enabled)std::cout<<"Entered case 1: is sell.empty-> "<<sell.empty()<<" Buy.empty()-> "<<buy.empty()<<"\n";
        auto samePrice=buyHighestPrio.find(newObj->price);
        if(samePrice!=buyHighestPrio.end()){//there are elements with same price
            auto x=samePrice->second;
            int lastPriority=x->second->priority;
            if(++x!=buy.end()){
                while(x->second->price==newObj->price){
                    lastPriority=x->second->priority;
                    if(++x==buy.end())break;
                    }
            }
            newObj->priority=++lastPriority;
            buy.insert(--x,{std::make_pair(newObj->price,newObj->priority),newObj});
        }else{//there are no elements with same price. so add the element and ad the reference to Highest Priority map
            bool isBuyEmpty=buy.empty();
            auto y=buy.insert({std::make_pair(newObj->price,newObj->priority),newObj});
            buyHighestPrio.emplace(newObj->price,y.first);
            if(isBuyEmpty)buyBegin=y.first;
            else if(newObj->price>buyBegin->second->price)buyBegin=y.first;//if new buy order has higher price, update map begin
        }
        writeToFile(newObj,newObj->qty,newObj->price);
        return;
    }else if(newObj->side==2&&(buy.empty() || (relBegin->second->price)<newObj->price)){
        if(LOGS_Enabled)std::cout<<"Entered case 2: is buy.empty-> "<<buy.empty()<<" sell.empty-> "<<sell.empty()<<"\n";
        auto samePrice=sellHighestPrio.find(newObj->price);
        if(samePrice!=sellHighestPrio.end()){//there are elements with same price
            auto x=samePrice->second;
            int lastPriority=x->second->priority;
            if(++x!=sell.end()){
                while(x->second->price==newObj->price){
                    lastPriority=x->second->priority;
                    if(++x==sell.end())break;
                    }
            }
            newObj->priority=++lastPriority;
            sell.insert(--x,{std::make_pair(newObj->price,newObj->priority),newObj});

        }else{//there are no elements with same price. so add the element and ad the reference to Highest Priority map
            bool isSellEmpty=sell.empty();
            auto y=sell.insert({std::make_pair(newObj->price,newObj->priority),newObj});
            sellHighestPrio.emplace(newObj->price,y.first);
            if(isSellEmpty)sellBegin=y.first;
            else if (newObj->price<sellBegin->second->price)sellBegin=y.first;//if new sell order has lower price, update map begin
        }
        writeToFile(newObj,newObj->qty,newObj->price);
        return;
    }else if(newObj->side==1){//this is a buy order and theres a sell order for equal or low price
        if(LOGS_Enabled)std::cout<<"inside side==1 but sell is not empty"<<std::endl;
        if(LOGS_Enabled)std::cout<<"Orders inside are "<<newObj->orderId<<" "<<relBegin->second->orderId;
        while(!sell.empty() && (relBegin->second->price)<=newObj->price){
            int execQty=minQty(newObj,relBegin->second);
            printfDetails(relBegin->second);
            if(LOGS_Enabled)std::cout<<"excec qty "<<execQty<<std::endl;
            int isFill1=newObj->executeQty(execQty);
            
            writeToFile(newObj,execQty,relBegin->second->price);
            int isFill2=relBegin->second->executeQty(execQty);
            writeToFile(relBegin->second,execQty,relBegin->second->price);
            
            if(isFill2){
                std::shared_ptr<orderObj> itr=relBegin->second;
                relBegin++;
                sellBegin=relBegin;
                if(relBegin!=sell.end()){//updating new priority
                    if(relBegin->second->price==itr->price)sellHighestPrio[itr->price]=relBegin;
                    else sellHighestPrio.erase(itr->price);//when next elemt is not same price
                }else sellHighestPrio.erase(itr->price);


                sell.erase(std::make_pair(itr->price,itr->priority));
                
                if(LOGS_Enabled)std::cout<<"About to be deleted!\n";
            }
            if(isFill1){
                //if(LOGS_Enabled)std::cout<<"Deleting order "<<newObj->clientOrderID<<std::endl;
                //delete newObj;
                if(LOGS_Enabled)std::cout<<"about to get deleted\n";
                return;
            }
            if(!sell.empty()) relBegin=sellBegin;
            else break;
            
        }
        // //when aggressive order is not fully completed
        // buy.emplace(std::make_pair(newObj->price,newObj->priority),newObj);
        bool isBuyEmpty=buy.empty();
        auto y=buy.insert({std::make_pair(newObj->price,newObj->priority),newObj});
        if(isBuyEmpty)buyBegin=y.first;
        else if(newObj->price>buyBegin->second->price)buyBegin=y.first;
        buyHighestPrio.emplace(newObj->price,y.first);
        

    }else if(newObj->side==2){//this is a sell order and theres a buy order for equal or high price
        if(LOGS_Enabled)std::cout<<"inside side==2 but buy is not empty"<<std::endl;
        while(!buy.empty() && (relBegin->second->price)>=newObj->price){
        //while(!buy.empty() && (buy.begin()->second->price)>=newObj->price){    
            if(LOGS_Enabled)std::cout<<"inside while loop\n";
            int execQty=minQty(newObj,relBegin->second);
            int isFill1=newObj->executeQty(execQty);
            writeToFile(newObj,execQty,relBegin->second->price);
            int isFill2=relBegin->second->executeQty(execQty);
            writeToFile(relBegin->second,execQty,relBegin->second->price);
            
            if(isFill2){
                std::shared_ptr<orderObj> itr=relBegin->second;
                relBegin++;
                buyBegin=relBegin;
                if(relBegin!=buy.end()){//updating new priority
                    if(relBegin->second->price==itr->price)buyHighestPrio[itr->price]=relBegin;
                    else buyHighestPrio.erase(itr->price);//when next elemt is not same price
                }else buyHighestPrio.erase(itr->price);

                buy.erase(std::make_pair(itr->price,itr->priority));
                
                if(LOGS_Enabled)std::cout<<"About to get deletd!\n";
                
            }

            if(isFill1){
                //if(LOGS_Enabled)std::cout<<"Deleting order "<<newObj->clientOrderID<<std::endl;
                //delete newObj;
                if(LOGS_Enabled)std::cout<<"About to get deleted!\n";
                return;
            }
            if(!buy.empty()) relBegin=buyBegin;
            else break;
        }
        //when aggressive order is not fully completed
        // sell.emplace(std::make_pair(newObj->price,newObj->priority),newObj);
        bool isSellEmpty=sell.empty();
        auto y=sell.insert({std::make_pair(newObj->price,newObj->priority),newObj});
        if(isSellEmpty)sellBegin=y.first;
        else if(newObj->price<sellBegin->second->price)sellBegin=y.first;
        sellHighestPrio.emplace(newObj->price,y.first);   
    }    
}


