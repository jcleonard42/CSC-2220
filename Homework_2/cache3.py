import subprocess

# Compiles Program
subprocess.run(["g++", "third.cpp", "-o", "test3", "-fopenmp"])

for x in range(1, 21): # Loops for number of threads

    # Sets command in each iteration
    command = ["./test3", str(2), str(x)]

    # Runs command with new argument
    subprocess.run(command)

    print("")
    