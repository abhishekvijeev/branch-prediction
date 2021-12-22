workloads=('fp_1' 'fp_2' 'int_1' 'int_2' 'mm_1' 'mm_2')

# G-Share
for workload in ${workloads[@]}; do
    echo "$workload"
    for i in {1..30}; do
        bunzip2 -kc "../traces/$workload.bz2" | ./predictor "--gshare:$i"
    done
done

# Tournament with variable global history
for workload in ${workloads[@]}; do
    echo "$workload"
    for i in {1..30}; do
        bunzip2 -kc "../traces/$workload.bz2" | ./predictor "--tournament:$i:10:10"
    done
done

# Tournament with variable local history
for workload in ${workloads[@]}; do
    echo "$workload"
    for i in {1..30}; do
        bunzip2 -kc "../traces/$workload.bz2" | ./predictor "--tournament:9:$i:10"
    done
done
