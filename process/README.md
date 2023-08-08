# COMP3300/COMP6330 - Lab 02

In this lab, we are going to learn how to interact with processes in linux using process APIs. 
Some of the exercises here ask you to use the `procps` library, which provides an interface to make it easier to query the `/proc` files. If you have not installed it, install this library as follows:

```bash
$ sudo apt install libprocps-dev
```

Use the provided template C files and the Makefile to compile your programs (simply type `make`). 

## Process creation (`run1.c`)

This exercise is for you to familiarise yourself with the system calls `fork()`, `exec()` and `wait()`. 

Write a program `run1.c` takes as argument(s) the name of another program (let's call it the target program) and its expected arguments, and execute the target program, passing on the arguments specified in the command line to the target program. Use a combination of `fork` and `exec` to create a child process to run the target program. The parent process must wait for the child process, and it must also check the status of the child process when it exits. 
Read the manual page for `wait()` (e.g., using the command `man wait`) to see an example of how to check the exit status of the child process (using the WEXITSTATUS macro).

Here are some examples of how your program `run1` should behave. (
_Note: we don't have any automated scripts to check the output of your program, so feel free to use your own messages in response to various exit status of the child process, so you don't need to print out exactly the messages in the examples below._) 


- A normal run, with no errors encounted in the child process and it exited normally: 
    
    ```bash
    $ ./run1 ls -al /etc/passwd
    -rw-r--r-- 1 root root 3016 Dec  5  2022 /etc/passwd

    ****
    run1 (parent): child process terminated with status 0
    ```

- The target program encountered some errors (a `permission denied` in this case). 

    ```bash 
    $ ./run1 cat /etc/shadow
    cat: /etc/shadow: Permission denied

    ****
    run1 (parent): child process terminated with status 1
    ```

- The target program does not exist: 

    ```bash 
    $ ./run1 a_non_existent_program

    ****
    run1 (child): execvp error

    ****
    run1 (parent): child process terminated with status 1
    ```



## Sending signals (`sendsignal.c`)

For this exercise, we will use the `kill()` syscall to implement a program (`sendsignal.c`) that sends a signal to a process. This is essentially a simplified version of the linux `kill` command. Make sure to catch the error to see if the process is killed or not. Your program takes two arguments: the first one is the signal to send (between 1 - 31) and the second one is the process id to send the signal to. 

To test your program, you need to open up two terminals (or use a terminal multiplexer program like `tmux` if you'd like). In one terminal, run a program that will be the recipient of the signal, and in the other terminal, run your program `sendsignal.` 

For example, let's run a `wc` program, in one terminal, without specifying any input file, so it will wait for the user input on the terminal. Then switch to the other terminal, and use the `ps` command to find the process id of the `wc` process running in the first terminal. Let's say that the process id is `67234`. Then to send a SIGTERM signal to that process, run: 

```bash
$ ./sendsignal 15 67234
```

To see the numerical values of various signals, see the man page for signals (`man 7 signal`). 

Note that Linux access control policies restrict who can send signals to a process. For example, a normal user would not be able to send a SIGTERM signal to a process owned by the `root` user (as doing so would create all sorts of security vulnerabilites). So make sure that your program anticipates that a signal may fail to be delivered to the target process. If the syscall `kill()` returns -1, indicating an error has occurred, the actual error number is stored in a global variable called `errno`, which you can access by including the header file `errno.h` from libc (use `#include <errno.h>`). See the man page for errno (`man errno`).



## Process listing (`lsproc.c`)

For this exercise, you don't have to write any program, but you are asked to read the provided program `lsproc.c`. This program uses the `procps` library to query the `/proc/` directory to list all the processes there. The output of the program shows, in each row, the process id, the (real) username, the process id, its state, and the command it executes. 

To use the `procps` library, include relevant header files from `/usr/include/proc`, e.g., use `#include <proc/readproc.h>` to utilise functions from `/usr/include/proc/readproc.h`. To compile the program that uses `procps` library, add the option `-lprocps` in your compilation command (see the included `Makefile` for an example). 

The C header files in `/usr/include/proc/` will be useful for you to learn what functions are available in the `procps` library. Use the `man` command to find more information about a particular function, e.g., `man openproc`. Generally, to read information about the `/proc` directory, one first creates a PROCTAB object, using the `openproc()` function. This PROCTAB object specifies what kinds of information we want to query. Then the PROCTAB object is used as an argument in the `readproc()` function that selectively retrieves information from `/proc` based on what we specify in the PROCTAB object. The structure that holds various information related to a process is `proc_t`, which is defined in `/usr/include/proc/readproc.h`. 

Once you understand how `openproc` and `readproc` work, proceed to the next exercise. 

## Killing zombies (`reaper.c`)

Write a program (`reaper.c`) to find and kill all the zombie processes in the system.  
Before you start killing zombies, you would need to create some first. Use the provided program (`zombie.c`) to start a bunch of processes in the background that would become zombies, e.g., 

```bash
$ ./zombie & ./zombie &
```

The `zombie` program, when launched, forks a child process that terminates almost immediately, but the parent process does not execute a `wait()` syscall, and enters a sleep state for 200 seconds. After the parent process terminates the `zombie` child process will be adopted by the `init` process and eventually be terminated. But in the case that the parent process does not terminate (e.g., it is a server process meant to run continuously), it may end up creating a large number of zombies that take up system resources, so a program like `reaper` would be needed to free up the resources taken up by zombies.   

You may notice that if you try to directly kill a zombie process (e.g., by sending a SIGTERM or even a SIGKILL) while its parent is still running, the signal will do nothing. You should instead send the signal to its parent, causing its parent to terminate and kick start the adoption (and eventual termination) of the zombie process by the `init` process. 

Here is an example of a successful run of `reaper`: 

```bash
$ ./zombie & ./zombie & ./zombie &
[1] 80906
[2] 80907
[3] 80908
hello world (pid:80906)
hello, I am parent of 80909 (pid:80906)
hello, I am child (pid:80909)
hello world (pid:80907)
hello, I am parent of 80911 (pid:80907)
hello, I am child (pid:80911)
hello world (pid:80908)
hello, I am parent of 80912 (pid:80908)
hello, I am child (pid:80912)
 
$ ./reaper 

Found zombie process 80909: zombie
Sending SIGTERM to parent process 80906
SIGTERM sent to parent process 80906

Found zombie process 80911: zombie
Sending SIGTERM to parent process 80907
SIGTERM sent to parent process 80907

Found zombie process 80912: zombie
Sending SIGTERM to parent process 80908
SIGTERM sent to parent process 80908
[1]   Terminated              ./zombie
[2]-  Terminated              ./zombie
[3]+  Terminated              ./zombie
$ 
```
## Handling signals (`sighandler.c`)

Write a program to print a simple message and wait for a signal to be sent to it. The program should handle all the signals it is allowed to handle, and print a text describing the signal received. The user types in `exit` to exit the program. For example, when run, the program should print: 

```bash
./sighandler 
Hello, send me a signal. My process id is 76726
Type exit to quit this program at anytime.
(waiting for signal) > 
```

If the user subsequently sends, e.g., a SIGINT signal (either through a Ctrl-C on the terminal or using the `kill` command), the program would print the signal number and continues waiting for signals: 

```
(waiting for signal) > Signal 2 received
```


