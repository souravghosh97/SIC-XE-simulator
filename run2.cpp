#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <iomanip>
#include <vector>

#define MEMORY_SIZE 32168
#define print(i) "^" << setfill('0') << setw(i)
using namespace std;

int memory[MEMORY_SIZE];
typedef char int8;
typedef short int16;
typedef pair<int8,int16> int24;
typedef struct{
	int opcode;//can be 8 or 6bit
	int flags;// 6 bit
	int disp;// 12 or 20 bits
	int format;
}instruction;

typedef struct{
	int opcode;
	int format;
}object;

typedef struct{
	int PC;
	int A;
	int B;
	int S;
	int T;
	int L;
	int X;
}registers;

bool pc_relative = true;//to denote base relative or pc

int r[8]={0,0,0,0,0,0,0,0};

bool is_branch(string str){
	return  (str == "JSUB" || str == "J" || str == "JEQ" || str == "JLT") ||
			(str == "+JSUB" || str == "+J" || str == "+JEQ" || str == "+JLT");

}
unordered_map <string, int> symtab;
unordered_map <string, object> optab;
unordered_map <int8, int8> registertab;



pair<int,bool> calculate_address(instruction objectcode){//if it return address bool is 0 else data,1
	int flags = objectcode.flags;
	int effective_address;
	int result;
	int disp;
	bool negative;
	if (objectcode.format == 4){
		disp = objectcode.disp & 0xFFFFF;
		negative = (objectcode.disp & 0x80000) >> 19;
	}
	else if(objectcode.format == 3){
		disp = objectcode.disp & 0xFFF;
		negative = (objectcode.disp & 0x800) >> 11;
	}
	effective_address = disp;
	if(objectcode.format == 3){
		if (flags & 0x4){//base relative
			if(negative)
				effective_address = r[3] + disp - 0xFFF - 1;
			else	
				effective_address = r[3] + disp;
		}
		else if (flags & 0x2){
			if(negative)
				effective_address = r[6] + disp - 0xFFF - 1;
			else	
				effective_address = r[6] + disp;	
		}
	}

	if(objectcode.format == 4){
		if (flags & 0x4){//base relative
			if(negative)
				effective_address = r[3] + disp - 0xFFFFF - 1;
			else	
				effective_address = r[3] + disp;
		}
		else if (flags & 0x2){
			if(negative)
				effective_address = r[6] + disp - 0xFFFFF - 1;
			else	
				effective_address = r[6] + disp;	
		}
	}

	if(flags & 0x08)
			effective_address = effective_address +r[1];

	if ((flags & 0x20) && (flags & 0x10))
		return make_pair(effective_address,0);
	else if (flags & 0x20)
		return make_pair(memory[effective_address],0);
	else if (flags & 0x10)
		return make_pair(disp,1);


}

instruction calculate_instruction(int loc){
	instruction result;
	result.opcode = memory[loc] & 0xFC;
	result.flags = ((memory[loc] & 0x03) << 4) + ((memory[loc+1] >> 4) & 0xF);
	result.disp = ((memory[loc+1] & 0x0F) << 8) +  memory[loc+2];
	r[6] = r[6] + 3;
	result.format = 3;
	if(result.flags & 0x1){
		result.disp = (result.disp << 8) + memory[loc+3];
		r[6]++;
		result.format = 4;
	}
	return result;
}


instruction calculate_objectcode(vector<string> vec, int lineno){
	instruction result;
	string str;
	int i;
	result.opcode = 0;
	result.flags = 0;
	result.disp = 0;
	result.format = 0;
	if(vec.size() <= 2)
		i = 0;
	else
		i = 1;
	/* opcode*/

	if (optab.find(vec[i]) != optab.end()){
		if(optab[vec[i]].format == 3){
				result.opcode = optab[vec[i]].opcode & 0xFC;
				result.format = 3;
		}
		else if(optab[vec[i]].format <= 2){
			result.opcode = optab[vec[i]].opcode;
			result.format = optab[vec[i]].format;
		}
		else if(optab[vec[i]].format == 4){
			result.format = 4;
			result.opcode = optab[vec[i]].opcode & 0xFC;
			result.flags = result.flags | 0x1;//0 0 0 0 0 1 
		}
		else {
			cout << "Undefined Symbol at line no." << lineno << endl;
			exit(0);
		}
	}
	if (result.format >= 3){
		if (vec[i] == "+JSUB" && symtab.find(vec[i+1]) != symtab.end()){/*flags and disp TO DO ADD DIRECT ADDRESSING*/
			result.disp = symtab[vec[i+1]];// have to add check here
			result.flags = result.flags | 0x30;// 1 1 0 0 0 0 direct addressing
		}
		else if (vec[i+1][0] == '#'){
			result.disp = stoi(vec[i+1].substr(1));
			result.flags = result.flags | 0x10;//0 1 0 0 0 0 immediate addressing
		} 
		else if (symtab.find(vec[i+1]) != symtab.end()){
			if(pc_relative){
				result.disp = symtab[vec[i+1]] - r[6];
				result.flags = result.flags | 0x32;//1 1 0 0 1 0 pc-relative addressing
				}
			else {
				result.disp = symtab[vec[i+1]] - r[3];
				result.flags = result.flags | 0x34;//1 1 0 1 0 0 base relative addressing 
			}
		}
		else if(vec[i+1].back() == 'X' && symtab.find(vec[i+1].substr(0,vec[i+1].length()-2)) != symtab.end()){//LDA ARRAY,X
			if(pc_relative) {
				result.disp = symtab[vec[i+1].substr(0,vec[i+1].length()-2)] - r[6];
				result.flags = result.flags | 0x3A;//1 1 1 0 1 0 index addressing pc relative 
			}
			else {
				result.disp = symtab[vec[i+1].substr(0,vec[i+1].length()-2)] - r[3];
				result.flags = result.flags | 0x34;//1 1 1 1 0 0 index addrssing base relative
			}  	
		}
		else if(vec[i+1].front() == '@' && symtab.find(vec[i+1].substr(1)) != symtab.end()){
			if(pc_relative){
				result.disp = symtab[vec[i+1].substr(0,vec[i+1].length()-2)] - r[6];
				result.flags = result.flags | 0x22;//1 0 0 0 1 0 pc relative indirect addressing
			}
			else {
				result.disp = symtab[vec[i+1].substr(0,vec[i+1].length()-2)] - r[3];
				result.flags = result.flags | 0x24;//1 0 0 1 0 0 base relative indirect addressing
			}
		}
		else{
				cout << "Undefined Symbol at line no." << lineno << endl;
				exit(0);//exit
		}
	} 
	else if(result.format == 2){//ADDR S,A
			if(vec[i+1].length()==3)
				result.disp = (registertab[vec[i+1][0]] << 4) + registertab[vec[i+1][2]];
			else if(vec[i+1].length() == 1 && (vec[i] == "TIXR" || vec[i] == "CLEAR"))
				result.disp = registertab[vec[i+1][0]];
			else{
				cout << "Invalid syntax at line no. " << lineno << endl;
				exit(0);
			}

	} 
	else {
		//RSUB
	}
	return result;
}

void register_initialize() {
	registertab['A'] = 0;
  	registertab['X'] = 1;
  	registertab['L'] = 2;
  	registertab['B'] = 3;
  	registertab['S'] = 4;
  	registertab['T'] = 5;
}

void opcode_initialize() {
	optab["ADD"].opcode 	= 0x18;
	optab["ADD"].format 	= 3;
	optab["ADDR"].opcode 	= 0x90;
	optab["ADDR"].format 	= 2;
	optab["DIV"].opcode	= 0x24;
	optab["DIV"].format 	= 3;
	optab["DIVR"].opcode	= 0x9C;
	optab["DIVR"].format 	= 2;
	optab["SUB"].opcode	= 0x1C;
	optab["SUB"].format  	= 3;
	optab["SUBR"].opcode	= 0x5C;
	optab["SUBR"].format	= 2;
	optab["MUL"].opcode	= 0x20;
	optab["MUL"].format	= 3;
	optab["MULR"].opcode	= 0x98;
	optab["MULR"].format	= 2;

	optab["COMP"].opcode	= 0x28;
	optab["COMP"].format	= 3;
	optab["COMPR"].opcode	= 0xA0;
	optab["COMPR"].format	= 2;

	optab["JEQ"].opcode	= 0x30;
	optab["JEQ"].format 	= 3;
	optab["JGT"].opcode	= 0x34;
	optab["JGT"].format 	= 3;
	optab["JLT"].opcode	= 0x38;
	optab["JLT"].format 	= 3;
	optab["J"].opcode	= 0x3C;
	optab["J"].format 	= 3;

	optab["LDA"].opcode	= 0x00;
	optab["LDA"].format	= 3;
	optab["LDB"].opcode	= 0x68;
	optab["LDB"].format	= 3;
	optab["LDL"].opcode	= 0x08;
	optab["LDL"].format	= 3;
	optab["LDS"].opcode	= 0x6C;
	optab["LDS"].format	= 3;
	optab["LDT"].opcode	= 0x74;
	optab["LDT"].format	= 3;
	optab["LDX"].opcode	= 0x04;
	optab["LDX"].format	= 3;
	optab["LDCH"].opcode	= 0x50;
	optab["LDCH"].format	= 3;

	optab["STA"].opcode	= 0x0C;
	optab["STA"].format	= 3;	
	optab["STB"].opcode	= 0x78;
	optab["STB"].format	= 3;
	optab["STL"].opcode	= 0x14;
	optab["STL"].format	= 3;
	optab["STS"].opcode	= 0x7C;
	optab["STS"].format	= 3;
	optab["STT"].opcode	= 0x84;
	optab["STT"].format	= 3;
	optab["STX"].opcode	= 0x10;
	optab["STX"].format	= 3;
	optab["STCH"].opcode	= 0x54;
	optab["STCH"].format	= 3;

	optab["TIX"].opcode	= 0x2C;
	optab["TIX"].format 	= 3;
	optab["TIXR"].opcode	= 0xB8;
	optab["TIXR"].format 	= 2;

  	optab["RSUB"].opcode	= 0x4C;
  	optab["RSUB"].format 	= 1;
  	optab["JSUB"].opcode	= 0x48;
  	optab["JSUB"].format 	= 3;

  	optab["AND"].opcode	= 0x40;
  	optab["AND"].format 	= 3;
  	optab["SHIFTL"].opcode	= 0xA4;
  	optab["SHIFTL"].format  = 2;
  	optab["SHIFTR"].opcode	= 0xA8;
  	optab["SHIFTR"].format  = 2;
  	optab["OR"].opcode	= 0x44;
  	optab["OR"].format	= 2;
  	optab["RMO"].opcode	= 0xAC;
  	optab["RMO"].format	= 2;

  	optab["CLEAR"].opcode = 0xB4;
  	optab["CLEAR"].format = 2;
}

int main() {
	vector<vector<string>> vec(1000);
	ifstream fin;
	int locattr = 0, start_location = 0, inst_end;
	istringstream s;
	string line, word;
	string program_name;
	int lineno = 0, i = -1, len;
	bool new_program = false;
	int length;
	bool end = false;

	fin.open("input.txt");

	opcode_initialize();
	/* PASS 1*/
	while (!fin.eof()) {
		getline(fin, line);
		s.str(line);
		i++;
		lineno++;
		while (s >> word) {
			vec[i].push_back(word);
		}
		s.clear();
		if (vec[i].size() == 0) {
			continue;
		}
		if(vec[i][1] == "START"){
			if(new_program){
				cout << "Duplicate START at line no. " << lineno << endl;
				return 0;
			}
			program_name = vec[i][0];
			locattr = stoi(vec[i][2]);
			new_program = true;
			symtab[vec[i][0]] = locattr;
			start_location = locattr;
			new_program = true;
			
			continue;
		}
		if(vec[i][0] == "END"){
			if(vec[i][1] != program_name){
				cout << "Error at line no." << lineno << endl;
				return 0;
			}

			break;
		}
		new_program = true;
		if (optab.find(vec[i][0]) != optab.end())
			locattr = locattr + optab[vec[i][0]].format;
		else {
			if (symtab.find(vec[i][0]) != symtab.end()) {
				cout << "Duplicate at line no. " << lineno << endl;
				return 0;
			}
			if (vec[i][1] == "WORD") {
				if(!end)
					inst_end = locattr;
				end = true;
				symtab[vec[i][0]] = locattr;
				locattr = locattr + 3;
			} 
			else if (vec[i][1] == "RESW") {
				if(!end)
					inst_end = locattr;
				end = true;
				symtab[vec[i][0]] = locattr;
				locattr = locattr + 3*stoi(vec[i][2]);
			} 
			else if (vec[i][1] == "RESB") {
				if(!end)
					inst_end = locattr;
				end = true;
				symtab[vec[i][0]] = locattr;
				locattr = locattr + stoi(vec[i][2]);
			} 
			else if (vec[i][1] == "BYTE") {
				if(!end)
					inst_end = locattr;
				end = true;
				symtab[vec[i][0]] = locattr;
				if (vec[i][2][0] == 'C' && vec[i][2][1] == '\'') 
						locattr += (vec[i][2].length()-3);
				else if (vec[i][2][0] == 'X' && vec[i][2][1] == '\'') {
					if (vec[i][2].length() > 5) {
						cout << "Invalid value at line no. " << lineno << endl;
						return 0;
					} else if (vec[i][2][4] == '\'') {
						++locattr;
					} else {
						cout << "Invalid value at line no. " << lineno << endl;
						return 0;
					}
				} 
				else {// P BYTE abc
						cout << "Invalid value at line no. " << lineno << endl;
						return 0;
				}

			} 
			else {
				symtab[vec[i][0]] = locattr;
				if (optab.find(vec[i][1]) != optab.end())//loop jsub abc
					locattr += optab[vec[i][1]].format;
				else{
					cout << "Invalid symbol at line no. " << lineno << endl;
					return 0;
				}
			}
		}
	}
	fin.close();
	length = (locattr - start_location);
	len = i;// len is the total no of line in input.txt
	/* PASS1 complete */

	/* PASS2 START */ 
	ofstream fout;
	int j=0;
	instruction objectcode;
	locattr = start_location;
	fout.open("output.txt");
	register_initialize();
	r[6] = start_location;
	fout.setf(ios_base::hex , ios_base::basefield);
	for (i = 0;i < len;i++){
		if(vec[i].size() == 0)
			continue;
		if(vec[i][1] == "START")
			continue;
		else if(vec[i][0] == "END")
			continue;
		else{
			if(vec[i].size() <= 2)
				j = 0;
			else
				j = 1;
			if(vec[i][j] == "WORD"){;
				r[6] = r[6] + 3;
				memory[locattr] = (stoi(vec[i][j+1]) & 0xFF0000) >> 16; 
				locattr++;
				memory[locattr] = (stoi(vec[i][j+1]) & 0xFF00) >> 8;
				locattr++;
				memory[locattr] = stoi(vec[i][j+1]) & 0xFF;
				locattr++;
				continue;
			}
			else if(vec[i][j] == "BYTE"){
				word = vec[i][j+1].substr(2,vec[i][j+1].size()-3);//C'ABC' TO ABC'
				r[6] = r[6] + word.length();
				for(int k = 0;k < word.length();k++){
					memory[locattr] = word[k];
					locattr++;
				}
				continue;

			}
			else if(vec[i][j] == "RESW"){
				r[6] = r[6] + 3*stoi(vec[i][2]);
				locattr = locattr + 3*stoi(vec[i][2]);
				continue;
			}
			else if(vec[i][j] == "RESB"){
				r[6] = r[6] + stoi(vec[i][2]);
				locattr = locattr + stoi(vec[i][2]);
				continue;
			}
			else{}
			r[6] = r[6] + optab[vec[i][j]].format;
			objectcode = calculate_objectcode(vec[i],i+1);
			if (objectcode.format == 1){
				memory[locattr] = objectcode.opcode;
				locattr++;	
			}
			else if(objectcode.format == 2){
				memory[locattr] = objectcode.opcode;
				locattr++;	
				memory[locattr] = objectcode.disp;
				locattr++;
			}
			else if(objectcode.format == 3){
				memory[locattr] = objectcode.opcode + (objectcode.flags >> 4);
				locattr++;
				memory[locattr] = ((objectcode.flags & 0xF) << 4) + ((objectcode.disp >> 8) & 0xF);
				locattr++;
				memory[locattr] = objectcode.disp & 0xFF;
				locattr++;
			}
			else{
				memory[locattr] = objectcode.opcode + objectcode.flags >> 4;
				locattr++;
				memory[locattr] = ((objectcode.flags & 0xF) << 4) + ((objectcode.disp >> 8) & 0xF);
				locattr++;
				memory[locattr] = (objectcode.disp & 0xFF00) >> 8;
				locattr++;
				memory[locattr] = objectcode.disp & 0xFF;
				locattr++;
			}
		}

	}
	for(int k=start_location;k<locattr;k++)
		fout  << setw(4) << setfill('0') << k << " " << setw(2) << setfill('0') << memory[k] << "\n";  

	r[6] = start_location;
	int code, loc, num, temp;
	pair<int,bool> pa;
	i=0;
	while(r[6] != inst_end){
		cout << "A= " << r[0] << " X= " << r[1] << " L= " << r[2] << " S= " << r[4] << " T= " << r[5] << " PC= " << r[6] << "\n";
		code = memory[r[6]];
		code = code & 0xFC;
	switch(code){
		case 0: //LDA
			loc = r[6]; 
			objectcode = calculate_instruction(loc);
			pa = calculate_address(objectcode);
			if (pa.second)
				r[0] = pa.first;
			else
				r[0] = (memory[pa.first] << 16) + (memory[pa.first+1] << 8) + memory[pa.first + 2];
			break;
		case 4: //LDX
			loc = r[6]; 
			objectcode = calculate_instruction(loc);
			pa = calculate_address(objectcode);
			if (pa.second)
				r[1] = pa.first;
			else
				r[1] = (memory[pa.first] << 16) + (memory[pa.first+1] << 8) + memory[pa.first + 2];
			break;
		case 12: //STA
			loc = r[6]; 
			objectcode = calculate_instruction(loc);
			pa = calculate_address(objectcode);
			memory[pa.first] = (r[0] & 0xFF0000) >> 16;
			memory[pa.first+1] = (r[0] & 0xFF00) >> 8;
			memory[pa.first+2] = (r[0] & 0xFF);
			break;
		case 16: //STX
			loc = r[6]; 
			objectcode = calculate_instruction(loc);
			pa = calculate_address(objectcode);
			memory[pa.first] = (r[0] & 0xFF0000) >> 16;
			memory[pa.first+1] = (r[1] & 0xFF00) >> 8;
			memory[pa.first+2] = (r[1] & 0xFF);
			break;
		case 24: //ADD
			loc = r[6]; 
			objectcode = calculate_instruction(loc);
			pa = calculate_address(objectcode);
			if (pa.second)
				r[0] = r[0] + pa.first;
			else
				r[0] = r[0] + (memory[pa.first] << 16) + (memory[pa.first+1] << 8) + memory[pa.first + 2];
			break;
		case 28: //SUB
			loc = r[6]; 
			objectcode = calculate_instruction(loc);
			pa = calculate_address(objectcode);
			if (pa.second)
				r[0] = r[0] - pa.first;
			else
				r[0] = r[0] - ((memory[pa.first] << 16) + (memory[pa.first+1] << 8) + memory[pa.first + 2]);
			break;
			case 32: //MUL
			loc = r[6]; 
			objectcode = calculate_instruction(loc);
			pa = calculate_address(objectcode);
			if (pa.second)
				r[0] = r[0]*pa.first;
			else
				r[0] = r[0]* ((memory[pa.first] << 16) + (memory[pa.first+1] << 8) + memory[pa.first + 2]);
			break;			
		case 40: //COMP
			loc = r[6]; 
			objectcode = calculate_instruction(loc);
			pa = calculate_address(objectcode);
			if (pa.second){
				if(r[0] > pa.first)
					r[7] = 1;
				else if(r[0] < pa.first)
					r[7] = -1;
				else
					r[7] = 0;
			}
			else{
				num = (memory[pa.first] << 16) + (memory[pa.first+1] << 8) + memory[pa.first + 2];
				if(r[0] > num)
					r[7] = 1;
				else if(r[0] < num)
					r[7] = -1;
				else
					r[7] = 0;
			}
			break;
		case 48: //JEQ
			loc = r[6]; 
			objectcode = calculate_instruction(loc);
			pa = calculate_address(objectcode);
			if (r[7]==0){
				r[6] = pa.first;		 	
			}
			break;
		case 52: //JGT
			loc = r[6]; 
			objectcode = calculate_instruction(loc);
			pa = calculate_address(objectcode);
			if (r[7]==1){
				r[6] = pa.first;		 	
			}
			break;
		case 56: //JLT
			loc = r[6]; 
			objectcode = calculate_instruction(loc);
			pa = calculate_address(objectcode);
			if (r[7]==-1){
				r[6] = pa.first;		 	
			}
			break;
		case 72: //JSUB
			loc = r[6];
			r[2] = loc; 
			objectcode = calculate_instruction(loc);
			r[2]=r[6];
			pa = calculate_address(objectcode);
			r[6] = pa.first;
			break;
		case 76: //RSUB
			r[6] = r[2];
			break;
		case 0x90://ADDR
			loc = memory[r[6]+1];
			r[6] = r[6] +2;
			r[(loc&0xF)] = r[(loc&0xF)] + r[((loc&0xF0)>>4)];
			break;
		case 0x5C://SUBR
			loc = memory[r[6]+1];
			r[6] = r[6] +2;
			r[(loc&0xF)] = r[(loc&0xF)] - r[((loc&0xF0)>>4)];
			break;
		case 0x98://MULR
			loc = memory[r[6]+1];
			r[6] = r[6] +2;
			r[(loc&0xF)] = r[(loc&0xF)] * r[((loc&0xF0)>>4)];
			break;
		case 0xAC://RMO
			loc = memory[r[6]+1];
			r[6] = r[6] +2;
			r[(loc&0xF)] = r[((loc&0xF0)>>4)];
			break;
		case 0xA0://COMPR
			loc = memory[r[6]+1];
			r[6] = r[6] +2;
			if(r[(loc&0xF)]<r[((loc&0xF0)>>4)])
				r[7]=-1;
			else if(r[(loc&0xF)]>r[((loc&0xF0)>>4)])
				r[7]=1;
			else
				r[7]=0;
			break;
		case 0x54://STCH
			loc = r[6]; 
			objectcode = calculate_instruction(loc);
			pa = calculate_address(objectcode);
			memory[pa.first] = (r[0] & 0xFF);
			break;
		case 0x50://LDCH
			loc = r[6]; 
			objectcode = calculate_instruction(loc);
			pa = calculate_address(objectcode);
			r[0] = memory[pa.first];
			break;
		case 0x40://AND
			loc = r[6]; 
			objectcode = calculate_instruction(loc);
			pa = calculate_address(objectcode);
			if(pa.second)
				num = (memory[pa.first] << 16) + (memory[pa.first+1] << 8) + memory[pa.first + 2];
			else
				num = pa.first; 
			r[0] = r[0] & num;
			break;
		case 0x44://OR
			loc = r[6]; 
			objectcode = calculate_instruction(loc);
			pa = calculate_address(objectcode);
			if(pa.second)
				num = (memory[pa.first] << 16) + (memory[pa.first+1] << 8) + memory[pa.first + 2];
			else
				num = pa.first;
			r[0] = r[0] | num;
			break;
		case 0xA4://SHIFTL
			loc = memory[r[6]+1];
			r[6] = r[6] +2;
			num = loc & 0xF;
			temp = (loc&0xF0)>>4;
			for(i=1;i<=num;i++){
				r[temp] = ((r[temp] & 0x80) >> 7) + (r[temp] << 1);
			}
			break;
		case 0xA8://SHIFTR
			loc = memory[r[6]+1];
			r[6] = r[6] +2;
			num = loc & 0xF;
			temp = (loc&0xF0)>>4;
			for(i=1;i<=num;i++){
				r[temp] = ((r[temp] & 0x1) << 7) + (r[temp] >> 1);
			}
			break;
		case 0x2C://TIX
			loc = r[6]; 
			objectcode = calculate_instruction(loc);
			pa = calculate_address(objectcode);
			if(pa.second)
				num = (memory[pa.first] << 16) + (memory[pa.first+1] << 8) + memory[pa.first + 2];
			else
				num = pa.first;
			r[1]++;
			if(r[1]<num)
				r[7]=-1;
			else if(r[7]>num)
				r[7]=1;
			else
				r[7]=0;
			break;
		case 0xB8://TIXR
			loc = memory[r[6]+1];
			r[6] = r[6] +2;
			r[1]++;
			temp=(loc&0xF);
			if(r[1]<r[temp])
				r[7]=-1;
			else if(r[1]>r[temp])
				r[7]=1;
			else
				r[7]=0;
			break;
		case 0xB4://CLEAR
			loc = memory[r[6]+1];
			r[6] = r[6] + 2;
			temp = loc & 0xF;
			r[temp] = 0;
			break;
		case 0x6C://LDS
			loc = r[6]; 
			objectcode = calculate_instruction(loc);
			pa = calculate_address(objectcode);
			if (pa.second)
				r[4] = pa.first;
			else
				r[4] = (memory[pa.first] << 16) + (memory[pa.first+1] << 8) + memory[pa.first + 2];
			break;
		case 0x74://LDT
			loc = r[6]; 
			objectcode = calculate_instruction(loc);
			pa = calculate_address(objectcode);
			if (pa.second)
				r[5] = pa.first;
			else
				r[5] = (memory[pa.first] << 16) + (memory[pa.first+1] << 8) + memory[pa.first + 2];
			break;
		case 0x7C://STS
			loc = r[6]; 
			objectcode = calculate_instruction(loc);
			pa = calculate_address(objectcode);
			memory[pa.first] = (r[4] & 0xFF0000) >> 16;
			memory[pa.first+1] = (r[4] & 0xFF00) >> 8;
			memory[pa.first+2] = (r[4] & 0xFF);
			break;
		case 0x84://STT
			loc = r[6]; 
			objectcode = calculate_instruction(loc);
			pa = calculate_address(objectcode);
			memory[pa.first] = (r[5] & 0xFF0000) >> 16;
			memory[pa.first+1] = (r[5] & 0xFF00) >> 8;
			memory[pa.first+2] = (r[5] & 0xFF);
			break;
			
		}
}
	fout << "\n";
	for(int k=start_location;k<locattr;k++)
		fout  << setw(4) << setfill('0') << k << " " << setw(2) << setfill('0') << memory[k] << "\n";
	fout.close();
	return 0;
}
