### patches
Contains the sets of patches to various codebases that I've needed to edit. All
stored here as patches to keep them centralised.

### scripts
* **docrunch.sh** - run from /usr/src to build the kernel. Can provide gcc or
  crunchcc as an argument to choose the compiler.
* **envs.sh** - source this (`. path/to/envs.sh`) to get environmental
  variables required by liballocs and the dependencies.
* **partialbuilds.sh** - functions to help with building a kernel which is
  partially crunched and non-crunched.
* **typessplitter** - useds to split a big types.c into multiple source files
  which will hopefully not break the linker. Invoked as `split.py <types.c
  source> <number to split to>`.

### conf
* **CRUNCHED** - kernel config with various options I needed added in. Should
  be linked like so: `ln -s
  /usr/local/src/part2-dissertation-code/configs/CRUNCHED
  /sys/amd64/conf/CRUNCHED`

### libcrunchk
My port of libcrunch runtime with tests, etc.
