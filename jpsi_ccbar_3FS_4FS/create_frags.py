	
model = 'generate_Jpsi_3FS_4FS_ccbar_model.ho'
path_new = 'fragments/'

nrun = 10
for i in range(0, nrun):
    #print(i)

    with open(model, 'r') as f:
        line = f.read()
        index_ini = line.index('seed = ') + 7
        index_fin = line.index('\nset p')
        current_seed = line[index_ini:index_fin]

    with open(model, 'r') as f:
        new_file = f.read().replace(str(current_seed), str(i))
        print(line)
        print(f'New seed: {i}')
        print(f'New file: {new_file}')

    with open(path_new + '/generate_Jpsi_3S11_ccbar_seed_' + str(i) + '.ho', 'w') as f:
            f.write(new_file)

