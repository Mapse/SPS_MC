#!/bin/bash

#set -e

#hostname
#date

echo "---------------Starting the processing---------------"

ls

#cd /afs/cern.ch/work/m/mabarros/public/MonteCarlo/SPS/condor_sps/

#python3 generate_seed.py 

#cd /afs/cern.ch/work/m/mabarros/public/MonteCarlo/SPS/condor_sps/HELAC-Onia-2.7.1

#./config


#cd /eos/user/m/mabarros/Monte_Carlo/SPS/SPS_MC/jpsi_ccbar_3FS_4FS/condor_jobs
#cd /eos/user/m/mabarros/Monte_Carlo/SPS/SPS_MC/jpsi_bbbar_3FS_4FS_VFNS/condor_jobs
#cd /eos/user/m/mabarros/Monte_Carlo/SPS/SPS_MC/jpsi_ccbar_bbbar_3FS/condor_jobs
cd /eos/user/s/sfonseca/LHE_SPS/jpsi_ccbar_bbbar_3FS/condor_jobs

python3 run_helac.py $1
#python test.py
