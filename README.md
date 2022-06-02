
# Lab 3: Encoder with Threads

## Introduction
Data compression is the process of encoding information using fewer bits than the original representation. Run-length encoding (RLE) is a simple yet effective compression algorithm: repeated data are stored as a single data and the count. In this lab, you will build a parallel run-length encoder called Not Your Usual ENCoder, or nyuenc for short.

## Run-length encoding
Run-length encoding (RLE) is quite simple. When you encounter n characters of the same type in a row, the encoder (nyuenc) will turn that into a single instance of the character followed by the count n.

For example, a file with the following contents:

```aaaaaabbbbbbbbba```

would be encoded (logically, as the numbers would be in binary format) as:

```a6b9a1```

Note that the exact format of the encoded file is important. You will store the character in ASCII and the count as a 1-byte unsigned integer in binary format. In other words, the output is actually:

```a\x06b\x09a\x01```

## parallel RLE
You will parallelize the encoding using POSIX threads. In particular, you will implement a thread pool for executing encoding tasks.

You should use mutexes, condition variables, or semaphores to realize proper synchronization among threads. Your code must be free of race conditions. You must not perform busy waiting, and you must not use ```sleep()```, ```usleep()```, or ```nanosleep()```.

Your nyuenc will take an optional command-line option ```-j jobs```, which specifies the number of worker threads. (If no such option is provided, it runs sequentially.)

For example:
```
$ time ./nyuenc file.txt > /dev/null
real    0m0.527s
user    0m0.475s
sys     0m0.233s
$ time ./nyuenc -j 3 file.txt > /dev/null
real    0m0.191s
user    0m0.443s
sys     0m0.179s
```
