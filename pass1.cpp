#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>
#define int8 char
#define int16 short
#define int24 pair <int8,int16>
#define MEMORY_SIZE 32168
using namespace std;

unordered_map <string,int16> symtab;
unordered_map <string, int8> optab;
unordered_map <int8, int8> registertab;

void register_initialize(){

	registertab['A'] = 0;
    registertab['X'] = 1;
    registertab['L'] = 2;
    registertab['B'] = 3;
    registertab['S'] = 4;
    registertab['T'] = 5;

}
void opcode_initialize(){

	optab["ADD"] = 0x18;
	optab["ADDR"] = 0x90;
	optab["DIV"] = 0x24;
	optab["DIVR"] = 0x9C;
	optab["SUB"] = 0x1C;
	optab["SUBR"] = 0x5C;
	optab["MUL"] = 0x20;
	optab["MULR"] = 0x98;

	optab["COMP"] = 0x28;
	optab["COMPR"] = 0xA0;

	optab["JEQ"] = 0x30;
	optab["JGT"] = 0x34;
	optab["JLT"] = 0x38;
	optab["J"] = 0x3C;

	optab["LDA"] = 0x00;
	optab["LDB"] = 0x68;
	optab["LDL"] = 0x08;
	optab["LDS"] = 0x6C;
	optab["LDT"] = 0x74;
	optab["LDX"] = 0x04;
	optab["LDCH"] = 0x50;

	optab["STA"] = 0x0C;
	optab["STB"] = 0x78;
	optab["STL"] = 0x14;
	optab["STS"] = 0x7C;
	optab["STT"] = 0x84;
	optab["STX"] = 0x10;
	optab["STCH"] = 0x54;

	optab["TIX"] = 0x2C;
	optab["TIXR"] = 0xB8;

    optab["RSUB"] = 0x4C;
    optab["JSUB"] = 0x48;

    optab["AND"] = 0x40;
    optab["SHIFTL"] = 0xA4;
    optab["SHIFTR"] = 0xA8;
    optab["OR"] = 0x44;
    optab["RMO"] = 0xAC;

}
int main(){
	int8 memory[MEMORY_SIZE];
	vector<vector<string>> vec(1000);
	ifstream fin;
	int16 locattr = 0;
	istringstream s;
	string line, word;
	int lineno = 0, i = 0;
	bool new_command = false;

	fin.open("input.txt");
	opcode_initialize();

	while(!fin.eof()){
		getline(fin, line);
		s.str(line);
		new_command = false;
		//ignore the blank lines
		while(getline(s, word,' ')){
			if(word.length() != 0){
				new_command = true;
				vec[i].push_back(word);
			}
		}
		if(vec[i][0][0] == '+' && optab.find(vec[i][0].substr(1)) != optab.end())// +jsub extended instruction
			locattr = locattr + 4;
		else if(optab.find(vec[i][0]) != optab.end())
			locattr = locattr + 3;
		else {
			if(symtab.find(vec[i][0]) != symtab.end()){
				cout << "Duplicate at line no. " << lineno << endl;
				break;
			}
			if(vec[i][1] == "WORD"){
				locattr = locattr + 3;
				symtab[vec[i][0]] = locattr;
			}
			else if(vec[i][1] == "RESW"){
				symtab[vec[i][0]] = locattr;	
				locattr = locattr + 3*stoi(vec[i][2]);
			}
			else if(vec[i][1] == "RESB"){
				symtab[vec[i][0]] = locattr;	
				locattr = locattr + stoi(vec[i][2]);
			}
			else if(vec[i][1] == "BYTE"){
				symtab[vec[i][0]] = locattr;	
				locattr = locattr + (vec[i][2].length()-3);// EOF BYTE C'EOF'. the C , and two ' are not considetred
			}
			else
				symtab[vec[i][0]] = locattr;// for labels LOOP LDA THREE
		}


		if (new_command)
			i++;
		lineno++;

	}
return 0;
}
