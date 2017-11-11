#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>

#define MEMORY_SIZE 32168

using namespace std;

typedef char int8;
typedef short int16;
typedef pair<int8,int16> int24;

unordered_map <string, int16> symtab;
unordered_map <string, int8> optab;
unordered_map <int8, int8> registertab;

void register_initialize() {
	registertab['A'] = 0;
  registertab['X'] = 1;
  registertab['L'] = 2;
  registertab['B'] = 3;
  registertab['S'] = 4;
  registertab['T'] = 5;
}

void opcode_initialize() {
	optab["ADD"]		= 0x18;
	optab["ADDR"]		= 0x90;
	optab["DIV"]		= 0x24;
	optab["DIVR"]		= 0x9C;
	optab["SUB"]		= 0x1C;
	optab["SUBR"]		= 0x5C;
	optab["MUL"]		= 0x20;
	optab["MULR"]		= 0x98;

	optab["COMP"]		= 0x28;
	optab["COMPR"]	= 0xA0;

	optab["JEQ"]		= 0x30;
	optab["JGT"]		= 0x34;
	optab["JLT"]		= 0x38;
	optab["J"]			= 0x3C;

	optab["LDA"]		= 0x00;
	optab["LDB"]		= 0x68;
	optab["LDL"]		= 0x08;
	optab["LDS"]		= 0x6C;
	optab["LDT"]		= 0x74;
	optab["LDX"]		= 0x04;
	optab["LDCH"]		= 0x50;

	optab["STA"]		= 0x0C;
	optab["STB"]		= 0x78;
	optab["STL"]		= 0x14;
	optab["STS"]		= 0x7C;
	optab["STT"]		= 0x84;
	optab["STX"]		= 0x10;
	optab["STCH"]		= 0x54;

	optab["TIX"]		= 0x2C;
	optab["TIXR"]		= 0xB8;

  optab["RSUB"]		= 0x4C;
  optab["JSUB"]		= 0x48;

  optab["AND"]		= 0x40;
  optab["SHIFTL"]	= 0xA4;
  optab["SHIFTR"]	= 0xA8;
  optab["OR"]			= 0x44;
  optab["RMO"]		= 0xAC;
}

int main() {
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

	while (!fin.eof()) {
		getline(fin, line);
		s.str(line);
		while (s >> word) {
			vec[i].push_back(word);
		}
		s.clear();
		if (vec[i].size() < 3) {
			i++;
			lineno++;
			continue;
		}
		if (vec[i][0][0] == '+' && optab.find(vec[i][0].substr(1)) != optab.end()) // +jsub extended instruction
			locattr = locattr + 4;
		else if (optab.find(vec[i][0]) != optab.end())
			locattr = locattr + 3;
		else {
			if (symtab.find(vec[i][0]) != symtab.end()) {
				cout << "Duplicate at line no. " << lineno << endl;
				break;
			}
			if (vec[i][1] == "WORD") {
				symtab[vec[i][0]] = locattr;
				locattr = locattr + 3;
			} else if (vec[i][1] == "RESW") {
				symtab[vec[i][0]] = locattr;
				locattr = locattr + 3*stoi(vec[i][2]);
			} else if (vec[i][1] == "RESB") {
				symtab[vec[i][0]] = locattr;
				locattr = locattr + stoi(vec[i][2]);
			} else if (vec[i][1] == "BYTE") {
				symtab[vec[i][0]] = locattr;
				if (vec[i][2][0] == 'C' && vec[i][2][1] == '\'') {
					if(vec[i][2].length() == 6) {
						if (vec[i][2] == "C\'EOF\'")
							++locattr;
						else {
							locattr += 3;
						}
					} else {
						locattr += (vec[i][2].length()-3);
					}
				} else if (vec[i][2][0] == 'X' && vec[i][2][1] == '\'') {
					if (vec[i][2].length() > 5) {
						cout << "Invalid value at line no. " << lineno << endl;
						break;
					} else if (vec[i][2][4] == '\'') {
						++locattr;
					} else {
						cout << "Invalid value at line no. " << lineno+1 << endl;
						break;
					}
				}
			} else {
				symtab[vec[i][0]] = locattr;
				if (vec[i][0][1] == '+' && optab.find(vec[i][1].substr(1)) != optab.end()) // +jsub extended instruction
					locattr += 4;
				else if (optab.find(vec[i][1]) != optab.end())
					locattr += 3;
				else
					cout << "Invalid symbol at line no. " << lineno+1 << endl;
			}
		}
		i++;
		lineno++;
	}
	return 0;
}
