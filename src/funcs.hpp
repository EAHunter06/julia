#ifndef FUNCS_HPP
#define FUNCS_HPP

#include <SFML/Graphics.hpp>
#include <iostream>
#include <mpc.h>
#include <string.h>
#include <string>
#include <stack>
#include <list>

#define R(x) mpc_realref(x)
#define I(x) mpc_imagref(x)

enum _type:uint8_t{OP,NUM,ANS};
enum _op:uint8_t{UD=0,ADD,SUB,DIV,MUL,EXP,SIN,COS,TAN,ASIN,ACOS,ATAN};

struct node{
    _type type;
    union{
        _op op;
        mpc_t val;
    };

    node(_op o):type(OP),op(o){}
    node(double r,double i=0):type(NUM){
        mpc_init2(val,64);
        mpc_set_d_d(val,r,i,MPC_RNDNN);
    }
    node():type(ANS){}
    node(mpc_t& z):type(NUM){
        mpc_init2(val,mpc_get_prec(z));
        mpc_set(val,z,MPC_RNDZZ);
    }
    ~node(){
        if(type==NUM) mpc_clear(val);
    }
    node(const node& n):type(n.type){
        if(type==NUM){
            mpc_init2(val,64);
            mpc_set(val,n.val,MPC_RNDZZ);
        }
        else if(type==OP) op=n.op;
    }
};

int getPriority(char C){
    if (C == '-' || C == '+')
        return 1;
    else if (C == '*' || C == '/')
        return 2;
    else if(C == '^' || (C>='a'&&C<='f'))
        return 3;
    else return 0;
}

std::string postfix(std::string infix){
    infix = '(' + infix + ')';
    bool number=0;
    std::stack<char> char_stack;
    std::string output;

    for (auto&& i:infix) {

        // If the scanned character is an
        // operand, add it to output.
        if (i=='i' || i=='z' || isdigit(i) || i=='.'){
            output += i;
            number=1;
        }

        else if(number){
            output += '|';
            number=0;
        }

        // If the scanned character is an
        // ‘(‘, push it to the stack.
        if (i == '(')
            char_stack.push('(');

        // If the scanned character is an
        // ‘)’, pop and output from the stack
        // until an ‘(‘ is encountered.
        else if (i == ')') {
            while (char_stack.top() != '(') {
                output += char_stack.top();
                char_stack.pop();
            }

            // Remove '(' from the stack
            char_stack.pop();
        }

        // Operator found
        else if (char_stack.top()!='i' && char_stack.top()!='z' && !isdigit(char_stack.top()) && !isdigit(i) && i!='i' && i!='z' && i!='.')
            {
                //std::cout<<i<<std::endl;
                while (getPriority(i) < getPriority(char_stack.top()))
                   {
                     output += char_stack.top();
                     char_stack.pop();
                   }

                // Push current Operator on stack
                char_stack.push(i);

        }
    }
      while(!char_stack.empty()){
          output += char_stack.top();
        char_stack.pop();
    }
    return output;
}

std::string preproc(std::string str){
    std::string ret;

    for(size_t i=0;i<str.length();i++){
        if(str[i]=='s'||str[i]=='c'||str[i]=='t'||str[i]=='a'){
            if(str.substr(i,3)=="sin"){
                ret+='a';
                i+=2;
            }else if(str.substr(i,3)=="cos"){
                ret+='b';
                i+=2;
            }else if(str.substr(i,3)=="tan"){
                ret+='c';
                i+=2;
            }else if(str.substr(i,4)=="asin"){
                ret+='d';
                i+=3;
            }else if(str.substr(i,4)=="acos"){
                ret+='e';
                i+=3;
            }else if(str.substr(i,4)=="atan"){
                ret+='f';
                i+=3;
            }else throw 3;
        }else ret+=str[i];
    }

    return ret;
}

void tokenize(std::string str,std::stack<node>& r){
    std::cout<<str<<std::endl; ///PRINT
    std::stack<node> ret;
    double num=0,mul=1;
    bool im=0;
    for(size_t i=0;i<str.length();i++){
        if(str[i]=='z'){
            ret.push(node());
            i++;
        }else if(isdigit(str[i])||str[i]=='.'||str[i]=='i'){
            for(;str[i]!='|';i++){
                if(str[i]=='.') mul/=10;
                else if(str[i]=='i') im=1;
                else if(mul==1){
                    num*=10;
                    num+=str[i]-'0';
                }else{
                    num+=(str[i]-'0')*mul;
                    mul/=10;
                }
            }
            if(im) ret.push(node(0,num));
            else ret.push(node(num));
            num=0;
            mul=1;
            im=0;
        }else{
            _op op=
            str[i]=='+'?ADD:
            str[i]=='-'?SUB:
            str[i]=='*'?MUL:
            str[i]=='/'?DIV:
            str[i]=='^'?EXP:
            str[i]=='a'?SIN:
            str[i]=='b'?COS:
            str[i]=='c'?TAN:
            str[i]=='d'?ASIN:
            str[i]=='e'?ACOS:
            str[i]=='f'?ATAN:
            UD;

            if(!op){
                std::cout<<str[i]<<std::endl;
                throw 5;
            }
            ret.push(node(op));
        }
    }

    while(!ret.empty()){
        r.push(ret.top());
        ret.pop();
    }
}

void eval(std::stack<node> tokens,mpc_t& res){
    mpc_t ans,l,r,buf;
    mpc_init2(l,mpc_get_prec(res));
    mpc_init2(r,mpc_get_prec(res));
    mpc_init2(ans,mpc_get_prec(res));
    mpc_set(ans,res,MPC_RNDZZ);

    _op op;

    std::stack<node> cur;

    while(!tokens.empty()){
        cur.push(tokens.top());

        if(cur.top().type==OP){
            op=cur.top().op;
            cur.pop();

            mpc_set(r,cur.top().type==NUM?cur.top().val:ans,MPC_RNDZZ);
            cur.pop();

            if(op<SIN){
                mpc_set(l,cur.top().type==NUM?cur.top().val:ans,MPC_RNDZZ);
                cur.pop();
            }

            switch(op){
            case ADD:
                mpc_add(l,l,r,MPC_RNDZZ);
                break;
            case SUB:
                mpc_sub(l,l,r,MPC_RNDZZ);
                break;
            case MUL:
                mpc_mul(l,l,r,MPC_RNDZZ);
                break;
            case DIV:
                mpc_div(l,l,r,MPC_RNDZZ);
                break;
            case EXP:
                mpc_pow(l,l,r,MPC_RNDZZ);
                break;
            case SIN:
                mpc_sin(l,r,MPC_RNDZZ);
                break;
            case COS:
                mpc_cos(l,r,MPC_RNDZZ);
                break;
            case TAN:
                mpc_tan(l,r,MPC_RNDZZ);
                break;
            case ASIN:
                mpc_asin(l,r,MPC_RNDZZ);
                break;
            case ACOS:
                mpc_cos(l,r,MPC_RNDZZ);
                break;
            case ATAN:
                mpc_atan(l,r,MPC_RNDZZ);
                break;
            }

            cur.push(node(l));
        }

        tokens.pop();
    }
    mpc_set(res,cur.top().val,MPC_RNDZZ);

    mpc_clear(ans);
    mpc_clear(l);
    mpc_clear(r);
}

#endif // FUNCS_HPP
