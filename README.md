# Implementation notes

- The shell receives input by reading the entire line into memory and then tokenizing based on spaces. This means that I set a large limit (16384) on the number of characters of the input. I realized that I could have implemented the read by looping over input and tokenizing input in chunks. This would have allowed me to read input of arbitrary length, with the number of tokens being the limiting factor (since the token pointers would need to be stored in a fixed-length array). I did not have time to change my implementation.

- When parsing tokens, the shell considers only spaces as token separarators. For example, `a |b` would not be read as `a | b`.

# References

I referenced K&R frequently concerning general C usage, as well as as documentation for basic I/O functions.

I referenced the online documenation for many of the functions used in this assignment: fork, read, exec, wait, dup2, pipe, chdir, and so on.

In addition, I superficially referenced several StackOverflow pages when I was researching usage of different commands (such as `strtol` when parsing the history offset). I found this resource useful when attempting to understand the behavior of newlines and EOF in input. I did not copy code directly.

I found this resource helpful for thinking about extended piping: http://web.cse.ohio-state.edu/~mamrak/CIS762/pipes_lab_notes.html

# Additional tests

```
$/bin/ls -lah | /bin/grep README | /usr/bin/wc -l
1
```