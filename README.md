# MIT 6.828: Operating System Engineering lab / JOS

This is my implementation of the labs from [MIT's operating system graduate class 6.828](https://pdos.csail.mit.edu/6.828/2014/overview.html). (MIT gracefully offers their course material to the public).

The following is quoted from the website.

> The lab is split into 6 major parts that build on each other, culminating in a primitive operating system on which you can run simple commands through your own shell. We reserve the last lecture for you to demo your operating system to the rest of the class.
>
> The operating system you will build, called JOS, will have Unix-like functions (e.g., fork, exec), but is implemented in an exokernel style (i.e., the Unix functions are implemented mostly as user-level library instead of built-in to the kernel). The major parts of the JOS operating system are:
> * Booting
> * Memory management
> * User environments
>  * Preemptive multitasking
>  * File system, spawn, and shell 
>  * Network driver
>  * Open-ended project
>  
>We will provide skeleton code for pieces of JOS, but you will have to do all the hard work. 

# INSTALLATION

See the lab webpage for the full configuration. 

A simple way to try the project is to use a docker image.

   docker build -t jos .

It can be used with:

    docker run -v $(pwd):/to_build -it jos

This open a shell in an environement with the right compiler and qemu.

    make
    make qemu-nox

TODO: add gdb and qemu with X

  