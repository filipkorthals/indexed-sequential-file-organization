# Indexed-sequential file organization

This project implements an indexed-sequential file organization which combines benefits of sequential and indexed file structures. It is designed to efficiently manage large datasets by storing records sequentially while using an index page for quick access to any part of the file. This approach is useful in applications requiring frequent updates, such as database management systems. 

## Key features

- Sequential storage - records are stored in sequential method.
- Index structure - an index is used to map keys to their physical locations, which helps with quick access to data.
- Reorganization process - frequent reorganization of the file improves efficiency by reducing the number of disk accesses.

## Additional benefits

That specific implementation allows you to track performance by displaying number of disk accesses after each operation. Besides, it supports both manual operations and reading multiple operations from a text file.

Implemented in C++ for efficiency.
