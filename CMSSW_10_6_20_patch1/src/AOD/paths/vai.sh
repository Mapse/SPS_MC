############################################################ To edit ############################################################

main='/store/group/uerj/mabarros/CRAB_PrivateMC_RunII_UL_SPS_2017/jpsi_ccbar_3FS_4FS_SPS_2017_13TeV/220812_032736'
rad='test_SPS_13TeV_HLT_' #Jpsi_60to100_Dstar_DPS_2016posVFP_13TeV_GS_
sv='jpsi_ccbar_3FS_4FS_SPS_2017_13TeV.txt' #Jpsi_60to100_Dstar_DPS_2016posVFP_13TeV

############################################################ End editing ########################################################

echo 'Taking paths...'

xrdfs xrootd-redir.ultralight.org ls -u $main/0000 > a0.txt
xrdfs xrootd-redir.ultralight.org ls -u $main/0001 > a1.txt
xrdfs xrootd-redir.ultralight.org ls -u $main/0002 > a2.txt
xrdfs xrootd-redir.ultralight.org ls -u $main/0003 > a3.txt
xrdfs xrootd-redir.ultralight.org ls -u $main/0004 > a4.txt
xrdfs xrootd-redir.ultralight.org ls -u $main/0005 > a5.txt
xrdfs xrootd-redir.ultralight.org ls -u $main/0006 > a6.txt

echo 'Done :)'
echo 'Correcting paths...'

python3 correct_path.py -t='a0.txt' -f=$rad
python3 correct_path.py -t='a1.txt' -f=$rad
python3 correct_path.py -t='a2.txt' -f=$rad
python3 correct_path.py -t='a3.txt' -f=$rad
python3 correct_path.py -t='a4.txt' -f=$rad
python3 correct_path.py -t='a5.txt' -f=$rad
python3 correct_path.py -t='a6.txt' -f=$rad

echo 'Making final txt file...'

cat a0.txt a1.txt a2.txt a3.txt a4.txt a5.txt a6.txt > $sv

rm a*.txt 

echo 'Finished'
