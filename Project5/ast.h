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
  bool isArr;
  int s_id;
  int index;
  // NOTE: Other basic AST information should go here, as needed.
  
public:
  ASTNode_Base(int in_type) : type(in_type) { isArr = false; }
  virtual ~ASTNode_Base() {
    for (int i = 0; i < children.size(); i++) {
      delete children[i];
    }
  }
  static int GetID() {
    static int next_id = 1;
    return next_id++;
  }

  static int JumpID() {
    static int next_id = 0;
    return next_id++;
  }
  int GetIndex() { return index; }
  int GetType() { return type; }
  int GetSid() { return s_id; }
  bool GetIsArr() { return isArr; }
  ASTNode_Base * GetChild(int id) { return children[id]; }
  int GetNumChildren() { return children.size(); }
  void AddChild(ASTNode_Base * in_child) { children.push_back(in_child); }
  void TransferChildren(ASTNode_Base * in_node);
  void SetIsArr(bool is) { isArr = is; }
  void SetIndex(int i) { index = i; }

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

class ASTNode_arr_Variable : public ASTNode_Base {
private:
  tableEntry * var_entry;
  int i;
public:
  ASTNode_arr_Variable(tableEntry * in_entry, ASTNode_Base * child)
    : ASTNode_Base(in_entry->GetType()), var_entry(in_entry) { i = var_entry->GetIndex(); 
      if(in_entry->GetType() == 3) type = 2;
      else if (in_entry->GetType() == 4) type = 1;
    children.push_back(child);
    }

  tableEntry * GetVarEntry() { return var_entry; }

  virtual std::string GetName() {
    std::string out_string = "ASTNode_arr_Variable (";
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
      //int tep_id = GetID();
      int var1 = children[0]->ProcessIC(outfile);
      int out_id = GetID();
      index = var1;
      //fprintf (outfile, "  val_copy %i s%i\n", i, tep_id);
      fprintf (outfile, "  ar_get_idx a%i s%i s%i\n", var_entry->GetSID(), var1, out_id);
      SetIsArr(true);
      s_id = var_entry->GetSID();
      return out_id;
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
    fprintf (outfile, "  val_copy %s s%i\n", lexeme.c_str(), out_id);
    return out_id;
  }
};

class ASTNode_Char : public ASTNode_Base {
private:
  std::string lexeme;                 // When we print, how should this node look?
public:
  ASTNode_Char(int in_type, std::string in_lex)
    : ASTNode_Base(in_type), lexeme(in_lex) { ; }  

  virtual std::string GetName() {
    std::string out_string = "ASTNode_Char (";
    out_string += lexeme;
    out_string += ")";
    return out_string;
  }

  int ProcessIC(FILE * outfile) {
    int s = lexeme.length() - 2;
    int out_id = GetID();
    
    for (int i = 1; i < lexeme.length()-1; i++)
    {
      std::string out;
      if(lexeme[i] == '\\')
      {
        if(lexeme[i+1] == '\"')
        {
          out = "' '";
          out[1] = lexeme[i+1];

        }
        else
        {
          out = "'  '";
          out[1] = lexeme[i];
          out[2] = lexeme[i+1];
        }
          i++;
          s--;
      }
      else
      {
          out = "' '";
          out[1] = lexeme[i];
      }
    }
    fprintf (outfile, "  ar_set_siz a%i %i\n", out_id, s);
    int cnt = 0;
    for (int i = 1; i < lexeme.length()-1; i++)
    {
      std::string out;
      if(lexeme[i] == '\\')
      {
        if(lexeme[i+1] == '\"')
        {
          out = "' '";
          out[1] = lexeme[i+1];

        }
        else
        {
          out = "'  '";
          out[1] = lexeme[i];
          out[2] = lexeme[i+1];
        }
          i++;
          s--;
      }
      else
      {
          out = "' '";
          out[1] = lexeme[i];
      }
      
      fprintf (outfile, "  ar_set_idx a%i %i %s\n", out_id, cnt, out.c_str());
      cnt++;
    }
    
    return out_id;
  }
};

class ASTNode_Method : public ASTNode_Base {
private:
  std::string lexeme;                 // When we print, how should this node look?
public:
  ASTNode_Method(ASTNode_Base * child, std::string in_lex)
    : ASTNode_Base(Type::VAL), lexeme(in_lex) { children.push_back(child);
        if(child->GetType() == 3 && in_lex == "pop") type = 2;
     } 

  ASTNode_Method(ASTNode_Base * lhs, std::string in_lex, ASTNode_Base * rhs)
    : ASTNode_Base(Type::VAL), lexeme(in_lex) { children.push_back(lhs); children.push_back(rhs);
       if(lhs->GetType() == 3 && in_lex == "pop") type = 2;
     } 

  virtual std::string GetName() {
    std::string out_string = "ASTNode_Method (";
    out_string += lexeme;
    out_string += ")";
    return out_string;
  }

  int ProcessIC(FILE * outfile) {
    if (lexeme == "size")
    {
      int var1 = children[0]->ProcessIC(outfile);
      int out_id = GetID();
      fprintf (outfile, "  ar_get_siz a%i s%i\n", var1, out_id);
      return out_id;
    }
    else if (lexeme == "resize")
    {
      int var1 = children[0]->ProcessIC(outfile);
      int var2 = children[1]->ProcessIC(outfile);
      fprintf (outfile, "  ar_set_siz a%i s%i\n", var1, var2);
      return 0;
    }
    else if (lexeme == "pop")
    {
      int var1 = children[0]->ProcessIC(outfile);
      int out_id = GetID();
      int out_id2 = GetID();
      fprintf (outfile, "  ar_get_siz a%i s%i\n", var1, out_id2);
      fprintf (outfile, "  sub s%i 1 s%i\n", out_id2, out_id2);
      fprintf (outfile, "  ar_get_idx a%i s%i s%i\n", var1, out_id2, out_id);
      fprintf (outfile, "  ar_set_siz a%i s%i\n", var1, out_id2);
      return out_id;
    }
    else if (lexeme == "push")
    {
      int var1 = children[0]->ProcessIC(outfile);
      int var2 = children[1]->ProcessIC(outfile);
      int out_id = GetID();
      int out_id2 = GetID();
      fprintf (outfile, "  ar_get_siz a%i s%i\n", var1, out_id);
      fprintf (outfile, "  add 1 s%i s%i\n", out_id, out_id2);
      fprintf (outfile, "  ar_set_siz a%i s%i\n", var1, out_id2); 
      fprintf (outfile, "  ar_set_idx a%i s%i s%i\n", var1, out_id, var2);
      return 0;

    }
  }
};

// Math...

class ASTNode_Assign : public ASTNode_Base {
public:
  ASTNode_Assign(ASTNode_Base * lhs, ASTNode_Base * rhs);
  ~ASTNode_Assign() { ; }

  virtual std::string GetName() { return "ASTNode_Assign (operator=)"; }

  int ProcessIC(FILE * outfile) {
    //int out_id = GetID();
    int lhs = children[0]->ProcessIC(outfile);
    int rhs = children[1]->ProcessIC(outfile);
    if (children[0]->GetType() == 3 && children[1]->GetType() == 3)
    {
      fprintf (outfile, "  ar_copy a%i a%i\n", rhs, lhs);
    }
    else if (children[0]->GetIsArr())
    {
      fprintf (outfile, "  ar_set_idx a%i s%i s%i\n", children[0]->GetSid(), children[0]->GetIndex(), rhs);
    }
    else
    {
      fprintf (outfile, "  val_copy s%i s%i\n", rhs, lhs);
    }

    
    return lhs;
  }
};

class ASTNode_Not : public ASTNode_Base {
public:
  ASTNode_Not(ASTNode_Base * child) : ASTNode_Base(Type::VAL)
  { children.push_back(child); }
  ~ASTNode_Not() { ; }
};

class ASTNode_Math0_Not : public ASTNode_Base {
public:
  ASTNode_Math0_Not(ASTNode_Base * child) : ASTNode_Base(Type::VAL)
  { children.push_back(child); }
  ~ASTNode_Math0_Not() { ; }

  virtual std::string GetName() { return "ASTNode_Math0_Not (boolean !)"; }

  int ProcessIC(FILE * outfile) {
    int var1 = children[0]->ProcessIC(outfile);
    int out_id = GetID();
    fprintf (outfile,"  test_equ s%i 0 s%i\n",var1,out_id);
    return out_id;
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
    fprintf (outfile,"  sub 0 s%i s%i\n",var1,out_id);
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
    int lhs = children[0]->ProcessIC(outfile);
    int rhs = children[1]->ProcessIC(outfile);
    
    int out_id = GetID();

    switch(math_op) {
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
    fprintf (outfile, "%i s%i s%i\n", lhs, rhs, out_id );

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

      fprintf (outfile,"  test_nequ s%i 0 s%i\n",var1, out_id);
      fprintf (outfile,"  jump_if_0 s%i end_bool_%i\n",out_id, boolJump);
      int var2 = children[1]->ProcessIC(outfile);
      fprintf (outfile,"  test_nequ s%i 0 s%i\n",var2,out_id);
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




class ASTNode_If : public ASTNode_Base {
private:
  ASTNode_Base * in_p2;
  ASTNode_Base * in_p3;

public:
  ASTNode_If(ASTNode_Base * in1, ASTNode_Base * in2, ASTNode_Base * in3) : ASTNode_Base(Type::VAL) {
    children.push_back(in1);
    children.push_back(in2);
    children.push_back(in3);
    in_p2 = in2;
    in_p3 = in3;
  }

  ~ASTNode_If() { ; }

  virtual std::string GetName() { return "ASTNode_If (ctrl)"; }

  int ProcessIC(FILE * outfile) {

    if (in_p3 == NULL && in_p2 != NULL) {
      int lhs = children[0]->ProcessIC(outfile);
      int out_id = GetID();
      int boolJump = lhs-1;
      fprintf (outfile,"  jump_if_0 s%i if_else_%i\n",lhs, boolJump);

      int rhs = children[1]->ProcessIC(outfile);
      fprintf (outfile,"if_else_%i:\n",boolJump);
      fprintf (outfile,"if_end_%i:\n",boolJump);
    } 

    else if (in_p2 == NULL) {
      int lhs = children[0]->ProcessIC(outfile);
      int out_id = GetID();
      int boolJump = lhs-1;
      int jump_id = JumpID();
      fprintf (outfile,"  jump_if_0 s%i if_else_%i\n",lhs, jump_id);

      fprintf (outfile,"  jump if_end_%i\n", jump_id);
      fprintf (outfile,"if_else_%i:\n", jump_id);
      int var = children[2]->ProcessIC(outfile);
      fprintf (outfile,"if_end_%i:\n", jump_id);
    }

    else {
      int lhs = children[0]->ProcessIC(outfile);
      int out_id = GetID();
      int boolJump = lhs-1;
      fprintf (outfile,"  jump_if_0 s%i if_else_%i\n",lhs, boolJump);

      int rhs = children[1]->ProcessIC(outfile);
      fprintf (outfile,"jump if_end_%i\n", boolJump);
      fprintf (outfile,"if_else_%i:\n", boolJump);
      int var = children[2]->ProcessIC(outfile);
      fprintf (outfile,"if_end_%i:\n", boolJump);
    }


  }

};


class ASTNode_While : public ASTNode_Base {
  private:
    int while_id;
public:
  ASTNode_While(ASTNode_Base * in1, ASTNode_Base * in2, int id) : ASTNode_Base(Type::VAL) {
    children.push_back(in1);
    children.push_back(in2);
    while_id = id;
  }

  ~ASTNode_While() { ; }

  virtual std::string GetName() { return "ASTNode_While (ctrl)"; }

  int ProcessIC(FILE * outfile) {

    fprintf (outfile, "while_start_%i:\n", while_id);
    int lhs = children[0]->ProcessIC(outfile);
    int out_id = GetID();
    int boolJump = lhs-1;
    fprintf (outfile,"  jump_if_0 s%i while_end_%i\n",out_id-1, while_id+1);

    int rhs = children[1]->ProcessIC(outfile);
    fprintf (outfile,"  jump while_start_%i\n", while_id);
    fprintf (outfile,"while_end_%i:\n",while_id+1);
  }

};

class ASTNode_Break : public ASTNode_Base {
private:
  int while_id;
public:
  ASTNode_Break(int id) : ASTNode_Base(Type::VOID) { while_id = id; }
  ~ASTNode_Break() { ; }

  virtual std::string GetName() { return "ASTNode_Break (ctrl)"; }

  int ProcessIC(FILE * outfile) {
    fprintf(outfile, "  jump while_end_%i\n", while_id+1);
  }

};


class ASTNode_CONTINUE : public ASTNode_Base {
private:
  int while_id;
public:
  ASTNode_CONTINUE(int id) : ASTNode_Base(Type::VOID) { while_id = id; }
  ~ASTNode_CONTINUE() { ; }

  virtual std::string GetName() { return "ASTNode_CONTINUE (ctrl)"; }

  int ProcessIC(FILE * outfile) {
    fprintf(outfile, "  jump while_start_%i\n", while_id+1);
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

      if (children[i]->GetType() == 1) {
        fprintf (outfile,"  out_val s%i\n",cnt);
      }
      else if (children[i]->GetType() == 3) 
      {
        int out_id2 = GetID();
        int out_id3 = GetID();
        int out_id4 = GetID();
        fprintf (outfile,"  val_copy 0 s%i\n",out_id3);
        fprintf (outfile,"  ar_get_siz a%i s%i\n",cnt, out_id2);
        fprintf (outfile,"print_array_start_%i:\n",out_id2-1);
        fprintf (outfile,"  test_gte s%i s%i s%i\n",out_id3, out_id2, out_id4);
        fprintf (outfile,"  jump_if_n0 s%i print_array_end_%i\n",out_id4, out_id3-1);
        fprintf (outfile,"  ar_get_idx a%i s%i s%i\n",cnt, out_id3, out_id4);
        fprintf (outfile,"  out_char s%i\n",out_id4);
        fprintf (outfile,"  add s%i 1 s%i\n",out_id3, out_id3);
        fprintf (outfile,"  jump print_array_start_%i\n",out_id2-1);
        fprintf (outfile,"print_array_end_%i:\n",out_id3-1);
      }
      else if (children[i]->GetType() == 4) 
      {
        int out_id2 = GetID();
        int out_id3 = GetID();
        int out_id4 = GetID();
        fprintf (outfile,"  val_copy 0 s%i\n",out_id3);
        fprintf (outfile,"  ar_get_siz a%i s%i\n",cnt, out_id2);
        fprintf (outfile,"print_array_start_%i:\n",out_id2-1);
        fprintf (outfile,"  test_gte s%i s%i s%i\n",out_id3, out_id2, out_id4);
        fprintf (outfile,"  jump_if_n0 s%i print_array_end_%i\n",out_id4, out_id3-1);
        fprintf (outfile,"  ar_get_idx a%i s%i s%i\n",cnt, out_id3, out_id4);
        fprintf (outfile,"  out_val s%i\n",out_id4);
        fprintf (outfile,"  add s%i 1 s%i\n",out_id3, out_id3);
        fprintf (outfile,"  jump print_array_start_%i\n",out_id2-1);
        fprintf (outfile,"print_array_end_%i:\n",out_id3-1);
      }
      else {
        fprintf (outfile,"  out_char s%i\n",cnt);
      }
       
    }

    // std::cout << "  out_char '\\n'" << std::endl;
    fprintf (outfile,"  out_char '\\n'                         # End print statements with a newline.\n");
  }
};

#endif