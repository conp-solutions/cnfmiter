#include "pb2cnf.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

using namespace PBLib;


const char *encoder[] = { "BEST", "BDD", "SWC", "SORTINGNETWORKS", "ADDER", "BINARY_MERGE" };

int main(int argc, char *argv[])
{
    cout << "c PBcoder, based on PBLib, Norbert Manthey, 2021" << endl;

    if (argc < 3) {
        cout << " USAGE: " << endl
             << endl
             << " pbcode [ENCODING] w1 w2 ... wn k" << endl
             << endl
             << " for: w1 1 + w2 2 + ... + wn n <= k" << endl
             << " specify the weights, as well as 'k' in order on the CLI" << endl
             << endl
             << " Encodings: 'BEST', 'BDD', 'SWC', 'SORTINGNETWORKS', 'ADDER', 'BINARY_MERGE'" << endl
             << " The generated CNF is printed on the screen" << endl;
        return 0;
    }

    PBConfig config = make_shared<PBConfigClass>();
    vector<WeightedLit> literals;
    int k = 0;

    std::string destination;
    int options = 0;
    for (int i = 1; i < argc; ++i) { // Remember argv[0] is the path to the program, we want from argv[1] onwards

        if (string(argv[i]) == string("BEST")) {
            config->pb_encoder = PB_ENCODER::BEST;
            options++;
            continue;
        } else if (string(argv[i]) == string("BDD")) {
            config->pb_encoder = PB_ENCODER::BDD;
            options++;
            continue;
        } else if (string(argv[i]) == string("SWC")) {
            config->pb_encoder = PB_ENCODER::SWC;
            options++;
            continue;
        } else if (string(argv[i]) == string("SORTINGNETWORKS")) {
            config->pb_encoder = PB_ENCODER::SORTINGNETWORKS;
            options++;
            continue;
        } else if (string(argv[i]) == string("ADDER")) {
            config->pb_encoder = PB_ENCODER::ADDER;
            options++;
            continue;
        } else if (string(argv[i]) == string("BINARY_MERGE")) {
            config->pb_encoder = PB_ENCODER::BINARY_MERGE;
            options++;
            continue;
        }

        int w;
        const char *c = argv[i];
        w = atoi(argv[i]);
        cout << "c parse at " << i - options << ": " << argv[i] << endl;
        if (w == 0) {
            cerr << "detected defective input " << argv[i] << endl;
            exit(1);
        }

        if (i + 1 < argc)
            literals.push_back(WeightedLit(i - options, w));
        else {
            k = w;
        }
    }
    cout << "c " << literals.size() << " literals, and k: " << k << endl;
    cout << "c code variables: " << argc - options << endl;

    AuxVarManager auxvars(argc - options);
    VectorClauseDatabase formula(config);
    PB2CNF pb2cnf(config);

    PBConstraint constraint(literals, LEQ, k);
    pb2cnf.encode(constraint, formula, auxvars);

    cout << "p cnf " << auxvars.getBiggestReturnedAuxVar() << " " << formula.getClauses().size() << endl;
    formula.printFormula();
}
