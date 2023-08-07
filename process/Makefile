EXEC:= run1 lsproc sendsignal sighandler zombie reaper

all: $(EXEC)

run1: run1.c
	gcc $? -o $@ -Wall -Werror

lsproc: lsproc.c
	gcc $? -o $@ -Wall -Werror -lprocps 

sendsignal: sendsignal.c
	gcc $? -o $@ -Wall -Werror

sighandler: sighandler.c
	gcc $? -o $@ -Wall -Werror

zombie: zombie.c
	gcc $? -o $@ -Wall -Werror

reaper: reaper.c
	gcc $? -o $@ -Wall -Werror -lprocps 


.PHONY: clean

clean:
	rm -rf $(EXEC)
