import os, subprocess, sys
import datetime

############################################################ To edit ############################################################

input_file = "jpsi_ccbar_3FS_4FS_SPS_2017_13TeV.txt" #"jpsi_ccbar_3s11_SPS_2017_13TeV.txt" 
njobs = 6967 # Change for the wanted number of jobs
############################################################ End editing ########################################################

today = datetime.date.today()

template = "crab_config_DR"

for path in subprocess.check_output("ls paths/", shell=True).decode("utf-8").splitlines():
    if not (path.endswith(input_file)):
        continue
    dataset = path[0: path.find(".")]
    with open(template + ".py", 'r') as f:
        new_file = f.read().replace("DATASET", dataset)
        new_file = new_file.replace("DATE", today.strftime("%d-%m-%Y"))
        new_file = new_file.replace("FILE", path)
        new_file = new_file.replace("NJOBS", str(njobs))

        with open(template + "_" + dataset + ".py", 'w') as nf:
            nf.write(new_file)
    os.system("crab submit -c " + template + "_" + dataset + ".py")

