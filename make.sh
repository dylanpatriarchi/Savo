bison -dy parser.y
flex -w lexer.l
gcc -w -c global.c
gcc -w -c statements/printstmt.c   -o statements/printstmt.o 
gcc -w -c y.tab.c
gcc -w -c lex.yy.c
gcc -w -o console.exe global.o statements/printstmt.o y.tab.o lex.yy.o
clear
rm *.o
rm statements/*.o
./console.exe
