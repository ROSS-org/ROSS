**[Breaking]** Removed the `USE_DAMARIS` CMake option and its dependent build paths.
The Damaris/RISA in-situ visualization has been inert for years and is
slated for a future rewrite; the RISA submodule and the
`#cmakedefine USE_DAMARIS` site in `config.h.in` remain pending that
work.
