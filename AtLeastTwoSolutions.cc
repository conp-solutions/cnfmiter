#include "Dimacs.h"

#include <zlib.h>

#include <iostream>
#include <sstream>
#include <string>

using namespace CNFMITER;

/// add clauses to f, which encode: (a <-> b <-> c)
void generate_equivalence(Formula &f, Lit a, Lit b, Lit c)
{
    static std::vector<Lit> C(3, a);

    C[0] = a;
    C[1] = b;
    C[2] = c;
    f.addClause_(C);

    C[0] = ~a;
    C[1] = ~b;
    C[2] = c;
    f.addClause_(C);

    C[0] = a;
    C[1] = ~b;
    C[2] = ~c;
    f.addClause_(C);

    C[0] = ~a;
    C[1] = b;
    C[2] = ~c;
    f.addClause_(C);
}

void print_formula(Formula &f, std::string s)
{
    std::cout << "c AtLeastTwoSolutions, Norbert Manthey, 2021" << std::endl;
    if (!s.empty()) std::cout << "c " << s << std::endl;
    std::cout << "c " << std::endl;
    std::cout << "p cnf " << f.nVars() << " " << f.clauses.size() << std::endl;
    for (const auto &c : f.clauses) {
        std::stringstream s;
        s << c;
        printf("%s 0\n", s.str().c_str());
    }
}

void print_maxsat_formula(Formula &hardclauses, std::vector <Lit> penalty_literals, std::string s, bool pre2021format = true)
{
    std::cout << "c AtLeastTwoSolutions, Norbert Manthey, 2021" << std::endl;
    if (!s.empty()) std::cout << "c " << s << std::endl;
    std::cout << "c print in pre2021 MaxSat format: " << pre2021format << std::endl;
    std::cout << "c " << std::endl;
    if(pre2021format)
    {
        size_t top = penalty_literals.size() + 1;
        std::cout << "p wcnf " << hardclauses.nVars() << " " << hardclauses.clauses.size() + penalty_literals.size() << " " << top << std::endl;
        /* print the soft unit clauses */
        for (const auto &unit : penalty_literals) {
            std::stringstream s;
            s << 1 << " " << unit;
            printf("%s 0\n", s.str().c_str());
        }
        /* print the hard clauses */
        for (const auto &c : hardclauses.clauses) {
            std::stringstream s;
            s << top << " " << c;
            printf("%s 0\n", s.str().c_str());
        }
    } else {
        /* print the soft unit clauses */
        for (const auto &unit : penalty_literals) {
            std::stringstream s;
            s << 1 << " " << unit;
            printf("%s 0\n", s.str().c_str());
        }
        /* print the hard clauses */
        for (const auto &c : hardclauses.clauses) {
            std::stringstream s;
            s << "h " << c;
            printf("%s 0\n", s.str().c_str());
        }
    }

}

int main(int argc, char **argv)
{
    int opt;
    int tseitin = 0;
    int maxsat = 0;
    std::cerr << "c AtLeastTwoSolutions generates a CNF formula " << std::endl
              << "c which is satisfiable if the given input formula has at least 2 models" << std::endl
              << "c" << std::endl
              << "c OPTIONS" << std::endl
              << "c -t x ... only force differences among the variables 1 to x" << std::endl
              << "c -W   ... encode a MaxSat formula that tries to get two solutions with largest hamming distance" << std::endl
              << "c -w   ... same as -w, but use the pre 2020 MaxSat format" << std::endl
              << std::endl;

    // Retrieve the options:
    while ((opt = getopt(argc, argv, "t:wW")) != -1) { // for each option...
        switch (opt) {
        case 't':
            tseitin = atoi(optarg);
            std::cerr << "c set tseitin variable to " << tseitin << std::endl;
            break;
        case 'w':
            maxsat = 1;
            std::cerr << "c enable maxsat pre-2021 mode" << std::endl;
            break;
        case 'W':
            maxsat = 2;
            std::cerr << "c enable maxsat post-2020 mode" << std::endl;
            break;
        case '?': // unknown option...
            std::cerr << "c unknown option: '" << char(optopt) << "'!" << std::endl;
            exit(1);
            break;
        }
    }

    if (optind + 1 != argc) {
        std::cerr << "not enough parameters, abort!" << std::endl;
        return 1;
    }

    std::string fn1 = (argv[optind + 0]);
    gzFile in1 = gzopen(fn1.c_str(), "rb");

    if (!in1) {
        std::cerr << "failed to open first file, abort!" << std::endl;
        return 1;
    }
    if (tseitin < 0) {
        std::cerr << "tseitin variable negative, abort" << std::endl;
        return 1;
    }

    Formula f1;

    parse_DIMACS(in1, f1);
    gzclose(in1);

    std::cerr << "c Parsed formula with " << f1.nVars() << " vars and " << f1.clauses.size() << std::endl;

    Formula result;
    int input_vars = f1.nVars();
    int var_offset = input_vars;
    int output_vars = 2 * input_vars;
    while (output_vars > result.nVars()) result.newVar();
    std::vector<Lit> rewritten_clause;

    /* add formula 2 times, once with a full variable offset */
    /* duplicate this, to check whether a formuala has more models */
    for (const auto &clause : f1.clauses) {
        result.addClause_(clause);
        rewritten_clause.clear();
        for (int i = 0; i < clause.size(); ++i) {
            Lit l = clause[i];
            Var v = var(l);
            rewritten_clause.push_back(mkLit(v + var_offset, sign(l)));
        }
        result.addClause_(rewritten_clause);
    }

    /* encode variable differences for given number of variables */
    /* this grows quadratic in the number of models that should be checked for */
    Var max_v = tseitin == 0 ? input_vars : tseitin;
    int next_var = output_vars;
    std::vector<Lit> one_unequal_clause;
    std::cerr << "c encode variable equivalences for first " << max_v << " variables" << std::endl;
    for (Var v = 0; v < max_v; ++v) {
        Lit a = mkLit(v);
        Lit A = mkLit(v + var_offset);
        Lit next_lit = mkLit(next_var++);
        while (var(next_lit) >= result.nVars()) result.newVar();
        /* a <-> (a+offset) <-> next_lit */
        generate_equivalence(result, a, A, next_lit);
        one_unequal_clause.push_back(~next_lit);
    }

    /* enforce that at least one assignment has to be different */
    result.addClause_(one_unequal_clause);

    std::stringstream s;
    if (maxsat == 0) {
        s << "encode formula to check whether there are more than 1 solution for " << fn1;
        if (tseitin != 0) s << " with tseitin base variable " << tseitin;
        /* one of the common literal pair should have unequal truth values has to be */
        print_formula(result, s.str());
    } else {
        s << "encode formula to find two solutions with the highest hamming distance for a given formula, at least 1, for " << fn1;
        if (tseitin != 0) s << " with tseitin base variable " << tseitin;
        /* there is a cost setting variables to equal truth values, hence, pay cost for each unit */
        print_maxsat_formula(result, one_unequal_clause, s.str(), maxsat == 1);
    }


    return 0;
}