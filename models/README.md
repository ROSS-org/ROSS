# Welcome to Modeling!

ROSS ships with one bundled model, `phold`, used to benchmark ROSS performance on new systems. For other example models built against ROSS:

- [A Template Model](https://github.com/ROSS-org/template-model) — starting point for any new model
- [Bare-bones postal network of mailboxes](https://github.com/nmcglo/ROSS-Mail-Model) — written with the intent of teaching ROSS to beginners
- [A Game of Life example](https://github.com/helq/highlife-ross) — shows how to build a model where ROSS is linked as a library (opposed to building the model inside of ROSS' space)
- [(Deprecated) A Suite of Stable Models](https://github.com/ROSS-org/ROSS-Models) — older models that may need some tweaking before they can be run; they were written for legacy ROSS

## Building your own model

Develop your model as a standalone CMake project that consumes the installed ROSS package via `find_package`. A minimal `CMakeLists.txt` for a single-source-file model looks like this:

```cmake
cmake_minimum_required(VERSION 3.16)
project(my_model C)

find_package(ROSS REQUIRED)

add_executable(my_model my_model.c)
target_link_libraries(my_model PRIVATE ROSS::ROSS m)
```

Build it after installing ROSS, pointing CMake at the install prefix:

```
cmake -S . -B build -DCMAKE_PREFIX_PATH=<ross-install-prefix>
cmake --build build -j
```

For more details on writing the model code itself please check out ROSS' documentation page.

### If you have a model symlinked into this directory

The older workflow — drop your model's source into a separate repo, symlink it into ROSS's `models/`, rely on ROSS's build to pick it up — still works for now, but is deprecated and will be removed in a future release. Please migrate to the standalone `find_package(ROSS)` setup shown above.
