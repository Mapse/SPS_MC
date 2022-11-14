#Get the output
py_gs=Jpsi_30to50_Dstar_SPS_2017_13TeV_GS_cfg.py
# For MC Gen parameters
REPORT_NAME=Jpsi_30to50_Dstar_SPS_2017_13TeV_report.xml
#cmsRun -e -j $REPORT_NAME $py_gs

#Get parameters from xml output
processedEvents=$(grep -Po "(?<=<Metric Name=\"NumberEvents\" Value=\")(.*)(?=\"/>)" $REPORT_NAME | tail -n 1)
producedEvents=$(grep -Po "(?<=<TotalEvents>)(\d*)(?=</TotalEvents>)" $REPORT_NAME | tail -n 1)
threads=$(grep -Po "(?<=<Metric Name=\"NumberOfThreads\" Value=\")(.*)(?=\"/>)" $REPORT_NAME | tail -n 1)
totalJobTime=$(grep -Po "(?<=<Metric Name=\"TotalJobTime\" Value=\")(.*)(?=\"/>)" $REPORT_NAME | tail -n 1)
totalJobCPU=$(grep -Po "(?<=<Metric Name=\"TotalJobCPU\" Value=\")(.*)(?=\"/>)" $REPORT_NAME | tail -n 1)
eventThroughput=$(grep -Po "(?<=<Metric Name=\"EventThroughput\" Value=\")(.*)(?=\"/>)" $REPORT_NAME | tail -n 1)
avgEventTime=$(grep -Po "(?<=<Metric Name=\"AvgEventTime\" Value=\")(.*)(?=\"/>)" $REPORT_NAME | tail -n 1)

#Get final numbers
#CPU efficiency = (totalJobCPU * 100) / (threads * totalJobTime)# [%]
#Size per event = totalSize * 1024 / producedEvents# [kB]
#Time per event = 1 /eventThroughput# [s]
#Filter efficiency = producedEvents/processedEvents

#Get events per lumi section
#Events per lumi section = 8*3600*totalEfficiency/timePerEvent
#where total_efficiency=filterEfficiency*matchingEfficiency (matchingEfficiency usually 1 for BPH requests)

echo $processedEvents
echo $producedEvents
echo $threads

echo $eventThroughput
echo $totalJobTime
echo $totalJobCPU
echo $avgEventTime
