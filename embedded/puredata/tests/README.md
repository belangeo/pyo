From this directory, in a POSIX shell (including MSYS2 on Windows), run:

```sh
cc -Wall -Wextra -Wno-cast-function-type -pthread $(pkg-config --cflags python3-embed) test_embedding.c -o test_embedding $(pkg-config --libs python3-embed)
./test_embedding
```

These checks require a 64-bit compiler and Python embedding headers/library.
They use a fake built-in `pyo` module to test full-width hexadecimal addresses,
missing and invalid attributes, failed imports, multiple interpreters, and
deletion followed by recreation. They do not test pyo's audio engine.

For a native Windows Python build, use that installation's `include` directory
and `libs/python314.lib` in place of the `pkg-config` flags, and define
`PYO_PYTHON_HOME` as a quoted C string containing the installation path.
Its `python314.dll` must be on the test executable's DLL search path.

Run the PATH discovery checks with any Python 3.9 or newer:

```sh
python -m unittest discover -s . -p test_find_python.py
```
