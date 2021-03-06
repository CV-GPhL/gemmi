#!/bin/bash

# Usage: compare_record.sh RECORD [DATE] < entries.idx
#
# For all PDB entries deposited in the specified month or year
# compares the specified record between PDB files converted by gemmi from mmCIF
# and the corresponding PDB files from a local copy of the PDB archive.
#
# The entries.idx file can be downloaded from the PDB:
# wget ftp://ftp.wwpdb.org/pub/pdb/derived_data/index/entries.idx
#
# The local copy of the the PDB archive must be pointed to by $PDB_DIR.
#
# Customization via environment variables:
#  - setting diff path or wrapper: DIFF=colordiff
#  - PDB->PDB conversion: FROM_PDB=1
#
# Example (date is Jan 2017): compare_record.sh LINK "01/../17" < entries.idx
#

set -eu
RECORD="^${1}"
MONTH="${2:-.}"'$'
cd `dirname $0`
PDB_COPY="$PDB_DIR/structures/divided"

if [ -f ../build/gemmi ]; then
  GEMMI=../build/gemmi
elif [ -f ../gemmi ]; then
  GEMMI=../gemmi
else
  echo "gemmi executable not found"
  exit 1
fi

cut -f1,3 - | grep "$MONTH" | while read -r code date; do
  code=${code,,}
  echo $code
  pdb=${PDB_COPY:-/hdd}/pdb/${code:1:2}/pdb${code}.ent.gz
  if [[ ${FROM_PDB:-} = 1 ]]; then
    inp="$pdb"
  else
    inp=${PDB_COPY:-/hdd}/mmCIF/${code:1:2}/${code}.cif.gz
  fi
  ${DIFF:-diff} -U0 --label="$pdb" --label="from $inp" \
      <(zgrep $RECORD "$pdb") \
      <($GEMMI convert --to=pdb "$inp" - | grep $RECORD) ||:
done
