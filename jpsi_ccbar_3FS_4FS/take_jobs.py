
import os
import numpy as np


def take_lhe(jobs_model_path, transfer_model_path, range_f):
    
    # Number of jobs send to the grid on condor
    # Model for the paths
    

    # List with files to be merged
    files_in = []
    # List with problematic files
    files_np = []
    # Loop over the file list
    print('Files to perform weighting:\n')
    for i in range(range_f[0], range_f[1]):
        # Takes a specific job
        lhe_in = jobs_model_path.replace('X', str(i))
        # ls command to store exist status (0 means it worked, any other is a problema)
        a = os.system('ls ' + lhe_in)
        # If the file exists, save it  
        if a == 0:
            files_in.append(lhe_in)
        # If not, save the problematic files
        else:
            print('File does not exist: \n')
            files_np.append(lhe_in)

    #print(f'Number of files to process: {len(files_in)}')

    for i in files_in:
        #print(i)
        ind_ini = i.index('job_')
        ind_fin = i.index('/PROC')
        #jdir = i[ind_ini:ind_fin]
        job_name = i[ind_ini:ind_fin]
        new_f = transfer_model_path + '/' + 'results_' + job_name + '.lhe'
        print(new_f)
        os.system(f'mv {i} {new_f}')
        #print(f'mv {i} {new_f}')
    return len(files_in)


if __name__ == '__main__':

    #################### Transfers the good results.lhe to a dedicated folder

    transfer_model_path = '/eos/user/m/mabarros/Monte_Carlo/SPS/jpsi_ccbar_3FS_4FS/condor_jobs_organized/'
    range_f = [4092, 4211] #3000 to 3500: with comment in __init__ program

    jobs_model_path = '/afs/cern.ch/work/m/mabarros/public/MonteCarlo/SPS/condor_sps/condor_jobs/job_X/PROC_HO_0/results/results.lhe'
    l0 = take_lhe(jobs_model_path, transfer_model_path, range_f)

    jobs_model_path = '/afs/cern.ch/work/m/mabarros/public/MonteCarlo/SPS/condor_sps/condor_jobs/job_X/PROC_HO_1/results/results.lhe'
    l1 = take_lhe(jobs_model_path, transfer_model_path, range_f)

    jobs_model_path = '/afs/cern.ch/work/m/mabarros/public/MonteCarlo/SPS/condor_sps/condor_jobs/job_X/PROC_HO_2/results/results.lhe'
    l2 = take_lhe(jobs_model_path, transfer_model_path, range_f)

    jobs_model_path = '/afs/cern.ch/work/m/mabarros/public/MonteCarlo/SPS/condor_sps/condor_jobs/job_X/PROC_HO_3/results/results.lhe'
    l3 = take_lhe(jobs_model_path, transfer_model_path, range_f)

    jobs_model_path = '/afs/cern.ch/work/m/mabarros/public/MonteCarlo/SPS/condor_sps/condor_jobs/job_X/PROC_HO_4/results/results.lhe'
    l4 = take_lhe(jobs_model_path, transfer_model_path, range_f)

    jobs_model_path = '/afs/cern.ch/work/m/mabarros/public/MonteCarlo/SPS/condor_sps/condor_jobs/job_X/PROC_HO_5/results/results.lhe'
    l5 = take_lhe(jobs_model_path, transfer_model_path, range_f)

    jobs_model_path = '/afs/cern.ch/work/m/mabarros/public/MonteCarlo/SPS/condor_sps/condor_jobs/job_X/PROC_HO_6/results/results.lhe'
    l6 = take_lhe(jobs_model_path, transfer_model_path, range_f)

    jobs_model_path = '/afs/cern.ch/work/m/mabarros/public/MonteCarlo/SPS/condor_sps/condor_jobs/job_X/PROC_HO_7/results/results.lhe'
    l7 = take_lhe(jobs_model_path, transfer_model_path, range_f)

    jobs_model_path = '/afs/cern.ch/work/m/mabarros/public/MonteCarlo/SPS/condor_sps/condor_jobs/job_X/PROC_HO_8/results/results.lhe'
    l8 = take_lhe(jobs_model_path, transfer_model_path, range_f)

    print(f'The number of good files is: {l0+l1+l2+l3+l4+l5+l6+l7+l8}')
    #print(f'The number of good files is: {l0+l1+l2}')

    #################### To exclude the rest of the files
    #print('Excluding the rest of files...')
    #path_bin = '/eos/user/m/mabarros/Monte_Carlo/SPS/jpsi_ccbar_3FS_4FS/condor_jobs'
    #os.system('rm -rf ' + path_bin + '/job*')

#splits = np.array_split(files_in, n_size)

""" array_new_name 
for array in splits:
    for f in array:

        ind_ini = f.index('job_')
        ind_fin = f.index('/PROC')
        jdir = f[ind_ini:ind_fin]
        job_name = f[ind_ini:ind_fin]
        new_f = f.replace('results.lhe', 'results_' + job_name + '.lhe')
        

print(array) """


""" fold = 0
for array in splits:
    os.system(f'gfal-rm -r gsiftp://transfer-lb.ultralight.org//storage/cms/store/group/uerj/mabarros/LHE/jpsi_ccbar_3s11/work_{fold}')
    os.system(f'gfal-mkdir gsiftp://transfer-lb.ultralight.org//storage/cms/store/group/uerj/mabarros/LHE/jpsi_ccbar_3s11/work_{fold}')
    for f in array:

        ind_ini = f.index('job_')
        ind_fin = f.index('/PROC')
        jdir = f[ind_ini:ind_fin]
        job_name = f[ind_ini:ind_fin]
        new_f = f.replace('results.lhe', 'results_' + job_name + '.lhe')
        new_f = 'results_' + job_name + '.lhe'

        #print(f'old file: {f}')
        #print(f'Work: {fold}')
        #print(f'File: {new_f}')

        os.system(f'gfal-copy {f} gsiftp://transfer-lb.ultralight.org//storage/cms/store/group/uerj/mabarros/LHE/jpsi_ccbar_3s11/work_{fold}/{new_f}')

    fold = fold + 1
    #print(array) """



'gfal-rm -r gsiftp://transfer-lb.ultralight.org//storage/cms/store/group/uerj/mabarros/LHE/jpsi_ccbar_3s11/work_0'


