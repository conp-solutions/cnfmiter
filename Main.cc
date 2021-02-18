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

/// add offset to all variables in formula, that are greater than largest_input_variable
void rewrite_variable_range(Formula &formula, Var largest_input_variable, int offset)
{
    if (offset == 0) return;
    if (largest_input_variable <= formula.nVars()) return;

    for (auto &c : formula.clauses) {
        for (size_t i = 0; i < c.size(); ++i) {
            if (var(c[i]) > largest_input_variable) {
                c[i] = mkLit(var(c[i]) + offset, sign(c[i]));
            }
        }
    }

    int oldVars = formula.nVars();
    while (formula.nVars() < oldVars + offset) formula.newVar();
}

std::vector<std::vector<Lit>> get_definition_clauses(Formula &f1, int largest_input_variable)
{
    std::vector<std::vector<Lit>> l2r;

    for (const auto &c : f1.clauses) {
        bool move = false;
        for (size_t i = 0; i < c.size(); ++i) {
            if (var(c[i]) >= largest_input_variable) {
                move = true;
                break;
            }
        }
        if (move) l2r.push_back(c);
    }

    return l2r;
}

/// make sure variables above largest_input_variables are similarly dependent in both formulas
/// for miters: assume variable sets being mutually exclusive
void exchange_definition_clauses(Formula &f1, Formula &f2, int largest_input_variable)
{
    std::vector<std::vector<Lit>> l2r, r2l;

    // get all clauses in f1 and f2 that have variable beyond largest variable
    l2r = get_definition_clauses(f1, largest_input_variable);
    std::cerr << "c extracted " << l2r.size() << " clauses from f1" << std::endl;
    r2l = get_definition_clauses(f2, largest_input_variable);
    std::cerr << "c extracted " << r2l.size() << " clauses from f2" << std::endl;

    // add the clauses mutually to each formula
    for (const auto &c : l2r) f2.addClause_(c);
    for (const auto &c : r2l) f1.addClause_(c);
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
    int opt;
    int tseitin = 0;
    int randmom_drop = 0;
    std::cerr << "c CNFmiter generates a CNF formula " << std::endl
              << "c which is unsatisfiable, if the given 2 formulas are equivalent" << std::endl;


    // Retrieve the options:
    while ((opt = getopt(argc, argv, "r:t:")) != -1) { // for each option...
        switch (opt) {
        case 'r':
            randmom_drop = atoi(optarg);
            std::cerr << "c randomly drop " << randmom_drop << " clauses from first formula" << std::endl;
            break;
        case 't':
            tseitin = atoi(optarg);
            std::cerr << "c set tseitin variable to " << tseitin << std::endl;
            break;
        case '?': // unknown option...
            std::cerr << "c unknown option: '" << char(optopt) << "'!" << std::endl;
            exit(1);
            break;
        }
    }

    if (optind + 2 != argc) {
        std::cerr << "not enough parameters, abort!" << std::endl;
        return 1;
    }

    std::string fn1 = (argv[optind + 0]);
    std::string fn2 = (argv[optind + 1]);
    gzFile in1 = gzopen(fn1.c_str(), "rb");
    gzFile in2 = gzopen(fn2.c_str(), "rb");

    if (!in1) {
        std::cerr << "failed to open first file, abort!" << std::endl;
        return 1;
    }
    if (!in2) {
        std::cerr << "failed to open second file, abort!" << std::endl;
        return 1;
    }
    if (tseitin < 0) {
        std::cerr << "tseitin variable negative, abort" << std::endl;
        return 1;
    }
    if (randmom_drop < 0) {
        std::cerr << "random_drop value negative, abort" << std::endl;
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
    if (tseitin > 0) {
        int offset = tseitin;
        offset = maxV > tseitin ? maxV - tseitin : 0;
        std::cerr << "c offset " << offset << " tseitin: " << tseitin << " maxV: " << maxV << std::endl;
        assert(offset == 0 || f2.nVars() <= tseitin || f2.nVars() + offset > f1.nVars());

        rewrite_variable_range(f2, tseitin, offset);

        maxV = f1.nVars() > f2.nVars() ? f1.nVars() : f2.nVars();

        while (f1.nVars() < maxV) f1.newVar();
        while (f2.nVars() < maxV) f2.newVar();

        exchange_definition_clauses(f1, f2, tseitin);
    }

    maxV = f1.nVars() > f2.nVars() ? f1.nVars() : f2.nVars();
    while (miter.nVars() < maxV) miter.newVar();

    std::cerr << "c Miter base formulas reserved " << miter.nVars() << " variables" << std::endl;

    generate_formula_miter(miter, f1, f2);

    std::size_t found = fn1.rfind("/");
    if (found != std::string::npos) fn1 = fn1.erase(0, found + 1);
    found = fn2.rfind("/");
    if (found != std::string::npos) fn2 = fn2.erase(0, found + 1);

    std::stringstream s;
    s << fn1 << " and " << fn2;
    if (tseitin != 0) s << " with tseitin base variable " << tseitin;
    if (randmom_drop) s << " with randomly dropping " << randmom_drop;
    print_formula(miter, s.str());

    return 0;
}