from operator import contains
import os, sys

job = sys.argv[1]

""" ind_ini = job.index('job_')
ind_fin = job.index('.txt')
jdir = job[ind_ini:ind_fin] """
path_jobs = '/afs/cern.ch/work/m/mabarros/public/MonteCarlo/SPS/condor_sps'
job = path_jobs + '/' + job
print(job)
seed_file = os.popen(f'cat {job}').read()
seed_file = seed_file.replace('\n', '')

with open(seed_file) as f:
    line = f.read()
    index_ini = line.index('seed = ') 
    index_fin = line.index('set pre') 
    current_seed = line[index_ini+7:index_fin-1]

job_name = 'job_' + current_seed

print(job_name)
os.system('mkdir ' + job_name)
# Change the directory to the job directory.
os.chdir(job_name)

files = '/afs/cern.ch/work/m/mabarros/public/MonteCarlo/SPS/condor_sps' + '/' + sys.argv[1]

with open(files) as f:
    ho_file = f.read()
    ho_file.replace('\n', '')

#print('/afs/cern.ch/work/m/mabarros/public/MonteCarlo/SPS/condor_sps/HELAC-Onia-2.7.1/ho_cluster' + ' < ' + ho_file)
os.system('pwd')
os.system('/afs/cern.ch/work/m/mabarros/public/MonteCarlo/SPS/condor_sps/HELAC-Onia-2.7.1/ho_cluster' + ' < ' + ho_file)

# To exclude sub-process
print('rm -rf ' + job_name + '/PROC_HO_0/P0_calc_*')
os.system('rm -rf '  + 'PROC_HO_*/P0_calc_*')


