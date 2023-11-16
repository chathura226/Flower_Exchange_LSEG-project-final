#include<iostream>
#include<fstream>
#include<vector>
#include<string>
void writeToFile(std::ofstream&,std::string,int,float);
int main(){
    std::ofstream file;
    file.open("execution_rep.csv",std::ios_base::app);//to append
    if(file){
        writeToFile(file,"chathura",69,5.63);
        writeToFile(file,"sajith",81,45);
        writeToFile(file,"nipun",85,999.563);
        
        //file<<"chathura,lakshan"<<std::endl;
    }else{
        std::cout<<"Error writing to file!\n";
    }
    file.close();
}

void writeToFile(std::ofstream &file,std::string a,int b,float c){
    file<<a<<","<<b<<","<<c<<std::endl;
}