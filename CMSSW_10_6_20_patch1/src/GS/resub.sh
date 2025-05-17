i=1000

while [ $i -gt 0 ]
do
  crab status -d crab_projects/crab_GS_jpsi_ccbar_3FS_4FS_SPS_2017_13TeV_09-08-2022
  crab resubmit crab_projects/crab_GS_jpsi_ccbar_3FS_4FS_SPS_2017_13TeV_09-08-2022
  sleep 500
  echo left: $i
  ((i--))
done
