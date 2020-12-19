#!/usr/bin/env bash
# Benchmark scaling performance of multithreaded toy analyzer ppodd

# Number of logical CPUs on this system
NLOGICAL=$(getconf _NPROCESSORS_ONLN)
# Maximum number of analysis threads. Oversubscribe to verify saturation behavior
N=$((NLOGICAL+2))

# Number of events per run
if [ $# -ge 1 ]; then
  NEV=$1
else
  NEV=1000
fi
NMARK=$((NEV/50))

[ -n "$NMARK" ] && MARK="-m $NMARK"

# Arguments for getting memory usage from time(1)
osname=$(uname -s)
if [ "$osname" = "Linux" ]; then
  TARGS="-f"
  TFMT="%M maximum resident set size"
elif [ "$osname" = "Darwin" ]; then
  TARGS="-lp"
  TFMT="--"  # kludge to avoid "no such file or directory"
else
  echo "Unsupported platform"
  exit
fi

# Find executable, preferably from this build
exepaths="cmake-build-release cmake-build-relwithdebinfo build ."
for P in $exepaths; do
  if [ -x "$P/ppodd" ]; then
    PPODD="$P/ppodd"
    break
  fi
done
# Not found? Maybe it is in the PATH?
[ -z "$PPODD" ] && PPODD="$(which ppodd)"
if [ -z "$PPODD" ]; then
  echo "Cannot find ppodd"
  exit
fi
echo "Benchmarking $PPODD"
echo "Found $NLOGICAL logical CPUs"

# Scratch file and result file
TMPF=/tmp/ppodd-benchmark.tmp
RESF=benchmark.out
rm -f $RESF $TMPF

echo $NEV > $RESF
J=1
while [ $J -le $N ]; do
  [ $J -gt 1 ] && PLURAL="s"
  printf "Running %d analysis thread$PLURAL\n" $J
  if /usr/bin/time $TARGS "$TFMT" "$PPODD" -d1 -n $NEV -j $J $MARK -z test.dat |& tee $TMPF ; then
    { printf "%d " $J
      grep "Init:.*ms" $TMPF | awk '{printf("%s ",$2)}'
      grep "Analysis:.*ms" $TMPF | awk '{printf("%s ",$2)}'
      grep "Output:.*ms" $TMPF | awk '{printf("%s ",$2)}'
      grep "Total CPU:.*ms" $TMPF | awk '{printf("%s ",$3)}'
      grep "Real:.*ms" $TMPF | awk '{printf("%s ",$2)}'
    } >> $RESF
    if [ "$osname" = "Linux" ]; then
      # Linux time(1) outputs kilobytes for the resident set size
      grep "maximum resident set size" $TMPF | awk '{printf("%s\n",$1*1000)}' >> $RESF
    else
      grep "maximum resident set size" $TMPF | awk '{printf("%s\n",$1)}' >> $RESF
    fi
  else
    echo "Error running ppodd for nthreads = $N"
    break
  fi
  J=$((J+1))
done
rm -f $TMPF
unset N NEV NMARK TARGS TFMT TMPF RESF PPODD
