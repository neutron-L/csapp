./csim -v -s 1 -E 1 -b 1 -t traces/yi2.trace > 1.txt
./csim-ref -v -s 1 -E 1 -b 1 -t traces/yi2.trace > 2.txt
diff 1.txt 2.txt

./csim -v -s 4 -E 2 -b 4 -t traces/yi.trace > 1.txt
./csim-ref -v -s 4 -E 2 -b 4 -t traces/yi.trace > 2.txt
diff 1.txt 2.txt
./csim -v -s 2 -E 1 -b 4 -t traces/dave.trace > 1.txt
./csim-ref -v -s 2 -E 1 -b 4 -t traces/dave.trace > 2.txt
diff 1.txt 2.txt
./csim -v -s 2 -E 1 -b 3 -t traces/trans.trace > 1.txt
./csim-ref -v -s 2 -E 1 -b 3 -t traces/trans.trace > 2.txt
diff 1.txt 2.txt
./csim -v -s 2 -E 2 -b 3 -t traces/trans.trace > 1.txt
./csim-ref -v -s 2 -E 2 -b 3 -t traces/trans.trace > 2.txt
diff 1.txt 2.txt
./csim -v -s 2 -E 4 -b 3 -t traces/trans.trace > 1.txt
./csim-ref -v -s 2 -E 4 -b 3 -t traces/trans.trace > 2.txt
diff 1.txt 2.txt
./csim -v -s 5 -E 1 -b 5 -t traces/trans.trace > 1.txt
./csim-ref -v -s 5 -E 1 -b 5 -t traces/trans.trace > 2.txt
diff 1.txt 2.txt
./csim -v -s 5 -E 1 -b 5 -t traces/long.trace > 1.txt
./csim-ref -v -s 5 -E 1 -b 5 -t traces/long.trace > 2.txt
diff 1.txt 2.txt

