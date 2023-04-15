all:
	gcc -g -o sim *.c
clean:
	rm -f sim && rm -f result.txt && rm -f output_memory_map.txt
run:
	rm -f ../logs/test.txt && rm -f sim && gcc -g -o sim *.c && clear && ./sim ../programs/program4.txt pipeline > result.txt
compare:
	diff -iw ../logs/program1_result.txt ../logs/test.txt
