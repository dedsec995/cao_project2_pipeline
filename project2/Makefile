all:
	gcc -g -o sim *.c
clean:
	rm -f sim
test:
	gcc -g -o test test.c && ./test && rm -f test
nt:
	gcc -g -o nt new_test.c && ./nt && rm -f nt
run:
	rm -f ../logs/test.txt && rm -f sim && gcc -g -o sim *.c && ./sim ../programs/program1.txt > result.txt
compare:
	diff -iw ../logs/program1_result.txt ../logs/test.txt
