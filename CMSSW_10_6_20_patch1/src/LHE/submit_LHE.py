import os, subprocess, sys
import datetime

############################################################ To edit ############################################################

input_file = "jpsi_ccbar_25to100_3FS_SPS_2017_13TeV.txt" #Jpsi_20to40_Dstar_DPS_2016posVFP_13TeV.txt
njobs = 7785 # Change for the wanted number of jobs

############################################################ End editing ########################################################

today = datetime.date.today()

template = "crab_config_LHE"

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

