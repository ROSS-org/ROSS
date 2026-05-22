**[Breaking]** Removed the `ross-config` shell-script wrapper from the install tree
(`<prefix>/bin/ross-config`). Its `--cflags` / `--libs` / `--cc` /
`--ld` queries are redundant with `pkg-config ross` (which ROSS has
always shipped), and CODES was verified not to depend on it before
removal.
