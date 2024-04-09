import subprocess

# Compiles Program
subprocess.run(["g++", "second.cpp", "-o", "test2", "-fopenmp"])

for x in range(1, 9): # Loops for number of threads

    print(f"printing with {x} threads")

    # Sets command in each isteration
    command = ["./test2", str(x)]

    # Runs command with new argument
    subprocess.run(command)

    print("")
    