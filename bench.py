processes_list = [2, 4, 8, 16, 32]
array_sizes = [100, 100_00, 100_000, 100_000_000, 100_000_000_000]

count = 0
group = 1

with open(f"sbatch_benches_{group}.sh", "w") as sh:
    for processes in processes_list:
        for size in array_sizes:
            script_name = f"py_bench_{processes}_{size}.sh"
            with open(script_name, "w") as f:
                content = f"""#!/bin/bash
#SBATCH --time=01:30:00
#SBATCH --job-name python_test
#SBATCH --output=result_{processes}_{size}.txt
#SBATCH --mail-type=END
#SBATCH --mail-user=wuch6840@mylaurier.ca

cd $SCRATCH
module load gcc
module load openmpi
module load anaconda3

conda activate work

mpirun -n {processes} python3 test.py {size}
    """
                f.write(content)
            sh.write(f"sbatch {script_name}\n")
            count += 1
            if count == 12:
                count = 0
                group += 1
                sh.close()
                sh = open(f"sbatch_benches_{group}.sh", "w")
