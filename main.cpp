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
    
};
void writeToFile(orderObj*,int,double);


void initializeInstrumentArray();
int initializeIns(int);
int validateAndCreate(std::string &cliOrd, std::string &inst, int &side,int &qty,double &price);

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

instrument* allInstruments[5];

int main(){
    initializeInstrumentArray();
    file.open("execution_rep.csv",std::ios_base::app);//to append
    char cliOrd[7],inst[8];
    int side,qty;
    double price;
	if (FILE* filePointer = fopen("orders.csv", "r")) {
        std::cout<<"hi"<<std::endl;
		while (fscanf(filePointer, "%[^,],%[^,],%d,%d,%lf", cliOrd, inst, &side,&qty,&price)==5) {
            std::cout<<cliOrd<<inst<<side<<qty<<price<<std::endl;
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