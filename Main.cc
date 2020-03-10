#include "Dimacs.h"

#include <zlib.h>

#include <iostream>
#include <sstream>
#include <string>

using namespace CNFMITER;

/// add clauses to f, which encode: (clause <-> enabler_lit)
void generate_or_equivalence(Formula &f, const std::vector<Lit> &clause, Lit enabler_lit)
{
    static std::vector<Lit> tmpClause;
    tmpClause.clear();

    // !enabler_lit -> clause
    tmpClause = clause;
    tmpClause.push_back(~enabler_lit);
    f.addClause_(tmpClause);

    // clause -> enabler_lit
    tmpClause.clear();
    tmpClause.push_back(enabler_lit);
    tmpClause.push_back(enabler_lit);
    for (Lit l : clause) {
        tmpClause[1] = ~l;
        f.addClause_(tmpClause);
    }
}

/// add formula (l <-> input), return
void generate_clause_sat(Formula &formula, const Formula &input, Lit &equivalence_lit)
{
    std::vector<Lit> enabler_lits; // literals that are equal to satisfiability of each clause

    for (const auto &c : input.clauses) {
        Lit enabler_lit = mkLit(formula.newVar());
        enabler_lits.push_back(enabler_lit);

        generate_or_equivalence(formula, c, enabler_lit);
    }

    equivalence_lit = mkLit(formula.newVar());

    // (x <-> (a or b)) is the same as (!x <->(!b and !c))
    for (int i = 0; i < enabler_lits.size(); ++i) enabler_lits[i] = ~enabler_lits[i];
    generate_or_equivalence(formula, enabler_lits, ~equivalence_lit);
}

void generate_formula_miter(Formula &formula, const Formula &input1, const Formula &input2)
{
    Lit e1, e2;
    generate_clause_sat(formula, input1, e1);
    std::cerr << "c after 1st equivalence formula, miter has " << formula.nVars() << " variables" << std::endl;
    generate_clause_sat(formula, input2, e2);
    std::cerr << "c after 2nd equivalence formula, miter has " << formula.nVars() << " variables" << std::endl;

    std::vector<Lit> clause;

    // !e1 -> e2
    clause.push_back(e1);
    clause.push_back(e2);
    formula.addClause_(clause);

    // e1 -> !e2
    clause[0] = ~e1;
    clause[1] = ~e2;
    formula.addClause_(clause);
    std::cerr << "c miter has " << formula.nVars() << " variables and " << formula.clauses.size() << " clauses" << std::endl;
}

void print_formula(Formula &f, std::string s)
{
    std::cout << "c CNFmiter, Norbert Manthey, 2020" << std::endl;
    if (!s.empty()) std::cout << "c " << s << std::endl;
    std::cout << "c " << std::endl;
    std::cout << "p cnf " << f.nVars() << " " << f.clauses.size() << std::endl;
    for (const auto &c : f.clauses) {
        std::stringstream s;
        s << c;
        printf("%s 0\n", s.str().c_str());
    }
}

int main(int argc, char **argv)
{
    std::cerr << "c CNFmiter generates a CNF formula " << std::endl
              << "c which is unsatisfiable, if the given 2 formulas are equivalent" << std::endl;

    if (argc < 2) {
        std::cerr << "not enough parameters, abort!" << std::endl;
        return 1;
    }

    gzFile in1 = gzopen(argv[1], "rb");
    gzFile in2 = gzopen(argv[2], "rb");

    if (!in1) {
        std::cerr << "failed to open first file, abort!" << std::endl;
        return 1;
    }
    if (!in2) {
        std::cerr << "failed to open second file, abort!" << std::endl;
        return 1;
    }

    Formula f1, f2;

    parse_DIMACS(in1, f1);
    gzclose(in1);

    parse_DIMACS(in2, f2);
    gzclose(in2);

    std::cerr << "c Parsed formulas 1 with " << f1.nVars() << " vars and " << f1.clauses.size()
              << " and formulas 2 with " << f2.nVars() << " vars and " << f2.clauses.size() << std::endl;

    Formula miter;

    Var maxV = f1.nVars() > f2.nVars() ? f1.nVars() : f2.nVars();
    while (miter.nVars() < maxV) miter.newVar();

    std::cerr << "c Miter base formulas reserved " << miter.nVars() << " variables" << std::endl;

    generate_formula_miter(miter, f1, f2);

    std::string fn1 = (argv[1]);
    std::string fn2 = (argv[2]);
    std::size_t found = fn1.rfind("/");
    if (found != std::string::npos) fn1 = fn1.erase(0, found + 1);
    found = fn2.rfind("/");
    if (found != std::string::npos) fn2 = fn2.erase(0, found + 1);
    print_formula(miter, fn1 + " and " + fn2);

    return 0;
}