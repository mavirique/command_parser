# command_parser

A modern C++23 command-line tool for generating and verifying file or text hashes (MD5, SHA-1, SHA-256) using OpenSSL.  
Built with security, clarity, and best practices in mind.

## Features

- Hash any file or text with MD5, SHA-1, or SHA-256
- Verify the hash of a file or text against an expected value
- Modern C++23 code, modular, secure, and ready for extension

## Usage

**Hash a file:**

./command_parser sha256 --file myfile.txt

**Hash text:**

./command_parser md5 --text "hello world"

**Write hash to a file:**

./command_parser sha1 --file myfile.txt --output hash.txt

**Verify a file or text:**

./command_parser verify sha256 --file myfile.txt --expected "<hash>"
./command_parser verify md5 --text "hello world" --expected "<hash>"

## Building

You need a C++23 compiler and OpenSSL development libraries.

### With CMake (recommended):

git clone https://github.com/YOUR_GITHUB/command_parser.git
cd command_parser
mkdir build && cd build
cmake ..
cmake --build .

Or compile manually:

g++ -std=c++23 -Wall -Wextra -O2 src/\*.cpp -Iinclude -lssl -lcrypto -o command_parser

## Folder Structure

include/ # Public headers
src/ # Source files
test/ # (optional) Unit tests
CMakeLists.txt # Build system file

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE) for details.

---

## Contributing

Pull requests, issues, and suggestions are welcome!  
Feel free to fork and hack on your own version.

---

## Security Notes

- Always verify input files before using in production.
- Use latest OpenSSL version for maximum security.
