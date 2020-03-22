#!/bin/bash
#
# Get and build dependencies for simplification miter

# In case something fails, fail
set -e

# Directory of this script
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

# build CNF miter tool
echo "build cnfmiter tool ..."
make -C "$SCRIPT_DIR/.."

# get coprocessor via riss repository
pushd "$SCRIPT_DIR"
echo "clone fresh riss repository ..."
rm -rf riss
git clone https://github.com/conp-solutions/riss.git

# build coprocessor
pushd riss
echo "get commit 2f6fac91d328efe2ce0720af11ef7df1fc288813 ..."
git checkout 2f6fac91d328efe2ce0720af11ef7df1fc288813 # make sure we know what we use
mkdir build
cd build
echo "run cmake for riss ..."
cmake -DCMAKE_BUILD_TYPE=Release -DDRATPROOF=OFF ..
echo "build coprocessor ..."
make coprocessor -j $(nproc)
popd

# add link to coprocessor binary
echo "add coprocessor link ..."
ln -fs riss/build/bin/coprocessor "$SCRIPT_DIR"

popd

# test whether we are good
echo "check success ..."
[ -x "$SCRIPT_DIR"/cnfmiter ] || echo "cannot execute cnfmiter at $SCRIPT_DIR/cnfmiter"
[ -x "$SCRIPT_DIR"/coprocessor ] || echo "cannot execute cnfmiter at $SCRIPT_DIR/cnfmiter"

echo "done"
