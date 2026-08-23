# libgmdata

This project builds as a reusable C library, and a separate Rust utility.

Build:

    make

Benchmark the threaded vs single-threaded parser:

    make bench MULTITHREAD=1
    ./bench/gmdata_benchmark ./path/to/data.win 10 1

    make bench MULTITHREAD=0
    ./bench/gmdata_benchmark ./path/to/data.win 10 1

Install the library and headers

    make install

By default this installs to `/usr/local`, producing:

- `/usr/local/lib/libgmdata.a`
- `/usr/local/bin/gmdataparser`
- `/usr/local/include/`

Public headers are kept under `include/` and are intended for downstream consumers.

## Difference from other parsers

While other projects such as Butterscotch and UndertaleModTool have their own parsers for GameMaker data.win files, this is a library that can be used in other C/C++ projects. Based on the Butterscotch parser, it has several improvements such as wider chunk coverage, better error handling and faster parsing via multithreading.

Through my benchmarks, I measured a **4.05x speed-up** when parsing DELTARUNE Chapter 5 using the multithreaded parser compared to the single-threaded parser, which is nearly identical to Butterscotch's.

## Chunk coverage

| Chunk | libgmdata | Butterscotch | UndertaleModTool |
| --- | --- | --- | --- |
| GEN8 | ✅ | ✅ | ✅ |
| TXTR | ✅ | ✅ | ✅ |
| AUDO | ✅ | ✅ | ✅ |
| OPTN | ✅ | ✅ | ✅ |
| LANG | ✅ | ✅ | ✅ |
| EXTN | ✅ | ✅ | ✅ |
| SOND | ✅ | ✅ | ✅ |
| AGRP | ✅ | ✅ | ✅ |
| SPRT | ✅ | ✅ | ✅ |
| BGND | ✅ | ✅ | ✅ |
| PATH | ✅ | ✅ | ✅ |
| SCPT | ✅ | ✅ | ✅ |
| GLOB | ✅ | ✅ | ✅ |
| GMEN | ✅ | ❌ | ✅ |
| SHDR | ✅ | ✅ | ✅ |
| FONT | ✅ | ✅ | ✅ |
| TMLN | ✅ | ✅ | ✅ |
| OBJT | ✅ | ✅ | ✅ |
| FEDS | ✅ | ❌ | ✅ |
| ACRV | ✅ | ✅ | ✅ |
| SEQN | ✅ | ❌ | ✅ |
| TAGS | ✅ | ❌ | ✅ |
| ROOM | ✅ | ✅ | ✅ |
| UILR | ✅ | ❌ | ✅ |
| DAFL | ✅ | ❌ | ✅ |
| EMBI | ✅ | ❌ | ✅ |
| PSEM | ✅ | ❌ | ✅ |
| PSYS | ✅ | ❌ | ✅ |
| TPAG | ✅ | ✅ | ✅ |
| TGIN | ✅ | ❌ | ✅ |
| CODE | ✅ | ✅ | ✅ |
| VARI | ✅ | ✅ | ✅ |
| FUNC | ✅ | ✅ | ✅ |
| FEAT | ✅ | ❌ | ✅ |
| STRG | ✅ | ✅ | ✅ |
| STAT | ✅ | ❌ | ❌ |

Thank you to [Butterscotch](https://github.com/ButterscotchRunner/Butterscotch) for data.win parsing code.