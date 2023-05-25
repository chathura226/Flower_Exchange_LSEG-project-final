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
#include<deque>
#include <chrono>
#include<mutex>
#include<unordered_map>
#include<condition_variable>
#include<boost/circular_buffer.hpp>
#define LOGS_Enabled 0//to enable my test logs
#define bufSize 20000//buffer size 
std::ofstream file;//for writing execution report
//below are for critical section control when treading
std::mutex mtx;
std::condition_variable bufFull, bufEmpty;
bool finished = 0;

struct bufOrder {//structure that includes order details in the buffer before creating order objects
    std::string cliOrd;
    std::string inst;
    int side;
    int qty;
    double price;
    bufOrder() {
        cliOrd = "";
        inst = "";
        side = -1;
        qty = -1;
        price = -1;
    }
    bufOrder(const bufOrder& temp) {
        cliOrd = temp.cliOrd;
        inst = temp.inst;
        side = temp.side;
        qty = temp.qty;
        price = temp.price;
    }
    bufOrder(std::string a, std::string b, int c, double d,int e) {
        cliOrd = a;
        inst = b;
        side = c;
        price = d;
        qty = e;
        
    }

};

boost::circular_buffer<bufOrder> buffer(bufSize);//fixed size circular buffer to exchange order between threads
int bufCount = 0;//no of items in the buffer

class OrderExec;//super class for order execution method
class sellOrderExec;//sub class of OrderExec for sell order execution method
class buyOrderExec;//sub class of OrderExec for buy order execution method
class instrument;//class that have structure for each instrument orderbook type

class orderObj {//class of each order object
public:
    static int orderNum;//generate number for the new orders
    std::string orderId;
    int side;
    std::string status;
private:
    friend void writeToFile(std::shared_ptr<orderObj>, int, double);
    friend sellOrderExec;
    friend buyOrderExec;
    friend int minQty(std::shared_ptr<orderObj> x, std::shared_ptr<orderObj>y);
    friend void printfDetails(std::shared_ptr<orderObj> x);//for test logs

    std::string clientOrderID;
    std::string inst;
    double price;
    int qty;
    int executeQty(int qty);
public:
    orderObj() {//default constructor - won't be called unless theres an error
        orderId=-1;
        clientOrderID = "";
        this->inst = "";
        this->side = -1;
        this->qty = -1;
        this->price = -1;
        status = "Rejected";
    }
    orderObj(std::string& cliOrd, std::string& inst, int& side, int& qty, double& price);//parameterized constructor - this also has the logic to validate order
    ~orderObj() {
        if (LOGS_Enabled)std::cout << "OrderObj destructor called for ordNum: " << orderId << "\n";
    }
};
int orderObj::orderNum = 0;
typedef std::deque<std::shared_ptr<orderObj>> OrderPtrDeque;//typedef for deque for maps



void writeToFile(std::shared_ptr<orderObj>, int, double);//writing to execution report
void writeHeader();//writing header of execution report
std::string transac_time();//calculating transaction time
void initializeInstrumentArray();//initializing insturmnt orderbook array
int initializeIns(int);//initialiizing corresponding index of instrument orderbook array
int validateAndCreate(std::string& cliOrd, std::string& inst, int& side, int& qty, double& price);//creating orderobj and validating
void printfDetails(std::shared_ptr<orderObj> x);//for test logs
int minQty(std::shared_ptr<orderObj> x, std::shared_ptr<orderObj>y);//compare qtys and find the correct qty

class instrument {//class for instrument orderbook
public:
    std::string name;//name of instrument
private:
    std::map<const double, double, std::greater<double>> buy;//descending order -ordered map to keep sorted prices of buy orders
    std::unordered_map<double, OrderPtrDeque> buyHash;//hash map that keeps relevent deques of prices for buy orders
    std::map<const double, double> sell;//ordered map to keep sorted prices of buy orders
    std::unordered_map<double, OrderPtrDeque> sellHash;//hash map that keeps relevent deques of prices for sell orders
    std::map<const double, double>::iterator buyBegin;//iterator that keeps begining of ordered map of buy orders - to reducing time by not calclating everytime
    std::map<const double, double>::iterator sellBegin;//iterator that keeps begining of ordered map of sell orders - to reducing time by not calclating everytime
    friend sellOrderExec;
    friend buyOrderExec;
public:
    instrument();
};

class OrderExec {//super class for sellOrderExec and buyOrderExec
protected:
    double price;
    std::map<const double, double>::iterator relBegin;
    virtual void insertOrder(std::shared_ptr<orderObj>newObj, instrument& ins) = 0;
    virtual void execTransac(std::shared_ptr<orderObj>newObj, instrument& ins) = 0;
public:
    virtual void exec(std::shared_ptr<orderObj>newObj, instrument& ins) = 0;
};

class sellOrderExec :public OrderExec {//class for sell order executions
protected:
    void insertOrder(std::shared_ptr<orderObj>newObj, instrument& ins) override;
    void execTransac(std::shared_ptr<orderObj>newObj, instrument& ins) override;
public:
    void exec(std::shared_ptr<orderObj>newObj, instrument& ins) override;
};

class buyOrderExec : public OrderExec {//class for buy order executions
protected:
    void insertOrder(std::shared_ptr<orderObj>newObj, instrument& ins) override;
    void execTransac(std::shared_ptr<orderObj>newObj, instrument& ins) override;
public:
    void exec(std::shared_ptr<orderObj>newObj, instrument& ins) override;
};

buyOrderExec buyOrderExecObj;//handles all types of buy order executions
sellOrderExec sellOrderExecObj;//handles all types of sell order executions

instrument* allInstruments[5];//pointer array for order books

void driveLogic();//pulling from buffer and validate and execute
int readFile();//reading from file and pushing to the buffer


int main() {
    initializeInstrumentArray();
    file.open("execution_rep.csv", std::ios_base::app);//to append
    file << std::fixed << std::setprecision(2);//to have only upto two decimal points
    writeHeader();
    //multithreading
    std::thread t1(readFile);
    std::thread t2(driveLogic);
    t1.join();
    t2.join();

    file.close();
}

int readFile() {//reading from file and pushing to the buffer
    std::string cliOrd = "", inst = "";
    int side = -1, qty = -1;
    double price = -1;
    if (FILE* filePointer = fopen("orders.csv", "r")) {
        char linechar[1024];

        int fieldOrder[] = { 0,1,2,3,4 };
        int nLines = 0;
        std::string line;
        while (fgets(linechar, 1024, filePointer)) {

            line = std::string(linechar);
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
            if (nLines == 0) {//first line of the file - headers and work according to custome header field order
                for (int k = 0; k < 5; k++) {
                    if (fields[k] == "Client Order ID" || fields[k] == "Client Order ID\n") fieldOrder[0] = k;
                    else if (fields[k] == "Instrument" || fields[k] == "Instrument\n") fieldOrder[1] = k;
                    else if (fields[k] == "Side" || fields[k] == "Side\n") fieldOrder[2] = k;
                    else if (fields[k] == "Price" || fields[k] == "Price\n") fieldOrder[3] = k;
                    else if (fields[k] == "Quantity" || fields[k] == "Quantity\n") fieldOrder[4] = k;
                    else {
                        std::cout << k << "Invalid header name found!\n";
                        return 1;
                    }
                }
                
                if (LOGS_Enabled)for (int k = 0; k < 5; k++)std::cout << fieldOrder[k];
                if (LOGS_Enabled)std::cout << std::endl;

                nLines++;
                continue;
            }
            std::unique_lock<std::mutex> bufWriteLock(mtx);
            bufFull.wait(bufWriteLock, [] {return bufCount != bufSize; });
            buffer.push_back(bufOrder(fields[fieldOrder[0]], fields[fieldOrder[1]], stoi(fields[fieldOrder[2]]), stoi(fields[fieldOrder[3]]), stod(fields[fieldOrder[4]])));
            bufCount++;
            bufWriteLock.unlock();
            bufEmpty.notify_one();
            nLines++;
        }
        finished = true;
        fclose(filePointer);
    }
}

void driveLogic() {//pulling from buffer and validate and execute
    bufOrder temp;
    while (!finished || bufCount != 0) {
        std::unique_lock<std::mutex> bufReadLock(mtx);
        if (!finished) bufEmpty.wait(bufReadLock, [] {return (bufCount != 0 || finished); });
        temp = buffer.front();
        buffer.pop_front();
        bufCount--;
        bufReadLock.unlock();
        bufFull.notify_one();
        validateAndCreate(temp.cliOrd, temp.inst, temp.side, temp.qty, temp.price);

    }
}

void writeToFile(std::shared_ptr<orderObj> x, int qty, double price) {
    //for testing ...............................................................................
    if (LOGS_Enabled)std::cout << "write to file called for following\n"
        << x->orderId << ","
        << x->clientOrderID << ","
        << x->inst << ","
        << x->side << ","
        << x->status << ","
        << qty << ","
        << price << "\n";
    //..........................................................................................
    file << x->clientOrderID << ","
        << x->orderId << ","
        << x->inst << ","
        << x->side << ","
        << price << ","
        << qty << ","
        << x->status << ","
        << transac_time() << "\n";

    if (LOGS_Enabled)std::cout << "write to file finished!\n";

}

void writeHeader() {
    file << "Client Order ID,"
        << "Order ID,"
        << "Instrument,"
        << "Side,"
        << "Price,"
        << "Quantity,"
        << "Status,"
        << "Transaction Time\n";
    if (LOGS_Enabled)std::cout << "Writing header finished!\n";
}
void initializeInstrumentArray() {
    allInstruments[0] = NULL;
    allInstruments[1] = NULL;
    allInstruments[2] = NULL;
    allInstruments[3] = NULL;
    allInstruments[4] = NULL;
}

int initializeIns(int i) {
    instrument* temp = new instrument;
    if (temp == NULL) {
        if (LOGS_Enabled)std::cout << "Error allocating memory for instrument book!\n";
        return 1;
    }
    allInstruments[i] = temp;

    switch (i) {
    case 0: temp->name = "Rose"; break;
    case 1: temp->name = "Lavender"; break;
    case 2: temp->name = "Lotus"; break;
    case 3: temp->name = "Tulip"; break;
    case 4: temp->name = "Orchid"; break;
    }
    if (LOGS_Enabled) std::cout << "Initialized the book for " << temp->name << "\n";
    return 0;

}

int validateAndCreate(std::string& cliOrd, std::string& inst, int& side, int& qty, double& price) {
    std::shared_ptr<orderObj> temp = std::make_shared<orderObj>(cliOrd, inst, side, qty, price);
    if (temp->status == "Rejected") {
        writeToFile(temp, qty, price);
        if (LOGS_Enabled)std::cout << "About to get deletd!\n";
        return 1;
    }
    //finding the corresponding index number of instrument order book
    int i = 0;
    if (inst == std::string("Rose"))i = 0;
    else if (inst == std::string("Lavender"))i = 1;
    else if (inst == std::string("Lotus"))i = 2;
    else if (inst == std::string("Tulip"))i = 3;
    else if (inst == std::string("Orchid"))i = 4;

    if (allInstruments[i] == NULL)initializeIns(i);
    if (side == 1)buyOrderExecObj.exec(temp, *allInstruments[i]);
    else sellOrderExecObj.exec(temp, *allInstruments[i]);
    
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

int minQty(std::shared_ptr<orderObj> x, std::shared_ptr<orderObj>y) {//comparing two order object's qty to find min qty
    if (LOGS_Enabled)std::cout << "minqty called for  " << x->qty << " " << y->qty << std::endl;
    if (x->qty > y->qty)return y->qty;
    else return x->qty;
}





//OrderObj member functions.....................................................................................................................................

orderObj::orderObj(std::string& cliOrd, std::string& inst, int& side, int& qty, double& price) {//logic for order rejection also implemented here
    if (LOGS_Enabled)std::cout << "orderObj constructor called for cliorder num " << cliOrd << "\n";
    orderId = "ord" + std::to_string(++orderNum);
    clientOrderID = cliOrd;
    this->inst = inst;
    this->side = side;
    this->qty = qty;
    this->price = price;
    status = "New";
    if (LOGS_Enabled)std::cout << "New order object created!\n";
    if (cliOrd.size() == 0 || inst.size() == 0)status = "Rejected";
    else if (inst != "Rose" && inst != "Lavender" && inst != "Lotus" && inst != "Tulip" && inst != "Orchid")status = "Rejected";
    else if (price <= 0 || qty < 10 || qty>1000 || qty % 10 != 0)status = "Rejected";
    else if (side != 1 && side != 2)status = "Rejected";
}

int orderObj::executeQty(int qty) {
    //doesnt need to check whether it becomes negative since i check that before calling this
    if (LOGS_Enabled)std::cout << "execute order called for " << clientOrderID << std::endl;
    this->qty -= qty;
    this->status = "Pfill";
    if (this->qty == 0) {
        this->status = "Fill";
        if (LOGS_Enabled)std::cout << "Execute order finished with fill!\n";
        return 1;//returns 1 when order is fill
    }
    if (LOGS_Enabled)std::cout << "Execute order finished with pFill!\n";
    return 0;//return 0 when pfill

}


// For test logs.................
void printfDetails(std::shared_ptr<orderObj> x) {
    if (LOGS_Enabled)std::cout << "print details of follwing\n" << x->clientOrderID << ","
        << x->inst << ","
        << x->side << ","
        << x->status << ","
        << x->qty << ","
        << x->price << "\n";
}


//Instrument member functions......................................................................................................................


instrument::instrument() {
    buyBegin = buy.begin();
    sellBegin = sell.begin();
}



//SellOrderExec member function defiitions..........................................................................................................

void sellOrderExec::insertOrder(std::shared_ptr<orderObj>newObj, instrument& ins) {
    if (LOGS_Enabled)std::cout << "Entered case 2: is buy.empty-> " << ins.buy.empty() << " sell.empty-> " << ins.sell.empty() << "\n";
    auto samePrice = ins.sellHash.find(price);
    if (samePrice != ins.sellHash.end()) {//there are elements with same price
        if (LOGS_Enabled)std::cout << "There are elements with same price\n";
        if (LOGS_Enabled)std::cout << "deque size: " << samePrice->second.size() << "\n";
        samePrice->second.push_back(newObj);
    }
    else {//there are no elements with same price. so add the element and ad the reference to Highest Priority map
        bool isSellEmpty = ins.sell.empty();
        auto y = ins.sell.insert({ price,price });
        ins.sellHash[price].push_back(newObj);
        if (LOGS_Enabled)std::cout << "deque is created for the price : " << price << std::endl;
        if (isSellEmpty)ins.sellBegin = y.first;
        else if (price < ins.sellBegin->second)ins.sellBegin = y.first;//if new sell order has lower price, update map begin

    }
    writeToFile(newObj, newObj->qty, newObj->price);
    return;

}
void sellOrderExec::execTransac(std::shared_ptr<orderObj>newObj, instrument& ins) {
    double relBeginPrice = relBegin->second;
    if (LOGS_Enabled)std::cout << "inside side==2 but buy is not empty" << std::endl;
    while (!ins.buy.empty() && (relBeginPrice) >= newObj->price) {

        std::shared_ptr<orderObj> relObj = ins.buyHash[relBeginPrice].front();//object that gonna execute with new order
        if (LOGS_Enabled)std::cout << "inside while loop\n";
        int execQty = minQty(newObj, relObj);
        if (LOGS_Enabled)printfDetails(relObj);
        if (LOGS_Enabled)std::cout << "excec qty " << execQty << std::endl;
        int isFill1 = newObj->executeQty(execQty);

        writeToFile(newObj, execQty, relBeginPrice);
        int isFill2 = relObj->executeQty(execQty);
        writeToFile(relObj, execQty, relBeginPrice);

        if (isFill2) {//if relevent object from the orderbook fully executed

            ins.buyHash[relBeginPrice].pop_front();
            if (ins.buyHash[relBeginPrice].empty()) {//queue is empty->so delete it from tree and hash map

                relBegin++;
                ins.buyBegin = relBegin;//update new begining of tree
                ins.buyHash.erase(relBeginPrice);
                ins.buy.erase(relBeginPrice);
            }//else -> there are elements with same price->same relbegin         
            if (LOGS_Enabled)std::cout << "About to be deleted!\n";
        }

        if (isFill1) {//if new order object fully executed
            //dont have to delete manually since its a shared pointer
            if (LOGS_Enabled)std::cout << "About to get deleted!\n";
            return;
        }
        if (!ins.buy.empty()) {
            relBegin = ins.buyBegin;
            relBeginPrice = relBegin->second;
        }
        else break;
    }

    // //when aggressive order is not fully completed->add to hash map and tree 
    //there cannot be orders with same price- since they would have executed before this. so just insert new one newly
    bool isSellEmpty = ins.sell.empty();
    auto y = ins.sell.insert({ price,price });
    ins.sellHash[price].push_back(newObj);
    if (LOGS_Enabled)std::cout << "deque is created for the price : " << price << std::endl;
    if (isSellEmpty)ins.sellBegin = y.first;
    else if (price > ins.sellBegin->second)ins.sellBegin = y.first;

}


void sellOrderExec::exec(std::shared_ptr<orderObj>newObj, instrument& ins) {
    relBegin = ins.sellBegin;
    price = newObj->price;
    if (ins.buy.empty() || (relBegin->first) < price)insertOrder(newObj, ins); //this is a sell order and buy is empty or buy orders are lower
    else execTransac(newObj, ins);//this is a sell order and theres a matching order in buy
}



//buyOrderExec member function defiitions..........................................................................................................

void buyOrderExec::insertOrder(std::shared_ptr<orderObj>newObj, instrument& ins)  {
    if (LOGS_Enabled)std::cout << "Entered case 1: is sell.empty-> " << ins.sell.empty() << " Buy.empty()-> " << ins.buy.empty() << "\n";
    auto samePrice = ins.buyHash.find(price);
    if (samePrice != ins.buyHash.end()) {//there are elements with same price
        if (LOGS_Enabled)std::cout << "There are elements with same price\n";
        if (LOGS_Enabled)std::cout << "deque size: " << samePrice->second.size() << "\n";
        samePrice->second.push_back(newObj);

    }
    else {//there are no elements with same price. so add the element and ad the reference to Highest Priority map
        if (LOGS_Enabled)std::cout << "There are no elements with same price\n";
        bool isBuyEmpty = ins.buy.empty();//checking whether the map is empty before insert
        auto y = ins.buy.insert({ price,price });
        ins.buyHash[price].push_back(newObj);
        if (LOGS_Enabled)std::cout << "deque is created for the price : " << price << std::endl;
        if (isBuyEmpty)ins.buyBegin = y.first;//if map was empty before insert, newOrder is the begin
        else if (price > ins.buyBegin->second)ins.buyBegin = y.first;//if new buy order has higher price, update map begin
    }
    writeToFile(newObj, newObj->qty, price);
    return;
}
void buyOrderExec::execTransac(std::shared_ptr<orderObj>newObj, instrument& ins) {
    double relBeginPrice = relBegin->second;
    if (LOGS_Enabled)std::cout << "inside side==1 but sell is not empty" << std::endl;
    while (!ins.sell.empty() && (relBeginPrice) <= price) {
        std::shared_ptr<orderObj> relObj = ins.sellHash[relBeginPrice].front();//object that gonna execute with new order
        int execQty = minQty(newObj, relObj);
        if (LOGS_Enabled)printfDetails(relObj);
        if (LOGS_Enabled)std::cout << "excec qty " << execQty << std::endl;
        int isFill1 = newObj->executeQty(execQty);

        writeToFile(newObj, execQty, relBeginPrice);
        int isFill2 = relObj->executeQty(execQty);
        writeToFile(relObj, execQty, relBeginPrice);

        if (isFill2) {//if relevent object from the orderbook fully executed
            ins.sellHash[relBeginPrice].pop_front();
            if (ins.sellHash[relBeginPrice].empty()) {//queue is empty->so delete it from tree and hash map
                relBegin++;
                ins.sellBegin = relBegin;//update new begining of tree
                ins.sellHash.erase(relBeginPrice);
                ins.sell.erase(relBeginPrice);
            }//else -> there are elements with same price->same relbegin         
            if (LOGS_Enabled)std::cout << "About to be deleted!\n";
        }
        if (isFill1) {//if new order object fully executed
            //dont have to delete manually since its a shared pointer
            if (LOGS_Enabled)std::cout << "about to get deleted\n";
            return;
        }

        if (!ins.sell.empty()) {
            relBegin = ins.sellBegin;
            relBeginPrice = relBegin->second;
        }
        else break;

    }
    //when i ha
    // //when aggressive order is not fully completed->add to hash map and tree 
    //there cannot be orders with same price- since they would have executed before this. so just insert new one newly
    bool isBuyEmpty = ins.buy.empty();
    auto y = ins.buy.insert({ price,price });
    ins.buyHash[price].push_back(newObj);
    if (LOGS_Enabled)std::cout << "deque is created for the price : " << price << std::endl;
    if (isBuyEmpty)ins.buyBegin = y.first;
    else if (price > ins.buyBegin->second)ins.buyBegin = y.first;

}


void buyOrderExec::exec(std::shared_ptr<orderObj>newObj, instrument& ins) {
    relBegin = ins.sellBegin;
    price = newObj->price;
    if (ins.sell.empty() || (relBegin->first) > price)insertOrder(newObj, ins); //this is a buy order and sell is empty or sell orders are higher
    else execTransac(newObj, ins);//this is a buy order and theres a matching order in sell
}

