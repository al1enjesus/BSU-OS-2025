set -euo pipefail

BIN=./thread_race
M=${M:-2000000}
R=${R:-5}
Ns=(${NS:-1 2 4 8})

gcc -O2 -std=gnu11 -pthread thread_race.c -o "$BIN"

modes=("mutex" "atomic")
: > task1_run.log
for mode in "${modes[@]}"; do
  for n in "${Ns[@]}"; do
    for r in $(seq 1 "$R"); do
      "$BIN" "$n" "$M" "$mode"
    done
  done
done | tee -a task1_run.log

echo
echo "Summary (avg time_ms over $R runs)"
awk -v R="$R" '
/^mode=/ {
  for(i=1;i<=NF;i++){
    split($i,a,"=");
    k=a[1]; v=a[2];
    data[k]=v;
  }
  key=data["mode"]"-N"data["threads"]"-M"data["iters_per_thread"];
  sum[key]+=data["time_ms"];
  cnt[key]++;
}
END{
  printf "%-8s %-6s %-12s %-12s\n","mode","N","M(per-thread)","avg_time_ms";
  PROCINFO["sorted_in"]="@ind_str_asc";
  for(k in sum){
    split(k,parts,"-");
    mode=parts[1]; N=substr(parts[2],2); M=substr(parts[3],2);
    avg=sum[k]/cnt[k];
    printf "%-8s %-6s %-12s %-12.2f\n", mode, N, M, avg;
    Ns[N]=1; Ms[N]=M;
  }
}' task1_run.log
