import subprocess

# Compiles Program
subprocess.run(["g++", "first.cpp", "-o", "test1"])

for i in range(20000, 40001, 1000):

    vals = []
    
    for x in range(20):

        # Sets command in each isteration
        command = ["./test1", str(i)]
        # Runs command with new argument
        output = subprocess.check_output(command)

        # Sets output value to append to array
        new = output[6:]
        vals.append(float(new))

    avg = sum(vals) / 20
    avg = round(avg, 4)
    
    print(f"{i} {avg}")