import numpy as np
import matplotlib.pyplot as plt
import subprocess
import sys
from random import randrange
import statistics

RUN_BENCHMARKS=False
MIN_LENGTH=1
MAX_LENGTH=20
TEST_REPETITIONS=30
BENCH_REPETITIONS=1

def random_matrix(rows, cols):
  return np.random.randint(-1000, high=1000, size=(rows, cols))

def mat_to_str(mat, dim):

  if(dim == "both"):
    dim_str = "{}:{}".format(mat.shape[0], mat.shape[1])
  elif(dim == "rows"):
    dim_str = str(mat.shape[0])
  elif(dim == "cols"):
    dim_str = str(mat.shape[1])
  else:
    raise "Wrong dim"

  return dim_str + "\n" + "\n".join([" ".join(str(item) for item in row) for row in mat]) + "\n"

def print_mat(mat, dim="both", file=sys.stdout):
  
  print(mat_to_str(mat, dim), file=file) 

def run_random_tests():

  failed = 0

  for i in range(0, TEST_REPETITIONS):

      rows    = randrange(MAX_LENGTH - MIN_LENGTH + 1) + MIN_LENGTH
      cols    = randrange(MAX_LENGTH - MIN_LENGTH + 1) + MIN_LENGTH
      shared  = randrange(min(rows, cols)) + 1

      print("RUNNING TEST: {}/{}, r|{}|c|{}|s|{}".format(i + 1, TEST_REPETITIONS, rows, cols, shared))

      mat1    = random_matrix(rows, shared)
      mat2    = random_matrix(shared, cols)

      with open("mat1", "w") as f:
        print_mat(mat1, "rows", f)

      with open("mat2", "w") as f:
        print_mat(mat2, "cols", f)

      verified = np.matmul(mat1, mat2)
      verified_str = mat_to_str(verified, "both")
   
      command = "mpirun --prefix /usr/local/share/OpenMPI --oversubscribe -np {} mm".format(rows * cols)
      stdout = subprocess.Popen(command, shell=True, stdout=subprocess.PIPE).communicate()[0].decode("utf-8")
      
      if(stdout != verified_str):

        print("TEST {} FAILED:".format(i + 1))
        print(stdout)
        print(verified_str)
        failed += 1

  if(failed == 0):
    print("ALL TESTS PASSED")
  else:
    print("FAILED {} TESTS OUT OF {}".format(failed, TEST_REPETITIONS))

def run_benchmarks():

  cpus  = []
  times = []

  for dim in range(MIN_LENGTH, MAX_LENGTH + 1):
    time_total = 0
    for _ in range(0, BENCH_REPETITIONS):

      mat1    = random_matrix(dim, dim)
      mat2    = random_matrix(dim, dim)

      with open("mat1", "w") as f:
        print_mat(mat1, "rows", f)

      with open("mat2", "w") as f:
        print_mat(mat2, "cols", f)
  
      command = "mpirun --prefix /usr/local/share/OpenMPI --oversubscribe -np {} mm".format(dim * dim)
    
      stdout = subprocess.Popen(command, shell=True, stdout=subprocess.PIPE).communicate()[0].decode("utf-8")
    
      time_total += float(stdout)
    
    time_total = (time_total / BENCH_REPETITIONS) * 1000

    cpus.append(dim * dim)
    times.append(time_total)

    print("BENCH RESULT: {:.2f}us for {}x{} matrix".format(time_total, dim, dim))

  return (cpus, times)
  
def main():

  if(RUN_BENCHMARKS):

    (x, y) = run_benchmarks()
    maxx = max(x)
    maxy = max(y) 
    m, b = np.polyfit(x, y, 1)
    plt.grid(True)
    plt.plot([0, maxx], [0, maxy], color="black", linestyle="dashed")
    plt.annotate("linear", (maxx, maxy), color="black", xytext=(-30, 0), textcoords="offset points")
    mean = statistics.mean(y)
    plt.plot([0, maxx], [mean, mean], color="red", linestyle="dashed")
    plt.annotate("mean: {:.0f}".format(mean), (maxx, mean), xytext=(0, 10), textcoords="offset points", color="red")
    plt.plot(x, y, color="blue")
    plt.scatter(x, y, color="blue")

    ypredict = m * np.array(x) + b
    plt.annotate("prediction", (x[-1], ypredict[-1]), xytext=(0, -15), textcoords="offset points", color="blue")
    plt.plot(x, ypredict, color="blue", linestyle="dashed")

    # # switcher = 1
    # switcher = True
    # for px, py in zip(x, y):
      
    #   # plt.annotate("{:.0f}".format(py), (px, py), xytext=(0,  switcher * 12), textcoords="offset points", color="blue")
    #   if(switcher):
    #     plt.annotate("{:.0f}".format(py), (px, py), xytext=(0,  -12), textcoords="offset points", color="blue")
    #   # switcher *= -1
    #   switcher = not switcher

    plt.ylabel("microseconds")
    plt.xlabel("cpus")
    plt.savefig("bench.png", bbox_inches="tight")
  
  else:
    run_random_tests()

if __name__ == "__main__":
  main()