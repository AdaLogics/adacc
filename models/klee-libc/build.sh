CS="atexit.c atoi.c bcmp.c calloc.c htonl.c memchr.c mempcpy.c putchar.c stpcpy.c strcat.c strchr.c strcmp.c strcoll.c strcpy.c strlen.c strncmp.c strncpy.c strrchr.c strtol.c strtoul.c tolower.c toupper.c"

for f in ${CS}
do
	echo ${f}
	SYMCC_REGULAR_LIBCXX=1 symcc -fsanitize-coverage=inline-8bit-counters -c ${f} -o ${f}.o
done

ar -cr minilibc.a *.o 
