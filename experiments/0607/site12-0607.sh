#!/bin/sh
ant clean-all build-all
ant hstore-prepare -Dproject=voterwintimehstore -Dhosts="localhost:0:0"
ant hstore-prepare -Dproject=voterwintimesstore -Dhosts="localhost:0:0"
ant hstore-prepare -Dproject=voterwintimesstorewinonly -Dhosts="localhost:0:0"
ant hstore-prepare -Dproject=voterwinhstore -Dhosts="localhost:0:0"
ant hstore-prepare -Dproject=voterwinsstore -Dhosts="localhost:0:0"
python ./tools/autorunexp.py -p voterwinhstore -o "experiments/0602/voterwinhstore-1c-100w1s-98-0602-site12.txt" --txnthreshold 0.98 -e "experiments/0602/site12-0602.txt" --winconfig "tuple 100w1s (site12)" --threads 1 --rmin 1000 --rstep 1000 --finalrstep 100 --warmup 10000 --numruns 5 
python ./tools/autorunexp.py -p voterwinsstore -o "experiments/0602/voterwinsstore-1c-100w1s-98-0602-site12.txt" --txnthreshold 0.98 -e "experiments/0602/site12-0602.txt" --winconfig "tuple 100w1s (site12)" --threads 1 --rmin 1000 --rstep 1000 --finalrstep 100 --warmup 10000 --numruns 5 
python ./tools/autorunexp.py -p voterwintimehstore -o "experiments/0602/voterwintimehstore-1c-30w1s100t-98-0602-site12.txt" --txnthreshold 0.98 -e "experiments/0602/site12-0602.txt" --winconfig "time 30w1s100t (site12)" --threads 1 --rmin 1000 --rstep 1000 --finalrstep 100 --warmup 10000 --numruns 5 
python ./tools/autorunexp.py -p voterwintimesstore -o "experiments/0602/voterwintimesstore-1c-30w1s100t-98-0602-site12.txt" --txnthreshold 0.98 -e "experiments/0602/site12-0602.txt" --winconfig "time 30w1s100t (site12)" --threads 1 --rmin 1000 --rstep 1000 --finalrstep 100 --warmup 10000 --numruns 5 
python ./tools/autorunexp.py -p voterwintimesstorewinonly -o "experiments/0602/voterwintimesstorewinonly-1c-30w1s100t-98-0602-site12.txt" --txnthreshold 0.98 -e "experiments/0602/site12-0602.txt" --winconfig "time 30w1s100t (site12)" --threads 1 --rmin 1000 --rstep 1000 --finalrstep 100 --warmup 10000 --numruns 5 

python ./tools/autorunexp.py -p voterwinhstore -o "experiments/0602/voterwinhstore-1c-100w1s-95-0602-site12.txt" --txnthreshold 0.95 -e "experiments/0602/site12-0602.txt" --winconfig "tuple 100w1s (site12)" --threads 1 --rmin 1000 --rstep 1000 --finalrstep 100 --warmup 10000 --numruns 5 
python ./tools/autorunexp.py -p voterwinsstore -o "experiments/0602/voterwinsstore-1c-100w1s-95-0602-site12.txt" --txnthreshold 0.95 -e "experiments/0602/site12-0602.txt" --winconfig "tuple 100w1s (site12)" --threads 1 --rmin 1000 --rstep 1000 --finalrstep 100 --warmup 10000 --numruns 5 
python ./tools/autorunexp.py -p voterwintimehstore -o "experiments/0602/voterwintimehstore-1c-30w1s100t-95-0602-site12.txt" --txnthreshold 0.95 -e "experiments/0602/site12-0602.txt" --winconfig "time 30w1s100t (site12)" --threads 1 --rmin 1000 --rstep 1000 --finalrstep 100 --warmup 10000 --numruns 5 
python ./tools/autorunexp.py -p voterwintimesstore -o "experiments/0602/voterwintimesstore-1c-30w1s100t-95-0602-site12.txt" --txnthreshold 0.95 -e "experiments/0602/site12-0602.txt" --winconfig "time 30w1s100t (site12)" --threads 1 --rmin 1000 --rstep 1000 --finalrstep 100 --warmup 10000 --numruns 5 
python ./tools/autorunexp.py -p voterwintimesstorewinonly -o "experiments/0602/voterwintimesstorewinonly-1c-30w1s100t-95-0602-site12.txt" --txnthreshold 0.95 -e "experiments/0602/site12-0602.txt" --winconfig "time 30w1s100t (site12)" --threads 1 --rmin 1000 --rstep 1000 --finalrstep 100 --warmup 10000 --numruns 5 

python ./tools/autorunexp.py -p voterwinhstore -o "experiments/0602/voterwinhstore-1c-100w1s-90-0602-site12.txt" --txnthreshold 0.90 -e "experiments/0602/site12-0602.txt" --winconfig "tuple 100w1s (site12)" --threads 1 --rmin 1000 --rstep 1000 --finalrstep 100 --warmup 10000 --numruns 5 
python ./tools/autorunexp.py -p voterwinsstore -o "experiments/0602/voterwinsstore-1c-100w1s-90-0602-site12.txt" --txnthreshold 0.90 -e "experiments/0602/site12-0602.txt" --winconfig "tuple 100w1s (site12)" --threads 1 --rmin 1000 --rstep 1000 --finalrstep 100 --warmup 10000 --numruns 5 
python ./tools/autorunexp.py -p voterwintimehstore -o "experiments/0602/voterwintimehstore-1c-30w1s100t-90-0602-site12.txt" --txnthreshold 0.90 -e "experiments/0602/site12-0602.txt" --winconfig "time 30w1s100t (site12)" --threads 1 --rmin 1000 --rstep 1000 --finalrstep 100 --warmup 10000 --numruns 5 
python ./tools/autorunexp.py -p voterwintimesstore -o "experiments/0602/voterwintimesstore-1c-30w1s100t-90-0602-site12.txt" --txnthreshold 0.90 -e "experiments/0602/site12-0602.txt" --winconfig "time 30w1s100t (site12)" --threads 1 --rmin 1000 --rstep 1000 --finalrstep 100 --warmup 10000 --numruns 5 
python ./tools/autorunexp.py -p voterwintimesstorewinonly -o "experiments/0602/voterwintimesstorewinonly-1c-30w1s100t-90-0602-site12.txt" --txnthreshold 0.90 -e "experiments/0602/site12-0602.txt" --winconfig "time 30w1s100t (site12)" --threads 1 --rmin 1000 --rstep 1000 --finalrstep 100 --warmup 10000 --numruns 5 