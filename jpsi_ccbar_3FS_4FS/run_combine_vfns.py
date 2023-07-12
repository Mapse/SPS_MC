import os

if __name__ == '__main__':

    run_files = "python lhe_combine_vfns.py --files='condor_jobs/job_X/PROC_HO_0/P0_calc_0/output/samplessbarpsi1ccbar.lhe condor_jobs/job_X/PROC_HO_0/P0_calc_1/output/samplecgpsi1c.lhe condor_jobs/job_X/PROC_HO_0/P0_calc_2/output/sampledbardpsi1ccbar.lhe condor_jobs/job_X/PROC_HO_0/P0_calc_3/output/sampleggpsi1ccbar.lhe condor_jobs/job_X/PROC_HO_0/P0_calc_4/output/samplesbarspsi1ccbar.lhe condor_jobs/job_X/PROC_HO_0/P0_calc_5/output/sampleubarupsi1ccbar.lhe condor_jobs/job_X/PROC_HO_0/P0_calc_6/output/samplegcbarpsi1cbar.lhe condor_jobs/job_X/PROC_HO_0/P0_calc_7/output/sampleddbarpsi1ccbar.lhe condor_jobs/job_X/PROC_HO_0/P0_calc_8/output/sampleuubarpsi1ccbar.lhe condor_jobs/job_X/PROC_HO_0/P0_calc_9/output/samplegcpsi1c.lhe condor_jobs/job_X/PROC_HO_0/P0_calc_10/output/samplecbargpsi1cbar.lhe' --out='condor_jobs/job_X/PROC_HO_0/results/results.lhe'"
    njobs = 2300
    list_jobs = []
    for i in range(njobs, njobs+200):
        job = run_files.replace('job_X', 'job_' + str(i))
        list_jobs.append(job)

    for jo in list_jobs:
        #print(f'Mergin samples for: {jo[48:53]}')
        os.system(jo)
        #print(jo)
