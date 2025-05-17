import argparse

## Argparse section
parser = argparse.ArgumentParser(description='Correct Caltech paths')
parser.add_argument('-t','--txt_file', type=str, help='The name of the text file you want to correct the paths. Ex: a0.txt, a1.txt.')
parser.add_argument('-f','--to_find', type=str, help='The cut of the name of the root file you want to remove.')

args = parser.parse_args()

def correct_path(txt_file, to_find):

    '''
    Function used to exclude the additional files path created by xrdfs on Caltech Tier 2.
    
    Parameters:
    txt_file (str): The name of the text file you want to correct the paths. Ex: a0.txt, a1.txt.
    to_find (str): The cut of the name of the root file you want to remove. 

    Ex: if your file name is  Jpsi_25to45_Dstar_DPS_2018_13TeV_GS_22.root, then to_find is Jpsi_25to45_Dstar_DPS_2018_13TeV_GS_.

    To Run: python3 a0.txt -f Jpsi_25to45_Dstar_DPS_2018_13TeV_GS_
    '''

    # Opens the file just to read the lines.
    with open(txt_file) as f1:
        # Stores the txt content in a list.
        lines = f1.readlines()
        # Takes the first element of the list.
        first = lines[0]
        # Takes the root file name of the first element
        name = first.find(to_find)
        root_name = first[name:]
        # Loop over the txt file content.
        count = 0
        for i in lines:
            # Takes each root file inside the loop
            n = i.find(to_find)
            rn = i[n:]
            # if the root file has the same name as the first we store it enters in the condition
            if rn == root_name:
                count = count + 1
                # if it is the second time, then it stores the position of the first element that repeats in the txt file.
                if count == 2:
                    # Variable to store the position
                    position = lines.index(i)
    # Open the file again in order to write on it
    with open(txt_file, 'w') as f2: 
        # Takes the list with non-repeateble paths
        new_lines = lines[0:position]
    
        # Loop on the it and creates the new file
        for new in new_lines:
            
            f2.write(new)



if __name__ == '__main__':
    correct_path(args.txt_file, args.to_find)

#'Jpsi_50to100_Dstar_DPS_13TeV_DR_'
