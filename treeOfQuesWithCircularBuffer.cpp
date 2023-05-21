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
#define bufSize 20000

std::ofstream file;
typedef std::pair<double, int> pair;
double epsilon = 0.001;
std::mutex mtx;
std::condition_variable bufFull, bufEmpty;
bool finished = 0;

struct bufOrder {
    std::string cliOrd;
    std::string inst;
    int side;
    int qty;
    double price;
    bufOrder() {
        cliOrd ="";
        inst = "";
        side = -1;
        qty =-1;
        price = -1;
    }
    bufOrder(const bufOrder& temp) {
        cliOrd = temp.cliOrd;
        inst = temp.inst;
        side = temp.side;
        qty = temp.qty;
        price = temp.price;
    }
    bufOrder(std::string a, std::string b, int c, int d, double e) {
        cliOrd = a;
        inst = b;
        side = c;
        qty = d;
        price = e;
    }

};
//std::deque<bufOrder> buffer;
boost::circular_buffer<bufOrder> buffer(bufSize);
int bufCount = 0;


class orderObj {
public:
    static int orderNum;
    std::string orderId;
    std::string clientOrderID;
    std::string inst;
    int side;
    double price;
    int qty;
    std::string status;
    orderObj();//define later
    orderObj(std::string& cliOrd, std::string& inst, int& side, int& qty, double& price);
    int executeQty(int qty);
    ~orderObj() {
        if (LOGS_Enabled)std::cout << "OrderObj destructor called for ordNum: " << orderId << "\n";
    }

};
int orderObj::orderNum = 0;
typedef std::deque<std::shared_ptr<orderObj>> OrderPtrDeque;

class instrument {
public:
    std::string name;
    std::map<const double, double, std::greater<double>> buy;//descending order
    std::unordered_map<double, OrderPtrDeque> buyHash;
    std::map<const double, double> sell;
    std::unordered_map<double, OrderPtrDeque> sellHash;
    std::map<const double, double>::iterator buyBegin;
    std::map<const double, double>::iterator sellBegin;
    instrument();
    void newOrder(std::shared_ptr<orderObj> newObj);
};


void writeToFile(std::shared_ptr<orderObj>, int, double);
void writeHeader();
std::string transac_time();
void initializeInstrumentArray();
int initializeIns(int);
int validateAndCreate(std::string& cliOrd, std::string& inst, int& side, int& qty, double& price);
void printfDetails(std::shared_ptr<orderObj> x);//for test logs
int minQty(std::shared_ptr<orderObj> x, std::shared_ptr<orderObj>y);//compare qtys and find the correct qty


instrument* allInstruments[5];//pointer array for order books


int readFile() {
    std::string cliOrd, inst;
    int side, qty;
    double price;
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
            if (nLines == 0) {//first line of the file - headers
                for (int i = 0; i < 5; i++) {
                    if (fields[i] == "Client Order ID" || fields[i] == "Client Order ID\n") fieldOrder[0] = i;
                    else if (fields[i] == "Instrument" || fields[i] == "Instrument\n") fieldOrder[1] = i;
                    else if (fields[i] == "Side" || fields[i] == "Side\n") fieldOrder[2] = i;
                    else if (fields[i] == "Price" || fields[i] == "Price\n") fieldOrder[3] = i;
                    else if (fields[i] == "Quantity" || fields[i] == "Quantity\n") fieldOrder[4] = i;
                    else {
                        std::cout << i << "Invalid header name found!\n";
                        return 1;
                    }
                }
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

void driveLogic() {
    bufOrder temp;
    while (!finished || bufCount != 0) {
        std::unique_lock<std::mutex> bufReadLock(mtx);
        if(!finished) bufEmpty.wait(bufReadLock, [] {return (bufCount != 0||finished); });
        temp=buffer.front();
        buffer.pop_front();
        bufCount--;
        bufReadLock.unlock();
        bufFull.notify_one();
        validateAndCreate(temp.cliOrd, temp.inst, temp.side, temp.qty, temp.price);
               
    }
}

int main() {
    initializeInstrumentArray();
    file.open("execution_rep.csv", std::ios_base::app);//to append
    file << std::fixed << std::setprecision(2);
    writeHeader();
    std::thread t1(readFile);
    std::thread t2(driveLogic);
    t1.join();
    t2.join();
    file.close();
}


void writeToFile(std::shared_ptr<orderObj> x, int qty, double price) {
    if (LOGS_Enabled)std::cout << "write to file called for following\n"
        << x->orderId << ","
        << x->clientOrderID << ","
        << x->inst << ","
        << x->side << ","
        << x->status << ","
        << qty << ","
        << price << "\n";

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
    case 4: temp->name = "Orchid";break;
    }
    if (LOGS_Enabled) std::cout << "Initialized the book for "<< temp->name<<"\n";
    return 0;

}

int validateAndCreate(std::string& cliOrd, std::string& inst, int& side, int& qty, double& price) {
    std::shared_ptr<orderObj> temp = std::make_shared<orderObj>(cliOrd, inst, side, qty, price);
    if (temp->status == "Rejected") {
        writeToFile(temp, temp->qty, temp->price);
        if (LOGS_Enabled)std::cout << "About to get deletd!\n";
        return 1;
    }
    int i;
    if (temp->inst == std::string("Rose"))i = 0;
    else if (temp->inst == std::string("Lavender"))i = 1;
    else if (temp->inst == std::string("Lotus"))i = 2;
    else if (temp->inst == std::string("Tulip"))i = 3;
    else if (temp->inst == std::string("Orchid"))i = 4;

    if (allInstruments[i] == NULL)initializeIns(i);
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

int minQty(std::shared_ptr<orderObj> x, std::shared_ptr<orderObj>y) {
    if (LOGS_Enabled)std::cout << "minqty called for  " << x->qty << " " << y->qty << std::endl;
    if (x->qty > y->qty)return y->qty;
    else return x->qty;
}





//OrderObj member functions

orderObj::orderObj(std::string& cliOrd, std::string& inst, int& side, int& qty, double& price) {
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
        if (LOGS_Enabled)std::cout << "Execute order finished!\n";
        return 1;//returns 1 when order is fill
    }
    if (LOGS_Enabled)std::cout << "Execute order finished!\n";
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


//Instrument member functions..........


instrument::instrument() {
    buyBegin = buy.begin();
    sellBegin = sell.begin();
}

void instrument::newOrder(std::shared_ptr<orderObj> newObj) {
    double price = newObj->price;
    if (LOGS_Enabled)std::cout << "Accessing " << name << " newOrder method\n";
    auto relBegin = (newObj->side == 2) ? buyBegin : sellBegin;
    if (newObj->side == 1 && (sell.empty() || (relBegin->first) > price)) {//this is a buy order and sell is empty or sell orders are higher
        if (LOGS_Enabled)std::cout << "Entered case 1: is sell.empty-> " << sell.empty() << " Buy.empty()-> " << buy.empty() << "\n";
        auto samePrice = buyHash.find(price);
        if (samePrice != buyHash.end()) {//there are elements with same price
            if (LOGS_Enabled)std::cout << "There are elements with same price\n";
            if (LOGS_Enabled)std::cout << "deque size: " << samePrice->second.size() << "\n";
            samePrice->second.push_back(newObj);

        }
        else {//there are no elements with same price. so add the element and ad the reference to Highest Priority map
            if (LOGS_Enabled)std::cout << "There are no elements with same price\n";
            bool isBuyEmpty = buy.empty();
            auto y = buy.insert({ price,price });
            buyHash[price].push_back(newObj);
            if (LOGS_Enabled)std::cout << "deque is created for the price : " << price << std::endl;
            if (isBuyEmpty)buyBegin = y.first;
            else if (price > buyBegin->second)buyBegin = y.first;//if new buy order has higher price, update map begin
        }
        writeToFile(newObj, newObj->qty, price);
        return;
    }
    else if (newObj->side == 2 && (buy.empty() || (relBegin->first) < price)) {
        if (LOGS_Enabled)std::cout << "Entered case 2: is buy.empty-> " << buy.empty() << " sell.empty-> " << sell.empty() << "\n";
        auto samePrice = sellHash.find(price);
        if (samePrice != sellHash.end()) {//there are elements with same price
            if (LOGS_Enabled)std::cout << "There are elements with same price\n";
            if (LOGS_Enabled)std::cout << "deque size: " << samePrice->second.size() << "\n";
            samePrice->second.push_back(newObj);
        }
        else {//there are no elements with same price. so add the element and ad the reference to Highest Priority map
            bool isSellEmpty = sell.empty();
            auto y = sell.insert({ price,price });
            sellHash[price].push_back(newObj);
            if (LOGS_Enabled)std::cout << "deque is created for the price : " << price << std::endl;
            if (isSellEmpty)sellBegin = y.first;
            else if (price < sellBegin->second)sellBegin = y.first;//if new sell order has lower price, update map begin

        }
        writeToFile(newObj, newObj->qty, newObj->price);
        return;

    }
    else if (newObj->side == 1) {//this is a buy order and theres a sell order for equal or low price
        double relBeginPrice = relBegin->second;
        if (LOGS_Enabled)std::cout << "inside side==1 but sell is not empty" << std::endl;
        while (!sell.empty() && (relBeginPrice) <= price) {
            std::shared_ptr<orderObj> relObj = sellHash[relBeginPrice].front();//object that gonna execute with new order
            int execQty = minQty(newObj, relObj);
            if (LOGS_Enabled)printfDetails(relObj);
            if (LOGS_Enabled)std::cout << "excec qty " << execQty << std::endl;
            int isFill1 = newObj->executeQty(execQty);

            writeToFile(newObj, execQty, relBeginPrice);
            int isFill2 = relObj->executeQty(execQty);
            writeToFile(relObj, execQty, relBeginPrice);

            if (isFill2) {//if relevent object from the orderbook fully executed
                sellHash[relBeginPrice].pop_front();
                if (sellHash[relBeginPrice].empty()) {//queue is empty->so delete it from tree and hash map
                    relBegin++;
                    sellBegin = relBegin;//update new begining of tree
                    sellHash.erase(relBeginPrice);
                    sell.erase(relBeginPrice);
                }//else -> there are elements with same price->same relbegin         
                if (LOGS_Enabled)std::cout << "About to be deleted!\n";
            }
            if (isFill1) {//if new order object fully executed
                //dont have to delete manually since its a shared pointer
                if (LOGS_Enabled)std::cout << "about to get deleted\n";
                return;
            }

            if (!sell.empty()) {
                relBegin = sellBegin;
                relBeginPrice = relBegin->second;
            }
            else break;

        }
        // //when aggressive order is not fully completed->add to hash map and tree 
        //there cannot be orders with same price- since they would have executed before this. so just insert new one newly
        bool isBuyEmpty = buy.empty();
        auto y = buy.insert({ price,price });
        buyHash[price].push_back(newObj);
        if (LOGS_Enabled)std::cout << "deque is created for the price : " << price << std::endl;
        if (isBuyEmpty)buyBegin = y.first;
        else if (price > buyBegin->second)buyBegin = y.first;

    }
    else if (newObj->side == 2) {//this is a sell order and theres a buy order for equal or high price
        double relBeginPrice = relBegin->second;
        if (LOGS_Enabled)std::cout << "inside side==2 but buy is not empty" << std::endl;
        while (!buy.empty() && (relBeginPrice) >= newObj->price) {

            std::shared_ptr<orderObj> relObj = buyHash[relBeginPrice].front();//object that gonna execute with new order
            if (LOGS_Enabled)std::cout << "inside while loop\n";
            int execQty = minQty(newObj, relObj);
            if (LOGS_Enabled)printfDetails(relObj);
            if (LOGS_Enabled)std::cout << "excec qty " << execQty << std::endl;
            int isFill1 = newObj->executeQty(execQty);

            writeToFile(newObj, execQty, relBeginPrice);
            int isFill2 = relObj->executeQty(execQty);
            writeToFile(relObj, execQty, relBeginPrice);

            if (isFill2) {//if relevent object from the orderbook fully executed

                buyHash[relBeginPrice].pop_front();
                if (buyHash[relBeginPrice].empty()) {//queue is empty->so delete it from tree and hash map

                    relBegin++;
                    buyBegin = relBegin;//update new begining of tree
                    buyHash.erase(relBeginPrice);
                    buy.erase(relBeginPrice);
                }//else -> there are elements with same price->same relbegin         
                if (LOGS_Enabled)std::cout << "About to be deleted!\n";
            }

            if (isFill1) {//if new order object fully executed
                //dont have to delete manually since its a shared pointer
                if (LOGS_Enabled)std::cout << "About to get deleted!\n";
                return;
            }
            if (!buy.empty()) {
                relBegin = buyBegin;
                relBeginPrice = relBegin->second;
            }
            else break;
        }

        // //when aggressive order is not fully completed->add to hash map and tree 
        //there cannot be orders with same price- since they would have executed before this. so just insert new one newly
        bool isSellEmpty = sell.empty();
        auto y = sell.insert({ price,price });
        sellHash[price].push_back(newObj);
        if (LOGS_Enabled)std::cout << "deque is created for the price : " << price << std::endl;
        if (isSellEmpty)sellBegin = y.first;
        else if (price > sellBegin->second)sellBegin = y.first;

    }
}


