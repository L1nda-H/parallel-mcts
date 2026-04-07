#!/bin/bash
#SBATCH -A m4341                     
#SBATCH -C cpu                       # Request CPU architecture
#SBATCH -q regular                   # Queue (QoS): use 'debug' for fast <30min tests, 'regular' for longer
#SBATCH -t 00:30:00                  
#SBATCH -N 1                         # Number of nodes requested
#SBATCH --ntasks-per-node=1          # Number of MPI tasks per node (1 for serial)
#SBATCH --job-name=go_serial       # The name that shows up in the queue
#SBATCH --output=slurm-%x-%j.out     # Where standard print statements go (%x=job_name, %j=job_id)

export OMP_NUM_THREADS=1

echo "Starting MCTS Serial Baseline..."

# Run executable
srun ./go_mcts > game_output.txt

echo "Game finished!"