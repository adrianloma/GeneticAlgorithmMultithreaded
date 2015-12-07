# GeneticAlgorithmMultithreaded
Simple genetic algorithm that finds strings. Implemented using POSIX threads.

Compilation should be straight forward.

In MacOSX:

    g++ main.cpp -o output

The only other compiler I have is the GNU compiler v4.8

    gcc-4.8 main.cpp -lpthread -lstdc++



If any UNIX machine doesn't want to compile, try deleting the header file and the include. MacOSX doesn't support POSIX barriers and included is a simple but functional implementation, obtained from: https://github.com/ademakov/DarwinPthreadBarrier 


Genetic algorithm based on:
http://www-cs-students.stanford.edu/~jl/Essays/ga.html

