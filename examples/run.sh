#!/bin/bash


check_unsat ()
{
    local S="$1"
    local C="$2"
    
    local STATUS=0
    "$S" "$C" | grep "s UNSATISFIABLE" || STATUS=$? &> /dev/null
    
    if [ "$STATUS" -ne 0 ]; then
        echo "Did not get exit code 20 when running $S $C, but $STATUS"
        exit 1
    fi
}

SCRIPTDIR=$(cd $(dirname $0) && pwd)

make -C "$SCRIPTDIR"/..

# select a solver that is hopefully avialable
for solver in minisat picosat
do
    if command -v "$solver" &> /dev/null
    then
        break
    fi
done

if [ -z "$solver" ]; then
    echo "did not find a solver"
    exit 1
fi

echo "Use solver: $solver"

# run in this script's directory
cd "$SCRIPTDIR"

trap '[ -r "$TMPCNF" ] && rm -f "$TMPCNF"' EXIT
TMPCNF=$(mktemp)

# plain miters
../cnfmiter 1.cnf 1.cnf > "$TMPCNF" 2> /dev/null
check_unsat "$solver" "$TMPCNF"
../cnfmiter 2.cnf 2.cnf > "$TMPCNF" 2> /dev/null
check_unsat "$solver" "$TMPCNF"
../cnfmiter 3.cnf 3.cnf > "$TMPCNF" 2> /dev/null
check_unsat "$solver" "$TMPCNF"
../cnfmiter 4.cnf 4.cnf > "$TMPCNF" 2> /dev/null
check_unsat "$solver" "$TMPCNF"

# Tseitin miters
../cnfmiter -t 4 amo-4-naive.cnf amo-4-eq.cnf > "$TMPCNF" 2> /dev/null
check_unsat "$solver" "$TMPCNF"
../cnfmiter -t 4 amo-4-eq.cnf amo-4-naive.cnf > "$TMPCNF" 2> /dev/null
check_unsat "$solver" "$TMPCNF"

../cnfmiter -t 7 amk-7-2-bdd.cnf amk-7-2-card.cnf > "$TMPCNF" 2> /dev/null
check_unsat "$solver" "$TMPCNF"
../cnfmiter -t 7 amk-7-2-card.cnf amk-7-2-bdd.cnf > "$TMPCNF" 2> /dev/null
check_unsat "$solver" "$TMPCNF"