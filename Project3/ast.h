#ifndef AST_H
#define AST_H

#include <iostream>
#include <string>
#include <vector>
#include <fstream>

#include "symbol_table.h"



class ASTNode_Base {
protected:
  int type;                              // What type should this node pass up?
  std::vector<ASTNode_Base *> children;  // What sub-trees does this node have?

  // NOTE: Other basic AST information should go here, as needed.
  
public:
  ASTNode_Base(int in_type) : type(in_type) { ; }
  virtual ~ASTNode_Base() {
    for (int i = 0; i < children.size(); i++) {
      delete children[i];
    }
  }
  static int GetID() {
    static int next_id = 0;
    return next_id++;
  }
  int GetType() { return type; }

  ASTNode_Base * GetChild(int id) { return children[id]; }
  int GetNumChildren() { return children.size(); }
  void AddChild(ASTNode_Base * in_child) { children.push_back(in_child); }
  void TransferChildren(ASTNode_Base * in_node);

  // Process a single node's calculations and return a variable pointer that
  // represents the result.  Call child nodes recursively....

  virtual void Debug(int indent=0);
  virtual std::string GetName() { return "ASTNode_Base"; }

  virtual int ProcessIC(FILE * outfile) = 0;

};



// This node is built as a placeholder in the AST.
class ASTNode_Temp : public ASTNode_Base {
public:
  ASTNode_Temp(int in_type) : ASTNode_Base(in_type) { ; }
  ~ASTNode_Temp() { ; }
  virtual std::string GetName() {
    return "ASTNode_Temp (under construction)";
  }

  int ProcessIC(FILE * outfile) { ; }
};

// Root...
class ASTNode_Root : public ASTNode_Base {
public:
  ASTNode_Root() : ASTNode_Base(Type::VOID) { ; }

  virtual std::string GetName() { return "ASTNode_Root (container class)"; }
  
  int ProcessIC(FILE * outfile) {
    int out_id = GetID();
    for (int i = 0; i < children.size(); i++) {
      children[i]->ProcessIC(outfile);
    }
    return out_id;
  }
};

// Leaves...

class ASTNode_Variable : public ASTNode_Base {
private:
  tableEntry * var_entry;
  //int i;
public:
  ASTNode_Variable(tableEntry * in_entry)
    : ASTNode_Base(in_entry->GetType()), var_entry(in_entry) {;}

  tableEntry * GetVarEntry() { return var_entry; }

  virtual std::string GetName() {
    std::string out_string = "ASTNode_Variable (";
    out_string += var_entry->GetName();
    out_string += ")";
    return out_string;
  }
  
  int ProcessIC(FILE * outfile) {
    
    // return out_id;
    if(var_entry->GetSID() == 0)
    {
      int out_id = GetID();
      var_entry->SetSID(out_id);
    }
      
    return var_entry->GetSID();
  }
};

class ASTNode_Literal : public ASTNode_Base {
private:
  std::string lexeme;                 // When we print, how should this node look?
public:
  ASTNode_Literal(int in_type, std::string in_lex)
    : ASTNode_Base(in_type), lexeme(in_lex) { ; }  

  virtual std::string GetName() {
    std::string out_string = "ASTNode_Literal (";
    out_string += lexeme;
    out_string += ")";
    return out_string;
  }

  int ProcessIC(FILE * outfile) {
    int out_id = GetID();
    // std::cout << "  val_copy " << lexeme << " s" << out_id << std::endl;
    fprintf (outfile, "  val_copy %s s%i\n", lexeme.c_str(), out_id);
    return out_id;
  }
};

// Math...

class ASTNode_Assign : public ASTNode_Base {
public:
  ASTNode_Assign(ASTNode_Base * lhs, ASTNode_Base * rhs)
    : ASTNode_Base(lhs->GetType())
  {  children.push_back(lhs);  children.push_back(rhs);  }
  ~ASTNode_Assign() { ; }

  virtual std::string GetName() { return "ASTNode_Assign (operator=)"; }

  int ProcessIC(FILE * outfile) {
    //int out_id = GetID();
    int lhs = children[0]->ProcessIC(outfile);
    int rhs = children[1]->ProcessIC(outfile);
    // std::cout << "  val_copy s" << rhs << " s" << lhs << std::endl;
    fprintf (outfile, "  val_copy s%i s%i\n", rhs, lhs);
    return lhs;
  }
};

class ASTNode_Math1_Minus : public ASTNode_Base {
public:
  ASTNode_Math1_Minus(ASTNode_Base * child) : ASTNode_Base(Type::VAL)
  { children.push_back(child); }
  ~ASTNode_Math1_Minus() { ; }

  virtual std::string GetName() { return "ASTNode_Math1_Minus (unary -)"; }
  
  int ProcessIC(FILE * outfile) {
    int var1 = children[0]->ProcessIC(outfile);
    int out_id = GetID();
    fprintf (outfile,"sub 0 s%i s%i\n",var1,out_id);
    //std::cout << "math1" << std::endl;
    return out_id;
  }
};

class ASTNode_Math2 : public ASTNode_Base {
protected:
  char math_op;
public:
  ASTNode_Math2(ASTNode_Base * in1, ASTNode_Base * in2, char op);
  virtual ~ASTNode_Math2() { ; }

  virtual std::string GetName() {
    std::string out_string = "ASTNode_Math2 (operator";
    out_string += math_op;
    out_string += ")";
    return out_string;
  }

  int ProcessIC(FILE * outfile) {
    int var1 = children[0]->ProcessIC(outfile);
    int var2 = children[1]->ProcessIC(outfile);
    int out_id = GetID();

    switch(math_op) {
    // case '+': std::cout << "add s"; break;
    // case '-': std::cout << "sub s"; break;
    // case '*': std::cout << "mult s"; break;
    // case '/': std::cout << "div s"; break;

    case '+': fprintf (outfile,"  add s"); break;
    case '-': fprintf (outfile,"  sub s"); break;
    case '*': fprintf (outfile,"  mult s"); break;
    case '/': fprintf (outfile,"  div s"); break;
    case 'Q': fprintf (outfile,"  test_equ s"); break;
    case 'U': fprintf (outfile,"  test_nequ s"); break;
    case 'L': fprintf (outfile,"  test_less s"); break;
    case 'T': fprintf (outfile,"  test_lte s"); break;
    case 'R': fprintf (outfile,"  test_gtr s"); break;
    case 'E': fprintf (outfile,"  test_gte s"); break;
    default:
      std::cerr << "Internal Compiler ERROR!!  AHHHHH!!!!" << std::endl;
    }

    // std::cout << var1 << " s" << var2 << " s" << out_id << std::endl;
    fprintf (outfile,"%i s%i s%i\n",var1,var2,out_id );

    return out_id;
  }
};
class ASTNode_Math3 : public ASTNode_Base {
protected:
  char math_op;
public:
  ASTNode_Math3(ASTNode_Base * in1, ASTNode_Base * in2, char op) 
    : ASTNode_Base(1)
  { children.push_back(in1);  children.push_back(in2); math_op = op; }
  virtual ~ASTNode_Math3() { ; }

  virtual std::string GetName() {
    std::string out_string = "ASTNode_Math3 (operator";
    out_string += math_op;
    out_string += ")";
    return out_string;
  }

  int ProcessIC(FILE * outfile) {

    switch(math_op) {
    
    case 'A': {
      if (children[1] == NULL)
      {
        std::cout << "null" << std::endl;
      }
      int var1 = children[0]->ProcessIC(outfile);
      int out_id = GetID();
      int boolJump = var1-1;
      //int var0 = args[0]->WriteIC(nvar);
      //int out_var = nvar++;
      //cout << "test_nequ" << " s" << var0 << " 0" << " s" << nvar << " # Move answer and convert to 0/1" << endl;
      fprintf (outfile,"  test_nequ s%i 0 s%i\n",var1,out_id);
      //cout << "jump_if_0" << " s" << nvar << " BOOL_END_" << boolJump << " # Short circuit if first part = 0" << endl;
      fprintf (outfile,"  jump_if_0 s%i end_bool_%i\n",out_id,boolJump);
      //int var1 = args[1]->WriteIC(--nvar);
      int var2 = children[1]->ProcessIC(outfile);
      //out_var = nvar++;
      //out_id = GetID();
      //cout << "test_nequ" << " s" << var1 << " 0" << " s" << out_var << " # Move answer and conver to 0/1" << endl;
      fprintf (outfile,"  test_nequ s%i 0 s%i\n",var2,out_id);
      //cout << "BOOL_END_" << boolJump << ":" << endl;
      fprintf (outfile,"end_bool_%i:\n",boolJump);
      return out_id;
      break;
    }
    case 'O': {
      
      int var1 = children[0]->ProcessIC(outfile);
      int out_id = GetID();
      int boolJump = var1-1;
      //int var0 = args[0]->WriteIC(nvar);
      //int out_var = nvar++;
      //cout << "test_equ" << " s" << var1 << " 0" << " s" << nvar << " # Move answer and convert to 0/1" << endl;
      fprintf (outfile,"  test_nequ s%i 0 s%i\n",var1,out_id);
      //cout << "jump_if_n0" << " s" << nvar << " BOOL_END_" << boolJump << " # Short circuit if first part = 0" << endl;
      fprintf (outfile,"  jump_if_n0 s%i end_bool_%i\n",out_id,boolJump);
      int var2 = children[1]->ProcessIC(outfile);
      //int var1 = args[1]->WriteIC(--nvar);//might need to be --
      //out_var = nvar++;
      //out_id = GetID();
      //cout << "test_equ" << " s" << var2 << " 0" << " s" << out_id << " # Move answer and conver to 0/1" << endl;
      fprintf (outfile,"  test_nequ s%i 0 s%i\n",var2,out_id);
      //cout << "BOOL_END_" << boolJump << ":" << endl;
      fprintf (outfile,"end_bool_%i:\n",boolJump);
      return out_id;
      break;
    }
    default:
      std::cerr << "Internal Compiler ERROR!!  AHHHHH!!!!" << std::endl;
    }

    // std::cout << var1 << " s" << var2 << " s" << out_id << std::endl;
    //fprintf (outfile,"%i s%i s%i\n",var1,var2,out_id );

    //return out_id;
  }

};

class ASTNode_Math4 : public ASTNode_Base {
protected:
  char math_op;
public:
  ASTNode_Math4(ASTNode_Base * in1, ASTNode_Base * in2, char op) 
    : ASTNode_Base(1)
  { children.push_back(in1);  children.push_back(in2); math_op = op; }
  virtual ~ASTNode_Math4() { ; }

  virtual std::string GetName() {
    std::string out_string = "ASTNode_Math4 (operator";
    out_string += math_op;
    out_string += ")";
    return out_string;
  }

  int ProcessIC(FILE * outfile) {

    switch(math_op) {
    
    case 'A': {
      if (children[1] == NULL)
      {
        std::cout << "null" << std::endl;
      }
      int var1 = children[0]->ProcessIC(outfile);
      int out_id = GetID();
      int boolJump = var1-1;
      //int var0 = args[0]->WriteIC(nvar);
      //int out_var = nvar++;
      //cout << "test_nequ" << " s" << var0 << " 0" << " s" << nvar << " # Move answer and convert to 0/1" << endl;
      fprintf (outfile,"  test_nequ s%i 0 s%i\n",var1,out_id);
      //cout << "jump_if_0" << " s" << nvar << " BOOL_END_" << boolJump << " # Short circuit if first part = 0" << endl;
      fprintf (outfile,"  jump_if_0 s%i end_bool_%i\n",out_id,boolJump);
      //int var1 = args[1]->WriteIC(--nvar);
      int var2 = children[1]->ProcessIC(outfile);
      //out_var = nvar++;
      //out_id = GetID();
      //cout << "test_nequ" << " s" << var1 << " 0" << " s" << out_var << " # Move answer and conver to 0/1" << endl;
      fprintf (outfile,"  test_nequ s%i 0 s%i\n",var2,out_id);
      //cout << "BOOL_END_" << boolJump << ":" << endl;
      fprintf (outfile,"end_bool_%i:\n",boolJump);
      return out_id;
      break;
    }
    default:
      std::cerr << "Internal Compiler ERROR!!  AHHHHH!!!!" << std::endl;
    }

    // std::cout << var1 << " s" << var2 << " s" << out_id << std::endl;
    //fprintf (outfile,"%i s%i s%i\n",var1,var2,out_id );

    //return out_id;
  }

};

class ASTNode_Random : public ASTNode_Base {
public:
  ASTNode_Random(ASTNode_Base * child) : ASTNode_Base(Type::VAL)
  { children.push_back(child); }
  ~ASTNode_Random() { ; }

  virtual std::string GetName() { return "ASTNode_Random (unary -)"; }
  
  int ProcessIC(FILE * outfile) {
    int var1 = children[0]->ProcessIC(outfile);
    int out_id = GetID();
    fprintf (outfile,"random s%i s%i\n",var1,out_id);
    //std::cout << "math1" << std::endl;
    return out_id;
  }
};

class ASTNode_Print : public ASTNode_Base {
public:
  ASTNode_Print() : ASTNode_Base(Type::VOID) { ; }
  virtual ~ASTNode_Print() {;}

  virtual std::string GetName() { return "ASTNode_Print (print command)"; }

  int ProcessIC(FILE * outfile) {
    for (size_t i = 0; i < children.size(); i++) {
      int cnt = children[i]->ProcessIC(outfile);
      // std::cout << "  out_val s" << cnt << std::endl;
      fprintf (outfile,"  out_val s%i\n",cnt);
    }

    // std::cout << "  out_char '\\n'" << std::endl;
    fprintf (outfile,"  out_char '\\n'\n");
  }
};

#endif