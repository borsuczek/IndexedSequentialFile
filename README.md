# IndexedSequentialFile
The project implements the index-sequential file organization method. The program enables adding a record, displaying records based on the key value, reading a record, and initiating both automatic and on-demand reorganization. Additionally, there is an option to display the data file content.

## Test file format
In the test file, consecutive operations can be specified, starting with the operation type (d - add a new record, o - read a record with a specified key, r – reorganization, w - display the file according to the content of the key, k – end the program), followed by operation arguments separated by spaces:
- `d [key] [set of 15 real numbers]`
- `o [key]`
- `w`, `r`, and `k` – have no arguments

## Files description
There are two files, one is "index.txt," which contains record keys at the beginning of each page and page numbers where they are located. The other is "data.txt," which contains records consisting of a key, a set of 15 real numbers, a pointer to the overflow location, and a flag indicating whether it is the first record on the page. It always contains the key -1 so that, given that keys are natural numbers, it is the smallest key. Reading and writing records operate through buffers accommodating 4 records each. During reorganization, a auxiliary file named "data2.txt" is used, which is deleted upon completion of reorganization.
