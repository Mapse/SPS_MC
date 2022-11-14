import os, subprocess
import getpass
import json

condor_jobcodes = {
    "000": "submited",
    "001": "running",
    "002": "error_exe",
    "004": "evicted",
    "005": "finished",
    "009": "aborted",
    "010": "suspended",
    "011": "running",
    "012": "held",
    "013": "released"
}

def job_divider(name, n_files=1):
    os.system("mkdir -p " + name + "_jobs")
    os.system("rm -rf " + name + "jobs/*")

    file_list = open(name + "_path.txt").readlines()
    

    chunks = [file_list[x:x+n_files] for x in range(0, len(file_list), n_files)]

    for idx, c in enumerate(chunks):
        nf = open(name + "_jobs/job_" + str(idx) + ".txt", 'w')
        nf.writelines(c)
        nf.close()

    with open(name + "_list.txt", 'w') as f:
        for txt in subprocess.check_output("ls -v " + name + "_jobs/", shell=True).decode("utf-8").splitlines():
            f.write(name + "_jobs/" + txt + '\n')
            


def submit(name):
    #os.system("rm -rf seeds.dat")
    #os.system("touch seeds.dat")
    os.system("rm -rf jobs_"+ name +".jdl")
    os.system("mkdir -p " + name + "_logs")
    os.system("mkdir -p " + name + "_output")
    os.system("mkdir -p " + name + "_config")
    os.system("rm -rf " + name + "_logs/*")
    os.system("rm -rf " + name + "_config/*")
    os.system("cp submit.sh " + name + "_config/" + name + "_submit.sh")

    user = getpass.getuser()

    with open("jobs_template.jdl", 'r') as f:
        new_file = f.read().replace("EXEC", name + "_config/" + name + "_submit.sh")
        new_file = new_file.replace("FILE_LIST", name + "_config/" + name + "_list.txt")
        new_file = new_file.replace("LOGS", name + "_logs")
        new_file = new_file.replace("BATCH_NAME", name)
        new_file = new_file.replace("INPUT_FILES", name + "_jobs")
        #new_file = new_file.replace("NJOBS", str(njobs))

    with open(name + "_config/" + name  + "_jobs.jdl", 'w') as nf:
        nf.write(new_file)
    
    os.system("mv " + name + "_list.txt " + name + "_config/.")
    out = subprocess.check_output("condor_submit " + name + "_config/" + name + "_jobs.jdl", shell=True).decode("utf-8").splitlines()[1]

    out_split = out.split()
    if out_split[0].isnumeric():
        n_jobs = int(out_split[0])
    else:
        raise("Problem in job submition: " + out)

    if out_split[-1][0:-1].isnumeric():
        cluster_id = out_split[-1][0:-1]
    else:
        raise("Problem in job submition: " + out)

    config = {
            'name': name,
            'n_jobs': n_jobs,
            'cluster_id': cluster_id
            }

    print(out)
    with open(name + "_config/config.json", 'w') as outfile:
        json.dump(config, outfile)


def check(name):
    with open(name + "_config/config.json") as config_file:
        config = json.load(config_file)
    #os.system("condor_q " + config['cluster_id'])
    
    summary = {}
    files = subprocess.check_output("ls -d " + name + "_logs/*.log", shell=True).decode("utf-8").splitlines()
    for log_file in files:
        with open(log_file) as f:
            log = f.read().splitlines()
            job_codes = []
            job_id = log_file.split("_")[-1].split(".")[0]
            for l in log:
                value = l.split()[0]
                if (value.isnumeric()) and (len(value) == 3) and (value in condor_jobcodes.keys()):
                    job_codes.append(value)
                
            summary[job_id] = job_codes
    
    final = []
    for i in summary.keys():
        final.append(summary[i][-1])

    print(f"---------- Summary ({name}, {config['cluster_id']}) ----------")
    print(f"Running:            {final.count('001') + final.count('011')}/{config['n_jobs']}")
    print(f"Finished:           {final.count('005')}/{config['n_jobs']}")
    print(f"Held:               {final.count('012')}/{config['n_jobs']}")
    print(f"Aborted/Error/Idle: {config['n_jobs'] - (final.count('001') + final.count('011') + final.count('005') + final.count('012'))}/{config['n_jobs']}")
            

def collect(name):
    #print (f"Collecting files: {name}*.coffea")
    #print("mv " + name + "*.coffea " + name + "_output/.")
    os.system("mv " + "*.coffea " + name + "_output/.")
    os.system("gfal-copy -rf " + name + "_output gsiftp://gridftp.hepgrid.uerj.br/mnt/hadoop/cms/store/user/mabarros/" + name + "_output/")


if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser(description="Submit jobs via condor")
    parser.add_argument("-n", "--name", help="batch name of jobs", type=str, required=True)
    #parser.add_argument("-nj", "--njobs", help="Number of jobs to be submitted", type=int, required=True)
    parser.add_argument("-s", "--submit", help="Submit jobs", action="store_true")
    parser.add_argument("-c","--check", help="Check jobs", action="store_true")
    parser.add_argument("-cl","--collect", help="Collect jobs to output dir", action="store_true")
    args = parser.parse_args()

    if args.submit:
        #print ("Dividing jobs...")
        job_divider(args.name)
        #print ("Submiting jobs...")
        submit(args.name)

    if args.check:
        check(args.name)

    if args.collect:
        collect(args.name)
