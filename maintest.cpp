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
#define LOGS_Enabled 1//to enable my test logs
static int orderNum=0;
std::ofstream file;
typedef std::pair<double,int> pair;
double epsilon=0.001;
// class own_double_less : public std::binary_function<double,double,bool>
// {
// public:
//   own_double_less( double arg_ = 0.001 ) : epsilon(arg_) {}
//   bool operator()( const double &left, const double &right  ) const
//   {

//     // you can choose other way to make decision
//     // (The original version is: return left < right;) 
//     return (abs(left - right) > epsilon) && (left < right);
//   }
//   double epsilon;
// };


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
    std::string orderId;
    std::string clientOrderID;
    std::string inst;
    int side;
    double price;
    int qty;
    int priority;
    std::string status;
    orderObj(std::string& cliOrd, std::string& inst, int &side,int &qty,double &price){
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
    int executeQty(int qty){
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
    ~orderObj(){
        if(LOGS_Enabled)std::cout<<"OrderObj destructor called for ordNum: "<<orderId<<"\n";
    }
    
};
void printOrderBook(std::shared_ptr<orderObj>);
void writeToFile(std::shared_ptr<orderObj>,int,double);
void writeHeader();
std::string transac_time();
void initializeInstrumentArray();
int initializeIns(int);
int validateAndCreate(std::string &cliOrd, std::string &inst, int &side,int &qty,double &price);
void printfDetails(std::shared_ptr<orderObj> x){
    if(LOGS_Enabled)std::cout<<"print details of follwing\n"<<x->clientOrderID<<","
        <<x->inst<<","
        <<x->side<<","
        <<x->status<<","
        <<x->qty<<","
        <<x->price<<"\n";
}

int minQty(std::shared_ptr<orderObj> x,std::shared_ptr<orderObj>y){
    if(LOGS_Enabled)std::cout<<"minqty called for  "<<x->qty<<" "<<y->qty<<std::endl;
    if(x->qty>y->qty)return y->qty;
    else return x->qty;
}
class instrument{
public:
    std::string name;
    std::map<const pair,std::shared_ptr<orderObj>,compareOrder> buy;//descending order
    std::unordered_map<double,std::pair<pair,std::shared_ptr<orderObj>>> buyHighestPrio;
    std::map<const pair,std::shared_ptr<orderObj>> sell;
    std::unordered_map<double,std::pair<pair,std::shared_ptr<orderObj>>> sellHighestPrio;




    void newOrder(std::shared_ptr<orderObj> newObj){
        if(LOGS_Enabled)std::cout<<"Accessing "<<name<<" newOrder method\n";
        auto relBegin = (newObj->side==2)  ? buy.begin() : sell.begin();
        if(newObj->side==1&&(sell.empty() || (relBegin->second->price)>newObj->price) ){
            if(LOGS_Enabled)std::cout<<"Entered case 1: is sell.empty-> "<<sell.empty()<<" Buy.empty()-> "<<buy.empty()<<"\n";
            auto samePrice=buyHighestPrio.find(newObj->price);
            if(samePrice!=buyHighestPrio.end()){//there are elements with same price
                auto x=buy.find(std::make_pair(newObj->price,samePrice->second.first.second));//finding the rel elemtn from map with its key's priority num
                int i=1;
                while((++x)->second->price==newObj->price)i++;
                newObj->priority=++i;
                buy.insert(x,{std::make_pair(newObj->price,newObj->priority),newObj});
            }else{//there are no elements with same price. so add the element and ad the reference to Highest Priority map
            
                auto y=buy.insert({std::make_pair(newObj->price,newObj->priority),newObj});
                buyHighestPrio.emplace(newObj->price,y.first);
            }
            writeToFile(newObj,newObj->qty,newObj->price);
            return;
            // if(!buy.empty()){
            //     if(LOGS_Enabled)std::cout<<"inside case 1-> buy is not empty \n";
            //     if(newObj->price==buy.begin()->second->price)std::cout<<"they are equal\n";
            //     auto x=buy.find(std::make_pair(newObj->price,newObj->priority));
            //     if(x!=buy.end()){
            //             if(LOGS_Enabled)std::cout<<"inside case 1-> buy is not empty-> theres element with same price \n";
            //             int i=1;
            //             while((++x)->second->price==newObj->price)i++;
            //             newObj->priority=++i;
            //     }
            // }
            // buy.emplace(std::make_pair(newObj->price,newObj->priority),newObj);
            
        }else if(newObj->side==2&&(buy.empty() || (relBegin->second->price)<newObj->price)){
            if(LOGS_Enabled)std::cout<<"Entered case 2: is buy.empty-> "<<buy.empty()<<" sell.empty-> "<<sell.empty()<<"\n";
            auto samePrice=sellHighestPrio.find(newObj->price);
            if(samePrice!=sellHighestPrio.end()){//there are elements with same price
                auto x=sell.find(std::make_pair(newObj->price,samePrice->second.first.second));//finding the rel elemtn from map with its key's priority num
                int i=1;
                while((++x)->second->price==newObj->price)i++;
                newObj->priority=++i;
                sell.insert(x,{std::make_pair(newObj->price,newObj->priority),newObj});
            }else{//there are no elements with same price. so add the element and ad the reference to Highest Priority map
                auto y=sell.insert({std::make_pair(newObj->price,newObj->priority),newObj});
                sellHighestPrio.emplace(newObj->price,y.first);
            }
            writeToFile(newObj,newObj->qty,newObj->price);
            return;
            // if(!sell.empty()){
            //     if(LOGS_Enabled)std::cout<<"inside case 2-> sell is not empty "<<newObj->orderId<<" "<<newObj->price<<" "<<newObj->priority<<"\n";
            //     auto x=sell.find(std::make_pair(newObj->price,newObj->priority));
            //     if(LOGS_Enabled)std::cout<<"Did i found it: "<<sell.count(std::make_pair(newObj->price,newObj->priority))<<"\n";
            //     if(LOGS_Enabled)std::cout<<sell.begin()->second->orderId<<" "<<sell.begin()->second->price<<" "<<sell.begin()->second->priority<<"\n";
            //     if(x!=sell.end()){
            //         if(LOGS_Enabled)std::cout<<"inside case 2-> sell is not empty-> theres element with same price \n";
            //         int i=1;
            //         while(++x!=sell.end()){
            //             if(x->second->price==newObj->price)i++;
            //         }
            //         newObj->priority=++i;
            //     }
            // }
            // sell.emplace(std::make_pair(newObj->price,newObj->priority),newObj);
            // writeToFile(newObj,newObj->qty,newObj->price);
            // return;
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
                    if(relBegin!=sell.end()){//updating new priority
                        if(relBegin->second->price==itr->price)sellHighestPrio.emplace(relBegin->second->price,relBegin);
                        else sellHighestPrio.erase(itr->price);//when next elemt is not same price
                    }else sellHighestPrio.erase(itr->price);


                    sell.erase(std::make_pair(itr->price,itr->priority));
                    // if(!sell.empty()){
                    //     auto temp=sell.begin();
                    //     while(temp->second->price==itr->price){
                    //         temp->second->priority--;
                    //         if(++temp==sell.end())break;
                    //     }
                    // }
                    //if(LOGS_Enabled)std::cout<<"Deleting order "<<itr->clientOrderID<<std::endl;
                    //delete itr;
                    if(LOGS_Enabled)std::cout<<"About to be deleted!\n";
                }
                if(isFill1){
                    //if(LOGS_Enabled)std::cout<<"Deleting order "<<newObj->clientOrderID<<std::endl;
                    //delete newObj;
                    if(LOGS_Enabled)std::cout<<"about to get deleted\n";
                    return;
                }
                if(!sell.empty()) relBegin=sell.begin();
                else break;
                
            }
            // //when aggressive order is not fully completed
            // buy.emplace(std::make_pair(newObj->price,newObj->priority),newObj);
            auto y=buy.insert({std::make_pair(newObj->price,newObj->priority),newObj});
            buyHighestPrio.emplace(newObj->price,y.first);
            

        }else if(newObj->side==2){//this is a sell order and theres a but order for equal or low price
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
                    if(relBegin!=buy.end()){//updating new priority
                        if(relBegin->second->price==itr->price)buyHighestPrio.emplace(relBegin->second->price,relBegin);
                        else buyHighestPrio.erase(itr->price);//when next elemt is not same price
                    }else buyHighestPrio.erase(itr->price);

                    buy.erase(std::make_pair(itr->price,itr->priority));
                    // if(!buy.empty()){
                    //     auto temp=buy.begin();
                    //     while(temp->second->price==itr->price){
                    //         temp->second->priority--;
                    //         if(++temp==buy.end())break;
                    //     }
                    // }
                    //if(LOGS_Enabled)std::cout<<"Deleting order "<<itr->clientOrderID<<std::endl;
                    //delete itr;
                    if(LOGS_Enabled)std::cout<<"About to get deletd!\n";
                    
                }

                if(isFill1){
                    //if(LOGS_Enabled)std::cout<<"Deleting order "<<newObj->clientOrderID<<std::endl;
                    //delete newObj;
                    if(LOGS_Enabled)std::cout<<"About to get deleted!\n";
                    return;
                }
                if(!buy.empty()) relBegin=buy.begin();
                else break;
            }
            //when aggressive order is not fully completed
            // sell.emplace(std::make_pair(newObj->price,newObj->priority),newObj);
            auto y=sell.insert({std::make_pair(newObj->price,newObj->priority),newObj});
            sellHighestPrio.emplace(newObj->price,y.first);
           
        }

        
    }
};

instrument* allInstruments[5];

/*
int main(){
    initializeInstrumentArray();
    file.open("execution_rep.csv",std::ios_base::app);//to append
    file<<std::fixed<<std::setprecision(2);
    char cliOrd[8]={'\0'},inst[9]={'\0'};
    int side,qty;
    double price;
	if (FILE* filePointer = fopen("orders.csv", "r")) {
        int n;
		while (1) {
            fscanf(filePointer, "%[^,],%[^,],%d,%d,%lf\n", cliOrd, inst, &side,&qty,&price);
            std::cout<<"read orders: "<<cliOrd<<" "<<inst<<std::endl;
            //std::cout<<"read order"<<std::endl;
            std::cout<<"Read folowing details:"<<cliOrd<<" "<<inst<<" "<<side<<" "<<qty<<" "<<price<<std::endl;
            std::string cliOrdString=std::string(cliOrd);
            std::string instString=std::string(inst);
            int temp=validateAndCreate(cliOrdString,instString,side,qty,price);
            if(temp==1)continue;

		}
        fclose(filePointer);
        
	    file.close();
	}

    
    
}
*/

// char* field = strtok(line, ",");
//             nthField=0;

//             while (field) {
                
//                 //std::cout<<bb<<" ";
//                 switch(nthField){
//                     case 0: cliOrd=std::string(field); break;
//                     case 1: inst=std::string(field); break;
//                     case 2: side=atoi(field); break;
//                     case 3: qty=atoi(field); break;
//                     case 4: price=atof(field); break;

//                 }
                
//                 field = strtok(NULL, ",");
//                 nthField=(nthField+1)%5;
                
//             }
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
        //int nthField=0;
        fgets(linechar, 1024, filePointer);//removes header
		while (fgets(linechar, 1024, filePointer)) {
            std::string line=std::string(linechar);
            std::regex field_regex(R"(([^,]*),?)");
            std::vector<std::string> fields;
            auto field_begin = std::sregex_iterator(line.begin(), line.end(), field_regex);
            auto field_end = std::sregex_iterator();
            for (auto i = field_begin; i != field_end; ++i) {
                fields.push_back(i->str(1));
            }
            cliOrd=fields[0];
            inst=fields[1];
            side=stoi(fields[2]);
            qty=stoi(fields[3]);
            price=stod(fields[4]);

            int temp=validateAndCreate(cliOrd,inst,side,qty,price);
            if(temp==1)continue;
        }
        //testing11111111111111
        auto itr=allInstruments[0]->buy.begin();
        file<<"npppppppp" <<allInstruments[0]->buy[std::make_pair(55.00,1)]->orderId<<"\n";
        
        // file<<"npppppppp" <<allInstruments[0]->buy[std::make_pair(55.00,1)]->orderId<<"\n"
        
         std::cout<<"begin "<<(itr)->first.first<<" "<<(itr)->first.second<<" "<<itr->second->orderId<<"\n";
         itr++;
         std::cout<<"begin "<<(itr)->first.first<<" "<<(itr)->first.second<<" "<<itr->second->orderId<<"\n";
         auto isFound=allInstruments[0]->buy.find(std::make_pair(itr->second->price,itr->second->qty));
        if(isFound!=allInstruments[0]->buy.end()){
            file<<"found : "<<isFound->second->orderId<<" "<<isFound->first.first<<"\n";
        }else file<<"Not found!\n";
        //testing11111111111111
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
        //<<x->priority;//remove this later
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

        //<<x->priority;//remove this later
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
  ss << "." << std::setfill('0') << std::setw(3) << std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() % 1000;
  return ss.str();
}
void printOrderBook(std::shared_ptr<orderObj> temp){//for test logs
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