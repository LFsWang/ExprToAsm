#include<bits/stdc++.h>
#ifdef DBG
#undef DBG 
#endif
using namespace std;
void throwmsg(string s)
{
    #ifdef DBG 
    cerr<<s<<endl;
    #endif
    throw "";
}
enum class COPER
{
    ADD,
    ADD_1,
    SUB,
    SUB_1,
    MUL,
    DIV,
    ASG,
    LP, //(
    RP, //)
};

enum Order:int{
    LR,
    RL,
    MX, //both LR RL
    NO, //not need
};

enum class CTYPE:int
{
    VALUE,  //23
    OPER,   //+
    VAR,    //x
    MIX,    //2+5+x
    ERROR   //25x
};

struct Element{
    union {
        int value;  //if type = VALUE
        COPER opid; //if type = OPER
    };
    CTYPE type;
    string row;
    int resid = -1; //where is stored
};
typedef vector<Element> CEXP;
/*
- -5 ok  => (-,(-,5))
--5  er  => no op --
+-5  ok  => (+,(-,5))
no ++ -- but +- -+ ok
*/

struct Computer{
    map<int,pair<char,int>> RtoP;//紀錄 
    map<pair<char,int>,int> PtoR;
    bool r[8];
    bool m[256/4];
    int tmpreg = 7;
};

using Estack = stack<Element>;
using OPPORT = add_pointer<void(Estack&)>::type;

struct Info{
    const char *ASM;
    int order;
    Order OD;
    OPPORT run;
    string name;
};

#define FNAME(X) F_##X

#define BIN_VALUE_PORT(N,D) \
void FNAME(N) (Estack& e){\
    Element R = e.top();e.pop();\
    Element L = e.top();e.pop();\
    if( R.type != CTYPE::VALUE || L.type != CTYPE::VALUE )\
        throw #N "error";\
    auto f = D<int>();\
    auto res = f(L.value,R.value);\
    e.push({res,CTYPE::VALUE,to_string(res)});\
}

BIN_VALUE_PORT(ADD,plus);
BIN_VALUE_PORT(SUB,minus);
BIN_VALUE_PORT(MUL,multiplies);
BIN_VALUE_PORT(DIV,divides);

void FNAME(ADD_1) (Estack& e){} //Yes! it nothing
void FNAME(SUB_1) (Estack& e){
    if( e.top().type != CTYPE::VALUE )
        throw "ADD_1 error";
    e.top().value = -e.top().value;
    e.top().row = to_string(e.top().value);
} 
map<COPER,Info> OPER_INFO = {
    {COPER::RP,     {"---",0,Order::NO,nullptr     ,")"        }},
    
    {COPER::ADD_1,  {"---",1,Order::RL,FNAME(ADD_1),"+(一元)"  }},
    {COPER::SUB_1,  {"---",1,Order::RL,FNAME(SUB_1),"-(一元)"  }},
    
    {COPER::MUL,    {"MUL",2,Order::LR,FNAME(MUL)  ,"*"        }},
    {COPER::DIV,    {"DIV",2,Order::LR,FNAME(DIV)  ,"/"        }},
    
    {COPER::ADD,    {"ADD",3,Order::LR,FNAME(ADD)  ,"+"        }},
    {COPER::SUB,    {"SUB",3,Order::LR,FNAME(SUB)  ,"-"        }},
    
    {COPER::ASG,    {"---",4,Order::RL,nullptr     ,"="        }},
    
    {COPER::LP,     {"---",9,Order::NO,nullptr     ,"("        }},
    
};

inline bool isvar(char c)
{
    return c=='x'||c=='y'||c=='z';
}
inline bool isvar(string c)
{
    return c.size()==1&&isvar(c[0]);
}
inline bool isCalcOper(char c)
{
    //+-*/=
    return c=='+'||c=='-'||c=='*'||c=='/'||c=='=';
}

void echo(const CEXP &s,char end = ' ')
{
    #ifdef DBG
    for(auto a:s)
    {
        if( a.type == CTYPE::OPER )
            cerr<<OPER_INFO[a.opid].name<<end;
        else
            cerr<<a.row<<end;
    }
    cerr<<endl;
    #endif
}

//Shunting-yard algorithm
CEXP spilt(string s)
{
    CEXP out;
    
    //檢查有沒有空白數字裡 ex 5  59 
    bool lIsNum = false;
    bool lIsBlank = false;
    for(char c:s)
    {
        if( c==' ' ){
            lIsBlank = true;
            continue;
        }
        if( lIsBlank && lIsNum && isdigit(c) )
        {
            throw "數字間有空白";
        }
        lIsNum = isdigit(c);
        lIsBlank = false;
    }
    //不能有++ -- ，但是+ + ,- -是合法的
    if( s.find("++") != string::npos )throw "++ not available";
    if( s.find("--") != string::npos )throw "-- not available";
    //去掉空白 
    string tmp;
    for(char c:s)
        if(!isblank(c))
            tmp.push_back(c);
    s.swap(tmp);
    
    int len = static_cast<int>(s.size());
    for(int i=0;i<len;++i)
    {
        char c=s[i];
        if( isdigit(c)|| isvar(c) )
        {
            if( out.empty() || out.back().type != CTYPE::MIX)
                out.push_back({0,CTYPE::MIX,""});
            out.back().row.push_back(c);
            continue;
        }
        if( c=='(' )
        {
            out.push_back({static_cast<int>(COPER::LP),CTYPE::OPER,"("});
            continue;
        }
        if( c==')' )
        {
            out.push_back({static_cast<int>(COPER::RP),CTYPE::OPER,")"});
            continue;
        }
        
        /*
            By StackOverflow
            If an operator is the first thing in your expression, 
            or comes after another operator, or comes after a left parenthesis, (+
            then it's an unary operator.
        */
        if( i==0 /*isFirstChar*/ ||
            s[i-1] =='(' ||
            isCalcOper(s[i-1]) )
        {
            switch(c){
            case '+':
                out.push_back({static_cast<int>(COPER::ADD_1),CTYPE::OPER,"+"});
                break;
            case '-':
                out.push_back({static_cast<int>(COPER::SUB_1),CTYPE::OPER,"-"});
                break;
            default :throwmsg("未知的 unary oper"+string(1,c));
            }
        }
        else
        {
            switch(c){
            case'+':
                out.push_back({static_cast<int>(COPER::ADD),CTYPE::OPER,"+"});
                break;
            case'-':
                out.push_back({static_cast<int>(COPER::SUB),CTYPE::OPER,"-"});
                break;
            case'*':
                out.push_back({static_cast<int>(COPER::MUL),CTYPE::OPER,"*"});
                break;
            case'/':
                out.push_back({static_cast<int>(COPER::DIV),CTYPE::OPER,"/"});
                break;
            case'=':
                out.push_back({static_cast<int>(COPER::ASG),CTYPE::OPER,"-"});
                break;
            default :throwmsg("未知的符號"+string(1,c));
            }
        }
    }
    
    //determin CTYPE::NUM or CTYPE::VAR
    for(auto &e:out)
    {
        auto isnum = [](char c)->bool{
            return isdigit(c);
        };
        if( e.type == CTYPE::MIX )
        {
            if( isvar(e.row) )e.type=CTYPE::VAR;
            else if( all_of(e.row.begin(),e.row.end(),isnum) )
            {
                e.value = stoi(e.row);
                e.type=CTYPE::VALUE;
            }
            else throwmsg("未知的符號"+e.row);
        }
    }
    return out;
}

//Shunting-yard algorithm
CEXP build(const CEXP &s)
{
    stack<Element> st;
    CEXP out;
    for(const auto &e:s)
    {
        if( e.type == CTYPE::VAR ||
            e.type == CTYPE::VALUE )
        {
            out.push_back(e);
            continue;
        }
        if( e.type == CTYPE::OPER )
        {
            if( e.opid == COPER::LP )
            {
                st.push(e);
                continue;
            }
            if( e.opid == COPER::RP )
            {
                while(true)
                {
                    if( st.empty() )throw "expr err : miss (";
                    if( st.top().opid == COPER::LP )break;
                    out.push_back(st.top());
                    st.pop();
                }
                st.pop();
                continue;
            }
            //normal expr
            /*
            5*6+5 56*5+
            * 1
            + 4 stack =>遞減 
            */
            auto LRC = [](const Element &a,const Element &b)->bool{
                return OPER_INFO[a.opid].order<=OPER_INFO[b.opid].order;
            };
            auto RLC = [](const Element &a,const Element &b)->bool{
                return OPER_INFO[a.opid].order< OPER_INFO[b.opid].order;
            };
            auto CMP = (OPER_INFO[e.opid].OD == Order::RL) ? RLC : LRC;
            while( !st.empty() && CMP(st.top(),e) )
            {
                out.push_back(st.top());
                st.pop();
            }
            st.push(e);  
        }
        else
        {
            throwmsg("Build error 未知的型態:row:"+e.row);
        }
    }
    while( !st.empty() )
    {
        out.push_back(st.top());
        st.pop();
    }
    return out;
}

//文法檢查 
bool REG(const CEXP &s)
{
    stack<CTYPE> st;
    for(const auto &e:s)
    {
        if( e.type == CTYPE::VALUE ||
            e.type == CTYPE::VAR ){
            st.push(e.type);
            continue;
        }
        if( e.type == CTYPE::OPER )
        {
            CTYPE v[2];
            if( e.opid == COPER::ADD_1 || e.opid == COPER::SUB_1 ){
                if(st.size()<1)return false;
                v[0] = st.top();st.pop();
                st.push(CTYPE::VALUE);
            }
            else{
                if(st.size()<2)return false;
                v[1] = st.top();st.pop();//R
                v[0] = st.top();st.pop();//L
                if( e.opid == COPER::ASG && v[0] != CTYPE::VAR )
                    return false;
                st.push(CTYPE::VALUE);
            }
        }
        else
            return false;
    }
    return st.size() == 1;
}
//壓縮
CEXP compress(const CEXP &s) 
{
    //刪除1元+ => 根本沒用
    //連續的1元- => 看有幾個
    CEXP S1,ST;
    
    bool lastIsNeg = false;
    int num=0;
    for(const auto &e:s)
    {
        if( e.type==CTYPE::OPER && e.opid==COPER::ADD_1 ){continue;}
        if( e.type==CTYPE::OPER && e.opid==COPER::SUB_1 )
        {
            num++;
            lastIsNeg = true;
            continue;
        }
        if( lastIsNeg )
        {
            if(num%2==1)
                S1.push_back({static_cast<int>(COPER::SUB_1),CTYPE::OPER,"-"}); 
        }
        S1.push_back(e); 
        lastIsNeg = false;
        num=0;
    }
     
    // ?1+1+x+1+ => ?2+x+1
    // (INF-2)+2+(-99)+1 ok
    // (INF-2)+3+(-99)   BOMB 不可換變數位置 
    // 同符號 => 可壓縮  (=不可) 
    // +-*先壓縮
    //在壓 -+混合 
    // */ 如果整除才可繼續 
    auto Z1 = {COPER::ADD,COPER::SUB,COPER::MUL};
    for(auto op:Z1)
    {
        int sz=static_cast<int>(S1.size());
        for(int i=0;i<sz;++i)
        {
            //最後一個 
            if( i == sz-1 )
            {
                ST.push_back(S1[i]);
                continue;
            }
            // 是 數字 符號 數字 符號
            int sts = static_cast<int>(ST.size());
            if( S1[i].type  == CTYPE::VALUE &&
                S1[i+1].type== CTYPE::OPER && S1[i+1].opid == op &&
                sts >= 2 && 
                ST[sts-2].type  == CTYPE::VALUE &&
                ST[sts-1].type  == CTYPE::OPER && ST[sts-1].opid == op )
            {
                //2-3- =>5-
                //2+3+ =>5+
                //2*3* =>6*
                if( op == COPER::MUL ){
                    ST[sts-2].value *= S1[i].value;  
                }else{
                    ST[sts-2].value += S1[i].value;
                }
                ST[sts-2].row = to_string(ST[sts-2].value);
                ++i;
                continue;
            }
            ST.push_back(S1[i]);
        }
        S1.swap(ST);
        ST.clear();
    }
    //?1-3+
    //?2+ or ?(-2)- 同號加 異號減 
    {
        int sz=static_cast<int>(S1.size());
        for(int i=0;i<sz;++i)
        {
            //最後一個 
            if( i == sz-1 )
            {
                ST.push_back(S1[i]);
                continue;
            }
            // 是 數字 符號 數字 符號
            int sts = static_cast<int>(ST.size());
            if( S1[i].type  == CTYPE::VALUE &&
                S1[i+1].type== CTYPE::OPER && ( S1[i+1].opid == COPER::ADD||
                                                S1[i+1].opid == COPER::SUB)   &&
                sts >= 2 && 
                ST[sts-2].type  == CTYPE::VALUE &&
                ST[sts-1].type  == CTYPE::OPER &&( ST[sts-1].opid == COPER::ADD||
                                                   ST[sts-1].opid == COPER::SUB)  )
            {
                //2-3- =>5-
                //2+3+ =>5+
                //2*3* =>6*
                if( S1[i+1].opid == ST[sts-1].opid ){
                    ST[sts-2].value += S1[i].value;  
                }else{
                    ST[sts-2].value -= S1[i].value;
                }
                if( ST[sts-2].value < 0 ){
                    ST[sts-2].value *= -1;
                    S1[i+1].opid = (S1[i+1].opid==COPER::ADD)?COPER::SUB:COPER::ADD;
                }
                ST[sts-2].row = to_string(ST[sts-2].value);
                ++i;
                continue;
            }
            ST.push_back(S1[i]);
        }
        S1.swap(ST);
        ST.clear();
    }
    return S1;
}

//把可以算的算起來 
CEXP zip(const CEXP &s)
{
    CEXP nw;
    for(const auto e:s)
    {
        if( e.type == CTYPE::OPER && e.opid != COPER::ASG )
        {
            if( OPER_INFO[e.opid].run == nullptr ){
                cerr<< "not imp op: "<<OPER_INFO[e.opid].name <<endl;
                nw.push_back(e);
                continue;
            }
                
            int req = 2;
            if(e.opid==COPER::ADD_1||e.opid==COPER::SUB_1)
                req = 1;
            
            int size = static_cast<int>(nw.size());
            if(size < req)throw "zip sim fail";
            
            bool flag = true;
            for(int i= size-req ; i<size ; ++i)
                if( nw[i].type != CTYPE::VALUE )
                {
                    flag = false;
                    break;
                }
            if(!flag){
                nw.push_back(e);
                continue;
            }
            
            Estack tmp;
            
            for(int i= size-req ; i<size ; ++i)
                tmp.push(nw[i]);
            nw.erase(nw.end()-req,nw.end());
            
            OPER_INFO[e.opid].run(tmp);
            nw.push_back( tmp.top() );
        }
        else
        {
            nw.push_back(e);
        }
        //echo(nw);
    }
    return nw;
}
/*

 x= 6;
 where is x?
 r0 default
*/
Computer Comp;

int findLastRegID(){
    static int i=7;
    i=(i+1)%8;
    return i;
}

Element VAR[3]={
    {0,CTYPE::VALUE,"x"},
    {0,CTYPE::VALUE,"y"},
    {0,CTYPE::VALUE,"z"},
};
void info()
{
    #ifdef DBG
    for(int i=0;i<3;++i)
    {
        cerr<<(char)('x'+i)<<" ";
        if( VAR[i].resid==-1 ){
            cerr<<"未被使用";
        }else{
            int res = VAR[i].resid;
            cerr<<"位置 "<<Comp.RtoP[res].first<<' '<<Comp.RtoP[res].second
                <<" 數值:"<< VAR[i].value;
        }
        cerr<<endl;
    }
    #endif
}

string POSSTR(pair<char,int> p)
{
    if( p.first=='r' )return "r"+to_string(p.second);
    return "["+to_string(p.second)+"]";
}
int getIndex(const Element &e)
{
    return e.row[0]-'x';
}


bool hotReg[8]={false};
int UCOINTER[8];
int useless()
{
    int mid = -1;
    for(int i=0;i<8;++i)
    {
        if( hotReg[i] ){
            continue;
        }
        if( mid==-1 || UCOINTER[i]<UCOINTER[mid] )
            mid = i;
    }
    if( mid == -1 )throw "HOT REG FULL!";
    return mid;
}

int regmem()
{
    for(int i=0;i<256/4;i++)
        if( !Comp.m[i] )
        {
            Comp.m[i] = true;
            return i*4;
        }
    throw "BOMB!";
}

void unreg(int resid,bool force = false)
{
    if(resid==-1)return;
    if(!force)
    {
        for(int i=0;i<3;++i)//dont remove xyz
            if(resid == VAR[i].resid)
                return;
    }
    if( Comp.RtoP[resid].first=='r' )
        Comp.r[Comp.RtoP[resid].second] = false;
    else
        Comp.m[Comp.RtoP[resid].second] = false;
    Comp.PtoR.erase(Comp.RtoP[resid]);
    Comp.RtoP.erase(resid);
}

int reg(int RPOS)
{
    static int uid = 0;
    ++uid;
    if( Comp.r[RPOS] ){
        cerr<<RPOS<<" Locked: resid"<<Comp.PtoR[{'r',RPOS}]<<endl; 
        throw "Locked!";
    }
    Comp.r[RPOS] = true;
    Comp.PtoR[{'r',RPOS}]=uid;
    Comp.RtoP[uid]={'r',RPOS};
    hotReg[uid] = true;
    return uid;
}

int load(Element &e,bool rol = false,int sug = -1)
{
    int flagIsVar = -1;
    if( e.type == CTYPE::VAR )
    {
        e = VAR[getIndex(e)];
        flagIsVar = getIndex(e);
        sug = flagIsVar;
    }
    if( e.resid != -1 && Comp.RtoP[e.resid].first == 'r' )
    {
        hotReg[Comp.RtoP[e.resid].second] = true;
        return Comp.RtoP[e.resid].second;
    }
    //找一個放的位置 
    int pos = -1;
    if( !Comp.r[sug] )
        pos = sug;
    for(int i=0;i<8&&pos==-1;++i)
        if(!Comp.r[i])
        {
            pos = i;
            break;
        }

    if( pos==-1 ){
        //找不到的話 搬一個到記憶體
        int uls = useless();
        int mem = regmem();
        
        //更新紀錄 
        //???
        int rm = Comp.PtoR[{'r',uls}];
        Comp.RtoP[rm] = {'m',mem};
        Comp.PtoR[{'m',mem}] = rm;
        Comp.PtoR.erase({'r',uls});
        Comp.r[uls] = false;
        printf("MOV [%d] r%d\n",mem,uls);
        pos = uls;
    }
    
    
    if( e.resid == -1 )
    {
        if(rol)
            printf("MOV r%d %s\n",pos,e.row.c_str());
        else
            printf("MOV r%d %d\n",pos,e.value);
        if( flagIsVar!=-1 )
        {
            e.resid = VAR[flagIsVar].resid = reg(pos);
        }
    }
    else
    {
        assert(Comp.RtoP[e.resid].first=='m');
        printf("MOV r%d [%d]\n",pos,Comp.RtoP[e.resid].second);
        Comp.m[Comp.RtoP[e.resid].second] = false;
        Comp.PtoR.erase(Comp.RtoP[e.resid]);
        Comp.RtoP[e.resid] = {'r',pos};
        Comp.PtoR[{'r',pos}] = e.resid;
        Comp.r[pos]=true;
        hotReg[pos]=true;
    }
    
    return pos;
}

void toASM(const CEXP &s)
{
    #ifdef DBG
    cerr<<"TO ASM"<<endl;
    #endif
    Element L,R;
    Estack E;
    int LP,RP;
    int _sz = static_cast<int>(s.size());
    for(int i=0;i<_sz;++i)
    {
        const auto &e = s[i];
        if( e.type != CTYPE::OPER ){
            assert(e.resid == -1);
            E.push(e);
            continue;
        }
        else
        {
            memset(hotReg,false,sizeof(hotReg));
            if( e.opid == COPER::SUB_1 )
            {
                L = E.top();E.pop();
                LP = load(L);
                if(L.resid==-1)
                    L.resid=reg(LP);
                Element _t{0,CTYPE::VALUE,"0"};
                int tmp = load(_t);
                printf("SUB r%d r%d\n",tmp,LP);
                unreg(L.resid);
                L.resid = reg(tmp);
                
                L.value = -L.value;
                L.row = to_string(L.value);
                E.push(L);
                //throw "not imp";
            }
            else if( e.opid == COPER::ASG )
            {
                R = E.top();E.pop();
                L = E.top();E.pop();
                int v = getIndex(L);
                bool RisVar = false;
                if( R.type == CTYPE::VAR ){
                    RP = load(R,false,getIndex(R));//any way
                    RisVar = true;
                }
                
                if( R.resid == -1 || //Nice, just cover it
                    i==_sz-1 && !RisVar ) //last one is ok
                {//cout<<"D"<<endl;
                    if( VAR[v].resid!=-1 )
                    {
                        int old = VAR[v].resid;
                        VAR[v].resid = -1;
                        unreg(old);
                    }
                    //cout<<"V="<<v<<endl;
                    RP = load(R,false,v);
                    //cout<<"RP="<<RP<<endl;
                    unreg(R.resid);
                    VAR[v].resid = reg(RP);
                }
                else
                {
                    RP = load(R);
                    if( VAR[v].resid == -1 )
                    {
                        Element _t={0,CTYPE::VALUE,POSSTR(Comp.RtoP[R.resid])};
                        LP = load(_t,true,v);
                        VAR[v].resid = reg(LP);
                    }
                    else
                    {
                        LP = load(L,false,v);
                        if(LP!=RP)
                            printf("MOV r%d r%d\n",LP,RP);
                    }
                    
                }
                VAR[v].value = R.value;
                E.push(R);
            }
            else
            {
                R = E.top();E.pop();
                L = E.top();E.pop();
                
                //if L is VAR, we should copy it!
                if( L.type == CTYPE::VAR )
                {
                    int v = getIndex(L);
                    //把var載入回reg 
                    if(VAR[v].resid == -1){//不存在，先生來用 
                        auto rel = L; //hack skill
                        LP = load(L);
                        L = rel;
                        assert(VAR[v].resid!=-1);
                    }
                    Element _t{0,CTYPE::VALUE,POSSTR(Comp.RtoP[VAR[v].resid])};
                    LP = load(_t,true);
                    L = {VAR[v].value,CTYPE::VALUE,to_string(VAR[v].value)};
                }
                else
                {
                    //if(Comp.r[1])cout<<"R1locked";
                    LP = load(L);
                    //cout<<"L"<<LP<<endl;
                }
                
                if( L.resid==-1 ) //L need to lock it
                    L.resid = reg(LP);

                RP = load(R);
                //cout<<"B"<<RP<<endl;
                printf("%s r%d r%d\n",OPER_INFO[e.opid].ASM,LP,RP);
                UCOINTER[LP]++;UCOINTER[RP]++;
                Estack tE;
                int tLres = L.resid;
                tE.push(L);tE.push(R);
                OPER_INFO[e.opid].run(tE);
                L = tE.top();
                
                L.resid = tLres;
                assert(L.resid!=-1);
                if(R.resid!=-1)      //R nerver used.
                    unreg(R.resid); //unreg will protect for xyz
                E.push(L);
            }
        }
    }
    assert(E.size()==1);
    unreg(E.top().resid);
}

void HOLD()
{
    memset(hotReg,false,sizeof(hotReg));
    array<int,3> p = {-1,-1,-1};
    int errc = 0;
    vector<int> deal,err;
    for(int i=0;i<3;++i)
    {
        if( VAR[i].resid == -1 )continue;
        p[i] = load(VAR[i]);
        if( VAR[i].resid == -1 )VAR[i].resid = reg(p[i]);
        if( i!=p[i] ){
            errc++;
            deal.push_back(i);
        }
    }
    if( errc == 0 ){}
    if( errc == 1 ){
        printf("MOV r%d r%d\n",deal[0],p[deal[0]]);
        unreg(VAR[deal[0]].resid,true);
        VAR[deal[0]].resid = reg(deal[0]);
    }
    else{
        for(int _p=0;_p<9;++_p)
        {
            err.clear();
            for(int i:deal)
            {
                if( find(p.begin(),p.end(),i) == p.end() )
                {
                    printf("MOV r%d r%d\n",i,p[i]);
                    unreg(VAR[i].resid,true);
                    VAR[i].resid = reg(i);
                    p[i]=i;
                }
                else err.push_back(i);
            }
            deal.swap(err);
        }

        if( err.size() == 2 ){
            //swap
            if( VAR[err[0]].value != VAR[err[1]].value )
            {
                printf("MOV r3 r%d\n",err[0]);
                printf("MOV r%d r%d\n",err[0],err[1]);
                printf("MOV r%d r3\n",err[1]);
            }
            swap(VAR[err[0]].resid,VAR[err[1]].resid);
        }else if(err.size()==3){
            //0 1 2
            //2 0 1 =>A
            //1 2 0 =>B
            if( p[0]==1 )//A
            {
                puts("MOV r3 r0");
                puts("MOV r0 r1");
                puts("MOV r1 r2");
                puts("MOV r2 r3");
            }
            else
            {
                puts("MOV r3 r2");
                puts("MOV r2 r1");
                puts("MOV r1 r0");
                puts("MOV r0 r3");
            }
            for(int i=0;i<3;++i)unreg(VAR[i].resid,true);
            for(int i=0;i<3;++i)VAR[i].resid = reg(i);
        }else{
            assert(err.size()==0);
        }
    }
//    for(int i=0;i<3;++i)
//    {
//        if( VAR[i].resid == -1 )
//        {
//            printf("MOV r%d 0\n",i);
//            VAR[i].resid=reg(i);
//        }
//    }
}
int _DV[3]={0,0,0,};
void SIM(const CEXP &s,int *V = _DV)
{
    stack<pair<bool,int>> st;
    for(const auto &t:s)
    {
        if(t.type==CTYPE::VALUE)
        {
            st.push({false,t.value});
            continue;
        }
        if(t.type==CTYPE::VAR)
        {
            st.push({true,getIndex(t)});
            continue;
        }
        else if(t.type == CTYPE::OPER)
        {
            if( t.opid == COPER::ADD_1 ){}
            else if( t.opid == COPER::SUB_1 ){
                auto L = st.top();st.pop();
                if(L.first)L = {false,V[L.second]};
                L.second = -L.second;
                st.push(L);
            }
            else if( t.opid == COPER::ASG )
            {
                auto R = st.top();st.pop();
                auto L = st.top();st.pop();
                if(R.first)R = {false,V[R.second]};
                V[L.second] = R.second;
                st.push(R);
            }
            else
            {
                auto R = st.top();st.pop();
                auto L = st.top();st.pop();
                if(L.first)L={false,V[L.second]};
                if(R.first)R={false,V[R.second]};
                switch( t.opid )
                {
                    case COPER::ADD: L.second+=R.second;break;
                    case COPER::SUB: L.second-=R.second;break;
                    case COPER::MUL: L.second*=R.second;break;
                    case COPER::DIV: {
                                        if(R.second==0)throw"DIV 0";
                                        L.second/=R.second;
                                    }break;
                    default: throw "error";
                }
                //cerr<<":"<<L.second<<endl;
                st.push(L);
            }
        }
        else throw "error";
    }
    #ifdef DBG 
    cerr<<"模擬\n";
    for(int i=0;i<3;++i)
    {
        cerr<<(char)('x'+i)<<"="<<V[i]<<" \t";
    }
    cerr<<"\n====\n";
    #endif
}

int main(int argc,char *argv[])
{
    string expr,pre;
    Comp.tmpreg = 7;
    memset(Comp.m,false,sizeof(Comp.m));
    memset(Comp.r,false,sizeof(Comp.r));
    try{
        while( getline(cin,expr) )
        {
            if(expr=="gg")break;
            CEXP cexp = spilt(expr); 
            //echo(cexp,' ');
            
            CEXP pre = build(cexp);
            echo(pre,' ');
            if( !REG(pre) )
                throw "expr err";
                
            if( expr.find("=") ==string::npos )
                continue;
                
            SIM(pre);
            
            CEXP zp1= zip(pre);
            if( !REG(pre) )
                throw "expr err";
            
            CEXP cp = compress(zp1);
            if( !REG(cp) )
                throw "expr err";
            echo(cp);
            if( argc==2 && argv[1][0]=='r' )
                toASM(pre);
            else
                toASM(cp);
            info();
        }
        HOLD();
        info();
        cout<<"EXIT 0"<<endl;
    }catch(const std::exception& e){
        #ifdef DBG
        cerr<<e.what()<<endl;
        #endif
        cout<<"EXIT 1"<<endl;
    }catch(const char *str){
        #ifdef DBG
        cerr<<str<<endl;
        #endif
        cout<<"EXIT 1"<<endl;
    }catch(...){
        #ifdef DBG
        cerr<<"Compile error"<<endl;
        #endif
        cout<<"EXIT 1"<<endl;
    }
}
//x=x=x=x=x=x=x=x=x=x=5 => x=5
/*
x=x*2*3/6 = x*1
x=x*1 = x
*/
