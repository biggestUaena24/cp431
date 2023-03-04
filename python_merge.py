import numpy as np
import argparse
from mpi4py import MPI


def generate_sorted_array(lower, upper, n, dtype):
    return np.sort(np.random.randint(lower, upper, size=n, dtype=dtype))


def binary_search(arr, elem, start):
    first = start
    last = len(arr)
    mid = int((first + last)/2)

    while first <= last:
        if (mid == len(arr) or arr[mid] == elem):
            return mid
        elif arr[mid] < elem:
            first = mid + 1
        else:
            last = mid - 1

        mid = int((first + last)/2)

    return mid + 1


def merge(arr1, arr2):
    return np.sort(np.concatenate((arr1, arr2)), kind='merge')


def is_correct(A, B, merged):
    original = merge(A, B)
    return np.array_equal(original, merged)


parser = argparse.ArgumentParser(description="Parallel Merge")
parser.add_argument('size', type=int, help="Size of arrays")
parser.add_argument('-c', '--check', action="store_true",
                    help="Check for correctness")
args = parser.parse_args()

ARRAY_SIZE = args.size
MAX_INT = 10 * ARRAY_SIZE
MAX_PRINT = 50
CHECK_CORRECTNESS = args.check

SEED = 200869340
np.random.seed(SEED)

program_run_time = MPI.Wtime()
comm = MPI.COMM_WORLD
rank = comm.Get_rank()
size = comm.Get_size()

if rank == 0:
    array_generation_time = MPI.Wtime()
    A = generate_sorted_array(0, MAX_INT, ARRAY_SIZE, 'i')
    B = generate_sorted_array(0, MAX_INT, ARRAY_SIZE, 'i')
    array_generation_end_time = MPI.Wtime()

    partition_A = np.array_split(A, size - 1)

    for i in range(1, size):
        comm.send(len(partition_A[i-1]), dest=i, tag=0)
        comm.Send([partition_A[i-1], MPI.INT], dest=i, tag=1)

    partition_B_index = 0
    for i, partition in enumerate(partition_A):
        b_index = binary_search(B, partition[-1], partition_B_index)
        comm.send((b_index - partition_B_index), dest=i + 1, tag=2)
        comm.Send([B[partition_B_index:b_index],
                  MPI.INT], dest=i + 1, tag=3)
        partition_B_index = b_index

    merged_arrays = []
    merge_global_start_time = MPI.Wtime()
    for i in range(1, size):
        merged_size = comm.recv(source=i, tag=4)
        merged = np.empty(merged_size, dtype='i')
        comm.Recv([merged, MPI.INT], source=i, tag=5)
        merged_arrays.append(merged)

    merged_arrays = np.concatenate(merged_arrays, dtype='i')
    merge_global_end_time = MPI.Wtime()
    program_end_time = MPI.Wtime()

    print(f"Array size = {ARRAY_SIZE}")
    print(f"Number of processes = {size}")
    print(
        f"Time to generate arrays = {array_generation_end_time - array_generation_time: .4f} seconds")
    print(
        f"Time to merge all the arrays received = {merge_global_end_time - merge_global_start_time: .4f} seconds")
    print(
        f"Total time to run the program = {program_end_time - program_run_time: .4f} seconds")
    if CHECK_CORRECTNESS:
        print(
            f"Is the merged array correct? {is_correct(A, B, merged_arrays)}")

elif rank >= 1:
    merge_a_size = comm.recv(source=0, tag=0)
    merge_a = np.empty(merge_a_size, dtype='i')
    comm.Recv([merge_a, MPI.INT], source=0, tag=1)

    merge_b_size = comm.recv(source=0, tag=2)
    merge_b = np.empty(merge_b_size, dtype='i')
    comm.Recv([merge_b, MPI.INT], source=0, tag=3)

    merge_start_time = MPI.Wtime()
    merged = merge(merge_a, merge_b)
    merge_end_time = MPI.Wtime()

    print(
        f"Rank {rank}: {merge_end_time - merge_start_time: .8f}s")
    comm.send(len(merged), dest=0, tag=4)
    comm.Send([merged, MPI.INT], dest=0, tag=5)
