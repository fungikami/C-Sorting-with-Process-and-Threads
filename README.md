# C-Sorting-with-Process-and-Threads
An application that sorts in ascending order the integers stored in files located in a directory hierarchy with processes and threads.

## How to run?
```
    make
    ./ordena \<root\> \<output file\>

    ./ordenaproc \<#Sorters\> \<#Mergers\> \<root\> \<output file\>
    
    ./ordenahilo \<#Sorters\> \<#Mergers\> \<root\> \<output file\>
```
- \<#Sorters\>: number of processes/threads that will order the integers.
- \<#Mergers\>: Number of processes/threads that will mix ordered sequences.
- \<root\>: root of the directory tree to be processed.
- \<output file\>: Name of the file where the ordered sequence with all the integers will remain.
