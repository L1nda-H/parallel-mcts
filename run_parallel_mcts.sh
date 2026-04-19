#!/bin/bash
#SBATCH -A m4341                     
#SBATCH -C cpu                       # Request CPU architecture
#SBATCH -q regular                   # Queue (QoS): use 'debug' for fast <30min tests, 'regular' for longer
#SBATCH -t 00:30:00                  
#SBATCH -N 1                         # Number of nodes requested
#SBATCH --ntasks-per-node=1          # Number of MPI tasks per node (1 for serial)
#SBATCH --job-name=go_serial       # The name that shows up in the queue
#SBATCH --output=slurm-%x-%j.out     # Where standard print statements go (%x=job_name, %j=job_id)

#OpenMP settings:
export OMP_NUM_THREADS=32
export OMP_PLACES=cores
export OMP_PROC_BIND=spread

#run the application:
echo "Starting MCTS Parallel..."

srun -n 1 ./go_mcts_parallel > parallel_game_output.txt

echo "Game finished!"