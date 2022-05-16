import random


def change_seed(real_file):
    import numpy as np

    seed_list = np.loadtxt('seeds.dat')
    print(f'Loading seed list.')
 
    with open(real_file, 'r') as f:
        line = f.read()
        current_seed = line[:line.index(' ')]
        print(f'Current seed: {current_seed}')
        print(f'Current file: {line}')

    #n = random.randint(0, 1000)
    a = current_seed
    while int(a) in seed_list:
        print(f'Current seed is in the seed list, finding another one')
        n = random.randint(0, 1000)
        a = str(n)
    if int(a) not in seed_list:
        seed_list = np.append(seed_list, int(a))
        print(f'Seed {a} appended to the seed list')
        print(f'The new list is: {seed_list}')

    with open(real_file, 'r') as f:
        new_file = f.read().replace(str(current_seed), a)
        print(f'New seed: {a}')
        print(f'New file: {new_file}')

    with open(real_file, 'w') as f:
            f.write(new_file)

    np.savetxt('seeds.dat', seed_list)
   

if __name__ == '__main__':

    real_file = '/afs/cern.ch/work/m/mabarros/public/MonteCarlo/SPS/condor_sps/HELAC-Onia-2.7.1/input/seed.input'
    change_seed(real_file)
