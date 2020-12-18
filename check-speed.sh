#!/usr/bin/env bash
# Benchmark scaling performance of multithreaded toy analyzer ppodd

# Maximum number of analysis threads. Oversubscribe to verify saturation behavior
N=$[$(nproc)+2]

# Number of events per run
NEV=1000

# Timing command
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
  if [ -x $P/ppodd ]; then
    PPODD=$P/ppodd
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

# Scratch file and result file
TMPF=/tmp/check-speed.tmp
RESF=check-speed.out
rm -f $RESF $TMPF

while [ $N -gt 0 ]; do
  printf "Running %d analysis threads\n" $N
  /usr/bin/time $TARGS "$TFMT" "$PPODD" -d1 -n $NEV -j $N -z test.dat |& tee $TMPF
  if [ $? -eq 0 ]; then
    printf "%d " $N >> $RESF
    grep "Init:.*ms" $TMPF | awk '{printf("%s ",$2)}' >> $RESF
    grep "Analysis:.*ms" $TMPF | awk '{printf("%s ",$2)}' >> $RESF
    grep "Output:.*ms" $TMPF | awk '{printf("%s ",$2)}' >> $RESF
    grep "Total CPU:.*ms" $TMPF | awk '{printf("%s ",$3)}' >> $RESF
    grep "Real:.*ms" $TMPF | awk '{printf("%s ",$2)}' >> $RESF
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
  N=$[$N-1]
done
rm -f $TMPF
unset N NEV TARGS TFMT TMPF RESF PPODD
