#! /usr/bin/env python

import sys
import os

run_files = "python lhe_combine_vfns.py --files='condor_jobs/job_X/PROC_HO_0/P0_calc_0/output/samplessbarpsi1ccbar.lhe condor_jobs/job_X/PROC_HO_0/P0_calc_1/output/samplecgpsi1c.lhe condor_jobs/job_X/PROC_HO_0/P0_calc_2/output/sampledbardpsi1ccbar.lhe condor_jobs/job_X/PROC_HO_0/P0_calc_3/output/sampleggpsi1ccbar.lhe condor_jobs/job_X/PROC_HO_0/P0_calc_4/output/samplesbarspsi1ccbar.lhe condor_jobs/job_X/PROC_HO_0/P0_calc_5/output/sampleubarupsi1ccbar.lhe condor_jobs/job_X/PROC_HO_0/P0_calc_6/output/samplegcbarpsi1cbar.lhe condor_jobs/job_X/PROC_HO_0/P0_calc_7/output/sampleddbarpsi1ccbar.lhe condor_jobs/job_X/PROC_HO_0/P0_calc_8/output/sampleuubarpsi1ccbar.lhe condor_jobs/job_X/PROC_HO_0/P0_calc_9/output/samplegcpsi1c.lhe condor_jobs/job_X/PROC_HO_0/P0_calc_10/output/samplecbargpsi1cbar.lhe' --out='results.lhe'"
njobs = 100
list_jobs = []
for i in range(0, njobs):
    job = run_files.replace('job_X', 'job_' + str(i))
    list_jobs.append(job)

for jb in list_jobs:
    print(jb)

files=[arg for arg in sys.argv[1:] if arg.startswith('--files=')]
if not files:
    raise Exception, "The usage of it should be e.g., ./lhe_combine --files='/PATH/TO/file1.lhe /PATH/TO/file2.lhe /PATH/TO/file3.lhe' --out='combine.lhe' "
files=files[0]
files=files.replace('--files=','')
#files=[file.lower() for file in files.split(' ')]
files=[file for file in files.split(' ')]

outfile=[arg for arg in sys.argv[1:] if arg.startswith('--out=')]
if not outfile:
    outfile=['combine.lhe']

outfile=outfile[0]
outfile=outfile.replace('--out=','')

currentdir=os.getcwd()

offset=100

headers=[]
inits=[]
events=[]
firstinit=""

ilil=0
for i,file in enumerate(files):
    stream=open(file,'r')
    headQ=True
    initQ=False
    iinit=-1
    ievent=-1
    eventQ=False
    procid=None
    proc_dict={}
    for line in stream:
        sline=line.replace('\n','')
        if "<init>" in line or "<init " in line:
            initQ=True
            headQ=False
            iinit=iinit+1
            if i==0: inits.append(sline)
        elif headQ and i == 0:
            headers.append(sline)
        elif "</init>" in line or "</init " in line:
            initQ=False
            iinit=-1
            if i==0: inits.append(sline)
        elif initQ:
            iinit=iinit+1
            if iinit == 1:
                if i == 0:
                    firstinit=sline
                    inits.append(sline)
                else:
                    if not sline == firstinit:
                        raise Exception, "the beam information of the LHE files is not identical"
            elif iinit >= 2:
                procid=sline.split()[-1]
                procpos=sline.index(' '+procid)
                ilil=ilil+1
                sline=sline[:procpos]+(' %d'%(offset+ilil))
                proc_dict[procid]=offset+ilil
                if i == 0:
                    inits.append(sline)
                else:
                    inits.insert(-1,sline)
            else:
                raise Exception, "should not reach here. Do not understand the <init> block"
        elif "<event>" in line or "<event " in line:
            eventQ=True
            ievent=ievent+1
            events.append(sline)
        elif "</event>" in line or "</event " in line:
            eventQ=False
            ievent=-1
            events.append(sline)
        elif eventQ:
            ievent=ievent+1
            if ievent == 1:
                found=False
                for procid,new_procid in proc_dict.items():
                    if ' '+procid+' ' not in sline: continue
                    procpos=sline.index(' '+procid+' ')
                    found=True
                    sline=sline[:procpos]+(' %d'%(new_procid))+sline[procpos+len(' '+procid):]
                    break
                if not found: raise Exception, "do not find the correct proc id !"
                #procpos=sline.index(' '+procid)
                #sline=sline[:procpos]+(' %d'%(offset+1+i))+sline[procpos+len(' '+procid):]
            events.append(sline)
    stream.close()

text='\n'.join(headers)+'\n'
text=text+'\n'.join(inits)+'\n'
text=text+'\n'.join(events)

stream=open(outfile,'w')
stream.write(text)
stream.close()
print "INFO: The final merged lhe file is %s"%outfile
