#!/usr/bin/env python3

import subprocess, os, re

def atoi(text):
    return int(text) if text.isdigit() else text

def natural_keys(text):
    '''
    alist.sort(key=natural_keys) sorts in human order
    http://nedbatchelder.com/blog/200712/human_sorting.html
    (See Toothy's implementation in the comments)
    '''
    return [ atoi(c) for c in re.split(r'(\d+)', text) ]

def generate_path(mc, dataset,  crab_folder, n_folders):

    print(f'There are {n_folders} paths to process')
    cmds = [ 
    f'xrdfs k8s-redir.ultralight.org:1094 ls -u /store/group/uerj/mabarros/{mc}/{dataset}/{crab_folder}/{i:04}/' for i in range(n_folders)
    ]
    #print(cmds)

    cat = ""
    out_file = dataset + ".txt"
    ct = 1
    for i in cmds:
        print(f'Processing path {ct}')
        os.system(f"{i} > {i.split('/')[-2]}")
        cat += f" {i.split('/')[-2]}"
        ct += 1

    os.system(f"cat {cat} > {out_file}")
    os.system(f"rm -rf {cat}")
    file_list = open(out_file, 'r').readlines()

    for idx, f in enumerate(file_list):
        file_list[idx] = re.sub('transfer-\d*', 'redir', f)

    list(set(file_list))
    file_list.sort(key=natural_keys)

    final_list = []
    for i in file_list:
        if i not in final_list:
            final_list.append(i)

    with open(out_file, 'w') as f:
        for i in final_list:
            f.write(i)

if __name__ == '__main__':

    mc = ['CRAB_PrivateMC_RunII_UL_2017_SPS_3FS_ccbar']
    
    dataset = ['jpsi_ccbar_25to100_3FS_SPS_2017_13TeV']
    
    crab_folder = ['250518_130208']
    
    n_folders = [8]
    
    for m, d, c, n in zip(mc, dataset, crab_folder, n_folders):
        generate_path(m, d, c, n)     
