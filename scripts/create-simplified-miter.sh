#!/bin/bash
#
# Use Coprocessor simplifier to create CNF based miter based on an existing
# miter.

# Directory of this script
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

# Location of coprocessor binary
COPROCESSOR="$SCRIPT_DIR"/coprocessor
CNFMITER="$SCRIPT_DIR"/cnfmiter
OUTPUT_FILE=""

while getopts "c:dm:o:" OPTION; do
    case $OPTION in
    c)
        COPROCESSOR=$OPTARG
        ;;
    d)
        set -x
        ;;
    m)
        CNFMITER=$OPTARG
        ;;
    o)
        OUTPUT_FILE=$OPTARG
        ;;
    *)
        echo "Incorrect options provided"
        exit 1
        ;;
    esac
done
shift $((OPTIND-1))

echo "c process remaining args: $*"

if [ ! -x "$COPROCESSOR" ]; then
    echo "Cannot execute coprocessor '$COPROCESSOR'"
    exit 1
fi

INPUT="$1"
if [ ! -r "$INPUT" ]; then
    echo "Cannot read input file '$INPUT'"
    exit 1
fi

VARS=0

# count number of vars in file
if [[ "$INPUT" = *.gz ]]; then
    VARS="$(zcat "$INPUT" | awk '/p cnf/ {print $4}')"
else
    VARS="$(cat "$INPUT" | awk '/p cnf/ {print $4}')"
fi

# create a temporary working directory
trap '[ ! -r "$WORK_DIR" ] || rm -f "$WHITE_FILE"' EXIT
WORK_DIR=$(mktemp -d)

# make sure coprocessor does not change equivalence
WHITE_FILE="$WORK_DIR/white.var"
touch "$WHITE_FILE"
if [ ! -r "$WHITE_FILE" ]; then
    echo "Could not create temporary file '$WHITE_FILE'"
    exit 1
fi
echo "1..$VARS" > "$WHITE_FILE"

# make sure coprocessor does not change equivalence
SIMPLIFIED_CNF="$WORK_DIR/simplified-$(basename $INPUT .gz)"
touch "$SIMPLIFIED_CNF"
if [ ! -r "$SIMPLIFIED_CNF" ]; then
    echo "Could not create temporary file '$SIMPLIFIED_CNF'"
    exit 1
fi

# we want the CP3 output
CP3_STDERR="$WORK_DIR/cp3_stderr.cnf"
touch "$CP3_STDERR"
if [ ! -r "$CP3_STDERR" ]; then
    echo "Could not create coprocessor stderr file '$CP3_STDERR'"
    exit 1
fi

# we want the cp3 miter output
CNFMITER_STDERR="$WORK_DIR/cnfmiter_stderr.cnf"
touch "$CNFMITER_STDERR"
if [ ! -r "$CNFMITER_STDERR" ]; then
    echo "Could not create miter stderr file '$CNFMITER_STDERR'"
    exit 1
fi

# we want a temporary CNF to dump all the files to
TMP_CNF="$WORK_DIR/tmp_miter_cnf.cnf"
touch "$TMP_CNF"
if [ ! -r "$TMP_CNF" ]; then
    echo "Could not create tmp cnf file '$TMP_CNF'"
    exit 1
fi

# we want a temporary CNF to dump all the files to
OUTPUT_CNF="$WORK_DIR/output_miter_cnf.cnf"
touch "$OUTPUT_CNF"
if [ ! -r "$OUTPUT_CNF" ]; then
    echo "Could not create miter output file '$OUTPUT_CNF'"
    exit 1
fi

# simplify formula to an equivalent one
SIMPLIFY_STATUS=0
"$COPROCESSOR" "$INPUT" -whiteList="$WHITE_FILE" -dimacs="$SIMPLIFIED_CNF" -no-dense 2> "$CP3_STDERR" 1> /dev/null || SIMPLIFY_STATUS=$?
echo "c simplficitaion returned with $SIMPLIFY_STATUS, abort"
if [ "$SIMPLIFY_STATUS" -ne 0 ] && [ "$SIMPLIFY_STATUS" -ne 20 ] && [ "$SIMPLIFY_STATUS" -ne 10 ]; then
    echo "c exit, due to simplification status $SIMPLIFY_STATUS"
    exit 1
fi

# Make sure we provide plain CNF
READ_INPUT="$WORK_DIR/$(basename $INPUT .gz)"
if [[ "$INPUT" = *.gz ]]; then
    zcat "$INPUT" > "$READ_INPUT"
else
    cat "$INPUT" > "$READ_INPUT"
fi

# Create the miter CNF to the given output, or stdout
MITER_STATUS=0
"$CNFMITER" "$READ_INPUT" "$SIMPLIFIED_CNF" 2> "$CNFMITER_STDERR" > "$TMP_CNF" || MITER_STATUS=$?

# Assemble all output in a single file, first miter output
cat << EOF >> "$CNFMITER_STDERR"
c miter input file: $INPUT
c cnfmiter status: $MITER_STATUS
c
c Coprocessor simplification output
EOF

# Next, simplification output
cat "$CP3_STDERR" >> "$CNFMITER_STDERR"
echo "c Coprocessor status: $SIMPLIFY_STATUS" >> "$CNFMITER_STDERR"

# Finally, formula
cat "$TMP_CNF" >> "$CNFMITER_STDERR"

# Add another header
cat << EOF > "$OUTPUT_CNF"
c CNF simplfication miter, 2020, Norbert Manthey
c
c This CNF has been generated from a given CNF input file $(basename "$INPUT")
c From this file, Coprocessor produced an equivalent, eventually simplified,
c CNF. From the original, and simplified formula, a miter formula is generated,
c that is unsatisfiable if and only if the original and simplified formula are
c actually equivalent.
c
c The default CLI of Coprocessor has been used.
c
EOF

# Drop info from output
cat "$CNFMITER_STDERR" | grep -v "$USER" >> "$OUTPUT_CNF"

# Write to file, if requested
if [ -n "$OUTPUT_FILE" ]; then
    # gzip, if requested
    if [[ "$OUTPUT_FILE" = *.gz ]]; then
        gzip "$OUTPUT_CNF"
        cp "$OUTPUT_CNF".gz "$OUTPUT_FILE"
    else
        cp "$OUTPUT_CNF" "$OUTPUT_FILE"
    fi
else
    # plain print, otherwise
    cat "$OUTPUT_CNF"
fi
