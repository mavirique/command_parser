# command_parser

A secure, modern C++23 command-line hash and verification tool for cybersecurity.

---

## Features

- **Hash text or files** using MD5, SHA1, or SHA256
- **Verify hashes** for files or text against expected values
- **Short, UNIX-style CLI flags:** `-h`, `-t`, `-f`, `-o`, `-v`, `-e`
- **Secure, modern C++23 code** with strong error handling
- **Refuses duplicate or unknown arguments**
- **Safe file handling:** only user-owned, non-symlinked files can be hashed/verified

---

## Build

**Requirements:**

- C++23 compiler (GCC 13+, Clang 16+, MSVC 2022+)
- [OpenSSL](https://www.openssl.org/) (dev package, e.g. `libssl-dev` on Linux)
- CMake 3.20+

**Build steps:**

```
git clone https://github.com/mavirique/command_parser.git
cd command_parser
mkdir build && cd build
cmake ..
cmake --build .
```

## Usage

```
command_parser -h <algo> -t <text> # Hash text
command_parser -h <algo> -f <file> # Hash file
command_parser -h <algo> -t <text> -o <out> # Hash text, write output
command_parser -h <algo> -f <file> -o <out> # Hash file, write output

command_parser -v -h <algo> -t <text> -e <hash> # Verify text
command_parser -v -h <algo> -f <file> -e <hash> # Verify file

command_parser --help
command_parser -H # Show usage
```

## Flags

Flag Purpose

```
-h <algo> Hash algorithm: md5, sha1, sha256
-t <text> Text to hash
-f <file> File to hash
-o <out> Output file (optional, for hash command)
-v Verification mode
-e <hash> Expected hash (required for verification)
--help Show usage
-H Show usage
```

Only one of -t or -f may be specified per command.
Duplicate or unknown flags are rejected with an error.
Output: Prints hash/OK/FAIL to stdout, or writes to file with -o.

## Examples

**Hash a file:**

```
command_parser -h sha256 -f myfile.txt
```

Hash text and write to file:

```
command_parser -h md5 -t "hello world" -o hash.txt
```

**Verify a file’s hash:**

```
command_parser -v -h sha256 -f myfile.txt -e 4d186321c1a7f0f354b297e8914ab240
```

## Security Notes

Refuses to hash or verify files not owned by the user.

Refuses to hash or verify symlinked files.

All command-line arguments are validated and parsed safely using modern C++23 practices.

## Development & Contributing

Code is modern C++23 and clang-tidy clean.

Pull requests for more algorithms, CLI improvements, or security reviews are welcome!

To test or audit, see the source for unit-testable logic in cli_parse.cpp and hash_util.cpp.

## License

```
MIT
Built by cybersecurity enthusiasts, for cybersecurity practitioners.
```
