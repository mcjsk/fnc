#include <stdio.h>
/**
   A quick hack to dump JavaScript input from stdin
   to a C character array.
*/
int main(int argc, char const ** argv )
{
    int ch = 0;
    int col = 0;
    char const * name;
    if( 2 != argc ) {
        fprintf(stderr, "Usage: %s dataName\n", argv[0]);
        return 1;
    }
    name = argv[1];
    puts("/* auto-generated code - edit at your own risk! (Good luck with that!) */");
    printf("static char const %s_a[] = {\n", name);
    while(EOF != (ch = getchar())) {
        printf("%d, ",ch);
        if( 0 == (++col%20) ) {
            putchar('\n');
            col = 0;
        }
    }
    puts("\n0};");
    /* Not sure why, but without this level of indirection
       i am getting segfaults when dereferencing any byte of the
       array from code which imports it via an 'extern' decl.
    */
    printf("char const * %s = %s_a;\n", name, name);
    return 0;
}
