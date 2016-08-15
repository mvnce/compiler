#ifndef AST_H
#define AST_H

#include <string>
#include <vector>
#include <fstream>

#include "tc.h"
#include "type_info.h"
#include "symbol_table.h"

// The classes in this file hold info about the nodes that form the Abstract Syntax Tree (AST)
//
// ASTNode : The base class for all of the others, with useful virtual functions.
//
// ASTNode_TempNode : AST Node that will be replaced (used for argument lists).
// ASTNode_Root : Blocks of statements, including the overall program.
// ASTNode_Variable : Leaf node containing a variable.
// ASTNode_Literal : Leaf node contiaing a literal value.
// ASTNode_Assign : Assignements
// ASTNode_Math1 : One-input math operations (unary '-' and '!')
// ASTNode_Math2 : Two-input math operations ('+', '-', '*', '/', and comparisons)
// ASTNode_Bool2 : Two-input bool operations ('&&' and '||')
// ASTNode_ArrayAccess : Index into an array
// ASTNode_MethodCall : Currently, array methods .size() and .resize()
// ASTNode_If    : If-conditional node.
// ASTNode_While : While-loop node.
// ASTNode_For   : For-loop node.
// ASTNode_Break : Break node
// ASTNode_Continue : Continue node
// ASTNode_Print : Print command

class ASTNode {
protected:
  int type;                        // Type this node will pass up the AST
  int line_num;                    // Original line from source program
  std::vector<ASTNode *> children; // What sub-trees does this node have?

  void SetType(int new_type) { type = new_type; } // Use inside constructor only!
public:
  ASTNode(int in_type) : type(in_type), line_num(-1) { ; }
  virtual ~ASTNode() {
    for (int i = 0; i < (int) children.size(); i++) {
      delete children[i];
    }
  }

  int GetType() { return type; }
  int GetLineNum() { return line_num; }
  ASTNode * GetChild(int id) { return children[id]; }
  int GetNumChildren() { return children.size(); }

  void SetLineNum(int _in) { line_num = _in; }
  void SetChild(int id, ASTNode * in_node) { children[id] = in_node; }

  void AddChild(ASTNode * in_child) { children.push_back(in_child); }
  void TransferChildren(ASTNode * in_node);

  // Convert a single node to TubeIC and return information about the
  // variable where the results are saved.  Call children recursively.
  virtual tableEntry * CompileTubeIC(symbolTable & table, TC_Array & ica) = 0;

  // Return the name of the node being called.  This function is useful for debbing the AST.
  virtual std::string GetName() { return "ASTNode (base class)"; }
};


// A placeholder in the AST.
class ASTNode_TempNode : public ASTNode {
public:
  ASTNode_TempNode(int in_type) : ASTNode(in_type) { ; }
  ~ASTNode_TempNode() { ; }
  tableEntry * CompileTubeIC(symbolTable & table, TC_Array & ica) { return NULL; }

  virtual std::string GetName() {
    return "ASTNode_TempNode (under construction)";
  }
};

// Root...
class ASTNode_Root : public ASTNode {
public:
  ASTNode_Root() : ASTNode(Type::VOID) { ; }
  tableEntry * CompileTubeIC(symbolTable & table, TC_Array & ica);

  virtual std::string GetName() { return "ASTNode_Root (container class)"; }
};

// Leaves...
class ASTNode_Variable : public ASTNode {
private:
  tableEntry * var_entry;
public:
  ASTNode_Variable(tableEntry * in_entry)
    : ASTNode(in_entry->GetType()), var_entry(in_entry) {;}

  tableEntry * GetVarEntry() { return var_entry; }
  tableEntry * CompileTubeIC(symbolTable & table, TC_Array & ica);

  virtual std::string GetName() {
    std::string out_string = "ASTNode_Variable (";
    out_string += var_entry->GetName();
    out_string += ")";
    return out_string;
  }
};

class ASTNode_Literal : public ASTNode {
private:
  std::string lexeme;     // When we print, how should this node look?
public:
  ASTNode_Literal(int in_type, std::string in_lex);
  tableEntry * CompileTubeIC(symbolTable & table, TC_Array & ica);

  virtual std::string GetName() {
    std::string out_string = "ASTNode_Literal (";
    out_string += lexeme;
    out_string += ")";
    return out_string;
  }
};

// Math...

class ASTNode_Assign : public ASTNode {
public:
  ASTNode_Assign(ASTNode * lhs, ASTNode * rhs);
  ~ASTNode_Assign() { ; }

  tableEntry * CompileTubeIC(symbolTable & table, TC_Array & ica);
  virtual std::string GetName() { return "ASTNode_Assign (operator=)"; }
};

class ASTNode_Math1 : public ASTNode {
protected:
  int math_op;
  static int ChooseType(ASTNode * child);
public:
  ASTNode_Math1(ASTNode * in_child, int op);
  virtual ~ASTNode_Math1() { ; }

  tableEntry * CompileTubeIC(symbolTable & table, TC_Array & ica);
  virtual std::string GetName() {
    std::string out_string = "ASTNode_Math1 (operator";
    out_string += (char) math_op;
    out_string += ")";
    return out_string;
  }
};

class ASTNode_Math2 : public ASTNode {
protected:
  int math_op;
  static int ChooseType(ASTNode * child1, ASTNode * child2);
public:
  ASTNode_Math2(ASTNode * in1, ASTNode * in2, int op);
  virtual ~ASTNode_Math2() { ; }

  tableEntry * CompileTubeIC(symbolTable & table, TC_Array & ica);
  virtual std::string GetName() {
    std::string out_string = "ASTNode_Math2 (operator";
    out_string += (char) math_op;
    out_string += ")";
    return out_string;
  }
};

class ASTNode_Bool2 : public ASTNode {
protected:
  int bool_op;
  static int ChooseType(ASTNode * child1, ASTNode * child2);
public:
  ASTNode_Bool2(ASTNode * in1, ASTNode * in2, int op);
  virtual ~ASTNode_Bool2() { ; }

  tableEntry * CompileTubeIC(symbolTable & table, TC_Array & ica);
  virtual std::string GetName() {
    std::string out_string = "ASTNode_Bool2 (operator";
    out_string += (char) bool_op;
    out_string += ")";
    return out_string;
  }
};

class ASTNode_ArrayAccess : public ASTNode {
protected:
public:
  ASTNode_ArrayAccess(ASTNode * in1, ASTNode * in2);
  virtual ~ASTNode_ArrayAccess() { ; }

  tableEntry * CompileTubeIC(symbolTable & table, TC_Array & ica);
  virtual std::string GetName() {
    std::string out_string = "ASTNode_ArrayAccess";
    return out_string;
  }
};


class ASTNode_MethodCall : public ASTNode {
protected:
  std::string name;
public:
  ASTNode_MethodCall(ASTNode * in_base, std::string in_name);
  virtual ~ASTNode_MethodCall() { ; }

  tableEntry * CompileTubeIC(symbolTable & table, TC_Array & ica);
  virtual std::string GetName() {
    std::string out_string = "ASTNode_MethodCall";
    return out_string;
  }

  void TypeCheckArgs();  // Run after args are added.
};

class ASTNode_If : public ASTNode {
protected:
public:
  ASTNode_If(ASTNode * in1, ASTNode * in2, ASTNode * in3);
  virtual ~ASTNode_If() { ; }

  tableEntry * CompileTubeIC(symbolTable & table, TC_Array & ica);
  virtual std::string GetName() {
    std::string out_string = "ASTNode_If";
    return out_string;
  }
};

class ASTNode_While : public ASTNode {
protected:
public:
  ASTNode_While(ASTNode * in1, ASTNode * in2);
  virtual ~ASTNode_While() { ; }

  tableEntry * CompileTubeIC(symbolTable & table, TC_Array & ica);
  virtual std::string GetName() {
    std::string out_string = "ASTNode_While";
    return out_string;
  }
};

class ASTNode_For : public ASTNode {
protected:
public:
  ASTNode_For(ASTNode * node_init, ASTNode * node_test, ASTNode * node_inc, ASTNode * node_body);
  virtual ~ASTNode_For() { ; }

  tableEntry * CompileTubeIC(symbolTable & table, TC_Array & ica);
  virtual std::string GetName() {
    std::string out_string = "ASTNode_For";
    return out_string;
  }
};

class ASTNode_Break : public ASTNode {
protected:
public:
  ASTNode_Break();
  virtual ~ASTNode_Break() { ; }

  tableEntry * CompileTubeIC(symbolTable & table, TC_Array & ica);
  virtual std::string GetName() {
    std::string out_string = "ASTNode_Break";
    return out_string;
  }
};

class ASTNode_Continue : public ASTNode {
protected:
public:
  ASTNode_Continue();
  virtual ~ASTNode_Continue() { ; }

  tableEntry * CompileTubeIC(symbolTable & table, TC_Array & ica);
  virtual std::string GetName() {
    std::string out_string = "ASTNode_Continue";
    return out_string;
  }
};

class ASTNode_Print : public ASTNode {
public:
  ASTNode_Print(ASTNode * out_child);
  virtual ~ASTNode_Print() {;}

  tableEntry * CompileTubeIC(symbolTable & table, TC_Array & ica);
  virtual std::string GetName() { return "ASTNode_Print"; }
};

class ASTNode_Random : public ASTNode {
public:
  ASTNode_Random(ASTNode * in_child);
  virtual ~ASTNode_Random() { ; }

  tableEntry * CompileTubeIC(symbolTable & table, TC_Array & ica);
  virtual std::string GetName() { return "ASTNode_Random"; }
};

class ASTNode_Ternary : public ASTNode {
public:
  ASTNode_Ternary(ASTNode * child_test, ASTNode * child_true, ASTNode * child_false);
  virtual ~ASTNode_Ternary() { ; }

  tableEntry * CompileTubeIC(symbolTable & table, TC_Array & ica);
  std::string GetName() { return "ASTNode_Ternary"; }
};


#endif
