#include <SyntaxTree.h>
#include <vector>
#include <llvm/IR/Value.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Argument.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IRBuilder.h>

using namespace std;
using namespace llvm;

extern LLVMContext &context;
extern IRBuilder<> builder;
extern Module module;
extern Function *startFunc;
extern string errorMsg;
extern Program *program;

extern Value* createCast(Value *value,Type *type);
extern Constant* getInitial(Type *type);
extern void throwError(Node *node);
extern void throwWarning(Node *node,string msg);
extern string getOperatorName(int op);
extern string getTypeName(Type *type);

//二元表达式集合
typedef enum {
	plus_kind,	// plus
	minus_kind,	// minus
	or_kind,	// or
	mul_kind,	// muliply
	div_kind,	// div
	mod_kind,	// mod
	and_kind,	// and
	ge_kind,	// greater equal
	gt_kind,	// greater than
	le_kind,	// less equal
	lt_kind,	// less than
	eq_kind,	// equal
	ueq_kind,	// unequal
	assignment_kind	// assignment
} BinaryOpKind;

//一元表达式集合
typedef enum  {
	not_kind,	// not
	neg_kind	// negative
} UnaryOpKind;

//const值类型
union constValue {
	integer i;
	double d;
	char c;
	string s;
	bool b;
};

//TypeAST中用来标记是哪种类型
typedef enum {  
	basic,
	array,
	self
} TypeName;

//SelfdefineTypeAST中用来标记类型
typedef enum {  
	record,
	changeName
} SelfType;

//BasicTypeAST中的物种基础类型
typedef enum {  
	INTEGER,
	STRING,
	REAL,
	CHAR,
	BOOL
} BasicType;

typedef enum {  
	Number,
	Real,
	Bool,
	String,
	Char,
	Variable,
	ArrayVariable,
	RecordVariable,
	UnaryExpr,
	BinaryExpr,
	CallExpr,
	IfExpr,
	ForExpr,
	WhileExpr,
	RepeatExpr
} ExprType;

class Node {
public:
	virtual ~Node(){}
	virtual Value* Codegen(CodeGenContext& context);
	TypeAST* Error(const char *str){fprintf(stderr, "%s\n", str); return 0;}
};

//const类
class ConstAST : public Node{
public:
	string variableName;
	BasicTypeAST* type;
	constValue value;

	ConstAST(string name, BasicTypeAST* ty, constValue val)
		:variableName(name), type(ty), value(val){}
	~ConstAST(){}
	Value* Codegen(CodeGenContext& context) override;
	ConstAST* ErrorC(const char *str){fprintf(stderr, "%s\n", str); return 0;}
};

//所有类型类的基类
class TypeAST : public Node{
public:
	TypeName type;

	TypeAST* ErrorT(const char *str){Error(str); return 0;}
};

//基础类型如string、integer等
class BasicTypeAST : public TypeAST {
public:
	BasicType type;

	BasicTypeAST(BasicType type):type(type){}
	BasicTypeAST* ErrorB(const char *str){Error(str); return 0;}
};

//数组类型
class ArrayTypeAST : public TypeAST {
public:
	TypeAST *type;
	ConstAST *startIndex, *endIndex;

	ArrayTypeAST(TypeAST *ty, ConstAST *s, ConstAST *e):type(ty), startIndex(s), endIndex(e){}
	ArrayTypeAST* ErrorA(const char *str){Error(str); return 0;}
};

//自定义类型
class SelfdefineTypeAST : public TypeAST {
public:
	SelfType type;

	virtual ~SelfDefineAST(){}
	virtual Value* Codegen(CodeGenContext& context) override;
	SelfdefineTypeAST* ErrorS(const char *str){Error(str); return 0;}
};

//record类型
class RecordTypeAST : public SelfdefineTypeAST {
public:
	std::vector<VariableDeclAST*> declList;

	RecordTypeAST(std::vector<VariableDeclAST*> v):declList(v){}
	RecordTypeAST* ErrorR(const char *str){ErrorS(str); return 0;}
	Value* Codegen(CodeGenContext& context) override;
};

//type A=integer;这种自定义类型
class ChangeNameTypeAST : public SelfdefineTypeAST {
public:
	string newName;
	TypeAST* originalType;

	ChangeNameTypeAST(string name, TypeAST* old):newName(name), originalType(old){}
	ChangeNameTypeAST* ErrorC(const char *str){ErrorS(str); return 0;}
	Value* Codegen(CodeGenContext& context) override;
};

//变量声明
class VariableDeclAST : public Node{
public:
	TypeAST *type;
	std::vector<string> variableName;
	VariableDeclAST(const string t, std::vector<string> v):type(t), variableName(v){}
	Value* Codegen(CodeGenContext& context) override;
	VariableDeclAST* ErrorN(const char *str){Error(str); return 0;}
};

//所有表达式类的基类
class ExprAST : public Node{
public:
	ExprType type;

	ExprAST* ErrorE(const char *str){Error(str); return 0;}//这里设置返回，是为了报错时可以扩展为定位到节点而不是简单出现错误信息
};

//整数常数表达式，如“1”
class NumberExprAST : public ExprAST {
public:
	int val;

	NumberExprAST(int value):val(value){}
	Value* Codegen(CodeGenContext& context) override;
	NumberExprAST* ErrorN(const char *str){Error(str); return 0;}
};

//实数常数表达式，如“1.0”
class RealExprAST : public ExprAST {
public:
	double val;

	RealExprAST(double value):val(value){}
	Value* Codegen(CodeGenContext& context) override;
	RealExprAST* ErrorR(const char *str){Error(str); return 0;}
};

//布尔常数表达式，如“true”
class BoolExprAST : public ExprAST {
public:
	bool val;

	BoolExprAST(bool value):val(value){}
	Value* Codegen(CodeGenContext& context) override;
	BoolExprAST* ErrorB(const char *str){Error(str); return 0;}
};

//字符串常数表达式，如“wasd”
class StringExprAST : public ExprAST {
public:
	string val;

	StringExprAST(const string value):val(value){}
	Value* Codegen(CodeGenContext& context) override;
	StringExprAST* ErrorS(const char *str){Error(str); return 0;}
};

//字符常数表达式，如“a”
class CharExprAST : public ExprAST {
public:
	char val;

	CharExprAST(char value):val(value){}
	Value* Codegen(CodeGenContext& context) override;
	CharExprAST* ErrorC(const char *str){Error(str); return 0;}
};

//普通变量表达式，如“i”
class VariableExprAST : public ExprAST {
public:
	string name;

	VariableExprAST(const string variableName):name(variableName){}
	Value* Codegen(CodeGenContext& context) override;
	VariableExprAST* ErrorV(const char *str){Error(str); return 0;}
};

//数组变量表达式，如"a[i]"
class ArrayVariableExprAST : public ExprAST {
public:
	string name;
	ExprAST *index;

	ArrayVariableExprAST(const string variableName, ExprAST *i)
		:name(variableName), index(i){}
	Value* Codegen(CodeGenContext& context) override;
	ArrayVariableExprAST* ErrorA(const char *str){Error(str); return 0;}
};

//记录变量表达式，如"a.p"
class RecordVariableExprAST : public ExprAST {
public:
	string name;
	ExprAST *sub;

	RecordVariableExprAST(const string variableName, ExprAST *s)
		:name(variableName), sub(s){}
	Value* Codegen(CodeGenContext& context) override;
	RecordVariableExprAST* ErrorR(const char *str){Error(str); return 0;}	
};

//一元表达式如"!"、“-”
class UnaryExprAST : public ExprAST {
public:
	UnaryOpKind op;
	ExprAST* expr;

	UnaryOpKind(UnaryOpKind o, ExprAST *e):op(o), expr(e){}
	Value* Codegen(CodeGenContext& context) override;
	UnaryExprAST* ErrorU(const char *str){Error(str); return 0;}
};


//二元表达式如“+”、“-”、“*”、“/”、“=”等
class BinaryExprAST : public ExprAST {
public:
	BinaryOpKind op;
	ExprAST *LExpr, *RExpr;

	BinaryExprAST(BinaryOpKind binaryOp, ExprAST *left, ExprAST *right)
		:op(binaryOp), LExpr(left), RExpr(right){}
	Value* Codegen(CodeGenContext& context) override;
	BinaryExprAST* ErrorB(const char *str){Error(str); return 0;}
};

//调用函数表达式，包含函数名称和参数
class CallExprAST : public ExprAST {
public:
	string callee;
	std::vector<ExprAST*> args;
	bool isSystemCall;

	CallExprAST(const string name, std::vector<ExprAST*> vec, bool is)
		:callee(name), args(vec), isSystemCall(is){}
	Value* Codegen(CodeGenContext& context) override;
	CallExprAST* ErrorC(const char *str){Error(str); return 0;}
};

//if else表达式
class IfExprAST : public ExprAST {
public:
	ExprAST *ifCond；
	std::vector<ExprAST*> thenComponent, elseComponent;

	IfExprAST(ExprAST *cond, std::vector<ExprAST*> thenCo, std::vector<ExprAST*> elseCo)
		:ifCond(cond), thenComponent(thenCo), elseComponent(elseCo){}
	Value* Codegen(CodeGenContext& context) override;
	IfExprAST* ErrorI(const char *str){Error(str); return 0;}
};

//for表达式
class ForExprAST : public ExprAST {
public:
	string varName;
	ExprAST *start, *end;
	std::vector<ExprAST*> body;
	bool increaseDirection;

	ForExprAST(const string name, ExprAST *st, ExprAST *en, std::vector<ExprAST*> bo, bool inc)
		:varName(name), start(st), end(en), body(bo), increaseDirection(inc){}
	Value* Codegen(CodeGenContext& context) override;
	ForExprAST* ErrorF(const char *str){Error(str); return 0;}
};

//while表达式
class WhileExprAST : public ExprAST {
public:
	ExprAST *whileCond;
	std::vector<ExprAST*> body;

	WhileExprAST(ExprAST *cond, std::vector<ExprAST*> bo)
		:whileCond(cond), body(bo){}
	value* Codegen(CodeGenContext& context) override;
	WhileExprAST* ErrorW(const char *str){Error(str); return 0;}
};

//repeat until表达式
class RepeatExprAST : public ExprAST {
public:
	ExprAST* untilCond;
	std::vector<ExprAST*> body;

	RepeatExprAST(ExprAST *cond, std::vector<ExprAST*> bo)
		:untilCond(cond), body(bo){}
	value* Codegen(CodeGenContext& context) override;
	RepeatExprAST* ErrorR(const char *str){Error(str); return 0;}	
};

//函数原型，包含函数名和参数名，暂区分形参和变参
class PrototypeAST : public Node{
public:
	string name;
	std::vector<VariableDeclAST*> decl;
	TypeAST* returnType;

	PrototypeAST(const string funcName, const std::vector<string> &funcArgs)
		:name(funcName), args(funcArgs){}
	Function* Codegen(CodeGenContext& context) override;
	PrototypeAST* ErrorP(const char *str) { Error(str); return 0; }
};

//函数，包含原型和函数体表达式
class FunctionAST : public Node{
public:
  PrototypeAST *proto;
  std::vector<ConstAST*> consts;
  std::vector<SelfdefineTypeAST*> selfdefineType;
  std::vector<VariableDeclAST*> decl;
  std::vector<FunctionAST*> functions;
  std::vector<ExprAST*> body;

  int type;//区分是procedure还是function

  FunctionAST(PrototypeAST *funcProto, std::vector<ConstAST*> con, std::vector<SelfdefineTypeAST*> de, 
  			  std::vector<FunctionAST*> subFun, std::vector<VariableDeclAST*> funcDecl, std::vector<ExprAST*> funcBody)
  			  :proto(funcProto), consts(con), selfdefineType(de), decl(funcDecl), functions(subFun), body(funcBody) {}
  Function* Codegen(CodeGenContext& context) override;
  FunctionAST* ErrorF(const char * str) {Error(str); return 0;}
};

class AstContext{
    AstContext *parent;
    map<string, Type*> typeTable;
    map<string, FunctionAST*> functionTable;
    map<string, Value*> varTable;
public:
    FunctionAST *currentFunc;

    AstContext(AstContext *parent=NULL):parent(parent){
        if(parent != NULL){
            currentFunc = parent->currentFunc;
        }else{
            currentFunc = NULL;
        }
    }

    Type* getType(string name);
    FunctionAST* getFunction(string name);
    Value* getVar(string name);
    bool addFunction(string name, FunctionAST *FunctionAST);
    bool addVar(string name, Value *var);
    bool addType(string name, Type *type);
};