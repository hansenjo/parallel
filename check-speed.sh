#!/usr/bin/env bash
# Benchmark scaling performance of multithreaded toy analyzer ppodd

# Maximum number of analysis threads. Oversubscribe to verify saturation behavior
N=$[$(nproc)+2]

# Number of events per run
NEV=1000

# Timing command. FIXME:  This is for macOS. Add Linux version.
TIME="/usr/bin/time -lp"

# Scratch file and result file
TMPF=/tmp/check-speed.tmp
RESF=./check-speed.out
rm -f $RESF $TMPF
while [ $N -gt 0 ]; do
  printf "Benchmarking N = %d\n" $N
  $TIME ./cmake-build-release/ppodd -d1 -n $NEV -j $N -z test.dat |& tee $TMPF
  if [ $? -eq 0 ]; then
    printf "%d " $N >> $RESF
    grep "Init:.*ms" $TMPF | awk '{printf("%s ",$2)}' >> $RESF
    grep "Analysis:.*ms" $TMPF | awk '{printf("%s ",$2)}' >> $RESF
    grep "Output:.*ms" $TMPF | awk '{printf("%s ",$2)}' >> $RESF
    grep "Total CPU:.*ms" $TMPF | awk '{printf("%s ",$3)}' >> $RESF
    grep "Real:.*ms" $TMPF | awk '{printf("%s ",$2)}' >> $RESF
    grep "maximum resident set size" $TMPF | awk '{printf("%s\n",$1)}' >> $RESF
  else
    echo "Error running ppodd for nthreads = $N"
    break
  fi
  N=$[$N-1]
done
rm -f $TMPF
unset N NEV TIME TMPF RESF
