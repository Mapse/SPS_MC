import os
existing_jobs_path = '/eos/user/m/mabarros/Monte_Carlo/SPS/jpsi_ccbar_3FS_4FS/condor_jobs_organized'

jobs_to_run_path = '/eos/user/m/mabarros/Monte_Carlo/SPS/jpsi_ccbar_3FS_4FS/fragments'

files_exi = []
with os.scandir(existing_jobs_path) as aux:
    for file in aux:
        if file.name.endswith('.lhe'):# and (file.stat().st_size != 0):
            in_lhe =  file.path.find('lhe')
            in_job = file.path.find('results_job_') 
            jn = file.path[in_job+12:in_lhe-1]
            files_exi.append(jn)


files_run = []
with os.scandir(jobs_to_run_path) as aux:
    for file in aux:
        if file.name.endswith('.ho'):# and (file.stat().st_size != 0):
            in_ho =  file.path.find('.ho')
            in_seed= file.path.find('seed_') 
            if not  file.path[in_seed+5:in_ho] in files_exi:
                files_run.append(file.path)
            else:
                print(f'Jobs already finished: {file.path}')


with open('jpsi_3FS_4FS_ccbar_pt90_rest_path.txt', 'w') as fp:
    for item in files_run:
        # write each item on a new line
        fp.write(item + '\n')
    

#################### To exclude the rest of the files
#print('Excluding    the rest of files...')
#path_bin = '/eos/user/m/mabarros/Monte_Carlo/SPS/jpsi_ccbar_3FS_4FS/condor_jobs'
#os.system('rm -rf ' + path_bin + '/job*')
