############################################################ To edit ############################################################

main='/store/group/uerj/mabarros/CRAB_PrivateMC_RunII_UL_SPS_2017/jpsi_ccbar_3s11_SPS_2017_13TeV/220603_034455'
rad='test_SPS_13TeV_DR_' #Jpsi_60to100_Dstar_DPS_2016posVFP_13TeV_GS_
sv='jpsi_ccbar_3s11_SPS_2017_13TeV.txt' #Jpsi_60to100_Dstar_DPS_2016posVFP_13TeV

############################################################ End editing ########################################################

echo 'Taking paths...'

xrdfs xrootd-redir.ultralight.org ls -u $main/0000 > a0.txt

echo 'Done :)'
echo 'Correcting paths...'

python3 correct_path.py -t='a0.txt' -f=$rad

echo 'Making final txt file...'

cat a0.txt > $sv

rm a*.txt 

echo 'Finished'