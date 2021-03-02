#include <iostream>
#include <unordered_map>
#include <string>
#include <unordered_set>
#include <vector>
#include <fstream>

using namespace std;

unordered_set<char> letters{'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z'};
unordered_set<char> digits{'0','1','2','3','4','5','6','7','8','9'};

unordered_map<string,int> labels;
unordered_map<string,pair<int,pair<int,int>>> variables;
unordered_set<string> instructions{"mov","add","inc","sub","dec","mul","div","xor","or","and","not","rcl","rcr","shl","shr","push","pop","nop","cmp","jmp","jz","jnz","je","jne","ja","jna","jae","jb","jbe","jnae","jnb","jnbe","jnc","jc","int"};
unordered_map<string,int> registers16{{"ax",0},{"bx",0},{"cx",0},{"dx",0},{"si",0},{"di",0},{"bp",0},{"sp",65534}};
unordered_map<string,int> registers8{{"ah",0},{"al",0},{"bh",0},{"bl",0},{"ch",0},{"cl",0},{"dh",0},{"dl",0},};
unordered_map<string,bool> flags{{"of",0},{"af",0},{"cf",0},{"zf",0},{"sf",0}};

vector<vector<string>> simulation;  // keeps instructions in the right order, every subvector is an instruction

bool isPrinted = false; // checks that error is printed or not
bool error = false; // checks that there is an error
vector<string> change;  // after every instruction it makes necessary register arrangement, used in update() method
int Counter = 0;    // decides which instruction is being executed

int mem[2<<15] = {-1};

bool isNumber(string s){    // returns 1 if the parameter s is a number
    if(s[s.length()-1] == 'h'){
        if(digits.find(s[0]) != digits.end()){
            s.pop_back();
            for(int i=0; i<s.length(); i++){
                if(digits.find(s[i]) == digits.end() && s[i] !=  'a' && s[i] !=  'b' && s[i] !=  'c' && s[i] !=  'd' && s[i] !=  'e' && s[i] !=  'f'){
                    return false;
                }
            }
            return true;
        }else{
            return false;
        }
    }else if(s[0] == 0){
        for(int i=0; i<s.length(); i++){
            if(digits.find(s[i]) == digits.end() && s[i] !=  'a' && s[i] !=  'b' && s[i] !=  'c' && s[i] !=  'd' && s[i] !=  'e' && s[i] !=  'f'){
                return false;
            }
        }
        return true;
    }else if(s[s.length()-1] == 'd'){
        s.pop_back();
        for(int i=0; i<s.length(); i++){
            if(digits.find(s[i]) == digits.end()){
                return false;
            }
        }
        return true;
    }else{
        for(int i=0; i<s.length(); i++){
            if(digits.find(s[i]) == digits.end()){
                return false;
            }
        }
        return true;
    }
}

int decimal(string s){  // returns the decimal value of the parameter s
    if(s[s.length()-1] == 'h'){
        s.pop_back();
        return stoi(s, nullptr,16);
    }else if(s[0] == 0){
        return stoi(s,nullptr,16);
    }else if(s[s.length()-1] == 'd'){
        s.pop_back();
        return stoi(s);
    }else{
        return stoi(s);
    }
}

int valueOfAddress(string address){     // returns the value of the adress
    string temp = "";
    for(int i=2; i<address.length()-1; i++){
        temp = temp + address[i];
    }
    if(isNumber(temp)){
        int final = decimal(temp);
        if(address[0] == 'w'){
            return (mem[final+1]*(2<<7)) + mem[final];
        }else if(address[0] == 'b'){
            return mem[final];
        }
    }else if(temp == "bx" || temp == "bp" || temp == "si" || temp == "di"){
        int final = registers16[temp];
        if(address[0] == 'w'){
            return (mem[final+1]*(2<<7)) + mem[final];
        }else if(address[0] == 'b'){
            return mem[final];
        }
    }
}

int findAddress(string address){        // returns what the adress is, if parameter is not appropriate returns -1
    string temp = "";
    for(int i=2; i<address.length()-1; i++){
        temp = temp + address[i];
    }
    if(isNumber(temp) && decimal(temp) >= 0 && decimal(temp) < (2<<15)){
        return decimal(temp);
    }else if(temp == "bx" || temp == "bp" || temp == "si" || temp == "di"){
        return registers16[temp];
    }else{
        return -1;
    }
}

void update(){              // after every instruction, updates 8-bit and 16-bit registers
    for(int i=0; i<change.size(); i++){
        if(change[i] == "ax"){
            registers8["ah"] = registers16["ax"]/(2<<7);
            registers8["al"] = registers16["ax"]%(2<<7);
        }else if(change[i] == "ah" || change[i] == "al"){
            registers16["ax"] = (registers8["ah"]*(2<<7))+registers8["al"];
        }else if(change[i] == "bx"){
            registers8["bh"] = registers16["bx"]/(2<<7);
            registers8["bl"] = registers16["bx"]%(2<<7);
        }else if(change[i] == "bh" || change[i] == "bl"){
            registers16["bx"] = (registers8["bh"]*(2<<7))+registers8["bl"];
        }else if(change[i] == "cx"){
            registers8["ch"] = registers16["cx"]/(2<<7);
            registers8["cl"] = registers16["cx"]%(2<<7);
        }else if(change[i] == "ch" || change[i] == "cl"){
            registers16["cx"] = (registers8["ch"]*(2<<7))+registers8["cl"];
        }else if(change[i] == "dx"){
            registers8["dh"] = registers16["dx"]/(2<<7);
            registers8["dl"] = registers16["dx"]%(2<<7);
        }else if(change[i] == "dh" || change[i] == "dl"){
            registers16["dx"] = (registers8["dh"]*(2<<7))+registers8["dl"];
        }
    }

    change.clear();

}

void mov(string dest, string source){       // mov instruction, puts the value of source into the dest
    if(registers16.find(dest) != registers16.end()){
        if(source[0] == '[' && source[source.length()-1] == ']'){
            source = "w" + source;
        }
        if(registers16.find(source) != registers16.end()){
            registers16[dest] = registers16[source];
            change.push_back(dest);
        }else if(variables.find(source) != variables.end()){
            if(variables[source].second.second == 16){
                registers16[dest] = variables[source].first;
                change.push_back(dest);
            }else{
                error = true;
            }
        }else if(source[1] == '[' && source[source.length()-1] == ']' && findAddress(source) != -1){
            if((source[0] == 'w') && (valueOfAddress(source) != -1) && (valueOfAddress(source) != -2)){
                registers16[dest] = valueOfAddress(source);
                change.push_back(dest);
            }else{
                error = true;
            }
        }else if(isNumber(source)){
            int temp = decimal(source);
            if(temp >= 0 && temp < (2<<15)){
                registers16[dest] = decimal(source);
                change.push_back(dest);
            }else{
                error = true;
            }
        }else{
            error = true;
        }
    }else if(registers8.find(dest) != registers8.end()){
        if(source[0] == '[' && source[source.length()-1] == ']'){
            source = "b" + source;
        }
        if(registers8.find(source) != registers8.end()){
            registers8[dest] = registers8[source];
            change.push_back(dest);
        }else if(variables.find(source) != variables.end()){
            if(variables[source].second.second == 8){
                registers8[dest] = variables[source].first;
                change.push_back(dest);
            }else{
                error = true;
            }
        }else if(source[1] == '[' && source[source.length()-1] == ']' && findAddress(source) != -1){
            if((source[0] == 'b') && (valueOfAddress(source) != -1) && (valueOfAddress(source) != -2)){
                registers8[dest] = valueOfAddress(source);
                change.push_back(dest);
            }else{
                error = true;
            }
        }else if(isNumber(source)){
            int temp = decimal(source);
            if(temp >= 0 && temp < (2<<7)){
                registers8[dest] = temp;
                change.push_back(dest);
            }else{
                error = true;
            }
        }else{
            error = true;
        }
    }else if(variables.find(dest) != variables.end()){
        if(registers16.find(source) != registers16.end()){
            if(variables[dest].second.second == 16){
                variables[dest].first = registers16[source];
                mem[variables[dest].second.first+1] = (registers16[source]) / (2<<7);
                mem[variables[dest].second.first] = (registers16[source]) % (2<<7);
            }else{
                error = true;
            }
        }else if(registers8.find(source) != registers8.end()){
            if(variables[dest].second.second == 8){
                variables[dest].first = registers8[source];
                mem[variables[dest].second.first] = registers8[source];
            }else{
                error = true;
            }
        }else if(isNumber(source)){
            int temp = decimal(source);
            if(temp >= 0 && temp < (2<<7) && variables[dest].second.second == 8){
                variables[dest].first = temp;
                mem[variables[dest].second.first] = temp;
            }else if(temp >= 0 && temp < (2<<15) && variables[dest].second.second == 16){
                variables[dest].first = temp;
                mem[variables[dest].second.first+1] = temp / (2<<7);
                mem[variables[dest].second.first] = temp % (2<<7);
            }else{
                error = true;
            }
        }else{
            error = true;
        }
    }else if(dest[0] == '[' && dest[dest.length()-1] == ']'){
        if(registers16.find(source) != registers16.end()){
            dest = "w"  + dest;
            if(findAddress(dest) != -1){
                if(valueOfAddress(dest) != -2){
                    mem[findAddress(dest)+1] = registers16[source] / (2<<7);
                    mem[findAddress(dest)] = registers16[source] % (2<<7);
                    for(auto itr = variables.begin(); itr != variables.end(); itr++){
                        if((*itr).second.second.first == findAddress(dest) && (*itr).second.second.second == 8){
                            (*itr).second.first = registers16[source] % (2<<7);
                        }else if((*itr).second.second.first == findAddress(dest) && (*itr).second.second.second == 16){
                            (*itr).second.first = registers16[source];
                            break;
                        }else if(((*itr).second.second.first == findAddress(dest)+1) && (*itr).second.second.second == 8){
                            (*itr).second.first = registers16[source] / (2<<7);
                        }
                    }
                }else{
                    error = true;
                }
            }else{
                error = true;
            }
        }else if(registers8.find(source) != registers8.end()){
            dest = "b" + dest;
            if(findAddress(dest) != -1){
                if(valueOfAddress(dest) != -2){
                    mem[findAddress(dest)] = registers8[source];
                    for(auto itr = variables.begin(); itr != variables.end(); itr++){
                        if((*itr).second.second.first == findAddress(dest) && (*itr).second.second.second == 8){
                            (*itr).second.first = registers8[source];
                            break;
                        }else if(((*itr).second.second.first == findAddress(dest)-1) && (*itr).second.second.second == 16){
                            (*itr).second.first = mem[(*itr).second.second.first] + (registers8[source]*(2<<7));
                            break;
                        }else if((*itr).second.second.first == findAddress(dest) && (*itr).second.second.second == 16){
                            (*itr).second.first = registers8[source] + mem[(*itr).second.second.first+1]*(2<<7);
                            break;
                        }
                    }
                }else{
                    error = true;
                }
            }else{
                error = true;
            }
        }else if(isNumber(source)){
            dest = "w" + dest;
            if(findAddress(dest) != -1){
                if(valueOfAddress(dest) != -2){
                    int temp = decimal(source);
                    if(temp >= (2<<7) && temp < (2<<15)){
                        mem[findAddress(dest)+1] = temp / (2<<7);
                        mem[findAddress(dest)] = temp % (2<<7);
                        for(auto itr = variables.begin(); itr != variables.end(); itr++){
                            if((*itr).second.second.first == findAddress(dest) && (*itr).second.second.second == 8){
                                (*itr).second.first = temp % (2<<7);
                            }else if((*itr).second.second.first == findAddress(dest) && (*itr).second.second.second == 16){
                                (*itr).second.first = temp;
                                break;
                            }else if(((*itr).second.second.first == findAddress(dest)+1) && (*itr).second.second.second == 8){
                                (*itr).second.first = temp / (2<<7);
                            }
                        }
                    }else{
                        error = true;
                    }
                }else{
                    error = true;
                }
            }else{
                error = true;
            }
        }else{
            error = true;
        }
    }else if(dest[1] == '[' && dest[dest.length()-1] == ']' && findAddress(dest) != -1){
        if(registers16.find(source) != registers16.end()){
            if(dest[0] == 'w'){
                if(valueOfAddress(dest) != -2){
                    mem[findAddress(dest)+1] = registers16[source] / (2<<7);
                    mem[findAddress(dest)] = registers16[source] % (2<<7);
                    for(auto itr = variables.begin(); itr != variables.end(); itr++){
                        if((*itr).second.second.first == findAddress(dest) && (*itr).second.second.second == 8){
                            (*itr).second.first = registers16[source] % (2<<7);
                        }else if((*itr).second.second.first == findAddress(dest) && (*itr).second.second.second == 16){
                            (*itr).second.first = registers16[source];
                            break;
                        }else if(((*itr).second.second.first == findAddress(dest)+1) && (*itr).second.second.second == 8){
                            (*itr).second.first = registers16[source] / (2<<7);
                        }
                    }
                }else{
                    error = true;
                }
            }else{
                error = true;
            }
        }else if(registers8.find(source) != registers8.end()){
            if(dest[0] == 'b'){
                if(valueOfAddress(dest) != -2){
                    mem[findAddress(dest)] = registers8[source];
                    for(auto itr = variables.begin(); itr != variables.end(); itr++){
                        if((*itr).second.second.first == findAddress(dest) && (*itr).second.second.second == 8){
                            (*itr).second.first = registers8[source];
                            break;
                        }else if(((*itr).second.second.first == findAddress(dest)-1) && (*itr).second.second.second == 16){
                            (*itr).second.first = mem[(*itr).second.second.first] + (registers8[source]*(2<<7));
                            break;
                        }else if((*itr).second.second.first == findAddress(dest) && (*itr).second.second.second == 16){
                            (*itr).second.first = registers8[source] + mem[(*itr).second.second.first+1]*(2<<7);
                            break;
                        }
                    }
                }else{
                    error = true;
                }
            }else{
                error = true;
            }
        }else if(isNumber(source)){
            if(valueOfAddress(dest) != -2){
                int temp = decimal(source);
                if(temp >= 0 && temp < (2<<7) && dest[0] == 'b'){
                    mem[findAddress(dest)] = temp;
                    for(auto itr = variables.begin(); itr != variables.end(); itr++){
                        if((*itr).second.second.first == findAddress(dest) && (*itr).second.second.second == 8){
                            (*itr).second.first = temp;
                            break;
                        }else if(((*itr).second.second.first == findAddress(dest)-1) && (*itr).second.second.second == 16){
                            (*itr).second.first = mem[(*itr).second.second.first] + (temp*(2<<7));
                            break;
                        }else if((*itr).second.second.first == findAddress(dest) && (*itr).second.second.second == 16){
                            (*itr).second.first = temp + mem[(*itr).second.second.first+1]*(2<<7);
                            break;
                        }
                    }
                }else if(temp >= 0 && temp < (2<<15) && dest[0] == 'w'){
                    mem[findAddress(dest)+1] = temp / (2<<7);
                    mem[findAddress(dest)] = temp % (2<<7);
                    for(auto itr = variables.begin(); itr != variables.end(); itr++){
                        if((*itr).second.second.first == findAddress(dest) && (*itr).second.second.second == 8){
                            (*itr).second.first = temp % (2<<7);
                        }else if((*itr).second.second.first == findAddress(dest) && (*itr).second.second.second == 16){
                            (*itr).second.first = temp;
                            break;
                        }else if(((*itr).second.second.first == findAddress(dest)+1) && (*itr).second.second.second == 8){
                            (*itr).second.first = temp / (2<<7);
                        }
                    }
                }else{
                    error = true;
                }
            }else{
                error = true;
            }
        }else{
            error = true;
        }
    }else{
        error = true;
    }

    update();
    Counter++;      // moves the counter to the next instruction

}

void add(string dest, string source, string op){    //add and inc instructions, adds the values of the source and dest and puts into the dest

    int sum = 0;        // sum of the two operands
    bool is8 = false;   // checks whether operands are 8-bits or 16-bits
    int op1 = 0;        // operand1
    int op2 = 0;        // operand2

    if(registers16.find(dest) != registers16.end()){
        if(source[1] == '[' && source[source.length()-1] == ']'){
            source = "w" + source;
        }
        if(registers16.find(source) != registers16.end()){
            op1 = registers16[dest];
            op2 = registers16[source];
            sum = registers16[dest] + registers16[source];
            registers16[dest] = sum % (2<<15);
            change.push_back(dest);
        }else if(variables.find(source) != variables.end()){
            if(variables[source].second.second == 8){
                error = true;
            }else{
                op1 = registers16[dest];
                op2 = variables[source].first;
                sum = registers16[dest] + variables[source].first;
                registers16[dest] = sum % (2<<15);
                change.push_back(dest);
            }
        }else if(source[1] == '[' && source[source.length()-1] == ']' && findAddress(source) != -1){
            if((source[0] == 'w') && (valueOfAddress(source) != -1) && (valueOfAddress(source) != -2)){
                op1 = registers16[dest];
                op2 = valueOfAddress(source);
                sum = registers16[dest] + valueOfAddress(source);
                registers16[dest] = sum % (2<<15);
                change.push_back(dest);
            }else{
                error = true;
            }
        }else if(isNumber(source)){
            int temp = decimal(source);
            if(temp >= 0 && temp < (2<<15)){
                op1 = registers16[dest];
                op2 = temp;
                sum = registers16[dest] + temp;
                registers16[dest] = sum % (2<<15);
                change.push_back(dest);
            }else{
                error = true;
            }
        }else{
            error = true;
        }
    }else if(registers8.find(dest) != registers8.end()){
        if(source[1] == '[' && source[source.length()-1] == ']'){
            source = "b" + source;
        }
        if(registers8.find(source) != registers8.end()){
            op1 = registers8[dest];
            op2 = registers8[source];
            sum = registers8[dest] + registers8[source];
            registers8[dest] = sum % (2<<7);
            is8 = true;
            change.push_back(dest);
        }else if(variables.find(source) != variables.end()){
            if(variables[source].second.second == 8){
                op1 = registers8[dest];
                op2 = variables[source].first;
                sum = registers8[dest] + variables[source].first;
                registers8[dest] = sum % (2<<7);
                is8 = true;
                change.push_back(dest);
            }else{
                error = true;
            }
        }else if(source[1] == '[' && source[source.length()-1] == ']' && findAddress(source) != -1){
            if((source[0] == 'b') && (valueOfAddress(source) != -1) && (valueOfAddress(source) != -2)){
                op1 = registers8[dest];
                op2 = valueOfAddress(source);
                sum = registers8[dest] + valueOfAddress(source);
                registers8[dest] = sum % (2<<7);
                is8 = true;
                change.push_back(dest);
            }else{
                error = true;
            }
        }else if(isNumber(source)){
            int temp = decimal(source);
            if(temp >= 0 && temp < (2<<7)){
                op1 = registers8[dest];
                op2 = temp;
                sum = registers8[dest] + temp;
                registers8[dest] = sum % (2<<7);
                is8 = true;
                change.push_back(dest);
            }else{
                error = true;
            }
        }else{
            error = true;
        }
    }else if(variables.find(dest) != variables.end()){
        if(registers16.find(source) != registers16.end()){
            if(variables[dest].second.second == 16){
                op1 = variables[dest].first;
                op2 = registers16[source];
                sum = variables[dest].first + registers16[source];
                variables[dest].first = sum % (2<<15);
                mem[variables[dest].second.first+1] = variables[dest].first / (2<<7);
                mem[variables[dest].second.first] = variables[dest].first % (2<<7);
            }else{
                error = true;
            }
        }else if(registers8.find(source) != registers8.end()){
            if(variables[dest].second.second == 8){
                op1 = variables[dest].first;
                op2 = registers8[source];
                sum = variables[dest].first + registers8[source];
                variables[dest].first = sum % (2<<7);
                is8 = true;
                mem[variables[dest].second.first] = variables[dest].first;
            }else{
                error = true;
            }
        }else if(isNumber(source)){
            int temp = decimal(source);
            sum = variables[dest].first + temp;
            if(temp >= 0 && temp < (2<<7) && variables[dest].second.second == 8){
                op1 = variables[dest].first;
                op2 = temp;
                variables[dest].first = sum % (2<<7);
                is8 = true;
                mem[variables[dest].second.first] = variables[dest].first;
            }else if(temp >= 0 && temp < (2<<15) && variables[dest].second.second == 16){
                op1 = variables[dest].first;
                op2 = temp;
                variables[dest].first = sum % (2<<15);
                mem[variables[dest].second.first+1] = (variables[dest].first) / (2<<7);
                mem[variables[dest].second.first] = (variables[dest].first) % (2<<7);
            }else{
                error = true;
            }
        }else{
            error = true;
        }
    }else if(dest[0] == '[' && dest[dest.length()-1] == ']'){
        if(valueOfAddress(dest) != -1 && valueOfAddress(dest) != -2){
            if(registers16.find(source) != registers16.end()){
                dest = "w" + dest;
                if(findAddress(dest) != -1){
                    sum = valueOfAddress(dest) + registers16[source];
                    op1 = valueOfAddress(dest);
                    op2 = registers16[source];
                    mem[findAddress(dest)+1] = (sum % (2<<15)) / (2<<7);
                    mem[findAddress(dest)] = (sum % (2<<15)) % (2<<7);
                    for(auto itr = variables.begin(); itr != variables.end(); itr++){
                        if((*itr).second.second.first == findAddress(dest) && (*itr).second.second.second == 8){
                            (*itr).second.first = (sum % (2<<15)) % (2<<7);
                        }else if((*itr).second.second.first == findAddress(dest) && (*itr).second.second.second == 16){
                            (*itr).second.first = sum % (2<<15);
                            break;
                        }else if(((*itr).second.second.first == findAddress(dest)+1) && (*itr).second.second.second == 8){
                            (*itr).second.first = (sum % (2<<15)) / (2<<7);
                        }
                    }
                }else{
                    error = true;
                }
            }else if(registers8.find(source) != registers8.end()){
                dest = "b" + dest;
                if(findAddress(dest) != -1){
                    sum = valueOfAddress(dest) + registers8[source];
                    is8 = true;
                    op1 = valueOfAddress(dest);
                    op2 = registers8[source];
                    mem[findAddress(dest)] = sum % (2<<7);
                    for(auto itr = variables.begin(); itr != variables.end(); itr++){
                        if((*itr).second.second.first == findAddress(dest) && (*itr).second.second.second == 8){
                            (*itr).second.first = sum % (2<<7);
                            break;
                        }else if(((*itr).second.second.first == findAddress(dest)-1) && (*itr).second.second.second == 16){
                            (*itr).second.first = mem[(*itr).second.second.first] + ((sum % (2<<7))*(2<<7));
                            break;
                        }else if((*itr).second.second.first == findAddress(dest) && (*itr).second.second.second == 16){
                            (*itr).second.first = (sum % (2<<7)) + mem[(*itr).second.second.first+1]*(2<<7);
                            break;
                        }
                    }
                }else{
                    error = true;
                }
            }else if(isNumber(source)){
                dest = "w" + dest;
                if(findAddress(dest) != -1){
                    int temp = decimal(source);
                    sum = valueOfAddress(dest) + temp;
                    if(temp >= (2<<7) && temp < (2<<15)){
                        op1 = valueOfAddress(dest);
                        op2 = temp;
                        mem[findAddress(dest)+1] = (sum % (2<<15)) / (2<<7);
                        mem[findAddress(dest)] = (sum % (2<<15)) % (2<<7);
                        for(auto itr = variables.begin(); itr != variables.end(); itr++){
                            if((*itr).second.second.first == findAddress(dest) && (*itr).second.second.second == 8){
                                (*itr).second.first = (sum % (2<<15)) % (2<<7);
                            }else if((*itr).second.second.first == findAddress(dest) && (*itr).second.second.second == 16){
                                (*itr).second.first = sum % (2<<15);
                                break;
                            }else if(((*itr).second.second.first == findAddress(dest)+1) && (*itr).second.second.second == 8){
                                (*itr).second.first = (sum % (2<<15)) / (2<<7);
                            }
                        }
                    }else{
                        error = true;
                    }
                }else{
                    error = true;
                }
            }else{
                error = true;
            }
        }else{
            error = true;
        }
    }else if(dest[1] == '[' && dest[dest.length()-1] == ']' & findAddress(dest) != -1){
        if(valueOfAddress(dest) != -2 && valueOfAddress(dest) != -1){
            if(registers16.find(source) != registers16.end()){
                if(dest[0] == 'w'){
                    sum = valueOfAddress(dest) + registers16[source];
                    op1 = valueOfAddress(dest);
                    op2 = registers16[source];
                    mem[findAddress(dest)+1] = (sum % (2<<15)) / (2<<7);
                    mem[findAddress(dest)] = (sum % (2<<15)) % (2<<7);
                    for(auto itr = variables.begin(); itr != variables.end(); itr++){
                        if((*itr).second.second.first == findAddress(dest) && (*itr).second.second.second == 8){
                            (*itr).second.first = (sum % (2<<15)) % (2<<7);
                        }else if((*itr).second.second.first == findAddress(dest) && (*itr).second.second.second == 16){
                            (*itr).second.first = sum % (2<<15);
                            break;
                        }else if(((*itr).second.second.first == findAddress(dest)+1) && (*itr).second.second.second == 8){
                            (*itr).second.first = (sum % (2<<15)) / (2<<7);
                        }
                    }
                }else{
                    error = true;
                }
            }else if(registers8.find(source) != registers8.end()){
                if(dest[0] == 'b'){
                    sum = valueOfAddress(dest) + registers8[source];
                    is8 = true;
                    op1 = valueOfAddress(dest);
                    op2 = registers8[source];
                    mem[findAddress(dest)] = sum % (2<<7);
                    for(auto itr = variables.begin(); itr != variables.end(); itr++){
                        if((*itr).second.second.first == findAddress(dest) && (*itr).second.second.second == 8){
                            (*itr).second.first = sum % (2<<7);
                            break;
                        }else if(((*itr).second.second.first == findAddress(dest)-1) && (*itr).second.second.second == 16){
                            (*itr).second.first = mem[(*itr).second.second.first] + ((sum % (2<<7))*(2<<7));
                            break;
                        }else if((*itr).second.second.first == findAddress(dest) && (*itr).second.second.second == 16){
                            (*itr).second.first = (sum % (2<<7)) + mem[(*itr).second.second.first+1]*(2<<7);
                            break;
                        }
                    }
                }else{
                    error = true;
                }
            }else if(isNumber(source)){
                int temp = decimal(source);
                sum = valueOfAddress(dest) + temp;
                if(temp >= 0 && temp < (2<<7) && dest[0] == 'b'){
                    is8 = true;
                    op1 = valueOfAddress(dest);
                    op2 = temp;
                    mem[findAddress(dest)] = sum % (2<<7);
                    for(auto itr = variables.begin(); itr != variables.end(); itr++){
                        if((*itr).second.second.first == findAddress(dest) && (*itr).second.second.second == 8){
                            (*itr).second.first = sum % (2<<7);
                            break;
                        }else if(((*itr).second.second.first == findAddress(dest)-1) && (*itr).second.second.second == 16){
                            (*itr).second.first = mem[(*itr).second.second.first] + ((sum % (2<<7))*(2<<7));
                            break;
                        }else if((*itr).second.second.first == findAddress(dest) && (*itr).second.second.second == 16){
                            (*itr).second.first = (sum % (2<<7)) + mem[(*itr).second.second.first+1]*(2<<7);
                            break;
                        }
                    }
                }else if(temp >= 0 && temp < (2<<15) && dest[0] == 'w'){
                    op1 = valueOfAddress(dest);
                    op2 = temp;
                    mem[findAddress(dest)+1] = (sum % (2<<15)) / (2<<7);
                    mem[findAddress(dest)] = (sum % (2<<15)) % (2<<7);
                    for(auto itr = variables.begin(); itr != variables.end(); itr++){
                        if((*itr).second.second.first == findAddress(dest) && (*itr).second.second.second == 8){
                            (*itr).second.first = (sum % (2<<15)) % (2<<7);
                        }else if((*itr).second.second.first == findAddress(dest) && (*itr).second.second.second == 16){
                            (*itr).second.first = sum % (2<<15);
                            break;
                        }else if(((*itr).second.second.first == findAddress(dest)+1) && (*itr).second.second.second == 8){
                            (*itr).second.first = (sum % (2<<15)) / (2<<7);
                        }
                    }
                }else{
                    error = true;
                }
            }else{
                error = true;
            }
        }else{
            error = true;
        }
    }else{
        error = true;
    }

    if(is8){        // updates flags for 8-bits operands
        int temp = sum % (2<<7);
        if(temp == 0){
            flags["zf"] = true;
        }else{
            flags["zf"] = false;
        }
        if(op == "add"){            // if instruction is inc, carry flag is not affected
            if(sum >= (2<<7)){
                flags["cf"] = true;
            }else{
                flags["cf"] = false;
            }
        }
        if((op1 / (2<<6) == 1 && op2 / (2<<6) == 1 && temp / (2<<6) == 0) || (op1 / (2<<6) == 0 && op2 / (2<<6) == 0 && temp / (2<<6) == 1)){
            flags["of"] = true;
        }else{
            flags["of"] = false;
        }
        if(temp / (2<<6) == 1){
            flags["sf"] = true;
        }else{
            flags["sf"] = false;
        }
        if((op1 % (2<<3) + op2 % (2<<3)) >= (2<<3)){
            flags["af"] = true;
        }else{
            flags["af"] = false;
        }
    }else{              // update flags for 16-bits operands
        int temp = sum % (2<<15);
        if(temp == 0){
            flags["zf"] = true;
        }else{
            flags["zf"] = false;
        }
        if(op == "add"){            // if instruction is inc, carry flag is not affected
            if(sum >= (2<<15)){
                flags["cf"] = true;
            }else{
                flags["cf"] = false;
            }
        }
        if((op1 / (2<<14) == 1 && op2 / (2<<14) == 1 && temp / (2<<14) == 0) || (op1 / (2<<14) == 0 && op2 / (2<<14) == 0 && temp / (2<<14) == 1)){
            flags["of"] = true;
        }else{
            flags["of"] = false;
        }
        if(temp / (2<<14) == 1){
            flags["sf"] = true;
        }else{
            flags["sf"] = false;
        }
        if((op1 % (2<<3) + op2 % (2<<3)) >= (2<<3)){
            flags["af"] = true;
        }else{
            flags["af"] = false;
        }
    }

    update();
    Counter++;               // moves the counter to the next instruction

}

void sub(string dest, string source, string op){    // sub instruction, subtract source from the dest and puts the result in the dest

    int diff = 0;           // difference between operands
    bool is8 = false;       // checks whether operands are 8-bits or 16-bits
    int op1 = 0;            // operand1
    int op2 = 0;            // operand2

    if(registers16.find(dest) != registers16.end()){
        if(source[0] == '[' && source[source.length()-1] == ']'){
            source = "w" + source;
        }
        if(registers16.find(source) != registers16.end()){
            op1 = registers16[dest];
            op2 = registers16[source];
            if(op1 >= op2){
                diff = op1 - op2;
            }else{
                diff = op1 - op2 + (2<<15);
            }
            registers16[dest] = diff;
            change.push_back(dest);
        }else if(variables.find(source) != variables.end()){
            if(variables[source].second.second == 16){
                op1 = registers16[dest];
                op2 = variables[source].first;
                if(op1 >= op2){
                    diff = op1 - op2;
                }else{
                    diff = op1 - op2 + (2<<15);
                }
                registers16[dest] = diff;
                change.push_back(dest);
            }else{
                error = true;
            }
        }else if(source[1] == '[' && source[source.length()-1] == ']' && findAddress(source) != -1){
            if((source[0] == 'w') && (valueOfAddress(source) != -1) && (valueOfAddress(source) != -2)){
                op1 = registers16[dest];
                op2 = valueOfAddress(source);
                if(op1 >= op2){
                    diff = op1 - op2;
                }else{
                    diff = op1 - op2 + (2<<15);
                }
                registers16[dest] = diff;
                change.push_back(dest);
            }else{
                error = true;
            }
        }else if(isNumber(source)){
            int temp = decimal(source);
            if(temp >= 0 && temp < (2<<15)){
                op1 = registers16[dest];
                op2 = temp;
                if(op1 >= op2){
                    diff = op1 - op2;
                }else{
                    diff = op1 - op2 + (2<<15);
                }
                registers16[dest] = diff;
                change.push_back(dest);
            }else{
                error = true;
            }
        }else{
            error = true;
        }
    }else if(registers8.find(dest) != registers8.end()){
        if(source[0] == '[' && source[source.length()-1] == ']'){
            source = "b" + source;
        }
        if(registers8.find(source) != registers8.end()){
            op1 = registers8[dest];
            op2 = registers8[source];
            if(op1 >= op2){
                diff = op1 - op2;
            }else{
                diff = op1 - op2 + (2<<7);
            }
            registers8[dest] = diff;
            is8 = true;
            change.push_back(dest);
        }else if(variables.find(source) != variables.end()){
            if(variables[source].second.second == 8){
                op1 = registers8[dest];
                op2 = variables[source].first;
                if(op1 >= op2){
                    diff = op1 - op2;
                }else{
                    diff = op1 - op2 + (2<<7);
                }
                registers8[dest] = diff;
                is8 = true;
                change.push_back(dest);
            }else{
                error = true;
            }
        }else if(source[1] == '[' && source[source.length()-1] == ']' && findAddress(source) != -1){
            if((source[0] == 'b') && (valueOfAddress(source) != -1) && (valueOfAddress(source) != -2)){
                op1 = registers8[dest];
                op2 = valueOfAddress(source);
                if(op1 >= op2){
                    diff = op1 - op2;
                }else{
                    diff = op1 - op2 + (2<<7);
                }
                registers8[dest] = diff;
                is8 = true;
                change.push_back(dest);
            }else{
                error = true;
            }
        }else if(isNumber(source)){
            int temp = decimal(source);
            if(temp >= 0 && temp < (2<<7)){
                op1 = registers8[dest];
                op2 = temp;
                if(op1 >= op2){
                    diff = op1 - op2;
                }else{
                    diff = op1 - op2 + (2<<7);
                }
                registers8[dest] = diff;
                is8 = true;
                change.push_back(dest);
            }else{
                error = true;
            }
        }else{
            error = true;
        }
    }else if(variables.find(dest) != variables.end()){
        if(registers16.find(source) != registers16.end()){
            if(variables[dest].second.second == 16){
                op1 = variables[dest].first;
                op2 = registers16[source];
                if(op1 >= op2){
                    diff = op1 - op2;
                }else{
                    diff = op1 - op2 + (2<<15);
                }
                variables[dest].first = diff;
                mem[variables[dest].second.first+1] = diff / (2<<7);
                mem[variables[dest].second.first] = diff % (2<<7);
            }else{
                error = true;
            }
        }else if(registers8.find(source) != registers8.end()){
            if(variables[dest].second.second == 8){
                op1 = variables[dest].first;
                op2 = registers8[source];
                if(op1 >= op2){
                    diff = op1 - op2;
                }else{
                    diff = op1 - op2 + (2<<7);
                }
                variables[dest].first = diff;
                is8 = true;
                mem[variables[dest].second.first] = diff;
            }else{
                error = true;
            }
        }else if(isNumber(source)){
            int temp = decimal(source);
            if(temp >= 0 && temp < (2<<7) && variables[dest].second.second == 8){
                op1 = variables[dest].first;
                op2 = temp;
                if(op1 >= op2){
                    diff = op1 - op2;
                }else{
                    diff = op1 - op2 + (2<<7);
                }
                variables[dest].first = diff;
                is8 = true;
                mem[variables[dest].second.first] = diff;
            }else if(temp >= 0 && temp < (2<<15) && variables[dest].second.second == 16){
                op1 = variables[dest].first;
                op2 = temp;
                if(op1 >= op2){
                    diff = op1 - op2;
                }else{
                    diff = op1 - op2 + (2<<15);
                }
                variables[dest].first = diff;
                mem[variables[dest].second.first+1] = diff / (2<<7);
                mem[variables[dest].second.first] = diff % (2<<7);
            }else{
                error = true;
            }
        }else{
            error = true;
        }
    }else if(dest[0] == '[' && dest[dest.length()-1] == ']'){
        if(valueOfAddress(dest) != -1 && valueOfAddress(dest) != -2){
            if(registers16.find(source) != registers16.end()){
                dest = "w" + dest;
                if(findAddress(dest) != -1){
                    op1 = valueOfAddress(dest);
                    op2 = registers16[source];
                    if(op1 >= op2){
                        diff = op1 - op2;
                    }else{
                        diff = op1 - op2 + (2<<15);
                    }
                    mem[findAddress(dest)+1] = diff / (2<<7);
                    mem[findAddress(dest)] = diff % (2<<7);
                    for(auto itr = variables.begin(); itr != variables.end(); itr++){
                        if((*itr).second.second.first == findAddress(dest) && (*itr).second.second.second == 8){
                            (*itr).second.first = diff % (2<<7);
                        }else if((*itr).second.second.first == findAddress(dest) && (*itr).second.second.second == 16){
                            (*itr).second.first = diff;
                            break;
                        }else if(((*itr).second.second.first == findAddress(dest)+1) && (*itr).second.second.second == 8){
                            (*itr).second.first = diff / (2<<7);
                        }
                    }
                }else{
                    error = true;
                }
            }else if(registers8.find(source) != registers8.end()){
                dest = "b" + dest;
                if(findAddress(dest) != -1){
                    op1 = valueOfAddress(dest);
                    op2 = registers8[source];
                    if(op1 >= op2){
                        diff = op1 - op2;
                    }else{
                        diff = op1 - op2 + (2<<7);
                    }
                    is8 = true;
                    mem[findAddress(dest)] = diff;
                    for(auto itr = variables.begin(); itr != variables.end(); itr++){
                        if((*itr).second.second.first == findAddress(dest) && (*itr).second.second.second == 8){
                            (*itr).second.first = diff;
                            break;
                        }else if(((*itr).second.second.first == findAddress(dest)-1) && (*itr).second.second.second == 16){
                            (*itr).second.first = mem[(*itr).second.second.first] + (diff*(2<<7));
                            break;
                        }else if((*itr).second.second.first == findAddress(dest) && (*itr).second.second.second == 16){
                            (*itr).second.first = diff + mem[(*itr).second.second.first+1]*(2<<7);
                            break;
                        }
                    }
                }else{
                    error = true;
                }
            }else if(isNumber(source)){
                dest = "w" + dest;
                if(findAddress(dest) != -1){
                    int temp = decimal(source);
                    if(temp >= (2<<7) && temp < (2<<15)){
                        op1 = valueOfAddress(dest);
                        op2 = temp;
                        if(op1 >= op2){
                            diff = op1 - op2;
                        }else{
                            diff = op1 - op2 + (2<<15);
                        }
                        mem[findAddress(dest)+1] = diff / (2<<7);
                        mem[findAddress(dest)] = diff % (2<<7);
                        for(auto itr = variables.begin(); itr != variables.end(); itr++){
                            if((*itr).second.second.first == findAddress(dest) && (*itr).second.second.second == 8){
                                (*itr).second.first = diff % (2<<7);
                            }else if((*itr).second.second.first == findAddress(dest) && (*itr).second.second.second == 16){
                                (*itr).second.first = diff;
                                break;
                            }else if(((*itr).second.second.first == findAddress(dest)+1) && (*itr).second.second.second == 8){
                                (*itr).second.first = diff / (2<<7);
                            }
                        }
                    }else{
                        error = true;
                    }
                }else{
                    error = true;
                }
            }else{
                error = true;
            }
        }else{
            error = true;
        }
    }else if(dest[1] == '[' && dest[dest.length()-1] == ']' & findAddress(dest) != -1){
        if(valueOfAddress(dest) != -1 && valueOfAddress(dest) != -2){
            if(registers16.find(source) != registers16.end()){
                op1 = valueOfAddress(dest);
                op2 = registers16[source];
                if(op1 >= op2){
                    diff = op1 - op2;
                }else{
                    diff = op1 - op2 + (2<<15);
                }
                mem[findAddress(dest)+1] = diff / (2<<7);
                mem[findAddress(dest)] = diff % (2<<7);
                for(auto itr = variables.begin(); itr != variables.end(); itr++){
                    if((*itr).second.second.first == findAddress(dest) && (*itr).second.second.second == 8){
                        (*itr).second.first = diff % (2<<7);
                    }else if((*itr).second.second.first == findAddress(dest) && (*itr).second.second.second == 16){
                        (*itr).second.first = diff;
                        break;
                    }else if(((*itr).second.second.first == findAddress(dest)+1) && (*itr).second.second.second == 8){
                        (*itr).second.first = diff / (2<<7);
                    }
                }
            }else if(registers8.find(source) != registers8.end()){
                op1 = valueOfAddress(dest);
                op2 = registers8[source];
                if(op1 >= op2){
                    diff = op1 - op2;
                }else{
                    diff = op1 - op2 + (2<<7);
                }
                is8 = true;
                mem[findAddress(dest)] = diff;
                for(auto itr = variables.begin(); itr != variables.end(); itr++){
                    if((*itr).second.second.first == findAddress(dest) && (*itr).second.second.second == 8){
                        (*itr).second.first = diff;
                        break;
                    }else if(((*itr).second.second.first == findAddress(dest)-1) && (*itr).second.second.second == 16){
                        (*itr).second.first = mem[(*itr).second.second.first] + (diff*(2<<7));
                        break;
                    }else if((*itr).second.second.first == findAddress(dest) && (*itr).second.second.second == 16){
                        (*itr).second.first = diff + mem[(*itr).second.second.first+1]*(2<<7);
                        break;
                    }
                }
            }else if(isNumber(dest)){
                int temp = decimal(source);
                if(temp >= 0 && temp < (2<<7) && dest[0] == 'b'){
                    op1 = valueOfAddress(dest);
                    op2 = temp;
                    if(op1 >= op2){
                        diff = op1 - op2;
                    }else{
                        diff = op1 - op2 + (2<<7);
                    }
                    is8 = true;
                    mem[findAddress(dest)] = diff;
                    for(auto itr = variables.begin(); itr != variables.end(); itr++){
                        if((*itr).second.second.first == findAddress(dest) && (*itr).second.second.second == 8){
                            (*itr).second.first = diff;
                            break;
                        }else if(((*itr).second.second.first == findAddress(dest)-1) && (*itr).second.second.second == 16){
                            (*itr).second.first = mem[(*itr).second.second.first] + (diff*(2<<7));
                            break;
                        }else if((*itr).second.second.first == findAddress(dest) && (*itr).second.second.second == 16){
                            (*itr).second.first = diff + mem[(*itr).second.second.first+1]*(2<<7);
                            break;
                        }
                    }
                }else if(temp >= 0 && temp < (2<<15) && dest[0] == 'w'){
                    op1 = valueOfAddress(dest);
                    op2 = temp;
                    if(op1 >= op2){
                        diff = op1 - op2;
                    }else{
                        diff = op1 - op2 + (2<<15);
                    }
                    mem[findAddress(dest)+1] = diff / (2<<7);
                    mem[findAddress(dest)] = diff % (2<<7);
                    for(auto itr = variables.begin(); itr != variables.end(); itr++){
                        if((*itr).second.second.first == findAddress(dest) && (*itr).second.second.second == 8){
                            (*itr).second.first = diff % (2<<7);
                        }else if((*itr).second.second.first == findAddress(dest) && (*itr).second.second.second == 16){
                            (*itr).second.first = diff;
                            break;
                        }else if(((*itr).second.second.first == findAddress(dest)+1) && (*itr).second.second.second == 8){
                            (*itr).second.first = diff / (2<<7);
                        }
                    }
                }else{
                    error = true;
                }
            }else{
                error = true;
            }
        }
    }else{
        error = true;
    }

    if(is8){            // updates flags for 8-bits operands
        if(diff == 0){
            flags["zf"] = true;
        }else{
            flags["zf"] = false;
        }
        if(op == "sub"){                // if instruction is dec, carry flag is not affected
            if(op1 < op2){
                flags["cf"] = true;
            }else{
                flags["cf"] = false;
            }
        }
        if((op1 / (2<<6) == 1 && op2 / (2<<6) == 0 && diff / (2<<6) == 0) || (op1 / (2<<6) == 0 && op2 / (2<<6) == 1 && diff / (2<<6) == 1)){
            flags["of"] = true;
        }else{
            flags["of"] = false;
        }
        if(diff / (2<<6) == 1){
            flags["sf"] = true;
        }else{
            flags["sf"] = false;
        }
        if((op1 % (2<<3) < op2 % (2<<3))){
            flags["af"] = true;
        }else{
            flags["af"] = false;
        }
    }else{                          // updates flags for 16-bits operands
        if(diff == 0){
            flags["zf"] = true;
        }else{
            flags["zf"] = false;
        }
        if(op == "sub"){                    // if instruction is dec, carry flag is not affected
            if(op1 < op2){
                flags["cf"] = true;
            }else{
                flags["cf"] = false;
            }
        }
        if((op1 / (2<<14) == 1 && op2 / (2<<14) == 0 && diff / (2<<14) == 0) || (op1 / (2<<14) == 0 && op2 / (2<<14) == 1 && diff / (2<<14) == 1)){
            flags["of"] = true;
        }else{
            flags["of"] = false;
        }
        if(diff / (2<<14) == 1){
            flags["sf"] = true;
        }else{
            flags["sf"] = false;
        }
        if((op1 % (2<<3) < op2 % (2<<3))){
            flags["af"] = true;
        }else{
            flags["af"] = false;
        }
    }

    update();
    Counter++;              // moves the counter to the next instruction

}

void mul(string source){            // mul instruction, multiply the source by an appropriate register

    long long int mul = 1;          // result of the multiplication
    bool is8 = false;               // checks if the operands are 8-bits or 16-bits

    if(registers16.find(source) != registers16.end()){
        mul = registers16["ax"] * registers16[source];
        registers16["dx"] = mul / (2<<15);
        registers16["ax"] = mul % (2<<15);
        change.push_back("ax");
        change.push_back("dx");
    }else if(registers8.find(source) != registers8.end()){
        mul = registers8["al"] * registers8[source];
        is8 = true;
        registers16["ax"] = mul;
        change.push_back("ax");
    }else if(variables.find(source) != variables.end()){
        if(variables[source].second.second == 8){
            mul = registers8["al"] * variables[source].first;
            is8 = true;
            registers16["ax"] = mul;
            change.push_back("ax");
        }else{
            mul = registers16["ax"] * variables[source].first;
            registers16["dx"] = mul / (2<<15);
            registers16["ax"] = mul % (2<<15);
            change.push_back("ax");
            change.push_back("dx");
        }
    }else if ((source[1] == '[') && (source[source.length()-1] == ']') && findAddress(source) != -1){
        if((valueOfAddress(source) != -1) && (valueOfAddress(source) != -2)){
            if(source[0] == 'b'){
                mul = registers8["al"] * valueOfAddress(source);
                is8 = true;
                registers16["ax"] = mul;
                change.push_back("ax");
            }else if(source[0] == 'w'){
                mul = registers16["ax"] * valueOfAddress(source);
                registers16["dx"] = mul / (2<<15);
                registers16["ax"] = mul % (2<<15);
                change.push_back("ax");
                change.push_back("dx");
            }
        }else{
            error = true;
        }
    }else{
        error = true;
    }

    if(is8){                    // updates flags for 8-bits operands
        if(mul / (2<<7) == 0){
            flags["cf"] = false;
            flags["of"] = false;
        }else{
            flags["cf"] = true;
            flags["of"] = true;
        }
    }else{                      // updates flags for 16-bits operands
        if(mul / (2<<15) == 0){
            flags["cf"] = false;
            flags["of"] = false;
        }else{
            flags["cf"] = true;
            flags["of"] = true;
        }
    }

    update();
    Counter++;              // moves the counter to the next instruction

}

void div(string source){            // div instruction, divide an appropriate register by the source

    long long int quotient = 1;     // quotient of the division operation

    if(registers16.find(source) != registers16.end()){
        if(registers16[source] == 0){           // error due to divisor being zero
            error = true;
        }else{
            long long int dividend = (registers16["dx"] * (2<<15)) + registers16["ax"];
            quotient = dividend / registers16[source];
            if((quotient >= 0) && (quotient < (2<<15))){
                registers16["ax"] = quotient;
                registers16["dx"] = dividend % (registers16[source]);
                change.push_back("ax");
                change.push_back("dx");
            }else{                      // error due to overflow
                error = true;
            }
        }
    }else if(registers8.find(source) != registers8.end()){
        if(registers8[source] == 0){             // error due to divisor being zero
            error = true;
        }else{
            quotient = registers16["ax"] / registers8[source];
            if((quotient >= 0) && (quotient < (2<<7))){
                registers8["al"] = quotient;
                registers8["ah"] = registers16["ax"] % (registers8[source]);
                change.push_back("al");
                change.push_back("ah");
            }else{                              // error due to overflow
                error = true;
            }
        }
    }else if (variables.find(source) != variables.end()){
        if(variables[source].second.second == 8){
            if(variables[source].first == 0){        // error due to divisor being zero
                error = true;
            }else{
                quotient = registers16["ax"] / variables[source].first;
                if((quotient >= 0) && (quotient < (2<<7))){
                    registers8["al"] = quotient;
                    registers8["ah"] = registers16["ax"] % (variables[source].first);
                    change.push_back("al");
                    change.push_back("ah");
                }else{                      // error due to overflow
                    error = true;
                }
            }
        }else{
            if(variables[source].first == 0){        // error due to divisor being zero
                error = true;
            }else{
                long long int dividend = (registers16["dx"] * (2<<15)) + registers16["ax"];
                quotient = dividend / variables[source].first;
                if((quotient >= 0) && (quotient < (2<<15))){
                    registers8["ax"] = quotient;
                    registers8["dx"] = dividend % (variables[source].first);
                    change.push_back("ax");
                    change.push_back("dx");
                }else{                              // error due to overflow
                    error = true;
                }
            }
        }
    }else if ((source[1] == '[') && (source[source.length()-1] == ']') && findAddress(source) != -1){
        if((valueOfAddress(source) != -1) && (valueOfAddress(source) != -2)){
            if(source[0] == 'b'){
                if(valueOfAddress(source) == 0){             // error due to divisor being zero
                    error = true;
                }else{
                    quotient = registers16["ax"] / valueOfAddress(source);
                    if((quotient >= 0) && (quotient < (2<<7))){
                        registers8["al"] = quotient;
                        registers8["ah"] = registers16["ax"] % (valueOfAddress(source));
                        change.push_back("al");
                        change.push_back("ah");
                    }else{                              // error due to overflow
                        error = true;
                    }
                }
            }else{
                if(valueOfAddress(source) == 0){         // error due to divisor being zero
                    error = true;
                }else{
                    long long int dividend = (registers16["dx"] * (2<<15)) + registers16["ax"];
                    quotient = dividend / valueOfAddress(source);
                    if((quotient >= 0) && (quotient < (2<<15))){
                        registers8["ax"] = quotient;
                        registers8["dx"] = dividend % (valueOfAddress(source));
                        change.push_back("ax");
                        change.push_back("dx");
                    }else{                              // error due to overflow
                        error = true;
                    }
                }
            }
        }else{
            error = true;
        }
    }else{
        error = true;
    }

    update();
    Counter++;                  // moves the counter to the next instruction

}

void binaryOp(string dest, string source, string op){       // and, or and xor operations, executes the operation and puts the result into the dest

    int result = 0;         // result of the operation
    bool is8 = false;       // checks whether the operands are 8-bits or 16-bits

    if(registers16.find(dest) != registers16.end()){
        if((source[0] == '[') && (source[source.length()-1] == ']')){
            source = "w" + source;
        }
        if(registers16.find(source) != registers16.end()){
            if(op == "xor"){
                result = registers16[dest] ^ registers16[source];
            }else if (op == "or"){
                result = registers16[dest] | registers16[source];
            }else if (op == "and"){
                result = registers16[dest] & registers16[source];
            }
            registers16[dest] = result;
            change.push_back(dest);
        }else if (variables.find(source) != variables.end()){
            if(variables[source].second.second == 16){
                if(op == "xor"){
                    result = registers16[dest] ^ variables[source].first;
                }else if (op == "or"){
                    result = registers16[dest] | variables[source].first;
                }else if (op == "and"){
                    result = registers16[dest] & variables[source].first;
                }
                registers16[dest] = result;
                change.push_back(dest);
            }else {
                error = true;
            }
        }else if ((source[1] == '[') && (source[source.length()-1] == ']') && findAddress(source) != -1){
            if((source[0] == 'w') && (valueOfAddress(source) != -1) && (valueOfAddress(source) != -2)){
                if(op == "xor"){
                    result = registers16[dest] ^ valueOfAddress(source);
                }else if (op == "or"){
                    result = registers16[dest] | valueOfAddress(source);
                }else if (op == "and"){
                    result = registers16[dest] & valueOfAddress(source);
                }
                registers16[dest] = result;
                change.push_back(dest);
            }else {
                error = true;
            }
        }else if (isNumber(source)){
            int temp = decimal(source);
            if((temp >= 0 ) && (temp < (2<<15))){
                if(op == "xor"){
                    result = registers16[dest] ^ temp;
                }else if (op == "or"){
                    result = registers16[dest] | temp;
                }else if (op == "and"){
                    result = registers16[dest] & temp;
                }
                registers16[dest] = result;
                change.push_back(dest);
            }
        }else{
            error = true;
        }
    }else if (registers8.find(dest) != registers8.end()){
        is8 = true;
        if((source[0] == '[') && (source[source.length()-1] == ']')){
            source = "b" + source;
        }
        if(registers8.find(source) != registers8.end()){
            if(op == "xor"){
                result = registers8[dest] ^ registers8[source];
            }else if (op == "or"){
                result = registers8[dest] | registers8[source];
            }else if (op == "and"){
                result = registers8[dest] & registers8[source];
            }
            registers8[dest] = result;
            change.push_back(dest);
        }else if (variables.find(source) != variables.end() ){
            if(variables[source].second.second == 8){
                if(op == "xor"){
                    result = registers8[dest] ^ variables[source].first;
                }else if (op == "or"){
                    result = registers8[dest] | variables[source].first;
                }else if (op == "and"){
                    result = registers8[dest] & variables[source].first;
                }
                registers8[dest] = result;
                change.push_back(dest);
            }else {
                error = true;
            }
        }else if ((source[1] == '[') && (source[source.length()-1] == ']') && (findAddress(source) != -1)){
            if((source[0] == 'b') && (valueOfAddress(source) != -1) && (valueOfAddress(source) != -2)){
                if(op == "xor"){
                    result = registers8[dest] ^ valueOfAddress(source);
                }else if (op == "or"){
                    result = registers8[dest] | valueOfAddress(source);
                }else if (op == "and"){
                    result = registers8[dest] & valueOfAddress(source);
                }
                registers8[dest] = result;
                change.push_back(dest);
            }else {
                error = true;
            }
        }else if (isNumber(source)){
            int temp = decimal(source);
            if((temp >= 0) && (temp < (2<<7))){
                if(op == "xor"){
                    result = registers8[dest] ^ temp;
                }else if (op == "or"){
                    result = registers8[dest] | temp;
                }else if (op == "and"){
                    result = registers8[dest] & temp;
                }
                registers8[dest] = result;
                change.push_back(dest);
            }else{
                error = true;
            }
        }else{
            error = true;
        }
    }else if (variables.find(dest) != variables.end()){
        if(registers16.find(source) != registers16.end()){
            if(variables[dest].second.second == 16){
                if(op == "xor"){
                    result = variables[dest].first ^ registers16[source];
                }else if (op == "or"){
                    result = variables[dest].first | registers16[source];
                }else if (op == "and"){
                    result = variables[dest].first & registers16[source];
                }
                variables[dest].first = result;
                mem[variables[dest].second.first+1] = result / (2<<7);
                mem[variables[dest].second.first] = result % (2<<7);
            }else{
                error = true;
            }
        }else if (registers8.find(source) != registers8.end()){
            is8 = true;
            if(variables[dest].second.second == 8){
                if(op == "xor"){
                    result = variables[dest].first ^ registers8[source];
                }else if (op == "or"){
                    result = variables[dest].first | registers8[source];
                }else if (op == "and"){
                    result = variables[dest].first & registers8[source];
                }
                variables[dest].first = result;
                mem[variables[dest].second.first] = result;
            }else{
                error = true;
            }
        }else if (isNumber(source)){
            int temp = decimal(source);
            if((temp >= 0) && (temp < (2<<15)) && variables[dest].second.second == 16){
                if(op == "xor"){
                    result = variables[dest].first ^ temp;
                }else if (op == "or"){
                    result = variables[dest].first | temp;
                }else if (op == "and"){
                    result = variables[dest].first & temp;
                }
                variables[dest].first = result;
                mem[variables[dest].second.first+1] = result / (2<<7);
                mem[variables[dest].second.first] = result % (2<<7);
            }else if ((temp >= 0) && (temp < (2<<7)) && variables[dest].second.second == 8){
                is8 = true;
                if(op == "xor"){
                    result = variables[dest].first ^ temp;
                }else if (op == "or"){
                    result = variables[dest].first | temp;
                }else if (op == "and"){
                    result = variables[dest].first & temp;
                }
                variables[dest].first = result;
                mem[variables[dest].second.first] = result;
            }else {
                error = true;
            }
        }else {
            error = true;
        }
    }else if((dest[0] == '[') && (dest[dest.length()-1] == ']')){
        if((valueOfAddress(dest) != -1) && (valueOfAddress(dest) != -2)){
            if(registers16.find(source) != registers16.end() ){
                dest = "w" + dest;
                if(findAddress(dest) != -1){
                    if(op == "xor"){
                        result = valueOfAddress(dest) ^ registers16[source];
                    }else if (op == "or"){
                        result = valueOfAddress(dest) | registers16[source];
                    }else if (op == "and"){
                        result = valueOfAddress(dest) & registers16[source];
                    }
                    mem[findAddress(dest)+1] = result / (2<<7);
                    mem[findAddress(dest)] = result % (2<<7);
                    for(auto itr = variables.begin(); itr != variables.end(); itr++){
                        if((*itr).second.second.first == findAddress(dest) && (*itr).second.second.second == 8){
                            (*itr).second.first = result % (2<<7);
                        }else if((*itr).second.second.first == findAddress(dest) && (*itr).second.second.second == 16){
                            (*itr).second.first = result;
                            break;
                        }else if(((*itr).second.second.first == findAddress(dest)+1) && (*itr).second.second.second == 8){
                            (*itr).second.first = result / (2<<7);
                        }
                    }
                }else{
                    error = true;
                }
            }else if (registers8.find(source) != registers8.end()){
                dest = "b" + dest;
                if(findAddress(dest) != -1){
                    is8 = true;
                    if(op == "xor"){
                        result = valueOfAddress(dest) ^ registers8[source];
                    }else if (op == "or"){
                        result = valueOfAddress(dest) | registers8[source];
                    }else if (op == "and"){
                        result = valueOfAddress(dest) & registers8[source];
                    }
                    mem[findAddress(dest)] = result;
                    for(auto itr = variables.begin(); itr != variables.end(); itr++){
                        if((*itr).second.second.first == findAddress(dest) && (*itr).second.second.second == 8){
                            (*itr).second.first = result;
                            break;
                        }else if(((*itr).second.second.first == findAddress(dest)-1) && (*itr).second.second.second == 16){
                            (*itr).second.first = mem[(*itr).second.second.first] + (result*(2<<7));
                            break;
                        }else if((*itr).second.second.first == findAddress(dest) && (*itr).second.second.second == 16){
                            (*itr).second.first = result + mem[(*itr).second.second.first+1]*(2<<7);
                            break;
                        }
                    }
                }else{
                    error = true;
                }
            }else if (isNumber(source)){
                int temp = decimal(source);
                dest = "w" + dest;
                if(findAddress(dest) != -1){
                    if ((temp >= (2<<7)) && (temp<(2<<15))){
                        if(op == "xor"){
                            result = valueOfAddress(dest) ^ temp;
                        }else if (op == "or"){
                            result = valueOfAddress(dest) | temp;
                        }else if (op == "and"){
                            result = valueOfAddress(dest) & temp;
                        }
                        mem[findAddress(dest)+1] = result / (2<<7);
                        mem[findAddress(dest)] = result % (2<<7);
                        for(auto itr = variables.begin(); itr != variables.end(); itr++){
                            if((*itr).second.second.first == findAddress(dest) && (*itr).second.second.second == 8){
                                (*itr).second.first = result % (2<<7);
                            }else if((*itr).second.second.first == findAddress(dest) && (*itr).second.second.second == 16){
                                (*itr).second.first = result;
                                break;
                            }else if(((*itr).second.second.first == findAddress(dest)+1) && (*itr).second.second.second == 8){
                                (*itr).second.first = result / (2<<7);
                            }
                        }
                    }else {
                        error = true;
                    }
                }else{
                    error = true;
                }
            }else {
                error = true;
            }
        }else{
            error = true;
        }
    }else if ((dest[1] == '[') && (dest[dest.length()-1] == ']') && findAddress(dest) != -1){
        if((valueOfAddress(dest) != -1) && (valueOfAddress(dest) != -2)){
            if(registers16.find(source) != registers16.end() ){
                if(dest[0] == 'w'){
                    if(op == "xor"){
                        result = valueOfAddress(dest) ^ registers16[source];
                    }else if (op == "or"){
                        result = valueOfAddress(dest) | registers16[source];
                    }else if (op == "and"){
                        result = valueOfAddress(dest) & registers16[source];
                    }
                    mem[findAddress(dest)+1] = result / (2<<7);
                    mem[findAddress(dest)] = result % (2<<7);
                    for(auto itr = variables.begin(); itr != variables.end(); itr++){
                        if((*itr).second.second.first == findAddress(dest) && (*itr).second.second.second == 8){
                            (*itr).second.first = result % (2<<7);
                        }else if((*itr).second.second.first == findAddress(dest) && (*itr).second.second.second == 16){
                            (*itr).second.first = result;
                            break;
                        }else if(((*itr).second.second.first == findAddress(dest)+1) && (*itr).second.second.second == 8){
                            (*itr).second.first = result / (2<<7);
                        }
                    }
                }else{
                    error = true;
                }
            }else if (registers8.find(source) != registers8.end()){
                is8 = true;
                if(dest[0] == 'b'){
                    if(op == "xor"){
                        result = valueOfAddress(dest) ^ registers8[source];
                    }else if (op == "or"){
                        result = valueOfAddress(dest) | registers8[source];
                    }else if (op == "and"){
                        result = valueOfAddress(dest) & registers8[source];
                    }
                    mem[findAddress(dest)] = result;
                    for(auto itr = variables.begin(); itr != variables.end(); itr++){
                        if((*itr).second.second.first == findAddress(dest) && (*itr).second.second.second == 8){
                            (*itr).second.first = result;
                            break;
                        }else if(((*itr).second.second.first == findAddress(dest)-1) && (*itr).second.second.second == 16){
                            (*itr).second.first = mem[(*itr).second.second.first] + (result*(2<<7));
                            break;
                        }else if((*itr).second.second.first == findAddress(dest) && (*itr).second.second.second == 16){
                            (*itr).second.first = result + mem[(*itr).second.second.first+1]*(2<<7);
                            break;
                        }
                    }
                }else{
                    error = true;
                }
            }else if (isNumber(source)){
                int temp = decimal(source);
                if((temp >= 0) && (temp<(2<<15)) && dest[0] == 'w'){
                    if(op == "xor"){
                        result = valueOfAddress(dest) ^ temp;
                    }else if (op == "or"){
                        result = valueOfAddress(dest) | temp;
                    }else if (op == "and"){
                        result = valueOfAddress(dest) & temp;
                    }
                    mem[findAddress(dest)+1] = result / (2<<7);
                    mem[findAddress(dest)] = result % (2<<7);
                    for(auto itr = variables.begin(); itr != variables.end(); itr++){
                        if((*itr).second.second.first == findAddress(dest) && (*itr).second.second.second == 8){
                            (*itr).second.first = result % (2<<7);
                        }else if((*itr).second.second.first == findAddress(dest) && (*itr).second.second.second == 16){
                            (*itr).second.first = result;
                            break;
                        }else if(((*itr).second.second.first == findAddress(dest)+1) && (*itr).second.second.second == 8){
                            (*itr).second.first = result / (2<<7);
                        }
                    }
                }else if ((temp >= 0) && (temp<(2<<7)) && dest[0] == 'b'){
                    is8 = true;
                    if(op == "xor"){
                        result = valueOfAddress(dest) ^ temp;
                    }else if (op == "or"){
                        result = valueOfAddress(dest) | temp;
                    }else if (op == "and"){
                        result = valueOfAddress(dest) & temp;
                    }
                    mem[findAddress(dest)] = result;
                    for(auto itr = variables.begin(); itr != variables.end(); itr++){
                        if((*itr).second.second.first == findAddress(dest) && (*itr).second.second.second == 8){
                            (*itr).second.first = result;
                            break;
                        }else if(((*itr).second.second.first == findAddress(dest)-1) && (*itr).second.second.second == 16){
                            (*itr).second.first = mem[(*itr).second.second.first] + (result*(2<<7));
                            break;
                        }else if((*itr).second.second.first == findAddress(dest) && (*itr).second.second.second == 16){
                            (*itr).second.first = result + mem[(*itr).second.second.first+1]*(2<<7);
                            break;
                        }
                    }
                }else {
                    error = true;
                }
            }else{
                error = true;
            }
        }else{
            error = true;
        }
    }else{
        error = true;
    }

    flags["cf"] = false;        // carry and overflow flags are cleared
    flags["of"] = false;
    if(result == 0){
        flags["zf"] = true;
    }else{
        flags["zf"] = false;
    }
    if(is8){                    // updates sign flag for 8-bits operands
        if(result / (2<<6) == 1){
            flags["sf"] = 1;
        }else{
            flags["sf"] = 0;
        }
    }else{                       // updates sign flag for 16-bits operands
        if(result / (2<<14) == 1){
            flags["sf"] = 1;
        }else{
            flags["sf"] = 0;
        }
    }

    update();
    Counter++;              // moves the counter to the next instruction

}

void opnot(string source){          // not instruction, performs a bitwise not operation and stores the result into the source

    if(registers16.find(source) != registers16.end()){
        registers16[source] = ((2<<15)-1) - registers16[source];
        change.push_back(source);
    }else if(registers8.find(source) != registers8.end()){
        registers8[source] = ((2<<7)-1) - registers8[source];
        change.push_back(source);
    }else if (variables.find(source) != variables.end()){
        if(variables[source].second.second == 8){
            variables[source].first = ((2<<7)-1) - variables[source].first;
            mem[variables[source].second.first] = variables[source].first;
        }else{
            variables[source].first = ((2<<15)-1) - variables[source].first;
            mem[variables[source].second.first+1] = variables[source].first / (2<<7);
            mem[variables[source].second.first] = variables[source].first % (2<<7);
        }
    }else if ((source[1] == '[') && (source[source.length()-1] == ']') && findAddress(source) != -1){
        if((valueOfAddress(source) != -1) && (valueOfAddress(source) != -2)){
            if(source[0] == 'b'){
                mem[findAddress(source)] = ((2<<7)-1) - valueOfAddress(source);
                for(auto itr = variables.begin(); itr != variables.end(); itr++){
                    if((*itr).second.second.first == findAddress(source) && (*itr).second.second.second == 8){
                        (*itr).second.first = mem[findAddress(source)];
                        break;
                    }else if(((*itr).second.second.first == findAddress(source)-1) && (*itr).second.second.second == 16){
                        (*itr).second.first = mem[(*itr).second.second.first] + (mem[findAddress(source)]*(2<<7));
                        break;
                    }else if((*itr).second.second.first == findAddress(source) && (*itr).second.second.second == 16){
                        (*itr).second.first = mem[findAddress(source)] + mem[(*itr).second.second.first+1]*(2<<7);
                        break;
                    }
                }
            }else{
                mem[findAddress(source)+1] = ((2<<7)-1) - mem[findAddress(source)+1];
                mem[findAddress(source)] = ((2<<7)-1) - mem[findAddress(source)];
                for(auto itr = variables.begin(); itr != variables.end(); itr++){
                    if((*itr).second.second.first == findAddress(source) && (*itr).second.second.second == 8){
                        (*itr).second.first = mem[findAddress(source)];
                    }else if((*itr).second.second.first == findAddress(source) && (*itr).second.second.second == 16){
                        (*itr).second.first = mem[findAddress(source)] + (mem[findAddress(source)+1]*(2<<7));
                        break;
                    }else if(((*itr).second.second.first == findAddress(source)+1) && (*itr).second.second.second == 8){
                        (*itr).second.first = mem[findAddress(source)+1];
                    }
                }
            }
        }else{
            error = true;
        }
    }else{
        error = true;
    }

    update();
    Counter++;          // moves the counter to the next instruction

}

void shift(string dest,string source, string op){      // shl, shr, rcl, rcr instructions, executes the operation and puts the result into the dest

    int result = 0;     // result of the operation
    int oper = 0;       // operand
    int count = 0;      // number of shifts
    bool is8 = false;   // checks whether the operand is 8-bits or 16-bits

    if(source == "cl"){                         // calculate the count variable
        count = registers8["cl"];
    }else if (isNumber(source)){
        if((decimal(source) >=0 ) && (decimal(source) < (2<<7))){
            count = decimal(source);
        }
    }else{
        error = true;
    }

    if(count != 0){                            // if count is zero dest and flags will not change, so just move to the next instruction
        if(registers16.find(dest) != registers16.end()){
            oper = registers16[dest];
            if(op == "shl"){
                long long int temp = registers16[dest] << count;
                registers16[dest] = temp % (2<<15);
            }else if (op == "shr"){
                registers16[dest] = registers16[dest] >> count;
            }else if (op == "rcl"){
                int cnt = count % 17;
                if(cnt != 0){               // if count is 17 or multiple of it, result and carry flag is not affected
                    int mynum = registers16[dest] + (flags["cf"]*(2<<15));
                    int left = mynum / (2<<(16-cnt));
                    int right = mynum %(2<<(16-cnt));
                    mynum = (right*(2<<(cnt-1)))+left;
                    registers16[dest] = mynum % (2<<15);
                    flags["cf"] = mynum / (2<<15);
                }
            }else if (op == "rcr"){
                int cnt = count % 17;
                if(cnt != 0){               // if count is 17 or multiple of it, result and carry flag is not affected
                    int mynum = registers16[dest] + (flags["cf"]*(2<<15));
                    int right = mynum % (2<<(cnt-1));
                    int left = mynum / (2<<(cnt-1));
                    mynum = (right*(2<<(16-cnt)))+left;
                    registers16[dest] = mynum % (2<<15);
                    flags["cf"] = mynum / (2<<15);
                }
            }else {
                error = true;
            }
            result = registers16[dest];
            change.push_back(dest);
        }else if (registers8.find(dest) != registers8.end()){
            oper = registers8[dest];
            if(op == "shl"){
                long long int temp = registers8[dest] << count;
                registers8[dest] = temp % (2<<7);
            }else if (op == "shr"){
                registers8[dest] = registers8[dest] >> count;
            }else if (op == "rcl"){
                int cnt = count % 9;            // if count is 9 or multiple of it, result and carry flag is not affected
                if(cnt != 0){
                    int mynum = registers8[dest] + (flags["cf"]*(2<<7));
                    int left = mynum / (2<<(8-cnt));
                    int right = mynum % (2<<(8-cnt));
                    mynum = (right*(2<<(cnt-1)))+left;
                    registers8[dest] = mynum % (2<<7);
                    flags["cf"] = mynum / (2<<7);
                }
            }else if (op == "rcr"){
                int cnt = count % 9;            // if count is 9 or multiple of it, result and carry flag is not affected
                if(cnt != 0){
                    int mynum = registers8[dest] + (flags["cf"]*(2<<7));
                    int right = mynum % (2<<(cnt-1));
                    int left = mynum / (2<<(cnt-1));
                    mynum = (right*(2<<(8-cnt)))+left;
                    registers8[dest] = mynum % (2<<7);
                    flags["cf"] = mynum / (2<<7);
                }
            }else {
                error = true;
            }
            is8 = true;
            result = registers8[dest];
            change.push_back(dest);
        }else if (variables.find(dest) != variables.end()){
            if(variables[dest].second.second == 16){
                oper = variables[dest].first;
                if(op == "shl"){
                    long long int temp = variables[dest].first << count;
                    variables[dest].first = temp % (2<<15);
                }else if (op == "shr"){
                    variables[dest].first = variables[dest].first >> count;
                }else if (op == "rcl"){
                    int cnt = count % 17;           // if count is 17 or multiple of it, result and carry flag is not affected
                    if(cnt != 0){
                        int mynum = variables[dest].first + (flags["cf"]*(2<<15));
                        int left = mynum / (2<<(16-cnt));
                        int right = mynum %(2<<(16-cnt));
                        mynum = (right*(2<<(cnt-1)))+left;
                        variables[dest].first = mynum % (2<<15);
                        flags["cf"] = mynum / (2<<15);
                    }
                }else if (op == "rcr"){
                    int cnt = count % 17;           // if count is 17 or multiple of it, result and carry flag is not affected
                    if(cnt != 0){
                        int mynum = variables[dest].first + (flags["cf"]*(2<<15));
                        int right = mynum % (2<<(cnt-1));
                        int left = mynum / (2<<(cnt-1));
                        mynum = (right*(2<<(16-cnt)))+left;
                        variables[dest].first = mynum % (2<<15);
                        flags["cf"] = mynum / (2<<15);
                    }
                }else {
                    error = true;
                }
                result = variables[dest].first;
                mem[variables[dest].second.first+1] = result / (2<<7);
                mem[variables[dest].second.first] = result % (2<<7);
            }else{
                oper = variables[dest].first;
                if(op == "shl"){
                    long long int temp = variables[dest].first << count;
                    variables[dest].first = temp % (2<<7);
                }else if (op == "shr"){
                    variables[dest].first = variables[dest].first >> count;
                }else if (op == "rcl"){
                    int cnt = count % 9;                // if count is 9 or multiple of it, result and carry flag is not affected
                    if(cnt != 0){
                        int mynum = variables[dest].first + (flags["cf"]*(2<<7));
                        int left = mynum / (2<<(8-cnt));
                        int right = mynum %(2<<(8-cnt));
                        mynum = (right*(2<<(cnt-1)))+left;
                        variables[dest].first = mynum % (2<<7);
                        flags["cf"] = mynum / (2<<7);
                    }
                }else if (op == "rcr"){
                    int cnt = count % 9;                // if count is 9 or multiple of it, result and carry flag is not affected
                    if(cnt != 0){
                        int mynum = variables[dest].first + (flags["cf"]*(2<<7));
                        int right = mynum % (2<<(cnt-1));
                        int left = mynum / (2<<(cnt-1));
                        mynum = (right*(2<<(8-cnt)))+left;
                        variables[dest].first = mynum % (2<<7);
                        flags["cf"] = mynum / (2<<7);
                    }
                }else {
                    error = true;
                }
                is8 = true;
                result = variables[dest].first;
                mem[variables[dest].second.first] = result;
            }
        }else if((dest[1] == '[') && (dest[dest.length()-1] == ']') && (findAddress(dest) != -1)){
            if((valueOfAddress(dest) != -1) && (valueOfAddress(dest) != -2)){
                if(dest[0] == 'b'){
                    oper = valueOfAddress(dest);
                    if(op == "shl"){
                        long long int temp = valueOfAddress(dest) << count;
                        mem[findAddress(dest)] = temp % (2<<7);
                    }else if (op == "shr"){
                        mem[findAddress(dest)] = valueOfAddress(dest) >> count;
                    }else if (op == "rcl"){
                        int cnt = count % 9;        // if count is 9 or multiple of it, result and carry flag is not affected
                        if(cnt != 0){
                            int mynum = valueOfAddress(dest) + (flags["cf"]*(2<<7));
                            int left = mynum / (2<<(8-cnt));
                            int right = mynum %(2<<(8-cnt));
                            mynum = (right*(2<<(cnt-1)))+left;
                            mem[findAddress(dest)] = mynum % (2<<7);
                            flags["cf"] = mynum / (2<<7);
                        }
                    }else if (op == "rcr"){
                        int cnt = count % 9;        // if count is 9 or multiple of it, result and carry flag is not affected
                        if(cnt != 0){
                            int mynum = valueOfAddress(dest) + (flags["cf"]*(2<<7));
                            int right = mynum % (2<<(cnt-1));
                            int left = mynum / (2<<(cnt-1));
                            mynum = (right*(2<<(8-cnt)))+left;
                            mem[findAddress(dest)] = mynum % (2<<7);
                            flags["cf"] = mynum / (2<<7);
                        }
                    }else {
                        error = true;
                    }
                    is8 = true;
                    result = valueOfAddress(dest);
                    for(auto itr = variables.begin(); itr != variables.end(); itr++){
                        if((*itr).second.second.first == findAddress(source) && (*itr).second.second.second == 8){
                            (*itr).second.first = result;
                            break;
                        }else if(((*itr).second.second.first == findAddress(source)-1) && (*itr).second.second.second == 16){
                            (*itr).second.first = mem[(*itr).second.second.first] + (result*(2<<7));
                            break;
                        }else if((*itr).second.second.first == findAddress(source) && (*itr).second.second.second == 16){
                            (*itr).second.first = result + mem[(*itr).second.second.first+1]*(2<<7);
                            break;
                        }
                    }
                }else{
                    oper = (mem[findAddress(dest)+1]*(2<<7)) + mem[findAddress(dest)];
                    if(op == "shl"){
                        long long int temp = oper << count;
                        mem[findAddress(dest)+1] = (temp % (2<<15)) / (2<<7);
                        mem[findAddress(dest)] = (temp % (2<<15)) % (2<<7);
                    }else if (op == "shr"){
                        mem[findAddress(dest)+1] = (oper >> count) / (2<<7);
                        mem[findAddress(dest)] = (oper >> count) % (2<<7);
                    }else if (op == "rcl"){
                        int cnt = count % 17;           // if count is 17 or multiple of it, result and carry flag is not affected
                        if(cnt != 0){
                            int mynum = oper + (flags["cf"]*(2<<15));
                            int left = mynum / (2<<(16-cnt));
                            int right = mynum % (2<<(16-cnt));
                            mynum = (right*(2<<(cnt-1)))+left;
                            mem[findAddress(dest)+1] = (mynum % (2<<15)) / (2<<7);
                            mem[findAddress(dest)] = (mynum % (2<<15)) % (2<<7);
                            flags["cf"] = mynum / (2<<15);
                        }
                    }else if (op == "rcr"){
                        int cnt = count % 17;           // if count is 17 or multiple of it, result and carry flag is not affected
                        if(cnt != 0){
                            int mynum = oper + (flags["cf"]*(2<<15));
                            int right = mynum % (2<<(cnt-1));
                            int left = mynum / (2<<(cnt-1));
                            mynum = (right*(2<<(16-cnt)))+left;
                            mem[findAddress(dest)+1] = (mynum % (2<<15)) / (2<<7);
                            mem[findAddress(dest)] = (mynum % (2<<15)) % (2<<7);
                            flags["cf"] = mynum / (2<<15);
                        }
                    }else {
                        error = true;
                    }
                    result = (mem[findAddress(dest)+1]*(2<<7)) + mem[findAddress(dest)];
                    for(auto itr = variables.begin(); itr != variables.end(); itr++){
                        if((*itr).second.second.first == findAddress(source) && (*itr).second.second.second == 8){
                            (*itr).second.first = result % (2<<7);
                        }else if((*itr).second.second.first == findAddress(source) && (*itr).second.second.second == 16){
                            (*itr).second.first = result;
                            break;
                        }else if(((*itr).second.second.first == findAddress(source)+1) && (*itr).second.second.second == 8){
                            (*itr).second.first = result / (2<<7);
                        }
                    }
                }
            }else{
                error = true;
            }
        }else {
            error = true;
        }

        if(op == "shl" || op == "shr"){     // updates flags for shl and shr instructions
            if(result == 0){
                flags["zf"] = true;
            }else{
                flags["zf"] = false;
            }
            if((is8 && (result / (2<<6) == 1)) || (!is8 && (result / (2<<14) == 1))){
                flags["sf"] = true;
            }
            else{
                flags["sf"] = false;
            }
            if(is8){                        // update flags for 8-bits operand
                if(count < 8 && op == "shl"){
                    int temp = oper / (2<<(7-count));
                    if(temp % 2 == 1){
                        flags["cf"] = true;
                    }else{
                        flags["cf"] = false;
                    }
                }else if(count < 8 && op == "shr"){
                    if(count == 1){
                        if(oper % 2 == 1){
                            flags["cf"] = true;
                        }else{
                            flags["cf"] = false;
                        }
                    }else{
                        int temp = oper / (2<<(count-2));
                        if(temp % 2 == 1){
                            flags["cf"] = true;
                        }else{
                            flags["cf"] = false;
                        }
                    }
                }
            }
            else{                       // update flags for 16-bits operand
                if(count < 16 && op == "shl"){
                    int temp = oper / (2<<(15-count));
                    if(temp % 2 == 1){
                        flags["cf"] = true;
                    }else{
                        flags["cf"] = false;
                    }
                }else if(count < 16 && op == "shr"){
                    if(count == 1){
                        if(oper % 2 == 1){
                            flags["cf"] = true;
                        }else{
                            flags["cf"] = false;
                        }
                    }else{
                        int temp = oper / (2<<(count-2));
                        if(temp % 2 == 1){
                            flags["cf"] = true;
                        }else{
                            flags["cf"] = false;
                        }
                    }
                }
            }
            if(count == 1){             // overflow flag can be affected only if count is one
                if(op == "shl" && is8){
                    int msb = oper / (2<<6);
                    int msb2 = (oper / (2<<5)) % 2;
                    if(msb != msb2){
                        flags["of"] = true;
                    }else{
                        flags["of"] = false;
                    }
                }else if(op == "shl" && !is8){
                    int msb = oper / (2<<14);
                    int msb2 = (oper / (2<<13)) % 2;
                    if(msb != msb2){
                        flags["of"] = true;
                    }else{
                        flags["of"] = false;
                    }
                }else if(op == "shr" && is8){
                    if(oper / (2<<6) == 1){
                        flags["of"] = true;
                    }else{
                        flags["of"] = false;
                    }
                }else if(op == "shr" && !is8){
                    if(oper / (2<<14) == 1){
                        flags["of"] = true;
                    }else{
                        flags["of"] = false;
                    }
                }
            }
        }else{                      // updates flags for rcl and rcr instructions
            if(count == 1){          // overflow flag can be affected only if count is one
                if(op == "rcl" && is8){
                    int msb = oper / (2<<6);
                    int msb2 = (oper / (2<<5)) % 2;
                    if(msb != msb2){
                        flags["of"] = true;
                    }else{
                        flags["of"] = false;
                    }
                }else if(op == "rcl" && !is8){
                    int msb = oper / (2<<14);
                    int msb2 = (oper / (2<<13)) % 2;
                    if(msb != msb2){
                        flags["of"] = true;
                    }else{
                        flags["of"] = false;
                    }
                }else if(op == "rcr" && is8){
                    int msb = result / (2<<6);
                    int msb2 = (result / (2<<5)) % 2;
                    if(msb != msb2){
                        flags["of"] = true;
                    }else{
                        flags["of"] = false;
                    }
                }else if(op == "rcr" && !is8){
                    int msb = result / (2<<14);
                    int msb2 = (result / (2<<13)) % 2;
                    if(msb != msb2){
                        flags["of"] = true;
                    }else{
                        flags["of"] = false;
                    }
                }
            }
        }
        update();
    }

    Counter++;              // moves the counter to the next instruction

}

void nop(){                 // nop instruction, just moves to the next instruction
    Counter++;
}

void push(string source){       // push instruction, pushes source into the stack

    int result = 0;             // the value that will be pushed

    if((source[0] == '[') && (source[source.length()-1] == ']')){
        source = "w" + source;
    }
    if(registers16.find(source) != registers16.end()){
        result = registers16[source];
    }else if (variables.find(source) != variables.end()){
        if(variables[source].second.second == 16){
            result = variables[source].first;
        }else {
            error = true;
        }
    }else if ((source[1] == '[') && (source[source.length()-1] == ']') && (findAddress(source) != -1)){
        if((valueOfAddress(source) != -1) && (valueOfAddress(source) != -2)){
            if(source[0] == 'w'){
                result = (mem[findAddress(source)+1]*(2<<7)) + valueOfAddress(source);
            }else {
                error = true;
            }
        }else{
            error = true;
        }
    }else if (isNumber(source)){
        int temp = decimal(source);
        if(temp>=0   &&  temp  < (2<<15)){
            result = temp;
        }else {
            error = true;
        }
    }else {
        error = true;
    }

    registers16["sp"] = registers16["sp"] - 2;      // decrement the stack pointer by two, so it points to top of the stack always
    mem[registers16["sp"]+1] = result / (2<<7);
    mem[registers16["sp"]] = result % (2<<7);

    Counter++;          // moves the counter to the next instruction

}

void pop(string source){            // pop instruction, pops a value from stack and puts into the source

    if(registers16["sp"] == 65534){     // if stack is empty then gives error
        error = true;
        return;
    }

    int result = (mem[registers16["sp"]+1]*(2<<7)) + mem[registers16["sp"]];    // value that will be popped into the source

    if((source[0] == '[') && (source[source.length()-1] == ']')){
        source = "w" + source;
    }
    if(registers16.find(source) != registers16.end()){
        registers16[source] = result;
        change.push_back(source);
    }else if (variables.find(source) != variables.end()){
        if(variables[source].second.second == 16){
            variables[source].first = result;
            mem[variables[source].second.first+1] = result / (2<<7);
            mem[variables[source].second.first] = result % (2<<7);
        }else {
            error = true;
        }
    }else if ((source[1] == '[') && (source[source.length()-1] == ']') && (findAddress(source) != -1)){
        if(valueOfAddress(source) != -2){
            if(source[0] == 'w'){
                mem[findAddress(source)+1] = result / (2<<7);
                mem[findAddress(source)] = result % (2<<7);
                for(auto itr = variables.begin(); itr != variables.end(); itr++){
                    if((*itr).second.second.first == findAddress(source) && (*itr).second.second.second == 8){
                        (*itr).second.first = result % (2<<7);
                    }else if((*itr).second.second.first == findAddress(source) && (*itr).second.second.second == 16){
                        (*itr).second.first = result;
                        break;
                    }else if(((*itr).second.second.first == findAddress(source)+1) && (*itr).second.second.second == 8){
                        (*itr).second.first = result / (2<<7);
                    }
                }
            }else {
                error = true;
            }
        }else{
            error = true;
        }
    }else {
        error = true;
    }

    registers16["sp"] = registers16["sp"] + 2;      // when the value is popped, increment the stack pointer so it points to top of the stack always

    update();
    Counter++;              // moves the counter to the next instruction

}

void cmp(string dest, string source){   // cmp instruction, compares source and dest and updates flags

    int diff = 0;       // difference between source and dest
    bool is8 = false;   // checks whether the operands are 8-bits or 16-bits
    int op1 = 0;        // operand1
    int op2 = 0;        // operand2

    if(registers16.find(dest) != registers16.end()){
        if(source[0] == '[' && source[source.length()-1] == ']'){
            source = "w" + source;
        }
        if(registers16.find(source) != registers16.end()){
            op1 = registers16[dest];
            op2 = registers16[source];
            if(op1 >= op2){
                diff = op1 - op2;
            }else{
                diff = op1 - op2 + (2<<15);
            }
        }else if(variables.find(source) != variables.end()){
            if(variables[source].second.second == 8){
                error = true;
            }else{
                op1 = registers16[dest];
                op2 = variables[source].first;
                if(op1 >= op2){
                    diff = op1 - op2;
                }else{
                    diff = op1 - op2 + (2<<15);
                }
            }
        }else if(source[1] == '[' && source[source.length()-1] == ']' && findAddress(source) != -1){
            if((valueOfAddress(source) != -1) && (valueOfAddress(source) != -2)){
                op1 = registers16[dest];
                op2 = valueOfAddress(source);
                if(op1 >= op2){
                    diff = op1 - op2;
                }else{
                    diff = op1 - op2 + (2<<15);
                }
            }else{
                error = true;
            }
        }else if(isNumber(source)){
            int temp = decimal(source);
            if(temp >= 0 && temp < (2<<15)){
                op1 = registers16[dest];
                op2 = temp;
                if(op1 >= op2){
                    diff = op1 - op2;
                }else{
                    diff = op1 - op2 + (2<<15);
                }
            }else{
                error = true;
            }
        }else{
            error = true;
        }
    }else if(registers8.find(dest) != registers8.end()){
        if(source[0] == '[' && source[source.length()-1] == ']'){
            source = "b" + source;
        }
        if(registers8.find(source) != registers8.end()){
            op1 = registers8[dest];
            op2 = registers8[source];
            if(op1 >= op2){
                diff = op1 - op2;
            }else{
                diff = op1 - op2 + (2<<7);
            }
            is8 = true;
        }else if(variables.find(source) != variables.end()){
            if(variables[source].second.second == 8){
                op1 = registers8[dest];
                op2 = variables[source].first;
                if(op1 >= op2){
                    diff = op1 - op2;
                }else{
                    diff = op1 - op2 + (2<<7);
                }
                is8 = true;
            }else{
                error = true;
            }
        }else if(source[1] == '[' && source[source.length()-1] == ']' && findAddress(source) != -1){
            if((source[0] == 'b') && (valueOfAddress(source) != -1) && (valueOfAddress(source) != -2)){
                op1 = registers8[dest];
                op2 = valueOfAddress(source);
                if(op1 >= op2){
                    diff = op1 - op2;
                }else{
                    diff = op1 - op2 + (2<<7);
                }
                is8 = true;
            }else{
                error = true;
            }
        }else if(isNumber(source)){
            int temp = decimal(source);
            if(temp >= 0 && temp < (2<<7)){
                op1 = registers8[dest];
                op2 = temp;
                if(op1 >= op2){
                    diff = op1 - op2;
                }else{
                    diff = op1 - op2 + (2<<7);
                }
                is8 = true;
            }else{
                error = true;
            }
        }else{
            error = true;
        }
    }else if(variables.find(dest) != variables.end()){
        if(registers16.find(source) != registers16.end()){
            if(variables[dest].second.second == 16){
                op1 = variables[dest].first;
                op2 = registers16[source];
                if(op1 >= op2){
                    diff = op1 - op2;
                }else{
                    diff = op1 - op2 + (2<<15);
                }
            }else{
                error = true;
            }
        }else if(registers8.find(source) != registers8.end()){
            if(variables[dest].second.second == 8){
                op1 = variables[dest].first;
                op2 = registers8[source];
                if(op1 >= op2){
                    diff = op1 - op2;
                }else{
                    diff = op1 - op2 + (2<<7);
                }
                is8 = true;
            }else{
                error = true;
            }
        }else if(isNumber(source)){
            int temp = decimal(source);
            if(temp >= 0 && temp < (2<<7) && variables[dest].second.second == 8){
                op1 = variables[dest].first;
                op2 = temp;
                if(op1 >= op2){
                    diff = op1 - op2;
                }else{
                    diff = op1 - op2 + (2<<7);
                }
                is8 = true;
            }else if(temp >= 0 && temp < (2<<15) && variables[dest].second.second == 16){
                op1 = variables[dest].first;
                op2 = temp;
                if(op1 >= op2){
                    diff = op1 - op2;
                }else{
                    diff = op1 - op2 + (2<<7);
                }
            }else{
                error = true;
            }
        }else{
            error = true;
        }
    }else if(dest[0] == '[' && dest[dest.length()-1] == ']'){
        if(registers16.find(source) != registers16.end()){
            dest = "w" + dest;
            if(findAddress(dest) != -1){
                if(valueOfAddress(dest) != -1 && valueOfAddress(dest) != -2){
                    op1 = valueOfAddress(dest);
                    op2 = registers16[source];
                    if(op1 >= op2){
                        diff = op1 - op2;
                    }else{
                        diff = op1 - op2 + (2<<15);
                    }
                }else{
                    error = true;
                }
            }else{
                error = true;
            }
        }else if(registers8.find(source) != registers8.end()){
            dest = "b" + dest;
            if(findAddress(dest) != -1){
                if(valueOfAddress(dest) != -1 && valueOfAddress(dest) != -2){
                    op1 = valueOfAddress(dest);
                    op2 = registers8[source];
                    if(op1 >= op2){
                        diff = op1 - op2;
                    }else{
                        diff = op1 - op2 + (2<<7);
                    }
                    is8 = true;
                }else{
                    error = true;
                }
            }else{
                error = true;
            }
        }else if(isNumber(source)){
            dest = "w" + dest;
            if(findAddress(dest) != -1){
                if((valueOfAddress(dest) != -1) && (valueOfAddress(dest) != -2)){
                    int temp = decimal(dest);
                    if(temp >= (2<<7) && temp < (2<<15)){
                        op1 = valueOfAddress(dest);
                        op2 = temp;
                        if(op1 >= op2){
                            diff = op1 - op2;
                        }else{
                            diff = op1 - op2 + (2<<15);
                        }
                    }else{
                        error = true;
                    }
                }else{
                    error = true;
                }
            }else{
                error = true;
            }
        }else{
            error = true;
        }
    }else if(dest[1] == '[' && dest[dest.length()-1] == ']' & findAddress(dest) != -1){
        if(registers16.find(source) != registers16.end()){
            if(dest[0] == 'w'){
                if(valueOfAddress(dest) != -1 && valueOfAddress(dest) != -2){
                    op1 = valueOfAddress(dest);
                    op2 = registers16[source];
                    if(op1 >= op2){
                        diff = op1 - op2;
                    }else{
                        diff = op1 - op2 + (2<<15);
                    }
                }else{
                    error = true;
                }
            }else{
                error = true;
            }
        }else if(registers8.find(source) != registers8.end()){
            if(dest[0] == 'b'){
                if(valueOfAddress(dest) != -1 && valueOfAddress(dest) != -2){
                    op1 = valueOfAddress(dest);
                    op2 = registers8[source];
                    if(op1 >= op2){
                        diff = op1 - op2;
                    }else{
                        diff = op1 - op2 + (2<<7);
                    }
                    is8 = true;
                }else{
                    error = true;
                }
            }else{
                error = true;
            }
        }else if(isNumber(source)){
            if((valueOfAddress(dest) != -1) && (valueOfAddress(dest) != -2)){
                int temp = decimal(dest);
                if(temp >= 0 && temp < (2<<7) && dest[0] == 'b'){
                    op1 = valueOfAddress(dest);
                    op2 = temp;
                    if(op1 >= op2){
                        diff = op1 - op2;
                    }else{
                        diff = op1 - op2 + (2<<7);
                    }
                    is8 = true;
                }else if(temp >= 0 && temp < (2<<15) && dest[0] == 'w'){
                    op1 = valueOfAddress(dest);
                    op2 = temp;
                    if(op1 >= op2){
                        diff = op1 - op2;
                    }else{
                        diff = op1 - op2 + (2<<15);
                    }
                }else{
                    error = true;
                }
            }else{
                error = true;
            }
        }else{
            error = true;
        }
    }else{
        error = true;
    }

    if(is8){                    // updates flags for 8-bits operands
        if(diff == 0){
            flags["zf"] = true;
        }else{
            flags["zf"] = false;
        }
        if(op1 < op2){
            flags["cf"] = true;
        }else{
            flags["cf"] = false;
        }
        if((op1 / (2<<6) == 1 && op2 / (2<<6) == 0 && diff / (2<<6) == 0) || (op1 / (2<<6) == 0 && op2 / (2<<6) == 1 && diff / (2<<6) == 1)){
            flags["of"] = true;
        }else{
            flags["of"] = false;
        }
        if(diff / (2<<6) == 1){
            flags["sf"] = true;
        }else{
            flags["sf"] = false;
        }
        if((op1 % (2<<3) < op2 % (2<<3))){
            flags["af"] = true;
        }else{
            flags["af"] = false;
        }
    }else{                           // updates flags for 16-bits operands
        if(diff == 0){
            flags["zf"] = true;
        }else{
            flags["zf"] = false;
        }
        if(op1 < op2){
            flags["cf"] = true;
        }else{
            flags["cf"] = false;
        }
        if((op1 / (2<<14) == 1 && op2 / (2<<14) == 0 && diff / (2<<14) == 0) || (op1 / (2<<14) == 0 && op2 / (2<<14) == 1 && diff / (2<<14) == 1)){
            flags["of"] = true;
        }else{
            flags["of"] = false;
        }
        if(diff / (2<<14) == 1){
            flags["sf"] = true;
        }else{
            flags["sf"] = false;
        }
        if((op1 % (2<<3) < op2 % (2<<3))){
            flags["af"] = true;
        }else{
            flags["af"] = false;
        }
    }

    Counter++;                  // moves the counter to the next instruction

}

void jmp(string label){         // jmp instruction, it jumps (changes the counter) to the right instruction
    if(labels.find(label) != labels.end()){
        Counter = labels[label];
    }else{
        error = true;
    }
}

void jc(string label, string op){      // conditional jump instructions, it jumps (changes the counter) to the right instruction if the condition is met
    if(op == "je" || op == "jz"){
        if(flags["zf"]){
            jmp(label);
        }else{
            Counter++;
        }
    }else if(op == "jne" || op == "jnz"){
        if(!flags["zf"]){
            jmp(label);
        }else{
            Counter++;
        }
    }else if(op == "jb" || op == "jc" || op == "jnae"){
        if(flags["cf"]){
            jmp(label);
        }else{
            Counter++;
        }
    }else if(op == "jna" || op == "jbe"){
        if(flags["cf"] || flags["zf"]){
            jmp(label);
        }else{
            Counter++;
        }
    }else if(op == "jnb" || op == "jae" || op == "jnc"){
        if(!flags["cf"]){
            jmp(label);
        }else{
            Counter++;
        }
    }else if(op == "ja" || op == "jnbe"){
        if(!flags["cf"] && !flags["zf"]){
            jmp(label);
        }else{
            Counter++;
        }
    }else{
        error = true;
    }
}

void int21h(){          // int 21h instruction, reads/writes a character according to the subfunction
    if(registers8["ah"] == 1){  // int 21h      01 subfunction
        string temp;
        cin >> temp;
        if(temp.length() > 1){
            error = true;
        }else {
            registers8["al"] = temp[0];
            change.push_back("al");
        }
    }else if(registers8["ah"] == 2){    // int 21h      02 subfunction
        char temp;
        temp = registers8["dl"];
        cout << temp;
        registers8["al"] = temp;
        change.push_back("al");
    }else{
        error = true;
    }

    Counter++;              // moves the counter to the next instruction
    update();

}

void int20h(){          // int 20h instruction, terminates the code and exit to the Dos
    Counter = simulation.size();
}

string toLowerCase(string line){        // convert to string to lower case

    string output = "";

    for(int i=0; i<line.length(); i++){
        output.push_back(tolower(line[i]));
    }

    return output;

}

void isTrue(string s){              // checks label or variable name is valid
    if(digits.find(s[0]) == digits.end() && (letters.find(s[0]) != letters.end() || s[0] == '$' || s[0] == '_')){
        for(int i=1; i<s.length(); i++){
            if(digits.find(s[i]) == digits.end() && letters.find(s[i]) == letters.end() && s[i] != '$' && s[i] != '_'){
                error = true;
                break;
            }
        }
    }else{
        error = true;
    }
}

void arrange(){             // deals with offsets, characters and b/w varName converts them to a proper state
    for(int i=0; i<simulation.size(); i++){
        vector<string> temp = simulation[i];
        for(int j=1; j<temp.size(); j++){
            if(temp[j].find("offset") != temp[j].npos){     // converts offset phrases into the address of varName
                string var = temp[j].substr(temp[j].find("offset")+6);
                if(variables.find(var) != variables.end()){     // handles offset varName
                    temp[j] = to_string(variables[var].second.first);
                }else if((var[0] == 'b' || var[0] == 'w') && variables.find(var.substr(1)) != variables.end()){     // handles offset b/w varName
                    temp[j] = to_string(variables[var.substr(1)].second.first);
                }
            }else if((temp[j][0] == 'b' || temp[j][0] == 'w') && variables.find(temp[j].substr(1)) != variables.end()){
                temp[j] = temp[j].substr(1);        // converts b/w varName into varName
            }else if(temp[j].length() == 3 && temp[j][0] == '\'' && temp[j][2] == '\''){    // converts character into Ascii value
                char c = temp[j][1];
                int b = c;
                temp[j] = to_string(b);
            }
        }
        simulation[i] = temp;
    }
}

vector<string> token1(string line){     // tokenizer1 function, deals with lines before int 20h
    vector<string> temp;
    string mystr = "";
    int i=0;
    while(line[i] == ' '){
        i++;
    }
    for(; i<line.length(); i++){
        if(line[i] == ' '){
            temp.push_back(mystr);
            break;
        }
        mystr.push_back(line[i]);
    }
    if(i == line.length()){
        temp.push_back(mystr);
        return temp;
    }else{
        mystr.clear();
        while(line[i] == ' '){
            i++;
        }
        if(line[i] == '\''){
            mystr.push_back('\'');
            mystr.push_back(line[i+1]);
            mystr.push_back('\'');
            i = i + 3;
        }
        else{
            for(; i<line.length(); i++){
                if(line[i] == ','){
                    temp.push_back(mystr);
                    break;
                }
                if(line[i] != ' '){
                    mystr.push_back(line[i]);
                }
            }
        }
        while(line[i] == ' '){
            i++;
        }
        if(i == line.length()){
            temp.push_back(mystr);
            return temp;
        }else{
            mystr.clear();
            i++;
            while(line[i] == ' '){
                i++;
            }
            if(line[i] == '\''){
                mystr.push_back('\'');
                mystr.push_back(line[i+1]);
                mystr.push_back('\'');
                i = i + 3;
            }
            else{
                for(;i<line.length(); i++){
                    if(line[i] != ' '){
                        mystr.push_back(line[i]);
                    }
                }
            }
            temp.push_back(mystr);
            return temp;
        }
    }
}

vector<string> token2(string line){            // tokenizer2 function, deals with lines after int 20h
    vector<string> temp;
    string mystr = "";
    int i=0;
    while(line[i] == ' '){
        i++;
    }
    for(; i<line.length(); i++){
        if(line[i] == ' '){
            temp.push_back(mystr);
            break;
        }
        mystr.push_back(line[i]);
    }
    if(i == line.length()){
        temp.push_back(mystr);
        return temp;
    }else{
        mystr.clear();
        while(line[i] == ' '){
            i++;
        }
        for(; i<line.length(); i++){
            if(line[i] == ' '){
                temp.push_back(mystr);
                break;
            }
            mystr.push_back(line[i]);
        }
        if(i == line.length()){
            temp.push_back(mystr);
            return temp;
        }else{
            mystr.clear();
            while(line[i] == ' '){
                i++;
            }
            if(line[i] == '\''){
                mystr.push_back('\'');
                mystr.push_back(line[i+1]);
                mystr.push_back('\'');
            }else{
                for(; i<line.length(); i++){
                    mystr.push_back(line[i]);
                }
            }
            temp.push_back(mystr);
            return temp;
        }
    }
}

int main(int argc, char* argv[]) {

    string line;
    ifstream myfile( argv[1]);
    getline(myfile, line);
    line = toLowerCase(line);
    vector<string> start;
    start = token1(line);

    if(start[0] == "code" && start[1] == "segment" && start.size() == 2){       // checks whether the code starts with "code segment"
        while (getline( myfile, line )){                    // parses the lines before int 20h

            if(line == ""){
                continue;
            }

            vector<string> temp;
            temp = token1(line);

            for(int i=0; i<temp.size(); i++){                   // keeps the Ascii numbers of characters and make lower case rest of the input
                if(temp[i][0] == '\'' && temp[i][2] == '\'' && temp[i].length() == 3){
                    continue;
                }
                temp[i] = toLowerCase(temp[i]);
            }

            if(temp.size() == 1 && temp[0][temp[0].length()-1] == ':'){  // handles the case "labelName:"
                string label = temp[0];
                label.pop_back();
                isTrue(label);
                if(!error && labels.find(label) == labels.end()){
                    labels.insert({label,simulation.size()});
                }else{
                    error = true;
                }
            }else if(temp.size() == 2 && temp[1] == ":"){       // handles the case "labelName  :"
                string label = temp[0];
                isTrue(label);
                if(!error && labels.find(label) == labels.end()){
                    labels.insert({label,simulation.size()});
                }else{
                    error = true;
                }
            }else if(instructions.find(temp[0]) != instructions.end()){     // handles cases with instructions
                for(int i=(simulation.size()*6); i<((simulation.size()*6)+6); i++){
                    mem[i] = -2;
                }
                simulation.push_back(temp);
            }else{
                error = true;
            }

            if(temp[0] == "int" && temp[1] == "20h" && temp.size() == 2){
                line = "int 20h";
                break;
            }
            temp.clear();
        }

        if(line == "int 20h"){                          // checks whether there is int 20h

            int varAddress = simulation.size()*6;

            while (getline( myfile, line )){        // parses the lines after int 20h

                if(line == ""){
                    continue;
                }

                vector<string> temp;
                temp = token2(line);

                for(int i=0; i<temp.size(); i++){           // keeps the Ascii numbers of characters and make lower case rest of the input
                    if(temp[i][0] == '\'' && temp[i][2] == '\'' && temp[i].length() == 3){
                        continue;
                    }
                    temp[i] = toLowerCase(temp[i]);
                }

                if(temp[0] == "code" && temp[1] == "ends" && temp.size() == 2){
                    line = "code ends";
                    break;
                }

                if(temp.size() == 3 && (temp[1] == "db" || temp[1] == "dw")){       // checks variable definition is correct
                    string variable = temp[0];
                    isTrue(variable);
                    int size = 0;
                    char val = ' ';
                    int value = 0;
                    if(!error && variables.find(variable) == variables.end()){
                        if(temp[2][0] == '\'' && temp[2][2] == '\'' && temp[2].length() == 3){
                            val = temp[2][1];
                            value = val;
                            if(temp[1] == "db" && value >= 0 && value < (2<<7)){
                                size = 8;
                                mem[varAddress] = value;
                                variables.insert({variable,{value,{varAddress,size}}});
                                varAddress++;
                            }else if(temp[1] == "dw" && value >= 0 && value < (2<<7)){
                                size = 16;
                                mem[varAddress] = value / (2<<7);
                                mem[varAddress+1] = value % (2<<7);
                                variables.insert({variable,{value,{varAddress,size}}});
                                varAddress = varAddress + 2;
                            }else{
                                error = true;
                            }
                        }else if(isNumber(temp[2])){
                            value = decimal(temp[2]);
                            if(temp[1] == "db" && value >= 0 && value < (2<<7)){
                                size = 8;
                                mem[varAddress] = value;
                                variables.insert({variable,{value,{varAddress,size}}});
                                varAddress++;
                            }else if(temp[1] == "dw" && value >= 0 && value < (2<<15)){
                                size = 16;
                                mem[varAddress+1] = value / (2<<7);
                                mem[varAddress] = value % (2<<7);
                                variables.insert({variable,{value,{varAddress,size}}});
                                varAddress = varAddress + 2;
                            }else{
                                error = true;
                            }
                        }else{
                            error = true;
                        }
                    }else{
                        error = true;
                    }
                }else{
                    error = true;
                }

                temp.clear();

            }

            myfile.close();

            if(error && !isPrinted){        // if errors is not printed and there is an error due to the syntax, write error message and terminate
                cout << "Error";
                isPrinted = true;
                return 0;
            }

            if(line == "code ends"){    // checks the code ends with "code ends"

                arrange();

                while(Counter != simulation.size()){        // the main loop, handles the instruction that is being executed now

                    vector<string> temp;
                    temp = simulation[Counter];

                    // decides current instruction and calls necessary function
                    if(temp[0] == "mov"){
                        if(temp.size() == 3){
                            mov(temp[1],temp[2]);
                        }else{
                            error = true;
                            break;
                        }
                    }else if(temp[0] == "add"){
                        if(temp.size() == 3){
                            add(temp[1],temp[2],temp[0]);
                        }else{
                            error = true;
                            break;
                        }
                    }else if(temp[0] == "inc"){
                        if(temp.size() == 2){
                            add(temp[1],"1",temp[0]);
                        }else{
                            error = true;
                            break;
                        }
                    }else if(temp[0] == "sub"){
                        if(temp.size() == 3){
                            sub(temp[1],temp[2],temp[0]);
                        }else{
                            error = true;
                            break;
                        }
                    }else if(temp[0] == "dec"){
                        if(temp.size() == 2){
                            sub(temp[1],"1",temp[0]);
                        }else{
                            error = true;
                            break;
                        }
                    }else if(temp[0] == "mul"){
                        if(temp.size() == 2){
                            mul(temp[1]);
                        }else{
                            error = true;
                            break;
                        }
                    }else if(temp[0] == "div"){
                        if(temp.size() == 2){
                            div(temp[1]);
                        }else{
                            error = true;
                            break;
                        }
                    }else if(temp[0] == "xor" || temp[0] == "or" || temp[0] == "and"){
                        if(temp.size() == 3){
                            binaryOp(temp[1],temp[2],temp[0]);
                        }else{
                            error = true;
                            break;
                        }
                    }else if(temp[0] == "not"){
                        if(temp.size() == 2){
                            opnot(temp[1]);
                        }else{
                            error = true;
                            break;
                        }
                    }else if(temp[0] == "rcl" || temp[0] == "rcr" || temp[0] == "shl" || temp[0] == "shr"){
                        if(temp.size() == 3){
                            shift(temp[1],temp[2],temp[0]);
                        }else{
                            error = true;
                            break;
                        }
                    }else if(temp[0] == "nop"){
                        if(temp.size() == 1){
                            nop();
                        }else{
                            error = true;
                            break;
                        }
                    }else if(temp[0] == "push"){
                        if(temp.size() == 2){
                            push(temp[1]);
                        }else{
                            error = true;
                            break;
                        }
                    }else if(temp[0] == "pop"){
                        if(temp.size() == 2){
                            pop(temp[1]);
                        }else{
                            error = true;
                            break;
                        }
                    }else if(temp[0] == "cmp"){
                        if(temp.size() == 3){
                            cmp(temp[1],temp[2]);
                        }else{
                            error = true;
                            break;
                        }
                    }else if(temp[0] == "jmp"){
                        if(temp.size() == 2){
                            jmp(temp[1]);
                        }else{
                            error = true;
                            break;
                        }
                    }else if(temp[0] == "jz" || temp[0] == "jnz" || temp[0] == "je" || temp[0] == "jne" || temp[0] == "ja" || temp[0] == "jna" || temp[0] == "jae" || temp[0] == "jb" || temp[0] == "jbe" || temp[0] == "jnae" || temp[0] == "jnb" || temp[0] == "jnbe" || temp[0] == "jnc" || temp[0] == "jc"){
                        if(temp.size() == 2){
                            jc(temp[1],temp[0]);
                        }else{
                            error = true;
                            break;
                        }
                    }else if(temp[0] == "int"){
                        if(temp.size() == 2 && temp[1] == "20h"){
                            int20h();
                        }else if(temp.size() == 2 && temp[1] == "21h"){
                            int21h();
                        }else{
                            error = true;
                            break;
                        }
                    }
                    if(error && !isPrinted){          // if error is not printed before and there is an error print an error message and terminate the program
                        cout << "Error" << endl;
                        isPrinted = true;
                        break;
                    }
                    temp.clear();
                }
            }else{
                cout << "Error" << endl;
            }
        }else{
            cout << "Error" << endl;
        }
    }else{
        cout << "Error" << endl;
    }

    if(error && !isPrinted){             // if error is not printed before and there is an error print an error message and terminate the program
        cout << "Error" << endl;
    }

    return 0;

}