#!/bin/bash
set -e

rm -rf deploy
mkdir deploy
cd deploy

windeployqt.exe ../release/acquisition.exe --release --no-compiler-runtime --dir .
cp ../release/acquisition.exe .
cp ../redist/* .
rm -rf bearer
echo "[Paths]\nLibraries=./plugins" > qt.conf

EXE_TIME=$(stat -c %Y ../release/acquisition.exe)
PDB_TIME=$(stat -c %Y ../release/acquisition.pdb)
TIME_DELTA=$(($EXE_TIME-$PDB_TIME))

if (( $TIME_DELTA > 60 )); then
    echo "Last modification time difference between exe/pdb is too large: $TIME_DELTA, please check that .pdb is correctly generated."
    exit 1
fi
cp ../release/acquisition.pdb .
echo $(git rev-parse HEAD) > gitrev.txt

cd ..
