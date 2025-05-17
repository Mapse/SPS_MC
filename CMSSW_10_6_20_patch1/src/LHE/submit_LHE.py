import os, subprocess, sys
import datetime

############################################################ To edit ############################################################

input_file = "jpsi_bbbar_30to50_3FS_4FS_SPS_2017_13TeV.txt" #Jpsi_20to40_Dstar_DPS_2016posVFP_13TeV.txt

############################################################ End editing ########################################################

today = datetime.date.today()

njobs = 1398 # Change for the wanted number of jobs
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

