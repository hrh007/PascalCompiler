#include <iostream>
#include <stdexcept>

#include "node.h"
#include "GenIR.h"

using namespace std;

extern int yyparse();
extern ast::Program* ast_root;

std::string red(const std::string& str) {
	string red_b = "\033[1;31m";
	string red_e = "\033[0m";
	return red_b + str + red_e;
}

int main(int argc, char** argv) {
	yyparse();
	cout << ast_root << endl;

	InitializeNativeTarget();
	CodeGenContext context;
	//createCoreFunctions(context);
	try {
		context.generateCode(*ast_root);
		context.runCode();
	} catch (const std::domain_error &de) {
		cout << red(de.what()) << endl;		
	} catch (...) {
		cout << "other uncaught error" << endl;
	}
	
	return 0;
}