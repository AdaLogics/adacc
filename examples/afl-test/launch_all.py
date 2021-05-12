import sys
import os
import threading
import time
import shutil
import subprocess

afl_dir="/home/dav/code/symcc-fork-4/afl"
symcc_dir="/home/dav/code/symcc-fork-4/symcc_build_qsym"
#symcc_fuzzing_helper="/home/dav/.cargo/bin/symcc_fuzzing_helper"
symcc_fuzzing_helper="./symcc_fuzzing_helper"

def input_setup():
    if os.path.isdir("corpus"):
        shutil.rmtree("corpus")
    os.mkdir("corpus")
    with open("./corpus/seed1", "w+") as s1:
        s1.write("R"*100)


# Build all of the targets. This assumes only we have a symcc directory and
# also a AFL directory prepared.
def build_all(target):
    # Build the afl binary
    afl_binary = os.path.join(afl_dir, "afl-clang")
    afl_cmd = [afl_binary, target, "-o", "afl-compiled"]
    subprocess.check_call(afl_cmd)

    # Build the symcc binary
    symcc_binary = os.path.join(symcc_dir, "sym++")
    symcc_cmd=[symcc_binary, target, "-o", "symcc-compiled"]
    subprocess.check_call(symcc_cmd)


def afl_worker(afl_target):
    print("In afl thread")
    afl_fuzz = os.path.join(afl_dir, "afl-fuzz")
    afl_cmd = [afl_fuzz, 
               "-M", "afl-master", 
               "-i", "corpus", 
               "-o", "afl-out", 
               "-m", "none", 
               "--", afl_target, "@@"]
    subprocess.check_call(afl_cmd)


def afl_worker_secondary(afl_target):
    print("In afl thread")
    afl_fuzz = os.path.join(afl_dir, "afl-fuzz")
    afl_cmd = [afl_fuzz, 
               "-S", "afl-secondary", 
               "-i", "corpus", 
               "-o", "afl-out", 
               "-m", "none", 
               "--", afl_target, "@@"]
    subprocess.check_call(afl_cmd)


def symcc_worker(symcc_target):
    the_env = os.environ
    the_env['PATH'] = the_env['PATH'] + ":%s"%(afl_dir)
    symcc_cmd = [symcc_fuzzing_helper,
                 "-o", "afl-out", 
                 "-a", "afl-secondary", 
                 "-n", "symcc", 
                 "--", symcc_target, "@@"]

    #subprocess.check_call(symcc_cmd, env=the_env)
    subprocess.Popen(symcc_cmd, env=the_env)


def run_all(afl_target, symcc_target):
    print("Creating AFL thread")
    afl_thread = threading.Thread(target=afl_worker, args=(afl_target,))
    afl_thread.start()
    time.sleep(4)

    print("Launching secondary")
    afl_thread_secondary = threading.Thread(target=afl_worker_secondary, args=(afl_target,))
    afl_thread_secondary.start()
    time.sleep(35)

    print("Launching symcc")
    symcc_thread = threading.Thread(target=symcc_worker, args=(symcc_target,))
    symcc_thread.start()
    print("Launched")
    time.sleep(10)
    print("Thanks for now")


if __name__ == "__main__":
    input_setup()
    if len(sys.argv) == 3:
        afl_target = sys.argv[1]
        symcc_target = sys.argv[2]
        print("afl-target: %s"%(afl_target))
        print("symcc-target: %s"%(symcc_target))
        run_all(afl_target, symcc_target)
    else:
        build_all("./afl-test.c")
        run_all("./afl-compiled", "./symcc-compiled")
    
