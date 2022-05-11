#!/usr/bin/env python
print "Merging files..."
#Credit to Hua-Sheng Shao, with a few tweaks by Mary Hadley 
import sys
import os



def combine_lhe(file_in, outfile):

    currentdir=os.getcwd()

    offset=100

    headers=[]
    inits=[]
    events=[]
    firstinit=""

    ilil=0
    for i,file in enumerate(file_in):
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
                    #sline=sline[:procpos]+(' %d'%(new_procid))+sline[procpos+len(' '+procid):]
                events.append(sline)
        stream.close()

    text='\n'.join(headers)+'\n'
    text=text+'\n'.join(inits)+'\n'
    text=text+'\n'.join(events)

    stream=open(outfile,'w')
    stream.write(text)
    stream.close()
    print "The final merged lhe files is %s"%outfile

if __name__ == '__main__':

    files_in = ['jpsi_charm_simple.lhe', 'jpsi_charm_simple_1.lhe']
    file_out = 'jpsi_charm_total.lhe'

    if len(files_in) < 2:
        raise Exception, "You need to have at least 2 input files in order to do a combination. Please make sure you are looking to combine at least 2 files."

    combine_lhe(files_in, file_out)