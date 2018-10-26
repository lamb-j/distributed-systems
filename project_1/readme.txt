CIS630, Spring 2017, Term Project I, Due date: May 3, 2017

Your First and Last name: Jacob Lambert  
Your Student ID:i 951503818

What programming language did you use to write your code?
  C++

Does your program compile on ix (CIS department server)? 
  Yes, on ix-dev

How should we compile your program on ix? 
  Run "make" in part1 or part2 directory, on ix-dev. Warnings are from the outdated
	version of boost library on ix-dev.

Does your program run on ix? 
  Yes
  For part1,
    ./project1 edge_file rounds
  For part2,
    mpirun -np 2 ./project1 edge_file partition_file rounds
    mpirun -np 4 ./project1 edge_file partition_file rounds

Does your program calculate the credit values accurately? 
  Yes

Does your program have a limit for the number of nodes it can handle in the input graph? 
  No

How long does it take for your program with two partitions to read the Flickr input files, perform 5 round and write 
the output of each round in the output files on ix-dev?

  Three timed runs:
    16.004
    16.260
    15.314


Does your program run correctly with four partitions on ix-dev? 
  Yes

Does you program end gracefully after completing the specified number of rounds? 
  Yes
