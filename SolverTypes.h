/***********************************************************************************[SolverTypes.h]
MiniSat -- Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
           Copyright (c) 2007-2010, Niklas Sorensson

Chanseok Oh's MiniSat Patch Series -- Copyright (c) 2015, Chanseok Oh

Maple_LCM, Based on MapleCOMSPS_DRUP -- Copyright (c) 2017, Mao Luo, Chu-Min LI, Fan Xiao: implementing a learnt clause
minimisation approach Reference: M. Luo, C.-M. Li, F. Xiao, F. Manya, and Z. L. , “An effective learnt clause
minimization approach for cdcl sat solvers,” in IJCAI-2017, 2017, pp. to–appear.

Maple_LCM_Dist, Based on Maple_LCM -- Copyright (c) 2017, Fan Xiao, Chu-Min LI, Mao Luo: using a new branching heuristic
called Distance at the beginning of search


Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**************************************************************************************************/


#ifndef Minisat_SolverTypes_h
#define Minisat_SolverTypes_h

#include <assert.h>

#include "IntTypes.h"

#include <vector>
#include <iostream>

namespace CNFMITER
{

//=================================================================================================
// Variables, literals, lifted booleans, clauses:


// NOTE! Variables are just integers. No abstraction here. They should be chosen from 0..N,
// so that they can be used as array indices.

typedef int Var;
#define var_Undef (-1)


struct Lit {
    int x;

    // Use this as a constructor:
    friend Lit mkLit(Var var, bool sign);

    bool operator==(Lit p) const { return x == p.x; }
    bool operator!=(Lit p) const { return x != p.x; }
    bool operator<(Lit p) const { return x < p.x; } // '<' makes p, ~p adjacent in the ordering.
};

inline Lit mkLit(Var var, bool sign = false)
{
    Lit p;
    p.x = var + var + (int)sign;
    return p;
}
inline Lit operator~(Lit p)
{
    Lit q;
    q.x = p.x ^ 1;
    return q;
}
inline Lit operator^(Lit p, bool b)
{
    Lit q;
    q.x = p.x ^ (unsigned int)b;
    return q;
}
inline bool sign(Lit p) { return p.x & 1; }
inline int var(Lit p) { return p.x >> 1; }

// Mapping Literals to and from compact integers suitable for array indexing:
inline int toInt(Var v) { return v; }
inline int toInt(Lit p) { return p.x; }
inline Lit toLit(int i)
{
    Lit p;
    p.x = i;
    return p;
}

// const Lit lit_Undef = mkLit(var_Undef, false);  // }- Useful special constants.
// const Lit lit_Error = mkLit(var_Undef, true );  // }

const Lit lit_Undef = { -2 }; // }- Useful special constants.
const Lit lit_Error = { -1 }; // }

inline std::ostream &operator<<(std::ostream &out, const Lit &val)
{
    out << (sign(val) ? -var(val)-1 : var(val)+1) << std::flush;
    return out;
}


//=================================================================================================
// Lifted booleans:
//
// NOTE: this implementation is optimized for the case when comparisons between values are mostly
//       between one variable and one constant. Some care had to be taken to make sure that gcc
//       does enough constant propagation to produce sensible code, and this appears to be somewhat
//       fragile unfortunately.

#define l_True (lbool((uint8_t)0)) // gcc does not do constant propagation if these are real constants.
#define l_False (lbool((uint8_t)1))
#define l_Undef (lbool((uint8_t)2))

class lbool
{
    uint8_t value;

    public:
    explicit lbool(uint8_t v) : value(v) {}

    lbool() : value(0) {}
    explicit lbool(bool x) : value(!x) {}

    bool operator==(lbool b) const { return ((b.value & 2) & (value & 2)) | (!(b.value & 2) & (value == b.value)); }
    bool operator!=(lbool b) const { return !(*this == b); }
    lbool operator^(bool b) const { return lbool((uint8_t)(value ^ (uint8_t)b)); }

    lbool operator&&(lbool b) const
    {
        uint8_t sel = (this->value << 1) | (b.value << 3);
        uint8_t v = (0xF7F755F4 >> sel) & 3;
        return lbool(v);
    }

    lbool operator||(lbool b) const
    {
        uint8_t sel = (this->value << 1) | (b.value << 3);
        uint8_t v = (0xFCFCF400 >> sel) & 3;
        return lbool(v);
    }

    friend int toInt(lbool l);
    friend lbool toLbool(int v);
};
inline int toInt(lbool l) { return l.value; }
inline lbool toLbool(int v) { return lbool((uint8_t)v); }

template <class T>
inline std::ostream &operator<<(std::ostream &out, const std::vector<T> &cls)
{
    for (int i = 0; i < cls.size(); ++i) {
        out << cls[i] << " ";
    }

    return out;
}

class Formula {
    Var vars = 0;
  public:
    std::vector< std::vector<Lit> > clauses;

    int nVars() const {return vars;}    // The current number of variables.
    Var newVar() {
        int v = nVars();
        vars ++;
        return v;
    }; // Add a new variable

    void addClause_(std::vector<Lit>& clause) {
        clauses.push_back(clause);
    }
};

//=================================================================================================
} // namespace CNFMITER

#endif
