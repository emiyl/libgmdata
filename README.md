# libgmdata

This project builds as a reusable C library plus a separate command-line executable.

Build everything:

    make

Build just the library:

    make lib

Install the library, headers, and CLI to a standard prefix:

    make install

By default this installs to `/usr/local`, producing:

- `/usr/local/lib/libgmdata.a`
- `/usr/local/bin/gmdataparser`
- `/usr/local/include/`

Public headers are kept under `include/` and are intended for downstream consumers.

Thank you to [Butterscotch](https://github.com/ButterscotchRunner/Butterscotch) for data.win parsing code.