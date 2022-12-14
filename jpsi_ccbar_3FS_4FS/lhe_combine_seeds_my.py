#! /usr/bin/env python

import sys
import os
import math
import gc
import psutil

def combine_lhe_seeds(file_in):

    procxs={}
    procnevt={}
    
    for i, file in enumerate(file_in):         
        with open(file, 'r') as stream:  
            #stream=open(file,'r')
            initQ, eventQ, found, iinit, ievent=False, False, False, -1, -1
            for line in stream:
                sline=line.replace('\n','')
                if "<init>" in line or "<init " in line:
                    initQ=True
                    iinit=iinit+1
                elif "</init>" in line or "</init " in line:
                    initQ=False
                    iinit=-1
                elif initQ:
                    iinit=iinit+1
                    if iinit == 1:
                        if i == 0:
                            firstinit=sline
                        else:
                            if not sline == firstinit:
                                raise Exception("the beam information of the LHE files is not identical")
                    elif iinit >= 2:
                        procid=sline.split()[-1]
                        xs=float(sline.split()[0])
                        xserr=float(sline.split()[1])
                        if procid not in procxs:
                            procxs[procid]=[(xs,xserr)]
                            procnevt[procid]=0
                        else:
                            procxs[procid].append((xs,xserr))
                    else:
                        raise Exception("should not reach here. Do not understand the <init> block")
                elif "<event>" in line or "<event " in line:
                    eventQ=True
                    ievent=ievent+1
                elif "</event>" in line or "</event " in line:
                    eventQ=False
                    ievent=-1
                elif eventQ:
                    ievent=ievent+1
                    if ievent == 1:
                        found=False
                        for procid in procxs.keys():
                            if ' '+procid+' ' not in sline: continue
                            procnevt[procid]=procnevt[procid]+1
                            found=True
                            break
                        if not found: raise Exception("do not find the correct proc id !")
            del stream
            gc.collect()
        

    # evaluate the average of cross sections
    procwgts={}
    for procid in procxs.keys():
        allxs=procxs[procid]
        xss=[xs[0]*xs[1]**(-2) for xs in allxs]
        err=[xs[1]**(-2) for xs in allxs]
        new_err=1/math.sqrt(sum(err))
        new_xs=new_err**2*sum(xss)
        procxs[procid]=(new_xs,new_err)
        procwgts[procid]=new_xs/procnevt[procid]

    headers, inits, events,  firstinit = [], [], [],  ""
    inits=[]
    events=[]
    firstinit=""
    
    for i,file in enumerate(file_in):
        with open(file, 'r') as stream:
            #stream=open(file,'r')
            headQ, initQ, iinit, ievent, old_wgt, new_wgt, eventQ, rwgtQ, procid, new_evt=True, False, -1, -1, -1, -1, False, False, None, ''
            for line in stream:
                sline=line.replace('\n','')
                if "<init>" in line or "<init " in line:
                    initQ,headQ=True, False
                    iinit=iinit+1
                    if i==0: inits.append(sline)
                elif headQ and i == 0:
                    headers.append(sline)
                elif "</init>" in line or "</init " in line:
                    initQ,iinit=False,-1
                    if i==0: inits.append(sline)
                elif initQ:
                    iinit=iinit+1
                    if iinit == 1:
                        if i == 0:
                            firstinit=sline
                            inits.append(sline)
                        else:
                            if not sline == firstinit:
                                raise Exception("the beam information of the LHE files is not identical")
                    elif iinit >= 2:
                        if i == 0:
                            procid=sline.split()[-1]
                            xs,err=procxs[procid]
                            sline=sline.split()
                            sline[0]="%16.10E"%xs
                            sline[1]="%16.10E"%err
                            sline="    "+"    ".join(sline)
                            inits.append(sline)
                    else:
                        raise Exception("should not reach here. Do not understand the <init> block")
                elif "<event>" in line or "<event " in line:
                    eventQ=True
                    ievent=ievent+1
                    #events.append(sline)
                    new_evt = new_evt  + sline
                    new_evt = new_evt + '\n'
                elif "</event>" in line or "</event " in line:
                    eventQ, ievent, old_wgt, new_wgt=False, -1, -1, -1
                    #events.append(sline)
                    new_evt = new_evt + sline
                    new_evt = new_evt + '\n'
                elif eventQ:
                    ievent=ievent+1
                    if ievent == 1:
                        found=False
                        for procid in procxs.keys():
                            if ' '+procid+' ' not in sline: continue
                            try:
                                old_wgt=float(sline.split()[2])
                            except:
                                print(f'File corrupted: {file}')
                                print('Skipping...')
                                continue
                            new_wgt=procwgts[procid]
                            procpos=sline.index(' %12.6E '%old_wgt)
                            found=True
                            sline=sline[:procpos]+(' %12.6E'%(new_wgt))+sline[procpos+len(' %12.6E'%old_wgt):]
                            break
                        if not found: raise Exception("do not find the correct proc id !")
                        #procpos=sline.index(' '+procid)
                        #sline=sline[:procpos]+(' %d'%(offset+1+i))+sline[procpos+len(' '+procid):]
                    elif '<rwgt>' in line or '<rwgt ' in line:
                        rwgtQ=True
                    elif "</rwgt>" in line or "</rwgt " in line:
                        rwgtQ=False
                    elif rwgtQ:
                        procpos1=sline.index("'>")
                        procpos2=sline.index("</")
                        wgt=sline[(procpos1+2):procpos2]
                        wgt=float(wgt)
                        sline=sline[:(procpos1+2)]+(" %12.6E "%(wgt*new_wgt/old_wgt))+sline[procpos2:]

                    #events.append(sline)
                    new_evt = new_evt  + sline
                    new_evt = new_evt + '\n'
            
            """ if i < 97:
                pass
            elif (i > 96) & (i < 140): """
            print(f'File: {i+1}')
            text_opt = '\n'.join(headers)+'\n'
            text_opt = text_opt +'\n'.join(inits)+'\n'
            text_opt = " ".join([text_opt, new_evt])
            text_opt = " ".join([text_opt, '</LesHouchesEvents>'])
            final_file = " ".join(['lhe_final/file_' + str(i) + '.lhe'])
            #final_file = " ".join(['lhe_test_f/file_' + str(i) + '.lhe'])

            #file_out = 'merged_final/jpsi_ccbar_color_singlet_total.lhe' #lhe_test_f

            with open(final_file, 'w') as stream:
                stream.write(text_opt)
                #del stream
                #gc.collect()
            print(f'Memory used: {psutil.Process().memory_info().rss / (1024 * 1024)}') # Converts bytes in MB
            
            #stream=open(final_file,'w')
            #text_opt = text_opt + new_evt 
            #text_opt = text_opt + '</LesHouchesEvents>'
            #stream=open('lhe_final/file_' + str(i) + '.lhe','w')
            
            #stream.close()
        del stream
        gc.collect()
            

        """ text='\n'.join(headers)+'\n'
        text=text+'\n'.join(inits)+'\n'
        text=text+'\n'.join(events) """

        """ stream=open(file_out,'w')
        stream.write(text)
        stream.close()
        print "\n INFO: The final merged lhe file is %s"%file_out """


if __name__ == '__main__':

    import time
    from numpy import array, append

    # Model for the paths
    job_paths = '/eos/user/m/mabarros/Monte_Carlo/SPS/jpsi_ccbar_3FS_4FS/condor_jobs_organized/'    

    files_in = []
    array_in = array([])
    with os.scandir(job_paths) as aux:
        for file in aux:
            if file.name.endswith('.lhe') and (file.stat().st_size != 0):
                #files_in.append(file.path)
                array_in = append(array_in, file.path)

    # Loop over the file list
    print(f'Number of files to process: {len(array_in[:])}')

    tstart = time.time()

    print(f'Calculating average cross section and doing reweighting..')
    #print(array_in[3000:3010])
    combine_lhe_seeds(array_in[:]) 
    print(f'Finished in: {time.time() - tstart}')

############################# OLD #############
""" if __name__ == '__main__':

    # Number of jobs set on condor
    n = 3
    # Model for the paths
    model_path = '/eos/user/m/mabarros/Monte_Carlo/SPS/jpsi_ccbar_3s11/condor_jobs/job_X/PROC_HO_0/results/results.lhe'
    # Output file name
    file_out = 'merged_final/jpsi_ccbar_color_singlet_total.lhe'

    # List with files to be merged
    files_in = []
    # List with problematic files
    files_np = []
    # Loop over the file list
    print('Files to perform weighting:\n')
    for i in range(n):
        # Takes a specific job
        lhe_in = model_path.replace('X', str(i))
        # ls command to store exist status (0 means it worked, any other is a problema)
        a = os.system('ls ' + lhe_in)
        # If the file exists, save it  
        if a == 0:
            files_in.append(lhe_in)
        # If not, save the problematic files
        else:
            print('File does not exist: \n')
            files_np.append(lhe_in)
    print(f'Number of files to process: {len(files_in)}')

    if len(files_in) < 2:   
        raise Exception("You need to have at least 2 input files in order to do a combination. Please make sure you are looking to combine at least 2 files.")
    print('Applying weights and reorganizing LHE files, this might take a while...')
    combine_lhe_seeds(files_in, file_out)
 """
