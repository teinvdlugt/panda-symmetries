#! /bin/bash
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=1
#SBATCH --partition=short
#SBATCH --time=12:00:00

../build/panda-symmetries --expand-and-reduce -i safe 3_LC1_vertex_classes_to_unpack > 4_LC_vertices
