import os

'''

Simple script to find problematic lhe files

'''

exclude = False

# Model for the paths
model_path = '/eos/user/s/sfonseca/LHE_SPS/LHE_minpt20d0'

files_in = []
with os.scandir(model_path) as aux:
    for file in aux:
        if file.name.endswith('.lhe') and (file.stat().st_size != 0):
            files_in.append(file.path)

for f in files_in:
    with open(f, 'r') as file:  
        content = file.read()
        prob_file = []
        if "</LesHouchesEvents>" not in content:
            
            prob_file.append(f)
print(prob_file)
    



        