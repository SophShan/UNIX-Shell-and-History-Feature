# UNIX-Shell-and-History-Feature

Designed a C program to serve as a shell interface that accepts user commands and then executes each command in a separate process (a child process created by forking the parent). The shell
interface gives the user a prompt, after which the user enters a command. The program runs continually until the user enters 'exit'. If '&' is put at the end of the command, the parent and child processes will run concurrently, instead of the parent process waiting for the child to finish execution before it resumes its own.

## The history feature

The history feature allows the user to access the 5 most recently entered commands. Entering the command 'history' will list the 5 most previously used commands. 

## Executing last used command

When the user enters '!!', the most recent command in history is executed. If there are no
commands in history, entering '!!' results in the message “No commands in history.”
