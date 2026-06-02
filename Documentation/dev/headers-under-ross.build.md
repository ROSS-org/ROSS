**[Breaking]** Installed headers moved from flat `<prefix>/include/` to
`<prefix>/include/ross/` so common names (`config.h`, `buddy.h`,
`lz4.h`, `io.h`, etc.) no longer pollute the consumer include
namespace. The CMake exported target and `ross.pc` both point at the
new location automatically, so consumers using `find_package(ROSS)` +
`ROSS::ROSS` or `pkg_check_modules(ROSS ross)` see the change
transparently with no source edits. Consumers that hard-coded
`-I<prefix>/include` outside those mechanisms need to update to
`-I<prefix>/include/ross`.
