Demos Commander (deco) is an text-mode file manager for Unix systems.

The main interface consists of two panels, which display the file system.
It is used in a similar way to many other programs run in the Unix shell.
Arrow keys control file selection, the insert key is used to select files
(but not directories), and the function keys perform operations such as
renaming, editing and copying files.

Sources are distributed freely under the terms of MIT license.

=== Build and Install ===

    git clone https://github.com/sergev/deco
    cd deco
    cmake -B build
    make -C build
    make -C build install
